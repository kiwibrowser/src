// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "sandbox/linux/services/libc_interceptor.h"

#include <dlfcn.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <set>
#include <string>

#include "base/lazy_instance.h"
#include "base/memory/protected_memory.h"
#include "base/memory/protected_memory_cfi.h"
#include "base/pickle.h"
#include "base/posix/eintr_wrapper.h"
#include "base/posix/global_descriptors.h"
#include "base/posix/unix_domain_socket.h"
#include "base/synchronization/lock.h"

namespace sandbox {

namespace {

// The global |g_am_zygote_or_renderer| is true iff we are in a zygote or
// renderer process. It's set in ZygoteMain and inherited by the renderers when
// they fork. (This means that it'll be incorrect for global constructor
// functions and before ZygoteMain is called - beware).
bool g_am_zygote_or_renderer = false;
bool g_use_localtime_override = true;
int g_backchannel_fd = -1;

base::LazyInstance<std::set<std::string>>::Leaky g_timezones =
    LAZY_INSTANCE_INITIALIZER;

base::LazyInstance<base::Lock>::Leaky g_timezones_lock =
    LAZY_INSTANCE_INITIALIZER;

bool ReadTimeStruct(base::PickleIterator* iter,
                    struct tm* output,
                    char* timezone_out,
                    size_t timezone_out_len) {
  int result;
  if (!iter->ReadInt(&result))
    return false;
  output->tm_sec = result;
  if (!iter->ReadInt(&result))
    return false;
  output->tm_min = result;
  if (!iter->ReadInt(&result))
    return false;
  output->tm_hour = result;
  if (!iter->ReadInt(&result))
    return false;
  output->tm_mday = result;
  if (!iter->ReadInt(&result))
    return false;
  output->tm_mon = result;
  if (!iter->ReadInt(&result))
    return false;
  output->tm_year = result;
  if (!iter->ReadInt(&result))
    return false;
  output->tm_wday = result;
  if (!iter->ReadInt(&result))
    return false;
  output->tm_yday = result;
  if (!iter->ReadInt(&result))
    return false;
  output->tm_isdst = result;
  if (!iter->ReadInt(&result))
    return false;
  output->tm_gmtoff = result;

  std::string timezone;
  if (!iter->ReadString(&timezone))
    return false;
  if (timezone_out_len) {
    const size_t copy_len = std::min(timezone_out_len - 1, timezone.size());
    memcpy(timezone_out, timezone.data(), copy_len);
    timezone_out[copy_len] = 0;
    output->tm_zone = timezone_out;
  } else {
    base::AutoLock lock(g_timezones_lock.Get());
    auto ret_pair = g_timezones.Get().insert(timezone);
    output->tm_zone = ret_pair.first->c_str();
  }

  return true;
}

void WriteTimeStruct(base::Pickle* pickle, const struct tm* time) {
  pickle->WriteInt(time->tm_sec);
  pickle->WriteInt(time->tm_min);
  pickle->WriteInt(time->tm_hour);
  pickle->WriteInt(time->tm_mday);
  pickle->WriteInt(time->tm_mon);
  pickle->WriteInt(time->tm_year);
  pickle->WriteInt(time->tm_wday);
  pickle->WriteInt(time->tm_yday);
  pickle->WriteInt(time->tm_isdst);
  pickle->WriteInt(time->tm_gmtoff);
  pickle->WriteString(time->tm_zone);
}

// See
// https://chromium.googlesource.com/chromium/src/+/master/docs/linux_zygote.md
void ProxyLocaltimeCallToBrowser(time_t input,
                                 struct tm* output,
                                 char* timezone_out,
                                 size_t timezone_out_len) {
  base::Pickle request;
  request.WriteInt(METHOD_LOCALTIME);
  request.WriteString(
      std::string(reinterpret_cast<char*>(&input), sizeof(input)));

  memset(output, 0, sizeof(struct tm));

  uint8_t reply_buf[512];
  const ssize_t r = base::UnixDomainSocket::SendRecvMsg(
      g_backchannel_fd, reply_buf, sizeof(reply_buf), nullptr, request);
  if (r == -1)
    return;

  base::Pickle reply(reinterpret_cast<char*>(reply_buf), r);
  base::PickleIterator iter(reply);
  if (!ReadTimeStruct(&iter, output, timezone_out, timezone_out_len)) {
    memset(output, 0, sizeof(struct tm));
  }
}

// The other side of this call is ProxyLocaltimeCallToBrowser().
bool HandleLocalTime(int fd,
                     base::PickleIterator iter,
                     const std::vector<base::ScopedFD>& fds) {
  std::string time_string;
  if (!iter.ReadString(&time_string) || time_string.size() != sizeof(time_t))
    return false;

  time_t time;
  memcpy(&time, time_string.data(), sizeof(time));
  // We use |localtime| here because we need the |tm_zone| field to be filled
  // out. Since we are a single-threaded process, this is safe.
  const struct tm* expanded_time = localtime(&time);

  base::Pickle reply;
  if (expanded_time) {
    WriteTimeStruct(&reply, expanded_time);
  } else {
    // The {} constructor ensures the struct is 0-initialized.
    struct tm zeroed_time = {};
    WriteTimeStruct(&reply, &zeroed_time);
  }

  struct msghdr msg;
  memset(&msg, 0, sizeof(msg));

  struct iovec iov = {const_cast<void*>(reply.data()), reply.size()};
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

  if (HANDLE_EINTR(sendmsg(fds[0].get(), &msg, MSG_DONTWAIT)) < 0)
    PLOG(ERROR) << "sendmsg";

  return true;
}

}  // namespace

typedef struct tm* (*LocaltimeFunction)(const time_t* timep);
typedef struct tm* (*LocaltimeRFunction)(const time_t* timep,
                                         struct tm* result);
struct LibcFunctions {
  LocaltimeFunction localtime;
  LocaltimeFunction localtime64;
  LocaltimeRFunction localtime_r;
  LocaltimeRFunction localtime64_r;
};

static pthread_once_t g_libc_funcs_guard = PTHREAD_ONCE_INIT;
// The libc function pointers are stored in read-only memory after being
// dynamically resolved as a security mitigation to prevent the pointer from
// being tampered with. See https://crbug.com/771365 for details.
static PROTECTED_MEMORY_SECTION base::ProtectedMemory<LibcFunctions>
    g_libc_funcs;

static void InitLibcLocaltimeFunctions() {
  auto writer = base::AutoWritableMemory::Create(g_libc_funcs);
  g_libc_funcs->localtime =
      reinterpret_cast<LocaltimeFunction>(dlsym(RTLD_NEXT, "localtime"));
  g_libc_funcs->localtime64 =
      reinterpret_cast<LocaltimeFunction>(dlsym(RTLD_NEXT, "localtime64"));
  g_libc_funcs->localtime_r =
      reinterpret_cast<LocaltimeRFunction>(dlsym(RTLD_NEXT, "localtime_r"));
  g_libc_funcs->localtime64_r =
      reinterpret_cast<LocaltimeRFunction>(dlsym(RTLD_NEXT, "localtime64_r"));

  if (!g_libc_funcs->localtime || !g_libc_funcs->localtime_r) {
    // http://code.google.com/p/chromium/issues/detail?id=16800
    //
    // Nvidia's libGL.so overrides dlsym for an unknown reason and replaces
    // it with a version which doesn't work. In this case we'll get a NULL
    // result. There's not a lot we can do at this point, so we just bodge it!
    LOG(ERROR) << "Your system is broken: dlsym doesn't work! This has been "
                  "reported to be caused by Nvidia's libGL. You should expect"
                  " time related functions to misbehave. "
                  "http://code.google.com/p/chromium/issues/detail?id=16800";
  }

  if (!g_libc_funcs->localtime)
    g_libc_funcs->localtime = gmtime;
  if (!g_libc_funcs->localtime64)
    g_libc_funcs->localtime64 = g_libc_funcs->localtime;
  if (!g_libc_funcs->localtime_r)
    g_libc_funcs->localtime_r = gmtime_r;
  if (!g_libc_funcs->localtime64_r)
    g_libc_funcs->localtime64_r = g_libc_funcs->localtime_r;
}

// Define localtime_override() function with asm name "localtime", so that all
// references to localtime() will resolve to this function. Notice that we need
// to set visibility attribute to "default" to export the symbol, as it is set
// to "hidden" by default in chrome per build/common.gypi.
__attribute__((__visibility__("default"))) struct tm* localtime_override(
    const time_t* timep) __asm__("localtime");

__attribute__((__visibility__("default"))) struct tm* localtime_override(
    const time_t* timep) {
  if (g_am_zygote_or_renderer && g_use_localtime_override) {
    static struct tm time_struct;
    static char timezone_string[64];
    ProxyLocaltimeCallToBrowser(*timep, &time_struct, timezone_string,
                                sizeof(timezone_string));
    return &time_struct;
  }

  CHECK_EQ(0, pthread_once(&g_libc_funcs_guard, InitLibcLocaltimeFunctions));
  struct tm* res =
      base::UnsanitizedCfiCall(g_libc_funcs, &LibcFunctions::localtime)(timep);
#if defined(MEMORY_SANITIZER)
  if (res)
    __msan_unpoison(res, sizeof(*res));
  if (res->tm_zone)
    __msan_unpoison_string(res->tm_zone);
#endif
  return res;
}

// Use same trick to override localtime64(), localtime_r() and localtime64_r().
__attribute__((__visibility__("default"))) struct tm* localtime64_override(
    const time_t* timep) __asm__("localtime64");

__attribute__((__visibility__("default"))) struct tm* localtime64_override(
    const time_t* timep) {
  if (g_am_zygote_or_renderer && g_use_localtime_override) {
    static struct tm time_struct;
    static char timezone_string[64];
    ProxyLocaltimeCallToBrowser(*timep, &time_struct, timezone_string,
                                sizeof(timezone_string));
    return &time_struct;
  }

  CHECK_EQ(0, pthread_once(&g_libc_funcs_guard, InitLibcLocaltimeFunctions));
  struct tm* res = base::UnsanitizedCfiCall(g_libc_funcs,
                                            &LibcFunctions::localtime64)(timep);
#if defined(MEMORY_SANITIZER)
  if (res)
    __msan_unpoison(res, sizeof(*res));
  if (res->tm_zone)
    __msan_unpoison_string(res->tm_zone);
#endif
  return res;
}

__attribute__((__visibility__("default"))) struct tm* localtime_r_override(
    const time_t* timep,
    struct tm* result) __asm__("localtime_r");

__attribute__((__visibility__("default"))) struct tm* localtime_r_override(
    const time_t* timep,
    struct tm* result) {
  if (g_am_zygote_or_renderer && g_use_localtime_override) {
    ProxyLocaltimeCallToBrowser(*timep, result, nullptr, 0);
    return result;
  }

  CHECK_EQ(0, pthread_once(&g_libc_funcs_guard, InitLibcLocaltimeFunctions));
  struct tm* res = base::UnsanitizedCfiCall(
      g_libc_funcs, &LibcFunctions::localtime_r)(timep, result);
#if defined(MEMORY_SANITIZER)
  if (res)
    __msan_unpoison(res, sizeof(*res));
  if (res->tm_zone)
    __msan_unpoison_string(res->tm_zone);
#endif
  return res;
}

__attribute__((__visibility__("default"))) struct tm* localtime64_r_override(
    const time_t* timep,
    struct tm* result) __asm__("localtime64_r");

__attribute__((__visibility__("default"))) struct tm* localtime64_r_override(
    const time_t* timep,
    struct tm* result) {
  if (g_am_zygote_or_renderer && g_use_localtime_override) {
    ProxyLocaltimeCallToBrowser(*timep, result, nullptr, 0);
    return result;
  }

  CHECK_EQ(0, pthread_once(&g_libc_funcs_guard, InitLibcLocaltimeFunctions));
  struct tm* res = base::UnsanitizedCfiCall(
      g_libc_funcs, &LibcFunctions::localtime64_r)(timep, result);
#if defined(MEMORY_SANITIZER)
  if (res)
    __msan_unpoison(res, sizeof(*res));
  if (res->tm_zone)
    __msan_unpoison_string(res->tm_zone);
#endif
  return res;
}

void SetUseLocaltimeOverride(bool enable) {
  g_use_localtime_override = enable;
}

void SetAmZygoteOrRenderer(bool enable, int backchannel_fd) {
  g_am_zygote_or_renderer = enable;
  g_backchannel_fd = backchannel_fd;
}

bool HandleInterceptedCall(int kind,
                           int fd,
                           base::PickleIterator iter,
                           const std::vector<base::ScopedFD>& fds) {
  if (kind != METHOD_LOCALTIME)
    return false;

  return HandleLocalTime(fd, iter, fds);
}

}  // namespace sandbox
