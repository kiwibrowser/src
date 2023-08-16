// Copyright 2019 The Dawn Authors
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

#include "dawn_wire/client/Device.h"

#include "common/Assert.h"
#include "dawn_wire/WireCmd_autogen.h"
#include "dawn_wire/client/ApiObjects_autogen.h"
#include "dawn_wire/client/Client.h"
#include "dawn_wire/client/ObjectAllocator.h"

namespace dawn_wire { namespace client {

    Device::Device(Client* client, uint32_t initialRefcount, uint32_t initialId)
        : ObjectBase(this, initialRefcount, initialId), mClient(client) {
        this->device = this;

        // Get the default queue for this device.
        ObjectAllocator<Queue>::ObjectAndSerial* allocation = mClient->QueueAllocator().New(this);
        mDefaultQueue = allocation->object.get();

        DeviceGetDefaultQueueCmd cmd;
        cmd.self = ToAPI(this);
        cmd.result = ObjectHandle{allocation->object->id, allocation->generation};

        mClient->SerializeCommand(cmd);
    }

    Device::~Device() {
        // Fire pending error scopes
        auto errorScopes = std::move(mErrorScopes);
        for (const auto& it : errorScopes) {
            it.second.callback(WGPUErrorType_Unknown, "Device destroyed", it.second.userdata);
        }

        // Destroy the default queue
        DestroyObjectCmd cmd;
        cmd.objectType = ObjectType::Queue;
        cmd.objectId = mDefaultQueue->id;

        mClient->SerializeCommand(cmd);

        mClient->QueueAllocator().Free(mDefaultQueue);
    }

    Client* Device::GetClient() {
        return mClient;
    }

    void Device::HandleError(WGPUErrorType errorType, const char* message) {
        if (mErrorCallback) {
            mErrorCallback(errorType, message, mErrorUserdata);
        }
    }

    void Device::HandleDeviceLost(const char* message) {
        if (mDeviceLostCallback && !mDidRunLostCallback) {
            mDidRunLostCallback = true;
            mDeviceLostCallback(message, mDeviceLostUserdata);
        }
    }

    void Device::SetUncapturedErrorCallback(WGPUErrorCallback errorCallback, void* errorUserdata) {
        mErrorCallback = errorCallback;
        mErrorUserdata = errorUserdata;
    }

    void Device::SetDeviceLostCallback(WGPUDeviceLostCallback callback, void* userdata) {
        mDeviceLostCallback = callback;
        mDeviceLostUserdata = userdata;
    }

    void Device::PushErrorScope(WGPUErrorFilter filter) {
        mErrorScopeStackSize++;

        DevicePushErrorScopeCmd cmd;
        cmd.self = ToAPI(this);
        cmd.filter = filter;

        mClient->SerializeCommand(cmd);
    }

    bool Device::PopErrorScope(WGPUErrorCallback callback, void* userdata) {
        if (mErrorScopeStackSize == 0) {
            return false;
        }
        mErrorScopeStackSize--;

        uint64_t serial = mErrorScopeRequestSerial++;
        ASSERT(mErrorScopes.find(serial) == mErrorScopes.end());

        mErrorScopes[serial] = {callback, userdata};

        DevicePopErrorScopeCmd cmd;
        cmd.device = ToAPI(this);
        cmd.requestSerial = serial;

        mClient->SerializeCommand(cmd);

        return true;
    }

    bool Device::OnPopErrorScopeCallback(uint64_t requestSerial,
                                         WGPUErrorType type,
                                         const char* message) {
        switch (type) {
            case WGPUErrorType_NoError:
            case WGPUErrorType_Validation:
            case WGPUErrorType_OutOfMemory:
            case WGPUErrorType_Unknown:
            case WGPUErrorType_DeviceLost:
                break;
            default:
                return false;
        }

        auto requestIt = mErrorScopes.find(requestSerial);
        if (requestIt == mErrorScopes.end()) {
            return false;
        }

        ErrorScopeData request = std::move(requestIt->second);

        mErrorScopes.erase(requestIt);
        request.callback(type, message, request.userdata);
        return true;
    }

    void Device::InjectError(WGPUErrorType type, const char* message) {
        DeviceInjectErrorCmd cmd;
        cmd.self = ToAPI(this);
        cmd.type = type;
        cmd.message = message;
        mClient->SerializeCommand(cmd);
    }

    WGPUBuffer Device::CreateBuffer(const WGPUBufferDescriptor* descriptor) {
        if (descriptor->mappedAtCreation) {
            return CreateBufferMapped(descriptor).buffer;
        } else {
            return Buffer::Create(this, descriptor);
        }
    }

    WGPUCreateBufferMappedResult Device::CreateBufferMapped(
        const WGPUBufferDescriptor* descriptor) {
        return Buffer::CreateMapped(this, descriptor);
    }

    WGPUBuffer Device::CreateErrorBuffer() {
        return Buffer::CreateError(this);
    }

    WGPUQueue Device::GetDefaultQueue() {
        mDefaultQueue->refcount++;
        return ToAPI(mDefaultQueue);
    }

}}  // namespace dawn_wire::client
