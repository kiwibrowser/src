// Copyright 2016 Google Inc. All rights reserved.
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

#include "src/mutator.h"

#include <algorithm>
#include <functional>
#include <map>
#include <random>
#include <string>

#include "src/field_instance.h"
#include "src/utf8_fix.h"
#include "src/weighted_reservoir_sampler.h"

namespace protobuf_mutator {

using protobuf::Descriptor;
using protobuf::FieldDescriptor;
using protobuf::FileDescriptor;
using protobuf::Message;
using protobuf::OneofDescriptor;
using protobuf::Reflection;
using protobuf::util::MessageDifferencer;
using std::placeholders::_1;

namespace {

const int kMaxInitializeDepth = 200;
const uint64_t kDefaultMutateWeight = 1000000;

enum class Mutation {
  None,
  Add,     // Adds new field with default value.
  Mutate,  // Mutates field contents.
  Delete,  // Deletes field.
  Copy,    // Copy values copied from another field.

  // TODO(vitalybuka):
  // Clone,  // Adds new field with value copied from another field.
};

// Return random integer from [0, count)
size_t GetRandomIndex(RandomEngine* random, size_t count) {
  assert(count > 0);
  if (count == 1) return 0;
  return std::uniform_int_distribution<size_t>(0, count - 1)(*random);
}

// Flips random bit in the buffer.
void FlipBit(size_t size, uint8_t* bytes, RandomEngine* random) {
  size_t bit = GetRandomIndex(random, size * 8);
  bytes[bit / 8] ^= (1u << (bit % 8));
}

// Flips random bit in the value.
template <class T>
T FlipBit(T value, RandomEngine* random) {
  FlipBit(sizeof(value), reinterpret_cast<uint8_t*>(&value), random);
  return value;
}

// Return true with probability about 1-of-n.
bool GetRandomBool(RandomEngine* random, size_t n = 2) {
  return GetRandomIndex(random, n) == 0;
}

bool IsProto3SimpleField(const FieldDescriptor& field) {
  assert(field.file()->syntax() == FileDescriptor::SYNTAX_PROTO3 ||
         field.file()->syntax() == FileDescriptor::SYNTAX_PROTO2);
  return field.file()->syntax() == FileDescriptor::SYNTAX_PROTO3 &&
         field.cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE &&
         !field.containing_oneof() && !field.is_repeated();
}

struct CreateDefaultField : public FieldFunction<CreateDefaultField> {
  template <class T>
  void ForType(const FieldInstance& field) const {
    T value;
    field.GetDefault(&value);
    field.Create(value);
  }
};

struct DeleteField : public FieldFunction<DeleteField> {
  template <class T>
  void ForType(const FieldInstance& field) const {
    field.Delete();
  }
};

struct CopyField : public FieldFunction<CopyField> {
  template <class T>
  void ForType(const ConstFieldInstance& source,
               const FieldInstance& field) const {
    T value;
    source.Load(&value);
    field.Store(value);
  }
};

struct AppendField : public FieldFunction<AppendField> {
  template <class T>
  void ForType(const ConstFieldInstance& source,
               const FieldInstance& field) const {
    T value;
    source.Load(&value);
    field.Create(value);
  }
};

class IsEqualValueField : public FieldFunction<IsEqualValueField, bool> {
 public:
  template <class T>
  bool ForType(const ConstFieldInstance& a, const ConstFieldInstance& b) const {
    T aa;
    a.Load(&aa);
    T bb;
    b.Load(&bb);
    return IsEqual(aa, bb);
  }

 private:
  bool IsEqual(const ConstFieldInstance::Enum& a,
               const ConstFieldInstance::Enum& b) const {
    assert(a.count == b.count);
    return a.index == b.index;
  }

  bool IsEqual(const std::unique_ptr<protobuf::Message>& a,
               const std::unique_ptr<protobuf::Message>& b) const {
    return MessageDifferencer::Equals(*a, *b);
  }

  template <class T>
  bool IsEqual(const T& a, const T& b) const {
    return a == b;
  }
};

// Selects random field and mutation from the given proto message.
class MutationSampler {
 public:
  MutationSampler(bool keep_initialized, RandomEngine* random, Message* message)
      : keep_initialized_(keep_initialized), random_(random), sampler_(random) {
    Sample(message);
    assert(mutation() != Mutation::None ||
           message->GetDescriptor()->field_count() == 0);
  }

  // Returns selected field.
  const FieldInstance& field() const { return sampler_.selected().field; }

  // Returns selected mutation.
  Mutation mutation() const { return sampler_.selected().mutation; }

 private:
  void Sample(Message* message) {
    const Descriptor* descriptor = message->GetDescriptor();
    const Reflection* reflection = message->GetReflection();

    int field_count = descriptor->field_count();
    for (int i = 0; i < field_count; ++i) {
      const FieldDescriptor* field = descriptor->field(i);
      if (const OneofDescriptor* oneof = field->containing_oneof()) {
        // Handle entire oneof group on the first field.
        if (field->index_in_oneof() == 0) {
          assert(oneof->field_count());
          const FieldDescriptor* current_field =
              reflection->GetOneofFieldDescriptor(*message, oneof);
          for (;;) {
            const FieldDescriptor* add_field =
                oneof->field(GetRandomIndex(random_, oneof->field_count()));
            if (add_field != current_field) {
              sampler_.Try(kDefaultMutateWeight,
                           {{message, add_field}, Mutation::Add});
              break;
            }
            if (oneof->field_count() < 2) break;
          }
          if (current_field) {
            if (current_field->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE) {
              sampler_.Try(kDefaultMutateWeight,
                           {{message, current_field}, Mutation::Mutate});
            }
            sampler_.Try(kDefaultMutateWeight,
                         {{message, current_field}, Mutation::Delete});
            sampler_.Try(kDefaultMutateWeight,
                         {{message, current_field}, Mutation::Copy});
          }
        }
      } else {
        if (field->is_repeated()) {
          int field_size = reflection->FieldSize(*message, field);
          sampler_.Try(
              kDefaultMutateWeight,
              {{message, field, GetRandomIndex(random_, field_size + 1)},
               Mutation::Add});

          if (field_size) {
            size_t random_index = GetRandomIndex(random_, field_size);
            if (field->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE) {
              sampler_.Try(kDefaultMutateWeight,
                           {{message, field, random_index}, Mutation::Mutate});
            }
            sampler_.Try(kDefaultMutateWeight,
                         {{message, field, random_index}, Mutation::Delete});
            sampler_.Try(kDefaultMutateWeight,
                         {{message, field, random_index}, Mutation::Copy});
          }
        } else {
          if (reflection->HasField(*message, field) ||
              IsProto3SimpleField(*field)) {
            if (field->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE)
              sampler_.Try(kDefaultMutateWeight,
                           {{message, field}, Mutation::Mutate});
            if (!IsProto3SimpleField(*field) &&
                (!field->is_required() || !keep_initialized_)) {
              sampler_.Try(kDefaultMutateWeight,
                           {{message, field}, Mutation::Delete});
            }
            sampler_.Try(kDefaultMutateWeight,
                         {{message, field}, Mutation::Copy});
          } else {
            sampler_.Try(kDefaultMutateWeight,
                         {{message, field}, Mutation::Add});
          }
        }
      }

      if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
        if (field->is_repeated()) {
          const int field_size = reflection->FieldSize(*message, field);
          for (int j = 0; j < field_size; ++j)
            Sample(reflection->MutableRepeatedMessage(message, field, j));
        } else if (reflection->HasField(*message, field)) {
          Sample(reflection->MutableMessage(message, field));
        }
      }
    }
  }

  bool keep_initialized_ = false;

  RandomEngine* random_;

  struct Result {
    Result() = default;
    Result(const FieldInstance& f, Mutation m) : field(f), mutation(m) {}

    FieldInstance field;
    Mutation mutation = Mutation::None;
  };
  WeightedReservoirSampler<Result, RandomEngine> sampler_;
};

// Selects random field of compatible type to use for clone mutations.
class DataSourceSampler {
 public:
  DataSourceSampler(const ConstFieldInstance& match, RandomEngine* random,
                    Message* message)
      : match_(match), random_(random), sampler_(random) {
    Sample(message);
  }

  // Returns selected field.
  const ConstFieldInstance& field() const {
    assert(!IsEmpty());
    return sampler_.selected();
  }

  bool IsEmpty() const { return sampler_.IsEmpty(); }

 private:
  void Sample(Message* message) {
    const Descriptor* descriptor = message->GetDescriptor();
    const Reflection* reflection = message->GetReflection();

    int field_count = descriptor->field_count();
    for (int i = 0; i < field_count; ++i) {
      const FieldDescriptor* field = descriptor->field(i);
      if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
        if (field->is_repeated()) {
          const int field_size = reflection->FieldSize(*message, field);
          for (int j = 0; j < field_size; ++j) {
            Sample(reflection->MutableRepeatedMessage(message, field, j));
          }
        } else if (reflection->HasField(*message, field)) {
          Sample(reflection->MutableMessage(message, field));
        }
      }

      if (field->cpp_type() != match_.cpp_type()) continue;
      if (match_.cpp_type() == FieldDescriptor::CPPTYPE_ENUM) {
        if (field->enum_type() != match_.enum_type()) continue;
      } else if (match_.cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
        if (field->message_type() != match_.message_type()) continue;
      }

      if (field->is_repeated()) {
        if (int field_size = reflection->FieldSize(*message, field)) {
          ConstFieldInstance source(message, field,
                                    GetRandomIndex(random_, field_size));
          if (match_.EnforceUtf8() && !source.EnforceUtf8()) continue;
          if (!IsEqualValueField()(match_, source))
            sampler_.Try(field_size, source);
        }
      } else {
        if (reflection->HasField(*message, field)) {
          ConstFieldInstance source(message, field);
          if (match_.EnforceUtf8() && !source.EnforceUtf8()) continue;
          if (!IsEqualValueField()(match_, source)) sampler_.Try(1, source);
        }
      }
    }
  }

  ConstFieldInstance match_;
  RandomEngine* random_;

  WeightedReservoirSampler<ConstFieldInstance, RandomEngine> sampler_;
};

}  // namespace

class FieldMutator {
 public:
  FieldMutator(size_t size_increase_hint, bool enforce_changes,
               bool enforce_utf8_strings, Mutator* mutator)
      : size_increase_hint_(size_increase_hint),
        enforce_changes_(enforce_changes),
        enforce_utf8_strings_(enforce_utf8_strings),
        mutator_(mutator) {}

  void Mutate(int32_t* value) const {
    RepeatMutate(value, std::bind(&Mutator::MutateInt32, mutator_, _1));
  }

  void Mutate(int64_t* value) const {
    RepeatMutate(value, std::bind(&Mutator::MutateInt64, mutator_, _1));
  }

  void Mutate(uint32_t* value) const {
    RepeatMutate(value, std::bind(&Mutator::MutateUInt32, mutator_, _1));
  }

  void Mutate(uint64_t* value) const {
    RepeatMutate(value, std::bind(&Mutator::MutateUInt64, mutator_, _1));
  }

  void Mutate(float* value) const {
    RepeatMutate(value, std::bind(&Mutator::MutateFloat, mutator_, _1));
  }

  void Mutate(double* value) const {
    RepeatMutate(value, std::bind(&Mutator::MutateDouble, mutator_, _1));
  }

  void Mutate(bool* value) const {
    RepeatMutate(value, std::bind(&Mutator::MutateBool, mutator_, _1), 2);
  }

  void Mutate(FieldInstance::Enum* value) const {
    RepeatMutate(&value->index,
                 std::bind(&Mutator::MutateEnum, mutator_, _1, value->count),
                 std::max<size_t>(value->count, 1));
    assert(value->index < value->count);
  }

  void Mutate(std::string* value) const {
    if (enforce_utf8_strings_) {
      RepeatMutate(value, std::bind(&Mutator::MutateUtf8String, mutator_, _1,
                                    size_increase_hint_));
    } else {
      RepeatMutate(value, std::bind(&Mutator::MutateString, mutator_, _1,
                                    size_increase_hint_));
    }
  }

  void Mutate(std::unique_ptr<Message>* message) const {
    assert(!enforce_changes_);
    assert(*message);
    if (GetRandomBool(mutator_->random(), 100)) return;
    mutator_->Mutate(message->get(), size_increase_hint_);
  }

 private:
  template <class T, class F>
  void RepeatMutate(T* value, F mutate,
                    size_t unchanged_one_out_of = 100) const {
    if (!enforce_changes_ &&
        GetRandomBool(mutator_->random(), unchanged_one_out_of)) {
      return;
    }
    T tmp = *value;
    for (int i = 0; i < 10; ++i) {
      *value = mutate(*value);
      if (!enforce_changes_ || *value != tmp) return;
    }
  }

  size_t size_increase_hint_;
  size_t enforce_changes_;
  bool enforce_utf8_strings_;
  Mutator* mutator_;
};

namespace {

struct MutateField : public FieldFunction<MutateField> {
  template <class T>
  void ForType(const FieldInstance& field, size_t size_increase_hint,
               Mutator* mutator) const {
    T value;
    field.Load(&value);
    FieldMutator(size_increase_hint, true, field.EnforceUtf8(), mutator)
        .Mutate(&value);
    field.Store(value);
  }
};

struct CreateField : public FieldFunction<CreateField> {
 public:
  template <class T>
  void ForType(const FieldInstance& field, size_t size_increase_hint,
               Mutator* mutator) const {
    T value;
    field.GetDefault(&value);
    FieldMutator field_mutator(size_increase_hint,
                               false /* defaults could be useful */,
                               field.EnforceUtf8(), mutator);
    field_mutator.Mutate(&value);
    field.Create(value);
  }
};

}  // namespace

Mutator::Mutator(RandomEngine* random) : random_(random) {}

void Mutator::Mutate(Message* message, size_t size_increase_hint) {
  bool repeat;
  do {
    repeat = false;
    MutationSampler mutation(keep_initialized_, random_, message);
    switch (mutation.mutation()) {
      case Mutation::None:
        break;
      case Mutation::Add:
        CreateField()(mutation.field(), size_increase_hint / 2, this);
        break;
      case Mutation::Mutate:
        MutateField()(mutation.field(), size_increase_hint / 2, this);
        break;
      case Mutation::Delete:
        DeleteField()(mutation.field());
        break;
      case Mutation::Copy: {
        DataSourceSampler source(mutation.field(), random_, message);
        if (source.IsEmpty()) {
          repeat = true;
          break;
        }
        CopyField()(source.field(), mutation.field());
        break;
      }
      default:
        assert(false && "unexpected mutation");
    }
  } while (repeat);

  InitializeAndTrim(message, kMaxInitializeDepth);
  assert(!keep_initialized_ || message->IsInitialized());
}

void Mutator::CrossOver(const protobuf::Message& message1,
                        protobuf::Message* message2) {
  // CrossOver can produce result which still equals to inputs. So we backup
  // message2 to later comparison. message1 is already constant.
  std::unique_ptr<protobuf::Message> message2_copy(message2->New());
  message2_copy->CopyFrom(*message2);

  CrossOverImpl(message1, message2);

  InitializeAndTrim(message2, kMaxInitializeDepth);
  assert(!keep_initialized_ || message2->IsInitialized());

  // Can't call mutate from crossover because of a bug in libFuzzer.
  return;
  // if (MessageDifferencer::Equals(*message2_copy, *message2) ||
  //     MessageDifferencer::Equals(message1, *message2)) {
  //   Mutate(message2, 0);
  // }
}

void Mutator::CrossOverImpl(const protobuf::Message& message1,
                            protobuf::Message* message2) {
  const Descriptor* descriptor = message2->GetDescriptor();
  const Reflection* reflection = message2->GetReflection();
  assert(message1.GetDescriptor() == descriptor);
  assert(message1.GetReflection() == reflection);

  for (int i = 0; i < descriptor->field_count(); ++i) {
    const FieldDescriptor* field = descriptor->field(i);

    if (field->is_repeated()) {
      const int field_size1 = reflection->FieldSize(message1, field);
      int field_size2 = reflection->FieldSize(*message2, field);
      for (int j = 0; j < field_size1; ++j) {
        ConstFieldInstance source(&message1, field, j);
        FieldInstance destination(message2, field, field_size2++);
        AppendField()(source, destination);
      }

      assert(field_size2 == reflection->FieldSize(*message2, field));

      // Shuffle
      for (int j = 0; j < field_size2; ++j) {
        if (int k = GetRandomIndex(random_, field_size2 - j)) {
          reflection->SwapElements(message2, field, j, j + k);
        }
      }

      int keep = GetRandomIndex(random_, field_size2 + 1);

      if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
        int remove = field_size2 - keep;
        // Cross some message to keep with messages to remove.
        int cross = GetRandomIndex(random_, std::min(keep, remove) + 1);
        for (int j = 0; j < cross; ++j) {
          int k = GetRandomIndex(random_, keep);
          int r = keep + GetRandomIndex(random_, remove);
          assert(k != r);
          CrossOverImpl(reflection->GetRepeatedMessage(*message2, field, r),
                        reflection->MutableRepeatedMessage(message2, field, k));
        }
      }

      for (int j = keep; j < field_size2; ++j)
        reflection->RemoveLast(message2, field);
      assert(keep == reflection->FieldSize(*message2, field));

    } else if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      if (!reflection->HasField(message1, field)) {
        if (GetRandomBool(random_))
          DeleteField()(FieldInstance(message2, field));
      } else if (!reflection->HasField(*message2, field)) {
        if (GetRandomBool(random_)) {
          ConstFieldInstance source(&message1, field);
          CopyField()(source, FieldInstance(message2, field));
        }
      } else {
        CrossOverImpl(reflection->GetMessage(message1, field),
                      reflection->MutableMessage(message2, field));
      }
    } else {
      if (GetRandomBool(random_)) {
        if (reflection->HasField(message1, field)) {
          ConstFieldInstance source(&message1, field);
          CopyField()(source, FieldInstance(message2, field));
        } else {
          DeleteField()(FieldInstance(message2, field));
        }
      }
    }
  }
}

void Mutator::InitializeAndTrim(Message* message, int max_depth) {
  const Descriptor* descriptor = message->GetDescriptor();
  const Reflection* reflection = message->GetReflection();
  for (int i = 0; i < descriptor->field_count(); ++i) {
    const FieldDescriptor* field = descriptor->field(i);
    if (keep_initialized_ && field->is_required() &&
        !reflection->HasField(*message, field))
      CreateDefaultField()(FieldInstance(message, field));

    if (field->cpp_type() == FieldDescriptor::CPPTYPE_MESSAGE) {
      if (max_depth <= 0 && !field->is_required()) {
        // Clear deep optional fields to avoid stack overflow.
        reflection->ClearField(message, field);
        if (field->is_repeated())
          assert(!reflection->FieldSize(*message, field));
        else
          assert(!reflection->HasField(*message, field));
        continue;
      }

      if (field->is_repeated()) {
        const int field_size = reflection->FieldSize(*message, field);
        for (int j = 0; j < field_size; ++j) {
          Message* nested_message =
              reflection->MutableRepeatedMessage(message, field, j);
          InitializeAndTrim(nested_message, max_depth - 1);
        }
      } else if (reflection->HasField(*message, field)) {
        Message* nested_message = reflection->MutableMessage(message, field);
        InitializeAndTrim(nested_message, max_depth - 1);
      }
    }
  }
}

int32_t Mutator::MutateInt32(int32_t value) { return FlipBit(value, random_); }

int64_t Mutator::MutateInt64(int64_t value) { return FlipBit(value, random_); }

uint32_t Mutator::MutateUInt32(uint32_t value) {
  return FlipBit(value, random_);
}

uint64_t Mutator::MutateUInt64(uint64_t value) {
  return FlipBit(value, random_);
}

float Mutator::MutateFloat(float value) { return FlipBit(value, random_); }

double Mutator::MutateDouble(double value) { return FlipBit(value, random_); }

bool Mutator::MutateBool(bool value) { return !value; }

size_t Mutator::MutateEnum(size_t index, size_t item_count) {
  if (item_count <= 1) return 0;
  return (index + 1 + GetRandomIndex(random_, item_count - 1)) % item_count;
}

std::string Mutator::MutateString(const std::string& value,
                                  size_t size_increase_hint) {
  std::string result = value;

  while (!result.empty() && GetRandomBool(random_)) {
    result.erase(GetRandomIndex(random_, result.size()), 1);
  }

  while (result.size() < size_increase_hint && GetRandomBool(random_)) {
    size_t index = GetRandomIndex(random_, result.size() + 1);
    result.insert(result.begin() + index, GetRandomIndex(random_, 1 << 8));
  }

  if (result != value) return result;

  if (result.empty()) {
    result.push_back(GetRandomIndex(random_, 1 << 8));
    return result;
  }

  if (!result.empty())
    FlipBit(result.size(), reinterpret_cast<uint8_t*>(&result[0]), random_);
  return result;
}

std::string Mutator::MutateUtf8String(const std::string& value,
                                      size_t size_increase_hint) {
  std::string str = MutateString(value, size_increase_hint);
  FixUtf8String(&str, random_);
  return str;
}

}  // namespace protobuf_mutator
