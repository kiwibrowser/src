// Copyright 2018 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <fstream>

#include "gflags/gflags.h"
#include "tools/render/trace_program.h"

// render_trace renders the specified trace file using an OpenGL-based viewer.
int main(int argc, char* argv[]) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);

  CHECK_GE(argc, 2) << "Specify file path";
  auto trace = absl::make_unique<quic_trace::Trace>();
  {
    std::ifstream f(argv[1]);
    trace->ParseFromIstream(&f);
  }

  quic_trace::render::TraceProgram program;
  program.LoadTrace(std::move(trace));
  program.Loop();
  return 0;
}
