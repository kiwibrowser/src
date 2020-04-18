// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_TRACING_RECORDER_H_
#define SERVICES_TRACING_RECORDER_H_

#include <memory>
#include <string>

#include "base/callback_forward.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/tracing/public/mojom/tracing.mojom.h"

namespace tracing {

class Recorder : public mojom::Recorder {
 public:
  // The tracing service creates instances of the |Recorder| class and send them
  // to agents. The agents then use the recorder for sending trace data to the
  // tracing service.
  //
  // |data_is_array| tells the recorder whether the data is of type array or
  // string. Chunks of type array are concatenated using a comma as the
  // separator; chuunks of type string are concatenated without a separator.
  //
  // |on_data_change_callback| is run whenever the recorder receives data from
  // the agent or when the connection is lost to notify the tracing service of
  // the data change.
  Recorder(mojom::RecorderRequest request,
           mojom::TraceDataType data_type,
           const base::RepeatingClosure& on_data_change_callback);
  ~Recorder() override;

  const std::string& data() const { return data_; }

  void clear_data() { data_.clear(); }

  const base::DictionaryValue& metadata() const { return metadata_; }
  bool is_recording() const { return is_recording_; }
  mojom::TraceDataType data_type() const { return data_type_; }

 private:
  friend class RecorderTest;  // For testing.
  // mojom::Recorder
  // These are called by agents for sending trace data to the tracing service.
  void AddChunk(const std::string& chunk) override;
  void AddMetadata(base::Value metadata) override;

  void OnConnectionError();

  std::string data_;
  base::DictionaryValue metadata_;
  bool is_recording_;
  mojom::TraceDataType data_type_;
  base::RepeatingClosure on_data_change_callback_;
  mojo::Binding<mojom::Recorder> binding_;

  base::WeakPtrFactory<Recorder> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(Recorder);
};

}  // namespace tracing
#endif  // SERVICES_TRACING_RECORDER_H_
