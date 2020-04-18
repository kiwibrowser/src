#!/bin/bash

pushd $(dirname "$0") > /dev/null

RunEnvironmentVariablePathsTest()
{
    # Check for proper handling of paths specified via environment variables.

    # Set up a layer path that includes default and user-specified locations,
    # so that the test app can find them.  Include some badly specified elements as well.
    vk_layer_path="$VK_LAYER_PATH"
    vk_layer_path+=":/usr/local/etc/vulkan/implicit_layer.d:/usr/local/share/vulkan/implicit_layer.d"
    vk_layer_path+=":/tmp/carol:::"
    vk_layer_path+=":/etc/vulkan/implicit_layer.d:/usr/share/vulkan/implicit_layer.d:$HOME/.local/share/vulkan/implicit_layer.d"
    vk_layer_path+=":::::/tandy:"

    # Set vars to include some "challenging" paths and run the test.
    output=$(VK_LOADER_DEBUG=all \
       XDG_CONFIG_DIRS=":/tmp/goober:::::/tmp/goober2/:/tmp/goober3/with spaces:::" \
       XDG_DATA_DIRS="::::/tmp/goober4:::::/tmp/goober5:/tmp/goober6/with spaces::::/tmp/goober7:" \
       VK_LAYER_PATH=${vk_layer_path} \
       GTEST_FILTER=CreateInstance.LayerPresent \
       ./vk_loader_validation_tests 2>&1)

    # Here is a path we expect to find.  The loader constructs these from the XDG* env vars.
    right_path="/tmp/goober/vulkan/icd.d:/tmp/goober2/vulkan/icd.d:/tmp/goober3/with spaces/vulkan/icd.d"
    # There are other paths that come from SYSCONFIG settings established at build time.
    # So we can't really guess at what those are here.
    right_path+=".*"
    # Also expect to find these, since we added them.
    right_path+="/tmp/goober4/vulkan/icd.d:/tmp/goober5/vulkan/icd.d:/tmp/goober6/with spaces/vulkan/icd.d:/tmp/goober7/vulkan/icd.d"
    echo "$output" | grep -q "$right_path"
    ec=$?
    if [ $ec -eq 1 ]
    then
       echo "Environment Variable Path test FAILED - ICD path incorrect" >&2
       exit 1
    fi
    # Change the string to implicit layers.
    right_path=${right_path//icd.d/implicit_layer.d}
    echo "$output" | grep -q "$right_path"
    ec=$?
    if [ $ec -eq 1 ]
    then
       echo "Environment Variable Path test FAILED - Implicit layer path incorrect" >&2
       exit 1
    fi
    # Sadly, the loader does not clean up this path and just stumbles through it.
    # So just make sure it is the same.
    right_path="${vk_layer_path}"
    echo "$output" | grep -q "$right_path"
    ec=$?
    if [ $ec -eq 1 ]
    then
       echo "Environment Variable Path test FAILED - VK_LAYER_PATH incorrect" >&2
       exit 1
    fi
    echo "Environment Variable Path test PASSED"
}

RunCreateInstanceTest()
{
    # Check for layer insertion via CreateInstance.
    output=$(VK_LOADER_DEBUG=all \
       GTEST_FILTER=CreateInstance.LayerPresent \
       ./vk_loader_validation_tests 2>&1)

    echo "$output" | grep -q "Insert instance layer VK_LAYER_LUNARG_test"
    ec=$?

    if [ $ec -eq 1 ]
    then
       echo "CreateInstance insertion test FAILED - test layer not detected in instance layers" >&2
       exit 1
    fi
    echo "CreateInstance Insertion test PASSED"
}

RunEnumerateInstanceLayerPropertiesTest()
{
    count=$(GTEST_FILTER=EnumerateInstanceLayerProperties.Count \
        ./vk_loader_validation_tests count 2>&1 |
        grep -o 'count=[0-9]\+' | sed 's/^.*=//')

    if [ "$count" -gt 1 ]
    then
        diff \
            <(GTEST_PRINT_TIME=0 \
                GTEST_FILTER=EnumerateInstanceLayerProperties.OnePass \
                ./vk_loader_validation_tests count "$count" properties 2>&1 |
                grep 'properties') \
            <(GTEST_PRINT_TIME=0 \
                GTEST_FILTER=EnumerateInstanceLayerProperties.TwoPass \
                ./vk_loader_validation_tests properties 2>&1 |
                grep 'properties')
    fi
    ec=$?

    if [ $ec -eq 1 ]
    then
        echo "EnumerateInstanceLayerProperties OnePass vs TwoPass test FAILED - properties do not match" >&2
        exit 1
    fi
    echo "EnumerateInstanceLayerProperties OnePass vs TwoPass test PASSED"
}

RunEnumerateInstanceExtensionPropertiesTest()
{
    count=$(GTEST_FILTER=EnumerateInstanceExtensionProperties.Count \
        ./vk_loader_validation_tests count 2>&1 |
        grep -o 'count=[0-9]\+' | sed 's/^.*=//')

    if [ "$count" -gt 1 ]
    then
        diff \
            <(GTEST_PRINT_TIME=0 \
                GTEST_FILTER=EnumerateInstanceExtensionProperties.OnePass \
                ./vk_loader_validation_tests count "$count" properties 2>&1 |
                grep 'properties') \
            <(GTEST_PRINT_TIME=0 \
                GTEST_FILTER=EnumerateInstanceExtensionProperties.TwoPass \
                ./vk_loader_validation_tests properties 2>&1 |
                grep 'properties')
    fi
    ec=$?

    if [ $ec -eq 1 ]
    then
        echo "EnumerateInstanceExtensionProperties OnePass vs TwoPass test FAILED - properties do not match" >&2
        exit 1
    fi
    echo "EnumerateInstanceExtensionProperties OnePass vs TwoPass test PASSED"
}

VK_LAYER_PATH="$PWD/layers"
./vk_loader_validation_tests

RunEnvironmentVariablePathsTest
RunCreateInstanceTest
RunEnumerateInstanceLayerPropertiesTest
RunEnumerateInstanceExtensionPropertiesTest

# Test the wrap objects layer.
./run_wrap_objects_tests.sh || exit 1

popd > /dev/null
