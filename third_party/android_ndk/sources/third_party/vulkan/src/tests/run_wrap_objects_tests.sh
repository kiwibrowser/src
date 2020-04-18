#!/bin/bash

pushd $(dirname "$0") > /dev/null

# Check for insertion of wrap-objects layer.
output=$(VK_LAYER_PATH=$VK_LAYER_PATH:`pwd`/layers \
   LD_LIBRARY_PATH=$LD_LIBRARY_PATH:`pwd`/layers \
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

echo "$output" | grep -q "Insert device layer VK_LAYER_LUNARG_wrap_objects"
ec=$?

if [ $ec -eq 1 ]
then
   echo "Insertion test FAILED - wrap-objects not detected in device layers" >&2
   exit 1
fi
echo "Insertion test PASSED"

# Check for insertion of wrap-objects layer in front.
output=$(VK_LAYER_PATH=$VK_LAYER_PATH:`pwd`/layers \
   LD_LIBRARY_PATH=$LD_LIBRARY_PATH:`pwd`/layers \
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

echo "$output" | grep -q "Insert device layer VK_LAYER_LUNARG_wrap_objects"
ec=$?

if [ $ec -eq 1 ]
then
   echo "Front insertion test FAILED - wrap-objects not detected in device layers" >&2
   exit 1
fi
echo "Front insertion test PASSED"

# Check for insertion of wrap-objects layer in back.
output=$(VK_LAYER_PATH=$VK_LAYER_PATH:`pwd`/layers \
   LD_LIBRARY_PATH=$LD_LIBRARY_PATH:`pwd`/layers \
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

echo "$output" | grep -q "Insert device layer VK_LAYER_LUNARG_wrap_objects"
ec=$?

if [ $ec -eq 1 ]
then
   echo "Back insertion test FAILED - wrap-objects not detected in device layers" >&2
   exit 1
fi
echo "Back insertion test PASSED"

# Check for insertion of wrap-objects layer in middle.
output=$(VK_LAYER_PATH=$VK_LAYER_PATH:`pwd`/layers \
   LD_LIBRARY_PATH=$LD_LIBRARY_PATH:`pwd`/layers \
   VK_INSTANCE_LAYERS=VK_LAYER_LUNARG_image:VK_LAYER_LUNARG_wrap_objects:VK_LAYER_LUNARG_parameter_validation \
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

echo "$output" | grep -q "Insert device layer VK_LAYER_LUNARG_wrap_objects"
ec=$?

if [ $ec -eq 1 ]
then
   echo "Middle insertion test FAILED - wrap-objects not detected in device layers" >&2
   exit 1
fi
echo "Middle insertion test PASSED"

# Run the layer validation tests with and without the wrap-objects layer. Diff the results.
# Filter out the "Unexpected:" lines because they contain varying object handles.
GTEST_PRINT_TIME=0 \
   ./vk_layer_validation_tests | grep -v "^Unexpected: " > unwrapped.out
GTEST_PRINT_TIME=0 \
   VK_LAYER_PATH=$VK_LAYER_PATH:`pwd`/layers \
   LD_LIBRARY_PATH=$LD_LIBRARY_PATH:`pwd`/layers \
   VK_INSTANCE_LAYERS=VK_LAYER_LUNARG_wrap_objects \
   ./vk_layer_validation_tests | grep -v "^Unexpected: " > wrapped.out
diff unwrapped.out wrapped.out
ec=$?

if [ $ec -eq 1 ]
then
   echo "Wrap-objects layer validation tests FAILED - wrap-objects altered the results of the layer validation tests" >&2
   exit 1
fi
echo "Wrap-objects layer validation tests PASSED"

popd > /dev/null

exit 0
