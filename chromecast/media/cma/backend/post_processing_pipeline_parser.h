// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_MEDIA_CMA_BACKEND_POST_PROCESSING_PIPELINE_PARSER_H_
#define CHROMECAST_MEDIA_CMA_BACKEND_POST_PROCESSING_PIPELINE_PARSER_H_

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "base/macros.h"

namespace base {
class DictionaryValue;
class ListValue;
}  // namespace base

namespace chromecast {
namespace media {

// Helper class to hold information about a stream pipeline.
struct StreamPipelineDescriptor {
  // The format for pipeline is:
  // [ {"processor": "PATH_TO_SHARED_OBJECT",
  //    "config": "CONFIGURATION_STRING"},
  //   {"processor": "PATH_TO_SHARED_OBJECT",
  //    "config": "CONFIGURATION_STRING"},
  //    ... ]
  const base::ListValue* pipeline;
  std::unordered_set<std::string> stream_types;

  StreamPipelineDescriptor(
      const base::ListValue* pipeline_in,
      const std::unordered_set<std::string>& stream_types_in);
  ~StreamPipelineDescriptor();
  StreamPipelineDescriptor(const StreamPipelineDescriptor& other);
  StreamPipelineDescriptor operator=(const StreamPipelineDescriptor& other) =
      delete;
};

// Helper class to parse post-processing pipeline descriptor file.
class PostProcessingPipelineParser {
 public:
  // |json|, if provided, is used instead of reading from file.
  // |json| should be provided in tests only.
  explicit PostProcessingPipelineParser(const std::string& json = "");
  ~PostProcessingPipelineParser();

  std::vector<StreamPipelineDescriptor> GetStreamPipelines();

  // Gets the list of processors for the mix/linearize stages.
  // Same format as StreamPipelineDescriptor.pipeline
  const base::ListValue* GetMixPipeline();
  const base::ListValue* GetLinearizePipeline();

 private:
  const base::ListValue* GetPipelineByKey(const std::string& key);

  std::unique_ptr<base::DictionaryValue> config_dict_;
  const base::DictionaryValue* postprocessor_config_;

  DISALLOW_COPY_AND_ASSIGN(PostProcessingPipelineParser);
};

}  // namespace media
}  // namespace chromecast

#endif  // CHROMECAST_MEDIA_CMA_BACKEND_POST_PROCESSING_PIPELINE_PARSER_H_
