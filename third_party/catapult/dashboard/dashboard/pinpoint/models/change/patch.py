# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import collections
import urlparse

from dashboard.services import gerrit_service


def FromDict(data):
  # If we have more Patch types in the future, we can loop
  # through them all and try each one's FromDict() in turn.
  return GerritPatch.FromDict(data)


class GerritPatch(collections.namedtuple(
    'GerritPatch', ('server', 'change', 'revision'))):
  """A patch in Gerrit.

  change is a change ID of the format '<project>~<branch>~<Change-Id>' and
  revision is a commit ID. Both are described in the Gerrit API documentation.
  https://gerrit-review.googlesource.com/Documentation/rest-api-changes.html#ids

  They must be in a canonical format so we can look up builds precisely.
  """

  def __str__(self):
    return self.revision[:7]

  @property
  def id_string(self):
    return '%s/%s/%s' % (self.server, self.change, self.revision)

  def BuildParameters(self):
    patch_info = gerrit_service.GetChange(
        self.server, self.change, fields=('ALL_REVISIONS',))
    revision_info = patch_info['revisions'][self.revision]
    return {
        'patch_gerrit_url': self.server,
        'patch_issue': patch_info['_number'],
        'patch_project': patch_info['project'],
        'patch_ref': revision_info['fetch']['http']['ref'],
        'patch_repository_url': revision_info['fetch']['http']['url'],
        'patch_set': revision_info['_number'],
        'patch_storage': 'gerrit',
    }

  def AsDict(self):
    patch_info = gerrit_service.GetChange(
        self.server, self.change, fields=('ALL_REVISIONS', 'DETAILED_ACCOUNTS'))
    # TODO: Cache this stuff in memcache.
    revision_info = patch_info['revisions'][self.revision]
    return {
        'server': self.server,
        'change': self.change,
        'revision': self.revision,

        'url': '%s/c/%s/+/%d/%d' % (
            self.server, patch_info['project'],
            patch_info['_number'], revision_info['_number']),
        'subject': patch_info['subject'],
        'time': revision_info['created'],
        'author': revision_info['uploader']['email'],
    }

  @classmethod
  def FromDict(cls, data):
    """Creates a new GerritPatch from the given data.

    Args:
      data: A patch URL string, for example:
        https://chromium-review.googlesource.com/c/chromium/tools/build/+/679595
        Or a dict containing {server, change, revision [optional]}.
        change is a {change-id} as described in the Gerrit API documentation.
        revision is a commit ID hash or numeric patch number.
        If revision is omitted, it is the change's current revision.

    Returns:
      A GerritPatch.

    Raises:
      KeyError: The patch doesn't have the given revision.
      ValueError: The URL has an unrecognized format.
    """
    if isinstance(data, basestring):
      url_parts = urlparse.urlparse(data)
      server = urlparse.urlunsplit(
          (url_parts.scheme, url_parts.netloc, '', '', ''))

      path_parts = iter(url_parts.path.split('/'))
      for path_part in path_parts:
        if path_part == '+':
          break
      else:
        raise ValueError('Unknown patch URL format: ' + data)

      change = path_parts.next()
      try:
        revision = int(path_parts.next())
      except StopIteration:
        revision = None
    else:
      server = data['server']
      change = data['change']
      revision = data.get('revision')

    # Look up the patch and convert everything to a canonical format.
    patch_info = gerrit_service.GetChange(
        server, change, fields=('ALL_REVISIONS',))
    change = patch_info['id']

    # Revision can be a revision ID or numeric patch number.
    if not revision:
      revision = patch_info['current_revision']
    for revision_id, revision_info in patch_info['revisions'].iteritems():
      if revision == revision_id or revision == revision_info['_number']:
        revision = revision_id
        break
    else:
      raise KeyError('Patch revision not found: %s/%s revision %s' %
                     (server, change, revision))

    return cls(server, change, revision)
