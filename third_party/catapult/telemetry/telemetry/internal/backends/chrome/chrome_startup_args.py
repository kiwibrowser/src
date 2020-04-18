# Copyright 2018 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

from telemetry.core import exceptions
from telemetry.core import util
from telemetry.internal.browser import user_agent


def GetFromBrowserOptions(browser_options):
  """Get a list of startup args from the given browser_options."""
  assert not '--no-proxy-server' in browser_options.extra_browser_args, (
      '--no-proxy-server flag is disallowed as Chrome needs to be route to '
      'ts_proxy_server')

  # Merge multiple instances of --enable-features and --disable-features since
  # Chrome ends up using whatever switch it finds last instead of merging
  # multiple instances.
  # TODO(crbug.com/799411): Remove this once the smarter ChromeArgsBuilder is
  # implemented.
  args = []
  disable_features = set()
  enable_features = set()
  for arg in browser_options.extra_browser_args:
    if arg.startswith('--disable-features='):
      disable_features.update(arg.split('=', 1)[1].split(','))
    elif arg.startswith('--enable-features='):
      enable_features.update(arg.split('=', 1)[1].split(','))
    else:
      args.append(arg)

  if disable_features:
    args.append('--disable-features=%s' % ','.join(disable_features))
  if enable_features:
    args.append('--enable-features=%s' % ','.join(enable_features))

  args.append('--enable-net-benchmarking')
  args.append('--metrics-recording-only')
  args.append('--no-default-browser-check')
  args.append('--no-first-run')

  # Turn on GPU benchmarking extension for all runs. The only side effect of
  # the extension being on is that render stats are tracked. This is believed
  # to be effectively free. And, by doing so here, it avoids us having to
  # programmatically inspect a pageset's actions in order to determine if it
  # might eventually scroll.
  args.append('--enable-gpu-benchmarking')

  # Suppress all permission prompts by atomatically denying them.
  args.append('--deny-permission-prompts')

  # Override the need for a user gesture in order to play media.
  args.append('--autoplay-policy=no-user-gesture-required')

  if browser_options.disable_background_networking:
    args.append('--disable-background-networking')

  args.extend(user_agent.GetChromeUserAgentArgumentFromType(
      browser_options.browser_user_agent_type))

  if browser_options.disable_component_extensions_with_background_pages:
    args.append('--disable-component-extensions-with-background-pages')

  # Disables the start page, as well as other external apps that can
  # steal focus or make measurements inconsistent.
  if browser_options.disable_default_apps:
    args.append('--disable-default-apps')

  # Disable the search geolocation disclosure infobar, as it is only shown a
  # small number of times to users and should not be part of perf comparisons.
  args.append('--disable-search-geolocation-disclosure')

  if (browser_options.logging_verbosity ==
      browser_options.NON_VERBOSE_LOGGING):
    args.extend(['--enable-logging', '--v=0'])
  elif (browser_options.logging_verbosity ==
        browser_options.VERBOSE_LOGGING):
    args.extend(['--enable-logging', '--v=1'])
  elif (browser_options.logging_verbosity ==
        browser_options.SUPER_VERBOSE_LOGGING):
    args.extend(['--enable-logging', '--v=2'])

  extensions = [e.local_path for e in browser_options.extensions_to_load]
  if extensions:
    args.append('--load-extension=%s' % ','.join(extensions))

  return args


def GetReplayArgs(network_backend, supports_spki_list=True):
  args = []
  if not network_backend.is_open:
    return args

  proxy_port = network_backend.forwarder.remote_port
  args.append('--proxy-server=socks://localhost:%s' % proxy_port)
  if not network_backend.use_live_traffic:
    if supports_spki_list:
      # Ignore certificate errors for certs that are signed with Wpr's root.
      # For more details on this flag, see crbug.com/753948.
      wpr_public_hash_file = os.path.join(
          util.GetCatapultDir(), 'web_page_replay_go', 'wpr_public_hash.txt')
      if not os.path.exists(wpr_public_hash_file):
        raise exceptions.PathMissingError(
            'Unable to find %s' % wpr_public_hash_file)
      with open(wpr_public_hash_file) as f:
        wpr_public_hash = f.readline().strip()
      args.append('--ignore-certificate-errors-spki-list=' + wpr_public_hash)
    else:
      # If --ignore-certificate-errors-spki-list is not supported ignore all
      # certificate errors.
      args.append('--ignore-certificate-errors')

  return args
