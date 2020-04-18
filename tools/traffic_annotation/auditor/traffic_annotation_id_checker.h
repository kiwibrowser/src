// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TRAFFIC_ANNOTATION_ID_CHECKER_H_
#define TRAFFIC_ANNOTATION_ID_CHECKER_H_

#include <set>
#include <vector>

#include "tools/traffic_annotation/auditor/instance.h"

// Performs all tests that are required to ensure that annotations have correct
// ids.
class TrafficAnnotationIDChecker {
 public:
  TrafficAnnotationIDChecker(const std::set<int>& reserved_ids,
                             const std::set<int>& deprecated_ids);
  ~TrafficAnnotationIDChecker();

  // Loads |extracted_annotations| into |annotations_|;
  void Load(const std::vector<AnnotationInstance>& extracted_annotations);

  // Checks loaded |annotations_| for all sort of ID related errors and writes
  // errors to |errors|.
  void CheckIDs(std::vector<AuditorResult>* errors);

 private:
  // TODO(https://crbug.com/690323): Merge struct with AnnotationInstance.
  struct AnnotationItem {
    struct {
      std::string text;
      int hash_code;
    } ids[2];
    int ids_count;  // Number of existing ids (1 or 2).
    AnnotationInstance::Type type;
    std::string file_path;
    int line_number;
    bool loaded_from_archive;
  };

  // Checks if the ids in |invalid_set| are not used in annotations. If found,
  // creates an error with |error_type| and writes it to |errors|.
  void CheckForInvalidValues(const std::set<int>& invalid_set,
                             AuditorResult::Type error_type,
                             std::vector<AuditorResult>* errors);

  // Check if annotations that need two ids, have two and the second one is
  // different from their unique id.
  void CheckForSecondIDs(std::vector<AuditorResult>* errors);

  // Checks if there are ids with colliding hash values.
  void CheckForHashCollisions(std::vector<AuditorResult>* errors);

  // Checks if there are invalid repeated ids.
  void CheckForInvalidRepeatedIDs(std::vector<AuditorResult>* errors);

  // Checks if ids only include alphanumeric characters and underline.
  void CheckIDsFormat(std::vector<AuditorResult>* errors);

  AuditorResult CreateRepeatedIDError(const std::string& common_id,
                                      const AnnotationItem& item1,
                                      const AnnotationItem& item2);

  std::vector<AnnotationItem> annotations_;
  std::set<int> deprecated_ids_;
  std::set<int> reserved_ids_;
};

#endif  // TRAFFIC_ANNOTATION_ID_CHECKER_H_