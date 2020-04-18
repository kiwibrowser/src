# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Django models for cidb tables."""

from __future__ import print_function

from django.db import models

from build_annotations import fields as ba_fields


class BaseModel(models.Model):
  """Abstract base class to store all app-wide Meta options."""

  class Meta(object):
    """Set meta options for all models in this module."""
    # This property not inherited.
    abstract = True

    # The schema for CIDB is maintained external to this app.
    managed = False
    # Allow us to split the models.py file into different modules.
    app_label = 'cq_stats_sheet'
    # Each model should explicitly set this option. The default django model to
    # table name mapping does not work for us.
    db_table = 'Please define me'

  def __iter__(self):
    for field_name in self._meta.get_all_field_names():
      value = None
      if hasattr(self, field_name):
        value = getattr(self, field_name)
      yield field_name, value

  def __unicode__(self):
    result = []
    for _, value in self:
      result.append(unicode(value))
    return u', '.join(result)

  def __str__(self):
    return str(unicode(self))


class BuildTable(BaseModel):
  """Model for cidb.buildTable."""

  class Meta(object):
    """Set extra table options."""
    db_table = 'buildTable'

  id = ba_fields.ReadOnlyIntegerField(primary_key=True)
  last_updated = ba_fields.ReadOnlyDateTimeField()
  master_build_id = ba_fields.ReadOnlyForeignKey('self',
                                                 db_column='master_build_id')
  buildbot_generation = ba_fields.ReadOnlyIntegerField()
  builder_name = ba_fields.ReadOnlyCharField()
  waterfall = ba_fields.ReadOnlyCharField()
  build_number = ba_fields.ReadOnlyIntegerField()
  build_config = ba_fields.ReadOnlyCharField()
  bot_hostname = ba_fields.ReadOnlyCharField()
  start_time = ba_fields.ReadOnlyDateTimeField()
  finish_time = ba_fields.ReadOnlyDateTimeField()
  status = ba_fields.ReadOnlyCharField()
  status_pickle = ba_fields.ReadOnlyBlobField()
  build_type = ba_fields.ReadOnlyCharField()
  chrome_version = ba_fields.ReadOnlyCharField()
  milestone_version = ba_fields.ReadOnlyCharField()
  platform_version = ba_fields.ReadOnlyCharField()
  full_version = ba_fields.ReadOnlyCharField()
  sdk_version = ba_fields.ReadOnlyCharField()
  toolchain_url = ba_fields.ReadOnlyURLField()
  final = ba_fields.ReadOnlyBooleanField()
  metadata_url = ba_fields.ReadOnlyURLField()
  summary = ba_fields.ReadOnlyCharField()
  deadline = ba_fields.ReadOnlyDateTimeField()


class BuildStageTable(BaseModel):
  """Model for cidb.buildStageTable."""

  class Meta(object):
    """Set extra table options."""
    db_table = 'buildStageTable'

  # Not used directly in field definition for readonly tables, but used
  # elsewhere as constants.
  FAIL = 'fail'
  PASS = 'pass'
  INFLIGHT = 'inflight'
  MISSING = 'missing'
  PLANNED = 'planned'
  SKIPPED = 'skipped'
  FORGIVEN = 'forgiven'
  STATUS_CHOICES = (
      (FAIL, 'Stage failed'),
      (PASS, 'Stage passed! Hurray!'),
      (INFLIGHT, 'Stage is inflight'),
      (MISSING, 'Status missing'),
      (PLANNED, 'Stage is planned'),
      (SKIPPED, 'Stage skipped'),
      (FORGIVEN, 'Stage failed but forgiven'))

  id = ba_fields.ReadOnlyIntegerField(primary_key=True)
  build_id = ba_fields.ReadOnlyForeignKey('BuildTable',
                                          db_column='build_id')
  name = ba_fields.ReadOnlyCharField()
  board = ba_fields.ReadOnlyCharField()
  status = ba_fields.ReadOnlyEnumField()
  last_updated = ba_fields.ReadOnlyDateTimeField()
  start_time = ba_fields.ReadOnlyDateTimeField()
  finish_time = ba_fields.ReadOnlyDateTimeField()
  final = ba_fields.ReadOnlyBooleanField()


class ClActionTable(BaseModel):
  """Model for cidb.clActionTable."""

  class Meta(object):
    """Set extra table options."""
    db_table = 'clActionTable'

  # Not used directly in field definition for readonly tables, but used
  # elsewhere as constants.
  PICKED_UP = 'picked_up'
  SUBMITTED = 'submitted'
  KICKED_OUT = 'kicked_out'
  SUBMIT_FAILED = 'submit_failed'
  VERIFIED = 'verified'
  FORGIVEN = 'forgiven'
  # This list of choices is not exhaustive yet. It's only enough for CQ stats.
  ACTION_CHOICES = (
      (PICKED_UP, 'CL picked up by CQ'),
      (SUBMITTED, 'CL submitted by CQ'),
      (KICKED_OUT, 'CL kicked out by CQ'),
      (SUBMIT_FAILED, 'CQ failed to submit CL'),
      (VERIFIED, 'CL verified by CQ'),
      (FORGIVEN, 'CQ run failed, but CL forgiven'))

  id = ba_fields.ReadOnlyIntegerField(primary_key=True)
  build_id = ba_fields.ReadOnlyForeignKey('BuildTable',
                                          db_column='build_id')
  change_number = ba_fields.ReadOnlyIntegerField()
  patch_number = ba_fields.ReadOnlyIntegerField()
  change_source = ba_fields.ReadOnlyEnumField()
  action = ba_fields.ReadOnlyEnumField()
  reason = ba_fields.ReadOnlyCharField()
  timestamp = ba_fields.ReadOnlyDateTimeField()


class FailureTable(BaseModel):
  """Model for cidb.failureTable."""

  class Meta(object):
    """Set extra table options."""
    db_table = 'failureTable'

  id = ba_fields.ReadOnlyIntegerField(primary_key=True)
  build_stage_id = ba_fields.ReadOnlyForeignKey('BuildStageTable',
                                                db_column='build_stage_id')
  outer_failure_id = ba_fields.ReadOnlyForeignKey('self',
                                                  db_column='outer_failure_id')
  exception_type = ba_fields.ReadOnlyCharField()
  exception_message = ba_fields.ReadOnlyCharField()
  exception_category = ba_fields.ReadOnlyEnumField()
  extra_info = ba_fields.ReadOnlyCharField()
  timestamp = ba_fields.ReadOnlyDateTimeField()


class AnnotationsTable(BaseModel):
  """Model for cidb.annotationsTable."""

  class Meta(object):
    """Set extra table options."""
    db_table = 'annotationsTable'

  BAD_CL = 'bad_cl'
  BUG_IN_TOT = 'bug_in_tot'
  MERGE_CONFLICT = 'merge_conflict'
  TREE_CONFLICT = 'tree_conflict'
  SCHEDULED_ABORT = 'scheduled_abort'
  CL_NOT_READY = 'cl_not_ready'
  BAD_CHROME = 'bad_chrome'
  TEST_FLAKE = 'test_flake'
  GERRIT_FAILURE = 'gerrit_failure'
  GS_FAILURE = 'gs_failure'
  LAB_FAILURE = 'lab_failure'
  BAD_BINARY_FAILURE = 'bad_binary_failure'
  BUILD_FLAKE = 'build_flake'
  INFRA_FAILURE = 'infra_failure'
  MYSTERY = 'mystery'
  FAILURE_CATEGORY_CHOICES = (
      (BAD_CL, 'Bad CL (Please specify CL)'),
      (BUG_IN_TOT, 'Bug in ToT (Please specify bug)'),
      (MERGE_CONFLICT, 'Merge conflict'),
      (TREE_CONFLICT, 'Tree conflict'),
      (SCHEDULED_ABORT, 'Scheduled Abort'),
      (CL_NOT_READY, 'CL was marked not ready (Please specify CL)'),
      (BAD_CHROME, 'Bad chrome (Please speficy bug)'),
      (TEST_FLAKE, 'Test flake'),
      (GERRIT_FAILURE, 'Gerrit failure'),
      (GS_FAILURE, 'Google Storage failure'),
      (LAB_FAILURE, 'Lab failure'),
      (BAD_BINARY_FAILURE, 'Bad binary packages'),
      (BUILD_FLAKE, 'Local build flake'),
      (INFRA_FAILURE, 'Other Infrastructure failure'),
      (MYSTERY, 'Unknown failure: MyStErY'))

  # Warning: Some field constraints are duplicated here from the database
  # schema in CIDB.
  id = models.AutoField(primary_key=True)
  build_id = models.ForeignKey('BuildTable', db_column='build_id')
  last_updated = models.DateTimeField(auto_now=True)
  last_annotator = models.CharField(max_length=80)
  failure_category = models.CharField(
      max_length=max(len(x) for x, y in FAILURE_CATEGORY_CHOICES),
      choices=FAILURE_CATEGORY_CHOICES,
      default='mystery')
  failure_message = models.CharField(max_length=1024, blank=True, null=True)
  blame_url = models.CharField(max_length=512, blank=True, null=True)
  notes = models.CharField(max_length=1024, blank=True, null=True)
  deleted = models.BooleanField(default=False, null=False)


class BuildMessageTable(BaseModel):
  """Model for cidb.buildMessageTable."""
  # Must be the same constant as CL-exonerator uses.

  class MESSAGE_TYPES(object):
    """The annotation message_types that we use."""
    ANNOTATIONS_FINALIZED = 'annotations_finalized'

  class Meta(object):
    """Set the table."""
    db_table = 'buildMessageTable'

  id = models.AutoField(primary_key=True)
  build_id = models.ForeignKey('BuildTable', db_column='build_id')
  message_type = models.CharField(max_length=240)
  message_subtype = models.CharField(max_length=240)
  message_value = models.CharField(max_length=480)
  timestamp = ba_fields.ReadOnlyDateTimeField()
  board = models.CharField(max_length=240)
