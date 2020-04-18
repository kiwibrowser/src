#!/bin/bash

# Note that the Validation Layers must be installed in the default system
# paths and/or VK_LAYER_PATH must point to Validation Layers to run this test.

pushd $(dirname "$0") > /dev/null

vk_layer_path=$VK_LAYER_PATH:`pwd`/layers
ld_library_path=$LD_LIBRARY_PATH:`pwd`/layers

# Check for insertion of wrap-objects layer.
output=$(VK_LAYER_PATH=$vk_layer_path \
   LD_LIBRARY_PATH=$ld_library_path \
   VK_INSTANCE_LAYERS=VK_LAYER_LUNARG_wrap_objects \
   VK_LOADER_DEBUG=all \
   GTEST_FILTER=WrapObjects.Insert \
   ./vk_loader_validation_tests 2>&1)

echo "$output" | grep -q "Insert instance layer VK_LAYER_LUNARG_wrap_objects"
ec=$?

if [ $ec -eq 1 ]
then
   echo "Insertion test FAILED - wrap-objects not detected in instance layers" >&2
   exit 1
fi

echo "$output" | grep -q "Inserted device layer VK_LAYER_LUNARG_wrap_objects"
ec=$?

if [ $ec -eq 1 ]
then
   echo "Insertion test FAILED - wrap-objects not detected in device layers" >&2
   exit 1
fi
echo "Insertion test PASSED"

# Check for insertion of wrap-objects layer in front.
output=$(VK_LAYER_PATH=$vk_layer_path \
   LD_LIBRARY_PATH=$ld_library_path \
   VK_INSTANCE_LAYERS=VK_LAYER_LUNARG_parameter_validation:VK_LAYER_LUNARG_wrap_objects \
   VK_LOADER_DEBUG=all \
   GTEST_FILTER=WrapObjects.Insert \
   ./vk_loader_validation_tests 2>&1)

echo "$output" | grep -q "Insert instance layer VK_LAYER_LUNARG_wrap_objects"
ec=$?

if [ $ec -eq 1 ]
then
   echo "Front insertion test FAILED - wrap-objects not detected in instance layers" >&2
   exit 1
fi

echo "$output" | grep -q "Inserted device layer VK_LAYER_LUNARG_wrap_objects"
ec=$?

if [ $ec -eq 1 ]
then
   echo "Front insertion test FAILED - wrap-objects not detected in device layers" >&2
   exit 1
fi
echo "Front insertion test PASSED"

# Check for insertion of wrap-objects layer in back.
output=$(VK_LAYER_PATH=$vk_layer_path \
   LD_LIBRARY_PATH=$ld_library_path \
   VK_INSTANCE_LAYERS=VK_LAYER_LUNARG_wrap_objects:VK_LAYER_LUNARG_parameter_validation \
   VK_LOADER_DEBUG=all \
   GTEST_FILTER=WrapObjects.Insert \
   ./vk_loader_validation_tests 2>&1)

echo "$output" | grep -q "Insert instance layer VK_LAYER_LUNARG_wrap_objects"
ec=$?

if [ $ec -eq 1 ]
then
   echo "Back insertion test FAILED - wrap-objects not detected in instance layers" >&2
   exit 1
fi

echo "$output" | grep -q "Inserted device layer VK_LAYER_LUNARG_wrap_objects"
ec=$?

if [ $ec -eq 1 ]
then
   echo "Back insertion test FAILED - wrap-objects not detected in device layers" >&2
   exit 1
fi
echo "Back insertion test PASSED"

# Check for insertion of wrap-objects layer in middle.
output=$(VK_LAYER_PATH=$vk_layer_path \
   LD_LIBRARY_PATH=$ld_library_path \
   VK_INSTANCE_LAYERS=VK_LAYER_LUNARG_core_validation:VK_LAYER_LUNARG_wrap_objects:VK_LAYER_LUNARG_parameter_validation \
   VK_LOADER_DEBUG=all \
   GTEST_FILTER=WrapObjects.Insert \
   ./vk_loader_validation_tests 2>&1)

echo "$output" | grep -q "Insert instance layer VK_LAYER_LUNARG_wrap_objects"
ec=$?

if [ $ec -eq 1 ]
then
   echo "Middle insertion test FAILED - wrap-objects not detected in instance layers" >&2
   exit 1
fi

echo "$output" | grep -q "Inserted device layer VK_LAYER_LUNARG_wrap_objects"
ec=$?

if [ $ec -eq 1 ]
then
   echo "Middle insertion test FAILED - wrap-objects not detected in device layers" >&2
   exit 1
fi
echo "Middle insertion test PASSED"

popd > /dev/null

exit 0
