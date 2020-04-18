#!/bin/sh
# Copyright 2012 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Tool to output Java classes for the protocol buffers used by the invalidation
# client into the generated-protos/ directory.
TOOL=${PROTOC- `which protoc`}
mkdir -p generated-protos/
$TOOL --javanano_out=optional_field_style=reftypes:generated-protos/ ../proto/* --proto_path=../proto/
EXAMPLE_PATH=../javaexample/com/google/ipc/invalidation/examples/android2
$TOOL --javanano_out=optional_field_style=reftypes:generated-protos/ $EXAMPLE_PATH/example_listener.proto --proto_path=$EXAMPLE_PATH
