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

#include "utils/TerribleCommandBuffer.h"

#include "common/Assert.h"

namespace utils {

    TerribleCommandBuffer::TerribleCommandBuffer() {
    }

    TerribleCommandBuffer::TerribleCommandBuffer(dawn_wire::CommandHandler* handler)
        : mHandler(handler) {
    }

    void TerribleCommandBuffer::SetHandler(dawn_wire::CommandHandler* handler) {
        mHandler = handler;
    }

    void* TerribleCommandBuffer::GetCmdSpace(size_t size) {
        // TODO(kainino@chromium.org): Should we early-out if size is 0?
        //   (Here and/or in the caller?) It might be good to make the wire receiver get a nullptr
        //   instead of pointer to zero-sized allocation in mBuffer.

        // Cannot have commands in mBuffer and mLargeBuffer at same time.
        ASSERT(mOffset == 0 || mLargeBufferCmdSize == 0);

        if (size > sizeof(mBuffer)) {
            // Flush current cmds in mBuffer to keep order.
            if (mOffset > 0) {
                if (!Flush()) {
                    return nullptr;
                }
                return GetCmdSpace(size);
            }

            // Resize large buffer to the size that can
            // contain incoming command if needed.
            if (mLargeBuffer.size() < size) {
                mLargeBuffer.resize(size);
            }

            // Record whole cmd space.
            mLargeBufferCmdSize = size;

            return mLargeBuffer.data();
        }

        // Trigger flush if large buffer contain cmds.
        if (mLargeBufferCmdSize > 0) {
            if (!Flush()) {
                return nullptr;
            }
            return GetCmdSpace(size);
        }

        // Need to flush large buffer first.
        ASSERT(mLargeBufferCmdSize == 0);

        char* result = &mBuffer[mOffset];

        if (sizeof(mBuffer) - size < mOffset) {
            if (!Flush()) {
                return nullptr;
            }
            return GetCmdSpace(size);
        }

        mOffset += size;

        return result;
    }

    bool TerribleCommandBuffer::Flush() {
        // Cannot have commands in mBuffer and mLargeBuffer at same time.
        ASSERT(mOffset == 0 || mLargeBufferCmdSize == 0);

        bool success = false;
        // Big buffer not empty, flush it!
        if (mLargeBufferCmdSize > 0) {
            success = mHandler->HandleCommands(mLargeBuffer.data(), mLargeBufferCmdSize) != nullptr;
            // Clear big command buffers.
            mLargeBufferCmdSize = 0;
            return success;
        }

        success = mHandler->HandleCommands(mBuffer, mOffset) != nullptr;
        mOffset = 0;

        return success;
    }

}  // namespace utils
