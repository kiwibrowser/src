# Copyright 2015 The PDFium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import json
import os
import shlex
import shutil
import urllib2


def _ParseKeyValuePairs(kv_str):
  """
  Parses a string of the type 'key1 value1 key2 value2' into a dict.
  """
  kv_pairs = shlex.split(kv_str)
  if len(kv_pairs) % 2:
    raise ValueError('Uneven number of key/value pairs. Got %s' % kv_str)
  return { kv_pairs[i]:kv_pairs[i + 1] for i in xrange(0, len(kv_pairs), 2) }


# This module downloads a json provided by Skia Gold with the expected baselines
# for each test file.
#
# The expected format for the json is:
# {
#   "commit": {
#     "author": "John Doe (jdoe@chromium.org)",
#     "commit_time": 1510598123,
#     "hash": "cee39e6e90c219cc91f2c94a912a06977f4461a0"
#   },
#   "master": {
#     "abc.pdf.1": {
#       "0ec3d86f545052acd7c9a16fde8ca9d4": 1,
#       "80455b71673becc9fbc100d6da56ca65": 1,
#       "b68e2ecb80090b4502ec89ad1be2322c": 1
#      },
#     "defgh.pdf.0": {
#       "01e020cd4cd05c6738e479a46a506044": 1,
#       "b68e2ecb80090b4502ec89ad1be2322c": 1
#     }
#   },
#   "changeLists": {
#     "18499" : {
#       "abc.pdf.1": {
#         "d5dd649124cf1779152253dc8fb239c5": 1,
#         "42a270581930579cdb0f28674972fb1a": 1,
#       }
#     }
#   }
# }
class GoldBaseline(object):

  def __init__(self, properties_str):
    """
    properties_str is a string with space separated key/value pairs that
               is used to find the cl number for which to baseline
    """
    self._properties = _ParseKeyValuePairs(properties_str)
    self._baselines = self._LoadSkiaGoldBaselines()

  def _LoadSkiaGoldBaselines(self):
    """
    Download the baseline json and return a list of the two baselines that
    should be used to match hashes (master and cl#).
    """
    GOLD_BASELINE_URL = ('https://storage.googleapis.com/skia-infra-gm/'
                         'hash_files/gold-pdfium-baseline.json')
    try:
      response = urllib2.urlopen(GOLD_BASELINE_URL, timeout=2)
      json_data = response.read()
    except (urllib2.HTTPError, urllib2.URLError) as e:
      print ('Error: Unable to read skia gold json from %s: %s'
             % (GOLD_BASELINE_URL, e))
      return None

    try:
      data = json.loads(json_data)
    except ValueError:
      print 'Error: Malformed json read from %s: %s' % (GOLD_BASELINE_URL, e)
      return None

    try:
      master_baseline = data['master']
    except (KeyError, TypeError):
      print ('Error: "master" key not in json read from %s: %s'
             % (GOLD_BASELINE_URL, e))
      return None

    cl_number_str = self._properties.get('issue')
    if cl_number_str is None:
      return [master_baseline]

    try:
      cl_baseline = data['changeLists'][cl_number_str]
    except KeyError:
      return [master_baseline]

    return [cl_baseline, master_baseline]

  # Return values for MatchLocalResult().
  MATCH = 'match'
  MISMATCH = 'mismatch'
  NO_BASELINE = 'no_baseline'
  BASELINE_DOWNLOAD_FAILED = 'baseline_download_failed'

  def MatchLocalResult(self, test_name, md5_hash):
    """
    Match a locally generated hash of a test cases rendered image with the
    expected hashes downloaded in the baselines json.

    Each baseline is a dict mapping the test case name to a dict with the
    expected hashes as keys. Therefore, this list of baselines should be
    searched until the test case name is found, then the hash should be matched
    with the options in that dict. If the hashes don't match, it should be
    considered a failure and we should not continue searching the baseline list.

    Returns MATCH if the md5 provided matches the ones in the baseline json,
    MISMATCH if it does not, NO_BASELINE if the test case has no baseline, or
    BASELINE_DOWNLOAD_FAILED if the baseline could not be downloaded and parsed.
    """
    if self._baselines is None:
      return GoldBaseline.BASELINE_DOWNLOAD_FAILED

    found_test_case = False
    for baseline in self._baselines:
      if test_name in baseline:
        found_test_case = True
        if md5_hash in baseline[test_name]:
          return GoldBaseline.MATCH

    return (GoldBaseline.MISMATCH if found_test_case
            else GoldBaseline.NO_BASELINE)


# This module collects and writes output in a format expected by the
# Gold baseline tool. Based on meta data provided explicitly and by
# adding a series of test results it can be used to produce
# a JSON file that is uploaded to Google Storage and ingested by Gold.
#
# The output will look similar this:
#
# {
#    "build_number" : "2",
#    "gitHash" : "a4a338179013b029d6dd55e737b5bd648a9fb68c",
#    "key" : {
#       "arch" : "arm64",
#       "compiler" : "Clang",
#    },
#    "results" : [
#       {
#          "key" : {
#             "config" : "vk",
#             "name" : "yuv_nv12_to_rgb_effect",
#             "source_type" : "gm"
#          },
#          "md5" : "7db34da246868d50ab9ddd776ce6d779",
#          "options" : {
#             "ext" : "png",
#             "gamma_correct" : "no"
#          }
#       },
#       {
#          "key" : {
#             "config" : "vk",
#             "name" : "yuv_to_rgb_effect",
#             "source_type" : "gm"
#          },
#          "md5" : "0b955f387740c66eb23bf0e253c80d64",
#          "options" : {
#             "ext" : "png",
#             "gamma_correct" : "no"
#          }
#       }
#    ],
# }
#
class GoldResults(object):
  def __init__(self, source_type, outputDir, propertiesStr, keyStr,
               ignore_hashes_file):
    """
    source_type is the source_type (=corpus) field used for all results.
    output_dir is the directory where the resulting images are copied and
               the dm.json file is written. If the directory exists it will
               be removed and recreated.
    propertiesStr is a string with space separated key/value pairs that
               is used to set the top level fields in the output JSON file.
    keyStr is a string with space separated key/value pairs that
               is used to set the 'key' field in the output JSON file.
    ignore_hashes_file is a file that contains a list of image hashes
               that should be ignored.
    """
    self._source_type = source_type
    self._properties = _ParseKeyValuePairs(propertiesStr)
    self._properties["key"] = _ParseKeyValuePairs(keyStr)
    self._results =  []
    self._outputDir = outputDir

    # make sure the output directory exists and is empty.
    if os.path.exists(outputDir):
      shutil.rmtree(outputDir, ignore_errors=True)
    os.makedirs(outputDir)

    self._ignore_hashes = set()
    if ignore_hashes_file:
      with open(ignore_hashes_file, 'r') as ig_file:
        hashes=[x.strip() for x in ig_file.readlines() if x.strip()]
        self._ignore_hashes = set(hashes)

  def AddTestResult(self, testName, md5Hash, outputImagePath):
    # If the hash is in the list of hashes to ignore then we don'try
    # make a copy, but add it to the result.
    imgExt = os.path.splitext(outputImagePath)[1].lstrip(".")
    if md5Hash not in self._ignore_hashes:
      # Copy the image to <output_dir>/<md5Hash>.<image_extension>
      if not imgExt:
        raise ValueError("File %s does not have an extension" % outputImagePath)
      newFilePath = os.path.join(self._outputDir, md5Hash + '.' + imgExt)
      shutil.copy2(outputImagePath, newFilePath)

    # Add an entry to the list of test results
    self._results.append({
      "key": {
        "name": testName,
        "source_type": self._source_type,
      },
      "md5": md5Hash,
      "options": {
        "ext": imgExt,
        "gamma_correct": "no"
      }
    })

  def WriteResults(self):
    self._properties.update({
      "results": self._results
    })

    outputFileName = os.path.join(self._outputDir, "dm.json")
    with open(outputFileName, 'wb') as outfile:
      json.dump(self._properties, outfile, indent=1)
      outfile.write("\n")

# Produce example output for manual testing.
if __name__ == "__main__":
  # Create a test directory with three empty 'image' files.
  testDir = "./testdirectory"
  if not os.path.exists(testDir):
    os.makedirs(testDir)
  open(os.path.join(testDir, "image1.png"), 'wb').close()
  open(os.path.join(testDir, "image2.png"), 'wb').close()
  open(os.path.join(testDir, "image3.png"), 'wb').close()

  # Create an instance and add results.
  propStr = """build_number 2 "builder name" Builder-Name gitHash a4a338179013b029d6dd55e737b5bd648a9fb68c"""

  keyStr = "arch arm64 compiler Clang configuration Debug"

  hash_file = os.path.join(testDir, "ignore_hashes.txt")
  with open(hash_file, 'wb') as f:
    f.write("\n".join(["hash-1","hash-4"]) + "\n")

  gr = GoldResults("pdfium", testDir, propStr, keyStr, hash_file)
  gr.AddTestResult("test-1", "hash-1", os.path.join(testDir, "image1.png"))
  gr.AddTestResult("test-2", "hash-2", os.path.join(testDir, "image2.png"))
  gr.AddTestResult("test-3", "hash-3", os.path.join(testDir, "image3.png"))
  gr.WriteResults()
