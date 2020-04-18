# -*- coding: utf-8 -*-
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""PayGen - Automatic Payload Generation.

This library processes a single build at a time, and decides which payloads
need to be generated. It then calls paygen_payload to generate each payload.

This library is reponsible for locking builds during processing, and checking
and setting flags to show that a build has been processed.
"""

from __future__ import print_function

import json
import operator
import os
import sys
import urlparse

from chromite.cbuildbot import commands
from chromite.lib import constants
from chromite.lib import config_lib
from chromite.lib import failures_lib
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import parallel
from chromite.lib import retry_util
from chromite.lib.paygen import download_cache
from chromite.lib.paygen import dryrun_lib
from chromite.lib.paygen import gslib
from chromite.lib.paygen import gslock
from chromite.lib.paygen import gspaths
from chromite.lib.paygen import paygen_payload_lib
from chromite.lib.paygen import urilib
from chromite.lib.paygen import utils

# For crostools access.
sys.path.insert(0, constants.SOURCE_ROOT)

AUTOTEST_DIR = os.path.join(constants.SOURCE_ROOT, 'src', 'third_party',
                            'autotest', 'files')
sys.path.insert(0, AUTOTEST_DIR)

# If we are an external only checkout, or a bootstrap environemnt these imports
# will fail. We quietly ignore the failure, but leave bombs around that will
# explode if people try to really use this library.
try:
  # pylint: disable=F0401
  from site_utils.autoupdate.lib import test_params
  from site_utils.autoupdate.lib import test_control
  # pylint: enable=F0401

except ImportError:
  test_params = None
  test_control = None


# The oldest release milestone for which run_suite should be attempted.
RUN_SUITE_MIN_MSTONE = 30

# Used to format timestamps on archived paygen.log file names in GS.
PAYGEN_LOG_TIMESTAMP_FORMAT = '%Y%m%d-%H%M%S-UTC'

# Board and device information published by goldeneye.
PAYGEN_URI = 'gs://chromeos-build-release-console/paygen.json'

# Sleep time used in _DiscoverRequiredPayloads. Export so tests can change.
BUILD_DISCOVER_RETRY_SLEEP = 90

class Error(Exception):
  """Exception base class for this module."""


class EarlyExit(Error):
  """Base class for paygen_build 'normal' errors.

  There are a number of cases in which a paygen run fails for reasons that
  require special reporting, but which are normal enough to avoid raising
  big alarms. We signal these results using exceptions derived from this
  class.

  Note that the docs strings on the subclasses may be displayed directly
  to the user, and RESULT may be returned as an exit code.
  """

  def __str__(self):
    """Return the doc string to the user as the exception description."""
    return self.__doc__


class BuildLocked(EarlyExit):
  """This build is locked and already being processed elsewhere."""
  RESULT = 23


class BuildNotReady(EarlyExit):
  """Not all images for this build are uploaded, don't process it yet."""
  RESULT = 25


class BoardNotConfigured(EarlyExit):
  """The board does not exist in the published goldeneye records."""
  RESULT = 26


class BuildCorrupt(Error):
  """Exception raised if a build has unexpected images."""


class ImageMissing(Error):
  """Exception raised if a build doesn't have expected images."""


class PayloadTestError(Error):
  """Raised when an error is encountered with generation of test artifacts."""


class ArchiveError(Error):
  """Raised when there was a failure to map a build to the images archive."""


def _LogList(title, obj_list):
  """Helper for logging a list of objects.

  Generates:
    1: ObjA
    2: ObjB
    3: ObjC
    ...

  Args:
    title: Title string for the list.
    obj_list: List of objects to convert to string and log, one per line.
  """
  logging.info('%s:', title)

  if not obj_list:
    logging.info(' (no objects listed)')
    return

  index = 0

  for obj in obj_list:
    index += 1
    logging.info(' %2d: %s', index, obj)


def _FilterForImages(artifacts):
  """Return only instances of Image from a list of artifacts."""
  return filter(gspaths.IsImage, artifacts)


def _FilterForMp(artifacts):
  """Return the MP keyed images in a list of artifacts.

  This returns all images with key names of the form "mp", "mp-v3", etc.

  Args:
    artifacts: The list of artifacts to filter.

  Returns:
    List of MP images.
  """
  return [i for i in _FilterForImages(artifacts) if 'mp' in i.key.split('-')]


def _FilterForPremp(artifacts):
  """Return the PreMp keyed images in a list of artifacts.

  The key for an images is expected to be of the form "premp", "mp", or
  "mp-vX". This filter returns everything that is "premp".

  Args:
    artifacts: The list of artifacts to filter.

  Returns:
    List of PreMP images.
  """
  return [i for i in _FilterForImages(artifacts) if 'premp' in i.key.split('-')]


def _FilterForBasic(artifacts):
  """Return the basic images in a list of artifacts.

  This only returns the basic images that match the target channel. This will
  filter out NPO and other duplicate channels that may exist in older builds.

  Args:
    artifacts: The list of artifacts to filter.

  Returns:
    List of basic images.
  """
  return [i for i in _FilterForImages(artifacts) if i.image_channel is None]


def _FilterForUnsignedImageArchives(artifacts):
  """Return only instances of UnsignedImageArchive from a list of artifacts."""
  return filter(gspaths.IsUnsignedImageArchive, artifacts)


def _FilterForImageType(artifacts, image_type):
  """Return only images for given |image_type|."""
  return [i for i in artifacts if i.image_type == image_type]


def _FilterForValidImageType(artifacts):
  """Return only images with image types that paygen supports."""
  v = gspaths.ChromeosReleases.UNSIGNED_IMAGE_TYPES
  return reduce(operator.add, [_FilterForImageType(artifacts, x) for x in v])


def _FilterForTest(artifacts):
  """Return only test images archives."""
  return [i for i in _FilterForUnsignedImageArchives(artifacts)
          if i.image_type == 'test']


def _GenerateSinglePayload(payload, work_dir, sign, dry_run):
  """Generate a single payload.

  This is intended to be safe to call inside a new process.

  Args:
    payload: gspath.Payload object defining the payloads to generate.
    work_dir: Working directory for payload generation.
    sign: boolean to decide if payload should be signed.
    dry_run: boolean saying if this is a dry run.
  """
  # This cache dir will be shared with other processes, but we need our
  # own instance of the cache manager to properly coordinate.
  cache_dir = paygen_payload_lib.FindCacheDir()
  with download_cache.DownloadCache(
      cache_dir, cache_size=PaygenBuild.CACHE_SIZE) as cache:
    # Actually generate the payload.
    paygen_payload_lib.CreateAndUploadPayload(
        payload,
        cache,
        work_dir=work_dir,
        sign=sign,
        dry_run=dry_run)


class PayloadTest(utils.RestrictedAttrDict):
  """A payload test definition.

  This specifies the payload to test, and (if it's a full payload) what source
  version to test an upgrade from. Delta payloads implicitly specify the source
  version.

  You must either use a delta payload, or specify both the src_channel and
  src_version.

  Attrs:
    payload: A gspaths.Payload object describing the payload to be tested.

    src_channel: The channel of the image to test updating from. Required
                 if the payload is a full payload, required to be None if
                 it's a delta.
    src_version: The version of the image to test updating from. Required
                 if the payload is a full payload, required to be None if
                 it's a delta.
  """
  _slots = ('payload', 'src_channel', 'src_version')
  _name = 'Payload Test'

  def __init__(self, payload, src_channel=None, src_version=None):
    assert bool(src_channel) == bool(src_version), (
        'src_channel(%s), src_version(%s) must both be set, or not set' %
        (src_channel, src_version))

    assert bool(src_channel and src_version) ^ bool(payload.src_image), (
        'src_channel(%s), src_version(%s) required for full, not allowed'
        ' for deltas. src_image: %s ' %
        (src_channel, src_version, payload.src_image))

    src_channel = src_channel or payload.src_image.channel
    src_version = src_version or payload.src_image.version

    super(PayloadTest, self).__init__(payload=payload,
                                      src_channel=src_channel,
                                      src_version=src_version)


class PaygenBuild(object):
  """This class is responsible for generating the payloads for a given build.

  It operates across a single build at a time, and is responsible for locking
  that build and for flagging it as finished when all payloads are generated.
  """
  # 50 GB of cache.
  CACHE_SIZE = 50 * 1024 * 1024 * 1024

  # Relative subpath for dumping control files inside the temp directory.
  CONTROL_FILE_SUBDIR = os.path.join('autotest', 'au_control_files')

  # The name of the suite of paygen-generated Autotest tests.
  PAYGEN_AU_SUITE_TEMPLATE = 'paygen_au_%s'

  # Name of the Autotest control file tarball.
  CONTROL_TARBALL_TEMPLATE = PAYGEN_AU_SUITE_TEMPLATE + '_control.tar.bz2'

  # Cache of full test payloads for a given version.
  _version_to_full_test_payloads = {}


  def __init__(self, build, work_dir, site_config,
               dry_run=False,
               skip_delta_payloads=False,
               skip_duts_check=False):
    """Initializer."""
    self._build = build
    self._work_dir = work_dir
    self._site_config = site_config
    self._drm = dryrun_lib.DryRunMgr(dry_run)
    self._skip_delta_payloads = skip_delta_payloads
    self._archive_board = None
    self._archive_build = None
    self._archive_build_uri = None
    self._skip_duts_check = skip_duts_check

  # Hidden class level cache value.
  _cachedPaygenJson = None

  @classmethod
  def GetPaygenJson(cls, board=None, channel=None):
    """Fetch the parsed Golden Eye payload generation configuration.

    Args:
      board: Board name in builder format (not release) or None for '*'
      channel: Channel name in 'stable' or 'stable-channel' format.
               Or None for '*'.

    Returns:
      List of GE delta values matching specification. Sample delta value:

      {
        'board': {
          'public_codename': 'x86-alex-he',
          'is_active': true,
          'builder_name': 'x86-alex_he'
        },
        'delta_type': 'MILESTONE',
        'channel': 'stable',
        'chrome_os_version': '8530.81.0',
        'chrome_version': '53.0.2785.103',
        'milestone': 53,
        'generate_delta': true,
        'delta_payload_tests': true,
        'full_payload_tests': false
      }
    """
    # We express channels in two different namespaces. Convert to the
    # namespace used by GE, if needed.
    if channel and channel.endswith('-channel'):
      channel = channel[:-len('-channel')]

    if not cls._cachedPaygenJson:
      cls._cachedPaygenJson = _GetJson(PAYGEN_URI)

    result = []

    for delta in cls._cachedPaygenJson['delta']:
      # 'channel' is an optional field, but None != channel.
      if ((not board or delta['board']['public_codename'] == board) and
          (not channel or delta.get('channel', None) == channel)):
        result.append(delta)

    return result

  def _GetFlagURI(self, flag):
    """Find the URI of the lock file associated with this build.

    Args:
      flag: Should be a member of gspaths.ChromeosReleases.FLAGS

    Returns:
      Returns a google storage path to the build flag requested.
    """
    return gspaths.ChromeosReleases.BuildPayloadsFlagUri(
        self._build.channel, self._build.board, self._build.version, flag,
        bucket=self._build.bucket)

  def _MapToArchive(self, board, version):
    """Returns the chromeos-image-archive equivalents for the build.

    Args:
      board: The board name (per chromeos-releases).
      version: The build version.

    Returns:
      A tuple consisting of the archive board name, build name and build URI.

    Raises:
      ArchiveError: if we could not compute the mapping.
    """
    # Map chromeos-releases board name to its chromeos-image-archive equivalent.
    archive_board_candidates = set([
        archive_board for archive_board in self._site_config.GetBoards()
        if archive_board.replace('_', '-') == board])
    if len(archive_board_candidates) == 0:
      raise ArchiveError('could not find build board name for %s' % board)
    elif len(archive_board_candidates) > 1:
      raise ArchiveError('found multiple build board names for %s: %s' %
                         (board, ', '.join(archive_board_candidates)))

    archive_board = archive_board_candidates.pop()

    # Find something in the respective chromeos-image-archive build directory.
    archive_build_search_uri = gspaths.ChromeosImageArchive.BuildUri(
        archive_board, '*', version)
    archive_build_file_uri_list = urilib.ListFiles(archive_build_search_uri)
    if not archive_build_file_uri_list:
      raise ArchiveError('cannot find archive build directory for %s' %
                         archive_build_search_uri)

    # Use the first search result.
    uri_parts = urlparse.urlsplit(archive_build_file_uri_list[0])
    archive_build_path = os.path.dirname(uri_parts.path)
    archive_build = archive_build_path.strip('/')
    archive_build_uri = urlparse.urlunsplit((uri_parts.scheme,
                                             uri_parts.netloc,
                                             archive_build_path,
                                             '', ''))

    return archive_board, archive_build, archive_build_uri

  def _ValidateExpectedBuildImages(self, build, images):
    """Validate that we got the expected images for a build.

    We expect that for any given build will have at most the following two
    signed images:

      premp basic build.
      mp basic build.

    Args:
      build: The build the images are from.
      images: The images discovered associated with the build.

    Raises:
      BuildCorrupt: Raised if unexpected images are found.
      ImageMissing: Raised if expected images are missing.
    """
    premp_basic = _FilterForBasic(_FilterForPremp(images))
    mp_basic = _FilterForBasic(_FilterForMp(images))

    # Make sure there is no more than one of each of our basic types.
    for i in (premp_basic, mp_basic):
      if len(i) > 1:
        msg = '%s has unexpected filtered images: %s.' % (build, i)
        raise BuildCorrupt(msg)

    # Make sure there were no unexpected types of images.
    if len(images) != len(premp_basic + mp_basic):
      msg = '%s has unexpected unfiltered images: %s' % (build, images)
      raise BuildCorrupt(msg)

    # Make sure there is at least one basic image.
    if not premp_basic and not mp_basic:
      msg = '%s has no basic images.' % build
      raise ImageMissing(msg)

  @retry_util.WithRetry(max_retry=3, exception=ImageMissing,
                        sleep=BUILD_DISCOVER_RETRY_SLEEP)
  def _DiscoverSignedImages(self, build):
    """Return a list of images associated with a given build.

    Args:
      build: The build to find images for.

    Returns:
      A list of images associated with the build. This may include premp, and mp
      images.

    Raises:
      BuildCorrupt: Raised if unexpected images are found.
      ImageMissing: Raised if expected images are missing.
    """
    # Ideally, |image_type| below should be constrained to the type(s) expected
    # for the board. But the board signing configs are not easily accessible at
    # this point, so we use the wildcard here and rely on the signers to upload
    # the expected artifacts.
    search_uri = gspaths.ChromeosReleases.ImageUri(
        build.channel, build.board, build.version, key='*', image_type='*',
        image_channel='*', image_version='*', bucket=build.bucket)

    image_uris = urilib.ListFiles(search_uri)
    images = [gspaths.ChromeosReleases.ParseImageUri(uri) for uri in image_uris]

    # Unparsable URIs will result in Nones; filter them out.
    images = [i for i in images if i]

    # We only care about recovery and test image types, ignore all others.
    images = _FilterForValidImageType(images)

    self._ValidateExpectedBuildImages(build, images)

    return images

  @retry_util.WithRetry(max_retry=3, exception=ImageMissing,
                        sleep=BUILD_DISCOVER_RETRY_SLEEP)
  def _DiscoverTestImage(self, build):
    """Return a list of unsigned image archives associated with a given build.

    Args:
      build: The build to find images for.

    Returns:
      A gspaths.UnsignedImageArchive instance.

    Raises:
      BuildCorrupt: Raised if unexpected images are found.
      ImageMissing: Raised if expected images are missing.
    """
    search_uri = gspaths.ChromeosReleases.UnsignedImageUri(
        build.channel, build.board, build.version, milestone='*',
        image_type='test', bucket=build.bucket)

    image_uris = urilib.ListFiles(search_uri)
    images = [gspaths.ChromeosReleases.ParseUnsignedImageUri(uri)
              for uri in image_uris]

    # Unparsable URIs will result in Nones; filter them out.
    images = [i for i in images if i]

    # Make sure we found the expected number of build images (1).
    if len(images) > 1:
      raise BuildCorrupt('%s has multiple test images: %s' % (build, images))

    if not images:
      raise ImageMissing('%s has no test image' % build)

    return images[0]

  def _DiscoverRequiredDeltasBuildToBuild(self, source_images, images):
    """Find the deltas to generate between two builds.

    We should generate deltas all combinations of:
      Test image -> Test image
      PreMP signed image -> PreMP signed image
      MP signed image -> MP signed image.

    Any given build should have exactly one test image, and zero or one of each
    signed type.

    Args:
      source_images: All images associated with the source build.
      images: All images associated with the target build.

    Returns:
      A list of gspaths.Payload objects.
    """
    results = []

    for f in (_FilterForMp, _FilterForPremp, _FilterForTest):
      filtered_source = f(source_images)
      filtered_target = f(images)

      if filtered_source and filtered_target:
        assert len(filtered_source) == 1, 'Unexpected: %s.' % filtered_source
        assert len(filtered_target) == 1, 'Unexpected: %s.' % filtered_target

        # A delta from each previous image to current image.
        results.append(gspaths.Payload(tgt_image=filtered_target[0],
                                       src_image=filtered_source[0]))

    return results

  def _DiscoverRequiredPayloads(self):
    """Find the payload definitions for the current build.

    This method finds the images for the current build, and for all builds we
    need deltas from, and decides exactly what payloads are needed.

    Returns:
      [<gspaths.Payload>...], [<PayloadTest>...]

      The list of payloads does NOT have URLs populated, and has not
      been tested for existence. delta payloads are NOT present if we are
      skipping them.

    Raises:
      BuildNotReady: If the current build doesn't seem to have all of it's
          images available yet. This commonly happens because the signer hasn't
          finished signing the current build.
      BuildCorrupt: If current or previous builds have unexpected images.
      ImageMissing: Raised if expected images are missing for previous builds.
    """
    payloads = []
    payload_tests = []

    try:
      # When discovering the images for our current build, they might not be
      # discoverable right away (GS eventual consistency). So, we retry.
      images = self._DiscoverSignedImages(self._build)
      test_image = self._DiscoverTestImage(self._build)

    except ImageMissing as e:
      # If the main build doesn't have the final build images, then it's
      # not ready.
      logging.info(e)
      raise BuildNotReady()

    _LogList('Images found', images+[test_image])

    # Add full payloads for PreMP and MP (as needed).
    for i in images:
      payloads.append(gspaths.Payload(tgt_image=i))

    # Add full test payload, and N2N test for it.
    full_test_payload = gspaths.Payload(tgt_image=test_image)
    payloads.append(full_test_payload)
    payload_tests.append(PayloadTest(
        full_test_payload, self._build.channel, self._build.version))

    # Add n2n test delta.
    if not self._skip_delta_payloads:
      n2n_payload = gspaths.Payload(tgt_image=test_image, src_image=test_image)
      payloads.append(n2n_payload)
      payload_tests.append(PayloadTest(n2n_payload))

    # Add in the payloads GE wants us to generate.
    for source in self.GetPaygenJson(self._build.board, self._build.channel):
      source_build = gspaths.Build(version=source['chrome_os_version'],
                                   board=self._build.board,
                                   channel=self._build.channel,
                                   bucket=self._build.bucket)

      # Extract the source values we care about.
      logging.info('Considering: %s %s', source['delta_type'], source_build)

      if not source['generate_delta'] and not source['full_payload_tests']:
        logging.warning('Skipping. No payloads or tests requested.')
        continue

      if not gspaths.VersionGreater(self._build.version, source_build.version):
        logging.warning('Skipping. Newer than current build.')
        continue

      source_images = self._DiscoverSignedImages(source_build)
      source_test_image = self._DiscoverTestImage(source_build)

      if not self._skip_delta_payloads and source['generate_delta']:
        # Generate the signed deltas.
        payloads.extend(self._DiscoverRequiredDeltasBuildToBuild(
            source_images, images+[test_image]))

        # Generate the test delta.
        test_payload = gspaths.Payload(
            tgt_image=test_image, src_image=source_test_image)
        payloads.append(test_payload)

        if source['delta_payload_tests']:
          payload_tests.append(PayloadTest(test_payload))

      if source['full_payload_tests']:
        # Test the full payload against this source version.
        payload_tests.append(PayloadTest(
            full_test_payload, source_build.channel, source_build.version))

    for p in payloads:
      paygen_payload_lib.FillInPayloadUri(p)

    for t in payload_tests:
      paygen_payload_lib.FillInPayloadUri(t.payload)

    return payloads, payload_tests

  def _GeneratePayloads(self, payloads):
    """Generate the payloads called for by a list of payload definitions.

    It will keep going, even if there is a failure.

    Args:
      payloads: gspath.Payload objects defining all of the payloads to generate.
      lock: gslock protecting this paygen_build run.

    Raises:
      Any arbitrary exception raised by CreateAndUploadPayload.
    """
    payloads_args = [(payload,
                      self._work_dir,
                      isinstance(payload.tgt_image, gspaths.Image),
                      bool(self._drm))
                     for payload in payloads]

    parallel.RunTasksInProcessPool(_GenerateSinglePayload, payloads_args)

  def _FindFullTestPayloads(self, channel, version):
    """Returns a list of full test payloads for a given version.

    Uses the current build's board and bucket values. This method caches the
    full test payloads previously discovered as we may be using them for
    multiple tests in a single run.

    Args:
      channel: Channel to look in for payload.
      version: A build version whose payloads to look for.

    Returns:
      A (possibly empty) list of payload URIs.
    """
    assert channel
    assert version

    if (channel, version) in self._version_to_full_test_payloads:
      # Serve from cache, if possible.
      return self._version_to_full_test_payloads[(channel, version)]

    payload_search_uri = gspaths.ChromeosReleases.PayloadUri(
        channel, self._build.board, version, '*',
        bucket=self._build.bucket)

    payload_candidate = urilib.ListFiles(payload_search_uri)

    # We create related files for each payload that have the payload name
    # plus these extensions. Skip these files.
    NOT_PAYLOAD = ('.json', '.log')
    full_test_payloads = [u for u in payload_candidate
                          if not any([u.endswith(n) for n in NOT_PAYLOAD])]
    # Store in cache.
    self._version_to_full_test_payloads[(channel, version)] = full_test_payloads
    return full_test_payloads

  def _EmitControlFile(self, payload_test, suite_name, control_dump_dir):
    """Emit an Autotest control file for a given payload test."""
    # Figure out the source version for the test.
    payload = payload_test.payload
    src_version = payload_test.src_version
    src_channel = payload_test.src_channel

    # Discover the full test payload that corresponds to the source version.
    src_payload_uri_list = self._FindFullTestPayloads(src_channel, src_version)
    if not src_payload_uri_list:
      logging.error('Cannot find full test payload for source version (%s), '
                    'control file not generated', src_version)
      raise PayloadTestError('cannot find source payload for testing %s' %
                             payload)

    if len(src_payload_uri_list) != 1:
      logging.error('Found multiple (%d) full test payloads for source version '
                    '(%s), control file not generated:\n%s',
                    len(src_payload_uri_list), src_version,
                    '\n'.join(src_payload_uri_list))
      raise PayloadTestError('multiple source payloads found for testing %s' %
                             payload)

    src_payload_uri = src_payload_uri_list[0]
    logging.info('Source full test payload found at %s', src_payload_uri)

    release_archive_uri = gspaths.ChromeosReleases.BuildUri(
        src_channel, self._build.board, src_version)

    # TODO(dgarrett): Remove if block after finishing crbug.com/523122
    stateful_uri = os.path.join(release_archive_uri, 'stateful.tgz')
    if not urilib.Exists(stateful_uri):
      logging.error('%s does not exist.', stateful_uri)
      logging.error('Full test payload for source version (%s) exists, but '
                    'stateful.tgz does not. Control file not generated',
                    src_version)
      raise PayloadTestError('cannot find source stateful.tgz for testing %s' %
                             payload)

    test = test_params.TestConfig(
        self._archive_board,
        suite_name,               # Name of the test (use the suite name).
        bool(payload.src_image),  # Whether this is a delta.
        src_version,
        payload.tgt_image.version,
        src_payload_uri,
        payload.uri,
        suite_name=suite_name,
        source_archive_uri=release_archive_uri)

    with open(test_control.get_control_file_name()) as f:
      control_code = f.read()
    control_file = test_control.dump_autotest_control_file(
        test, None, control_code, control_dump_dir)
    logging.info('Control file emitted at %s', control_file)
    return control_file

  def _AutotestPayloads(self, payload_tests):
    """Create necessary test artifacts and initiate Autotest runs.

    Args:
      payload_tests: An iterable of PayloadTest objects defining payload tests.
    """
    # Create inner hierarchy for dumping Autotest control files.
    control_dir = os.path.join(self._work_dir, 'autotests')
    control_dump_dir = os.path.join(control_dir, self.CONTROL_FILE_SUBDIR)
    os.makedirs(control_dump_dir)

    # Customize the test suite's name based on this build's channel.
    test_channel = self._build.channel.rpartition('-')[0]
    suite_name = (self.PAYGEN_AU_SUITE_TEMPLATE % test_channel)

    # Emit a control file for each payload.
    logging.info('Emitting control files into %s', control_dump_dir)
    for payload_test in payload_tests:
      self._EmitControlFile(payload_test, suite_name, control_dump_dir)

    tarball_name = self.CONTROL_TARBALL_TEMPLATE % test_channel

    # Must use an absolute tarball path since tar is run in a different cwd.
    tarball_path = os.path.join(control_dir, tarball_name)

    # Create the tarball.
    logging.info('Packing %s in %s into %s', self.CONTROL_FILE_SUBDIR,
                 control_dir, tarball_path)
    cmd_result = cros_build_lib.CreateTarball(
        tarball_path, control_dir,
        compression=cros_build_lib.COMP_BZIP2,
        inputs=[self.CONTROL_FILE_SUBDIR])
    if cmd_result.returncode != 0:
      logging.error('Error (%d) when tarring control files',
                    cmd_result.returncode)
      raise PayloadTestError(
          'failed to create autotest tarball (return code %d)' %
          cmd_result.returncode)

    # Upload the tarball, be sure to make it world-readable.
    upload_target = os.path.join(self._archive_build_uri, tarball_name)
    logging.info('Uploading autotest control tarball to %s', upload_target)
    gslib.Copy(tarball_path, upload_target, acl='public-read')

    # Do not run the suite for older builds whose suite staging logic is
    # broken.  We use the build's milestone number as a rough estimate to
    # whether or not it's recent enough. We derive the milestone number from
    # the archive build name, which takes the form
    # boardname-release/R12-3456.78.9 (in this case it is 12).
    try:
      build_mstone = int(self._archive_build.partition('/')[2]
                         .partition('-')[0][1:])
      if build_mstone < RUN_SUITE_MIN_MSTONE:
        logging.warning('Build milestone < %s, test suite scheduling skipped',
                        RUN_SUITE_MIN_MSTONE)
        return
    except ValueError:
      raise PayloadTestError(
          'Failed to infer archive build milestone number (%s)' %
          self._archive_build)

    # Send the information needed to actually schedule and run the tests.
    return suite_name

  def _CleanupBuild(self):
    """Clean up any leaked temp files associated with this build in GS."""
    # Clean up any signer client files that leaked on this or previous
    # runs.
    self._drm(gslib.Remove,
              gspaths.ChromeosReleases.BuildPayloadsSigningUri(
                  self._build.channel, self._build.board, self._build.version,
                  bucket=self._build.bucket),
              recurse=True, ignore_no_match=True)

  def CreatePayloads(self):
    """Get lock on this build, and Process if we succeed.

    While holding the lock, check assorted build flags to see if we should
    process this build.

    Raises:
      BuildLocked: If the build is locked by another server or process.
    """
    lock_uri = self._GetFlagURI(gspaths.ChromeosReleases.LOCK)
    suite_name = None

    logging.info('Examining: %s', self._build)

    try:
      with gslock.Lock(lock_uri, dry_run=bool(self._drm)):
        logging.info('Starting: %s', self._build)

        payloads, payload_tests = self._DiscoverRequiredPayloads()

        # Find out which payloads already exist, updating the payload object's
        # URI accordingly. In doing so we're creating a list of all payload
        # objects and their skip/exist attributes. We're also recording whether
        # this run will be skipping any actual work.
        for p in payloads:
          result = paygen_payload_lib.FindExistingPayloads(p)
          if result:
            p.exists = True
            p.uri = result[0]

        # Display the required payload generation list.
        log_items = []
        for p in payloads:
          desc = str(p)
          if p['exists']:
            desc += ' (exists)'
          log_items.append(desc)

        _LogList('All payloads for the build', log_items)

        # Generate new payloads.
        new_payloads = [p for p in payloads if not p['exists']]
        if new_payloads:
          logging.info('Generating %d new payload(s)', len(new_payloads))
          self._GeneratePayloads(new_payloads)
          logging.info('Finished generating payloads: %s', self._build)
        else:
          logging.info('No new payloads to generate')

        # Check that the build has a corresponding archive directory. The lab
        # can only execute control files for tests from this location.
        archive_board, archive_build, archive_build_uri = (
            self._MapToArchive(self._build.board, self._build.version))
        self._archive_board = archive_board
        self._archive_build = archive_build
        self._archive_build_uri = archive_build_uri

        # We have a control file directory and all payloads have been
        # generated. Lets create the list of tests to conduct.
        logging.info('Uploading %d payload tests', len(payload_tests))
        suite_name = self._drm(self._AutotestPayloads, payload_tests)

    except gslock.LockNotAcquired as e:
      logging.info('Build already being processed: %s', e)
      raise BuildLocked()

    except EarlyExit:
      logging.info('Nothing done: %s', self._build)
      raise

    except Exception:
      logging.error('Failed: %s', self._build)
      raise

    finally:
      self._CleanupBuild()

    return suite_name, self._archive_board, self._archive_build


def ValidateBoardConfig(board):
  """Validate that we have config values for the specified |board|.

  Args:
    board: Name of board to check in release namespace.

  Raises:
    BoardNotConfigured if the board is unknown.
  """
  if not PaygenBuild.GetPaygenJson(board):
    raise BoardNotConfigured(board)


def ScheduleAutotestTests(suite_name, board, model, build, skip_duts_check,
                          debug, job_keyvals=None):
  """Run the appropriate command to schedule the Autotests we have prepped.

  Args:
  suite_name: The name of the test suite.
  board: A string representing the name of the archive board.
  model: The model that will be tested against.
  build: A string representing the name of the archive build.
  skip_duts_check: A boolean indicating to not check minimum available DUTs.
  debug: A boolean indicating whether or not we are in debug mode.
  job_keyvals: A dict of job keyvals to be injected to suite control file.
  """
  timeout_mins = config_lib.HWTestConfig.SHARED_HW_TEST_TIMEOUT / 60
  cmd_result = commands.RunHWTestSuite(
      board=board,
      model=model,
      build=build,
      suite=suite_name,
      file_bugs=True,
      pool='bvt',
      priority=constants.HWTEST_BUILD_PRIORITY,
      retry=True,
      wait_for_results=False,
      timeout_mins=timeout_mins,
      suite_min_duts=2,
      debug=debug,
      skip_duts_check=skip_duts_check,
      job_keyvals=job_keyvals)

  if cmd_result.to_raise:
    if isinstance(cmd_result.to_raise, failures_lib.TestWarning):
      logging.warning('Warning running test suite; error output:\n%s',
                      cmd_result.to_raise)
    else:
      raise cmd_result.to_raise


# If the downloaded JSON is bad, a ValueError exception will be rasied.
# This appears to be a sporadic GS flake that a retry can fix.
def _GetJson(uri):
  """Downloads JSON from URI and parses it.

  Args:
    uri: The URI of a JSON file at the given GS URI.

  Returns:
    Valid JSON retrieved from given uri.
  """
  downloaded_json = gslib.Cat(uri)
  return json.loads(downloaded_json)
