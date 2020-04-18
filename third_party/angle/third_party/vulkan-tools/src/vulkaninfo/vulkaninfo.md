<!-- markdownlint-disable MD041 -->
<!-- Copyright 2015-2019 LunarG, Inc. -->

[![Khronos Vulkan][1]][2]

[1]: https://vulkan.lunarg.com/img/Vulkan_100px_Dec16.png "https://www.khronos.org/vulkan/"
[2]: https://www.khronos.org/vulkan/

[![Creative Commons][3]][4]

[3]: https://i.creativecommons.org/l/by-nd/4.0/88x31.png "Creative Commons License"
[4]: https://creativecommons.org/licenses/by-nd/4.0/

# Vulkan Information

Vulkan Info is a program provided in the SDK which outputs various types of Vulkan information such as:
- device properties of identified GPUs
- Vulkan extensions supported by each GPU
- recognized layers
- supported image formats and format properties.

## Running Vulkan Info

After downloading and installing the SDK and setting up the runtime environment (see the [Getting Started](./getting_started.md#user-content-download-the-sdk) documentation) you will be able to run the Vulkan Info program from a command prompt.

```
vulkaninfo
```

Executing `vulkaninfo` without specifying the type of output will default to human-readable output to the console.

```
vulkaninfo --html
```

To organize output in a convenient HTML format use the `--html` option. Executing `vulkaninfo` with the `--html` option produces a file called `vulkaninfo.html` and can be found in your build directory.

```
vulkaninfo --json
```

 Use the `--json` option to produce [DevSim-schema](https://schema.khronos.org/vulkan/devsim_1_0_0.json)-compatible JSON output for your device. Additionally, JSON output can be specified with the `-j` option and for multi-GPU systems, a single GPU can be targeted using the `--json=`*`GPU-number`* option where the *`GPU-number`* indicates the GPU of interest (e.g., `--json=0`). To determine the GPU number corresponding to a particular GPU, execute `vulkaninfo` with the `--html` option (or none at all) first; doing so will summarize all GPUs in the system.
 The generated configuration information can be used as input for the [`VK_LAYER_LUNARG_device_simulation`](./device_simulation_layer.html) layer.


 Use the `--help` or `-h` option to produce a list of all available Vulkan Info options.
```
vulkaninfo - Summarize Vulkan information in relation to the current environment.

USAGE: ./vulkaninfo [options]

OPTIONS:
-h, --help            Print this help.
--html                Produce an html version of vulkaninfo output, saved as
                      "vulkaninfo.html" in the directory in which the command is
                      run.
-j, --json            Produce a json version of vulkaninfo output to standard
                      output.
--json=<gpu-number>   For a multi-gpu system, a single gpu can be targetted by
                      specifying the gpu-number associated with the gpu of
                      interest. This number can be determined by running
                      vulkaninfo without any options specified.
```

### Windows

Vulkan Info can also be found as a shortcut under the Start Menu.
* `Start Menu -> Vulkan SDK`*`version`*`-> vulkaninfo`

Note: In order to review and/or save the output produced when using Visual Studio execute `vulkaninfo` with the JSON option, you will have to redirect output to a file by modifying the command line arguments in the debug options.
