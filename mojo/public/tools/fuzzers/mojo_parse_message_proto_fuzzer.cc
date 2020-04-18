// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Implementation of a proto version of mojo_parse_message_fuzzer that sends
// multiple messages per run.

#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/task_scheduler/task_scheduler.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/tools/fuzzers/fuzz_impl.h"
#include "mojo/public/tools/fuzzers/mojo_fuzzer.pb.h"
#include "testing/libfuzzer/proto/lpm_interface.h"

namespace mojo_proto_fuzzer {

void FuzzMessage(const MojoFuzzerMessages& mojo_fuzzer_messages,
                 base::RunLoop* run) {
  fuzz::mojom::FuzzInterfacePtr fuzz;
  auto impl = std::make_unique<FuzzImpl>(MakeRequest(&fuzz));
  auto router = impl->binding_.RouterForTesting();

  for (auto& message_str : mojo_fuzzer_messages.messages()) {
    // Create a mojo message with the appropriate payload size.
    mojo::Message message(0, 0, message_str.size(), 0, nullptr);
    if (message.data_num_bytes() < message_str.size()) {
      message.payload_buffer()->Allocate(message_str.size() -
                                         message.data_num_bytes());
    }

    // Set the raw message data.
    memcpy(message.mutable_data(), message_str.data(), message_str.size());

    // Run the message through header validation, payload validation, and
    // dispatch to the impl.
    router->SimulateReceivingMessageForTesting(&message);
  }

  // Allow the harness function to return now.
  run->Quit();
}

// Environment for the fuzzer. Initializes the mojo EDK and sets up a
// TaskScheduler, because Mojo messages must be sent and processed from
// TaskRunners.
struct Environment {
  Environment() : message_loop(base::MessageLoop::TYPE_UI) {
    base::TaskScheduler::CreateAndStartWithDefaultParams(
        "MojoParseMessageFuzzerProcess");
    mojo::edk::Init();
  }

  // Message loop to send and handle messages on.
  base::MessageLoop message_loop;

  // Suppress mojo validation failure logs.
  mojo::internal::ScopedSuppressValidationErrorLoggingForTests log_suppression;
};

DEFINE_PROTO_FUZZER(const MojoFuzzerMessages& mojo_fuzzer_messages) {
  static Environment* env = new Environment();
  // Pass the data along to run on a MessageLoop, and wait for it to finish.
  base::RunLoop run;
  env->message_loop.task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&FuzzMessage, mojo_fuzzer_messages, &run));
  run.Run();
}
}  // namespace mojo_proto_fuzzer
