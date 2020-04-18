# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Custom django model field definitions.

This module defines some convenience fields and readonly versions of required
django field types.
"""

from __future__ import print_function

from django.db import models


class BlobField(models.Field):
  """A binary blob field."""
  description = 'Blob'

  def db_type(self, connection):
    return 'blob'


class EnumField(models.CharField):
  """An enumeration field.

  This is a text field that additionally provides attributes to access the
  choices available for the enum values.
  """

  def __init__(self, *args, **kwargs):
    choices = kwargs.get('choices', [])
    max_length = max(len(x) for x in choices)
    kwargs['max_length'] = max_length
    for choice in choices:
      setattr(self, choice.upper(), choice)
    super(EnumField, self).__init__(*args, **kwargs)


# For all ReadOnly* fields, set null=True
# This allows us to use test data that has null values. Without this option,
# tests complain during loaddata if any of the fields (that we don't care about
# in the test itself) are null. Since this data is readonly, this data storage
# option is irrelevant in prod.

class ReadOnlyIntegerField(models.IntegerField):
  """Thou shalt not edit this field, otherwise, we're very accomodating."""
  def __init__(self, *args, **kwargs):
    kwargs['editable'] = False
    kwargs['blank'] = True
    if not kwargs.get('primary_key', False):
      kwargs['null'] = True
    super(ReadOnlyIntegerField, self).__init__(*args, **kwargs)


class ReadOnlyBooleanField(models.NullBooleanField):
  """Thou shalt not edit this field, otherwise, we're very accomodating."""
  def __init__(self, *args, **kwargs):
    kwargs['editable'] = False
    kwargs['blank'] = True
    super(ReadOnlyBooleanField, self).__init__(*args, **kwargs)


class ReadOnlyDateTimeField(models.DateTimeField):
  """Thou shalt not edit this field, otherwise, we're very accomodating."""
  def __init__(self, *args, **kwargs):
    kwargs['editable'] = False
    kwargs['blank'] = True
    if not kwargs.get('primary_key', False):
      kwargs['null'] = True
    super(ReadOnlyDateTimeField, self).__init__(*args, **kwargs)


class ReadOnlyForeignKey(models.ForeignKey):
  """Thou shalt not edit this field, otherwise, we're very accomodating."""
  def __init__(self, *args, **kwargs):
    kwargs['editable'] = False
    kwargs['blank'] = True
    if not kwargs.get('primary_key', False):
      kwargs['null'] = True
    super(ReadOnlyForeignKey, self).__init__(*args, **kwargs)


class ReadOnlyCharField(models.CharField):
  """Thou shalt not edit this field, otherwise, we're very accomodating."""
  def __init__(self, *args, **kwargs):
    kwargs['editable'] = False
    kwargs['blank'] = True
    if not kwargs.get('primary_key', False):
      kwargs['null'] = True
    kwargs['max_length'] = 1024
    super(ReadOnlyCharField, self).__init__(*args, **kwargs)


class ReadOnlyBlobField(BlobField):
  """Thou shalt not edit this field, otherwise, we're very accomodating."""
  def __init__(self, *args, **kwargs):
    kwargs['editable'] = False
    kwargs['blank'] = True
    if not kwargs.get('primary_key', False):
      kwargs['null'] = True
    super(ReadOnlyBlobField, self).__init__(*args, **kwargs)


class ReadOnlyEnumField(ReadOnlyCharField):
  """Thou shalt not edit this field, otherwise, we're very accomodating."""


class ReadOnlyURLField(models.URLField):
  """Thou shalt not edit this field, otherwise, we're very accomodating."""
  def __init__(self, *args, **kwargs):
    kwargs['editable'] = False
    kwargs['blank'] = True
    if not kwargs.get('primary_key', False):
      kwargs['null'] = True
    super(ReadOnlyURLField, self).__init__(*args, **kwargs)
