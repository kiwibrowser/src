# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import re

from recipe_engine import recipe_api

class GSUtilApi(recipe_api.RecipeApi):
  @property
  def gsutil_py_path(self):
    return self.package_repo_resource('gsutil.py')

  def __call__(self, cmd, name=None, use_retry_wrapper=True, version=None,
               parallel_upload=False, multithreaded=False, **kwargs):
    """A step to run arbitrary gsutil commands.

    Note that this assumes that gsutil authentication environment variables
    (AWS_CREDENTIAL_FILE and BOTO_CONFIG) are already set, though if you want to
    set them to something else you can always do so using the env={} kwarg.

    Note also that gsutil does its own wildcard processing, so wildcards are
    valid in file-like portions of the cmd. See 'gsutil help wildcards'.

    Arguments:
      cmd: list of (string) arguments to pass to gsutil.
           Include gsutil-level options first (see 'gsutil help options').
      name: the (string) name of the step to use.
            Defaults to the first non-flag token in the cmd.
    """
    if not name:
      name = (t for t in cmd if not t.startswith('-')).next()
    full_name = 'gsutil ' + name

    gsutil_path = self.gsutil_py_path
    cmd_prefix = []

    if use_retry_wrapper:
      # We pass the real gsutil_path to the wrapper so it doesn't have to do
      # brittle path logic.
      cmd_prefix = ['--', gsutil_path]
      gsutil_path = self.resource('gsutil_smart_retry.py')

    if version:
      cmd_prefix.extend(['--force-version', version])

    if parallel_upload:
      cmd_prefix.extend([
          '-o',
          'GSUtil:parallel_composite_upload_threshold=50M'
      ])

    if multithreaded:
      cmd_prefix.extend(['-m'])

    if use_retry_wrapper:
      # The -- argument for the wrapped gsutil.py is escaped as ---- as python
      # 2.7.3 removes all occurrences of --, not only the first. It is unescaped
      # in gsutil_wrapper.py and then passed as -- to gsutil.py.
      # Note, that 2.7.6 doesn't have this problem, but it doesn't hurt.
      cmd_prefix.append('----')
    else:
      cmd_prefix.append('--')

    return self.m.python(full_name, gsutil_path, cmd_prefix + cmd,
                         infra_step=True, **kwargs)

  def upload(self, source, bucket, dest, args=None, link_name='gsutil.upload',
             metadata=None, unauthenticated_url=False, **kwargs):
    args = [] if args is None else args[:]
    # Note that metadata arguments have to be passed before the command cp.
    metadata_args = self._generate_metadata_args(metadata)
    full_dest = 'gs://%s/%s' % (bucket, dest)
    cmd = metadata_args + ['cp'] + args + [source, full_dest]
    name = kwargs.pop('name', 'upload')

    result = self(cmd, name, **kwargs)

    if link_name:
      result.presentation.links[link_name] = self._http_url(
          bucket, dest, unauthenticated_url=unauthenticated_url)
    return result

  def download(self, bucket, source, dest, args=None, **kwargs):
    args = [] if args is None else args[:]
    full_source = 'gs://%s/%s' % (bucket, source)
    cmd = ['cp'] + args + [full_source, dest]
    name = kwargs.pop('name', 'download')
    return self(cmd, name, **kwargs)

  def download_url(self, url, dest, args=None, **kwargs):
    args = args or []
    url = self._normalize_url(url)
    cmd = ['cp'] + args + [url, dest]
    name = kwargs.pop('name', 'download_url')
    self(cmd, name, **kwargs)

  def cat(self, url, args=None, **kwargs):
    args = args or []
    url = self._normalize_url(url)
    cmd = ['cat'] + args + [url]
    name = kwargs.pop('name', 'cat')
    return self(cmd, name, **kwargs)

  def copy(self, source_bucket, source, dest_bucket, dest, args=None,
           link_name='gsutil.copy', metadata=None, unauthenticated_url=False,
           **kwargs):
    args = args or []
    args += self._generate_metadata_args(metadata)
    full_source = 'gs://%s/%s' % (source_bucket, source)
    full_dest = 'gs://%s/%s' % (dest_bucket, dest)
    cmd = ['cp'] + args + [full_source, full_dest]
    name = kwargs.pop('name', 'copy')

    result = self(cmd, name, **kwargs)

    if link_name:
      result.presentation.links[link_name] = self._http_url(
          dest_bucket, dest, unauthenticated_url=unauthenticated_url)

  def list(self, url, args=None, **kwargs):
    args = args or []
    url = self._normalize_url(url)
    cmd = ['ls'] + args + [url]
    name = kwargs.pop('name', 'list')
    return self(cmd, name, **kwargs)

  def signurl(self, private_key_file, bucket, dest, args=None, **kwargs):
    args = args or []
    full_source = 'gs://%s/%s' % (bucket, dest)
    cmd = ['signurl'] + args + [private_key_file, full_source]
    name = kwargs.pop('name', 'signurl')
    return self(cmd, name, **kwargs)

  def remove_url(self, url, args=None, **kwargs):
    args = args or []
    url = self._normalize_url(url)
    cmd = ['rm'] + args + [url]
    name = kwargs.pop('name', 'remove')
    self(cmd, name, **kwargs)

  def _generate_metadata_args(self, metadata):
    result = []
    if metadata:
      for k, v in sorted(metadata.iteritems(), key=lambda (k, _): k):
        field = self._get_metadata_field(k)
        param = (field) if v is None else ('%s:%s' % (field, v))
        result += ['-h', param]
    return result

  def _normalize_url(self, url):
    gs_prefix = 'gs://'
    # Defines the regex that matches a normalized URL.
    for prefix in (
        gs_prefix,
        'https://storage.cloud.google.com/',
        'https://storage.googleapis.com/',
        ):
      if url.startswith(prefix):
        return gs_prefix + url[len(prefix):]
    raise AssertionError("%s cannot be normalized" % url)

  @classmethod
  def _http_url(cls, bucket, dest, unauthenticated_url=False):
    if unauthenticated_url:
      base = 'https://storage.googleapis.com/%s/%s'
    else:
      base = 'https://storage.cloud.google.com/%s/%s'
    return base % (bucket, dest)

  @staticmethod
  def _get_metadata_field(name, provider_prefix=None):
    """Returns: (str) the metadata field to use with Google Storage

    The Google Storage specification for metadata can be found at:
    https://developers.google.com/storage/docs/gsutil/addlhelp/WorkingWithObjectMetadata
    """
    # Already contains custom provider prefix
    if name.lower().startswith('x-'):
      return name

    # See if it's innately supported by Google Storage
    if name in (
        'Cache-Control',
        'Content-Disposition',
        'Content-Encoding',
        'Content-Language',
        'Content-MD5',
        'Content-Type',
    ):
      return name

    # Add provider prefix
    if not provider_prefix:
      provider_prefix = 'x-goog-meta'
    return '%s-%s' % (provider_prefix, name)
