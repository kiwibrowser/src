# -*- coding: utf-8 -*-
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module containing class for recording metadata about a run."""

from __future__ import print_function

import datetime
import json
import math
import os

from chromite.lib import results_lib
from chromite.lib import constants
from chromite.lib import clactions
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import gs


# Number of parallel processes used when uploading/downloading GS files.
MAX_PARALLEL = 40

ARCHIVE_ROOT = 'gs://chromeos-image-archive/%(target)s'
# NOTE: gsutil 3.42 has a bug where '/' is ignored in this context unless it
#       is listed twice. So we list it twice here for now.
METADATA_URL_GLOB = os.path.join(ARCHIVE_ROOT,
                                 'R%(milestone)s**//metadata.json')
LATEST_URL = os.path.join(ARCHIVE_ROOT, 'LATEST-master')


GerritPatchTuple = clactions.GerritPatchTuple
GerritChangeTuple = clactions.GerritChangeTuple


class _DummyLock(object):
  """A Dummy clone of RLock that does nothing."""
  def acquire(self, blocking=1):
    pass

  def release(self):
    pass

  def __exit__(self, exc_type, exc_value, traceback):
    pass

  def __enter__(self):
    pass

class CBuildbotMetadata(object):
  """Class for recording metadata about a run."""

  def __init__(self, metadata_dict=None, multiprocess_manager=None):
    """Constructor for CBuildbotMetadata.

    Args:
      metadata_dict: Optional dictionary containing initial metadata,
                     as returned by loading metadata from json.
      multiprocess_manager: Optional multiprocess.Manager instance. If
                            supplied, the metadata instance will use
                            multiprocess containers so that its state
                            is correctly synced across processes.
    """
    super(CBuildbotMetadata, self).__init__()
    if multiprocess_manager:
      self._metadata_dict = multiprocess_manager.dict()
      self._cl_action_list = multiprocess_manager.list()
      self._per_board_dict = multiprocess_manager.dict()
      self._subdict_update_lock = multiprocess_manager.RLock()
    else:
      self._metadata_dict = {}
      self._cl_action_list = []
      self._per_board_dict = {}
      # If we are not using a manager, then metadata is not expected to be
      # multiprocess safe. Use a dummy RLock.
      self._subdict_update_lock = _DummyLock()

    if metadata_dict:
      self.UpdateWithDict(metadata_dict)

  @staticmethod
  def FromJSONString(json_string):
    """Construct a CBuildbotMetadata from a json representation.

    Args:
      json_string: A string json representation of a CBuildbotMetadata
                   dictionary.

    Returns:
      A CbuildbotMetadata instance.
    """
    return CBuildbotMetadata(json.loads(json_string))

  def UpdateWithDict(self, metadata_dict):
    """Update metadata dictionary with values supplied in |metadata_dict|

    This method is effectively the inverse of GetDict. Existing key-values
    in metadata will be overwritten by those supplied in |metadata_dict|,
    with the exceptions of:
     - the cl_actions list which will be extended with the contents (if any)
     of the supplied dict's cl_actions list.
     - the per-board metadata dict, which will be recursively extended with the
       contents of the supplied dict's board-metadata

    Args:
      metadata_dict: A dictionary of key-value pairs to be added this
                     metadata instance. Keys should be strings, values
                     should be json-able.

    Returns:
      self
    """
    # This is effectively the inverse of the dictionary construction in GetDict,
    # to reconstruct the correct internal representation of a metadata
    # object.
    metadata_dict = metadata_dict.copy()
    cl_action_list = metadata_dict.pop('cl_actions', None)
    per_board_dict = metadata_dict.pop('board-metadata', None)
    self._metadata_dict.update(metadata_dict)
    if cl_action_list:
      self._cl_action_list.extend(cl_action_list)
    if per_board_dict:
      for k, v in per_board_dict.items():
        self.UpdateBoardDictWithDict(k, v)

    return self

  def UpdateBoardDictWithDict(self, board, board_dict):
    """Update the per-board dict for |board| with |board_dict|.

    Note: both |board| and and all the keys of |board_dict| musts be strings
          that do not contain the character ':'

    Returns:
      self
    """
    # Wrap the per-board key-value pairs as key-value pairs in _per_board_dict.
    # Note -- due to http://bugs.python.org/issue6766 it is not possible to
    # store a multiprocess dict proxy inside another multiprocess dict proxy.
    # That is why we are using this flattened representation of board dicts.
    assert not ':' in board
    # Even if board_dict is {}, ensure that an entry with this board
    # gets written.
    self._per_board_dict[board + ':'] = None
    for k, v in board_dict.items():
      assert not ':' in k
      self._per_board_dict['%s:%s' % (board, k)] = v

    return self

  def UpdateKeyDictWithDict(self, key, key_metadata_dict):
    """Update metadata for the given key with values supplied in |metadata_dict|

    This method merges the dictionary for the given key with the given key
    metadata dictionary (allowing them to be effectively updated from any
    stage).

    This method is multiprocess safe.

    Args:
      key: The key name (e.g. 'version' or 'status')
      key_metadata_dict: A dictionary of key-value pairs to be added this
                     metadata key. Keys should be strings, values
                     should be json-able.

    Returns:
      self
    """
    with self._subdict_update_lock:
      # If the key already exists, then use its dictionary
      target_dict = self._metadata_dict.setdefault(key, {})
      target_dict.update(key_metadata_dict)
      self._metadata_dict[key] = target_dict

    return self

  def ExtendKeyListWithList(self, key, value_list):
    """Update metadata for the given key with the value_list.

    This method extends the mapped list in metadata_dict with value_list.
    This method is multiprocess safe.

    Args:
      key: The key name of string type.
      value_list: A list of values to be added to this metadata key.
                  Keys should be strings, values should be a json-able list.

    Returns:
      self
    """
    with self._subdict_update_lock:
      # If the key already exists, then use its list value
      target_list = self._metadata_dict.setdefault(key, [])
      target_list.extend(value_list)
      self._metadata_dict[key] = target_list

    return self

  def GetDict(self):
    """Returns a dictionary representation of metadata."""
    # CL actions are be stored in self._cl_action_list instead of
    # in self._metadata_dict['cl_actions'], because _cl_action_list
    # is potentially a multiprocess.lis. So, _cl_action_list needs to
    # be copied into a normal list.
    temp = self._metadata_dict.copy()
    temp['cl_actions'] = list(self._cl_action_list)

    # Similarly, the per-board dicts are stored in a flattened form in
    # _per_board_dict. Un-flatten into nested dict.
    per_board_dict = {}
    for k, v in self._per_board_dict.items():
      board, key = k.split(':')
      board_dict = per_board_dict.setdefault(board, {})
      if key:
        board_dict[key] = v

    temp['board-metadata'] = per_board_dict
    return temp

  # TODO(akeshet): crbug.com/406522 special case cl_actions and board-metadata
  # so that GetValue can work with them as well.
  def GetValue(self, key):
    """Get an item from the metadata dictionary.

    This method is in most cases an inexpensive equivalent to:
    GetDict()[key]

    However, it cannot be used for items like 'cl_actions' or 'board-metadata'
    which are not stored directly in the metadata dictionary.
    """
    return self._metadata_dict[key]

  def GetValueWithDefault(self, key, default=None):
    """Return the value for key if key is in the dictionary, else default.

    If default is not given, it defaults to None.
    This method is in most cases an inexpensive equivalent to:
    GetDict().get(key, default)

    However, it cannot be used for items like 'cl_actions' or 'board-metadata'
    which are not stored directly in the metadata dictionary.
    """
    return self._metadata_dict.get(key, default)

  def GetJSON(self, key=None):
    """Return a JSON string representation of metadata.

    Args:
      key: Key to return as JSON representation.  If None, returns all
           metadata.  Default: None
    """
    if key:
      return json.dumps(self.GetValue(key))
    else:
      return json.dumps(self.GetDict())

  def RecordCLAction(self, change, action, timestamp=None, reason=''):
    """Record an action that was taken on a CL, to the metadata.

    Args:
      change: A GerritPatch object for the change acted on.
      action: The action taken, should be one of constants.CL_ACTIONS
      timestamp: An integer timestamp such as int(time.time()) at which
                 the action was taken. Default: Now.
      reason: Description of the reason the action was taken. Default: ''

    Returns:
      self
    """
    cl_action = clactions.CLAction.FromGerritPatchAndAction(change, action,
                                                            reason, timestamp)
    self._cl_action_list.append(cl_action.AsMetadataEntry())
    return self

  @staticmethod
  def GetReportMetadataDict(builder_run, get_statuses_from_slaves,
                            config=None, stage=None, final_status=None,
                            completion_instance=None, child_configs_list=None):
    """Return a metadata dictionary summarizing a build.

    This method replaces code that used to exist in the ArchivingStageMixin
    class from cbuildbot_stage. It contains all the Report-stage-time
    metadata construction logic. The logic here is intended to be gradually
    refactored out so that the metadata is constructed gradually by the
    stages that are responsible for pieces of data, as they run.

    Args:
      builder_run: BuilderRun instance for this run.
      get_statuses_from_slaves: If True, status information of slave
                                builders will be recorded.
      config: The build config for this run.  Defaults to self._run.config.
      stage: The stage name that this metadata file is being uploaded for.
      final_status: Whether the build passed or failed. If None, the build
                    will be treated as still running.
      completion_instance: The stage instance that was used to wait for slave
                           completion. Used to add slave build information to
                           master builder's metadata. If None, no such status
                           information will be included. It not None, this
                           should be a derivative of
                           MasterSlaveSyncCompletionStage.
      child_configs_list: The list of child config metadata.  If specified it
                          should be added to the metadata.

    Returns:
       A metadata dictionary suitable to be json-serialized.
    """
    config = config or builder_run.config
    start_time = results_lib.Results.start_time
    current_time = datetime.datetime.now()
    start_time_stamp = cros_build_lib.UserDateTimeFormat(timeval=start_time)
    current_time_stamp = cros_build_lib.UserDateTimeFormat(timeval=current_time)
    duration = '%s' % (current_time - start_time,)

    metadata = {
        'status': {
            'current-time': current_time_stamp,
            'status': final_status if final_status else 'running',
            'summary': stage or '',
        },
        'time': {
            'start': start_time_stamp,
            'finish': current_time_stamp if final_status else '',
            'duration': duration,
        }
    }

    metadata['results'] = []
    for entry in results_lib.Results.Get():
      timestr = datetime.timedelta(seconds=math.ceil(entry.time))
      if entry.result in results_lib.Results.NON_FAILURE_TYPES:
        status = constants.BUILDER_STATUS_PASSED
      else:
        status = constants.BUILDER_STATUS_FAILED
      metadata['results'].append({
          'name': entry.name,
          'status': status,
          # The result might be a custom exception.
          'summary': str(entry.result),
          'duration': '%s' % timestr,
          'board': entry.board,
          'description': entry.description,
          'log': builder_run.ConstructDashboardURL(stage=entry.name),
      })

    if child_configs_list:
      metadata['child-configs'] = child_configs_list

    # If we were a CQ master, then include a summary of the status of slave cq
    # builders in metadata
    if get_statuses_from_slaves:
      statuses = completion_instance.GetSlaveStatuses()
      if not statuses:
        logging.warning('completion_instance did not have any statuses '
                        'to report. Will not add slave status to metadata.')

      metadata['slave_targets'] = {}
      for builder, status in statuses.iteritems():
        metadata['slave_targets'][builder] = status.AsFlatDict()

    return metadata


# Formats we like for output.
NICE_DATE_FORMAT = '%Y/%m/%d'
NICE_TIME_FORMAT = '%H:%M:%S'
NICE_DATETIME_FORMAT = NICE_DATE_FORMAT + ' ' + NICE_TIME_FORMAT


# TODO(akeshet): Delete this class once last remaining hackish caller in
# ReportStage is updated.
class BuildData(object):
  """Mostly obsolete helper class for interacting with build metadata."""
  __slots__ = (
      'gathered_dict',  # Dict with gathered data (sheets version).
      'gathered_url',   # URL to metadata.json.gathered location in GS.
      'metadata_dict',  # Dict representing metadata data from JSON.
      'metadata_url',   # URL to metadata.json location in GS.
  )

  SHEETS_VER_KEY = 'sheets_version'

  def __init__(self, metadata_url, metadata_dict, sheets_version=None):
    self.metadata_url = metadata_url
    self.metadata_dict = metadata_dict

    # If a stats version is not specified default to -1 so that the initial
    # version (version 0) will be considered "newer".
    self.gathered_url = metadata_url + '.gathered'
    self.gathered_dict = {
        self.SHEETS_VER_KEY: -1 if sheets_version is None else sheets_version,
    }

  def __getitem__(self, key):
    """Relay dict-like access to self.metadata_dict."""
    return self.metadata_dict[key]

  def get(self, key, default=None):
    """Relay dict-like access to self.metadata_dict."""
    return self.metadata_dict.get(key, default)

  @property
  def stages(self):
    return self['results']

  @property
  def slaves(self):
    return self.get('slave_targets', {})


  @property
  def failure_message(self):
    message_list = []
    # First collect failures in the master stages.
    failed_stages = [s for s in self.stages if s['status'] == 'failed']
    for stage in failed_stages:
      if stage['summary']:
        message_list.append('master: %s' % stage['summary'])

    mapping = {}
    # Dedup the messages from the slaves.
    for slave in self.GetFailedSlaves():
      message = self.slaves[slave]['reason']
      mapping[message] = mapping.get(message, []) + [slave]

    for message, slaves in mapping.iteritems():
      if len(slaves) >= 6:
        # Do not print all the names when there are more than 6 (an
        # arbitrary number) builders.
        message_list.append('%d builders: %s' % (len(slaves), message))
      else:
        message_list.append('%s: %s' % (','.join(slaves), message))

    return ' | '.join(message_list)

  def GetFailedSlaves(self, with_urls=False):
    def _Failed(slave):
      return slave['status'] == 'fail'

    # Older metadata has no slave_targets entry.
    slaves = self.slaves
    if with_urls:
      return [(name, slave['dashboard_url'])
              for name, slave in slaves.iteritems() if _Failed(slave)]
    else:
      return [name for name, slave in slaves.iteritems() if _Failed(slave)]

    return []


class MetadataException(Exception):
  """Base exception class for exceptions in this module."""


class GetMilestoneError(MetadataException):
  """Base exception class for exceptions in this module."""


def GetLatestMilestone():
  """Get the latest milestone from CQ Master LATEST-master file."""
  # Use CQ Master target to get latest milestone.
  latest_url = LATEST_URL % {'target': constants.CQ_MASTER}
  gs_ctx = gs.GSContext()

  logging.info('Getting latest milestone from %s', latest_url)
  try:
    content = gs_ctx.Cat(latest_url).strip()

    # Expected syntax is like the following: "R35-1234.5.6-rc7".
    assert content.startswith('R')
    milestone = content.split('-')[0][1:]
    logging.info('Latest milestone determined to be: %s', milestone)
    return int(milestone)

  except gs.GSNoSuchKey:
    raise GetMilestoneError('LATEST file missing: %s' % latest_url)


def GetMetadataURLsSince(target, start_date, end_date):
  """Get metadata.json URLs for |target| from |start_date| until |end_date|.

  The modified time of the GS files is used to compare with start_date, so
  the completion date of the builder run is what is important here.

  Args:
    target: Builder target name.
    start_date: datetime.date object of starting date.
    end_date: datetime.date object of ending date.

  Returns:
    Metadata urls for runs found.
  """
  ret = []
  milestone = GetLatestMilestone()
  gs_ctx = gs.GSContext()
  while True:
    base_url = METADATA_URL_GLOB % {'target': target, 'milestone': milestone}
    logging.info('Getting %s builds for R%d from "%s"', target, milestone,
                 base_url)

    try:
      # Get GS URLs.  We want the datetimes to quickly know when we are done
      # collecting URLs.
      urls = gs_ctx.List(base_url, details=True)
    except gs.GSNoSuchKey:
      # We ran out of metadata to collect.  Stop searching back in time.
      logging.info('No %s builds found for $%d.  I will not continue search'
                   ' to older milestones.', target, milestone)
      break

    # Sort by timestamp.
    urls = sorted(urls, key=lambda x: x.creation_time, reverse=True)

    # Add relevant URLs to our list.
    ret.extend([x.url for x in urls
                if (x.creation_time.date() >= start_date and
                    x.creation_time.date() <= end_date)])

    # See if we have gone far enough back by checking datetime of oldest URL
    # in the current batch.
    if urls[-1].creation_time.date() < start_date:
      break
    else:
      milestone -= 1
      logging.info('Continuing on to R%d.', milestone)

  return ret
