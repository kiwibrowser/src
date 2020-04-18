# Copyright 2016 The LUCI Authors. All rights reserved.
# Use of this source code is governed under the Apache License, Version 2.0
# that can be found in the LICENSE file.

import collections
import re
import string
import types
import urllib
import urlparse

_ALNUM_CHARS = string.ascii_letters + string.digits
_SEGMENT_RE_BASE = r'[a-zA-Z0-9][a-zA-Z0-9:_\-.]*'
_STREAM_NAME_RE = re.compile('^(' + _SEGMENT_RE_BASE + ')(/' +
                             _SEGMENT_RE_BASE + ')*$')
_MAX_STREAM_NAME_LENGTH = 4096

_MAX_TAG_KEY_LENGTH = 64
_MAX_TAG_VALUE_LENGTH = 4096


def validate_stream_name(v, maxlen=None):
  """Verifies that a given stream name is valid.

  Args:
    v (str): The stream name string.


  Raises:
    ValueError if the stream name is invalid.
  """
  maxlen = maxlen or _MAX_STREAM_NAME_LENGTH
  if len(v) > maxlen:
    raise ValueError('Maximum length exceeded (%d > %d)' % (len(v), maxlen))
  if _STREAM_NAME_RE.match(v) is None:
    raise ValueError('Invalid stream name')


def validate_tag(key, value):
  """Verifies that a given tag key/value is valid.

  Args:
    k (str): The tag key.
    v (str): The tag value.

  Raises:
    ValueError if the tag is not valid.
  """
  validate_stream_name(key, maxlen=_MAX_TAG_KEY_LENGTH)
  validate_stream_name(value, maxlen=_MAX_TAG_VALUE_LENGTH)


def normalize(v, prefix=None):
  """Given a string, "v", mutate it into a valid stream name.

  This operates by replacing invalid stream naem characters with underscores (_)
  when encountered.

  A special case is when "v" begins with an invalid character. In this case, we
  will replace it with the "prefix", if one is supplied.

  See _STREAM_NAME_RE for a description of a valid stream name.

  Raises:
    ValueError: If normalization could not be successfully performed.
  """
  if len(v) == 0:
    if not prefix:
      raise ValueError('Cannot normalize empty name with no prefix.')
    v = prefix
  else:
    out = []
    for i, ch in enumerate(v):
      if i == 0 and not _is_valid_stream_char(ch, first=True):
        # The first letter is special, and must be alphanumeric.
        # If we have a prefix, prepend that to the resulting string.
        if prefix is None:
          raise ValueError('Name has invalid beginning, and no prefix was '
                           'provided.')
        out.append(prefix)

      if not _is_valid_stream_char(ch):
        ch = '_'
      out.append(ch)
    v = ''.join(out)

  # Validate the resulting string.
  validate_stream_name(v)
  return v


def _is_valid_stream_char(ch, first=False):
  """Returns (bool): True if a character is alphanumeric.

  The first character must be alphanumeric, matching [a-zA-Z0-9].
  Additional characters must either be alphanumeric or one of: (: _ - .).

  Args:
    ch (str): the character to evaluate.
    first (bool): if true, apply special first-character constraints.
  """
  # Alphanumeric check.
  if ch in _ALNUM_CHARS:
    return True
  if first:
    # The first character must be alphanumeric.
    return False

  # Check additional middle-name characters:
  return ch in ':_-./'


class StreamPath(collections.namedtuple('_StreamPath', ('prefix', 'name'))):
  """StreamPath is a full stream path.

  This consists of both a stream prefix and a stream name.

  When constructed with parse or make, the stream path must be completely valid.
  However, invalid stream paths may be constructed by manually instantiation.
  This can be useful for wildcard query values (e.g., "prefix='foo/*/bar/**'").
  """

  @classmethod
  def make(cls, prefix, name):
    """Returns (StreamPath): The validated StreamPath instance.

    Args:
      prefix (str): the prefix component
      name (str): the name component

    Raises:
      ValueError: If path is not a full, valid stream path string.
    """
    inst = cls(prefix=prefix, name=name)
    inst.validate()
    return inst

  @classmethod
  def parse(cls, path):
    """Returns (StreamPath): The parsed StreamPath instance.

    Args:
      path (str): the full stream path to parse.

    Raises:
      ValueError: If path is not a full, valid stream path string.
    """
    parts = path.split('/+/', 1)
    if len(parts) != 2:
      raise ValueError('Not a full stream path: [%s]' % (path,))
    return cls.make(*parts)

  def validate(self):
    """Raises: ValueError if this is not a valid stream name."""
    try:
      validate_stream_name(self.prefix)
    except ValueError as e:
      raise ValueError('Invalid prefix component [%s]: %s' % (
          self.prefix, e.message,))

    try:
      validate_stream_name(self.name)
    except ValueError as e:
      raise ValueError('Invalid name component [%s]: %s' % (
          self.name, e.message,))

  def __str__(self):
    return '%s/+/%s' % (self.prefix, self.name)


def get_logdog_viewer_url(host, project, *stream_paths):
  """Returns (str): The LogDog viewer URL for the named stream(s).

  Args:
    host (str): The name of the Coordiantor host.
    project (str): The project name.
    stream_paths: A set of StreamPath instances for the stream paths to
        generate the URL for.
  """
  return urlparse.urlunparse((
    'https', # Scheme
    host, # netloc
    'v/', # path
    '', # params
    '&'.join(('s=%s' % (urllib.quote('%s/%s' % (project, path), safe=''))
              for path in stream_paths)), # query
    '', # fragment
  ))
