# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Get and parse options from CQ config files for changes."""

from __future__ import print_function

import ConfigParser
import os

from chromite.lib import config_lib
from chromite.lib import constants
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import git


site_config = config_lib.GetConfig()


class MalformedCQConfigException(Exception):
  """Exception class presenting a malformed CQ config file."""

  def __init__(self, change, config_file, error):
    self.change = change
    self.config_file = config_file
    self.error = error
    super(MalformedCQConfigException, self).__init__(
        '%s has malformed config file %s: %s' % (
            self.change, self.config_file, self.error))


class CQConfigParser(object):
  """Class to parse options for a change from its CQ config files."""

  def __init__(self, build_root, change, forgiving=True):
    """Initialize a CQConfigParser instance for a change.

    Args:
      build_root: The path to the build root.
      change: An instance of cros_patch.GerritPatch.
      forgiving: If false, throw MalformedCQConfigException if encountering
                 parsing errors. Otherwise, log them and discard failures.
                 Default: True.
    """
    self.build_root = build_root
    self.change = change
    self._common_config_file = self.GetCommonConfigFileForChange(
        build_root, change)
    self.forgiving = forgiving

  def GetOption(self, section, option, config_path=None):
    """Get |option| from |section| for self.change.

    Args:
      section: Section header name (string).
      option: Option name (string).
      config_path: The path to the config to get the option value. When
        config_path is None, use self._common_config_file as the default config.

    Returns:
      The value of the option (string) or None.

    Raises:
      MalformedCQConfigException if the config is malformed and parser is
      non-forgiving.
    """
    result = None
    config_path = config_path or self._common_config_file
    if config_path is not None:
      try:
        result = self._GetOptionFromConfigFile(
            config_path, section, option)
      except ConfigParser.Error as e:
        error = MalformedCQConfigException(
            self.change, config_path, e)
        if self.forgiving:
          logging.error('Forgiving a malformed CQ config: %s', error)
        else:
          raise error
    return result

  def GetPreCQConfigs(self):
    """Get a list of Pre-CQ configs from config for self.change.

    Retuns:
      A list of Pre-CQ configs (strings).
    """
    result = self.GetOption(constants.CQ_CONFIG_SECTION_GENERAL,
                            constants.CQ_CONFIG_PRE_CQ_CONFIGS)
    return result.split() if result else []

  def GetStagesToIgnore(self):
    """Get a list of stages that the CQ should ignore for self.change.

    The section and option in the config file COMMIT-QUEUE.ini would be like:

    [GENERAL]
      ignored-stages: HWTest VMTest

    The CQ will submit changes to the given project even if the listed stages
    failed. These strings are stage name prefixes, meaning that "HWTest" would
    match any HWTest stage (e.g. "HWTest [bvt]" or "HWTest [foo]")

    Returns:
      A list of stages (strings) to ignore for self.change.
    """
    result = self.GetOption(constants.CQ_CONFIG_SECTION_GENERAL,
                            constants.CQ_CONFIG_IGNORED_STAGES)
    return result.split() if result else []

  def GetSubsystems(self):
    """Get a list of subsystems from config for self.change.

    Retuns:
      A list of subsystems (strings).
    """
    result = self.GetOption(constants.CQ_CONFIG_SECTION_GENERAL,
                            constants.CQ_CONFIG_SUBSYSTEM)
    return result.split() if result else []

  def GetConfigFlag(self, section, option):
    """Get config flag.

    Args:
      section: Section header name (string).
      option: Option name (string).

    Returns:
      A boolean value of the config flag.
    """
    result = self.GetOption(section, option)
    return bool(result and result.lower() == 'yes')

  def GetUnionPreCQSubConfigsFlag(self):
    """Whether to union Pre-CQ configs of the config files from sub dirs.

    Returns:
      A boolean indicating whether to union Pre-CQ from sub dir configs.
    """
    return self.GetConfigFlag(
        constants.CQ_CONFIG_SECTION_GENERAL,
        constants.CQ_CONFIG_UNION_PRE_CQ_SUB_CONFIGS)

  def GetUnionedPreCQConfigs(self):
    """Get Pre-CQ configs from unioned options of sub configs.

    Returns:
      A list of Pre-CQ configs (strings).
    """
    unioned_pre_cq_config_options = self._GetUnionedOptionFromSubConfigs(
        constants.CQ_CONFIG_SECTION_GENERAL,
        constants.CQ_CONFIG_PRE_CQ_CONFIGS)

    pre_cq_configs = set()
    for option in unioned_pre_cq_config_options:
      if option:
        pre_cq_configs.update(option.split())

    return pre_cq_configs

  def CanSubmitChangeInPreCQ(self):
    """Infer if the Pre-CQ config lets this change to be submitted in pre-cq.

    This looks up the "submit-in-pre-cq" setting inside all the relevant
    COMMIT-QUEUE.ini files for the current change and checks whether it is set
    to "yes".

    [GENERAL]
      submit-in-pre-cq: yes

    Returns:
      True if current change may be submitted from pre-cq, False otherwise.
    """
    if not self.GetUnionPreCQSubConfigsFlag():
      option = self.GetOption(constants.CQ_CONFIG_SECTION_GENERAL,
                              constants.CQ_CONFIG_SUBMIT_IN_PRE_CQ)
      return bool(option and option.lower() == 'yes')

    return self._AllSubConfigsAgreeOnOption(
        constants.CQ_CONFIG_SECTION_GENERAL,
        constants.CQ_CONFIG_SUBMIT_IN_PRE_CQ,
        'yes'
    )

  @classmethod
  def GetCheckout(cls, build_root, change):
    """Get the ProjectCheckout associated with change.

    Args:
      build_root: The path to the build root.
      change: An instance of cros_patch.GerritPatch.

    Returns:
      A ProjectCheckout instance of |change| or None (See more details in
      patch.GitRepoPatch.GetCheckout)
    """
    manifest = git.ManifestCheckout.Cached(build_root)
    return change.GetCheckout(manifest)

  def _GetUnionedOptionFromSubConfigs(self, section, option):
    """Get unioned options from sub dir configs for change.

    This method looks for the config file for each diff file in the change,
    gets the option from each config file and returns the set of found options.

    Args:
      section: The section header (string) of the option.
      option: The option name (string) to get.

    Returns:
      A set of options (strings) from sub-dir configs.
    """
    options = self._GetAllValuesForOptionFromSubConfigs(section, option)
    return set(options)

  def _AllSubConfigsAgreeOnOption(self, section, option, value):
    """Determines if all sub-configs have the given value for the option.

    Args:
      section: The section header (string) of the option.
      option: The option name (string) to get.
      value: The option value (string) to compare with all options. Comparison
          is case insensitive.

    Returns:
      True if all sub-configs agree on the option, False otherwise.
    """
    options = self._GetAllValuesForOptionFromSubConfigs(section, option,
                                                        'not-%s' % value)
    return set(o.lower() for o in options) == {value.lower()}

  def _GetAllValuesForOptionFromSubConfigs(self, section, option, default=None):
    """Get a list of all values for the given option from relevant sub-configs.

    This method looks for the config file for each diff file in the change, gets
    the option from each config file and returns the list of all values found.

    Args:
      section: The section header (string) of the option.
      option: The option name (string) to get.
      default: If not None, this value is assumed for a sub-config with the
          option missing.

    Returns:
      A list of options (strings) from sub-dir configs.
    """
    checkout = self.GetCheckout(self.build_root, self.change)
    if not checkout:
      return []

    checkout_path = checkout.GetPath(absolute=True)
    affected_paths = [os.path.join(checkout_path, path)
                      for path in self.change.GetDiffStatus(checkout_path)]
    config_paths = set()
    for affected_path in affected_paths:
      config_path = self._GetConfigFileForAffectedPath(
          affected_path, checkout_path)
      if config_path:
        config_paths.add(config_path)

    options = []
    for config_path in config_paths:
      result = self.GetOption(section, option, config_path=config_path)
      if result:
        options.append(result)
      elif default is not None:
        options.append(default)
    return options

  @classmethod
  def _GetOptionFromConfigFile(cls, config_path, section, option):
    """Get |option| from |section| in |config_path|.

    Args:
      config_path: The path to the CQ config file.
      section: Section header name (string).
      option: Option name (string).

    Returns:
      The value (string) of the option.
    """
    parser = ConfigParser.SafeConfigParser()
    parser.read(config_path)
    if parser.has_option(section, option):
      return parser.get(section, option)

  @classmethod
  def _GetCommonAffectedSubdir(cls, change, git_repo):
    """Gets the longest common path of changes in |change|.

    Args:
      change: An instance of cros_patch.GerritPatch.
      git_repo: Path to checkout of git repository.

    Returns:
      An absolute path in |git_repo|.
    """
    affected_paths = [os.path.join(git_repo, path)
                      for path in change.GetDiffStatus(git_repo)]
    return cros_build_lib.GetCommonPathPrefix(affected_paths)

  @classmethod
  def GetCommonConfigFileForChange(cls, build_root, change):
    """Get the config file from the lowest common parent dir of the |change|.

    This will look for a config file from the common parent directory of all the
    changed files from |change|. If no config file is found in that directory,
    it will continue up the directory tree until it finds one. If no config file
    is found within the project checkout path, a config file path in the root of
    the checkout will be returned, in which case the file is not guaranteed to
    exist. See
    http://chromium.org/chromium-os/build/bypassing-tests-on-a-per-project-basis

    Args:
      build_root: The path to the build root.
      change: An instance of cros_patch.GerritPatch.

    Returns:
      Path to the config file to be read for |change|. If no config file is
      found within the project checkout, return a config file path in the root
      of the checkout.
    """
    checkout = cls.GetCheckout(build_root, change)
    if checkout:
      checkout_path = checkout.GetPath(absolute=True)
      current_dir = cls._GetCommonAffectedSubdir(change, checkout_path)
      while True:
        config_file = os.path.join(current_dir, constants.CQ_CONFIG_FILENAME)
        if os.path.isfile(config_file) or checkout_path.startswith(current_dir):
          return config_file
        assert current_dir not in ('/', '')
        current_dir = os.path.dirname(current_dir)

  @classmethod
  def _GetConfigFileForAffectedPath(cls, affected_path, checkout_path):
    """Get config file for the affected path starting from the base dir.

    If the checkout_path is '/project_root', the changed file is located at
    '/project_root/dir_1/dir_2/file', and there're config files located at
    '/porject_root/config.ini' and at '/project_root/dir_1/config.ini'. This
    method looks for the config file starting from '/project_root/dir_1/dir_2/'
    to the parent dirs till the checkout_path '/project_root', returns the path
    to the config file once it's found. In this case,
    '/project_root/dir_1/config.ini' will be returned.

    Args:
      affected_path: The path to the affected(changed) file.
      checkout_path: The path to the project checkout.

    Returns:
      The path to the config. The returned path will be within |checkout_path|.
      If no config file was found, a config file path in the root of the
      checkout will be returned, in which case the file is not guaranteed to
      exist.
    """
    current_path = affected_path

    while not checkout_path.startswith(current_path):
      current_path = os.path.dirname(current_path)
      config_file = os.path.join(current_path, constants.CQ_CONFIG_FILENAME)
      if os.path.isfile(config_file):
        return config_file

    return os.path.join(current_path, constants.CQ_CONFIG_FILENAME)
