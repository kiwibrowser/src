// Copyright 2017 The Dawn Authors
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

#include <gtest/gtest.h>

#include "dawn_native/CommandAllocator.h"

#include <limits>

using namespace dawn_native;

// Definition of the command types used in the tests
enum class CommandType {
    Draw,
    Pipeline,
    PushConstants,
    Big,
    Small,
};

struct CommandDraw {
    uint32_t first;
    uint32_t count;
};

struct CommandPipeline {
    uint64_t pipeline;
    uint32_t attachmentPoint;
};

struct CommandPushConstants {
    uint8_t size;
    uint8_t offset;
};

constexpr int kBigBufferSize = 65536;

struct CommandBig {
    uint32_t buffer[kBigBufferSize];
};

struct CommandSmall {
    uint16_t data;
};

// Test allocating nothing works
TEST(CommandAllocator, DoNothingAllocator) {
    CommandAllocator allocator;
}

// Test iterating over nothing works
TEST(CommandAllocator, DoNothingAllocatorWithIterator) {
    CommandAllocator allocator;
    CommandIterator iterator(std::move(allocator));
    iterator.DataWasDestroyed();
}

// Test basic usage of allocator + iterator
TEST(CommandAllocator, Basic) {
    CommandAllocator allocator;

    uint64_t myPipeline = 0xDEADBEEFBEEFDEAD;
    uint32_t myAttachmentPoint = 2;
    uint32_t myFirst = 42;
    uint32_t myCount = 16;

    {
        CommandPipeline* pipeline = allocator.Allocate<CommandPipeline>(CommandType::Pipeline);
        pipeline->pipeline = myPipeline;
        pipeline->attachmentPoint = myAttachmentPoint;

        CommandDraw* draw = allocator.Allocate<CommandDraw>(CommandType::Draw);
        draw->first = myFirst;
        draw->count = myCount;
    }

    {
        CommandIterator iterator(std::move(allocator));
        CommandType type;

        bool hasNext = iterator.NextCommandId(&type);
        ASSERT_TRUE(hasNext);
        ASSERT_EQ(type, CommandType::Pipeline);

        CommandPipeline* pipeline = iterator.NextCommand<CommandPipeline>();
        ASSERT_EQ(pipeline->pipeline, myPipeline);
        ASSERT_EQ(pipeline->attachmentPoint, myAttachmentPoint);

        hasNext = iterator.NextCommandId(&type);
        ASSERT_TRUE(hasNext);
        ASSERT_EQ(type, CommandType::Draw);

        CommandDraw* draw = iterator.NextCommand<CommandDraw>();
        ASSERT_EQ(draw->first, myFirst);
        ASSERT_EQ(draw->count, myCount);

        hasNext = iterator.NextCommandId(&type);
        ASSERT_FALSE(hasNext);

        iterator.DataWasDestroyed();
    }
}

// Test basic usage of allocator + iterator with data
TEST(CommandAllocator, BasicWithData) {
    CommandAllocator allocator;

    uint8_t mySize = 8;
    uint8_t myOffset = 3;
    uint32_t myValues[5] = {6, 42, 0xFFFFFFFF, 0, 54};

    {
        CommandPushConstants* pushConstants =
            allocator.Allocate<CommandPushConstants>(CommandType::PushConstants);
        pushConstants->size = mySize;
        pushConstants->offset = myOffset;

        uint32_t* values = allocator.AllocateData<uint32_t>(5);
        for (size_t i = 0; i < 5; i++) {
            values[i] = myValues[i];
        }
    }

    {
        CommandIterator iterator(std::move(allocator));
        CommandType type;

        bool hasNext = iterator.NextCommandId(&type);
        ASSERT_TRUE(hasNext);
        ASSERT_EQ(type, CommandType::PushConstants);

        CommandPushConstants* pushConstants = iterator.NextCommand<CommandPushConstants>();
        ASSERT_EQ(pushConstants->size, mySize);
        ASSERT_EQ(pushConstants->offset, myOffset);

        uint32_t* values = iterator.NextData<uint32_t>(5);
        for (size_t i = 0; i < 5; i++) {
            ASSERT_EQ(values[i], myValues[i]);
        }

        hasNext = iterator.NextCommandId(&type);
        ASSERT_FALSE(hasNext);

        iterator.DataWasDestroyed();
    }
}

// Test basic iterating several times
TEST(CommandAllocator, MultipleIterations) {
    CommandAllocator allocator;

    uint32_t myFirst = 42;
    uint32_t myCount = 16;

    {
        CommandDraw* draw = allocator.Allocate<CommandDraw>(CommandType::Draw);
        draw->first = myFirst;
        draw->count = myCount;
    }

    {
        CommandIterator iterator(std::move(allocator));
        CommandType type;

        // First iteration
        bool hasNext = iterator.NextCommandId(&type);
        ASSERT_TRUE(hasNext);
        ASSERT_EQ(type, CommandType::Draw);

        CommandDraw* draw = iterator.NextCommand<CommandDraw>();
        ASSERT_EQ(draw->first, myFirst);
        ASSERT_EQ(draw->count, myCount);

        hasNext = iterator.NextCommandId(&type);
        ASSERT_FALSE(hasNext);

        // Second iteration
        hasNext = iterator.NextCommandId(&type);
        ASSERT_TRUE(hasNext);
        ASSERT_EQ(type, CommandType::Draw);

        draw = iterator.NextCommand<CommandDraw>();
        ASSERT_EQ(draw->first, myFirst);
        ASSERT_EQ(draw->count, myCount);

        hasNext = iterator.NextCommandId(&type);
        ASSERT_FALSE(hasNext);

        iterator.DataWasDestroyed();
    }
}
// Test large commands work
TEST(CommandAllocator, LargeCommands) {
    CommandAllocator allocator;

    const int kCommandCount = 5;

    uint32_t count = 0;
    for (int i = 0; i < kCommandCount; i++) {
        CommandBig* big = allocator.Allocate<CommandBig>(CommandType::Big);
        for (int j = 0; j < kBigBufferSize; j++) {
            big->buffer[j] = count++;
        }
    }

    CommandIterator iterator(std::move(allocator));
    CommandType type;
    count = 0;
    int numCommands = 0;
    while (iterator.NextCommandId(&type)) {
        ASSERT_EQ(type, CommandType::Big);

        CommandBig* big = iterator.NextCommand<CommandBig>();
        for (int i = 0; i < kBigBufferSize; i++) {
            ASSERT_EQ(big->buffer[i], count);
            count++;
        }
        numCommands++;
    }
    ASSERT_EQ(numCommands, kCommandCount);

    iterator.DataWasDestroyed();
}

// Test many small commands work
TEST(CommandAllocator, ManySmallCommands) {
    CommandAllocator allocator;

    // Stay under max representable uint16_t
    const int kCommandCount = 50000;

    uint16_t count = 0;
    for (int i = 0; i < kCommandCount; i++) {
        CommandSmall* small = allocator.Allocate<CommandSmall>(CommandType::Small);
        small->data = count++;
    }

    CommandIterator iterator(std::move(allocator));
    CommandType type;
    count = 0;
    int numCommands = 0;
    while (iterator.NextCommandId(&type)) {
        ASSERT_EQ(type, CommandType::Small);

        CommandSmall* small = iterator.NextCommand<CommandSmall>();
        ASSERT_EQ(small->data, count);
        count++;
        numCommands++;
    }
    ASSERT_EQ(numCommands, kCommandCount);

    iterator.DataWasDestroyed();
}

/*        ________
 *       /        \
 *       | POUIC! |
 *       \_ ______/
 *         v
 *    ()_()
 *    (O.o)
 *    (> <)o
 */

// Test usage of iterator.Reset
TEST(CommandAllocator, IteratorReset) {
    CommandAllocator allocator;

    uint64_t myPipeline = 0xDEADBEEFBEEFDEAD;
    uint32_t myAttachmentPoint = 2;
    uint32_t myFirst = 42;
    uint32_t myCount = 16;

    {
        CommandPipeline* pipeline = allocator.Allocate<CommandPipeline>(CommandType::Pipeline);
        pipeline->pipeline = myPipeline;
        pipeline->attachmentPoint = myAttachmentPoint;

        CommandDraw* draw = allocator.Allocate<CommandDraw>(CommandType::Draw);
        draw->first = myFirst;
        draw->count = myCount;
    }

    {
        CommandIterator iterator(std::move(allocator));
        CommandType type;

        bool hasNext = iterator.NextCommandId(&type);
        ASSERT_TRUE(hasNext);
        ASSERT_EQ(type, CommandType::Pipeline);

        CommandPipeline* pipeline = iterator.NextCommand<CommandPipeline>();
        ASSERT_EQ(pipeline->pipeline, myPipeline);
        ASSERT_EQ(pipeline->attachmentPoint, myAttachmentPoint);

        iterator.Reset();

        hasNext = iterator.NextCommandId(&type);
        ASSERT_TRUE(hasNext);
        ASSERT_EQ(type, CommandType::Pipeline);

        pipeline = iterator.NextCommand<CommandPipeline>();
        ASSERT_EQ(pipeline->pipeline, myPipeline);
        ASSERT_EQ(pipeline->attachmentPoint, myAttachmentPoint);

        hasNext = iterator.NextCommandId(&type);
        ASSERT_TRUE(hasNext);
        ASSERT_EQ(type, CommandType::Draw);

        CommandDraw* draw = iterator.NextCommand<CommandDraw>();
        ASSERT_EQ(draw->first, myFirst);
        ASSERT_EQ(draw->count, myCount);

        hasNext = iterator.NextCommandId(&type);
        ASSERT_FALSE(hasNext);

        iterator.DataWasDestroyed();
    }
}

// Test iterating empty iterators
TEST(CommandAllocator, EmptyIterator) {
    {
        CommandAllocator allocator;
        CommandIterator iterator(std::move(allocator));

        CommandType type;
        bool hasNext = iterator.NextCommandId(&type);
        ASSERT_FALSE(hasNext);

        iterator.DataWasDestroyed();
    }
    {
        CommandAllocator allocator;
        CommandIterator iterator1(std::move(allocator));
        CommandIterator iterator2(std::move(iterator1));

        CommandType type;
        bool hasNext = iterator2.NextCommandId(&type);
        ASSERT_FALSE(hasNext);

        iterator1.DataWasDestroyed();
        iterator2.DataWasDestroyed();
    }
    {
        CommandIterator iterator1;
        CommandIterator iterator2(std::move(iterator1));

        CommandType type;
        bool hasNext = iterator2.NextCommandId(&type);
        ASSERT_FALSE(hasNext);

        iterator1.DataWasDestroyed();
        iterator2.DataWasDestroyed();
    }
}

template <size_t A>
struct alignas(A) AlignedStruct {
    char dummy;
};

// Test for overflows in Allocate's computations, size 1 variant
TEST(CommandAllocator, AllocationOverflow_1) {
    CommandAllocator allocator;
    AlignedStruct<1>* data =
        allocator.AllocateData<AlignedStruct<1>>(std::numeric_limits<size_t>::max() / 1);
    ASSERT_EQ(data, nullptr);
}

// Test for overflows in Allocate's computations, size 2 variant
TEST(CommandAllocator, AllocationOverflow_2) {
    CommandAllocator allocator;
    AlignedStruct<2>* data =
        allocator.AllocateData<AlignedStruct<2>>(std::numeric_limits<size_t>::max() / 2);
    ASSERT_EQ(data, nullptr);
}

// Test for overflows in Allocate's computations, size 4 variant
TEST(CommandAllocator, AllocationOverflow_4) {
    CommandAllocator allocator;
    AlignedStruct<4>* data =
        allocator.AllocateData<AlignedStruct<4>>(std::numeric_limits<size_t>::max() / 4);
    ASSERT_EQ(data, nullptr);
}

// Test for overflows in Allocate's computations, size 8 variant
TEST(CommandAllocator, AllocationOverflow_8) {
    CommandAllocator allocator;
    AlignedStruct<8>* data =
        allocator.AllocateData<AlignedStruct<8>>(std::numeric_limits<size_t>::max() / 8);
    ASSERT_EQ(data, nullptr);
}

template <int DefaultValue>
struct IntWithDefault {
    IntWithDefault() : value(DefaultValue) {
    }

    int value;
};

// Test that the allcator correctly defaults initalizes data for Allocate
TEST(CommandAllocator, AllocateDefaultInitializes) {
    CommandAllocator allocator;

    IntWithDefault<42>* int42 = allocator.Allocate<IntWithDefault<42>>(CommandType::Draw);
    ASSERT_EQ(int42->value, 42);

    IntWithDefault<43>* int43 = allocator.Allocate<IntWithDefault<43>>(CommandType::Draw);
    ASSERT_EQ(int43->value, 43);

    IntWithDefault<44>* int44 = allocator.Allocate<IntWithDefault<44>>(CommandType::Draw);
    ASSERT_EQ(int44->value, 44);

    CommandIterator iterator(std::move(allocator));
    iterator.DataWasDestroyed();
}

// Test that the allcator correctly defaults initalizes data for AllocateData
TEST(CommandAllocator, AllocateDataDefaultInitializes) {
    CommandAllocator allocator;

    IntWithDefault<33>* int33 = allocator.AllocateData<IntWithDefault<33>>(1);
    ASSERT_EQ(int33[0].value, 33);

    IntWithDefault<34>* int34 = allocator.AllocateData<IntWithDefault<34>>(2);
    ASSERT_EQ(int34[0].value, 34);
    ASSERT_EQ(int34[0].value, 34);

    IntWithDefault<35>* int35 = allocator.AllocateData<IntWithDefault<35>>(3);
    ASSERT_EQ(int35[0].value, 35);
    ASSERT_EQ(int35[1].value, 35);
    ASSERT_EQ(int35[2].value, 35);

    CommandIterator iterator(std::move(allocator));
    iterator.DataWasDestroyed();
}
