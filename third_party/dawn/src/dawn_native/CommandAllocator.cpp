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

#include "dawn_native/CommandAllocator.h"

#include "common/Assert.h"
#include "common/Math.h"

#include <algorithm>
#include <climits>
#include <cstdlib>

namespace dawn_native {

    constexpr uint32_t EndOfBlock = UINT_MAX;          // std::numeric_limits<uint32_t>::max();
    constexpr uint32_t AdditionalData = UINT_MAX - 1;  // std::numeric_limits<uint32_t>::max() - 1;

    // TODO(cwallez@chromium.org): figure out a way to have more type safety for the iterator

    CommandIterator::CommandIterator() : mEndOfBlock(EndOfBlock) {
        Reset();
    }

    CommandIterator::~CommandIterator() {
        ASSERT(mDataWasDestroyed);

        if (!IsEmpty()) {
            for (auto& block : mBlocks) {
                free(block.block);
            }
        }
    }

    CommandIterator::CommandIterator(CommandIterator&& other) : mEndOfBlock(EndOfBlock) {
        if (!other.IsEmpty()) {
            mBlocks = std::move(other.mBlocks);
            other.Reset();
        }
        other.DataWasDestroyed();
        Reset();
    }

    CommandIterator& CommandIterator::operator=(CommandIterator&& other) {
        if (!other.IsEmpty()) {
            mBlocks = std::move(other.mBlocks);
            other.Reset();
        } else {
            mBlocks.clear();
        }
        other.DataWasDestroyed();
        Reset();
        return *this;
    }

    CommandIterator::CommandIterator(CommandAllocator&& allocator)
        : mBlocks(allocator.AcquireBlocks()), mEndOfBlock(EndOfBlock) {
        Reset();
    }

    CommandIterator& CommandIterator::operator=(CommandAllocator&& allocator) {
        mBlocks = allocator.AcquireBlocks();
        Reset();
        return *this;
    }

    void CommandIterator::Reset() {
        mCurrentBlock = 0;

        if (mBlocks.empty()) {
            // This will case the first NextCommandId call to try to move to the next block and stop
            // the iteration immediately, without special casing the initialization.
            mCurrentPtr = reinterpret_cast<uint8_t*>(&mEndOfBlock);
            mBlocks.emplace_back();
            mBlocks[0].size = sizeof(mEndOfBlock);
            mBlocks[0].block = mCurrentPtr;
        } else {
            mCurrentPtr = AlignPtr(mBlocks[0].block, alignof(uint32_t));
        }
    }

    void CommandIterator::DataWasDestroyed() {
        mDataWasDestroyed = true;
    }

    bool CommandIterator::IsEmpty() const {
        return mBlocks[0].block == reinterpret_cast<const uint8_t*>(&mEndOfBlock);
    }

    bool CommandIterator::NextCommandId(uint32_t* commandId) {
        uint8_t* idPtr = AlignPtr(mCurrentPtr, alignof(uint32_t));
        ASSERT(idPtr + sizeof(uint32_t) <=
               mBlocks[mCurrentBlock].block + mBlocks[mCurrentBlock].size);

        uint32_t id = *reinterpret_cast<uint32_t*>(idPtr);

        if (id == EndOfBlock) {
            mCurrentBlock++;
            if (mCurrentBlock >= mBlocks.size()) {
                Reset();
                *commandId = EndOfBlock;
                return false;
            }
            mCurrentPtr = AlignPtr(mBlocks[mCurrentBlock].block, alignof(uint32_t));
            return NextCommandId(commandId);
        }

        mCurrentPtr = idPtr + sizeof(uint32_t);
        *commandId = id;
        return true;
    }

    void* CommandIterator::NextCommand(size_t commandSize, size_t commandAlignment) {
        uint8_t* commandPtr = AlignPtr(mCurrentPtr, commandAlignment);
        ASSERT(commandPtr + sizeof(commandSize) <=
               mBlocks[mCurrentBlock].block + mBlocks[mCurrentBlock].size);

        mCurrentPtr = commandPtr + commandSize;
        return commandPtr;
    }

    void* CommandIterator::NextData(size_t dataSize, size_t dataAlignment) {
        uint32_t id;
        bool hasId = NextCommandId(&id);
        ASSERT(hasId);
        ASSERT(id == AdditionalData);

        return NextCommand(dataSize, dataAlignment);
    }

    // Potential TODO(cwallez@chromium.org):
    //  - Host the size and pointer to next block in the block itself to avoid having an allocation
    //    in the vector
    //  - Assume T's alignof is, say 64bits, static assert it, and make commandAlignment a constant
    //    in Allocate
    //  - Be able to optimize allocation to one block, for command buffers expected to live long to
    //    avoid cache misses
    //  - Better block allocation, maybe have Dawn API to say command buffer is going to have size
    //    close to another

    CommandAllocator::CommandAllocator()
        : mCurrentPtr(reinterpret_cast<uint8_t*>(&mDummyEnum[0])),
          mEndPtr(reinterpret_cast<uint8_t*>(&mDummyEnum[1])) {
    }

    CommandAllocator::~CommandAllocator() {
        ASSERT(mBlocks.empty());
    }

    CommandBlocks&& CommandAllocator::AcquireBlocks() {
        ASSERT(mCurrentPtr != nullptr && mEndPtr != nullptr);
        ASSERT(IsPtrAligned(mCurrentPtr, alignof(uint32_t)));
        ASSERT(mCurrentPtr + sizeof(uint32_t) <= mEndPtr);
        *reinterpret_cast<uint32_t*>(mCurrentPtr) = EndOfBlock;

        mCurrentPtr = nullptr;
        mEndPtr = nullptr;
        return std::move(mBlocks);
    }

    uint8_t* CommandAllocator::Allocate(uint32_t commandId,
                                        size_t commandSize,
                                        size_t commandAlignment) {
        ASSERT(mCurrentPtr != nullptr);
        ASSERT(mEndPtr != nullptr);
        ASSERT(commandId != EndOfBlock);

        // It should always be possible to allocate one id, for EndOfBlock tagging,
        ASSERT(IsPtrAligned(mCurrentPtr, alignof(uint32_t)));
        ASSERT(mEndPtr >= mCurrentPtr);
        ASSERT(static_cast<size_t>(mEndPtr - mCurrentPtr) >= sizeof(uint32_t));

        // The memory after the ID will contain the following:
        //   - the current ID
        //   - padding to align the command, maximum kMaxSupportedAlignment
        //   - the command of size commandSize
        //   - padding to align the next ID, maximum alignof(uint32_t)
        //   - the next ID of size sizeof(uint32_t)
        //
        // To avoid checking for overflows at every step of the computations we compute an upper
        // bound of the space that will be needed in addition to the command data.
        static constexpr size_t kWorstCaseAdditionalSize =
            sizeof(uint32_t) + kMaxSupportedAlignment + alignof(uint32_t) + sizeof(uint32_t);

        // This can't overflow because by construction mCurrentPtr always has space for the next ID.
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

        // When there is not enough space, we signal the EndOfBlock, so that the iterator knows to
        // move to the next one. EndOfBlock on the last block means the end of the commands.
        uint32_t* idAlloc = reinterpret_cast<uint32_t*>(mCurrentPtr);
        *idAlloc = EndOfBlock;

        // We'll request a block that can contain at least the command ID, the command and an
        // additional ID to contain the EndOfBlock tag.
        size_t requestedBlockSize = commandSize + kWorstCaseAdditionalSize;

        // The computation of the request could overflow.
        if (DAWN_UNLIKELY(requestedBlockSize <= commandSize)) {
            return nullptr;
        }

        if (DAWN_UNLIKELY(!GetNewBlock(requestedBlockSize))) {
            return nullptr;
        }
        return Allocate(commandId, commandSize, commandAlignment);
    }

    uint8_t* CommandAllocator::AllocateData(size_t commandSize, size_t commandAlignment) {
        return Allocate(AdditionalData, commandSize, commandAlignment);
    }

    bool CommandAllocator::GetNewBlock(size_t minimumSize) {
        // Allocate blocks doubling sizes each time, to a maximum of 16k (or at least minimumSize).
        mLastAllocationSize =
            std::max(minimumSize, std::min(mLastAllocationSize * 2, size_t(16384)));

        uint8_t* block = static_cast<uint8_t*>(malloc(mLastAllocationSize));
        if (DAWN_UNLIKELY(block == nullptr)) {
            return false;
        }

        mBlocks.push_back({mLastAllocationSize, block});
        mCurrentPtr = AlignPtr(block, alignof(uint32_t));
        mEndPtr = block + mLastAllocationSize;
        return true;
    }

}  // namespace dawn_native
