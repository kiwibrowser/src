# -*- coding: utf-8 -*-
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Module containing the AFDO stages."""

from __future__ import print_function

from chromite.cbuildbot import afdo
from chromite.lib import constants
from chromite.lib import alerts
from chromite.lib import cros_logging as logging
from chromite.lib import gs
from chromite.lib import path_util
from chromite.lib import portage_util
from chromite.cbuildbot.stages import generic_stages


class AFDODataGenerateStage(generic_stages.BoardSpecificBuilderStage,
                            generic_stages.ForgivingBuilderStage):
  """Stage that generates AFDO profile data from a perf profile."""

  def _GetCurrentArch(self):
    """Get architecture for the current board being built."""
    return self._GetPortageEnvVar('ARCH', self._current_board)

  def PerformStage(self):
    """Collect a 'perf' profile and convert it into the AFDO format."""
    super(AFDODataGenerateStage, self).PerformStage()

    board = self._current_board
    if not afdo.CanGenerateAFDOData(board):
      logging.warning('Board %s cannot generate its own AFDO profile.', board)
      return

    arch = self._GetCurrentArch()
    buildroot = self._build_root
    gs_context = gs.GSContext()
    cpv = portage_util.BestVisible(constants.CHROME_CP,
                                   buildroot=buildroot)
    afdo_file = None

    # Generation of AFDO could fail for different reasons.
    # We will ignore the failures and let the master PFQ builder try
    # to find an older AFDO profile.
    try:
      if afdo.WaitForAFDOPerfData(cpv, arch, buildroot, gs_context):
        afdo_file = afdo.GenerateAFDOData(cpv, arch, board,
                                          buildroot, gs_context)
        assert afdo_file
        logging.info('Generated %s AFDO profile %s', arch, afdo_file)
      else:
        raise afdo.MissingAFDOData('Could not find current "perf" profile. '
                                   'Master PFQ builder will try to use stale '
                                   'AFDO profile.')
    # Will let system-exiting exceptions through.
    except Exception:
      logging.PrintBuildbotStepWarnings()
      logging.warning('AFDO profile generation failed with exception ',
                      exc_info=True)

      alert_msg = ('Please triage. This will become a fatal error.\n\n'
                   'arch=%s buildroot=%s\n\nURL=%s' %
                   (arch, buildroot, self._run.ConstructDashboardURL()))
      subject_msg = ('Failure in generation of AFDO Data for builder %s' %
                     self._run.config.name)
      alerts.SendEmailLog(subject_msg,
                          afdo.AFDO_ALERT_RECIPIENTS,
                          server=alerts.SmtpServer(constants.GOLO_SMTP_SERVER),
                          message=alert_msg)
      # Re-raise whatever exception we got here. This stage will only
      # generate a warning but we want to make sure the warning is
      # generated.
      raise


class AFDOUpdateChromeEbuildStage(generic_stages.BuilderStage):
  """Updates the Chrome ebuild with the names of the AFDO profiles."""

  def PerformStage(self):
    buildroot = self._build_root
    gs_context = gs.GSContext()
    cpv = portage_util.BestVisible(constants.CHROME_CP,
                                   buildroot=buildroot)

    # We need the name of one board that has been setup in this
    # builder to find the Chrome ebuild. The chrome ebuild should be
    # the same for all the boards, so just use the first one.
    # If we don't have any boards, leave the called function to guess.
    board = self._boards[0] if self._boards else None
    profiles = {}

    for source, getter in afdo.PROFILE_SOURCES.iteritems():
      profile = getter(cpv, source, buildroot, gs_context)
      if not profile:
        raise afdo.MissingAFDOData(
            'Could not find appropriate profile for %s' % source)
      logging.info('Found AFDO profile %s for %s', profile, source)
      profiles[source] = profile

    # Now update the Chrome ebuild file with the AFDO profiles we found
    # for each source.
    afdo.UpdateChromeEbuildAFDOFile(board, profiles)


class AFDOUpdateKernelEbuildStage(generic_stages.BuilderStage):
  """Updates the Kernel ebuild with the names of the AFDO profiles."""

  def _WarnSheriff(self, versions):
    subject_msg = ('Kernel AutoFDO profile too old for builder %s' %
                   self._run.config.name)
    alert_msg = ('AutoFDO profile too old for kernel %s. URL=%s' %
                 (versions, self._run.ConstructDashboardURL()))
    alerts.SendEmailLog(subject_msg, afdo.AFDO_ALERT_RECIPIENTS,
                        server=alerts.SmtpServer(constants.GOLO_SMTP_SERVER),
                        message=alert_msg)

  def PerformStage(self):
    version_info = self._run.GetVersionInfo()
    build_version = map(int, version_info.VersionString().split('.'))
    chrome_version = int(version_info.chrome_branch)
    target_version = [chrome_version] + build_version
    profile_versions = afdo.GetAvailableKernelProfiles()
    candidates = sorted(afdo.FindKernelEbuilds())
    expire_soon = set()
    not_found = set()
    expired = set()
    for candidate, kver in candidates:
      profile_version = None
      if kver in profile_versions:
        profile_version = afdo.FindLatestProfile(target_version,
                                                 profile_versions[kver])
      if not profile_version:
        not_found.add(kver)
        continue
      if afdo.ProfileAge(profile_version) > afdo.KERNEL_ALLOWED_STALE_DAYS:
        expired.add('%s: %s' % (kver, profile_version))
        continue
      if afdo.ProfileAge(profile_version) > afdo.KERNEL_WARN_STALE_DAYS:
        expire_soon.add('%s: %s' % (kver, profile_version))

      afdo.PatchKernelEbuild(candidate, profile_version)
      # If the *-9999.ebuild is not the last entry in its directory, Manifest
      # will contain an unused line for previous profile which is still fine.
      if candidate.endswith('-9999.ebuild'):
        afdo.UpdateManifest(path_util.ToChrootPath(candidate))
    afdo.CommitIfChanged(afdo.KERNEL_EBUILD_ROOT,
                         'Update profiles and manifests for Kernel.')

    if not_found or expired:
      raise afdo.NoValidProfileFound(
          'Cannot find AutoFDO profiles: %s or expired: %s' %
          (not_found, expired)
      )

    if expire_soon:
      self._WarnSheriff(expire_soon)
