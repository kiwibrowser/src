// Copyright 2015 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/utils/path_service.h"

#ifdef _WIN32
#include <Windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <sys/stat.h>
#else  // Linux
#include <linux/limits.h>
#include <sys/stat.h>
#include <unistd.h>
#endif  // _WIN32

#include <string>

#include "core/fxcrt/fx_system.h"

namespace {

#if defined(__APPLE__) || (defined(ANDROID) && __ANDROID_API__ < 21)
using stat_wrapper_t = struct stat;

int CallStat(const char* path, stat_wrapper_t* sb) {
  return stat(path, sb);
}
#elif !_WIN32
using stat_wrapper_t = struct stat64;

int CallStat(const char* path, stat_wrapper_t* sb) {
  return stat64(path, sb);
}
#endif

}  // namespace

// static
bool PathService::DirectoryExists(const std::string& path) {
#ifdef _WIN32
  DWORD fileattr = GetFileAttributesA(path.c_str());
  if (fileattr != INVALID_FILE_ATTRIBUTES)
    return (fileattr & FILE_ATTRIBUTE_DIRECTORY) != 0;
  return false;
#else
  stat_wrapper_t file_info;
  if (CallStat(path.c_str(), &file_info) != 0)
    return false;
  return S_ISDIR(file_info.st_mode);
#endif
}

// static
bool PathService::EndsWithSeparator(const std::string& path) {
  return path.size() > 1 && path[path.size() - 1] == PATH_SEPARATOR;
}

// static
bool PathService::GetExecutableDir(std::string* path) {
// Get the current executable file path.
#ifdef _WIN32
  char path_buffer[MAX_PATH];
  path_buffer[0] = 0;

  if (GetModuleFileNameA(NULL, path_buffer, MAX_PATH) == 0)
    return false;
  *path = std::string(path_buffer);
#elif defined(__APPLE__)
  ASSERT(path);
  unsigned int path_length = 0;
  _NSGetExecutablePath(NULL, &path_length);
  if (path_length == 0)
    return false;

  path->reserve(path_length);
  path->resize(path_length - 1);
  if (_NSGetExecutablePath(&((*path)[0]), &path_length))
    return false;
#else   // Linux
  static const char kProcSelfExe[] = "/proc/self/exe";
  char buf[PATH_MAX];
  ssize_t count = ::readlink(kProcSelfExe, buf, PATH_MAX);
  if (count <= 0)
    return false;

  *path = std::string(buf, count);
#endif  // _WIN32

  // Get the directory path.
  std::size_t pos = path->size() - 1;
  if (EndsWithSeparator(*path))
    pos--;
  std::size_t found = path->find_last_of(PATH_SEPARATOR, pos);
  if (found == std::string::npos)
    return false;
  path->resize(found);
  return true;
}

// static
bool PathService::GetSourceDir(std::string* path) {
  if (!GetExecutableDir(path))
    return false;

  if (!EndsWithSeparator(*path))
    path->push_back(PATH_SEPARATOR);
  path->append("..");
  path->push_back(PATH_SEPARATOR);
#if defined(ANDROID)
  path->append("chromium_tests_root");
#else   // Non-Android
  path->append("..");
#endif  // defined(ANDROID)
  return true;
}

// static
bool PathService::GetTestDataDir(std::string* path) {
  if (!GetSourceDir(path))
    return false;

  if (!EndsWithSeparator(*path))
    path->push_back(PATH_SEPARATOR);

  std::string potential_path = *path;
  potential_path.append("testing");
  potential_path.push_back(PATH_SEPARATOR);
  potential_path.append("resources");
  if (PathService::DirectoryExists(potential_path)) {
    *path = potential_path;
    return true;
  }

  potential_path = *path;
  potential_path.append("third_party");
  potential_path.push_back(PATH_SEPARATOR);
  potential_path.append("pdfium");
  potential_path.push_back(PATH_SEPARATOR);
  potential_path.append("testing");
  potential_path.push_back(PATH_SEPARATOR);
  potential_path.append("resources");
  if (PathService::DirectoryExists(potential_path)) {
    *path = potential_path;
    return true;
  }

  return false;
}

// static
bool PathService::GetTestFilePath(const std::string& file_name,
                                  std::string* path) {
  if (!GetTestDataDir(path))
    return false;

  if (!EndsWithSeparator(*path))
    path->push_back(PATH_SEPARATOR);
  path->append(file_name);
  return true;
}
