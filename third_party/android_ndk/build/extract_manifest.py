#
# Copyright (C) 2017 The Android Open Source Project
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
#
"""Extracts values from the AndroidManifest.xml file."""
import argparse
import os.path
import xml.etree.ElementTree


def parse_args():
    """Parse and return command line arguments."""
    parser = argparse.ArgumentParser()

    parser.add_argument(
        'property', metavar='PROPERTY',
        choices=('minSdkVersion', 'debuggable'),
        help='Property to extract from the manifest file.')

    parser.add_argument(
        'manifest_file', metavar='MANIFEST_FILE', type=os.path.abspath,
        help='Path to the AndroidManifest.xml file.')

    return parser.parse_args()


def get_rpath_attribute(root, element_path, attribute, default=None):
    """Returns the value of an attribute at an rpath.

    If more than one element exists with the same name, only the first is
    checked.

    Args:
        root: The XML element to search from.
        path: The path to the element.
        attribute: The name of the attribute to fetch.

    Returns:
        The attribute's value as a string if found, else the value of
        `default`.
    """
    ns_url = 'http://schemas.android.com/apk/res/android'
    ns = {
        'android': ns_url,
    }

    elem = root.find(element_path, ns)
    if elem is None:
        return ''
    # ElementTree elements don't have the same helpful namespace parameter that
    # the find family does :(
    attrib_name = attribute.replace('android:', '{' + ns_url + '}')
    return elem.get(attrib_name, default)


def get_minsdkversion(root):
    """Finds and returns the value of android:minSdkVersion in the manifest.

    Returns:
        String form of android:minSdkVersion if found, else the empty string.
    """
    return get_rpath_attribute(root, './uses-sdk', 'android:minSdkVersion', '')


def get_debuggable(root):
    """Finds and returns the value of android:debuggable in the manifest.

    Returns:
        String form of android:debuggable if found, else the empty string.
    """
    debuggable = get_rpath_attribute(
        root, './application', 'android:debuggable', '')

    # Though any such manifest would be invalid, the awk script rewrote bogus
    # values to false. Missing attributes should also be false.
    if debuggable != 'true':
        debuggable = 'false'

    return debuggable


def main():
    args = parse_args()

    tree = xml.etree.ElementTree.parse(args.manifest_file)
    if args.property == 'minSdkVersion':
        print get_minsdkversion(tree.getroot())
    elif args.property == 'debuggable':
        print get_debuggable(tree.getroot())
    else:
        raise ValueError


if __name__ == '__main__':
    main()
