# Layer Description and Status

## Layer Library Interface

All layer libraries must support the layer library interface defined in
[`LoaderAndLayerInterface.md`][].

[`LoaderAndLayerInterface.md`]: ../loader/LoaderAndLayerInterface.md#layer-library-interface

## Overview

Layer libraries can be written to intercept or hook VK entry points for various
debug and validation purposes.  One or more VK entry points can be defined in your Layer
library.  Undefined entrypoints in the Layer library will be passed to the next Layer which
may be the driver.  Multiple layer libraries can be chained (actually a hierarchy) together.
vkEnumerateInstanceLayerProperties can be called to list the
available layers and their properties.  Layers can intercept all Vulkan commands
that take a dispatchable object as it's first argument. I.e.  VkInstance, VkPhysicalDevice,
VkDevice, VkCommandBuffer, and VkQueue.
vkXXXXGetProcAddr is used internally by the Layers and Loader to initialize dispatch tables.
Layers can also be activated via the VK_INSTANCE_LAYERS environment variable.

All validation layers work with the DEBUG_REPORT extension to provide validation feedback.
When a validation layer is enabled, it will look for a vk_layer_settings.txt file (see"Using
Layers" section below for more details) to define its logging behavior, which can include
sending output to a file, stdout, or debug output (Windows). Applications can also register
debug callback functions via the DEBUG_REPORT extension to receive callbacks when validation
events occur. Application callbacks are independent of settings in a vk_layer_settings.txt
file which will be carried out separately. If no vk_layer_settings.txt file is present and
no application callbacks are registered, error messages will be output through default
logging callbacks.

### Layer library example code

Note that some layers are code-generated and will therefore exist in the directory `(build_dir)/layers`

`include/vkLayer.h` - header file for layer code.

### Layer Details
For complete details of current validation layers, including all of the validation checks that they perform, please refer to the document `layers/vk_validation_layer_details.md`. Below is a brief overview of each layer.

### Standard Validation
This is a meta-layer managed by the loader. (name = `VK_LAYER_LUNARG_standard_validation`) - specifying this layer name will cause the loader to load the all of the standard validation layers (listed below) in the following optimal order:  `VK_LAYER_GOOGLE_threading`, `VK_LAYER_LUNARG_parameter_validation`, `VK_LAYER_LUNARG_object_tracker`, `VK_LAYER_LUNARG_image`, `VK_LAYER_LUNARG_core_validation`,` VK_LAYER_LUNARG_swapchain`, and `VK_LAYER_GOOGLE_unique_objects`. Other layers can be specified and the loader will remove duplicates.

### Object Validation and Statistics
(build dir)/layers/object_tracker.cpp (name=`VK_LAYER_LUNARG_object_tracker`) - Track object creation, use, and destruction. As objects are created they are stored in a map. As objects are used the layer verifies they exist in the map, flagging errors for unknown objects. As objects are destroyed they are removed from the map. At `vkDestroyDevice()` and `vkDestroyInstance()` times, if any objects have not been destroyed they are reported as leaked objects. If a Dbg callback function is registered this layer will use callback function(s) for reporting, otherwise it will use stdout.

### Validate API State and Shaders
layers/core\_validation.cpp (name=`VK_LAYER_LUNARG_core_validation`) - The core\_validation layer does the bulk of the API validation that requires storing state. Some of the state it tracks includes the Descriptor Set, Pipeline State, Shaders, and dynamic state, and memory objects and bindings. It performs some point validation as states are created and used, and further validation Draw call and QueueSubmit time. Of primary interest is making sure that the resources bound to Descriptor Sets correctly align with the layout specified for the Set. Also, all of the image and buffer layouts are validated to make sure explicit layout transitions are properly managed. Related to memory, core\_validation includes tracking object bindings, memory hazards, and memory object lifetimes. It also validates several other hazard-related issues related to command buffers, fences, and memory mapping. Additionally core\_validation include shader validation (formerly separate shader\_checker layer) that inspects the SPIR-V shader images and fixed function pipeline stages at PSO creation time. It flags errors when inconsistencies are found across interfaces between shader stages. The exact behavior of the checks depends on the pair of pipeline stages involved. If a Dbg callback function is registered, this layer will use callback function(s) for reporting, otherwise uses stdout.

### Check parameters
layers/parameter_validation.cpp (name=`VK_LAYER_LUNARG_parameter_validation`) - Check the input parameters to API calls for validity. If a Dbg callback function is registered, this layer will use callback function(s) for reporting, otherwise uses stdout.

### Image Validity
layers/image.cpp (name=`VK_LAYER_LUNARG_image`) - The image layer is intended to validate image parameters, formats, and correct use. Images are a significant enough area that they were given a separate layer. If a Dbg callback function is registered, this layer will use callback function(s) for reporting, otherwise uses stdout.

### Check threading
layers/threading.cpp (name=`VK_LAYER_GOOGLE_threading`) - Check multithreading of API calls for validity. Currently this checks that only one thread at a time uses an object in free-threaded API calls. If a Dbg callback function is registered, this layer will use callback function(s) for reporting, otherwise uses stdout.

### Swapchain
layers/swapchain.cpp (name=`VK_LAYER_LUNARG_swapchain`) - Check that WSI extensions are being used correctly.

### Unique Objects
(build dir)/layers/unique_objects.cpp (name=`VK_LAYER_GOOGLE_unique_objects`) - The Vulkan specification allows objects that have non-unique handles. This makes tracking object lifetimes difficult in that it is unclear which object is being referenced on deletion. The unique_objects layer was created to address this problem. If loaded in the correct position (last, which is closest to the display driver) it will alias all objects with a unique object representation, allowing proper object lifetime tracking. This layer does no validation on its own and may not be required for the proper operation of all layers or all platforms. One sign that it is needed is the appearance of errors emitted from the object_tracker layer indicating the use of previously destroyed objects.

## Using Layers

1. Build VK loader using normal steps (cmake and make)
2. Place `libVkLayer_<name>.so` in the same directory as your VK test or app:

    `cp build/layer/libVkLayer_threading.so  build/tests`

    This is required for the Loader to be able to scan and enumerate your library.
    Alternatively, use the `VK_LAYER_PATH` environment variable to specify where the layer libraries reside.

3. To specify how your layers should behave, create a vk_layer_settings.txt file. This file can exist in the same directory as your app or in a directory given by the `VK_LAYER_SETTINGS_PATH` environment variable. Alternatively, you can use any filename you want if you set `VK_LAYER_SETTINGS_PATH` to the full path of the file, rather than the directory that contains it.

    Model the file after the following example:  [*vk_layer_settings.txt*](vk_layer_settings.txt)

4. Specify which layers to activate using environment variables.

    `export VK\_INSTANCE\_LAYERS=VK\_LAYER\_LUNARG\_parameter\_validation:VK\_LAYER\_LUNARG\_core\_validation`
    `cd build/tests; ./vkinfo`


## Status


### Current known issues

