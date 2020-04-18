// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/services/assistant/utils.h"

#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/sys_info.h"
#include "base/values.h"
#include "chromeos/assistant/internal/internal_constants.h"
#include "chromeos/system/version_loader.h"

namespace chromeos {
namespace assistant {

std::string CreateLibAssistantConfig() {
  using Value = base::Value;
  using Type = base::Value::Type;

  Value config(Type::DICTIONARY);

  Value device(Type::DICTIONARY);
  device.SetKey("board_name", Value(base::SysInfo::GetLsbReleaseBoard()));
  device.SetKey("board_revision", Value("1"));
  device.SetKey("embedder_build_info",
                Value(chromeos::version_loader::GetVersion(
                    chromeos::version_loader::VERSION_FULL)));
  device.SetKey("model_id", Value(kModelId));
  device.SetKey("model_revision", Value(1));
  config.SetKey("device", std::move(device));

  Value discovery(Type::DICTIONARY);
  discovery.SetKey("enable_mdns", Value(false));
  config.SetKey("discovery", std::move(discovery));

  Value internal(Type::DICTIONARY);
  internal.SetKey("disable_log_files", Value(true));
  config.SetKey("internal", std::move(internal));

  std::string json;
  base::JSONWriter::Write(config, &json);
  return json;
}

}  // namespace assistant
}  // namespace chromeos
