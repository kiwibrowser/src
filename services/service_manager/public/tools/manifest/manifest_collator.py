#!/usr/bin/env python
# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

""" A collator for Service Manifests """

import argparse
import json
import os
import shutil
import sys
import urlparse


# Keys which are completely overridden by manifest overlays
_MANIFEST_OVERLAY_OVERRIDE_KEYS = [
  "display_name",
]

# Keys which are merged with content from manifest overlays
_MANIFEST_OVERLAY_MERGE_KEYS = [
  "interface_provider_specs",
  "required_files",
]

eater_relative = "../../../../../../tools/json_comment_eater"
eater_relative = os.path.join(os.path.abspath(__file__), eater_relative)
sys.path.insert(0, os.path.normpath(eater_relative))
try:
  import json_comment_eater
finally:
  sys.path.pop(0)


def ParseJSONFile(filename):
  with open(filename) as json_file:
    try:
      return json.loads(json_comment_eater.Nom(json_file.read()))
    except ValueError as e:
      print "%s is not a valid JSON document" % filename
      raise e


def MergeDicts(left, right):
  for k, v in right.iteritems():
    if k not in left:
      left[k] = v
    else:
      if isinstance(v, dict):
        assert isinstance(left[k], dict)
        MergeDicts(left[k], v)
      elif isinstance(v, list):
        assert isinstance(left[k], list)
        left[k].extend(v)
      else:
        raise "Refusing to merge conflicting non-collection values."
  return left


def MergeManifestOverlay(manifest, overlay):

  for key in _MANIFEST_OVERLAY_MERGE_KEYS:
    if key in overlay:
      MergeDicts(manifest[key], overlay[key])

  if "services" in overlay:
    if "services" not in manifest:
      manifest["services"] = []
    manifest["services"].extend(overlay["services"])

  for key in _MANIFEST_OVERLAY_OVERRIDE_KEYS:
    if key in overlay:
      manifest[key] = overlay[key]


def SanityCheckManifestServices(manifest):
  """Ensures any given service name appears only once within a manifest."""
  known_services = set()
  def has_no_dupes(root):
    if "name" in root:
      name = root["name"]
      if name in known_services:
        raise ValueError(
            "Duplicate manifest entry found for service: %s" % name)
      known_services.add(name)
    if "services" not in root:
      return True
    return all(has_no_dupes(service) for service in root["services"])
  return has_no_dupes(manifest)


def main():
  parser = argparse.ArgumentParser(
      description="Collate Service Manifests.")
  parser.add_argument("--parent")
  parser.add_argument("--output")
  parser.add_argument("--name")
  parser.add_argument("--pretty", action="store_true")
  parser.add_argument("--overlays", nargs="+", dest="overlays", default=[])
  parser.add_argument("--packaged-services", nargs="+",
                      dest="packaged_services", default=[])
  args, _ = parser.parse_known_args()

  parent = ParseJSONFile(args.parent)

  service_name = parent["name"] if "name" in parent else ""
  if args.name and args.name != service_name:
    raise ValueError("Service name '%s' specified in build file does not " \
                     "match name '%s' specified in manifest." %
                     (args.name, service_name))

  packaged_services = []
  for child_manifest in args.packaged_services:
    packaged_services.append(ParseJSONFile(child_manifest))

  if len(packaged_services) > 0:
    if "services" not in parent:
      parent["services"] = packaged_services
    else:
      parent["services"].extend(packaged_services)

  for overlay_path in args.overlays:
    MergeManifestOverlay(parent, ParseJSONFile(overlay_path))

  with open(args.output, "w") as output_file:
    json.dump(parent, output_file, indent=2 if args.pretty else -1)

  # NOTE: We do the sanity check and possible failure *after* outputting the
  # aggregate manifest so it's easier to inspect erroneous output.
  SanityCheckManifestServices(parent)

  return 0

if __name__ == "__main__":
  sys.exit(main())
