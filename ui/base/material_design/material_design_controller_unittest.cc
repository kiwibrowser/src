// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/command_line.h"
#include "base/macros.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/material_design/material_design_controller.h"
#include "ui/base/test/material_design_controller_test_api.h"
#include "ui/base/ui_base_switches.h"

namespace ui {
namespace {

// Test fixture for the MaterialDesignController class.
class MaterialDesignControllerTest : public testing::Test {
 public:
  MaterialDesignControllerTest();
  ~MaterialDesignControllerTest() override;

 protected:
  // testing::Test:
  void SetUp() override;
  void TearDown() override;
  void SetCommandLineSwitch(const std::string& value_string);

 private:
  DISALLOW_COPY_AND_ASSIGN(MaterialDesignControllerTest);
};

MaterialDesignControllerTest::MaterialDesignControllerTest() {
}

MaterialDesignControllerTest::~MaterialDesignControllerTest() {
}

void MaterialDesignControllerTest::SetUp() {
  testing::Test::SetUp();
  MaterialDesignController::Initialize();
}

void MaterialDesignControllerTest::TearDown() {
  test::MaterialDesignControllerTestAPI::Uninitialize();
  testing::Test::TearDown();
}

void MaterialDesignControllerTest::SetCommandLineSwitch(
    const std::string& value_string) {
  base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
      switches::kTopChromeMD, value_string);
}

class MaterialDesignControllerTestMaterial :
    public MaterialDesignControllerTest {
 public:
  MaterialDesignControllerTestMaterial() {
    SetCommandLineSwitch("material");
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(MaterialDesignControllerTestMaterial);
};

class MaterialDesignControllerTestHybrid : public MaterialDesignControllerTest {
 public:
  MaterialDesignControllerTestHybrid() {
    SetCommandLineSwitch("material-hybrid");
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(MaterialDesignControllerTestHybrid);
};

class MaterialDesignControllerTestDefault :
    public MaterialDesignControllerTest {
 public:
  MaterialDesignControllerTestDefault() {
    SetCommandLineSwitch(std::string());
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(MaterialDesignControllerTestDefault);
};

class MaterialDesignControllerTestInvalid :
    public MaterialDesignControllerTest {
 public:
  MaterialDesignControllerTestInvalid() {
    const std::string kInvalidValue = "1nvalid-valu3";
    SetCommandLineSwitch(kInvalidValue);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(MaterialDesignControllerTestInvalid);
};

// Verify command line value "material" maps to Mode::MATERIAL when the compile
// time flag is defined.
TEST_F(MaterialDesignControllerTestMaterial,
       EnabledCommandLineValueMapsToMaterialModeWhenCompileTimeFlagEnabled) {
  EXPECT_EQ(MaterialDesignController::Mode::MATERIAL_NORMAL,
            MaterialDesignController::GetMode());
}

// Verify command line value "material-hybrid" maps to Mode::MATERIAL_HYBRID
// when the compile time flag is defined.
TEST_F(
    MaterialDesignControllerTestHybrid,
    EnabledHybridCommandLineValueMapsToMaterialHybridModeWhenCompileTimeFlagEnabled) {
  EXPECT_EQ(MaterialDesignController::Mode::MATERIAL_HYBRID,
            MaterialDesignController::GetMode());
}

// Verify command line value "" maps to the default mode when the compile time
// flag is defined.
TEST_F(
    MaterialDesignControllerTestDefault,
    DisabledCommandLineValueMapsToNonMaterialModeWhenCompileTimeFlagEnabled) {
  EXPECT_EQ(MaterialDesignController::DefaultMode(),
            MaterialDesignController::GetMode());
}

// Verify the current mode is reported as the default mode when no command line
// flag is defined.
TEST_F(MaterialDesignControllerTest,
       NoCommandLineValueMapsToNonMaterialModeWhenCompileTimeFlagEnabled) {
  ASSERT_FALSE(base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kTopChromeMD));
  EXPECT_EQ(MaterialDesignController::DefaultMode(),
            MaterialDesignController::GetMode());
}

// Verify an invalid command line value uses the default mode.
TEST_F(MaterialDesignControllerTestInvalid, InvalidCommandLineValue) {
  EXPECT_EQ(MaterialDesignController::DefaultMode(),
            MaterialDesignController::GetMode());
}

}  // namespace
}  // namespace ui
