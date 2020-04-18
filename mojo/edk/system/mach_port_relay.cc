// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/edk/system/mach_port_relay.h"

#include <mach/mach.h>

#include <utility>

#include "base/logging.h"
#include "base/mac/mach_port_util.h"
#include "base/mac/scoped_mach_port.h"
#include "base/metrics/histogram_macros.h"
#include "base/process/process.h"
#include "mojo/edk/embedder/scoped_platform_handle.h"

namespace mojo {
namespace edk {

namespace {

// Errors that can occur in the broker (privileged parent) process.
// These match tools/metrics/histograms.xml.
// This enum is append-only.
enum class BrokerUMAError : int {
  SUCCESS = 0,
  // Couldn't get a task port for the process with a given pid.
  ERROR_TASK_FOR_PID = 1,
  // Couldn't make a port with receive rights in the destination process.
  ERROR_MAKE_RECEIVE_PORT = 2,
  // Couldn't change the attributes of a Mach port.
  ERROR_SET_ATTRIBUTES = 3,
  // Couldn't extract a right from the destination.
  ERROR_EXTRACT_DEST_RIGHT = 4,
  // Couldn't send a Mach port in a call to mach_msg().
  ERROR_SEND_MACH_PORT = 5,
  // Couldn't extract a right from the source.
  ERROR_EXTRACT_SOURCE_RIGHT = 6,
  ERROR_MAX
};

// Errors that can occur in a child process.
// These match tools/metrics/histograms.xml.
// This enum is append-only.
enum class ChildUMAError : int {
  SUCCESS = 0,
  // An error occurred while trying to receive a Mach port with mach_msg().
  ERROR_RECEIVE_MACH_MESSAGE = 1,
  ERROR_MAX
};

void ReportBrokerError(BrokerUMAError error) {
  UMA_HISTOGRAM_ENUMERATION("Mojo.MachPortRelay.BrokerError",
                            static_cast<int>(error),
                            static_cast<int>(BrokerUMAError::ERROR_MAX));
}

void ReportChildError(ChildUMAError error) {
  UMA_HISTOGRAM_ENUMERATION("Mojo.MachPortRelay.ChildError",
                            static_cast<int>(error),
                            static_cast<int>(ChildUMAError::ERROR_MAX));
}

}  // namespace

// static
void MachPortRelay::ReceivePorts(
    std::vector<ScopedInternalPlatformHandle>* handles) {
  DCHECK(handles);

  for (auto& handle : *handles) {
    DCHECK(handle.get().type != InternalPlatformHandle::Type::MACH);
    if (handle.get().type != InternalPlatformHandle::Type::MACH_NAME)
      continue;

    handle.get().type = InternalPlatformHandle::Type::MACH;

    // MACH_PORT_NULL doesn't need translation.
    if (handle.get().port == MACH_PORT_NULL)
      continue;

    // TODO(wez): Wrapping handle.get().port in this way causes it to be
    // Free()d via mach_port_mod_refs() - should InternalPlatformHandle also do
    // that if the handle never reaches here, or should this code not be
    // wrapping it?
    base::mac::ScopedMachReceiveRight message_port(handle.get().port);
    base::mac::ScopedMachSendRight received_port(
        base::ReceiveMachPort(message_port.get()));
    handle.get().port = received_port.release();
    if (!handle.is_valid()) {
      ReportChildError(ChildUMAError::ERROR_RECEIVE_MACH_MESSAGE);
      DLOG(ERROR) << "Error receiving mach port";
      continue;
    }

    ReportChildError(ChildUMAError::SUCCESS);
  }
}

MachPortRelay::MachPortRelay(base::PortProvider* port_provider)
    : port_provider_(port_provider) {
  DCHECK(port_provider);
  port_provider_->AddObserver(this);
}

MachPortRelay::~MachPortRelay() {
  port_provider_->RemoveObserver(this);
}

void MachPortRelay::SendPortsToProcess(Channel::Message* message,
                                       base::ProcessHandle process) {
  DCHECK(message);
  mach_port_t task_port = port_provider_->TaskForPid(process);

  std::vector<ScopedInternalPlatformHandle> handles = message->TakeHandles();
  // Message should have handles, otherwise there's no point in calling this
  // function.
  DCHECK(!handles.empty());
  for (auto& handle : handles) {
    DCHECK(handle.get().type != InternalPlatformHandle::Type::MACH_NAME);
    if (handle.get().type != InternalPlatformHandle::Type::MACH)
      continue;

    if (!handle.is_valid()) {
      handle.get().type = InternalPlatformHandle::Type::MACH_NAME;
      continue;
    }

    if (task_port == MACH_PORT_NULL) {
      // Callers check the port provider for the task port before calling this
      // function, in order to queue pending messages. Therefore, if this fails,
      // it should be considered a genuine, bona fide, electrified, six-car
      // error.
      ReportBrokerError(BrokerUMAError::ERROR_TASK_FOR_PID);

      // For MACH_PORT_NULL, use Type::MACH to indicate that no extraction is
      // necessary.
      // TODO(wez): But we're not setting Type::Mach... is the comment above
      // out of date?
      handle.get().port = MACH_PORT_NULL;
      continue;
    }

    mach_port_name_t intermediate_port;
    base::MachCreateError error_code;
    intermediate_port = base::CreateIntermediateMachPort(
        task_port, base::mac::ScopedMachSendRight(handle.get().port),
        &error_code);
    if (intermediate_port == MACH_PORT_NULL) {
      BrokerUMAError uma_error;
      switch (error_code) {
        case base::MachCreateError::ERROR_MAKE_RECEIVE_PORT:
          uma_error = BrokerUMAError::ERROR_MAKE_RECEIVE_PORT;
          break;
        case base::MachCreateError::ERROR_SET_ATTRIBUTES:
          uma_error = BrokerUMAError::ERROR_SET_ATTRIBUTES;
          break;
        case base::MachCreateError::ERROR_EXTRACT_DEST_RIGHT:
          uma_error = BrokerUMAError::ERROR_EXTRACT_DEST_RIGHT;
          break;
        case base::MachCreateError::ERROR_SEND_MACH_PORT:
          uma_error = BrokerUMAError::ERROR_SEND_MACH_PORT;
          break;
      }
      ReportBrokerError(uma_error);
      handle.get().port = MACH_PORT_NULL;
      continue;
    }

    ReportBrokerError(BrokerUMAError::SUCCESS);
    handle.get().port = intermediate_port;
    handle.get().type = InternalPlatformHandle::Type::MACH_NAME;
  }
  message->SetHandles(std::move(handles));
}

void MachPortRelay::ExtractPort(ScopedInternalPlatformHandle* handle,
                                base::ProcessHandle process) {
  DCHECK_EQ(handle->get().type, InternalPlatformHandle::Type::MACH_NAME);
  handle->get().type = InternalPlatformHandle::Type::MACH;

  // No extraction necessary for MACH_PORT_NULL.
  if (!handle->is_valid())
    return;

  mach_port_t task_port = port_provider_->TaskForPid(process);
  if (task_port == MACH_PORT_NULL) {
    ReportBrokerError(BrokerUMAError::ERROR_TASK_FOR_PID);
    handle->get().port = MACH_PORT_NULL;
    return;
  }

  mach_port_t extracted_right = MACH_PORT_NULL;
  mach_msg_type_name_t extracted_right_type;
  kern_return_t kr = mach_port_extract_right(
      task_port, handle->get().port, MACH_MSG_TYPE_MOVE_SEND, &extracted_right,
      &extracted_right_type);
  if (kr != KERN_SUCCESS) {
    ReportBrokerError(BrokerUMAError::ERROR_EXTRACT_SOURCE_RIGHT);
    handle->get().port = MACH_PORT_NULL;
    return;
  }

  ReportBrokerError(BrokerUMAError::SUCCESS);
  DCHECK_EQ(static_cast<mach_msg_type_name_t>(MACH_MSG_TYPE_PORT_SEND),
            extracted_right_type);
  handle->get().port = extracted_right;
}

void MachPortRelay::AddObserver(Observer* observer) {
  base::AutoLock locker(observers_lock_);
  bool inserted = observers_.insert(observer).second;
  DCHECK(inserted);
}

void MachPortRelay::RemoveObserver(Observer* observer) {
  base::AutoLock locker(observers_lock_);
  observers_.erase(observer);
}

void MachPortRelay::OnReceivedTaskPort(base::ProcessHandle process) {
  base::AutoLock locker(observers_lock_);
  for (auto* observer : observers_)
    observer->OnProcessReady(process);
}

}  // namespace edk
}  // namespace mojo
