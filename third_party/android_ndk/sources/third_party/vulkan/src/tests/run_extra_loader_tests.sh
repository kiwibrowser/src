#!/bin/bash

pushd $(dirname "$0") > /dev/null

RunImplicitLayerTest()
{
    # Check for local implicit directory.
    : "${HOME:?}"
    local implicitDirectory="$HOME/.local/share/vulkan/implicit_layer.d"
    if [ ! -d "$implicitDirectory" ]
    then
        mkdir -p "$implicitDirectory"
    fi

    # Check for the shared object.
    local sharedObject="libVkLayer_test.so"
    local layerDirectory="./layers"
    if [ ! -f "$layerDirectory/$sharedObject" ]
    then
        echo "The file, $layerDirectory/$sharedObject, can not be found." >&2
        return 1
    fi

    # Check for the json which does not include the optional enable environment variable.
    local json="VkLayer_test.json"
    if [ ! -f "$layerDirectory/$json" ]
    then
        echo "The file, $layerDirectory/$json, can not be found." >&2
        return 1
    fi

    # Copy the test layer into the implicit directory.
    if ! cp "$layerDirectory/$sharedObject" "$implicitDirectory/" || ! cp "$layerDirectory/$json" "$implicitDirectory/"
    then
        echo "unable to install test layer" >&2
        return 1
    fi

    # Test without setting enable environment variable. The loader should not load the layer.
    output=$(GTEST_FILTER=ImplicitLayer.Present \
        ./vk_loader_validation_tests 2>&1)
    if echo "$output" | grep -q "VK_LAYER_LUNARG_test: CreateInstance"
    then
       echo "test layer detected but enable environment variable was not set" >&2
       return 1
    fi

    # Test enable environment variable with good value. The loader should load the layer.
    output=$(ENABLE_LAYER_TEST_1=enable \
        GTEST_FILTER=ImplicitLayer.Present \
        ./vk_loader_validation_tests 2>&1)
    if ! echo "$output" | grep -q "VK_LAYER_LUNARG_test: CreateInstance"
    then
       echo "test layer not detected" >&2
       return 1
    fi

    # Test enable environment variable with bad value. The loader should not load the layer.
    output=$(ENABLE_LAYER_TEST_1=wrong \
        GTEST_FILTER=ImplicitLayer.Present \
        ./vk_loader_validation_tests 2>&1)
    if echo "$output" | grep -q "VK_LAYER_LUNARG_test: CreateInstance"
    then
       echo "test layer detected but enable environment variable was set to wrong value" >&2
       return 1
    fi

    # Test disable environment variable. The loader should not load the layer.
    output=$(DISABLE_LAYER_TEST_1=value \
        GTEST_FILTER=ImplicitLayer.Present \
        ./vk_loader_validation_tests 2>&1)
    if echo "$output" | grep -q "VK_LAYER_LUNARG_test: CreateInstance"
    then
       echo "test layer detected but disable environment variable was set" >&2
       return 1
    fi

    # Remove the enable environment variable.
    if ! sed -i '/enable_environment\|ENABLE_LAYER_TEST_1\|},/d' "$implicitDirectory/$json"
    then
        echo "unable to remove enable environment variable" >&2
        return 1
    fi

    # Test without setting enable environment variable. The loader should load the layer.
    output=$(GTEST_FILTER=ImplicitLayer.Present \
        ./vk_loader_validation_tests 2>&1)
    if ! echo "$output" | grep -q "VK_LAYER_LUNARG_test: CreateInstance"
    then
       echo "test layer not detected" >&2
       return 1
    fi

    # Remove the test layer.
    if ! rm "$implicitDirectory/$sharedObject" || ! rm "$implicitDirectory/$json"
    then
        echo "unable to uninstall test layer" >&2
        return 1
    fi

    echo "ImplicitLayer test PASSED"
}

! RunImplicitLayerTest && echo "ImplicitLayer test FAILED" >&2 && exit 1

popd > /dev/null
