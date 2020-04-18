# -*- coding: utf-8 -*-
# Copyright 2011 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""Class that runs a named gsutil command."""

from __future__ import absolute_import

import difflib
import logging
import os
import pkgutil
import sys
import textwrap
import time

import boto
from boto.storage_uri import BucketStorageUri
import gslib
from gslib import metrics
from gslib.cloud_api_delegator import CloudApiDelegator
from gslib.command import Command
from gslib.command import CreateGsutilLogger
from gslib.command import GetFailureCount
from gslib.command import OLD_ALIAS_MAP
from gslib.command import ShutDownGsutil
import gslib.commands
from gslib.cs_api_map import ApiSelector
from gslib.cs_api_map import GsutilApiClassMapFactory
from gslib.cs_api_map import GsutilApiMapFactory
from gslib.exception import CommandException
from gslib.gcs_json_api import GcsJsonApi
from gslib.no_op_credentials import NoOpCredentials
from gslib.tab_complete import MakeCompleter
from gslib.util import CheckMultiprocessingAvailableAndInit
from gslib.util import CompareVersions
from gslib.util import DiscardMessagesQueue
from gslib.util import GetGsutilVersionModifiedTime
from gslib.util import GSUTIL_PUB_TARBALL
from gslib.util import InsistAsciiHeader
from gslib.util import InsistAsciiHeaderValue
from gslib.util import IsCustomMetadataHeader
from gslib.util import IsRunningInteractively
from gslib.util import LAST_CHECKED_FOR_GSUTIL_UPDATE_TIMESTAMP_FILE
from gslib.util import LookUpGsutilVersion
from gslib.util import RELEASE_NOTES_URL
from gslib.util import SECONDS_PER_DAY
from gslib.util import UTF8


def HandleHeaderCoding(headers):
  """Handles coding of headers and their values. Alters the dict in-place.

  Converts a dict of headers and their values to their appropriate types. We
  ensure that all headers and their values will contain only ASCII characters,
  with the exception of custom metadata header values; these values may contain
  Unicode characters, and thus if they are not already unicode-type objects,
  we attempt to decode them to Unicode using UTF-8 encoding.

  Args:
    headers: A dict mapping headers to their values. All keys and values must
        be either str or unicode objects.

  Raises:
    CommandException: If a header or its value cannot be encoded in the
        required encoding.
  """
  if not headers:
    return

  for key in headers:
    InsistAsciiHeader(key)
    if IsCustomMetadataHeader(key):
      if not isinstance(headers[key], unicode):
        try:
          headers[key] = headers[key].decode(UTF8)
        except UnicodeDecodeError:
          raise CommandException('\n'.join(textwrap.wrap(
              'Invalid encoding for header value (%s: %s). Values must be '
              'decodable as Unicode. NOTE: the value printed above '
              'replaces the problematic characters with a hex-encoded '
              'printable representation. For more details (including how to '
              'convert to a gsutil-compatible encoding) see `gsutil help '
              'encoding`.' % (repr(key), repr(headers[key])))))
    else:
      # Non-custom-metadata headers and their values must be ASCII characters.
      InsistAsciiHeaderValue(key, headers[key])


def HandleArgCoding(args):
  """Handles coding of command-line args. Alters the list in-place.

  Args:
    args: A list of command-line args.

  Raises:
    CommandException: if errors encountered.
  """
  # Python passes arguments from the command line as byte strings. To
  # correctly interpret them, we decode them as utf-8.
  for i in range(len(args)):
    arg = args[i]
    if not isinstance(arg, unicode):
      try:
        args[i] = arg.decode(UTF8)
      except UnicodeDecodeError:
        raise CommandException('\n'.join(textwrap.wrap(
            'Invalid encoding for argument (%s). Arguments must be decodable '
            'as Unicode. NOTE: the argument printed above replaces the '
            'problematic characters with a hex-encoded printable '
            'representation. For more details (including how to convert to a '
            'gsutil-compatible encoding) see `gsutil help encoding`.' %
            repr(arg))))


class CommandRunner(object):
  """Runs gsutil commands and does some top-level argument handling."""

  def __init__(self, bucket_storage_uri_class=BucketStorageUri,
               gsutil_api_class_map_factory=GsutilApiClassMapFactory,
               command_map=None):
    """Instantiates a CommandRunner.

    Args:
      bucket_storage_uri_class: Class to instantiate for cloud StorageUris.
                                Settable for testing/mocking.
      gsutil_api_class_map_factory: Creates map of cloud storage interfaces.
                                    Settable for testing/mocking.
      command_map: Map of command names to their implementations for
                   testing/mocking. If not set, the map is built dynamically.
    """
    self.bucket_storage_uri_class = bucket_storage_uri_class
    self.gsutil_api_class_map_factory = gsutil_api_class_map_factory
    if command_map:
      self.command_map = command_map
    else:
      self.command_map = self._LoadCommandMap()

  def _LoadCommandMap(self):
    """Returns dict mapping each command_name to implementing class."""
    # Import all gslib.commands submodules.
    for _, module_name, _ in pkgutil.iter_modules(gslib.commands.__path__):
      __import__('gslib.commands.%s' % module_name)

    command_map = {}
    # Only include Command subclasses in the dict.
    for command in Command.__subclasses__():
      command_map[command.command_spec.command_name] = command
      for command_name_aliases in command.command_spec.command_name_aliases:
        command_map[command_name_aliases] = command
    return command_map

  def _ConfigureCommandArgumentParserArguments(
      self, parser, arguments, gsutil_api):
    """Configures an argument parser with the given arguments.

    Args:
      parser: argparse parser object.
      arguments: array of CommandArgument objects.
      gsutil_api: gsutil Cloud API instance to use.
    Raises:
      RuntimeError: if argument is configured with unsupported completer
    """
    for command_argument in arguments:
      action = parser.add_argument(
          *command_argument.args, **command_argument.kwargs)
      if command_argument.completer:
        action.completer = MakeCompleter(command_argument.completer, gsutil_api)

  def ConfigureCommandArgumentParsers(self, subparsers):
    """Configures argparse arguments and argcomplete completers for commands.

    Args:
      subparsers: argparse object that can be used to add parsers for
                  subcommands (called just 'commands' in gsutil)
    """

    # This should match the support map for the "ls" command.
    support_map = {
        'gs': [ApiSelector.XML, ApiSelector.JSON],
        's3': [ApiSelector.XML]
    }
    default_map = {
        'gs': ApiSelector.JSON,
        's3': ApiSelector.XML
    }
    gsutil_api_map = GsutilApiMapFactory.GetApiMap(
        self.gsutil_api_class_map_factory, support_map, default_map)

    logger = CreateGsutilLogger('tab_complete')
    gsutil_api = CloudApiDelegator(
        self.bucket_storage_uri_class, gsutil_api_map,
        logger, DiscardMessagesQueue(), debug=0)

    for command in set(self.command_map.values()):
      command_parser = subparsers.add_parser(
          command.command_spec.command_name, add_help=False)
      if isinstance(command.command_spec.argparse_arguments, dict):
        subcommand_parsers = command_parser.add_subparsers()
        subcommand_argument_dict = command.command_spec.argparse_arguments
        for subcommand, arguments in subcommand_argument_dict.iteritems():
          subcommand_parser = subcommand_parsers.add_parser(
              subcommand, add_help=False)
          self._ConfigureCommandArgumentParserArguments(
              subcommand_parser, arguments, gsutil_api)
      else:
        self._ConfigureCommandArgumentParserArguments(
            command_parser, command.command_spec.argparse_arguments, gsutil_api)

  def RunNamedCommand(self, command_name, args=None, headers=None, debug=0,
                      trace_token=None, parallel_operations=False,
                      skip_update_check=False, logging_filters=None,
                      do_shutdown=True, perf_trace_token=None,
                      user_project=None,
                      collect_analytics=False):
    """Runs the named command.

    Used by gsutil main, commands built atop other commands, and tests.

    Args:
      command_name: The name of the command being run.
      args: Command-line args (arg0 = actual arg, not command name ala bash).
      headers: Dictionary containing optional HTTP headers to pass to boto.
      debug: Debug level to pass in to boto connection (range 0..3).
      trace_token: Trace token to pass to the underlying API.
      parallel_operations: Should command operations be executed in parallel?
      skip_update_check: Set to True to disable checking for gsutil updates.
      logging_filters: Optional list of logging.Filters to apply to this
          command's logger.
      do_shutdown: Stop all parallelism framework workers iff this is True.
      perf_trace_token: Performance measurement trace token to pass to the
          underlying API.
      user_project: The project to bill this request to.
      collect_analytics: Set to True to collect an analytics metric logging this
          command.

    Raises:
      CommandException: if errors encountered.

    Returns:
      Return value(s) from Command that was run.
    """
    command_changed_to_update = False
    if (not skip_update_check and
        self.MaybeCheckForAndOfferSoftwareUpdate(command_name, debug)):
      command_name = 'update'
      command_changed_to_update = True
      args = ['-n']

      # Check for opt-in analytics.
      if IsRunningInteractively() and collect_analytics:
        metrics.CheckAndMaybePromptForAnalyticsEnabling()

    if not args:
      args = []

    # Include api_version header in all commands.
    api_version = boto.config.get_value('GSUtil', 'default_api_version', '1')
    if not headers:
      headers = {}
    headers['x-goog-api-version'] = api_version

    if command_name not in self.command_map:
      close_matches = difflib.get_close_matches(
          command_name, self.command_map.keys(), n=1)
      if close_matches:
        # Instead of suggesting a deprecated command alias, suggest the new
        # name for that command.
        translated_command_name = (
            OLD_ALIAS_MAP.get(close_matches[0], close_matches)[0])
        print >> sys.stderr, 'Did you mean this?'
        print >> sys.stderr, '\t%s' % translated_command_name
      elif command_name == 'update' and gslib.IS_PACKAGE_INSTALL:
        sys.stderr.write(
            'Update command is not supported for package installs; '
            'please instead update using your package manager.')

      raise CommandException('Invalid command "%s".' % command_name)
    if '--help' in args:
      new_args = [command_name]
      original_command_class = self.command_map[command_name]
      subcommands = original_command_class.help_spec.subcommand_help_text.keys()
      for arg in args:
        if arg in subcommands:
          new_args.append(arg)
          break  # Take the first match and throw away the rest.
      args = new_args
      command_name = 'help'

    HandleArgCoding(args)
    HandleHeaderCoding(headers)

    command_class = self.command_map[command_name]
    command_inst = command_class(
        self, args, headers, debug, trace_token, parallel_operations,
        self.bucket_storage_uri_class, self.gsutil_api_class_map_factory,
        logging_filters, command_alias_used=command_name,
        perf_trace_token=perf_trace_token, user_project=user_project)

    # Log the command name, command alias, and sub-options after being parsed by
    # RunCommand and the command constructor. For commands with subcommands and
    # suboptions, we need to log the suboptions again within the command itself
    # because the command constructor will not parse the suboptions fully.
    if collect_analytics:
      metrics.LogCommandParams(command_name=command_inst.command_name,
                               sub_opts=command_inst.sub_opts,
                               command_alias=command_name)

    return_code = command_inst.RunCommand()

    if CheckMultiprocessingAvailableAndInit().is_available and do_shutdown:
      ShutDownGsutil()
    if GetFailureCount() > 0:
      return_code = 1
    if command_changed_to_update:
      # If the command changed to update, the user's original command was
      # not executed.
      return_code = 1
      print '\n'.join(textwrap.wrap(
          'Update was successful. Exiting with code 1 as the original command '
          'issued prior to the update was not executed and should be re-run.'))
    return return_code

  def MaybeCheckForAndOfferSoftwareUpdate(self, command_name, debug):
    """Checks the last time we checked for an update and offers one if needed.

    Offer is made if the time since the last update check is longer
    than the configured threshold offers the user to update gsutil.

    Args:
      command_name: The name of the command being run.
      debug: Debug level to pass in to boto connection (range 0..3).

    Returns:
      True if the user decides to update.
    """
    # Don't try to interact with user if:
    # - gsutil is not connected to a tty (e.g., if being run from cron);
    # - user is running gsutil -q
    # - user is running the config command (which could otherwise attempt to
    #   check for an update for a user running behind a proxy, who has not yet
    #   configured gsutil to go through the proxy; for such users we need the
    #   first connection attempt to be made by the gsutil config command).
    # - user is running the version command (which gets run when using
    #   gsutil -D, which would prevent users with proxy config problems from
    #   sending us gsutil -D output).
    # - user is running the update command (which could otherwise cause an
    #   additional note that an update is available when user is already trying
    #   to perform an update);
    # - user specified gs_host (which could be a non-production different
    #   service instance, in which case credentials won't work for checking
    #   gsutil tarball).
    # - user is using a Cloud SDK install (which should only be updated via
    #   gcloud components update)
    logger = logging.getLogger()
    gs_host = boto.config.get('Credentials', 'gs_host', None)
    if (not IsRunningInteractively()
        or command_name in ('config', 'update', 'ver', 'version')
        or not logger.isEnabledFor(logging.INFO)
        or gs_host
        or os.environ.get('CLOUDSDK_WRAPPER') == '1'):
      return False

    software_update_check_period = boto.config.getint(
        'GSUtil', 'software_update_check_period', 30)
    # Setting software_update_check_period to 0 means periodic software
    # update checking is disabled.
    if software_update_check_period == 0:
      return False

    cur_ts = int(time.time())
    if not os.path.isfile(LAST_CHECKED_FOR_GSUTIL_UPDATE_TIMESTAMP_FILE):
      # Set last_checked_ts from date of VERSION file, so if the user installed
      # an old copy of gsutil it will get noticed (and an update offered) the
      # first time they try to run it.
      last_checked_ts = GetGsutilVersionModifiedTime()
      with open(LAST_CHECKED_FOR_GSUTIL_UPDATE_TIMESTAMP_FILE, 'w') as f:
        f.write(str(last_checked_ts))
    else:
      try:
        with open(LAST_CHECKED_FOR_GSUTIL_UPDATE_TIMESTAMP_FILE, 'r') as f:
          last_checked_ts = int(f.readline())
      except (TypeError, ValueError):
        return False

    if (cur_ts - last_checked_ts
        > software_update_check_period * SECONDS_PER_DAY):
      # Create a credential-less gsutil API to check for the public
      # update tarball.
      gsutil_api = GcsJsonApi(self.bucket_storage_uri_class, logger,
                              DiscardMessagesQueue(),
                              credentials=NoOpCredentials(), debug=debug)

      cur_ver = LookUpGsutilVersion(gsutil_api, GSUTIL_PUB_TARBALL)
      with open(LAST_CHECKED_FOR_GSUTIL_UPDATE_TIMESTAMP_FILE, 'w') as f:
        f.write(str(cur_ts))
      (g, m) = CompareVersions(cur_ver, gslib.VERSION)
      if m:
        print '\n'.join(textwrap.wrap(
            'A newer version of gsutil (%s) is available than the version you '
            'are running (%s). NOTE: This is a major new version, so it is '
            'strongly recommended that you review the release note details at '
            '%s before updating to this version, especially if you use gsutil '
            'in scripts.' % (cur_ver, gslib.VERSION, RELEASE_NOTES_URL)))
        if gslib.IS_PACKAGE_INSTALL:
          return False
        print
        answer = raw_input('Would you like to update [y/N]? ')
        return answer and answer.lower()[0] == 'y'
      elif g:
        print '\n'.join(textwrap.wrap(
            'A newer version of gsutil (%s) is available than the version you '
            'are running (%s). A detailed log of gsutil release changes is '
            'available at %s if you would like to read them before updating.'
            % (cur_ver, gslib.VERSION, RELEASE_NOTES_URL)))
        if gslib.IS_PACKAGE_INSTALL:
          return False
        print
        answer = raw_input('Would you like to update [Y/n]? ')
        return not answer or answer.lower()[0] != 'n'
    return False
