// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ASSIST_RANKER_ASSIST_RANKER_SERVICE_IMPL_H_
#define COMPONENTS_ASSIST_RANKER_ASSIST_RANKER_SERVICE_IMPL_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/sequence_checker.h"
#include "components/assist_ranker/assist_ranker_service.h"
#include "components/assist_ranker/predictor_config.h"

namespace net {
class URLRequestContextGetter;
}

namespace assist_ranker {

class BasePredictor;
class BinaryClassifierPredictor;

class AssistRankerServiceImpl : public AssistRankerService {
 public:
  AssistRankerServiceImpl(
      base::FilePath base_path,
      net::URLRequestContextGetter* url_request_context_getter);
  ~AssistRankerServiceImpl() override;

  // AssistRankerService...
  base::WeakPtr<BinaryClassifierPredictor> FetchBinaryClassifierPredictor(
      const PredictorConfig& config) override;

 private:
  // Returns the full path to the model cache.
  base::FilePath GetModelPath(const std::string& model_filename);

  // Request Context Getter used for RankerURLFetcher.
  scoped_refptr<net::URLRequestContextGetter> url_request_context_getter_;

  // Base path where models are stored.
  const base::FilePath base_path_;

  std::unordered_map<std::string, std::unique_ptr<BasePredictor>>
      predictor_map_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(AssistRankerServiceImpl);
};

}  // namespace assist_ranker

#endif  // COMPONENTS_ASSIST_RANKER_ASSIST_RANKER_SERVICE_IMPL_H_
