# -*- coding: utf-8 -*-
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Provide a class for collecting info on one builder run.

There are two public classes, BuilderRun and ChildBuilderRun, that serve
this function.  The first is for most situations, the second is for "child"
configs within a builder config that has entries in "child_configs".

Almost all functionality is within the common _BuilderRunBase class.  The
only thing the BuilderRun and ChildBuilderRun classes are responsible for
is overriding the self.config value in the _BuilderRunBase object whenever
it is accessed.

It is important to note that for one overall run, there will be one
BuilderRun object and zero or more ChildBuilderRun objects, but they
will all share the same _BuilderRunBase *object*.  This means, for example,
that run attributes (e.g. self.attrs.release_tag) are shared between them
all, as intended.
"""

from __future__ import print_function

import cPickle
import functools
import os
import re
try:
  import Queue
except ImportError:
  # Python-3 renamed to "queue".  We still use Queue to avoid collisions
  # with naming variables as "queue".  Maybe we'll transition at some point.
  # pylint: disable=F0401
  import queue as Queue
import types

from chromite.cbuildbot import archive_lib
from chromite.lib.const import waterfall
from chromite.lib import constants
from chromite.lib import metadata_lib
from chromite.lib import cidb
from chromite.lib import cros_build_lib
from chromite.lib import osutils
from chromite.lib import path_util
from chromite.lib import portage_util
from chromite.lib import tree_status


class RunAttributesError(Exception):
  """Base class for exceptions related to RunAttributes behavior."""

  def __str__(self):
    """Handle stringify because base class will just spit out self.args."""
    return self.msg


class VersionNotSetError(RuntimeError):
  """Error raised if trying to access version_info before it's set."""


class ParallelAttributeError(AttributeError):
  """Custom version of AttributeError."""

  def __init__(self, attr, board=None, target=None, *args):
    if board or target:
      self.msg = ('No such board-specific parallel run attribute %r for %s/%s' %
                  (attr, board, target))
    else:
      self.msg = 'No such parallel run attribute %r' % attr
    super(ParallelAttributeError, self).__init__(self.msg, *args)
    self.args = (attr, board, target) + tuple(args)

  def __str__(self):
    return self.msg


class AttrSepCountError(ValueError):
  """Custom version of ValueError for when BOARD_ATTR_SEP is misused."""
  def __init__(self, attr, *args):
    self.msg = ('Attribute name has an unexpected number of "%s" occurrences'
                ' in it: %s' % (RunAttributes.BOARD_ATTR_SEP, attr))
    super(AttrSepCountError, self).__init__(self.msg, *args)
    self.args = (attr, ) + tuple(args)

  def __str__(self):
    return self.msg


class AttrNotPickleableError(RunAttributesError):
  """For when attribute value to queue is not pickleable."""

  def __init__(self, attr, value, *args):
    self.msg = 'Run attribute "%s" value cannot be pickled: %r' % (attr, value)
    super(AttrNotPickleableError, self).__init__(self.msg, *args)
    self.args = (attr, value) + tuple(args)


class AttrTimeoutError(RunAttributesError):
  """For when timeout is reached while waiting for attribute value."""

  def __init__(self, attr, *args):
    self.msg = 'Timed out waiting for value for run attribute "%s".' % attr
    super(AttrTimeoutError, self).__init__(self.msg, *args)
    self.args = (attr, ) + tuple(args)


class NoAndroidBranchError(Exception):
  """For when Android branch cannot be determined."""


class NoAndroidABIError(Exception):
  """For when Android ABI cannot be determined."""


class NoAndroidVersionError(Exception):
  """For when Android version cannot be determined."""


class LockableQueue(object):
  """Multiprocessing queue with associated recursive lock.

  Objects of this class function just like a regular multiprocessing Queue,
  except that there is also an rlock attribute for getting a multiprocessing
  RLock associated with this queue.  Actual locking must still be handled by
  the calling code.  Example usage:

  with queue.rlock:
    ... process the queue in some way.
  """

  def __init__(self, manager):
    self._queue = manager.Queue()
    self.rlock = manager.RLock()

  def __getattr__(self, attr):
    """Relay everything to the underlying Queue object at self._queue."""
    return getattr(self._queue, attr)


class RunAttributes(object):
  """Hold all run attributes for a particular builder run.

  There are two supported flavors of run attributes: REGULAR attributes are
  available only to stages that are run sequentially as part of the main (top)
  process and PARALLEL attributes are available to all stages, no matter what
  process they are in.  REGULAR attributes are accessed directly as normal
  attributes on a RunAttributes object, while PARALLEL attributes are accessed
  through the {Set|Has|Get}Parallel methods.  PARALLEL attributes also have the
  restriction that their values must be pickle-able (in order to be sent
  through multiprocessing queue).

  The currently supported attributes of each kind are listed in REGULAR_ATTRS
  and PARALLEL_ATTRS below.  To add support for a new run attribute simply
  add it to one of those sets.

  A subset of PARALLEL_ATTRS is BOARD_ATTRS.  These attributes only have meaning
  in the context of a specific board and config target.  The attributes become
  available once a board/config is registered for a run, and then they can be
  accessed through the {Set|Has|Get}BoardParallel methods or through the
  {Get|Set|Has}Parallel methods of a BoardRunAttributes object.  The latter is
  encouraged.

  To add a new BOARD attribute simply add it to the BOARD_ATTRS set below, which
  will also add it to PARALLEL_ATTRS (all BOARD attributes are assumed to need
  PARALLEL support).
  """

  REGULAR_ATTRS = frozenset((
      'chrome_version',   # Set by SyncChromeStage, if it runs.
      'manifest_manager', # Set by ManifestVersionedSyncStage.
      'release_tag',      # Set by cbuildbot after sync stage.
      'version_info',     # Set by the builder after sync+patch stage.
      'metadata',         # Used by various build stages to record metadata.
  ))

  # TODO(mtennant): It might be useful to have additional info for each board
  # attribute:  1) a log-friendly pretty name, 2) a rough upper bound timeout
  # value for consumers of the attribute to use when waiting for it.
  BOARD_ATTRS = frozenset((
      'breakpad_symbols_generated',   # Set by DebugSymbolsStage.
      'debug_tarball_generated',      # Set by DebugSymbolsStage.
      'images_generated',             # Set by BuildImageStage.
      'test_artifacts_uploaded',      # Set by UploadHWTestArtifacts.
      'instruction_urls_per_channel', # Set by ArchiveStage
      'success',                      # Set by cbuildbot.py:Builder
      'packages_under_test',          # Set by BuildPackagesStage.
      'signed_images_ready',          # Set by SigningStage
      'paygen_test_payloads_ready',   # Set by PaygenStage
  ))

  # Attributes that need to be set by stages that can run in parallel
  # (i.e. in a subprocess) must be included here.  All BOARD_ATTRS are
  # assumed to fit into this category.
  PARALLEL_ATTRS = BOARD_ATTRS | frozenset((
      'unittest_value',   # For unittests.  An example of a PARALLEL attribute
                          # that is not also a BOARD attribute.
  ))

  # This separator is used to create a unique attribute name for any
  # board-specific attribute.  For example:
  # breakpad_symbols_generated||stumpy||stumpy-full-config
  BOARD_ATTR_SEP = '||'

  # Sanity check, make sure there is no overlap between the attr groups.
  assert not REGULAR_ATTRS & PARALLEL_ATTRS

  # REGULAR_ATTRS show up as attributes directly on the RunAttributes object.
  __slots__ = tuple(REGULAR_ATTRS) + (
      '_board_targets', # Set of registered board/target combinations.
      '_manager',       # The multiprocessing.Manager to use.
      '_queues',        # Dict of parallel attribute names to LockableQueues.
  )

  def __init__(self, multiprocess_manager):
    # The __slots__ logic above confuses pylint.
    # https://bitbucket.org/logilab/pylint/issue/380/
    # pylint: disable=assigning-non-slot

    # Create queues for all non-board-specific parallel attributes now.
    # Parallel board attributes must wait for the board to be registered.
    self._manager = multiprocess_manager
    self._queues = {}
    for attr in RunAttributes.PARALLEL_ATTRS:
      if attr not in RunAttributes.BOARD_ATTRS:
        # pylint: disable=E1101
        self._queues[attr] = LockableQueue(self._manager)

    # Set of known <board>||<target> combinations.
    self._board_targets = set()

  def RegisterBoardAttrs(self, board, target):
    """Register a new valid board/target combination.  Safe to repeat.

    Args:
      board: Board name to register.
      target: Build config name to register.

    Returns:
      A new BoardRunAttributes object for more convenient access to the newly
        registered attributes specific to this board/target combination.
    """
    board_target = RunAttributes.BOARD_ATTR_SEP.join((board, target))

    if not board_target in self._board_targets:
      # Register board/target as a known board/target.
      self._board_targets.add(board_target)

      # For each board attribute that should be queue-able, create its queue
      # now.  Queues are kept by the uniquified run attribute name.
      for attr in RunAttributes.BOARD_ATTRS:
        # Every attr in BOARD_ATTRS is in PARALLEL_ATTRS, by construction.
        # pylint: disable=E1101
        uniquified_attr = self._GetBoardAttrName(attr, board, target)
        self._queues[uniquified_attr] = LockableQueue(self._manager)

    return BoardRunAttributes(self, board, target)

  # TODO(mtennant): Complain if a child process attempts to set a non-parallel
  # run attribute?  It could be done something like this:
  #def __setattr__(self, attr, value):
  #  """Override __setattr__ to prevent misuse of run attributes."""
  #  if attr in self.REGULAR_ATTRS:
  #    assert not self._IsChildProcess()
  #  super(RunAttributes, self).__setattr__(attr, value)

  def _GetBoardAttrName(self, attr, board, target):
    """Translate plain |attr| to uniquified board attribute name.

    Args:
      attr: Plain run attribute name.
      board: Board name.
      target: Build config name.

    Returns:
      The uniquified board-specific attribute name.

    Raises:
      AssertionError if the board/target combination does not exist.
    """
    board_target = RunAttributes.BOARD_ATTR_SEP.join((board, target))
    assert board_target in self._board_targets, \
        'Unknown board/target combination: %s/%s' % (board, target)

    # Translate to the unique attribute name for attr/board/target.
    return RunAttributes.BOARD_ATTR_SEP.join((attr, board, target))

  def SetBoardParallel(self, attr, value, board, target):
    """Set board-specific parallel run attribute value.

    Args:
      attr: Plain board run attribute name.
      value: Value to set.
      board: Board name.
      target: Build config name.
    """
    unique_attr = self._GetBoardAttrName(attr, board, target)
    self.SetParallel(unique_attr, value)

  def HasBoardParallel(self, attr, board, target):
    """Return True if board-specific parallel run attribute is known and set.

    Args:
      attr: Plain board run attribute name.
      board: Board name.
      target: Build config name.
    """
    unique_attr = self._GetBoardAttrName(attr, board, target)
    return self.HasParallel(unique_attr)

  def SetBoardParallelDefault(self, attr, default_value, board, target):
    """Set board-specific parallel run attribute value, if not already set.

    Args:
      attr: Plain board run attribute name.
      default_value: Value to set.
      board: Board name.
      target: Build config name.
    """
    if not self.HasBoardParallel(attr, board, target):
      self.SetBoardParallel(attr, default_value, board, target)

  def GetBoardParallel(self, attr, board, target, timeout=0):
    """Get board-specific parallel run attribute value.

    Args:
      attr: Plain board run attribute name.
      board: Board name.
      target: Build config name.
      timeout: See GetParallel for description.

    Returns:
      The value found.
    """
    unique_attr = self._GetBoardAttrName(attr, board, target)
    return self.GetParallel(unique_attr, timeout=timeout)

  def _GetQueue(self, attr, strict=False):
    """Return the queue for the given attribute, if it exists.

    Args:
      attr: The run attribute name.
      strict: If True, then complain if queue for |attr| is not found.

    Returns:
      The LockableQueue for this attribute, if it has one, or None
        (assuming strict is False).

    Raises:
      ParallelAttributeError if no queue for this attribute is registered,
        meaning no parallel attribute by this name is known.
    """
    queue = self._queues.get(attr)

    if queue is None and strict:
      raise ParallelAttributeError(attr)

    return queue

  def SetParallel(self, attr, value):
    """Set the given parallel run attribute value.

    Called to set the value of any parallel run attribute.  The value is
    saved onto a multiprocessing queue for that attribute.

    Args:
      attr: Name of the attribute.
      value: Value to give the attribute.  This value must be pickleable.

    Raises:
      ParallelAttributeError if attribute is not a valid parallel attribute.
      AttrNotPickleableError if value cannot be pickled, meaning it cannot
        go through the queue system.
    """
    # Confirm that value can be pickled, because otherwise it will fail
    # in the queue.
    try:
      cPickle.dumps(value, cPickle.HIGHEST_PROTOCOL)
    except cPickle.PicklingError:
      raise AttrNotPickleableError(attr, value)

    queue = self._GetQueue(attr, strict=True)

    with queue.rlock:
      # First empty the queue.  Any value already on the queue is now stale.
      while True:
        try:
          queue.get(False)
        except Queue.Empty:
          break

      queue.put(value)

  def HasParallel(self, attr):
    """Return True if the given parallel run attribute is known and set.

    Args:
      attr: Name of the attribute.
    """
    try:
      queue = self._GetQueue(attr, strict=True)

      with queue.rlock:
        return not queue.empty()
    except ParallelAttributeError:
      return False

  def SetParallelDefault(self, attr, default_value):
    """Set the given parallel run attribute only if it is not already set.

    This leverages HasParallel and SetParallel in a convenient pattern.

    Args:
      attr: Name of the attribute.
      default_value: Value to give the attribute if it is not set.  This value
        must be pickleable.

    Raises:
      ParallelAttributeError if attribute is not a valid parallel attribute.
      AttrNotPickleableError if value cannot be pickled, meaning it cannot
        go through the queue system.
    """
    if not self.HasParallel(attr):
      self.SetParallel(attr, default_value)

  # TODO(mtennant): Add an option to log access, including the time to wait
  # or waited.  It could be enabled with an optional announce=False argument.
  # See GetParallel helper on BoardSpecificBuilderStage class for ideas.
  def GetParallel(self, attr, timeout=0):
    """Get value for the given parallel run attribute, optionally waiting.

    If the given parallel run attr already has a value in the queue it will
    return that value right away.  Otherwise, it will wait for a value to
    appear in the queue up to the timeout specified (timeout of None means
    wait forever) before returning the value found or raising AttrTimeoutError
    if a timeout was reached.

    Args:
      attr: The name of the run attribute.
      timeout: Timeout, in seconds.  A None value means wait forever,
        which is probably never a good idea.  A value of 0 does not wait at all.

    Raises:
      ParallelAttributeError if attribute is not set and timeout was 0.
      AttrTimeoutError if timeout is greater than 0 and timeout is reached
        before a value is available on the queue.
    """
    got_value = False
    queue = self._GetQueue(attr, strict=True)

    # First attempt to get a value off the queue, without the lock.  This
    # allows a blocking get to wait for a value to appear.
    try:
      value = queue.get(True, timeout)
      got_value = True
    except Queue.Empty:
      # This means there is nothing on the queue.  Let this fall through to
      # the locked code block to see if another process is in the process
      # of re-queuing a value.  Any process doing that will have a lock.
      pass

    # Now grab the queue lock and flush any other values that are on the queue.
    # This should only happen if another process put a value in after our first
    # queue.get above.  If so, accept the updated value.
    with queue.rlock:
      while True:
        try:
          value = queue.get(False)
          got_value = True
        except Queue.Empty:
          break

      if got_value:
        # First re-queue the value, then return it.
        queue.put(value)
        return value

      else:
        # Handle no value differently depending on whether timeout is 0.
        if timeout == 0:
          raise ParallelAttributeError(attr)
        else:
          raise AttrTimeoutError(attr)


class BoardRunAttributes(object):
  """Convenience class for accessing board-specific run attributes.

  Board-specific run attributes (actually board/target-specific) are saved in
  the RunAttributes object but under uniquified names.  A BoardRunAttributes
  object provides access to these attributes using their plain names by
  providing the board/target information where needed.

  For example, to access the breakpad_symbols_generated board run attribute on
  a regular RunAttributes object requires this:

    value = attrs.GetBoardParallel('breakpad_symbols_generated', board, target)

  But on a BoardRunAttributes object:

    boardattrs = BoardRunAttributes(attrs, board, target)
    ...
    value = boardattrs.GetParallel('breakpad_symbols_generated')

  The same goes for setting values.
  """

  __slots__ = ('_attrs', '_board', '_target')

  def __init__(self, attrs, board, target):
    """Initialize.

    Args:
      attrs: The main RunAttributes object.
      board: The board name this is specific to.
      target: The build config name this is specific to.
    """
    self._attrs = attrs
    self._board = board
    self._target = target

  def SetParallel(self, attr, value, *args, **kwargs):
    """Set the value of parallel board attribute |attr| to |value|.

    Relay to SetBoardParallel on self._attrs, supplying board and target.
    See documentation on RunAttributes.SetBoardParallel for more details.
    """
    self._attrs.SetBoardParallel(attr, value, self._board, self._target,
                                 *args, **kwargs)

  def HasParallel(self, attr, *args, **kwargs):
    """Return True if parallel board attribute |attr| exists.

    Relay to HasBoardParallel on self._attrs, supplying board and target.
    See documentation on RunAttributes.HasBoardParallel for more details.
    """
    return self._attrs.HasBoardParallel(attr, self._board, self._target,
                                        *args, **kwargs)

  def SetParallelDefault(self, attr, default_value, *args, **kwargs):
    """Set the value of parallel board attribute |attr| to |value|, if not set.

    Relay to SetBoardParallelDefault on self._attrs, supplying board and target.
    See documentation on RunAttributes.SetBoardParallelDefault for more details.
    """
    self._attrs.SetBoardParallelDefault(attr, default_value, self._board,
                                        self._target, *args, **kwargs)

  def GetParallel(self, attr, *args, **kwargs):
    """Get the value of parallel board attribute |attr|.

    Relay to GetBoardParallel on self._attrs, supplying board and target.
    See documentation on RunAttributes.GetBoardParallel for more details.
    """
    return self._attrs.GetBoardParallel(attr, self._board, self._target,
                                        *args, **kwargs)


# TODO(mtennant): Consider renaming this _BuilderRunState, then renaming
# _RealBuilderRun to _BuilderRunBase.
class _BuilderRunBase(object):
  """Class to represent one run of a builder.

  This class should never be instantiated directly, but instead be
  instantiated as part of a BuilderRun object.
  """

  # Class-level dict of RunAttributes objects to make it less
  # problematic to send BuilderRun objects between processes through
  # pickle.  The 'attrs' attribute on a BuilderRun object will look
  # up the RunAttributes for that particular BuilderRun here.
  _ATTRS = {}

  __slots__ = (
      'site_config',     # SiteConfig for this run.
      'config',          # BuildConfig for this run.
      'options',         # The cbuildbot options object for this run.

      # Run attributes set/accessed by stages during the run.  To add support
      # for a new run attribute add it to the RunAttributes class above.
      '_attrs_id',       # Object ID for looking up self.attrs.

      # Some pre-computed run configuration values.
      'buildnumber',     # The build number for this run.
      'buildroot',       # The build root path for this run.
      'debug',           # Boolean, represents "dry run" concept, really.
      'manifest_branch', # The manifest branch to build and test for this run.

      # Some attributes are available as properties.  In particular, attributes
      # that use self.config must be determined after __init__.
      # self.bot_id      # Effective name of builder for this run.
  )

  def __init__(self, site_config, options, multiprocess_manager):
    self.site_config = site_config
    self.options = options

    # Note that self.config is filled in dynamically by either of the classes
    # that are actually instantiated: BuilderRun and ChildBuilderRun.  In other
    # words, self.config can be counted on anywhere except in this __init__.
    # The implication is that any plain attributes that are calculated from
    # self.config contents must be provided as properties (or methods).
    # See the _RealBuilderRun class and its __getattr__ method for details.
    self.config = None

    # Create the RunAttributes object for this BuilderRun and save
    # the id number for it in order to look it up via attrs property.
    attrs = RunAttributes(multiprocess_manager)
    self._ATTRS[id(attrs)] = attrs
    self._attrs_id = id(attrs)

    # Fill in values for all pre-computed "run configs" now, which are frozen
    # by this time.

    # TODO(mtennant): Should this use os.path.abspath like builderstage does?
    self.buildroot = self.options.buildroot
    self.buildnumber = self.options.buildnumber
    self.manifest_branch = self.options.branch

    # For remote_trybot runs, options.debug is implied, but we want true dryrun
    # mode only if --debug was actually specified (i.e. options.debug_forced).
    # TODO(mtennant): Get rid of confusing debug and debug_forced, if at all
    # possible.  Also, eventually use "dry_run" and "verbose" options instead to
    # represent two distinct concepts.
    self.debug = self.options.debug
    if self.options.remote_trybot:
      self.debug = self.options.debug_forced

    # The __slots__ logic above confuses pylint.
    # https://bitbucket.org/logilab/pylint/issue/380/
    # pylint: disable=assigning-non-slot

    # Certain run attributes have sensible defaults which can be set here.
    # This allows all code to safely assume that the run attribute exists.
    attrs.chrome_version = None
    attrs.metadata = metadata_lib.CBuildbotMetadata(
        multiprocess_manager=multiprocess_manager)

  @property
  def bot_id(self):
    """Return the bot_id for this run."""
    return self.config.name

  @property
  def attrs(self):
    """Look up the RunAttributes object for this BuilderRun object."""
    return self._ATTRS[self._attrs_id]

  def IsToTBuild(self):
    """Returns True if Builder is running on ToT."""
    return self.manifest_branch == 'master'

  def GetArchive(self):
    """Create an Archive object for this BuilderRun object."""
    # The Archive class is very lightweight, and is read-only, so it
    # is ok to generate a new one on demand.  This also avoids worrying
    # about whether it can go through pickle.
    # Almost everything the Archive class does requires GetVersion(),
    # which means it cannot be used until the version has been settled on.
    # However, because it does have some use before then we provide
    # the GetVersion function itself to be called when needed later.
    return archive_lib.Archive(self.bot_id, self.GetVersion, self.options,
                               self.config)

  def GetBoardRunAttrs(self, board):
    """Create a BoardRunAttributes object for this run and given |board|."""
    return BoardRunAttributes(self.attrs, board, self.config.name)

  def GetWaterfall(self):
    """Gets the waterfall of the current build."""
    # Metadata dictionary may not have been written at this time (it
    # should be written in the BuildStartStage), fall back to get the
    # environment variable in that case. Assume we are on the trybot
    # waterfall if no waterfall can be found.
    return (self.attrs.metadata.GetDict().get('buildbot-master-name') or
            os.environ.get('BUILDBOT_MASTERNAME') or
            waterfall.WATERFALL_TRYBOT)

  def GetBuildbotUrl(self):
    """Gets the URL of the waterfall hosting the current build."""
    # Metadata dictionary may not have been written at this time (it
    # should be written in the BuildStartStage), fall back to the
    # environment variable in that case. Assume we are on the trybot
    # waterfall if no waterfall can be found.
    return (self.attrs.metadata.GetDict().get('buildbot-url') or
            os.environ.get('BUILDBOT_BUILDBOTURL') or
            constants.TRYBOT_DASHBOARD)

  def GetBuilderName(self):
    """Get the name of this builder on the current waterfall."""
    return os.environ.get('BUILDBOT_BUILDERNAME', self.config.name)

  def ConstructDashboardURL(self, stage=None):
    """Return the dashboard URL

    This is the direct link to buildbot logs as seen in build.chromium.org

    Args:
      stage: Link to a specific |stage|, otherwise the general buildbot log

    Returns:
      The fully formed URL
    """
    # TODO: Stage links are only used in metadata_lib, and may not be
    # necessary. The logs links are currently limited an uninteresting. We
    # should remove or update to logdog.
    if stage:
      return tree_status.ConstructBuildStageURL(
          self.GetBuildbotUrl(),
          self.GetBuilderName(),
          self.options.buildnumber, stage=stage)
    else:
      # If we have a buildbucket_id, use it to construct URLs.
      if self.options.buildbucket_id:
        return tree_status.ConstructLegolandBuildURL(
            self.options.buildbucket_id)

      # If not, assume buildbot URLs are needed.
      return tree_status.ConstructDashboardURL(
          self.GetWaterfall(),
          self.GetBuilderName(),
          self.options.buildnumber)

  def ShouldBuildAutotest(self):
    """Return True if this run should build autotest and artifacts."""
    return self.options.tests

  def ShouldUploadPrebuilts(self):
    """Return True if this run should upload prebuilts."""
    return self.options.prebuilts and self.config.prebuilts

  def GetCIDBHandle(self):
    """Get the build_id and cidb handle, if available.

    Returns:
      A (build_id, CIDBConnection) tuple if cidb is set up and a build_id is
      known in metadata. Otherwise, (None, None).
    """
    try:
      build_id = self.attrs.metadata.GetValue('build_id')
    except KeyError:
      return (None, None)

    if not cidb.CIDBConnectionFactory.IsCIDBSetup():
      return (None, None)

    cidb_handle = cidb.CIDBConnectionFactory.GetCIDBConnectionForBuilder()
    if cidb_handle:
      return (build_id, cidb_handle)
    else:
      return (None, None)

  def ShouldReexecAfterSync(self):
    """Return True if this run should re-exec itself after sync stage."""
    return (self.options.postsync_reexec and self.config.postsync_reexec and
            not self.options.resume)

  def ShouldPatchAfterSync(self):
    """Return True if this run should patch changes after sync stage."""
    return self.options.postsync_patch and self.config.postsync_patch

  def InProduction(self):
    """Return True if this is a production run."""
    return cidb.CIDBConnectionFactory.GetCIDBConnectionType() == 'prod'

  def InEmailReportingEnvironment(self):
    """Return True if this run should send reporting emails.."""
    in_email_waterfall = self.GetWaterfall() in waterfall.EMAIL_WATERFALLS
    return self.InProduction() or in_email_waterfall

  def GetVersionInfo(self):
    """Helper for picking apart various version bits.

    The Builder must set attrs.version_info before calling this.  Further, it
    should do so only after the sources have been fully synced & patched, else
    it could return a confusing value.

    Returns:
      A manifest_version.VersionInfo object.

    Raises:
      VersionNotSetError if the version has not yet been set.
    """
    if not hasattr(self.attrs, 'version_info'):
      raise VersionNotSetError('builder must call SetVersionInfo first')
    return self.attrs.version_info

  def GetVersion(self):
    """Calculate full R<chrome_version>-<chromeos_version> version string.

    See GetVersionInfo() notes about runtime usage.

    Returns:
      The version string for this run.
    """
    verinfo = self.GetVersionInfo()
    release_tag = self.attrs.release_tag

    # Use a default of zero, in case we are a local tryjob or other build
    # without a CIDB id.
    build_id = self.attrs.metadata.GetValueWithDefault('build_id', 0)

    if release_tag:
      calc_version = 'R%s-%s' % (verinfo.chrome_branch, release_tag)
    else:
      # Non-versioned builds need the build number to uniquify the image.
      calc_version = 'R%s-%s-b%s' % (verinfo.chrome_branch,
                                     verinfo.VersionString(),
                                     build_id)

    return calc_version

  def HasUseFlag(self, board, use_flag):
    """Return the state of a USE flag for a board as a boolean."""
    return use_flag in portage_util.GetBoardUseFlags(board)

  def DetermineAndroidBranch(self, board):
    """Returns the Android branch in use by the active container ebuild."""
    try:
      android_package = self.DetermineAndroidPackage(board)
    except cros_build_lib.RunCommandError:
      raise NoAndroidBranchError(
          'Android branch could not be determined for %s' % board)
    if not android_package:
      raise NoAndroidBranchError(
          'Android branch could not be determined for %s (no package?)' % board)
    ebuild_path = portage_util.FindEbuildForBoardPackage(android_package, board)
    host_ebuild_path = path_util.FromChrootPath(ebuild_path)
    # We assume all targets pull from the same branch and that we always
    # have an ARM_TARGET or an AOSP_X86_USERDEBUG_TARGET.
    targets = ['ARM_TARGET', 'AOSP_X86_USERDEBUG_TARGET']
    ebuild_content = osutils.SourceEnvironment(host_ebuild_path, targets)
    for target in targets:
      if target in ebuild_content:
        branch = re.search(r'(.*?)-linux-', ebuild_content[target])
        if branch is not None:
          return branch.group(1)
    raise NoAndroidBranchError(
        'Android branch could not be determined for %s (ebuild empty?)' % board)

  def DetermineAndroidABI(self, board):
    """Returns the Android ABI in use by the active container ebuild."""
    try:
      android_package = self.DetermineAndroidPackage(board)
    except cros_build_lib.RunCommandError:
      raise NoAndroidABIError(
          'Android ABI could not be determined for %s' % board)
    if not android_package:
      raise NoAndroidABIError(
          'Android ABI could not be determined for %s (no package?)' % board)

    use_flags = portage_util.GetInstalledPackageUseFlags(
        'sys-devel/arc-build', board)
    if 'abi_x86_64' in use_flags.get('sys-devel/arc-build', []):
      return 'x86_64'
    elif 'abi_x86_32' in use_flags.get('sys-devel/arc-build', []):
      return 'x86'
    else:
      # ARM only supports 32-bit so it does not have abi_x86_{32,64} set. But it
      # is also the last possible ABI, so returning by default.
      return 'arm'

  def DetermineAndroidPackage(self, board):
    """Returns the active Android container package in use by the board."""
    packages = portage_util.GetPackageDependencies(board, 'virtual/target-os')
    # We assume there is only one Android package in the depgraph.
    for package in packages:
      if package.startswith('chromeos-base/android-container'):
        return package
    return None

  def DetermineAndroidVersion(self, boards=None):
    """Determine the current Android version in buildroot now and return it.

    This uses the typical portage logic to determine which version of Android
    is active right now in the buildroot.

    Args:
      boards: List of boards to check version of.

    Returns:
      The Android build ID of the container for the boards.

    Raises:
      NoAndroidVersionError: if no unique Android version can be determined.
    """
    if not boards:
      return None
    # Verify that all boards have the same version.
    version = None
    for board in boards:
      package = self.DetermineAndroidPackage(board)
      if not package:
        raise NoAndroidVersionError(
            'Android version could not be determined for %s' % boards)
      cpv = portage_util.SplitCPV(package)
      if not cpv:
        raise NoAndroidVersionError(
            'Android version could not be determined for %s' % board)
      if not version:
        version = cpv.version_no_rev
      elif version != cpv.version_no_rev:
        raise NoAndroidVersionError(
            'Different Android versions (%s vs %s) for %s' %
            (version, cpv.version_no_rev, boards))
    return version

  def DetermineChromeVersion(self):
    """Determine the current Chrome version in buildroot now and return it.

    This uses the typical portage logic to determine which version of Chrome
    is active right now in the buildroot.

    Returns:
      The new value of attrs.chrome_version (e.g. "35.0.1863.0").
    """
    cpv = portage_util.BestVisible(constants.CHROME_CP,
                                   buildroot=self.buildroot)
    return cpv.version_no_rev.partition('_')[0]


class _RealBuilderRun(object):
  """Base BuilderRun class that manages self.config access.

  For any builder run, sometimes the build config is the top-level config and
  sometimes it is a "child" config.  In either case, the config to use should
  override self.config for all cases.  This class provides a mechanism for
  overriding self.config access generally.

  Also, methods that do more than access state for a BuilderRun should
  live here.  In particular, any method that uses 'self' as an object
  directly should be here rather than _BuilderRunBase.
  """

  __slots__ = _BuilderRunBase.__slots__ + (
      '_run_base',  # The _BuilderRunBase object where most functionality is.
      '_config',    # BuildConfig to use for dynamically overriding self.config.
  )

  def __init__(self, run_base, build_config):
    """_RealBuilderRun constructor.

    Args:
      run_base: _BuilderRunBase object.
      build_config: BuildConfig object.
    """
    self._run_base = run_base
    self._config = build_config

    # Make sure self.attrs has board-specific attributes for each board
    # in build_config.
    for board in build_config.boards:
      self.attrs.RegisterBoardAttrs(board, build_config.name)

  def __getattr__(self, attr):
    # Remember, __getattr__ only called if attribute was not found normally.
    # In normal usage, the __init__ guarantees that self._run_base and
    # self._config will be present.  However, the unpickle process bypasses
    # __init__, and this object must be pickle-able.  That is why we access
    # self._run_base and self._config through __getattribute__ here, otherwise
    # unpickling results in infinite recursion.
    # TODO(mtennant): Revisit this if pickling support is changed to go through
    # the __init__ method, such as by supplying __reduce__ method.
    run_base = self.__getattribute__('_run_base')
    config = self.__getattribute__('_config')

    # TODO(akeshet): This logic seems to have a subtle flaky bug that only
    # manifests itself when using unit tests with ParallelMock. As a workaround,
    # we have simply eliminiated ParallelMock from the affected tests. See
    # crbug.com/470907 for context.
    try:
      # run_base.config should always be None except when accessed through
      # this routine.  Override the value here, then undo later.
      run_base.config = config

      result = getattr(run_base, attr)
      if isinstance(result, types.MethodType):
        # Make sure run_base.config is also managed when the method is called.
        @functools.wraps(result)
        def FuncWrapper(*args, **kwargs):
          run_base.config = config
          try:
            return result(*args, **kwargs)
          finally:
            run_base.config = None

        # TODO(mtennant): Find a way to make the following actually work.  It
        # makes pickling more complicated, unfortunately.
        # Cache this function wrapper to re-use next time without going through
        # __getattr__ again.  This ensures that the same wrapper object is used
        # each time, which is nice for identity and equality checks.  Subtle
        # gotcha that we accept: if the function itself on run_base is replaced
        # then this will continue to provide the behavior of the previous one.
        #setattr(self, attr, FuncWrapper)

        return FuncWrapper
      else:
        return result

    finally:
      run_base.config = None

  def GetChildren(self):
    """Get ChildBuilderRun objects for child configs, if they exist.

    Returns:
      List of ChildBuilderRun objects if self.config has child_configs.  []
        otherwise.
    """
    # If there are child configs, construct a list of ChildBuilderRun objects
    # for those child configs and return that.
    return [ChildBuilderRun(self, ix)
            for ix in range(len(self.config.child_configs))]

  def GetUngroupedBuilderRuns(self):
    """Same as GetChildren, but defaults to [self] if no children exist.

    Returns:
      Result of self.GetChildren, if children exist, otherwise [self].
    """
    return self.GetChildren() or [self]

  def GetBuilderIds(self):
    """Return a list of builder names for this config and the child configs."""
    bot_ids = [self.config.name]
    for config in self.config.child_configs:
      if config.name:
        bot_ids.append(config.name)
    return bot_ids


class BuilderRun(_RealBuilderRun):
  """A standard BuilderRun for a top-level build config."""

  def __init__(self, options, site_config, build_config, multiprocess_manager):
    """Initialize.

    Args:
      options: Command line options from this cbuildbot run.
      site_config: Site config for this cbuildbot run.
      build_config: Build config for this cbuildbot run.
      multiprocess_manager: A multiprocessing.Manager.
    """
    run_base = _BuilderRunBase(site_config, options, multiprocess_manager)
    super(BuilderRun, self).__init__(run_base, build_config)


class ChildBuilderRun(_RealBuilderRun):
  """A BuilderRun for a "child" build config."""

  def __init__(self, builder_run, child_index):
    """Initialize.

    Args:
      builder_run: BuilderRun for the parent (main) cbuildbot run.  Extract
        the _BuilderRunBase from it to make sure the same base is used for
        both the main cbuildbot run and any child runs.
      child_index: The child index of this child run, used to index into
        the main run's config.child_configs.
    """
    # pylint: disable=W0212
    run_base = builder_run._run_base
    config = builder_run.config.child_configs[child_index]
    super(ChildBuilderRun, self).__init__(run_base, config)
