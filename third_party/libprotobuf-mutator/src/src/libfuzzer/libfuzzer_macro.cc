// Copyright 2017 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "src/libfuzzer/libfuzzer_macro.h"

#include "src/binary_format.h"
#include "src/libfuzzer/libfuzzer_mutator.h"
#include "src/text_format.h"

namespace protobuf_mutator {
namespace libfuzzer {

namespace {

class InputReader {
 public:
  InputReader(const uint8_t* data, size_t size) : data_(data), size_(size) {}
  virtual ~InputReader() = default;

  virtual bool Read(protobuf::Message* message) const = 0;

  const uint8_t* data() const { return data_; }
  size_t size() const { return size_; }

 private:
  const uint8_t* data_;
  size_t size_;
};

class OutputWriter {
 public:
  OutputWriter(uint8_t* data, size_t size) : data_(data), size_(size) {}
  virtual ~OutputWriter() = default;

  virtual size_t Write(const protobuf::Message& message) = 0;

  uint8_t* data() const { return data_; }
  size_t size() const { return size_; }

 private:
  uint8_t* data_;
  size_t size_;
};

class TextInputReader : public InputReader {
 public:
  using InputReader::InputReader;

  bool Read(protobuf::Message* message) const override {
    return ParseTextMessage(data(), size(), message);
  }
};

class TextOutputWriter : public OutputWriter {
 public:
  using OutputWriter::OutputWriter;

  size_t Write(const protobuf::Message& message) override {
    return SaveMessageAsText(message, data(), size());
  }
};

class BinaryInputReader : public InputReader {
 public:
  using InputReader::InputReader;

  bool Read(protobuf::Message* message) const override {
    return ParseBinaryMessage(data(), size(), message);
  }
};

class BinaryOutputWriter : public OutputWriter {
 public:
  using OutputWriter::OutputWriter;

  size_t Write(const protobuf::Message& message) override {
    return SaveMessageAsBinary(message, data(), size());
  }
};

size_t MutateMessage(unsigned int seed, const InputReader& input,
                     OutputWriter* output, protobuf::Message* message) {
  RandomEngine random(seed);
  Mutator mutator(&random);
  input.Read(message);
  mutator.Mutate(message, output->size() > input.size()
                              ? (output->size() - input.size())
                              : 0);
  if (size_t new_size = output->Write(*message)) {
    assert(new_size <= output->size());
    return new_size;
  }
  return 0;
}

size_t CrossOverMessages(unsigned int seed, const InputReader& input1,
                         const InputReader& input2, OutputWriter* output,
                         protobuf::Message* message1,
                         protobuf::Message* message2) {
  RandomEngine random(seed);
  Mutator mutator(&random);
  input1.Read(message1);
  input2.Read(message2);
  mutator.CrossOver(*message2, message1);
  if (size_t new_size = output->Write(*message1)) {
    assert(new_size <= output->size());
    return new_size;
  }
  return 0;
}

size_t MutateTextMessage(uint8_t* data, size_t size, size_t max_size,
                         unsigned int seed, protobuf::Message* message) {
  TextInputReader input(data, size);
  TextOutputWriter output(data, max_size);
  return MutateMessage(seed, input, &output, message);
}

size_t CrossOverTextMessages(const uint8_t* data1, size_t size1,
                             const uint8_t* data2, size_t size2, uint8_t* out,
                             size_t max_out_size, unsigned int seed,
                             protobuf::Message* message1,
                             protobuf::Message* message2) {
  TextInputReader input1(data1, size1);
  TextInputReader input2(data2, size2);
  TextOutputWriter output(out, max_out_size);
  return CrossOverMessages(seed, input1, input2, &output, message1, message2);
}

size_t MutateBinaryMessage(uint8_t* data, size_t size, size_t max_size,
                           unsigned int seed, protobuf::Message* message) {
  BinaryInputReader input(data, size);
  BinaryOutputWriter output(data, max_size);
  return MutateMessage(seed, input, &output, message);
}

size_t CrossOverBinaryMessages(const uint8_t* data1, size_t size1,
                               const uint8_t* data2, size_t size2, uint8_t* out,
                               size_t max_out_size, unsigned int seed,
                               protobuf::Message* message1,
                               protobuf::Message* message2) {
  BinaryInputReader input1(data1, size1);
  BinaryInputReader input2(data2, size2);
  BinaryOutputWriter output(out, max_out_size);
  return CrossOverMessages(seed, input1, input2, &output, message1, message2);
}

}  // namespace

size_t CustomProtoMutator(bool binary, uint8_t* data, size_t size,
                          size_t max_size, unsigned int seed,
                          protobuf::Message* input) {
  auto mutate = binary ? &MutateBinaryMessage : &MutateTextMessage;
  return mutate(data, size, max_size, seed, input);
}

size_t CustomProtoCrossOver(bool binary, const uint8_t* data1, size_t size1,
                            const uint8_t* data2, size_t size2, uint8_t* out,
                            size_t max_out_size, unsigned int seed,
                            protobuf::Message* input1,
                            protobuf::Message* input2) {
  auto cross = binary ? &CrossOverBinaryMessages : &CrossOverTextMessages;
  return cross(data1, size1, data2, size2, out, max_out_size, seed, input1,
               input2);
}

bool LoadProtoInput(bool binary, const uint8_t* data, size_t size,
                    protobuf::Message* input) {
  return binary ? ParseBinaryMessage(data, size, input)
                : ParseTextMessage(data, size, input);
}

}  // namespace libfuzzer
}  // namespace protobuf_mutator
