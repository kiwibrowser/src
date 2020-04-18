/*
 * Copyright (c) 2015-2016 The Khronos Group Inc.
 * Copyright (c) 2015-2016 Valve Corporation
 * Copyright (c) 2015-2016 LunarG, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: Chia-I Wu <olvaffe@gmail.com>
 * Author: Chris Forbes <chrisf@ijw.co.nz>
 * Author: Courtney Goeltzenleuchter <courtney@LunarG.com>
 * Author: Mark Lobodzinski <mark@lunarg.com>
 * Author: Mike Stroyan <mike@LunarG.com>
 * Author: Tobin Ehlis <tobine@google.com>
 * Author: Tony Barbour <tony@LunarG.com>
 */

#ifndef TEST_ENVIRONMENT_H
#define TEST_ENVIRONMENT_H

#include "vktestbinding.h"

namespace vk_testing {
class Environment : public ::testing::Environment {
  public:
    Environment();

    bool parse_args(int argc, char **argv);

    virtual void SetUp();
    virtual void TearDown();

    const std::vector<Device *> &devices() { return devs_; }
    Device &default_device() { return *(devs_[default_dev_]); }
    VkInstance get_instance() { return inst; }
    VkPhysicalDevice gpus[16];

  private:
    VkApplicationInfo app_;
    uint32_t default_dev_;
    VkInstance inst;

    std::vector<Device *> devs_;
};
}
#endif // TEST_ENVIRONMENT_H
