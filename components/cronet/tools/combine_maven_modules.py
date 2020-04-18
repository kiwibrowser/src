#!/usr/bin/python
# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""combine_maven_modules.py - Combine Cronet maven modules together."""

import fileinput
import optparse
import os
import shutil
import sys
import tempfile
import zipfile

REPOSITORY_ROOT = os.path.abspath(os.path.join(
    os.path.dirname(__file__), '..', '..', '..'))
ANDROID_MANIFEST = os.path.join(REPOSITORY_ROOT, 'build', 'android',
                                'AndroidManifest.xml')
VERSION_SCRIPT = os.path.join(REPOSITORY_ROOT, 'build', 'util', 'version.py')
KEEP_RESOURCE = os.path.join(REPOSITORY_ROOT, 'components', 'cronet', 'android',
                            'api', 'res', 'raw', 'keep_cronet_api.xml')

def zipdir(path, zip_file):
  ziph = zipfile.ZipFile(zip_file, 'w', zipfile.ZIP_DEFLATED)
  for root, _, files in os.walk(path):
    for file_to_zip in files:
      filename = os.path.join(root, file_to_zip)
      ziph.write(filename, filename[len(path):])
  ziph.close()


class ModuleBuilder(object):

  def __init__(self, work_dir, version, suffix):
    """ModuleBuilder builds Maven modules.

    Args:
      work_dir: Working directory to build Maven modules.
      version: Chromium version (e.g. 66.0.3359.126).
      suffix: Maven module version suffix (e.g. -alpha).
    """
    self._build_dir = os.path.join(work_dir, 'Release', 'cronet')
    self._version_file = os.path.join(work_dir, 'Release', 'VERSION')
    self._modules_dir = os.path.join(work_dir, 'org', 'chromium', 'net')

    # Convert from Chromium's four number version (e.g. 66.0.3359.126) to a
    # three number version more compatible with Maven version comparing.
    # Remove the second number from Chromium's which is always 0.
    version = version.split('.')
    del version[1]
    version = '.'.join(version)

    self._version_without_suffix = version
    self._version = '%s%s' % (version, suffix)
    self._suffix = suffix

  def make_module(self, module_name, aar_jar, include_javadocs=False,
                  include_keep_resource=False, aar_proguard_config=None,
                  aar_native_lib=None):
    """Make a Maven module.

    Args:
      module_name: Maven module name (e.g. cronet-api).
      aar_jar: Name of jar to include in aar.
      include_javadocs: Boolean indicating if javadocs should be put in aar.
      include_keep_resource: Boolean indicating if keep_cronet_api.xml
                             resource should be included in aar.
      aar_proguard_config: Proguard config file to include in aar.
      aar_native_lib: Native library name to include in aar.
    """
    aar_jar = os.path.join(self._build_dir, aar_jar)

    module_dir = os.path.join(self._modules_dir, module_name, self._version)
    os.makedirs(module_dir)
    module_prefix = '%s-%s' % (module_name, self._version)

    aar_dir = tempfile.mkdtemp()
    shutil.copyfile(aar_jar, os.path.join(aar_dir, 'classes.jar'))
    open(os.path.join(aar_dir, 'public.txt'), 'a').close()
    shutil.copy(ANDROID_MANIFEST, aar_dir)
    manifest = fileinput.FileInput(os.path.join(aar_dir, 'AndroidManifest.xml'),
                                   inplace=True)
    for line in manifest:
      print line.replace('org.dummy', 'org.chromium.net'),

    if aar_proguard_config:
      aar_proguard_config = os.path.join(self._build_dir, aar_proguard_config)
      shutil.copyfile(aar_proguard_config, os.path.join(aar_dir,
                                                        'proguard.txt'))
    if aar_native_lib:
      for arch in ['arm64-v8a', 'armeabi-v7a', 'x86', 'x86_64']:
        lib_dir = os.path.join(aar_dir, 'jni', arch)
        os.makedirs(lib_dir)
        shutil.copyfile(os.path.join(self._build_dir, 'libs', arch,
                                     aar_native_lib),
                        os.path.join(lib_dir, aar_native_lib))
    with open(os.path.join(aar_dir, 'R.txt'), 'a') as r_file:
      if include_keep_resource:
        r_file.write('int raw keep_cronet_api 0x7f020000\n')
        res_dir = os.path.join(aar_dir, 'res', 'raw')
        os.makedirs(res_dir)
        shutil.copy(KEEP_RESOURCE, res_dir)

    zipdir(aar_dir, os.path.join(module_dir, '%s.aar' % module_prefix))
    shutil.rmtree(aar_dir)

    shutil.copyfile(aar_jar.replace('.jar', '-src.jar'),
                    os.path.join(module_dir, '%s-sources.jar' % module_prefix))

    pom_template = os.path.join(REPOSITORY_ROOT, 'components', 'cronet',
                                'android', 'maven',
                                '%s.pom.template' % module_name)
    pom_file = os.path.join(module_dir, '%s.pom' % module_prefix)
    if os.system('%s -f %s -i %s -o %s' % (VERSION_SCRIPT, self._version_file,
                                           pom_template, pom_file)):
      sys.stderr.write('version.py failed.')
      exit(1)
    if self._suffix != '':
      pom_file = fileinput.FileInput(pom_file, inplace=True)
      for line in pom_file:
        print line.replace('%s</version>' % self._version_without_suffix,
                           '%s</version>' % self._version),

    if include_javadocs:
      javadoc_dir = os.path.join(self._build_dir, 'javadoc')

      # Create an index.html file at the root as this is the accepted format.
      # Do this by copying reference/index.html and adjusting the path.
      with open(os.path.join(javadoc_dir, 'reference', 'index.html'), 'r') as \
          old_index, open(os.path.join(javadoc_dir, 'index.html'), 'w') as \
          new_index:
        for line in old_index:
          new_index.write(line.replace('classes.html',
                                       'reference/classes.html'))

      zipdir(javadoc_dir, os.path.join(module_dir,
                                       '%s-javadoc.jar' % module_prefix))

def main():
  parser = optparse.OptionParser()
  parser.add_option('--version',
                    help='Version of Cronet to download (e.g. 66.0.3359.126).')
  parser.add_option('--suffix',
                    help='The suffix to add. Must be alpha or beta.')
  options, _ = parser.parse_args()

  if not options.version:
    parser.error('Version not provided.')

  suffix = options.suffix
  if suffix:
    if suffix != 'alpha' and suffix != 'beta':
      parser.error('Suffix must be alpha or beta')
    suffix = '-%s' % suffix
  else:
    suffix = ''

  work_dir = tempfile.mkdtemp()

  if os.system(
            'cd %s && gsutil -m cp -R gs://chromium-cronet/android/%s/Release .'
               % (work_dir, options.version)):
    sys.stderr.write('Google cloud storage download failed.')
    exit(1)

  module_builder = ModuleBuilder(work_dir, options.version, suffix)

  module_builder.make_module(
    module_name="cronet-api",
    aar_jar="cronet_api.jar",
    include_javadocs=True,
    include_keep_resource=True,
  )

  module_builder.make_module(
    module_name="cronet-common",
    aar_jar="cronet_impl_common_java.jar",
    aar_proguard_config="cronet_impl_common_proguard.cfg",
  )

  module_builder.make_module(
    module_name="cronet-embedded",
    aar_jar="cronet_impl_native_java.jar",
    aar_proguard_config="cronet_impl_native_proguard.cfg",
    aar_native_lib="libcronet.%s.so" % options.version
  )

  module_builder.make_module(
    module_name="cronet-fallback",
    aar_jar="cronet_impl_platform_java.jar",
    aar_proguard_config="cronet_impl_platform_proguard.cfg"
  )

  shutil.rmtree(os.path.join(work_dir, 'Release'))

  print 'Maven modules in: %s' % work_dir

if __name__ == '__main__':
  main()
