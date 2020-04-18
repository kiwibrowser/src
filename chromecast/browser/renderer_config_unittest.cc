// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromecast/browser/renderer_config.h"

#include <memory>

#include "base/command_line.h"
#include "base/memory/ref_counted.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromecast {
namespace shell {

namespace {

constexpr char kSwitch1[] = "switch1";
constexpr char kSwitch2[] = "switch2";
constexpr char kValue[] = "value";
constexpr int kRenderProcessId = 123;

}  // namespace

TEST(RendererConfigTest, AppendCommandLineSwitches) {
  RendererConfigManager manager;
  ASSERT_FALSE(manager.GetRendererConfig(kRenderProcessId));
  scoped_refptr<const RendererConfig> config;
  {
    auto configurator = manager.CreateRendererConfigurator();
    configurator.AppendSwitch(kSwitch1);
    configurator.AppendSwitchASCII(kSwitch2, kValue);
    configurator.Configure(kRenderProcessId);
    config = manager.GetRendererConfig(kRenderProcessId);
    ASSERT_TRUE(config);
  }
  EXPECT_FALSE(manager.GetRendererConfig(kRenderProcessId));
  base::CommandLine command_line(base::CommandLine::NO_PROGRAM);
  config->AppendSwitchesTo(&command_line);
  EXPECT_TRUE(command_line.HasSwitch(kSwitch1));
  EXPECT_TRUE(command_line.HasSwitch(kSwitch2));
  EXPECT_EQ(kValue, command_line.GetSwitchValueASCII(kSwitch2));
}

TEST(RendererConfigTest, ConfiguratorOutlivesManager) {
  auto manager = std::make_unique<RendererConfigManager>();
  ASSERT_FALSE(manager->GetRendererConfig(kRenderProcessId));
  auto configurator = manager->CreateRendererConfigurator();
  configurator.Configure(kRenderProcessId);
  EXPECT_TRUE(manager->GetRendererConfig(kRenderProcessId));
  manager.reset();
}

TEST(RendererConfigTest, ConfigureAfterManagerDestroyed) {
  auto manager = std::make_unique<RendererConfigManager>();
  auto configurator = manager->CreateRendererConfigurator();
  manager.reset();
  configurator.Configure(kRenderProcessId);
}

}  // namespace shell
}  // namespace chromecast
