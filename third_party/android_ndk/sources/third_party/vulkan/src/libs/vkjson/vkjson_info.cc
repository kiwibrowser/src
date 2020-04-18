///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2015-2016 The Khronos Group Inc.
// Copyright (c) 2015-2016 Valve Corporation
// Copyright (c) 2015-2016 LunarG, Inc.
// Copyright (c) 2015-2016 Google, Inc.
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
///////////////////////////////////////////////////////////////////////////////

#define VK_PROTOTYPES
#include "vkjson.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <iostream>
#include <vector>

const uint32_t unsignedNegOne = (uint32_t)(-1);

struct Options {
  bool instance = false;
  uint32_t device_index = unsignedNegOne;
  std::string device_name;
  std::string output_file;
};

bool ParseOptions(int argc, char* argv[], Options* options) {
  for (int i = 1; i < argc; ++i) {
    std::string arg(argv[i]);
    if (arg == "--instance" || arg == "-i") {
      options->instance = true;
    } else if (arg == "--first" || arg == "-f") {
      options->device_index = 0;
    } else {
      ++i;
      if (i >= argc) {
        std::cerr << "Missing parameter after: " << arg << std::endl;
        return false;
      }
      std::string arg2(argv[i]);
      if (arg == "--device-index" || arg == "-d") {
        int result = sscanf(arg2.c_str(), "%u", &options->device_index);
        if (result != 1) {
          options->device_index = -1;
          std::cerr << "Unable to parse index: " << arg2 << std::endl;
          return false;
        }
      } else if (arg == "--device-name" || arg == "-n") {
        options->device_name = arg2;
      } else if (arg == "--output" || arg == "-o") {
        options->output_file = arg2;
      } else {
        std::cerr << "Unknown argument: " << arg << std::endl;
        return false;
      }
    }
  }
  if (options->instance && (options->device_index != unsignedNegOne ||
                            !options->device_name.empty())) {
    std::cerr << "Specifying a specific device is incompatible with dumping "
                 "the whole instance." << std::endl;
    return false;
  }
  if (options->device_index != unsignedNegOne && !options->device_name.empty()) {
    std::cerr << "Must specify only one of device index and device name."
              << std::endl;
    return false;
  }
  if (options->instance && options->output_file.empty()) {
    std::cerr << "Must specify an output file when dumping the whole instance."
              << std::endl;
    return false;
  }
  if (!options->output_file.empty() && !options->instance &&
      options->device_index == unsignedNegOne && options->device_name.empty()) {
    std::cerr << "Must specify instance, device index, or device name when "
                 "specifying "
                 "output file." << std::endl;
    return false;
  }
  return true;
}

bool Dump(const VkJsonInstance& instance, const Options& options) {
  const VkJsonDevice* out_device = nullptr;
  if (options.device_index != unsignedNegOne) {
    if (static_cast<uint32_t>(options.device_index) >=
        instance.devices.size()) {
      std::cerr << "Error: device " << options.device_index
                << " requested but only " << instance.devices.size()
                << " devices found." << std::endl;
      return false;
    }
    out_device = &instance.devices[options.device_index];
  } else if (!options.device_name.empty()) {
    for (const auto& device : instance.devices) {
      if (device.properties.deviceName == options.device_name) {
        out_device = &device;
      }
    }
    if (!out_device) {
      std::cerr << "Error: device '" << options.device_name
                << "' requested but not found." << std::endl;
      return false;
    }
  }

  std::string output_file;
  if (options.output_file.empty()) {
    assert(out_device);
    output_file.assign(out_device->properties.deviceName);
    output_file.append(".json");
  } else {
    output_file = options.output_file;
  }
  FILE* file = nullptr;
  if (output_file == "-") {
    file = stdout;
  } else {
    file = fopen(output_file.c_str(), "w");
    if (!file) {
      std::cerr << "Unable to open file " << output_file << "." << std::endl;
      return false;
    }
  }

  std::string json = out_device ? VkJsonDeviceToJson(*out_device)
                                : VkJsonInstanceToJson(instance);
  fwrite(json.data(), 1, json.size(), file);
  fputc('\n', file);

  if (output_file != "-") {
    fclose(file);
    std::cout << "Wrote file " << output_file;
    if (out_device)
      std::cout << " for device " << out_device->properties.deviceName;
    std::cout << "." << std::endl;
  }
  return true;
}

int main(int argc, char* argv[]) {
  Options options;
  if (!ParseOptions(argc, argv, &options))
    return 1;

  VkJsonInstance instance = VkJsonGetInstance();
  if (options.instance || options.device_index != unsignedNegOne ||
      !options.device_name.empty()) {
    Dump(instance, options);
  } else {
    for (uint32_t i = 0, n = static_cast<uint32_t>(instance.devices.size()); i < n; i++) {
      options.device_index = i;
      Dump(instance, options);
    }
  }

  return 0;
}
