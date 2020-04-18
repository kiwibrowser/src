#! /usr/bin/python
# Copyright 2015 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import cgi
import json
import logging
import os
import subprocess
import sys
import tempfile
import time

_SRC_DIR = os.path.abspath(os.path.join(
    os.path.dirname(__file__), '..', '..', '..'))

sys.path.append(os.path.join(_SRC_DIR, 'third_party', 'catapult', 'devil'))
from devil.android import device_utils
from devil.android.sdk import intent

sys.path.append(os.path.join(_SRC_DIR, 'build', 'android'))
import devil_chromium
from pylib import constants

import activity_lens
import clovis_constants
import content_classification_lens
import controller
import device_setup
import frame_load_lens
import loading_graph_view
import loading_graph_view_visualization
import loading_trace
import options
import request_dependencies_lens
import request_track
import xvfb_helper

# TODO(mattcary): logging.info isn't that useful, as the whole (tools) world
# uses logging info; we need to introduce logging modules to get finer-grained
# output. For now we just do logging.warning.


OPTIONS = options.OPTIONS


def _LoadPage(device, url):
  """Load a page on chrome on our device.

  Args:
    device: an AdbWrapper for the device on which to load the page.
    url: url as a string to load.
  """
  load_intent = intent.Intent(
      package=OPTIONS.ChromePackage().package,
      activity=OPTIONS.ChromePackage().activity,
      data=url)
  logging.warning('Loading ' + url)
  device.StartActivity(load_intent, blocking=True)


def _GetPrefetchHtml(graph_view, name=None):
  """Generate prefetch page for the resources in resource graph.

  Args:
    graph_view: (LoadingGraphView)
    name: optional string used in the generated page.

  Returns:
    HTML as a string containing all the link rel=prefetch directives necessary
    for prefetching the given ResourceGraph.
  """
  if name:
    title = 'Prefetch for ' + cgi.escape(name)
  else:
    title = 'Generated prefetch page'
  output = []
  output.append("""<!DOCTYPE html>
<html>
<head>
<title>%s</title>
""" % title)
  for node in graph_view.deps_graph.graph.Nodes():
    output.append('<link rel="prefetch" href="%s">\n' % node.request.url)
  output.append("""</head>
<body>%s</body>
</html>
  """ % title)
  return '\n'.join(output)


def _LogRequests(url, clear_cache_override=None):
  """Logs requests for a web page.

  Args:
    url: url to log as string.
    clear_cache_override: if not None, set clear_cache different from OPTIONS.

  Returns:
    JSON dict of logged information (ie, a dict that describes JSON).
  """
  xvfb_process = None
  if OPTIONS.local:
    chrome_ctl = controller.LocalChromeController()
    if OPTIONS.headless:
      xvfb_process =  xvfb_helper.LaunchXvfb()
      chrome_ctl.SetChromeEnvOverride(xvfb_helper.GetChromeEnvironment())
  else:
    chrome_ctl = controller.RemoteChromeController(
        device_setup.GetFirstDevice())

  clear_cache = (clear_cache_override if clear_cache_override is not None
                 else OPTIONS.clear_cache)
  if OPTIONS.emulate_device:
    chrome_ctl.SetDeviceEmulation(OPTIONS.emulate_device)
  if OPTIONS.emulate_network:
    chrome_ctl.SetNetworkEmulation(OPTIONS.emulate_network)
  try:
    with chrome_ctl.Open() as connection:
      if clear_cache:
        connection.ClearCache()
      trace = loading_trace.LoadingTrace.RecordUrlNavigation(
          url, connection, chrome_ctl.ChromeMetadata(),
          categories=clovis_constants.DEFAULT_CATEGORIES)
  except controller.ChromeControllerError as e:
    e.Dump(sys.stderr)
    raise

  if xvfb_process:
    xvfb_process.terminate()

  return trace.ToJsonDict()


def _FullFetch(url, json_output, prefetch):
  """Do a full fetch with optional prefetching."""
  if not url.startswith('http') and not url.startswith('file'):
    url = 'http://' + url
  logging.warning('Cold fetch')
  cold_data = _LogRequests(url)
  assert cold_data, 'Cold fetch failed to produce data. Check your phone.'
  if prefetch:
    assert not OPTIONS.local
    logging.warning('Generating prefetch')
    prefetch_html = _GetPrefetchHtml(_ProcessJsonTrace(cold_data), name=url)
    tmp = tempfile.NamedTemporaryFile()
    tmp.write(prefetch_html)
    tmp.flush()
    # We hope that the tmpfile name is unique enough for the device.
    target = os.path.join('/sdcard/Download', os.path.basename(tmp.name))
    device = device_setup.GetFirstDevice()
    device.adb.Push(tmp.name, target)
    logging.warning('Pushed prefetch %s to device at %s' % (tmp.name, target))
    _LoadPage(device, 'file://' + target)
    time.sleep(OPTIONS.prefetch_delay_seconds)
    logging.warning('Warm fetch')
    warm_data = _LogRequests(url, clear_cache_override=False)
    with open(json_output, 'w') as f:
      json.dump(warm_data, f)
    logging.warning('Wrote ' + json_output)
    with open(json_output + '.cold', 'w') as f:
      json.dump(cold_data, f)
    logging.warning('Wrote ' + json_output + '.cold')
  else:
    with open(json_output, 'w') as f:
      json.dump(cold_data, f)
    logging.warning('Wrote ' + json_output)


def _ProcessTraceFile(filename):
  with open(filename) as f:
    return _ProcessJsonTrace(json.load(f))


def _ProcessJsonTrace(json_dict):
    trace = loading_trace.LoadingTrace.FromJsonDict(json_dict)
    content_lens = (
        content_classification_lens.ContentClassificationLens.WithRulesFiles(
            trace, OPTIONS.ad_rules, OPTIONS.tracking_rules))
    frame_lens = frame_load_lens.FrameLoadLens(trace)
    activity = activity_lens.ActivityLens(trace)
    deps_lens = request_dependencies_lens.RequestDependencyLens(trace)
    graph_view = loading_graph_view.LoadingGraphView(
        trace, deps_lens, content_lens, frame_lens, activity)
    if OPTIONS.noads:
      graph_view.RemoveAds()
    return graph_view


def InvalidCommand(cmd):
  sys.exit('Invalid command "%s"\nChoices are: %s' %
           (cmd, ' '.join(COMMAND_MAP.keys())))


def DoPng(arg_str):
  OPTIONS.ParseArgs(arg_str, description='Generates a PNG from a trace',
                    extra=['request_json', ('--png_output', ''),
                           ('--eog', False)])
  graph_view = _ProcessTraceFile(OPTIONS.request_json)
  visualization = (
      loading_graph_view_visualization.LoadingGraphViewVisualization(
          graph_view))
  tmp = tempfile.NamedTemporaryFile()
  visualization.OutputDot(tmp)
  tmp.flush()
  png_output = OPTIONS.png_output
  if not png_output:
    if OPTIONS.request_json.endswith('.json'):
      png_output = OPTIONS.request_json[
          :OPTIONS.request_json.rfind('.json')] + '.png'
    else:
      png_output = OPTIONS.request_json + '.png'
  subprocess.check_call(['dot', '-Tpng', tmp.name, '-o', png_output])
  logging.warning('Wrote ' + png_output)
  if OPTIONS.eog:
    subprocess.Popen(['eog', png_output])
  tmp.close()


def DoPrefetchSetup(arg_str):
  OPTIONS.ParseArgs(arg_str, description='Sets up prefetch',
                    extra=['request_json', 'target_html', ('--upload', False)])
  graph_view = _ProcessTraceFile(OPTIONS.request_json)
  with open(OPTIONS.target_html, 'w') as html:
    html.write(_GetPrefetchHtml(
        graph_view, name=os.path.basename(OPTIONS.request_json)))
  if OPTIONS.upload:
    device = device_setup.GetFirstDevice()
    destination = os.path.join('/sdcard/Download',
                               os.path.basename(OPTIONS.target_html))
    device.adb.Push(OPTIONS.target_html, destination)

    logging.warning(
        'Pushed %s to device at %s' % (OPTIONS.target_html, destination))


def DoLogRequests(arg_str):
  OPTIONS.ParseArgs(arg_str, description='Logs requests of a load',
                    extra=['--url', '--output', ('--prefetch', False)])
  _FullFetch(url=OPTIONS.url,
             json_output=OPTIONS.output,
             prefetch=OPTIONS.prefetch)


def DoFetch(arg_str):
  OPTIONS.ParseArgs(arg_str,
                    description=('Fetches SITE into DIR with '
                                 'standard naming that can be processed by '
                                 './cost_to_csv.py.  Both warm and cold '
                                 'fetches are done.  SITE can be a full url '
                                 'but the filename may be strange so better '
                                 'to just use a site (ie, domain).'),
                    extra=['--site', '--dir'])
  if not os.path.exists(OPTIONS.dir):
    os.makedirs(OPTIONS.dir)
  _FullFetch(url=OPTIONS.site,
             json_output=os.path.join(OPTIONS.dir, OPTIONS.site + '.json'),
             prefetch=True)


def DoLongPole(arg_str):
  OPTIONS.ParseArgs(arg_str, description='Calculates long pole',
                    extra='request_json')
  graph_view = _ProcessTraceFile(OPTIONS.request_json)
  path_list = []
  cost = graph_view.deps_graph.Cost(path_list=path_list)
  print '%s (%s)' % (path_list[-1].request.url, cost)


def DoNodeCost(arg_str):
  OPTIONS.ParseArgs(arg_str,
                    description='Calculates node cost',
                    extra='request_json')
  graph_view = _ProcessTraceFile(OPTIONS.request_json)
  print sum((n.cost for n in graph_view.deps_graph.graph.Nodes()))


def DoCost(arg_str):
  OPTIONS.ParseArgs(arg_str,
                    description='Calculates total cost',
                    extra=['request_json', ('--path', False)])
  graph_view = _ProcessTraceFile(OPTIONS.request_json)
  path_list = []
  print 'Graph cost: %s' % graph_view.deps_graph.Cost(path_list=path_list)
  if OPTIONS.path:
    for n in path_list:
      print '  ' + request_track.ShortName(n.request.url)


COMMAND_MAP = {
    'png': DoPng,
    'prefetch_setup': DoPrefetchSetup,
    'log_requests': DoLogRequests,
    'longpole': DoLongPole,
    'nodecost': DoNodeCost,
    'cost': DoCost,
    'fetch': DoFetch,
}

def main():
  logging.basicConfig(level=logging.WARNING)
  OPTIONS.AddGlobalArgument(
      'clear_cache', True, 'clear browser cache before loading')
  OPTIONS.AddGlobalArgument(
      'emulate_device', '',
      'Name of the device to emulate. Must be present '
      'in --devices_file, or empty for no emulation.')
  OPTIONS.AddGlobalArgument('emulate_network', '',
      'Type of network emulation. Empty for no emulation.')
  OPTIONS.AddGlobalArgument(
      'local', False,
      'run against local desktop chrome rather than device '
      '(see also --local_binary and local_profile_dir)')
  OPTIONS.AddGlobalArgument(
      'noads', False, 'ignore ad resources in modeling')
  OPTIONS.AddGlobalArgument(
      'ad_rules', '', 'AdBlocker+ ad rules file.')
  OPTIONS.AddGlobalArgument(
      'tracking_rules', '', 'AdBlocker+ tracking rules file.')
  OPTIONS.AddGlobalArgument(
      'prefetch_delay_seconds', 5,
      'delay after requesting load of prefetch page '
      '(only when running full fetch)')
  OPTIONS.AddGlobalArgument(
      'headless', False, 'Do not display Chrome UI (only works in local mode).')

  parser = argparse.ArgumentParser(description='Analyzes loading')
  parser.add_argument('command', help=' '.join(COMMAND_MAP.keys()))
  parser.add_argument('rest', nargs=argparse.REMAINDER)
  args = parser.parse_args()
  devil_chromium.Initialize()
  COMMAND_MAP.get(args.command,
                  lambda _: InvalidCommand(args.command))(args.rest)


if __name__ == '__main__':
  main()
