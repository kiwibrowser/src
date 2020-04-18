// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_COMMON_EXPERIMENTS_MEMORY_ABLATION_EXPERIMENT_H_
#define THIRD_PARTY_BLINK_PUBLIC_COMMON_EXPERIMENTS_MEMORY_ABLATION_EXPERIMENT_H_

#include <memory>

#include "base/feature_list.h"
#include "base/memory/ref_counted.h"
#include "third_party/blink/common/common_export.h"

namespace base {
class SequencedTaskRunner;
}

namespace blink {

BLINK_COMMON_EXPORT extern const base::Feature kMemoryAblationFeature;
BLINK_COMMON_EXPORT extern const char kMemoryAblationFeatureSizeParam[];
BLINK_COMMON_EXPORT extern const char kMemoryAblationFeatureMinRAMParam[];
BLINK_COMMON_EXPORT extern const char kMemoryAblationFeatureMaxRAMParam[];

/* When enabled, this experiment allocates a chunk of memory to study
 * correlation between memory usage and performance metrics.
 */
class BLINK_COMMON_EXPORT MemoryAblationExperiment {
 public:
  ~MemoryAblationExperiment();

  // Starts the experiment if corresponding field trial is enabled
  static void MaybeStart(scoped_refptr<base::SequencedTaskRunner> task_runner);
  static void MaybeStartForRenderer(
      scoped_refptr<base::SequencedTaskRunner> task_runner);

 private:
  MemoryAblationExperiment();

  static void MaybeStartInternal(
      const base::Feature& memory_ablation_feature,
      scoped_refptr<base::SequencedTaskRunner> task_runner);
  void Start(scoped_refptr<base::SequencedTaskRunner> task_runner, size_t size);

  void AllocateMemory(size_t size);
  void TouchMemory(size_t offset);
  void ScheduleTouchMemory(size_t offset);

  static MemoryAblationExperiment* GetInstance();

  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  size_t memory_size_ = 0;
  std::unique_ptr<uint8_t[]> memory_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_COMMON_EXPERIMENTS_MEMORY_ABLATION_EXPERIMENT_H_
