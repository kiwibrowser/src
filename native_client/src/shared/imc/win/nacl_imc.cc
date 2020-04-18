/*
 * Copyright (c) 2012 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


/* NaCl inter-module communication primitives. */

/* Disables the generation of the min and max macro in <windows.h> */
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <algorithm>
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <string>
#include <windows.h>
#include <sys/types.h>

#include "native_client/src/include/atomic_ops.h"
#include "native_client/src/include/portability.h"
#include "native_client/src/shared/imc/nacl_imc_c.h"


/* Duplicate a Windows HANDLE within the current process. */
NaClHandle NaClDuplicateNaClHandle(NaClHandle handle) {
  NaClHandle dup_handle;
  if (DuplicateHandle(GetCurrentProcess(),
                      handle,
                      GetCurrentProcess(),
                      &dup_handle,
                      0,
                      FALSE,
                      DUPLICATE_SAME_ACCESS)) {
    return dup_handle;
  } else {
    return NACL_INVALID_HANDLE;
  }
}

/*
 * This prefix used to be appended to pipe names for pipes
 * created in BoundSocket. We keep it for backward compatibility.
 * TODO(gregoryd): implement versioning support
 */
static const char kOldPipePrefix[] = "\\\\.\\pipe\\google-nacl-";
static const char kPipePrefix[] = "\\\\.\\pipe\\chrome.nacl.";

static const size_t kPipePrefixSize =
    sizeof kPipePrefix / sizeof kPipePrefix[0];

static const int kPipePathMax = kPipePrefixSize + NACL_PATH_MAX + 1;
static const int kOutBufferSize = 4096;  /* TBD */
static const int kInBufferSize = 4096;   /* TBD */
static const int kDefaultTimeoutMilliSeconds = 1000;

/* ControlHeader::command */
static const int kEchoRequest = 0;
static const int kEchoResponse = 1;
static const int kMessage = 2;
static const int kCancel = 3;   /* Cancels Handle transfer operations */

struct ControlHeader {
  int command;
  DWORD pid;
  uint32_t message_length;
  uint32_t handle_count;
};

/*
 * TODO(gregoryd): a similar function exists in Chrome's base, but we cannot
 * use it here since it cannot be built with scons.
 */
static std::wstring ASCIIToWide(const char* ascii) {
  return std::wstring(ascii, &ascii[strlen(ascii)]);
}

static bool GetSocketName(const NaClSocketAddress* address, char* name) {
  if (address == NULL || !isprint(address->path[0])) {
    SetLastError(ERROR_INVALID_PARAMETER);
    return false;
  }
  sprintf_s(name, kPipePathMax, "%s%.*s",
    kPipePrefix, NACL_PATH_MAX, address->path);
  return true;
}

static bool GetSocketNameWithOldPrefix(const NaClSocketAddress* address,
                                       char* name) {
  if (address == NULL || !isprint(address->path[0])) {
    SetLastError(ERROR_INVALID_PARAMETER);
    return false;
  }
  sprintf_s(name, kPipePathMax, "%s%.*s",
            kOldPipePrefix, NACL_PATH_MAX, address->path);
  return true;
}

static int ReadAll(HANDLE handle, void* buffer, size_t length) {
  size_t count = 0;
  while (count < length) {
    DWORD len;
    DWORD chunk = (DWORD) (
      ((length - count) <= UINT_MAX) ? (length - count) : UINT_MAX);
    if (ReadFile(handle, (char *) buffer + count,
                 chunk, &len, NULL) == FALSE) {
      return (int) ((0 < count) ? count : -1);
    }
    count += len;
  }
  return (int) count;
}

static int WriteAll(HANDLE handle, const void* buffer, size_t length) {
  size_t count = 0;
  while (count < length) {
    DWORD len;
    /* The following statement is for the 64 bit portability. */
    DWORD chunk = (DWORD) (
      ((length - count) <= UINT_MAX) ? (length - count) : UINT_MAX);
    if (WriteFile(handle, (const char *) buffer + count,
                  chunk, &len, NULL) == FALSE) {
      return (int) ((0 < count) ? count : -1);
    }
    count += len;
  }
  return (int) count;
}

static BOOL SkipFile(HANDLE handle, size_t length) {
  while (0 < length) {
    char scratch[1024];
    size_t count = std::min(sizeof scratch, length);
    if (ReadAll(handle, scratch, count) != (int) count) {
      return FALSE;
    }
    length -= count;
  }
  return TRUE;
}

static BOOL SkipHandles(HANDLE handle, size_t count) {
  while (0 < count) {
    uint64_t discard;
    if (ReadAll(handle, &discard, sizeof discard) != sizeof discard) {
      return FALSE;
    }
    CloseHandle((HANDLE) discard);
    --count;
  }
  return TRUE;
}

int NaClWouldBlock() {
  return GetLastError() == ERROR_PIPE_LISTENING;
}

NaClHandle NaClBoundSocket(const NaClSocketAddress* address) {
  char name[kPipePathMax];
  if (!GetSocketName(address, name)) {
    return NACL_INVALID_HANDLE;
  }
  /* Create a named pipe in nonblocking mode. */
  return CreateNamedPipeW(
      ASCIIToWide(name).c_str(),
      PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE,
      PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_NOWAIT,
      PIPE_UNLIMITED_INSTANCES,
      kOutBufferSize,
      kInBufferSize,
      kDefaultTimeoutMilliSeconds,
      NULL);
}

int NaClSocketPair(NaClHandle pair[2]) {
  static Atomic32 socket_pair_count;
  char name[kPipePathMax];

  do {
    sprintf_s(name, kPipePathMax, "%s%u.%lu",
              kPipePrefix, GetCurrentProcessId(),
              AtomicIncrement(&socket_pair_count, 1));
    pair[0] = CreateNamedPipeW(
        ASCIIToWide(name).c_str(),
        PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE,
        1,
        kOutBufferSize,
        kInBufferSize,
        kDefaultTimeoutMilliSeconds,
        NULL);
    if (pair[0] == INVALID_HANDLE_VALUE &&
        GetLastError() != ERROR_ACCESS_DENIED &&
        GetLastError() != ERROR_PIPE_BUSY) {
      return -1;
    }
  } while (pair[0] == INVALID_HANDLE_VALUE);
  pair[1] = CreateFileW(ASCIIToWide(name).c_str(),
                        GENERIC_READ | GENERIC_WRITE,
                        0,              /* no sharing */
                        NULL,           /* default security attributes */
                        OPEN_EXISTING,  /* opens existing pipe */
                        SECURITY_SQOS_PRESENT | SECURITY_IDENTIFICATION,
                        NULL);          /* no template file */
  if (pair[1] == INVALID_HANDLE_VALUE) {
    CloseHandle(pair[0]);
    return -1;
  }
  if (ConnectNamedPipe(pair[0], NULL) == FALSE) {
    DWORD error = GetLastError();
    if (error != ERROR_PIPE_CONNECTED) {
      CloseHandle(pair[0]);
      CloseHandle(pair[1]);
      return -1;
    }
  }
  return 0;
}

int NaClClose(NaClHandle handle) {
  if (handle == NULL || handle == INVALID_HANDLE_VALUE) {
    return 0;
  }
  return CloseHandle(handle) ? 0 : -1;
}

int NaClSendDatagram(NaClHandle handle, const NaClMessageHeader* message,
                     int flags) {
  ControlHeader header = { kEchoRequest, GetCurrentProcessId(), 0, 0 };
  uint64_t remote_handles[NACL_HANDLE_COUNT_MAX];
  uint32_t i;

  if (NACL_HANDLE_COUNT_MAX < message->handle_count) {
    SetLastError(ERROR_INVALID_PARAMETER);
    return -1;
  }
  if (!NaClMessageSizeIsValid(message)) {
    SetLastError(ERROR_INVALID_PARAMETER);
    return -1;
  }
  if (0 < message->handle_count && message->handles) {
    HANDLE target = NULL;
    /*
     * TODO(shiki): On Windows Vista, we can use GetNamedPipeClientProcessId()
     * and GetNamedPipeServerProcessId() and probably we can remove
     * kEchoRequest and kEchoResponse completely.
     */
    if (WriteAll(handle, &header, sizeof header) != sizeof header ||
        ReadAll(handle, &header, sizeof header) != sizeof header ||
        header.command != kEchoResponse) {
      return -1;
    }
    target = OpenProcess(PROCESS_DUP_HANDLE, FALSE, header.pid);
    if (target == NULL) {
      return -1;
    }
    for (i = 0; i < message->handle_count; ++i) {
      HANDLE temp_remote_handle;
      bool success = DuplicateHandle(GetCurrentProcess(), message->handles[i],
                                     target, &temp_remote_handle,
                                     0, FALSE, DUPLICATE_SAME_ACCESS) != 0;
      if (!success) {
        /*
         * Send the kCancel message to revoke the handles duplicated
         * so far in the remote peer.
         */
        header.command = kCancel;
        header.handle_count = i;
        if (0 < i) {
          WriteAll(handle, &header, sizeof header);
          WriteAll(handle, remote_handles, sizeof(uint64_t) * i);
        }
        CloseHandle(target);
        return -1;
      }
      remote_handles[i] = (uint64_t) temp_remote_handle;
    }
    CloseHandle(target);
  }
  header.command = kMessage;
  header.handle_count = message->handle_count;
  for (i = 0; i < message->iov_length; ++i) {
    if (UINT32_MAX - header.message_length < message->iov[i].length) {
      return -1;
    }
    header.message_length += (uint32_t) message->iov[i].length;
  }
  if (WriteAll(handle, &header, sizeof header) != sizeof header) {
    return -1;
  }
  for (i = 0; i < message->iov_length; ++i) {
    if (WriteAll(handle, message->iov[i].base, message->iov[i].length) !=
        (int) message->iov[i].length) {
      return -1;
    }
  }
  if (0 < message->handle_count && message->handles &&
      WriteAll(handle,
               remote_handles,
               sizeof(uint64_t) * message->handle_count) !=
      (int) (sizeof(uint64_t) * message->handle_count)) {
    return -1;
  }
  return (int) header.message_length;
}

int NaClSendDatagramTo(const NaClMessageHeader* message, int flags,
                       const NaClSocketAddress* name) {
  NaClHandle handle;
  char pipe_name[kPipePathMax];
  int timeout_ms;
  int result;

  if (NACL_HANDLE_COUNT_MAX < message->handle_count) {
    SetLastError(ERROR_INVALID_PARAMETER);
    return -1;
  }
  if (!NaClMessageSizeIsValid(message)) {
    SetLastError(ERROR_INVALID_PARAMETER);
    return -1;
  }
  if (!GetSocketName(name, pipe_name)) {
    return -1;
  }
  timeout_ms = 10;
  for (;;) {
    handle = CreateFileW(ASCIIToWide(pipe_name).c_str(),
                         GENERIC_READ | GENERIC_WRITE,
                         0,              /* no sharing */
                         NULL,           /* default security attributes */
                         OPEN_EXISTING,  /* opens existing pipe */
                         SECURITY_SQOS_PRESENT | SECURITY_IDENTIFICATION,
                         NULL);          /* no template file */
    if (handle != INVALID_HANDLE_VALUE) {
      break;
    }

    /* If the pipe is busy it means it exists, so we can try and wait. */
    if (GetLastError() != ERROR_PIPE_BUSY) {
      if (GetLastError() != ERROR_FILE_NOT_FOUND) {
        return -1;
      } else {
        /*
         * Try to find the file using name with prefix (can be created
         * by an old version of IMC library.
         */
        if (!GetSocketNameWithOldPrefix(name, pipe_name)) {
          return -1;
        }
        handle = CreateFileW(ASCIIToWide(pipe_name).c_str(),
                             GENERIC_READ | GENERIC_WRITE,
                             0,              /* no sharing */
                             NULL,           /* default security attributes */
                             OPEN_EXISTING,  /* opens existing pipe */
                             SECURITY_SQOS_PRESENT | SECURITY_IDENTIFICATION,
                             NULL);          /* no template file */
        if (handle != INVALID_HANDLE_VALUE) {
          break;
        }
        if (GetLastError() != ERROR_PIPE_BUSY) {
          /* Could not find the pipe - nothing to do */
          return -1;
        }
      }
      break;
    }
    if (flags & NACL_DONT_WAIT) {
      SetLastError(ERROR_PIPE_LISTENING);
      return -1;
    }
    /* Cannot call WaitNamedPipe here because it's blocked by Chrome sandbox. */
    Sleep(timeout_ms);
    timeout_ms *= 2;
    if (timeout_ms > kDefaultTimeoutMilliSeconds) {
      timeout_ms = kDefaultTimeoutMilliSeconds;
    }
  }
  result = NaClSendDatagram(handle, message, flags);
  CloseHandle(handle);
  return result;
}

static int ReceiveDatagram(NaClHandle handle, NaClMessageHeader* message,
                           int flags, bool bound_socket) {
  ControlHeader header;
  int result = -1;
  bool dontPeek = false;
  uint32_t i;
 Repeat:
  if ((flags & NACL_DONT_WAIT) && !dontPeek) {
    DWORD len;
    DWORD total;
    if (PeekNamedPipe(handle, &header, sizeof header, &len, &total, NULL)) {
      if (len < sizeof header) {
        SetLastError(ERROR_PIPE_LISTENING);
      } else {
        switch (header.command) {
        case kEchoRequest:
          /*
           * Send back the process id to the remote peer to duplicate handles.
           * TODO(shiki) : It might be better to keep remote pid by the initial
           *               handshake rather than send kEchoRequest each time
           *               before duplicating handles.
           */
          if (ReadAll(handle, &header, sizeof header) == sizeof header) {
            header.command = kEchoResponse;
            header.pid = GetCurrentProcessId();
            WriteAll(handle, &header, sizeof header);
            if (bound_socket) {
              /* We must not close this connection. */
              dontPeek = true;
            }
            goto Repeat;
          }
          break;
        case kEchoResponse:
          SkipFile(handle, sizeof header);
          goto Repeat;
          break;
        case kMessage:
          if (header.message_length + sizeof header <= total) {
            if (flags & NACL_DONT_WAIT) {
              flags &= ~NACL_DONT_WAIT;
              goto Repeat;
            }
            result = (int) header.message_length;
          } else {
            SetLastError(ERROR_PIPE_LISTENING);
          }
          break;
        case kCancel:
          if (sizeof header + sizeof(uint64_t) * header.handle_count <= len &&
              ReadAll(handle, &header, sizeof header) == sizeof header) {
            SkipHandles(handle, header.handle_count);
            goto Repeat;
          }
          break;
        default:
          break;
        }
      }
    }
  } else if (ReadAll(handle, &header, sizeof header) == sizeof header) {
    dontPeek = false;
    switch (header.command) {
    case kEchoRequest:
      header.command = kEchoResponse;
      header.pid = GetCurrentProcessId();
      WriteAll(handle, &header, sizeof header);
      goto Repeat;
      break;
    case kEchoResponse:
      goto Repeat;
      break;
    case kMessage: {
      uint32_t total_message_bytes = header.message_length;
      size_t count = 0;
      message->flags = 0;
      for (i = 0;
           i < message->iov_length && count < header.message_length;
           ++i) {
        NaClIOVec* iov = &message->iov[i];
        uint32_t len = std::min((uint32_t) iov->length, total_message_bytes);
        if (ReadAll(handle, iov->base, len) != (int) len) {
          break;
        }
        total_message_bytes -= len;
        count += len;
      }
      if (count < header.message_length) {
        if (SkipFile(handle, header.message_length - count) == FALSE) {
          break;
        }
        message->flags |= NACL_MESSAGE_TRUNCATED;
      }
      if (0 < message->handle_count && message->handles) {
        uint64_t received_handles[NACL_HANDLE_COUNT_MAX];
        message->handle_count = std::min(message->handle_count,
                                         header.handle_count);
        if (ReadAll(handle, received_handles,
                    message->handle_count * sizeof(uint64_t)) !=
            (int) (message->handle_count * sizeof(uint64_t))) {
          break;
        }
        for (i = 0; i < message->handle_count; ++i) {
          message->handles[i] = (HANDLE) received_handles[i];
        }
      } else {
        message->handle_count = 0;
      }
      if (message->handle_count < header.handle_count) {
        if (SkipHandles(handle, header.handle_count - message->handle_count) ==
            FALSE) {
          break;
        }
        message->flags |= NACL_HANDLES_TRUNCATED;
      }
      result = (int) count;
      break;
    }
    case kCancel:
      SkipHandles(handle, header.handle_count);
      goto Repeat;
      break;
    default:
      break;
    }
  }
  return result;
}

int NaClReceiveDatagram(NaClHandle handle, NaClMessageHeader* message,
                        int flags) {
  DWORD state;
  if (!NaClMessageSizeIsValid(message)) {
    SetLastError(ERROR_INVALID_PARAMETER);
    return -1;
  }

  /*
   * If handle is a bound socket, it is a named pipe in non-blocking mode.
   * Set is_bound_socket to true if handle has been created by BoundSocket().
   */
  if (!GetNamedPipeHandleState(handle, &state, NULL, NULL, NULL, NULL, NULL)) {
    return -1;
  }

  if (!(state & PIPE_NOWAIT)) {
    /* handle is a connected socket. */
    return ReceiveDatagram(handle, message, flags, false);
  }

  /* handle is a bound socket. */
  for (;;) {
    if (ConnectNamedPipe(handle, NULL)) {
      /*
       * Note ConnectNamedPipe() for a handle in non-blocking mode returns a
       * nonzero value just to indicate that the pipe is now available to be
       * connected.
       */
      continue;
    }
    switch (GetLastError()) {
    case ERROR_PIPE_LISTENING: {
      /* Set handle to blocking mode */
      DWORD mode = PIPE_READMODE_BYTE | PIPE_WAIT;
      if (flags & NACL_DONT_WAIT) {
        return -1;
      }
      SetNamedPipeHandleState(handle, &mode, NULL, NULL);
      break;
    }
    case ERROR_PIPE_CONNECTED: {
      /* Set handle to blocking mode */
      DWORD mode = PIPE_READMODE_BYTE | PIPE_WAIT;
      int result;
      SetNamedPipeHandleState(handle, &mode, NULL, NULL);
      result = ReceiveDatagram(handle, message, flags, true);
      FlushFileBuffers(handle);
      /* Set handle back to non-blocking mode. */
      mode = PIPE_READMODE_BYTE | PIPE_NOWAIT;
      SetNamedPipeHandleState(handle, &mode, NULL, NULL);
      DisconnectNamedPipe(handle);
      if (result == -1 && GetLastError() == ERROR_BROKEN_PIPE) {
        if (flags & NACL_DONT_WAIT) {
          SetLastError(ERROR_PIPE_LISTENING);
          return result;
        }
      } else {
        return result;
      }
      break;
    }
    default:
      return -1;
      break;
    }
  }
}
