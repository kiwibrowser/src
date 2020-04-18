// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/serial/serial_device_enumerator_impl.h"

#include "base/sequenced_task_runner.h"
#include "base/task_scheduler/post_task.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace device {

namespace {

void CreateAndBindOnBlockableRunner(
    mojom::SerialDeviceEnumeratorRequest request) {
  mojo::MakeStrongBinding(std::make_unique<SerialDeviceEnumeratorImpl>(),
                          std::move(request));
}

}  // namespace

// static
void SerialDeviceEnumeratorImpl::Create(
    mojom::SerialDeviceEnumeratorRequest request) {
  // SerialDeviceEnumeratorImpl must live on a thread that is allowed to do
  // blocking IO.
  scoped_refptr<base::SequencedTaskRunner> blockable_sequence_runner =
      base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND});
  blockable_sequence_runner->PostTask(
      FROM_HERE,
      base::BindOnce(&CreateAndBindOnBlockableRunner, std::move(request)));
}

SerialDeviceEnumeratorImpl::SerialDeviceEnumeratorImpl()
    : enumerator_(device::SerialDeviceEnumerator::Create()) {}

SerialDeviceEnumeratorImpl::~SerialDeviceEnumeratorImpl() = default;

void SerialDeviceEnumeratorImpl::GetDevices(GetDevicesCallback callback) {
  DCHECK(enumerator_);
  std::move(callback).Run(enumerator_->GetDevices());
}

}  // namespace device
