// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TRAFFIC_ANNOTATION_FILE_FILTER_H_
#define TRAFFIC_ANNOTATION_FILE_FILTER_H_

#include <string>
#include <vector>

#include "base/files/file_path.h"

// Provides the list of files that might be relevant to network traffic
// annotation by matching filename and searching for keywords in the file
// content.
// The file should end with either .cc or .mm and the content should include a
// keyword specifying definition of network traffic annotations or the name of
// a function that needs annotation.
class TrafficAnnotationFileFilter {
 public:
  TrafficAnnotationFileFilter();
  ~TrafficAnnotationFileFilter();

  // Adds the list of relevant files in the given |directory_name| to the
  // |file_paths|. If |directory_name| is empty, all files are returned.
  // |source_path| should be the repository source directory, e.g. C:/src.
  // |ignore_list| provides a list of partial paths to ignore.
  void GetRelevantFiles(const base::FilePath& source_path,
                        const std::vector<std::string>& ignore_list,
                        std::string directory_name,
                        std::vector<std::string>* file_paths);

  // Checks the name and content of a file and returns true if it is relevant.
  bool IsFileRelevant(const std::string& file_path);

  // Gets the list of all relevant files in the repository and stores them in
  // |git_files|.
  void GetFilesFromGit(const base::FilePath& source_path);

  // Sets the path to a file that would be used to mock the output of
  // 'git ls-files' in tests.
  void SetGitFileForTesting(const base::FilePath& file_path) {
    git_file_for_test_ = file_path;
  }

  const std::vector<std::string>& git_files() { return git_files_; }

 private:
  std::vector<std::string> git_files_;
  base::FilePath git_file_for_test_;
};

#endif  // TRAFFIC_ANNOTATION_FILE_FILTER_H_