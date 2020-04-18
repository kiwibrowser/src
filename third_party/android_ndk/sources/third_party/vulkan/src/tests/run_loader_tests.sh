#!/bin/bash

pushd $(dirname "$0") > /dev/null

RunCreateInstanceTest()
{
    # Check for layer insertion via CreateInstance.
    output=$(VK_LOADER_DEBUG=all \
       GTEST_FILTER=CreateInstance.LayerPresent \
       ./vk_loader_validation_tests 2>&1)

    echo "$output" | grep -q "Insert instance layer VK_LAYER_LUNARG_parameter_validation"
    ec=$?

    if [ $ec -eq 1 ]
    then
       echo "CreateInstance insertion test FAILED - parameter-validation not detected in instance layers" >&2
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

./vk_loader_validation_tests

RunCreateInstanceTest
RunEnumerateInstanceLayerPropertiesTest
RunEnumerateInstanceExtensionPropertiesTest

# Test the wrap objects layer.
./run_wrap_objects_tests.sh || exit 1

popd > /dev/null
