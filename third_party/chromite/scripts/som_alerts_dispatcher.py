# -*- coding: utf-8 -*-
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Script to generate alerts of failing builds to Sheriff-o-Matic."""

from __future__ import print_function

import collections
import datetime
import json
import re

from chromite.cbuildbot import topology
from chromite.lib.const import waterfall
from chromite.lib import cidb
from chromite.lib import classifier
from chromite.lib import commandline
from chromite.lib import constants
from chromite.lib import cros_logging as logging
from chromite.lib import logdog
from chromite.lib import milo
from chromite.lib import parallel
from chromite.lib import prpc
from chromite.lib import som
from chromite.lib import tree_status


# Only display this many links per stage
MAX_STAGE_LINKS = 7

# Max history to look back in days and number of builds.
MAX_HISTORY_DAYS = 30
MAX_CONSECUTIVE_BUILDS = 50
# Last N builds to show as history.
MAX_LAST_N_BUILDS = 10

CROSLAND_VERSION_RE = re.compile(r'^\d+\.\d+\.\d+$')
SANITY_BUILD_CONFIG_RE = re.compile(r'.*-tot-paladin$')

def GetParser():
  """Creates the argparse parser."""
  parser = commandline.ArgumentParser(description=__doc__)
  parser.add_argument('--service_acct_json', type=str, action='store',
                      help='Path to service account credentials JSON file.')
  parser.add_argument('cred_dir', action='store',
                      metavar='CIDB_CREDENTIALS_DIR',
                      help='Database credentials directory with certificates '
                           'and other connection information. Obtain your '
                           'credentials at go/cros-cidb-admin .')
  parser.add_argument('--logdog_host', type=str, action='store',
                      help='URL of Logdog host.')
  parser.add_argument('--milo_host', type=str, action='store',
                      help='URL of MILO host.')
  parser.add_argument('--som_host', type=str, action='store',
                      help='Sheriff-o-Matic host to post alerts to.')
  parser.add_argument('--som_insecure', action='store_true', default=False,
                      help='Use insecure Sheriff-o-Matic connection.')
  parser.add_argument('--som_tree', type=str, action='store',
                      default=constants.SOM_TREE,
                      help='Sheriff-o-Matic tree to post alerts to.')
  parser.add_argument('--output_json', type=str, action='store',
                      help='Filename to write JSON to.')
  parser.add_argument('--json_file', type=str, action='store',
                      help='JSON file to send.')
  parser.add_argument('--allow_experimental', action='store_true',
                      help='Include experimental builds.')
  parser.add_argument('builds', type=str, nargs='*', action='store',
                      metavar='WATERFALL,TREE,SEVERITY|BUILD_ID,SEVERITY',
                      help='Builds to report on.  eg chromeos,elm-release,1000 '
                           'or chromeos,master-paladin,1001 or 1234567,,1000')
  return parser


class ObjectEncoder(json.JSONEncoder):
  """Helper object to encode object dictionaries to JSON."""
  # pylint: disable=method-hidden
  def default(self, obj):
    return obj.__dict__


def MapCIDBToSOMStatus(status, message_type=None, message_subtype=None):
  """Map CIDB status to Sheriff-o-Matic display status.

  In particular, maps inflight stages to being timed out since if they're
  being reported, the master has finished and the inflight stage has not
  completed, likely due to the entire build timing out.

  Args:
    status: A status string from CIDB.
    message_type: A message type string from CIDB.
    message_subtype: A message subtype string from CIDB.

  Returns:
    A string suitable for displaying by Sheriff-o-Matic.
  """
  STATUS_MAP = {
      constants.BUILDER_STATUS_FAILED: 'failed',
      constants.BUILDER_STATUS_INFLIGHT: 'timed out',
  }
  if status in STATUS_MAP:
    status = STATUS_MAP[status]
  if message_type == constants.MESSAGE_TYPE_IGNORED_REASON:
    IGNORED_MAP = {
        constants.MESSAGE_SUBTYPE_SELF_DESTRUCTION: 'self destructed',
    }
    if message_subtype in IGNORED_MAP:
      status = IGNORED_MAP[message_subtype]
  return status


def AddLogsLink(logdog_client, name,
                wfall, logdog_prefix, annotation_stream, logs_links):
  """Helper to add a Logdog link.

  Args:
    logdog_client: logdog.LogdogClient object.
    name: A name for the the link.
    wfall: Waterfall for the Logdog stream
    logdog_prefix: Logdog prefix of the stream.
    annotation_stream: Logdog annotation for the stream.
    logs_links: List to add to if the stream is valid.
  """
  if annotation_stream and annotation_stream['name']:
    url = logdog_client.ConstructViewerURL(wfall,
                                           logdog_prefix,
                                           annotation_stream['name'])
    logs_links.append(som.Link(name, url))


def GenerateAlertStage(build, stage, exceptions, aborted,
                       buildinfo, logdog_client):
  """Generate alert details for a single build stage.

  Args:
    build: Dictionary of build details from CIDB.
    stage: Dictionary of stage details from CIDB.
    exceptions: A list of instances of failure_message_lib.StageFailure.
    aborted: Boolean indicated if the build was aborted.
    buildinfo: BuildInfo build JSON file from MILO.
    logdog_client: logdog.LogdogClient object.

  Returns:
    som.CrosStageFailure object if stage requires alert.  None otherwise.
  """
  STAGE_IGNORE_STATUSES = frozenset([constants.BUILDER_STATUS_PASSED,
                                     constants.BUILDER_STATUS_PLANNED,
                                     constants.BUILDER_STATUS_SKIPPED])
  ABORTED_IGNORE_STATUSES = frozenset([constants.BUILDER_STATUS_INFLIGHT,
                                       constants.BUILDER_STATUS_FORGIVEN,
                                       constants.BUILDER_STATUS_WAITING])
  NO_LOG_RETRY_STATUSES = frozenset([constants.BUILDER_STATUS_INFLIGHT,
                                     constants.BUILDER_STATUS_ABORTED])
  # IGNORE_EXCEPTIONS should be ignored if they're the only exception.
  IGNORE_EXCEPTIONS = frozenset(['ImportantBuilderFailedException'])
  # ABORTED_DISREGARD_EXCEPTIONS should cause any failures of aborted stages
  # to be entirely disregarded.
  ABORTED_DISREGARD_EXCEPTIONS = frozenset(['_ShutDownException'])
  if (stage['build_id'] != build['id'] or
      stage['status'] in STAGE_IGNORE_STATUSES):
    return None
  if aborted and stage['status'] in ABORTED_IGNORE_STATUSES:
    return None

  logging.info('    stage %s (id %d): %s', stage['name'], stage['id'],
               stage['status'])
  logs_links = []
  notes = []

  # Generate links to the logs of the stage and use them for classification.
  if buildinfo and stage['name'] in buildinfo['steps']:
    prefix = buildinfo['annotationStream']['prefix']
    annotation = buildinfo['steps'][stage['name']]
    AddLogsLink(logdog_client, 'stdout', buildinfo['project'],
                prefix, annotation.get('stdoutStream'), logs_links)
    AddLogsLink(logdog_client, 'stderr', buildinfo['project'],
                prefix, annotation.get('stderrStream'), logs_links)

    # Use the logs in an attempt to classify the failure.
    if (annotation.get('stdoutStream') and
        annotation['stdoutStream'].get('name')):
      path = '%s/+/%s' % (prefix, annotation['stdoutStream']['name'])
      try:
        # If either the build or stage is reporting as being inflight,
        # LogDog might still be waiting for logs so don't wait unnecesarily
        # for them.
        retry = (build['status'] not in NO_LOG_RETRY_STATUSES and
                 stage['status'] not in NO_LOG_RETRY_STATUSES)
        logs = logdog_client.GetLines(buildinfo['project'], path,
                                      allow_retries=retry)
        classification = classifier.ClassifyFailure(stage['name'], logs)
        for c in classification or []:
          notes.append('Classification: %s' % (c))
      except Exception as e:
        logging.exception('Could not classify logs: %s', e)
        notes.append('Warning: unable to classify logs: %s' % (e))
  elif aborted:
    # Aborted build with no stage logs is not worth reporting on.
    return None
  else:
    notes.append('Warning: stage logs unavailable')

  # Copy the links from the buildbot build JSON.
  stage_links = []
  if buildinfo:
    if stage['status'] == constants.BUILDER_STATUS_FORGIVEN:
      # TODO: Include these links but hide them by default in frontend.
      pass
    elif stage['name'] in buildinfo['steps']:
      step = buildinfo['steps'][stage['name']]
      stage_links = [som.Link(l['label'], l['url'])
                     for l in step.get('otherLinks', [])]
    else:
      steps = [s for s in buildinfo['steps'].keys()
               if s is not None and not isinstance(s, tuple)]
      logging.warn('Could not find stage %s in: %s',
                   stage['name'], ', '.join(steps))
  else:
    notes.append('Warning: stage details unavailable')

  # Limit the number of links that will be displayed for a single stage.
  # Let there be one extra since it doesn't make sense to have a line
  # saying there is one more.
  # TODO: Move this to frontend so they can be unhidden by clicking.
  if len(stage_links) > MAX_STAGE_LINKS + 1:
    # Insert at the beginning of the notes which come right after the links.
    notes.insert(0, '... and %d more URLs' % (len(stage_links) -
                                              MAX_STAGE_LINKS))
    del stage_links[MAX_STAGE_LINKS:]

  # Add all exceptions recording in CIDB as notes.
  has_other_exceptions = False
  has_ignore_exception = False
  for e in exceptions:
    if e.build_stage_id == stage['id']:
      notes.append('%s: %s' % (e.exception_type, e.exception_message))
      if aborted and e.exception_type in ABORTED_DISREGARD_EXCEPTIONS:
        # Don't generate alert if the exception indicates it should be
        # entirely disregarded.
        return None
      elif e.exception_type in IGNORE_EXCEPTIONS:
        # Ignore this exception (and stage if there aren't other exceptions).
        has_ignore_exception = True
        continue
      has_other_exceptions = True

  # If there is an ignored exception and no other exceptions, treat this
  # stage as non-failed.
  if has_ignore_exception and not has_other_exceptions:
    return None

  # Add the stage to the alert.
  return som.CrosStageFailure(stage['name'],
                              MapCIDBToSOMStatus(stage['status']),
                              logs_links, stage_links, notes)


def GenerateBlameLink(old_version, new_version):
  """Generates a link to show diffs between two versions.

  Args:
    old_version: version string.
    new_version: version string.

  Returns:
    string of URL to generate blame list, or None if it cannot be generated.
  """
  if (CROSLAND_VERSION_RE.match(old_version) and
      CROSLAND_VERSION_RE.match(new_version)):
    return 'http://go/crosland/log/%s..%s' % (old_version, new_version)
  else:
    return None


def GenerateCompareBuildsLink(build_ids, siblings):
  """Return the URL to compare siblings for this build.

  Args:
    build_ids: list of CIDB id for the builds.
    siblings: boolean indicating whether sibling builds should be included.

  Returns:
    The fully formed URL.
  """
  params = ['buildIds=%s' % ','.join([str(b) for b in build_ids])]
  if siblings:
    params.append('includeSiblings=true')
  return 'http://go/buildCompare?%s' % '&'.join(params)


def SummarizeHistory(build, db):
  """Summarizes recent history.

  Args:
    build: Dictionary of build details from CIDB.
    db: cidb.CIDBConnection object.

  Returns:
    list of string describing recent history and diff links.
  """
  # Get all the recent builds of the same type.
  now = datetime.datetime.utcnow()
  start_date = now - datetime.timedelta(days=MAX_HISTORY_DAYS)
  history = db.GetBuildHistory(
      build['build_config'], MAX_CONSECUTIVE_BUILDS, start_date=start_date,
      ending_build_id=build['id'], waterfall=build['waterfall'],
      buildbot_generation=build['buildbot_generation'])
  history = sorted(history, key=lambda s: s['build_number'], reverse=True)

  # Count how many times the current status happened consecutively.
  ids = []
  for h in history:
    if h['status'] != build['status']:
      break
    ids.append(h['id'])

  # Determine histogram of last N builds.
  last_n, frequencies = SummarizeStatuses(history[:MAX_LAST_N_BUILDS])

  # Generate string.
  notes = []
  compare_link = ''
  if build['status'] != constants.BUILDER_STATUS_PASSED and len(ids) > 1:
    compare_link = ' \\[[compare](%s)\\]' % GenerateCompareBuildsLink(ids,
                                                                      False)
  note = 'History of %s: %d %s build(s) in a row%s' % (
      build['builder_name'], len(ids),
      MapCIDBToSOMStatus(build['status']), compare_link)
  if len(frequencies) > 1:
    ids = [h['id'] for h in history[:MAX_LAST_N_BUILDS]]
    note += '; Last %d builds: %s \\[[compare](%s)\\]' % (
        MAX_LAST_N_BUILDS, last_n,
        GenerateCompareBuildsLink(ids, False))
  notes.append(note)

  # Look for transition from most recent passing build.
  if build['status'] != constants.BUILDER_STATUS_PASSED:
    failure = None
    for h in history:
      if h['status'] == constants.BUILDER_STATUS_PASSED:
        # Generate diff link from h .. failure
        blame_link = GenerateBlameLink(h['platform_version'],
                                       failure['platform_version'])
        if blame_link:
          note = ('[Diff](%s) from last passed (%s:%d) to first failure (%s:%d)'
                  % (blame_link, h['builder_name'], h['build_number'],
                     failure['builder_name'], failure['build_number']))
          notes.append(note)
        break
      failure = h

  return notes


def SummarizeStatuses(statuses):
  """Summarizes a list of statuses.

  Args:
    statuses: A list of dictionaries of build details from CIDB.

  Returns:
    (string describing frequency of all the statuses,
     Counter object of status to count)
  """
  frequencies = collections.Counter([s['status'] for s in statuses])
  return ', '.join('%d %s' % (frequencies[h], MapCIDBToSOMStatus(h))
                   for h in frequencies), frequencies


def GenerateBuildAlert(build, stages, exceptions, messages, annotations,
                       siblings, severity, now, db,
                       logdog_client, milo_client, allow_experimental=False):
  """Generate an alert for a single build.

  Args:
    build: Dictionary of build details from CIDB.
    stages: A list of dictionaries of stage details from CIDB.
    exceptions: A list of instances of failure_message_lib.StageFailure.
    messages: A list of build message dictionaries from CIDB.
    annotations: A list of dictionaries of build annotations from CIDB.
    siblings: A list of dictionaries of build details from CIDB.
    severity: Sheriff-o-Matic severity to use for the alert.
    now: Current datettime.
    db: cidb.CIDBConnection object.
    logdog_client: logdog.LogdogClient object.
    milo_client: milo.MiloClient object.
    allow_experimental: Boolean if non-important builds should be included.

  Returns:
    som.Alert object if build requires alert.  None otherwise.
  """
  BUILD_IGNORE_STATUSES = frozenset([constants.BUILDER_STATUS_PASSED])
  CIDB_INDETERMINATE_STATUSES = frozenset([constants.BUILDER_STATUS_INFLIGHT,
                                           constants.BUILDER_STATUS_ABORTED])
  if ((not allow_experimental and not build['important']) or
      build['status'] in BUILD_IGNORE_STATUSES):
    logging.debug('  %s:%d (id %d) skipped important %s status %s',
                  build['builder_name'], build['build_number'], build['id'],
                  build['important'], build['status'])
    return None

  # Record any relevant build messages, keeping track if it was aborted.
  message = (None, None)
  aborted = build['status'] == constants.BUILDER_STATUS_ABORTED
  for m in messages:
    # MESSAGE_TYPE_IGNORED_REASON implies that the target of the message
    # is stored as message_value (as a string).
    if (m['message_type'] == constants.MESSAGE_TYPE_IGNORED_REASON and
        str(build['id']) == m['message_value']):
      if m['message_subtype'] == constants.MESSAGE_SUBTYPE_SELF_DESTRUCTION:
        aborted = True
      message = (m['message_type'], m['message_subtype'])

  logging.info('  %s:%d (id %d) %s %s', build['builder_name'],
               build['build_number'], build['id'], build['status'],
               '%s/%s' % message if message[0] else '')

  # Create links for details on the build.
  dashboard_url = tree_status.ConstructLegolandBuildURL(
      build['buildbucket_id'])
  annotator_url = tree_status.ConstructAnnotatorURL(
      build.get('master_build_id', build['id']))
  links = [
      som.Link('build_details', dashboard_url),
      som.Link('goldeneye',
               tree_status.ConstructGoldenEyeBuildDetailsURL(build['id'])),
      som.Link('viceroy',
               tree_status.ConstructViceroyBuildDetailsURL(build['id'])),
      som.Link('buildbot',
               tree_status.ConstructBuildStageURL(
                   waterfall.WATERFALL_TO_DASHBOARD[build['waterfall']],
                   build['builder_name'], build['build_number'])),
      som.Link('annotator', annotator_url),
  ]

  notes = SummarizeHistory(build, db)
  if len(siblings) > 1:
    notes.append('Siblings: %s \\[[compare](%s)\\]' %
                 (SummarizeStatuses(siblings)[0],
                  GenerateCompareBuildsLink([build['id']], True)))
  # Link to any existing annotations, along with a link back to the annotator.
  notes.extend([('[Annotation](%(link)s): %(failure_category)s'
                 '(%(failure_message)s) %(blame_url)s %(notes)s') %
                dict(a, **{'link': annotator_url}) for a in annotations])

  # If the CIDB status was indeterminate (inflight/aborted), provide link
  # for sheriffs.
  if build['status'] in CIDB_INDETERMINATE_STATUSES:
    notes.append('Indeterminate CIDB status: '
                 'https://yaqs.googleplex.com/eng/q/5238815784697856')

  # Annotate sanity builders as such.
  if SANITY_BUILD_CONFIG_RE.match(build['build_config']):
    notes.append('%s is a sanity builder: '
                 'https://yaqs.googleplex.com/eng/q/5913965810155520' %
                 build['build_config'])

  # TODO: Gather similar failures.
  builders = [som.AlertedBuilder(build['builder_name'], dashboard_url,
                                 ToEpoch(build['finish_time'] or now),
                                 build['build_number'], build['build_number'])]

  # Access the BuildInfo for per-stage links of failed stages.
  try:
    buildinfo = milo_client.BuildInfoGetBuildbot(build['waterfall'],
                                                 build['builder_name'],
                                                 build['build_number'])
  except prpc.PRPCResponseException as e:
    logging.warning('Unable to retrieve BuildInfo: %s', e)
    buildinfo = None

  # Highlight the problematic stages.
  alert_stages = []
  for stage in stages:
    alert_stage = GenerateAlertStage(build, stage, exceptions, aborted,
                                     buildinfo, logdog_client)
    if alert_stage:
      alert_stages.append(alert_stage)

  if (aborted or build['master_build_id'] is None) and len(alert_stages) == 0:
    logging.debug('  %s:%d (id %d) skipped aborted and no stages',
                  build['builder_name'], build['build_number'], build['id'])
    return None

  # Add the alert to the summary.
  key = '%s:%s:%d' % (build['waterfall'], build['build_config'],
                      build['build_number'])
  alert_name = '%s:%d %s' % (build['build_config'], build['build_number'],
                             MapCIDBToSOMStatus(build['status'],
                                                message[0], message[1]))
  return som.Alert(key, alert_name, alert_name, int(severity),
                   ToEpoch(now), ToEpoch(build['finish_time'] or now),
                   links, [], 'cros-failure',
                   som.CrosBuildFailure(notes, alert_stages, builders))


def GenerateAlertsSummary(db, builds=None,
                          logdog_client=None, milo_client=None,
                          allow_experimental=False):
  """Generates the full set of alerts to send to Sheriff-o-Matic.

  Args:
    db: cidb.CIDBConnection object.
    builds: A list of (waterfall, builder_name, severity) tuples to summarize.
      Defaults to SOM_BUILDS[SOM_TREE].
    logdog_client: logdog.LogdogClient object.
    milo_client: milo.MiloClient object.
    allow_experimental: Boolean if non-important builds should be included.

  Returns:
    JSON-marshalled AlertsSummary object.
  """
  if not builds:
    builds = constants.SOM_BUILDS[constants.SOM_TREE]
  if not logdog_client:
    logdog_client = logdog.LogdogClient()
  if not milo_client:
    milo_client = milo.MiloClient()

  funcs = []
  now = datetime.datetime.utcnow()

  # Iterate over relevant masters.
  # build_tuple is either: waterfall, build_config, severity
  #  or: build_id, severity
  for build_tuple in builds:
    # Find the specified build.
    if len(build_tuple) == 2:
      # pylint: disable=unbalanced-tuple-unpacking
      build_id, severity = build_tuple
      # pylint: enable=unbalanced-tuple-unpacking
      master = db.GetBuildStatus(build_id)
      if master is None:
        logging.warn('Could not locate build id %s', build_id)
        continue
      wfall = master['waterfall']
      build_config = master['build_config']
    elif len(build_tuple) == 3:
      wfall, build_config, severity = build_tuple
      master = db.GetMostRecentBuild(wfall, build_config)
      if master is None:
        logging.warn('Could not locate build %s %s', wfall, build_config)
        continue
    else:
      logging.error('Invalid build tuple: %s' % str(build_tuple))
      continue

    statuses = [master]
    stages = db.GetBuildStages(master['id'])
    exceptions = db.GetBuildsFailures([master['id']])
    messages = db.GetBuildMessages(master['id'])
    annotations = []
    logging.info('%s %s (id %d): single/master build, %d stages, %d messages',
                 wfall, build_config, master['id'],
                 len(stages), len(messages))

    # Find any slave builds, and the individual slave stages.
    slave_statuses = db.GetSlaveStatuses(master['id'])
    if len(slave_statuses):
      statuses.extend(slave_statuses)
      slave_stages = db.GetSlaveStages(master['id'])
      stages.extend(slave_stages)
      exceptions.extend(db.GetSlaveFailures(master['id']))
      annotations.extend(db.GetAnnotationsForBuilds(
          [master['id']]).get(master['id'], []))
      logging.info('- %d slaves, %d slave stages, %d annotations',
                   len(slave_statuses), len(slave_stages), len(annotations))

    # Look for failing and inflight (signifying timeouts) slave builds.
    for build in sorted(statuses, key=lambda s: s['builder_name']):
      funcs.append(lambda build_=build, stages_=stages, exceptions_=exceptions,
                          messages_=messages, annotations_=annotations,
                          siblings_=statuses, severity_=severity:
                   GenerateBuildAlert(build_, stages_, exceptions_, messages_,
                                      annotations_, siblings_, severity_,
                                      now, db, logdog_client, milo_client,
                                      allow_experimental=allow_experimental))

  alerts = [alert for alert in parallel.RunParallelSteps(funcs,
                                                         return_values=True)
            if alert]

  revision_summaries = {}
  summary = som.AlertsSummary(alerts, revision_summaries, ToEpoch(now))

  return json.dumps(summary, cls=ObjectEncoder)


def ToEpoch(value):
  """Convert a datetime to number of seconds past epoch."""
  epoch = datetime.datetime(1970, 1, 1)
  return (value - epoch).total_seconds()


def main(argv):
  parser = GetParser()
  options = parser.parse_args(argv)

  # Determine which hosts to connect to.
  db = cidb.CIDBConnection(options.cred_dir)
  topology.FetchTopologyFromCIDB(db)

  if options.json_file:
    # Use the specified alerts.
    logging.info('Using JSON file %s', options.json_file)
    with open(options.json_file) as f:
      summary_json = f.read()
      print(summary_json)
  else:
    builds = [tuple(x.split(',')) for x in options.builds]
    if not builds:
      builds = constants.SOM_BUILDS[options.som_tree]

    # Generate the set of alerts to send.
    logdog_client = logdog.LogdogClient(options.service_acct_json,
                                        host=options.logdog_host)
    milo_client = milo.MiloClient(options.service_acct_json,
                                  host=options.milo_host)
    summary_json = GenerateAlertsSummary(
        db, builds=builds, logdog_client=logdog_client, milo_client=milo_client,
        allow_experimental=options.allow_experimental)
    if options.output_json:
      with open(options.output_json, 'w') as f:
        logging.info('Writing JSON file %s', options.output_json)
        f.write(summary_json)

  # Authenticate and send the alerts.
  som_client = som.SheriffOMaticClient(options.service_acct_json,
                                       insecure=options.som_insecure,
                                       host=options.som_host)
  som_client.SendAlerts(summary_json, tree=options.som_tree)
