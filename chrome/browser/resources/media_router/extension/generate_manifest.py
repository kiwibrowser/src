# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This script generates manifest.json from manifest.yaml."""

import argparse
import json
import re


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument("--manifest_in")
  parser.add_argument("--output_dir")
  args = parser.parse_args()

  # Load the input file, removing YAML-style comment lines to produce
  # valid JSON.
  json_data = ''
  with open(args.manifest_in) as manifest_in:
    for line in manifest_in:
      if re.match("^ *#", line):
        # Insert an empty line so line numbers aren't changed.
        json_data += '\n'
      else:
        json_data += line

  # Verify that the result is valid JSON.
  json.loads(json_data)

  # Dump the output to the requested location.
  with open(args.output_dir + "/manifest.json", "w") as manifest_out:
    manifest_out.writelines(json_data)


main()
