// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/remote_commands/device_command_set_volume_job.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/json/json_reader.h"
#include "base/syslog_logging.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/values.h"
#include "chromeos/audio/cras_audio_handler.h"
#include "components/policy/proto/device_management_backend.pb.h"

namespace policy {

namespace {

const char kVolumeFieldName[] = "volume";

}  // namespace

DeviceCommandSetVolumeJob::DeviceCommandSetVolumeJob() {}

DeviceCommandSetVolumeJob::~DeviceCommandSetVolumeJob() {}

enterprise_management::RemoteCommand_Type DeviceCommandSetVolumeJob::GetType()
    const {
  return enterprise_management::RemoteCommand_Type_DEVICE_SET_VOLUME;
}

bool DeviceCommandSetVolumeJob::ParseCommandPayload(
    const std::string& command_payload) {
  std::unique_ptr<base::Value> root(
      base::JSONReader().ReadToValue(command_payload));
  if (!root.get())
    return false;
  base::DictionaryValue* payload = nullptr;
  if (!root->GetAsDictionary(&payload))
    return false;
  if (!payload->GetInteger(kVolumeFieldName, &volume_))
    return false;
  if (volume_ < 0 || volume_ > 100)
    return false;
  return true;
}

void DeviceCommandSetVolumeJob::RunImpl(
    const CallbackWithResult& succeeded_callback,
    const CallbackWithResult& failed_callback) {
  SYSLOG(INFO) << "Running set volume command, volume = " << volume_;
  auto* audio_handler = chromeos::CrasAudioHandler::Get();
  audio_handler->SetOutputVolumePercent(volume_);
  bool mute = audio_handler->IsOutputVolumeBelowDefaultMuteLevel();
  audio_handler->SetOutputMute(mute);

  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(succeeded_callback, nullptr));
}

}  // namespace policy
