// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Defines StructTraits specializations for translating between mojo types and
// base::StackSamplingProfiler types, with data validity checks.

#ifndef COMPONENTS_METRICS_PUBLIC_CPP_CALL_STACK_PROFILE_STRUCT_TRAITS_H_
#define COMPONENTS_METRICS_PUBLIC_CPP_CALL_STACK_PROFILE_STRUCT_TRAITS_H_

#include <string>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/profiler/stack_sampling_profiler.h"
#include "components/metrics/public/interfaces/call_stack_profile_collector.mojom.h"

namespace mojo {

template <>
struct StructTraits<metrics::mojom::CallStackModuleDataView,
                    base::StackSamplingProfiler::Module> {
  static uint64_t base_address(
      const base::StackSamplingProfiler::Module& module) {
    return module.base_address;
  }
  static const std::string& id(
      const base::StackSamplingProfiler::Module& module) {
    return module.id;
  }
  static const base::FilePath& filename(
      const base::StackSamplingProfiler::Module& module) {
    return module.filename;
  }

  static bool Read(metrics::mojom::CallStackModuleDataView data,
                   base::StackSamplingProfiler::Module* out) {
    std::string id;
    base::FilePath filename;
    if (!data.ReadId(&id) || !data.ReadFilename(&filename))
      return false;

    *out =
        base::StackSamplingProfiler::Module(data.base_address(), id, filename);
    return true;
  }
};

template <>
struct StructTraits<metrics::mojom::CallStackFrameDataView,
                    base::StackSamplingProfiler::Frame> {
  static uint64_t instruction_pointer(
      const base::StackSamplingProfiler::Frame& frame) {
    return frame.instruction_pointer;
  }
  static uint64_t module_index(
      const base::StackSamplingProfiler::Frame& frame) {
    return frame.module_index ==
        base::StackSamplingProfiler::Frame::kUnknownModuleIndex ?
        static_cast<uint64_t>(-1) :
        frame.module_index;
  }

  static bool Read(metrics::mojom::CallStackFrameDataView data,
                   base::StackSamplingProfiler::Frame* out) {
    size_t module_index = data.module_index() == static_cast<uint64_t>(-1) ?
        base::StackSamplingProfiler::Frame::kUnknownModuleIndex :
        data.module_index();

    // We can't know whether the module_index field is valid at this point since
    // we don't have access to the number of modules here. This will be checked
    // in CallStackProfile's Read function below.
    *out = base::StackSamplingProfiler::Frame(data.instruction_pointer(),
                                              module_index);
    return true;
  }
};

template <>
struct StructTraits<metrics::mojom::CallStackSampleDataView,
                    base::StackSamplingProfiler::Sample> {
  static const std::vector<base::StackSamplingProfiler::Frame>& frames(
      const base::StackSamplingProfiler::Sample& sample) {
    return sample.frames;
  }
  static int32_t process_milestones(
      const base::StackSamplingProfiler::Sample& sample) {
    return sample.process_milestones;
  }

  static bool Read(metrics::mojom::CallStackSampleDataView data,
                   base::StackSamplingProfiler::Sample* out) {
    std::vector<base::StackSamplingProfiler::Frame> frames;
    if (!data.ReadFrames(&frames))
      return false;

    *out = base::StackSamplingProfiler::Sample();
    out->frames = std::move(frames);
    out->process_milestones = data.process_milestones();
    return true;
  }
};

template <>
struct StructTraits<metrics::mojom::CallStackProfileDataView,
                    base::StackSamplingProfiler::CallStackProfile> {
  static const std::vector<base::StackSamplingProfiler::Module>& modules(
      const base::StackSamplingProfiler::CallStackProfile& profile) {
    return profile.modules;
  }
  static const std::vector<base::StackSamplingProfiler::Sample>& samples(
      const base::StackSamplingProfiler::CallStackProfile& profile) {
    return profile.samples;
  }
  static const base::TimeDelta profile_duration(
      const base::StackSamplingProfiler::CallStackProfile& profile) {
    return profile.profile_duration;
  }
  static const base::TimeDelta sampling_period(
      const base::StackSamplingProfiler::CallStackProfile& profile) {
    return profile.sampling_period;
  }

  static bool ValidateSamples(
      std::vector<base::StackSamplingProfiler::Sample> samples,
      size_t module_count) {
    for (const base::StackSamplingProfiler::Sample& sample : samples) {
      for (const base::StackSamplingProfiler::Frame& frame : sample.frames) {
        if (frame.module_index >= module_count &&
            frame.module_index !=
                base::StackSamplingProfiler::Frame::kUnknownModuleIndex)
          return false;
      }
    }
    return true;
  }

  static bool Read(metrics::mojom::CallStackProfileDataView data,
                   base::StackSamplingProfiler::CallStackProfile* out) {
    std::vector<base::StackSamplingProfiler::Module> modules;
    std::vector<base::StackSamplingProfiler::Sample> samples;
    base::TimeDelta profile_duration, sampling_period;
    if (!data.ReadModules(&modules) || !data.ReadSamples(&samples) ||
        !data.ReadProfileDuration(&profile_duration) ||
        !data.ReadSamplingPeriod(&sampling_period) ||
        !ValidateSamples(samples, modules.size()))
      return false;

    *out = base::StackSamplingProfiler::CallStackProfile();
    out->modules = std::move(modules);
    out->samples = std::move(samples);
    out->profile_duration = profile_duration;
    out->sampling_period = sampling_period;
    return true;
  }
};

template <>
struct EnumTraits<metrics::mojom::Process,
                  metrics::CallStackProfileParams::Process> {
  static metrics::mojom::Process ToMojom(
      metrics::CallStackProfileParams::Process process) {
    switch (process) {
      case metrics::CallStackProfileParams::Process::UNKNOWN_PROCESS:
        return metrics::mojom::Process::UNKNOWN_PROCESS;
      case metrics::CallStackProfileParams::Process::BROWSER_PROCESS:
        return metrics::mojom::Process::BROWSER_PROCESS;
      case metrics::CallStackProfileParams::Process::RENDERER_PROCESS:
        return metrics::mojom::Process::RENDERER_PROCESS;
      case metrics::CallStackProfileParams::Process::GPU_PROCESS:
        return metrics::mojom::Process::GPU_PROCESS;
      case metrics::CallStackProfileParams::Process::UTILITY_PROCESS:
        return metrics::mojom::Process::UTILITY_PROCESS;
      case metrics::CallStackProfileParams::Process::ZYGOTE_PROCESS:
        return metrics::mojom::Process::ZYGOTE_PROCESS;
      case metrics::CallStackProfileParams::Process::SANDBOX_HELPER_PROCESS:
        return metrics::mojom::Process::SANDBOX_HELPER_PROCESS;
      case metrics::CallStackProfileParams::Process::PPAPI_PLUGIN_PROCESS:
        return metrics::mojom::Process::PPAPI_PLUGIN_PROCESS;
      case metrics::CallStackProfileParams::Process::PPAPI_BROKER_PROCESS:
        return metrics::mojom::Process::PPAPI_BROKER_PROCESS;
    }
    NOTREACHED();
    return metrics::mojom::Process::UNKNOWN_PROCESS;
  }

  static bool FromMojom(metrics::mojom::Process process,
                        metrics::CallStackProfileParams::Process* out) {
    switch (process) {
      case metrics::mojom::Process::UNKNOWN_PROCESS:
        *out = metrics::CallStackProfileParams::Process::UNKNOWN_PROCESS;
        return true;
      case metrics::mojom::Process::BROWSER_PROCESS:
        *out = metrics::CallStackProfileParams::Process::BROWSER_PROCESS;
        return true;
      case metrics::mojom::Process::RENDERER_PROCESS:
        *out = metrics::CallStackProfileParams::Process::RENDERER_PROCESS;
        return true;
      case metrics::mojom::Process::GPU_PROCESS:
        *out = metrics::CallStackProfileParams::Process::GPU_PROCESS;
        return true;
      case metrics::mojom::Process::UTILITY_PROCESS:
        *out = metrics::CallStackProfileParams::Process::UTILITY_PROCESS;
        return true;
      case metrics::mojom::Process::ZYGOTE_PROCESS:
        *out = metrics::CallStackProfileParams::Process::ZYGOTE_PROCESS;
        return true;
      case metrics::mojom::Process::SANDBOX_HELPER_PROCESS:
        *out = metrics::CallStackProfileParams::Process::SANDBOX_HELPER_PROCESS;
        return true;
      case metrics::mojom::Process::PPAPI_PLUGIN_PROCESS:
        *out = metrics::CallStackProfileParams::Process::PPAPI_PLUGIN_PROCESS;
        return true;
      case metrics::mojom::Process::PPAPI_BROKER_PROCESS:
        *out = metrics::CallStackProfileParams::Process::PPAPI_BROKER_PROCESS;
        return true;
    }
    return false;
  }
};

template <>
struct EnumTraits<metrics::mojom::Thread,
                  metrics::CallStackProfileParams::Thread> {
  static metrics::mojom::Thread ToMojom(
      metrics::CallStackProfileParams::Thread thread) {
    switch (thread) {
      case metrics::CallStackProfileParams::Thread::UNKNOWN_THREAD:
        return metrics::mojom::Thread::UNKNOWN_THREAD;
      case metrics::CallStackProfileParams::Thread::MAIN_THREAD:
        return metrics::mojom::Thread::MAIN_THREAD;
      case metrics::CallStackProfileParams::Thread::IO_THREAD:
        return metrics::mojom::Thread::IO_THREAD;
      case metrics::CallStackProfileParams::Thread::COMPOSITOR_THREAD:
        return metrics::mojom::Thread::COMPOSITOR_THREAD;
    }
    NOTREACHED();
    return metrics::mojom::Thread::UNKNOWN_THREAD;
  }

  static bool FromMojom(metrics::mojom::Thread thread,
                        metrics::CallStackProfileParams::Thread* out) {
    switch (thread) {
      case metrics::mojom::Thread::UNKNOWN_THREAD:
        *out = metrics::CallStackProfileParams::Thread::UNKNOWN_THREAD;
        return true;
      case metrics::mojom::Thread::MAIN_THREAD:
        *out = metrics::CallStackProfileParams::Thread::MAIN_THREAD;
        return true;
      case metrics::mojom::Thread::IO_THREAD:
        *out = metrics::CallStackProfileParams::Thread::IO_THREAD;
        return true;
      case metrics::mojom::Thread::COMPOSITOR_THREAD:
        *out = metrics::CallStackProfileParams::Thread::COMPOSITOR_THREAD;
        return true;
    }
    return false;
  }
};

template <>
struct EnumTraits<metrics::mojom::Trigger,
                  metrics::CallStackProfileParams::Trigger> {
  static metrics::mojom::Trigger ToMojom(
      metrics::CallStackProfileParams::Trigger trigger) {
    switch (trigger) {
      case metrics::CallStackProfileParams::Trigger::UNKNOWN:
        return metrics::mojom::Trigger::UNKNOWN;
      case metrics::CallStackProfileParams::Trigger::PROCESS_STARTUP:
        return metrics::mojom::Trigger::PROCESS_STARTUP;
      case metrics::CallStackProfileParams::Trigger::JANKY_TASK:
        return metrics::mojom::Trigger::JANKY_TASK;
      case metrics::CallStackProfileParams::Trigger::THREAD_HUNG:
        return metrics::mojom::Trigger::THREAD_HUNG;
      case metrics::CallStackProfileParams::Trigger::PERIODIC_COLLECTION:
        return metrics::mojom::Trigger::PERIODIC_COLLECTION;
    }
    NOTREACHED();
    return metrics::mojom::Trigger::UNKNOWN;
  }

  static bool FromMojom(metrics::mojom::Trigger trigger,
                        metrics::CallStackProfileParams::Trigger* out) {
    switch (trigger) {
      case metrics::mojom::Trigger::UNKNOWN:
        *out = metrics::CallStackProfileParams::Trigger::UNKNOWN;
        return true;
      case metrics::mojom::Trigger::PROCESS_STARTUP:
        *out = metrics::CallStackProfileParams::Trigger::PROCESS_STARTUP;
        return true;
      case metrics::mojom::Trigger::JANKY_TASK:
        *out = metrics::CallStackProfileParams::Trigger::JANKY_TASK;
        return true;
      case metrics::mojom::Trigger::THREAD_HUNG:
        *out = metrics::CallStackProfileParams::Trigger::THREAD_HUNG;
        return true;
      case metrics::mojom::Trigger::PERIODIC_COLLECTION:
        *out = metrics::CallStackProfileParams::Trigger::PERIODIC_COLLECTION;
        return true;
    }
    return false;
  }
};

template <>
struct StructTraits<metrics::mojom::CallStackProfileParamsDataView,
                    metrics::CallStackProfileParams> {
  static metrics::CallStackProfileParams::Process process(
      const metrics::CallStackProfileParams& params) {
    return params.process;
  }
  static metrics::CallStackProfileParams::Thread thread(
      const metrics::CallStackProfileParams& params) {
    return params.thread;
  }
  static metrics::CallStackProfileParams::Trigger trigger(
      const metrics::CallStackProfileParams& params) {
    return params.trigger;
  }
  static metrics::CallStackProfileParams::SampleOrderingSpec ordering_spec(
      const metrics::CallStackProfileParams& params) {
    return params.ordering_spec;
  }

  static bool Read(metrics::mojom::CallStackProfileParamsDataView data,
                   metrics::CallStackProfileParams* out) {
    metrics::CallStackProfileParams::Process process;
    metrics::CallStackProfileParams::Thread thread;
    metrics::CallStackProfileParams::Trigger trigger;
    metrics::CallStackProfileParams::SampleOrderingSpec ordering_spec;
    if (!data.ReadProcess(&process) || !data.ReadThread(&thread) ||
        !data.ReadTrigger(&trigger) || !data.ReadOrderingSpec(&ordering_spec)) {
      return false;
    }
    *out = metrics::CallStackProfileParams(process, thread, trigger,
                                           ordering_spec);
    return true;
  }
};

template <>
struct EnumTraits<metrics::mojom::SampleOrderingSpec,
                  metrics::CallStackProfileParams::SampleOrderingSpec> {
  static metrics::mojom::SampleOrderingSpec ToMojom(
      metrics::CallStackProfileParams::SampleOrderingSpec spec) {
    switch (spec) {
      case metrics::CallStackProfileParams::SampleOrderingSpec::MAY_SHUFFLE:
        return metrics::mojom::SampleOrderingSpec::MAY_SHUFFLE;
      case metrics::CallStackProfileParams::SampleOrderingSpec::PRESERVE_ORDER:
        return metrics::mojom::SampleOrderingSpec::PRESERVE_ORDER;
    }
    NOTREACHED();
    return metrics::mojom::SampleOrderingSpec::MAY_SHUFFLE;
  }

  static bool FromMojom(
      metrics::mojom::SampleOrderingSpec spec,
      metrics::CallStackProfileParams::SampleOrderingSpec* out) {
    switch (spec) {
      case metrics::mojom::SampleOrderingSpec::MAY_SHUFFLE:
        *out = metrics::CallStackProfileParams::SampleOrderingSpec::MAY_SHUFFLE;
        return true;
      case metrics::mojom::SampleOrderingSpec::PRESERVE_ORDER:
        *out =
            metrics::CallStackProfileParams::SampleOrderingSpec::PRESERVE_ORDER;
        return true;
    }
    return false;
  }
};

}  // namespace mojo

#endif  // COMPONENTS_METRICS_PUBLIC_CPP_CALL_STACK_PROFILE_STRUCT_TRAITS_H_
