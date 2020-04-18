// Copyright 2015 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TESTING_UTILS_PATH_SERVICE_H_
#define TESTING_UTILS_PATH_SERVICE_H_

#include <string>

#ifdef _WIN32
#define PATH_SEPARATOR '\\'
#else
#define PATH_SEPARATOR '/'
#endif

// Get the various file directory and path information.
class PathService {
 public:
  // Return true when the path is a directory that exists.
  static bool DirectoryExists(const std::string& path);

  // Return true when the path ends with a path separator.
  static bool EndsWithSeparator(const std::string& path);

  // Retrieve the directory where executables run from.
  static bool GetExecutableDir(std::string* path);

  // Retrieve the root directory of the source tree.
  // Assume executables always run from out/<build_dir_name>/, so the source
  // directory is two levels above the executable directory.
  static bool GetSourceDir(std::string* path);

  // Retrieve the test data directory where test files are stored.
  // Try <source_dir>/testing/resources/ first. If it does not exist, try
  // checking <source_dir>/third_party/pdfium/testing/resources/.
  static bool GetTestDataDir(std::string* path);

  // Get the full path for a test file under the test data directory.
  static bool GetTestFilePath(const std::string& file_name, std::string* path);
};
#endif  // TESTING_UTILS_PATH_SERVICE_H_
