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

#ifndef DAWNNATIVE_COMMAND_ALLOCATOR_H_
#define DAWNNATIVE_COMMAND_ALLOCATOR_H_

#include "common/Assert.h"
#include "common/Math.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace dawn_native {

    // Allocation for command buffers should be fast. To avoid doing an allocation per command
    // or to avoid copying commands when reallocing, we use a linear allocator in a growing set
    // of large memory blocks. We also use this to have the format to be (u32 commandId, command),
    // so that iteration over the commands is easy.

    // Usage of the allocator and iterator:
    //     CommandAllocator allocator;
    //     DrawCommand* cmd = allocator.Allocate<DrawCommand>(CommandType::Draw);
    //     // Fill command
    //     // Repeat allocation and filling commands
    //
    //     CommandIterator commands(allocator);
    //     CommandType type;
    //     while(commands.NextCommandId(&type)) {
    //         switch(type) {
    //              case CommandType::Draw:
    //                  DrawCommand* draw = commands.NextCommand<DrawCommand>();
    //                  // Do the draw
    //                  break;
    //              // other cases
    //         }
    //     }

    // Note that you need to extract the commands from the CommandAllocator before destroying it
    // and must tell the CommandIterator when the allocated commands have been processed for
    // deletion.

    // These are the lists of blocks, should not be used directly, only through CommandAllocator
    // and CommandIterator
    struct BlockDef {
        size_t size;
        uint8_t* block;
    };
    using CommandBlocks = std::vector<BlockDef>;

    namespace detail {
        constexpr uint32_t kEndOfBlock = std::numeric_limits<uint32_t>::max();
        constexpr uint32_t kAdditionalData = std::numeric_limits<uint32_t>::max() - 1;
    }  // namespace detail

    class CommandAllocator;

    // TODO(cwallez@chromium.org): prevent copy for both iterator and allocator
    class CommandIterator {
      public:
        CommandIterator();
        ~CommandIterator();

        CommandIterator(CommandIterator&& other);
        CommandIterator& operator=(CommandIterator&& other);

        CommandIterator(CommandAllocator&& allocator);
        CommandIterator& operator=(CommandAllocator&& allocator);

        template <typename E>
        bool NextCommandId(E* commandId) {
            return NextCommandId(reinterpret_cast<uint32_t*>(commandId));
        }
        template <typename T>
        T* NextCommand() {
            return static_cast<T*>(NextCommand(sizeof(T), alignof(T)));
        }
        template <typename T>
        T* NextData(size_t count) {
            return static_cast<T*>(NextData(sizeof(T) * count, alignof(T)));
        }

        // Needs to be called if iteration was stopped early.
        void Reset();

        void DataWasDestroyed();

      private:
        bool IsEmpty() const;

        DAWN_FORCE_INLINE bool NextCommandId(uint32_t* commandId) {
            uint8_t* idPtr = AlignPtr(mCurrentPtr, alignof(uint32_t));
            ASSERT(idPtr + sizeof(uint32_t) <=
                   mBlocks[mCurrentBlock].block + mBlocks[mCurrentBlock].size);

            uint32_t id = *reinterpret_cast<uint32_t*>(idPtr);

            if (id != detail::kEndOfBlock) {
                mCurrentPtr = idPtr + sizeof(uint32_t);
                *commandId = id;
                return true;
            }
            return NextCommandIdInNewBlock(commandId);
        }

        bool NextCommandIdInNewBlock(uint32_t* commandId);

        DAWN_FORCE_INLINE void* NextCommand(size_t commandSize, size_t commandAlignment) {
            uint8_t* commandPtr = AlignPtr(mCurrentPtr, commandAlignment);
            ASSERT(commandPtr + sizeof(commandSize) <=
                   mBlocks[mCurrentBlock].block + mBlocks[mCurrentBlock].size);

            mCurrentPtr = commandPtr + commandSize;
            return commandPtr;
        }

        DAWN_FORCE_INLINE void* NextData(size_t dataSize, size_t dataAlignment) {
            uint32_t id;
            bool hasId = NextCommandId(&id);
            ASSERT(hasId);
            ASSERT(id == detail::kAdditionalData);

            return NextCommand(dataSize, dataAlignment);
        }

        CommandBlocks mBlocks;
        uint8_t* mCurrentPtr = nullptr;
        size_t mCurrentBlock = 0;
        // Used to avoid a special case for empty iterators.
        uint32_t mEndOfBlock = detail::kEndOfBlock;
        bool mDataWasDestroyed = false;
    };

    class CommandAllocator {
      public:
        CommandAllocator();
        ~CommandAllocator();

        template <typename T, typename E>
        T* Allocate(E commandId) {
            static_assert(sizeof(E) == sizeof(uint32_t), "");
            static_assert(alignof(E) == alignof(uint32_t), "");
            static_assert(alignof(T) <= kMaxSupportedAlignment, "");
            T* result = reinterpret_cast<T*>(
                Allocate(static_cast<uint32_t>(commandId), sizeof(T), alignof(T)));
            if (!result) {
                return nullptr;
            }
            new (result) T;
            return result;
        }

        template <typename T>
        T* AllocateData(size_t count) {
            static_assert(alignof(T) <= kMaxSupportedAlignment, "");
            T* result = reinterpret_cast<T*>(AllocateData(sizeof(T) * count, alignof(T)));
            if (!result) {
                return nullptr;
            }
            for (size_t i = 0; i < count; i++) {
                new (result + i) T;
            }
            return result;
        }

      private:
        // This is used for some internal computations and can be any power of two as long as code
        // using the CommandAllocator passes the static_asserts.
        static constexpr size_t kMaxSupportedAlignment = 8;

        // To avoid checking for overflows at every step of the computations we compute an upper
        // bound of the space that will be needed in addition to the command data.
        static constexpr size_t kWorstCaseAdditionalSize =
            sizeof(uint32_t) + kMaxSupportedAlignment + alignof(uint32_t) + sizeof(uint32_t);

        friend CommandIterator;
        CommandBlocks&& AcquireBlocks();

        DAWN_FORCE_INLINE uint8_t* Allocate(uint32_t commandId,
                                            size_t commandSize,
                                            size_t commandAlignment) {
            ASSERT(mCurrentPtr != nullptr);
            ASSERT(mEndPtr != nullptr);
            ASSERT(commandId != detail::kEndOfBlock);

            // It should always be possible to allocate one id, for kEndOfBlock tagging,
            ASSERT(IsPtrAligned(mCurrentPtr, alignof(uint32_t)));
            ASSERT(mEndPtr >= mCurrentPtr);
            ASSERT(static_cast<size_t>(mEndPtr - mCurrentPtr) >= sizeof(uint32_t));

            // The memory after the ID will contain the following:
            //   - the current ID
            //   - padding to align the command, maximum kMaxSupportedAlignment
            //   - the command of size commandSize
            //   - padding to align the next ID, maximum alignof(uint32_t)
            //   - the next ID of size sizeof(uint32_t)

            // This can't overflow because by construction mCurrentPtr always has space for the next
            // ID.
            size_t remainingSize = static_cast<size_t>(mEndPtr - mCurrentPtr);

            // The good case were we have enough space for the command data and upper bound of the
            // extra required space.
            if ((remainingSize >= kWorstCaseAdditionalSize) &&
                (remainingSize - kWorstCaseAdditionalSize >= commandSize)) {
                uint32_t* idAlloc = reinterpret_cast<uint32_t*>(mCurrentPtr);
                *idAlloc = commandId;

                uint8_t* commandAlloc = AlignPtr(mCurrentPtr + sizeof(uint32_t), commandAlignment);
                mCurrentPtr = AlignPtr(commandAlloc + commandSize, alignof(uint32_t));

                return commandAlloc;
            }
            return AllocateInNewBlock(commandId, commandSize, commandAlignment);
        }

        uint8_t* AllocateInNewBlock(uint32_t commandId,
                                    size_t commandSize,
                                    size_t commandAlignment);

        DAWN_FORCE_INLINE uint8_t* AllocateData(size_t commandSize, size_t commandAlignment) {
            return Allocate(detail::kAdditionalData, commandSize, commandAlignment);
        }

        bool GetNewBlock(size_t minimumSize);

        CommandBlocks mBlocks;
        size_t mLastAllocationSize = 2048;

        // Pointers to the current range of allocation in the block. Guaranteed to allow for at
        // least one uint32_t if not nullptr, so that the special kEndOfBlock command id can always
        // be written. Nullptr iff the blocks were moved out.
        uint8_t* mCurrentPtr = nullptr;
        uint8_t* mEndPtr = nullptr;

        // Data used for the block range at initialization so that the first call to Allocate sees
        // there is not enough space and calls GetNewBlock. This avoids having to special case the
        // initialization in Allocate.
        uint32_t mDummyEnum[1] = {0};
    };

}  // namespace dawn_native

#endif  // DAWNNATIVE_COMMAND_ALLOCATOR_H_
