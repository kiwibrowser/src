#!/usr/bin/env python3
# Copyright 2018 The Dawn Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
import os, sys, json

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: extract_json.py JSON DIR")
        sys.exit(1)

    with open(sys.argv[1]) as f:
        files = json.loads(f.read())

    output_dir = sys.argv[2]

    for (name, content) in files.items():
        output_file = output_dir + os.path.sep + name

        directory = os.path.dirname(output_file)
        if not os.path.exists(directory):
            os.makedirs(directory)

        with open(output_file, 'w') as outfile:
            outfile.write(content)
