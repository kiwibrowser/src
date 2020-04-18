// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/client/host_experiment_sender.h"

#include "remoting/base/constants.h"

namespace remoting {

HostExperimentSender::HostExperimentSender(const std::string& experiment_config)
    : experiment_config_(experiment_config) {}

std::unique_ptr<buzz::XmlElement> HostExperimentSender::GetNextMessage() {
  if (message_sent_ || experiment_config_.empty()) {
    return nullptr;
  }
  message_sent_ = true;
  std::unique_ptr<buzz::XmlElement> configuration(new buzz::XmlElement(
      buzz::QName(kChromotingXmlNamespace, "host-configuration")));
  configuration->SetBodyText(experiment_config_);
  return configuration;
}

void HostExperimentSender::OnIncomingMessage(
    const buzz::XmlElement& attachments) {}

}  // namespace remoting
