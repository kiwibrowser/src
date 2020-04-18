// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/browser/renderer_config.h"

#include <utility>

#include "base/bind.h"
#include "base/callback.h"
#include "base/command_line.h"
#include "content/public/common/child_process_host.h"

namespace chromecast {
namespace shell {

namespace {

class RendererConfigImpl : public RendererConfig {
 public:
  RendererConfigImpl(int render_process_id,
                     base::flat_set<std::string> switches,
                     base::flat_map<std::string, std::string> ascii_switches)
      : render_process_id_(render_process_id),
        switches_(std::move(switches)),
        ascii_switches_(std::move(ascii_switches)) {
    DCHECK_NE(content::ChildProcessHost::kInvalidUniqueID, render_process_id);
  }

  int GetRenderProcessId() const override { return render_process_id_; }

  void AppendSwitchesTo(base::CommandLine* command_line) const override {
    DCHECK(command_line);
    for (const std::string& switch_string : switches_) {
      command_line->AppendSwitch(switch_string);
    }
    for (const auto& ascii_switch : ascii_switches_) {
      command_line->AppendSwitchASCII(ascii_switch.first, ascii_switch.second);
    }
  }

 private:
  ~RendererConfigImpl() override = default;

  const int render_process_id_;
  const base::flat_set<std::string> switches_;
  const base::flat_map<std::string /* switch */, std::string /* value */>
      ascii_switches_;

  DISALLOW_COPY_AND_ASSIGN(RendererConfigImpl);
};

}  // namespace

RendererConfig::RendererConfig() = default;
RendererConfig::~RendererConfig() = default;

RendererConfigurator::RendererConfigurator(
    AddRendererConfigCallback add_renderer_config_callback,
    RemoveRendererConfigCallback remove_renderer_config_callback)
    : add_renderer_config_callback_(std::move(add_renderer_config_callback)),
      remove_renderer_config_callback_(
          std::move(remove_renderer_config_callback)) {
  DCHECK(add_renderer_config_callback_);
  DCHECK(remove_renderer_config_callback_);
}

RendererConfigurator::RendererConfigurator(RendererConfigurator&& other)
    : add_renderer_config_callback_(
          std::move(other.add_renderer_config_callback_)),
      remove_renderer_config_callback_(
          std::move(other.remove_renderer_config_callback_)),
      render_process_id_(other.render_process_id_),
      switches_(std::move(other.switches_)),
      ascii_switches_(std::move(other.ascii_switches_)) {
  other.render_process_id_ = content::ChildProcessHost::kInvalidUniqueID;
}

RendererConfigurator::~RendererConfigurator() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!add_renderer_config_callback_ && remove_renderer_config_callback_) {
    std::move(remove_renderer_config_callback_).Run(render_process_id_);
  }
}

void RendererConfigurator::Configure(int render_process_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(add_renderer_config_callback_);
  DCHECK_NE(content::ChildProcessHost::kInvalidUniqueID, render_process_id);
  render_process_id_ = render_process_id;
  std::move(add_renderer_config_callback_)
      .Run(render_process_id_, base::MakeRefCounted<RendererConfigImpl>(
                                   render_process_id, std::move(switches_),
                                   std::move(ascii_switches_)));
}

void RendererConfigurator::AppendSwitch(const std::string& switch_string) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(add_renderer_config_callback_);
  switches_.insert(switch_string);
}

void RendererConfigurator::AppendSwitchASCII(const std::string& switch_string,
                                             const std::string& value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(add_renderer_config_callback_);
  ascii_switches_[switch_string] = value;
}

RendererConfigManager::RendererConfigManager() : weak_factory_(this) {}

RendererConfigManager::~RendererConfigManager() = default;

RendererConfigurator RendererConfigManager::CreateRendererConfigurator() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return RendererConfigurator(
      base::BindOnce(&RendererConfigManager::AddRendererConfig,
                     weak_factory_.GetWeakPtr()),
      base::BindOnce(&RendererConfigManager::RemoveRendererConfig,
                     weak_factory_.GetWeakPtr()));
}

scoped_refptr<const RendererConfig> RendererConfigManager::GetRendererConfig(
    int render_process_id) {
  if (render_process_id == content::ChildProcessHost::kInvalidUniqueID) {
    return nullptr;
  }
  base::AutoLock lock(renderer_configs_lock_);
  auto it = renderer_configs_.find(render_process_id);
  if (it == renderer_configs_.end()) {
    return nullptr;
  }
  return it->second;
}

void RendererConfigManager::AddRendererConfig(
    int render_process_id,
    scoped_refptr<RendererConfig> renderer_config) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK_NE(content::ChildProcessHost::kInvalidUniqueID, render_process_id);
  base::AutoLock lock(renderer_configs_lock_);
  DCHECK_EQ(0U, renderer_configs_.count(render_process_id));
  renderer_configs_.emplace(render_process_id, std::move(renderer_config));
}

void RendererConfigManager::RemoveRendererConfig(int render_process_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  base::AutoLock lock(renderer_configs_lock_);
  DCHECK_NE(0U, renderer_configs_.count(render_process_id));
  renderer_configs_.erase(render_process_id);
}

}  // namespace shell
}  // namespace chromecast
