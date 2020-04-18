# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""The database model Sheriff, for sheriff rotations."""

import re
import urlparse

from google.appengine.ext import ndb

from dashboard.models import internal_only_model

# Acceptable suffixes for internal-only sheriff rotation email addresses.
_ALLOWED_INTERNAL_ONLY_EMAIL_DOMAINS = (
    '@google.com',
    '@chromium.org',
    '@grotations.appspotmail.com',
)


class ValidationError(Exception):
  """Represents a failed validation."""
  pass


def _UrlValidator(prop, val):  # pylint: disable=unused-argument
  """Validates an URL property.

  The first argument is required, since this function is used as a validator
  function for a property. For more about validator functions in ndb, see:
  https://developers.google.com/appengine/docs/python/ndb/properties#options

  Args:
    prop: The property object.
    val: The string value to be validated as a URL.

  Raises:
    ValidationError: The input string is invalid.
  """
  parsed = urlparse.urlparse(val)
  if not (parsed.scheme and parsed.netloc and parsed.path):
    raise ValidationError('Invalid URL %s' % val)


def _EmailValidator(prop, val):  # pylint: disable=unused-argument
  """Validates an email address property."""
  # Note, this doesn't check the domain of the email address;
  # that is done in the pre-put hook in Sheriff.
  if not re.match(r'[\w.+-]+@[\w.-]', val):
    raise ValidationError('Invalid email %s' % val)


class Sheriff(internal_only_model.InternalOnlyModel):
  """Configuration options for sheriffing alerts. One per sheriff."""

  # Many perf sheriff email addresses are stored at a url. If there is a url
  # specified here, it will be used as the perf sheriff address. But the email
  # below will also be cc-ed, so that alerts can be archived to groups.
  url = ndb.StringProperty(validator=_UrlValidator, indexed=False)
  email = ndb.StringProperty(validator=_EmailValidator, indexed=False)

  internal_only = ndb.BooleanProperty(indexed=True, default=False)
  summarize = ndb.BooleanProperty(indexed=True, default=False)

  # A list of patterns. Each pattern is a string which can match parts of the
  # test path either exactly, or use '*' as a wildcard. Test paths matching
  # these patterns will use this sheriff configuration for alerting.
  patterns = ndb.StringProperty(repeated=True, indexed=False)

  # A list of labels to add to all bugs for the sheriffing rotation.
  labels = ndb.StringProperty(repeated=True, indexed=False)

  def _pre_put_hook(self):
    """Checks that the Sheriff properties are OK before putting."""
    if (self.internal_only and self.email
        and not _IsAllowedInternalOnlyEmail(self.email)):
      raise ValidationError('Invalid internal sheriff email %s' % self.email)


def _IsAllowedInternalOnlyEmail(email):
  """Checks whether an email address is OK for an internal-only sheriff."""
  return email.endswith(_ALLOWED_INTERNAL_ONLY_EMAIL_DOMAINS)
