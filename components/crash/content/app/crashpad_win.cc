// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/crash/content/app/crashpad.h"

#include <memory>

#include "base/debug/crash_logging.h"
#include "base/environment.h"
#include "base/files/file_util.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/string16.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "components/crash/content/app/crash_export_thunks.h"
#include "components/crash/content/app/crash_reporter_client.h"
#include "components/crash/content/app/crash_switches.h"
#include "third_party/crashpad/crashpad/client/crashpad_client.h"
#include "third_party/crashpad/crashpad/client/crashpad_info.h"
#include "third_party/crashpad/crashpad/client/simulate_crash_win.h"

namespace crash_reporter {
namespace internal {

void GetPlatformCrashpadAnnotations(
    std::map<std::string, std::string>* annotations) {
  CrashReporterClient* crash_reporter_client = GetCrashReporterClient();
  wchar_t exe_file[MAX_PATH] = {};
  CHECK(::GetModuleFileName(nullptr, exe_file, arraysize(exe_file)));
  base::string16 product_name, version, special_build, channel_name;
  crash_reporter_client->GetProductNameAndVersion(
      exe_file, &product_name, &version, &special_build, &channel_name);
  (*annotations)["prod"] = base::UTF16ToUTF8(product_name);
  (*annotations)["ver"] = base::UTF16ToUTF8(version);
#if defined(GOOGLE_CHROME_BUILD)
  // Empty means stable.
  const bool allow_empty_channel = true;
#else
  const bool allow_empty_channel = false;
#endif
  if (allow_empty_channel || !channel_name.empty())
    (*annotations)["channel"] = base::UTF16ToUTF8(channel_name);
  if (!special_build.empty())
    (*annotations)["special"] = base::UTF16ToUTF8(special_build);
#if defined(ARCH_CPU_X86)
  (*annotations)["plat"] = std::string("Win32");
#elif defined(ARCH_CPU_X86_64)
  (*annotations)["plat"] = std::string("Win64");
#endif
}

base::FilePath PlatformCrashpadInitialization(bool initial_client,
                                              bool browser_process,
                                              bool embedded_handler,
                                              const std::string& user_data_dir,
                                              const base::FilePath& exe_path) {
  base::FilePath database_path;  // Only valid in the browser process.
  base::FilePath metrics_path;  // Only valid in the browser process.

  const char kPipeNameVar[] = "CHROME_CRASHPAD_PIPE_NAME";
  const char kServerUrlVar[] = "CHROME_CRASHPAD_SERVER_URL";
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  if (initial_client) {
    CrashReporterClient* crash_reporter_client = GetCrashReporterClient();

    base::string16 database_path_str;
    if (crash_reporter_client->GetCrashDumpLocation(&database_path_str))
      database_path = base::FilePath(database_path_str);

    base::string16 metrics_path_str;
    if (crash_reporter_client->GetCrashMetricsLocation(&metrics_path_str)) {
      metrics_path = base::FilePath(metrics_path_str);
      CHECK(base::CreateDirectoryAndGetError(metrics_path, nullptr));
    }

    std::map<std::string, std::string> process_annotations;
    GetPlatformCrashpadAnnotations(&process_annotations);

#if defined(GOOGLE_CHROME_BUILD)
    std::string url = "https://clients2.google.com/cr/report";
#else
    std::string url;
#endif

    // Allow the crash server to be overridden for testing. If the variable
    // isn't present in the environment then the default URL will remain.
    env->GetVar(kServerUrlVar, &url);

    base::FilePath exe_file(exe_path);
    if (exe_file.empty()) {
      wchar_t exe_file_path[MAX_PATH] = {};
      CHECK(::GetModuleFileName(nullptr, exe_file_path,
                                arraysize(exe_file_path)));

      exe_file = base::FilePath(exe_file_path);
    }

    if (crash_reporter_client->GetShouldDumpLargerDumps()) {
      const uint32_t kIndirectMemoryLimit = 4 * 1024 * 1024;
      crashpad::CrashpadInfo::GetCrashpadInfo()
          ->set_gather_indirectly_referenced_memory(
              crashpad::TriState::kEnabled, kIndirectMemoryLimit);
    }

    // If the handler is embedded in the binary (e.g. chrome, setup), we
    // reinvoke it with --type=crashpad-handler. Otherwise, we use the
    // standalone crashpad_handler.exe (for tests, etc.).
    std::vector<std::string> start_arguments;
    if (embedded_handler) {
      start_arguments.push_back(std::string("--type=") +
                                switches::kCrashpadHandler);
      if (!user_data_dir.empty()) {
        start_arguments.push_back(std::string("--user-data-dir=") +
                                  user_data_dir);
      }
      // The prefetch argument added here has to be documented in
      // chrome_switches.cc, below the kPrefetchArgument* constants. A constant
      // can't be used here because crashpad can't depend on Chrome.
      start_arguments.push_back("/prefetch:7");
    } else {
      base::FilePath exe_dir = exe_file.DirName();
      exe_file = exe_dir.Append(FILE_PATH_LITERAL("crashpad_handler.exe"));
    }

    std::vector<std::string> arguments(start_arguments);

    if (crash_reporter_client->ShouldMonitorCrashHandlerExpensively()) {
      arguments.push_back("--monitor-self");
      for (const std::string& start_argument : start_arguments) {
        arguments.push_back(std::string("--monitor-self-argument=") +
                            start_argument);
      }
    }

    // Set up --monitor-self-annotation even in the absence of --monitor-self so
    // that minidumps produced by Crashpad's generate_dump tool will contain
    // these annotations.
    arguments.push_back(std::string("--monitor-self-annotation=ptype=") +
                        switches::kCrashpadHandler);

    GetCrashpadClient().StartHandler(exe_file, database_path, metrics_path, url,
                                     process_annotations, arguments, false,
                                     false);

    // If we're the browser, push the pipe name into the environment so child
    // processes can connect to it. If we inherited another crashpad_handler's
    // pipe name, we'll overwrite it here.
    env->SetVar(kPipeNameVar,
                base::UTF16ToUTF8(GetCrashpadClient().GetHandlerIPCPipe()));
  } else {
    std::string pipe_name_utf8;
    if (env->GetVar(kPipeNameVar, &pipe_name_utf8)) {
      GetCrashpadClient().SetHandlerIPCPipe(base::UTF8ToUTF16(pipe_name_utf8));
    }
  }

  return database_path;
}

// We need to prevent ICF from folding DumpProcessForHungInputThread(),
// together, since that makes them indistinguishable in crash dumps.
// We do this by making the function body unique, and prevent optimization
// from shuffling things around.
MSVC_DISABLE_OPTIMIZE()
MSVC_PUSH_DISABLE_WARNING(4748)

DWORD WINAPI DumpProcessForHungInputThread(void* param) {
  DumpWithoutCrashing();
  return 0;
}

MSVC_POP_WARNING()
MSVC_ENABLE_OPTIMIZE()


#if defined(ARCH_CPU_X86_64)

static int CrashForExceptionInNonABICompliantCodeRange(
    PEXCEPTION_RECORD ExceptionRecord,
    ULONG64 EstablisherFrame,
    PCONTEXT ContextRecord,
    PDISPATCHER_CONTEXT DispatcherContext) {
  EXCEPTION_POINTERS info = { ExceptionRecord, ContextRecord };
  return CrashForException_ExportThunk(&info);
}

// See https://msdn.microsoft.com/en-us/library/ddssxxy8.aspx
typedef struct _UNWIND_INFO {
  unsigned char Version : 3;
  unsigned char Flags : 5;
  unsigned char SizeOfProlog;
  unsigned char CountOfCodes;
  unsigned char FrameRegister : 4;
  unsigned char FrameOffset : 4;
  ULONG ExceptionHandler;
} UNWIND_INFO, *PUNWIND_INFO;

struct ExceptionHandlerRecord {
  RUNTIME_FUNCTION runtime_function;
  UNWIND_INFO unwind_info;
  unsigned char thunk[12];
};

void RegisterNonABICompliantCodeRangeImpl(void* start, size_t size_in_bytes) {
  ExceptionHandlerRecord* record =
      reinterpret_cast<ExceptionHandlerRecord*>(start);

  // We assume that the first page of the code range is executable and
  // committed and reserved for breakpad. What could possibly go wrong?

  // All addresses are 32bit relative offsets to start.
  record->runtime_function.BeginAddress = 0;
  record->runtime_function.EndAddress =
      base::checked_cast<DWORD>(size_in_bytes);
  record->runtime_function.UnwindData =
      offsetof(ExceptionHandlerRecord, unwind_info);

  // Create unwind info that only specifies an exception handler.
  record->unwind_info.Version = 1;
  record->unwind_info.Flags = UNW_FLAG_EHANDLER;
  record->unwind_info.SizeOfProlog = 0;
  record->unwind_info.CountOfCodes = 0;
  record->unwind_info.FrameRegister = 0;
  record->unwind_info.FrameOffset = 0;
  record->unwind_info.ExceptionHandler =
      offsetof(ExceptionHandlerRecord, thunk);

  // Hardcoded thunk.
  // mov imm64, rax
  record->thunk[0] = 0x48;
  record->thunk[1] = 0xb8;
  void* handler =
      reinterpret_cast<void*>(&CrashForExceptionInNonABICompliantCodeRange);
  memcpy(&record->thunk[2], &handler, 8);

  // jmp rax
  record->thunk[10] = 0xff;
  record->thunk[11] = 0xe0;

  // Protect reserved page against modifications.
  DWORD old_protect;
  CHECK(VirtualProtect(
      start, sizeof(ExceptionHandlerRecord), PAGE_EXECUTE_READ, &old_protect));
  CHECK(RtlAddFunctionTable(
      &record->runtime_function, 1, reinterpret_cast<DWORD64>(start)));
}

void UnregisterNonABICompliantCodeRangeImpl(void* start) {
  ExceptionHandlerRecord* record =
      reinterpret_cast<ExceptionHandlerRecord*>(start);

  CHECK(RtlDeleteFunctionTable(&record->runtime_function));
}
#endif  // ARCH_CPU_X86_64

}  // namespace internal
}  // namespace crash_reporter
