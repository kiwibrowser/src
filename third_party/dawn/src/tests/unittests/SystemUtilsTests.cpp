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

#include <gtest/gtest.h>

#include "common/SystemUtils.h"

// Tests for GetEnvironmentVar
TEST(SystemUtilsTests, GetEnvironmentVar) {
    // Test nonexistent environment variable
    ASSERT_EQ(GetEnvironmentVar("NonexistentEnvironmentVar"), "");
}

// Tests for SetEnvironmentVar
TEST(SystemUtilsTests, SetEnvironmentVar) {
    // Test new environment variable
    ASSERT_TRUE(SetEnvironmentVar("EnvironmentVarForTest", "NewEnvironmentVarValue"));
    ASSERT_EQ(GetEnvironmentVar("EnvironmentVarForTest"), "NewEnvironmentVarValue");
    // Test override environment variable
    ASSERT_TRUE(SetEnvironmentVar("EnvironmentVarForTest", "OverrideEnvironmentVarValue"));
    ASSERT_EQ(GetEnvironmentVar("EnvironmentVarForTest"), "OverrideEnvironmentVarValue");
}

// Tests for GetExecutableDirectory
TEST(SystemUtilsTests, GetExecutableDirectory) {
    // Test returned value is non-empty string
    ASSERT_NE(GetExecutableDirectory(), "");
    // Test last charecter in path
    ASSERT_EQ(GetExecutableDirectory().back(), *GetPathSeparator());
}

// Tests for ScopedEnvironmentVar
TEST(SystemUtilsTests, ScopedEnvironmentVar) {
    SetEnvironmentVar("ScopedEnvironmentVarForTest", "original");

    // Test empty environment variable doesn't crash
    { ScopedEnvironmentVar var; }

    // Test setting empty environment variable
    {
        ScopedEnvironmentVar var;
        var.Set("ScopedEnvironmentVarForTest", "NewEnvironmentVarValue");
        ASSERT_EQ(GetEnvironmentVar("ScopedEnvironmentVarForTest"), "NewEnvironmentVarValue");
    }
    ASSERT_EQ(GetEnvironmentVar("ScopedEnvironmentVarForTest"), "original");

    // Test that the environment variable can be set, and it is unset at the end of the scope.
    {
        ScopedEnvironmentVar var("ScopedEnvironmentVarForTest", "NewEnvironmentVarValue");
        ASSERT_EQ(GetEnvironmentVar("ScopedEnvironmentVarForTest"), "NewEnvironmentVarValue");
    }
    ASSERT_EQ(GetEnvironmentVar("ScopedEnvironmentVarForTest"), "original");

    // Test nested scopes
    {
        ScopedEnvironmentVar outer("ScopedEnvironmentVarForTest", "outer");
        ASSERT_EQ(GetEnvironmentVar("ScopedEnvironmentVarForTest"), "outer");
        {
            ScopedEnvironmentVar inner("ScopedEnvironmentVarForTest", "inner");
            ASSERT_EQ(GetEnvironmentVar("ScopedEnvironmentVarForTest"), "inner");
        }
        ASSERT_EQ(GetEnvironmentVar("ScopedEnvironmentVarForTest"), "outer");
    }
    ASSERT_EQ(GetEnvironmentVar("ScopedEnvironmentVarForTest"), "original");

    // Test redundantly setting scoped variables
    {
        ScopedEnvironmentVar var1("ScopedEnvironmentVarForTest", "var1");
        ASSERT_EQ(GetEnvironmentVar("ScopedEnvironmentVarForTest"), "var1");

        ScopedEnvironmentVar var2("ScopedEnvironmentVarForTest", "var2");
        ASSERT_EQ(GetEnvironmentVar("ScopedEnvironmentVarForTest"), "var2");
    }
    ASSERT_EQ(GetEnvironmentVar("ScopedEnvironmentVarForTest"), "original");
}
