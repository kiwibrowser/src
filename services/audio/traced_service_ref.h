// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_AUDIO_TRACED_SERVICE_REF_H_
#define SERVICES_AUDIO_TRACED_SERVICE_REF_H_

#include <memory>

#include "base/macros.h"

namespace service_manager {
class ServiceContextRef;
}

namespace audio {

// Wraps a service_manager::ServiceContextRef, and provides tracing information.
class TracedServiceRef final {
 public:
  TracedServiceRef();
  // |trace_name| must have static lifetime since it is used in trace macros.
  // By convention, "audio::InterfaceName Binding" is used, e.g.
  // "audio::StreamFactory Binding".
  TracedServiceRef(
      std::unique_ptr<service_manager::ServiceContextRef> context_ref,
      const char* trace_name);
  TracedServiceRef(TracedServiceRef&& other);
  TracedServiceRef& operator=(TracedServiceRef&& other);

  ~TracedServiceRef();

  // This id can be used to add TRACE_EVENT_NESTABLE_ASYNC traces to the scope
  // of the TracedServiceRef.
  const void* id_for_trace() const { return context_ref_.get(); }

 private:
  std::unique_ptr<service_manager::ServiceContextRef> context_ref_;
  const char* trace_name_;

  DISALLOW_COPY_AND_ASSIGN(TracedServiceRef);
};

}  // namespace audio

#endif  // SERVICES_AUDIO_TRACED_SERVICE_REF_H_
