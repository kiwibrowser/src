# -*- coding: utf-8 -*-
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Integration tests for cidb.py module."""

from __future__ import print_function

import datetime
import glob
import os
import random
import shutil
import time

from chromite.lib.const import waterfall
from chromite.lib import build_requests
from chromite.lib import constants
from chromite.lib import metadata_lib
from chromite.lib import cidb
from chromite.lib import clactions
from chromite.lib import cros_build_lib
from chromite.lib import cros_logging as logging
from chromite.lib import cros_test_lib
from chromite.lib import hwtest_results
from chromite.lib import osutils
from chromite.lib import parallel
from chromite.lib import patch_unittest


# pylint: disable=protected-access

# Used to ensure that all build_number values we use are unique.
def _random():
  return random.randint(1, 1000000000)


SERIES_0_TEST_DATA_PATH = os.path.join(
    constants.CHROMITE_DIR, 'cidb', 'test_data', 'series_0')

SERIES_1_TEST_DATA_PATH = os.path.join(
    constants.CHROMITE_DIR, 'cidb', 'test_data', 'series_1')


class CIDBIntegrationTest(cros_test_lib.LocalSqlServerTestCase):
  """Base class for cidb tests that connect to a test MySQL instance."""

  CIDB_USER_ROOT = 'root'
  CIDB_USER_BOT = 'bot'
  CIDB_USER_READONLY = 'readonly'

  CIDB_CREDS_DIR = {
      CIDB_USER_BOT: os.path.join(constants.SOURCE_ROOT, 'crostools', 'cidb',
                                  'cidb_test_bot'),
      CIDB_USER_READONLY: os.path.join(constants.SOURCE_ROOT, 'crostools',
                                       'cidb', 'cidb_test_readonly'),
  }

  def LocalCIDBConnection(self, cidb_user):
    """Create a CIDBConnection with the local mysqld instance.

    Args:
      cidb_user: The mysql user to connect as.

    Returns:
      The created CIDBConnection object.
    """
    creds_dir_path = os.path.join(self.tempdir, 'local_cidb_creds')
    osutils.RmDir(creds_dir_path, ignore_missing=True)
    osutils.SafeMakedirs(creds_dir_path)

    osutils.WriteFile(os.path.join(creds_dir_path, 'host.txt'),
                      self.mysqld_host)
    osutils.WriteFile(os.path.join(creds_dir_path, 'port.txt'),
                      str(self.mysqld_port))
    osutils.WriteFile(os.path.join(creds_dir_path, 'user.txt'), cidb_user)

    if cidb_user in self.CIDB_CREDS_DIR:
      shutil.copy(os.path.join(self.CIDB_CREDS_DIR[cidb_user], 'password.txt'),
                  creds_dir_path)

    return cidb.CIDBConnection(
        creds_dir_path,
        query_retry_args=cidb.SqlConnectionRetryArgs(4, 1, 1.1))

  def _PrepareFreshDatabase(self, max_schema_version=None):
    """Create an empty database with migrations applied.

    Args:
      max_schema_version: The highest schema version migration to apply,
      defaults to None in which case all migrations will be applied.

    Returns:
      A CIDBConnection instance, connected to a an empty database as the
      root user.
    """
    # Note: We do not use the cidb.CIDBConnectionFactory
    # in this module. That factory method is used only to construct
    # connections as the bot user, which is how the builders will always
    # connect to the database. In this module, however, we need to test
    # database connections as other mysql users.

    # Connect to database and drop its contents.
    db = self.LocalCIDBConnection(self.CIDB_USER_ROOT)
    db.DropDatabase()

    # Connect to now fresh database and apply migrations.
    db = self.LocalCIDBConnection(self.CIDB_USER_ROOT)
    db.ApplySchemaMigrations(max_schema_version)

    return db

  def _PrepareDatabase(self):
    """Prepares a database at the latest known schema version.

    If database already exists, do not delete existing database. This
    optimization can save a lot of time, when used by tests that do not
    require an empty database.
    """
    # Connect to now fresh database and apply migrations.
    db = self.LocalCIDBConnection(self.CIDB_USER_ROOT)
    db.ApplySchemaMigrations()

    return db

class SchemaDumpTest(CIDBIntegrationTest):
  """Ensure that schema.dump remains current."""

  def mysqlDump(self):
    """Helper to dump the schema.

    Returns:
      CIDB database schema as a single string.
    """
    cmd = [
        'mysqldump',
        '-S', os.path.join(self._mysqld_dir, 'mysqld.socket'),
        '-u', 'root',
        '--no-data',
        '--single-transaction',
        # '--skip-comments',  # Required to avoid dumping a timestamp.
        'cidb',
    ]
    result = cros_build_lib.RunCommand(cmd, capture_output=True, quiet=True)

    # Strip out comment lines, to avoid dumping a problematic timestamp.
    lines = [l for l in result.output.splitlines() if not l.startswith('--')]
    return "\n".join(lines)

  def testDump(self):
    """Ensure generated file is up to date."""
    DUMP_FILE = os.path.join(constants.CHROMITE_DIR, 'cidb', 'schema.dump')

    self._PrepareFreshDatabase()

    # schema.dump
    new_dump = self.mysqlDump()
    old_dump = osutils.ReadFile(DUMP_FILE)

    if new_dump != old_dump:
      if cros_test_lib.GlobalTestConfig.UPDATE_GENERATED_FILES:
        osutils.WriteFile(DUMP_FILE, new_dump)
      else:
        self.fail('schema.dump does not match the '
                  'migrations generated schema. Run '
                  'lib/cidb_integration_test --update')


class CIDBMigrationsTest(CIDBIntegrationTest):
  """Test that all migrations apply correctly."""

  def testMigrations(self):
    """Test that all migrations apply in bulk correctly."""
    self._PrepareFreshDatabase()

  def testIncrementalMigrations(self):
    """Test that all migrations apply incrementally correctly."""
    db = self._PrepareFreshDatabase(0)
    migrations = db._GetMigrationScripts()
    max_version = migrations[-1][0]

    for i in range(1, max_version + 1):
      db.ApplySchemaMigrations(i)

  def testActions(self):
    """Test that InsertCLActions accepts 0-, 1-, and multi-item lists."""
    db = self._PrepareDatabase()
    build_id = db.InsertBuild('my builder', 'chromiumos', _random(),
                              'my config', 'my bot hostname')

    a1 = clactions.CLAction.FromGerritPatchAndAction(
        metadata_lib.GerritPatchTuple(1, 1, True),
        constants.CL_ACTION_PICKED_UP)
    a2 = clactions.CLAction.FromGerritPatchAndAction(
        metadata_lib.GerritPatchTuple(1, 1, True),
        constants.CL_ACTION_PICKED_UP)
    a3 = clactions.CLAction.FromGerritPatchAndAction(
        metadata_lib.GerritPatchTuple(1, 1, True),
        constants.CL_ACTION_PICKED_UP)

    db.InsertCLActions(build_id, [])
    db.InsertCLActions(build_id, [a1])
    db.InsertCLActions(build_id, [a2, a3])

    action_count = db._GetEngine().execute(
        'select count(*) from clActionTable').fetchall()[0][0]
    self.assertEqual(action_count, 3)

    # Test that all known CL action types can be inserted
    fakepatch = metadata_lib.GerritPatchTuple(1, 1, True)
    all_actions_list = [
        clactions.CLAction.FromGerritPatchAndAction(fakepatch, action)
        for action in constants.CL_ACTIONS]
    db.InsertCLActions(build_id, all_actions_list)

  def testActionsWithNoBuildId(self):
    """Test that we can insert a CLAction with no build id."""
    db = self._PrepareDatabase()

    a1 = clactions.CLAction.FromGerritPatchAndAction(
        metadata_lib.GerritPatchTuple(1, 1, True),
        constants.CL_ACTION_PICKED_UP)

    db.InsertCLActions(None, [a1])

    action_count = db._GetEngine().execute(
        'select count(*) from clActionTable').fetchall()[0][0]
    self.assertEqual(action_count, 1)

  def testWaterfallMigration(self):
    """Test that migrating waterfall from enum to varchar preserves value."""
    self.skipTest('Skipped obsolete waterfall migration test.')
    # This test no longer runs. It was used only to confirm the correctness of
    # migration #41. In #43, the InsertBuild API changes in a way that is not
    # compatible with this test.
    # The test code remains in place for demonstration purposes only.
    db = self._PrepareFreshDatabase(40)
    build_id = db.InsertBuild('my builder', 'chromiumos', _random(),
                              'my config', 'my bot hostname')
    db.ApplySchemaMigrations(41)
    self.assertEqual('chromiumos', db.GetBuildStatus(build_id)['waterfall'])


class CIDBAPITest(CIDBIntegrationTest):
  """Tests of the CIDB API."""

  def testSchemaVersionTooLow(self):
    """Tests that the minimum_schema decorator works as expected."""
    db = self._PrepareFreshDatabase(2)
    with self.assertRaises(cidb.UnsupportedMethodException):
      db.InsertCLActions(0, [])

  def testSchemaVersionOK(self):
    """Tests that the minimum_schema decorator works as expected."""
    db = self._PrepareFreshDatabase(4)
    db.InsertCLActions(0, [])

  def testGetTime(self):
    db = self._PrepareFreshDatabase(1)
    current_db_time = db.GetTime()
    self.assertEqual(type(current_db_time), datetime.datetime)

  def testGetBuildStatusKeys(self):
    db = self._PrepareFreshDatabase()
    build_id = db.InsertBuild('builder_name', 'waterfall', 1, 'build_config',
                              'bot_hostname')
    build_status = db.GetBuildStatus(build_id)
    for k in db.BUILD_STATUS_KEYS:
      self.assertIn(k, build_status)

  def testBuildMessages(self):
    db = self._PrepareFreshDatabase(56)
    self.assertEqual([], db.GetBuildMessages(1))
    master_build_id = db.InsertBuild('builder name',
                                     waterfall.WATERFALL_TRYBOT,
                                     1,
                                     'master',
                                     'hostname')
    slave_build_id = db.InsertBuild('slave builder name',
                                    waterfall.WATERFALL_TRYBOT,
                                    2,
                                    'slave',
                                    'slave hostname',
                                    master_build_id=master_build_id)
    db.InsertBuildMessage(master_build_id)
    db.InsertBuildMessage(master_build_id, 'message_type', 'message_subtype',
                          'message_value', 'board')
    for i in range(10):
      db.InsertBuildMessage(slave_build_id,
                            'message_type', 'message_subtype', str(i), 'board')

    master_messages = db.GetBuildMessages(master_build_id)
    master_messages_right_type = db.GetBuildMessages(
        master_build_id, message_type='message_type',
        message_subtype='message_subtype')
    master_messages_wrong_type = db.GetBuildMessages(
        master_build_id, message_type='wrong_message_type',
        message_subtype='wrong_message_subtype')
    slave_messages = db.GetSlaveBuildMessages(master_build_id)

    self.assertEqual(2, len(master_messages))
    self.assertEqual(1, len(master_messages_right_type))
    self.assertEqual(0, len(master_messages_wrong_type))
    self.assertEqual(10, len(slave_messages))

    mm2 = master_messages[1]
    mm2.pop('timestamp')
    self.assertEqual({'build_id': master_build_id,
                      'build_config': 'master',
                      'waterfall': waterfall.WATERFALL_TRYBOT,
                      'builder_name': 'builder name',
                      'build_number': 1L,
                      'message_type': 'message_type',
                      'message_subtype': 'message_subtype',
                      'message_value': 'message_value',
                      'board': 'board'},
                     mm2)
    message_right_type = master_messages_right_type[0]
    message_right_type.pop('timestamp')
    self.assertEqual(message_right_type, mm2)

    sm10 = slave_messages[9]
    sm10.pop('timestamp')
    self.assertEqual({'build_id': slave_build_id,
                      'build_config': 'slave',
                      'waterfall': waterfall.WATERFALL_TRYBOT,
                      'builder_name': 'slave builder name',
                      'build_number': 2L,
                      'message_type': 'message_type',
                      'message_subtype': 'message_subtype',
                      'message_value': '9',
                      'board': 'board'},
                     sm10)

  def testGetKeyVals(self):
    db = self._PrepareFreshDatabase(40)
    # In production we would never insert into this table from a bot, but for
    # testing purposes here this is convenient.
    db._Execute('INSERT INTO keyvalTable(k, v) VALUES '
                '("/foo/bar", "baz"), ("/qux/norf", NULL)')
    self.assertEqual(db.GetKeyVals(), {'/foo/bar': 'baz', '/qux/norf': None})


def GetTestDataSeries(test_data_path):
  """Get metadata from json files at |test_data_path|.

  Returns:
    A list of CBuildbotMetadata objects, sorted by their start time.
  """
  filenames = glob.glob(os.path.join(test_data_path, '*.json'))
  metadatas = []
  for fname in filenames:
    metadatas.append(
        metadata_lib.CBuildbotMetadata.FromJSONString(osutils.ReadFile(fname)))

  # Convert start time values, which are stored in RFC 2822 string format,
  # to seconds since epoch.
  timestamp_from_dict = lambda x: cros_build_lib.ParseUserDateTimeFormat(
      x.GetDict()['time']['start'])

  metadatas.sort(key=timestamp_from_dict)
  return metadatas


class DataSeries0Test(CIDBIntegrationTest):
  """Simulate a set of 630 master/slave CQ builds."""

  def testCQWithSchema56(self):
    """Run the CQ test with schema version 56."""
    db = self._PrepareFreshDatabase(56)
    self._runCQTest(db)

  def _runCQTest(self, db):
    """Simulate a set of 630 master/slave CQ builds.

    Note: This test takes about 2.5 minutes to populate its 630 builds
    and their corresponding cl actions into the test database.
    """
    metadatas = GetTestDataSeries(SERIES_0_TEST_DATA_PATH)
    self.assertEqual(len(metadatas), 630, 'Did not load expected amount of '
                                          'test data')

    bot_db = self.LocalCIDBConnection(self.CIDB_USER_BOT)

    # Simulate the test builds, using a database connection as the
    # bot user.
    self.simulate_builds(bot_db, metadatas)

    # Perform some sanity check queries against the database, connected
    # as the readonly user. Apply schema migrations first to ensure that we can
    # use the latest version or the readonly password.
    db.ApplySchemaMigrations()
    readonly_db = self.LocalCIDBConnection(self.CIDB_USER_READONLY)

    self._start_and_finish_time_checks(readonly_db)

    build_types = readonly_db._GetEngine().execute(
        'select build_type from buildTable').fetchall()
    self.assertTrue(all(x == ('paladin',) for x in build_types))

    self._cl_action_checks(readonly_db)

    build_config_count = readonly_db._GetEngine().execute(
        'select COUNT(distinct build_config) from buildTable').fetchall()[0][0]
    self.assertEqual(build_config_count, 30)

    # Test the _Select method, and verify that the first inserted
    # build is a master-paladin build.
    first_row = readonly_db._Select('buildTable', 1, ['id', 'build_config'])
    self.assertEqual(first_row['build_config'], 'master-paladin')

    # First master build has 29 slaves. Build with id 2 is a slave
    # build with no slaves of its own.
    self.assertEqual(len(readonly_db.GetSlaveStatuses(1)), 29)
    self.assertEqual(len(readonly_db.GetSlaveStatuses(2)), 0)

    # Make sure we can get build status by build id.
    self.assertEqual(readonly_db.GetBuildStatus(2).get('id'), 2)

    # Make sure we can get build statuses by build ids.
    build_dicts = readonly_db.GetBuildStatuses([1, 2])
    self.assertEqual([x.get('id') for x in build_dicts], [1, 2])

    self._start_and_finish_time_checks(readonly_db)
    self._cl_action_checks(readonly_db)
    self._last_updated_time_checks(readonly_db)

    #| Test get build_status from -- here's the relevant data from
    # master-paladin
    #|          id | status |
    #|         601 | pass   |
    #|         571 | pass   |
    #|         541 | fail   |
    #|         511 | pass   |
    #|         481 | pass   |
    # From 1929 because we always go back one build first.
    last_status = readonly_db.GetBuildHistory('master-paladin', 1)
    self.assertEqual(len(last_status), 1)
    last_status = readonly_db.GetBuildHistory('master-paladin', 5)
    self.assertEqual(len(last_status), 5)
    last_status = readonly_db.GetBuildHistory('master-paladin', 5,
                                              milestone_version=52)
    self.assertEqual(len(last_status), 0)
    last_status = readonly_db.GetBuildHistory('master-paladin', 1,
                                              platform_version='6029.0.0-rc1')
    self.assertEqual(len(last_status), 1)
    self.assertEqual(last_status[0]['platform_version'], '6029.0.0-rc1')
    last_status = readonly_db.GetBuildHistory('master-paladin', 1,
                                              platform_version='6029.0.0-rc2')
    self.assertEqual(len(last_status), 1)
    self.assertEqual(last_status[0]['platform_version'], '6029.0.0-rc2')
    last_status = readonly_db.GetBuildHistory('master-paladin', 1,
                                              platform_version='6029.0.0-rc3')
    self.assertEqual(len(last_status), 0)
    last_build = readonly_db.GetMostRecentBuild('chromeos', 'master-paladin')
    self.assertEqual(last_build['id'], 601)
    last_build = readonly_db.GetMostRecentBuild('chromeos', 'master-paladin',
                                                38)
    self.assertEqual(last_build['id'], 601)
    last_build = readonly_db.GetMostRecentBuild('chromeos', 'master-paladin',
                                                39)
    self.assertEqual(last_build, None)
    # Make sure keys are sorted correctly.
    build_ids = []
    for index, status in enumerate(last_status):
      # Add these to list to confirm they are sorted afterwards correctly.
      # Should be descending.
      build_ids.append(status['id'])
      if index == 2:
        self.assertEqual(status['status'], 'fail')
      else:
        self.assertEqual(status['status'], 'pass')

    # Check the sort order.
    self.assertEqual(sorted(build_ids, reverse=True), build_ids)

  def _last_updated_time_checks(self, db):
    """Sanity checks on the last_updated column."""
    # We should have a diversity of last_updated times. Since the timestamp
    # resolution is only 1 second, and we have lots of parallelism in the test,
    # we won't have a distinct last_updated time per row.
    # As the test is now local, almost everything happens together, so we check
    # for a tiny number of distinct timestamps.
    distinct_last_updated = db._GetEngine().execute(
        'select count(distinct last_updated) from buildTable').fetchall()[0][0]
    self.assertTrue(distinct_last_updated > 3)

    ids_by_last_updated = db._GetEngine().execute(
        'select id from buildTable order by last_updated').fetchall()

    ids_by_last_updated = [id_tuple[0] for id_tuple in ids_by_last_updated]

    # Build #1 should have been last updated before build # 200.
    self.assertLess(ids_by_last_updated.index(1),
                    ids_by_last_updated.index(200))

    # However, build #1 (which was a master build) should have been last updated
    # AFTER build #2 which was its slave.
    self.assertGreater(ids_by_last_updated.index(1),
                       ids_by_last_updated.index(2))

  def _cl_action_checks(self, db):
    """Sanity checks that correct cl actions were recorded."""
    submitted_cl_count = db._GetEngine().execute(
        'select count(*) from clActionTable where action="submitted"'
        ).fetchall()[0][0]
    rejected_cl_count = db._GetEngine().execute(
        'select count(*) from clActionTable where action="kicked_out"'
        ).fetchall()[0][0]
    total_actions = db._GetEngine().execute(
        'select count(*) from clActionTable').fetchall()[0][0]
    self.assertEqual(submitted_cl_count, 56)
    self.assertEqual(rejected_cl_count, 8)
    self.assertEqual(total_actions, 1877)

    actions_for_change = db.GetActionsForChanges(
        [metadata_lib.GerritChangeTuple(205535, False)])

    actions_for_build = db.GetActionsForBuild(511)
    self.assertEqual(len(actions_for_build), 22)

    self.assertEqual(len(actions_for_change), 60)
    last_action_dict = dict(actions_for_change[-1]._asdict())
    last_action_dict.pop('timestamp')
    last_action_dict.pop('id')
    self.assertEqual(last_action_dict, {'action': 'submitted',
                                        'build_config': 'master-paladin',
                                        'build_id': 511L,
                                        'change_number': 205535L,
                                        'change_source': 'external',
                                        'patch_number': 1L,
                                        'reason': '',
                                        'buildbucket_id': None,
                                        'status': 'pass'})

  def _start_and_finish_time_checks(self, db):
    """Sanity checks that correct data was recorded, and can be retrieved."""
    max_start_time = db._GetEngine().execute(
        'select max(start_time) from buildTable').fetchall()[0][0]
    min_start_time = db._GetEngine().execute(
        'select min(start_time) from buildTable').fetchall()[0][0]
    max_fin_time = db._GetEngine().execute(
        'select max(finish_time) from buildTable').fetchall()[0][0]
    min_fin_time = db._GetEngine().execute(
        'select min(finish_time) from buildTable').fetchall()[0][0]
    self.assertGreater(max_start_time, min_start_time)
    self.assertGreater(max_fin_time, min_fin_time)

    # For all builds, finish_time should equal last_updated.
    mismatching_times = db._GetEngine().execute(
        'select count(*) from buildTable where finish_time != last_updated'
        ).fetchall()[0][0]
    self.assertEqual(mismatching_times, 0)

  def simulate_builds(self, db, metadatas):
    """Simulate a series of Commit Queue master and slave builds.

    This method use the metadata objects in |metadatas| to simulate those
    builds insertions and updates to the cidb. All metadatas encountered
    after a particular master build will be assumed to be slaves of that build,
    until a new master build is encountered. Slave builds for a particular
    master will be simulated in parallel.

    The first element in |metadatas| must be a CQ master build.

    Args:
      db: A CIDBConnection instance.
      metadatas: A list of CBuildbotMetadata instances, sorted by start time.
    """
    m_iter = iter(metadatas)

    def is_master(m):
      return m.GetDict()['bot-config'] == 'master-paladin'

    next_master = m_iter.next()

    while next_master:
      master = next_master
      next_master = None
      assert is_master(master)
      master_build_id = _SimulateBuildStart(db, master)

      def simulate_slave(slave_metadata):
        build_id = _SimulateBuildStart(db, slave_metadata,
                                       master_build_id,
                                       important=True)
        _SimulateCQBuildFinish(db, slave_metadata, build_id)
        logging.debug('Simulated slave build %s on pid %s', build_id,
                      os.getpid())
        return build_id

      slave_metadatas = []
      for slave in m_iter:
        if is_master(slave):
          next_master = slave
          break
        slave_metadatas.append(slave)

      with parallel.BackgroundTaskRunner(simulate_slave, processes=15) as queue:
        for slave in slave_metadatas:
          queue.put([slave])

      # Yes, this introduces delay in the test. But this lets us do some basic
      # sanity tests on the |last_update| column later.
      time.sleep(1)
      _SimulateCQBuildFinish(db, master, master_build_id)
      logging.debug('Simulated master build %s', master_build_id)


class BuildStagesAndFailureTest(CIDBIntegrationTest):
  """Test buildStageTable functionality."""

  def runTest(self):
    """Test basic buildStageTable and failureTable functionality."""
    self._PrepareDatabase()

    bot_db = self.LocalCIDBConnection(self.CIDB_USER_BOT)

    master_build_id = bot_db.InsertBuild('master build',
                                         waterfall.WATERFALL_INTERNAL,
                                         _random(),
                                         'master_config',
                                         'master.hostname')

    build_id = bot_db.InsertBuild('builder name',
                                  waterfall.WATERFALL_INTERNAL,
                                  _random(),
                                  'build_config',
                                  'bot_hostname',
                                  master_build_id=master_build_id)

    build_stage_id = bot_db.InsertBuildStage(build_id,
                                             'My Stage',
                                             board='bunny')
    values = bot_db.GetBuildStage(build_stage_id)
    self.assertEqual(None, values['start_time'])

    bot_db.WaitBuildStage(build_stage_id)
    values = bot_db.GetBuildStage(build_stage_id)
    self.assertEqual(None, values['start_time'])
    self.assertEqual(constants.BUILDER_STATUS_WAITING, values['status'])

    bot_db.StartBuildStage(build_stage_id)
    values = bot_db.GetBuildStage(build_stage_id)
    self.assertNotEqual(None, values['start_time'])
    self.assertEqual(constants.BUILDER_STATUS_INFLIGHT, values['status'])

    bot_db.FinishBuildStage(build_stage_id, constants.BUILDER_STATUS_PASSED)
    values = bot_db.GetBuildStage(build_stage_id)
    self.assertNotEqual(None, values['finish_time'])
    self.assertEqual(True, values['final'])
    self.assertEqual(constants.BUILDER_STATUS_PASSED, values['status'])

    self.assertFalse(bot_db.HasFailureMsgForStage(build_stage_id))
    for category in constants.EXCEPTION_CATEGORY_ALL_CATEGORIES:
      e = ValueError('The value was erroneous.')
      bot_db.InsertFailure(build_stage_id, type(e).__name__, str(e), category)
      self.assertTrue(bot_db.HasFailureMsgForStage(build_stage_id))

    failures = bot_db.GetSlaveFailures(master_build_id)
    self.assertEqual(len(failures),
                     len(constants.EXCEPTION_CATEGORY_ALL_CATEGORIES))
    for f in failures:
      self.assertEqual(f.build_id, build_id)

    slave_stages = bot_db.GetSlaveStages(master_build_id)
    self.assertEqual(len(slave_stages), 1)
    self.assertEqual(slave_stages[0]['status'], 'pass')
    self.assertEqual(slave_stages[0]['build_config'], 'build_config')
    self.assertEqual(slave_stages[0]['name'], 'My Stage')

  def testGetMasterStages(self):
    """test GetMasterStages."""
    self._PrepareDatabase()

    bot_db = self.LocalCIDBConnection(self.CIDB_USER_BOT)

    master_build_id = bot_db.InsertBuild('master',
                                         waterfall.WATERFALL_INTERNAL,
                                         _random(),
                                         'master',
                                         'master.hostname')

    build_id_1 = bot_db.InsertBuild('slave1',
                                    waterfall.WATERFALL_INTERNAL,
                                    _random(),
                                    'slave1',
                                    'bot_hostname',
                                    master_build_id=master_build_id,
                                    buildbucket_id='bb_id_1')

    bot_db.InsertBuildStage(
        master_build_id, 'master_build_stage_1', board='test1')
    bot_db.InsertBuildStage(
        master_build_id, 'master_build_stage_2', board='test2')
    bot_db.InsertBuildStage(
        build_id_1, 'build_1_stage', board='test1')

    master_stages = bot_db.GetMasterStages(master_build_id)
    self.assertEqual(len(master_stages), 2)

    self.assertEqual(master_stages[0]['status'], 'planned')
    self.assertEqual(master_stages[0]['build_config'], 'master')
    self.assertEqual(master_stages[0]['name'], 'master_build_stage_1')
    self.assertEqual(master_stages[1]['name'], 'master_build_stage_2')

  def testGetSlaveStages(self):
    """test GetSlaveStages."""
    self._PrepareDatabase()

    bot_db = self.LocalCIDBConnection(self.CIDB_USER_BOT)

    master_build_id = bot_db.InsertBuild('master',
                                         waterfall.WATERFALL_INTERNAL,
                                         _random(),
                                         'master',
                                         'master.hostname')

    build_id_1 = bot_db.InsertBuild('slave1',
                                    waterfall.WATERFALL_INTERNAL,
                                    _random(),
                                    'slave1',
                                    'bot_hostname',
                                    master_build_id=master_build_id,
                                    buildbucket_id='bb_id_1')
    build_id_2 = bot_db.InsertBuild('slave2',
                                    waterfall.WATERFALL_INTERNAL,
                                    _random(),
                                    'slave2',
                                    'bot_hostname',
                                    master_build_id=master_build_id,
                                    buildbucket_id='bb_id_2')
    build_id_3 = bot_db.InsertBuild('slave1',
                                    waterfall.WATERFALL_INTERNAL,
                                    _random(),
                                    'slave1',
                                    'bot_hostname',
                                    master_build_id=master_build_id,
                                    buildbucket_id='bb_id_3')

    bot_db.InsertBuildStage(
        build_id_1, 'build_1_stage', board='test1')
    bot_db.InsertBuildStage(
        build_id_2, 'build_2_stage', board='test2')
    bot_db.InsertBuildStage(
        build_id_3, 'build_3_stage', board='test1')

    slave_stages = bot_db.GetSlaveStages(master_build_id)
    self.assertEqual(len(slave_stages), 3)

    buildbucket_ids = ['bb_id_3']
    slave_stages = bot_db.GetSlaveStages(
        master_build_id, buildbucket_ids=buildbucket_ids)
    self.assertEqual(len(slave_stages), 1)
    self.assertEqual(slave_stages[0]['status'], 'planned')
    self.assertEqual(slave_stages[0]['build_config'], 'slave1')
    self.assertEqual(slave_stages[0]['name'], 'build_3_stage')

    slave_stages = bot_db.GetSlaveStages(
        master_build_id, buildbucket_ids=[])
    self.assertEqual(len(slave_stages), 0)

  def testGetSlaveFailures(self):
    """Test GetSlaveFailures and GetBuildFailures"""
    self._PrepareDatabase()

    bot_db = self.LocalCIDBConnection(self.CIDB_USER_BOT)

    master_build_id = bot_db.InsertBuild('master',
                                         waterfall.WATERFALL_INTERNAL,
                                         _random(),
                                         'master',
                                         'master.hostname')

    build_id_1 = bot_db.InsertBuild('slave1',
                                    waterfall.WATERFALL_INTERNAL,
                                    _random(),
                                    'slave1',
                                    'bot_hostname',
                                    master_build_id=master_build_id,
                                    buildbucket_id='bb_id_1')
    build_id_2 = bot_db.InsertBuild('slave1',
                                    waterfall.WATERFALL_INTERNAL,
                                    _random(),
                                    'slave1',
                                    'bot_hostname',
                                    master_build_id=master_build_id,
                                    buildbucket_id='bb_id_2')
    build_stage_id_1 = bot_db.InsertBuildStage(
        build_id_1, 'build_1_stage', board='test1')
    build_stage_id_2 = bot_db.InsertBuildStage(
        build_id_2, 'build_2_stage', board='test1')

    e = ValueError('The value was erroneous.')
    bot_db.InsertFailure(build_stage_id_1, type(e).__name__, str(e),
                         constants.EXCEPTION_CATEGORY_UNKNOWN)
    bot_db.InsertFailure(build_stage_id_2, type(e).__name__, str(e),
                         constants.EXCEPTION_CATEGORY_UNKNOWN)

    failures = bot_db.GetSlaveFailures(master_build_id)
    self.assertEqual(len(failures), 2)

    failures = bot_db.GetSlaveFailures(master_build_id,
                                       buildbucket_ids=[])
    self.assertEqual(len(failures), 0)

    failures = bot_db.GetSlaveFailures(master_build_id,
                                       buildbucket_ids=['bb_id_2'])
    self.assertEqual(len(failures), 1)
    self.assertEqual(failures[0].buildbucket_id, 'bb_id_2')

    failures = bot_db.GetBuildsFailures([build_id_1])
    self.assertEqual(len(failures), 1)
    self.assertEqual(failures[0].build_id, build_id_1)

    failures = bot_db.GetBuildsFailures([build_id_1, build_id_2])
    self.assertEqual(len(failures), 2)


class BuildTableTest(CIDBIntegrationTest):
  """Test buildTable functionality not tested by the DataSeries tests."""

  def testInsertWithDeadline(self):
    """Test deadline setting/querying API."""
    self._PrepareDatabase()
    bot_db = self.LocalCIDBConnection(self.CIDB_USER_BOT)

    build_id = bot_db.InsertBuild('build_name',
                                  waterfall.WATERFALL_INTERNAL,
                                  _random(),
                                  'build_config',
                                  'bot_hostname',
                                  timeout_seconds=30 * 60)
    # This will flake if the few cidb calls above take hours. Unlikely.
    self.assertLess(10, bot_db.GetTimeToDeadline(build_id))

    build_id = bot_db.InsertBuild('build_name',
                                  waterfall.WATERFALL_INTERNAL,
                                  _random(),
                                  'build_config',
                                  'bot_hostname',
                                  timeout_seconds=1)
    # Sleep till the deadline expires.
    time.sleep(3)
    self.assertEqual(0, bot_db.GetTimeToDeadline(build_id))

    build_id = bot_db.InsertBuild('build_name',
                                  waterfall.WATERFALL_INTERNAL,
                                  _random(),
                                  'build_config',
                                  'bot_hostname')
    self.assertEqual(None, bot_db.GetTimeToDeadline(build_id))

    self.assertEqual(None, bot_db.GetTimeToDeadline(build_id))

  def testExtendDeadline(self):
    """Test that a deadline in the future can be extended."""

    self._PrepareDatabase()
    bot_db = self.LocalCIDBConnection(self.CIDB_USER_BOT)

    build_id = bot_db.InsertBuild('build_name',
                                  waterfall.WATERFALL_INTERNAL,
                                  _random(),
                                  'build_config',
                                  'bot_hostname')
    self.assertEqual(None, bot_db.GetTimeToDeadline(build_id))

    self.assertEqual(1, bot_db.ExtendDeadline(build_id, 1))
    time.sleep(2)
    self.assertEqual(0, bot_db.GetTimeToDeadline(build_id))
    self.assertEqual(0, bot_db.ExtendDeadline(build_id, 10 * 60))
    self.assertEqual(0, bot_db.GetTimeToDeadline(build_id))

    build_id = bot_db.InsertBuild('build_name',
                                  waterfall.WATERFALL_INTERNAL,
                                  _random(),
                                  'build_config',
                                  'bot_hostname',
                                  timeout_seconds=30 * 60)
    self.assertLess(10, bot_db.GetTimeToDeadline(build_id))

    self.assertEqual(0, bot_db.ExtendDeadline(build_id, 10 * 60))
    self.assertLess(20 * 60, bot_db.GetTimeToDeadline(build_id))

    self.assertEqual(1, bot_db.ExtendDeadline(build_id, 60 * 60))
    self.assertLess(40 * 60, bot_db.GetTimeToDeadline(build_id))

  def testBuildbucketId(self):
    """Test InsertBuild with buildbucket_id."""
    self._PrepareDatabase()
    bot_db = self.LocalCIDBConnection(self.CIDB_USER_BOT)

    tmp_buildbucket_id = 'tmp_buildbucket_id'
    build_id = bot_db.InsertBuild('build_name',
                                  waterfall.WATERFALL_INTERNAL,
                                  _random(),
                                  'build_config',
                                  'bot_hostname',
                                  buildbucket_id=tmp_buildbucket_id)
    self.assertEqual(tmp_buildbucket_id,
                     bot_db.GetBuildStatus(build_id)['buildbucket_id'])

    build_status = bot_db.GetBuildStatusWithBuildbucketId(
        tmp_buildbucket_id)
    self.assertEqual(build_status['id'], build_id)

  def testFinishBuild(self):
    """Test FinishBuild."""
    self._PrepareDatabase()
    bot_db = self.LocalCIDBConnection(self.CIDB_USER_BOT)

    build_id = bot_db.InsertBuild('build_name',
                                  waterfall.WATERFALL_INTERNAL,
                                  _random(),
                                  'build_config',
                                  'bot_hostname')

    r = bot_db.FinishBuild(build_id, status=constants.BUILDER_STATUS_ABORTED,
                           summary='summary')
    self.assertEqual(r, 1)
    self.assertEqual(bot_db.GetBuildStatus(build_id)['status'],
                     constants.BUILDER_STATUS_ABORTED)
    self.assertEqual(bot_db.GetBuildStatus(build_id)['build_config'],
                     'build_config')
    self.assertEqual(bot_db.GetBuildStatus(build_id)['summary'],
                     'summary')

    # Final status cannot be changed with strict=True
    r = bot_db.FinishBuild(build_id, status=constants.BUILDER_STATUS_PASSED,
                           summary='updated summary')
    self.assertEqual(r, 0)
    self.assertEqual(bot_db.GetBuildStatus(build_id)['status'],
                     constants.BUILDER_STATUS_ABORTED)
    self.assertEqual(bot_db.GetBuildStatus(build_id)['build_config'],
                     'build_config')
    self.assertEqual(bot_db.GetBuildStatus(build_id)['summary'],
                     'summary')

    # Final status can be changed with strict=False
    r = bot_db.FinishBuild(build_id, status=constants.BUILDER_STATUS_PASSED,
                           summary='updated summary', strict=False)
    self.assertEqual(r, 1)
    self.assertEqual(bot_db.GetBuildStatus(build_id)['status'],
                     constants.BUILDER_STATUS_PASSED)
    self.assertEqual(bot_db.GetBuildStatus(build_id)['build_config'],
                     'build_config')
    self.assertEqual(bot_db.GetBuildStatus(build_id)['summary'],
                     'updated summary')

  def _GetBuildToBuildbucketIdList(self, slave_statuses):
    """Convert slave_statuses to a list of (build_config, buildbucket_id)."""
    return [(x['build_config'], x['buildbucket_id']) for x in slave_statuses]

  def testGetSlaveStatus(self):
    """Test GetSlaveStatus."""
    self._PrepareDatabase()
    bot_db = self.LocalCIDBConnection(self.CIDB_USER_BOT)

    build_id = bot_db.InsertBuild('build_name',
                                  waterfall.WATERFALL_INTERNAL,
                                  _random(),
                                  'build_config',
                                  'bot_hostname')

    bot_db.InsertBuild('build_1',
                       waterfall.WATERFALL_INTERNAL,
                       _random(),
                       'build_1',
                       'bot_hostname',
                       master_build_id=build_id,
                       buildbucket_id='id_1')
    bot_db.InsertBuild('build_2',
                       waterfall.WATERFALL_INTERNAL,
                       _random(),
                       'build_2',
                       'bot_hostname',
                       master_build_id=build_id,
                       buildbucket_id='id_2')
    bot_db.InsertBuild('build_1',
                       waterfall.WATERFALL_INTERNAL,
                       _random(),
                       'build_1',
                       'bot_hostname',
                       master_build_id=build_id,
                       buildbucket_id='id_3')

    build_bb_id_list = self._GetBuildToBuildbucketIdList(
        bot_db.GetSlaveStatuses(build_id))
    expected_list = ([
        ('build_1', 'id_1'),
        ('build_2', 'id_2'),
        ('build_1', 'id_3')
    ])
    self.assertListEqual(build_bb_id_list, expected_list)

    build_bb_id_list = self._GetBuildToBuildbucketIdList(
        bot_db.GetSlaveStatuses(build_id, buildbucket_ids=['id_2', 'id_3']))
    expected_list = ([
        ('build_2', 'id_2'),
        ('build_1', 'id_3')
    ])
    self.assertListEqual(build_bb_id_list, expected_list)

    build_bb_id_list = self._GetBuildToBuildbucketIdList(
        bot_db.GetSlaveStatuses(build_id, buildbucket_ids=[]))
    self.assertListEqual(build_bb_id_list, [])

  def _InsertBuildAndUpdateMetadata(self, bot_db, build_config, milestone,
                                    platform):
    build_id = bot_db.InsertBuild(build_config,
                                  waterfall.WATERFALL_INTERNAL,
                                  _random(),
                                  build_config,
                                  'bot_hostname')
    metadata = metadata_lib.CBuildbotMetadata(metadata_dict={
        'version': {
            'milestone': milestone,
            'platform': platform
        }
    })
    return bot_db.UpdateMetadata(build_id, metadata)

  def testGetPlatformVersions(self):
    """Test GetPlatformVersions."""
    self._PrepareDatabase()
    bot_db = self.LocalCIDBConnection(self.CIDB_USER_BOT)
    self._InsertBuildAndUpdateMetadata(
        bot_db, 'master-release', '59', '9352.0.0')
    self._InsertBuildAndUpdateMetadata(
        bot_db, 'master-release', '60', '9462.0.0')
    self._InsertBuildAndUpdateMetadata(
        bot_db, 'master-release', '60', '9475.0.0')
    self._InsertBuildAndUpdateMetadata(
        bot_db, 'master-release', '61', '9623.0.0')

    r = bot_db.GetPlatformVersions('master-paladin')
    self.assertItemsEqual(r, [])

    r = bot_db.GetPlatformVersions('master-release')
    self.assertItemsEqual(r, ['9352.0.0', '9462.0.0', '9475.0.0', '9623.0.0'])

    r = bot_db.GetPlatformVersions('master-release',
                                   starting_milestone_version=60)
    self.assertItemsEqual(r, ['9462.0.0', '9475.0.0', '9623.0.0'])

    r = bot_db.GetPlatformVersions('master-release', num_results=1,
                                   starting_milestone_version=60)
    self.assertItemsEqual(r, ['9462.0.0'])

  def testBuildHistory(self):
    """Test Get operations on build history."""
    self._PrepareDatabase()
    bot_db = self.LocalCIDBConnection(self.CIDB_USER_BOT)
    for _ in range(0, 3):
      bot_db.InsertBuild('build_1',
                         constants.WATERFALL_INTERNAL,
                         _random(),
                         'build_1',
                         'bot_hostname')
      bot_db.InsertBuild('build_2',
                         constants.WATERFALL_INTERNAL,
                         _random(),
                         'build_2',
                         'bot_hostname')
    result = bot_db.GetBuildsHistory(['build_1', 'build_2'], -1)
    self.assertEqual(len(result), 6)

    result = bot_db.GetBuildsHistory(['build_3'], -1)
    self.assertEqual(len(result), 0)

    result = bot_db.GetBuildHistory('build_1', -1)
    self.assertEqual(len(result), 3)

    result = bot_db.GetBuildHistory('build_3', -1)
    self.assertEqual(len(result), 0)


class HWTestResultTableTest(CIDBIntegrationTest):
  """Tests for hwTestResultTable."""

  def testHWTestResults(self):
    """Test Insert and Get operations on hwTestResultTable."""
    HWTestResult = hwtest_results.HWTestResult
    self._PrepareDatabase()
    bot_db = self.LocalCIDBConnection(self.CIDB_USER_BOT)
    b_id_1 = bot_db.InsertBuild('build_name',
                                waterfall.WATERFALL_INTERNAL,
                                _random(),
                                'build_config',
                                'bot_hostname')
    b_id_2 = bot_db.InsertBuild('build_name',
                                waterfall.WATERFALL_INTERNAL,
                                _random(),
                                'build_config',
                                'bot_hostname')

    r1 = HWTestResult.FromReport(b_id_1, 'test_a', 'pass')
    r2 = HWTestResult.FromReport(b_id_1, 'test_b', 'fail')
    r3 = HWTestResult.FromReport(b_id_1, 'test_c', 'abort')
    r4 = HWTestResult.FromReport(b_id_2, 'test_d', 'other')
    bot_db.InsertHWTestResults([r1, r2, r3, r4])

    expected_result_1 = [
        HWTestResult(1, b_id_1, 'test_a', 'pass'),
        HWTestResult(2, b_id_1, 'test_b', 'fail'),
        HWTestResult(3, b_id_1, 'test_c', 'abort')]
    self.assertItemsEqual(bot_db.GetHWTestResultsForBuilds([b_id_1]),
                          expected_result_1)

    expected_result_2 = [HWTestResult(4, b_id_2, 'test_d', 'other')]
    self.assertItemsEqual(bot_db.GetHWTestResultsForBuilds([b_id_2]),
                          expected_result_2)

    expected_result_3 = expected_result_1 + expected_result_2
    self.assertItemsEqual(bot_db.GetHWTestResultsForBuilds([b_id_1, b_id_2]),
                          expected_result_3)

    self.assertItemsEqual(bot_db.GetHWTestResultsForBuilds([3]), [])


class BuildRequestTableTest(CIDBIntegrationTest):
  """Tests for BuildRequestTable."""

  def _InsertCQBuildRequests(self, bot_db):
    b_id = bot_db.InsertBuild(
        'master-paladin', constants.WATERFALL_INTERNAL, _random(),
        'master-paladin', 'bot_hostname')

    build_req_1 = build_requests.BuildRequest(
        id=None, build_id=b_id,
        request_build_config='test1-paladin',
        request_build_args='test_build_args_1',
        request_buildbucket_id='test_bb_id_1',
        request_reason=build_requests.REASON_IMPORTANT_CQ_SLAVE,
        timestamp=None)
    build_req_2 = build_requests.BuildRequest(
        id=None, build_id=b_id,
        request_build_config='test1-paladin',
        request_build_args='test_build_args_2',
        request_buildbucket_id='test_bb_id_2',
        request_reason=build_requests.REASON_IMPORTANT_CQ_SLAVE,
        timestamp=None)
    build_req_3 = build_requests.BuildRequest(
        id=None, build_id=b_id,
        request_build_config='test1-paladin',
        request_build_args='test_build_args_3',
        request_buildbucket_id='test_bb_id_3',
        request_reason=build_requests.REASON_EXPERIMENTAL_CQ_SLAVE,
        timestamp=None)
    bot_db.InsertBuildRequests([build_req_1, build_req_2, build_req_3])

    return b_id

  def _InsertPreCQBuildRequests(self, bot_db, current_ts):
    new_timestamp = current_ts.strftime('%Y-%m-%d %H:%M:%S')
    old_ts = current_ts - datetime.timedelta(hours=2)
    old_timestamp = old_ts.strftime('%Y-%m-%d %H:%M:%S')

    b_id = bot_db.InsertBuild(
        'pre_cq_launcher', constants.WATERFALL_INTERNAL, _random(),
        'pre_cq_launcher', 'bot_hostname')

    build_req_1 = build_requests.BuildRequest(
        id=None, build_id=b_id,
        request_build_config='test_pre_cq_1',
        request_build_args='test_build_args_1',
        request_buildbucket_id='test_bb_id_1',
        request_reason=build_requests.REASON_SANITY_PRE_CQ,
        timestamp=old_timestamp)
    build_req_2 = build_requests.BuildRequest(
        id=None, build_id=b_id,
        request_build_config='test_pre_cq_1',
        request_build_args='test_build_args_2',
        request_buildbucket_id='test_bb_id_2',
        request_reason=build_requests.REASON_SANITY_PRE_CQ,
        timestamp=new_timestamp)
    build_req_3 = build_requests.BuildRequest(
        id=None, build_id=b_id,
        request_build_config='test_pre_cq_2',
        request_build_args='test_build_args_3',
        request_buildbucket_id='test_bb_id_3',
        request_reason=build_requests.REASON_SANITY_PRE_CQ,
        timestamp=new_timestamp)
    bot_db.InsertBuildRequests([build_req_1, build_req_2, build_req_3])

    return b_id

  def testLatestBuildRequestsForReason(self):
    self._PrepareDatabase()
    bot_db = self.LocalCIDBConnection(self.CIDB_USER_BOT)

    now = datetime.datetime.now()

    old_ts = now - datetime.timedelta(days=9)
    self._InsertPreCQBuildRequests(bot_db, old_ts)

    requests = bot_db.GetLatestBuildRequestsForReason(
        build_requests.REASON_SANITY_PRE_CQ)

    self.assertEqual(requests, [])

    three_days_ago = now - datetime.timedelta(days=3)
    self._InsertPreCQBuildRequests(bot_db, three_days_ago)

    requests = bot_db.GetLatestBuildRequestsForReason(
        build_requests.REASON_SANITY_PRE_CQ)

    self.assertEqual(requests[0].request_build_config, 'test_pre_cq_1')
    self.assertEqual(requests[1].request_build_config, 'test_pre_cq_2')
    self.assertEqual(len(requests), 2)

    self._InsertPreCQBuildRequests(bot_db, now)

    requests = bot_db.GetLatestBuildRequestsForReason(
        build_requests.REASON_SANITY_PRE_CQ)

    self.assertEqual(requests[0].request_build_config, 'test_pre_cq_1')
    self.assertEqual(requests[1].request_build_config, 'test_pre_cq_2')
    self.assertEqual(len(requests), 2)

    # It should return the latest request for each build config.
    now = now.replace(microsecond=0)
    self.assertEqual(requests[0].timestamp, now)
    self.assertEqual(requests[1].timestamp, now)

  def testBuildRequests(self):
    """Test insert and get operations on BuildRequestTable."""
    self._PrepareDatabase()
    bot_db = self.LocalCIDBConnection(self.CIDB_USER_BOT)

    bot_db.InsertBuild(
        'pre_cq_launcher', constants.WATERFALL_INTERNAL, _random(),
        'pre_cq_launcher', 'bot_hostname')

    current_ts = datetime.datetime.now()
    start_ts = current_ts - datetime.timedelta(hours=1)
    start_timestamp = start_ts.strftime('%Y-%m-%d %H:%M:%S')

    self._InsertPreCQBuildRequests(bot_db, current_ts)
    self._InsertCQBuildRequests(bot_db)

    requests = bot_db.GetBuildRequestsForBuildConfig(
        'test_pre_cq_1', num_results=1)
    self.assertEqual(len(requests), 1)
    self.assertEqual(requests[0].id, 2)
    self.assertEqual(requests[0].request_build_config, 'test_pre_cq_1')
    self.assertEqual(requests[0].request_buildbucket_id, 'test_bb_id_2')

    requests = bot_db.GetBuildRequestsForBuildConfig('test_pre_cq_1')
    self.assertEqual(len(requests), 2)

    requests = bot_db.GetBuildRequestsForBuildConfig('test_pre_cq_2')
    self.assertEqual(len(requests), 1)
    self.assertEqual(requests[0].id, 3)
    self.assertEqual(requests[0].request_build_config, 'test_pre_cq_2')
    self.assertEqual(requests[0].request_buildbucket_id, 'test_bb_id_3')

    requests = bot_db.GetBuildRequestsForBuildConfigs(
        ['test_pre_cq_1', 'test_pre_cq_2'], num_results=10)
    self.assertItemsEqual([r.request_buildbucket_id for r in requests],
                          ['test_bb_id_1', 'test_bb_id_2', 'test_bb_id_3'])

    requests = bot_db.GetBuildRequestsForBuildConfigs(
        ['test_pre_cq_1', 'test_pre_cq_2'], start_time=start_timestamp,
        num_results=10)
    self.assertItemsEqual([r.request_buildbucket_id for r in requests],
                          ['test_bb_id_2', 'test_bb_id_3'])

    requests = bot_db.GetBuildRequestsForBuildConfigs(
        ['test1-paladin', 'test2-paladin', 'test3-paladin'])
    self.assertItemsEqual([r.request_buildbucket_id for r in requests],
                          ['test_bb_id_1', 'test_bb_id_2', 'test_bb_id_3'])

  def testGetBuildRequestsForRequesterBuild(self):
    """Test GetBuildRequestsForRequesterBuild."""
    self._PrepareDatabase()
    bot_db = self.LocalCIDBConnection(self.CIDB_USER_BOT)

    b_id_1 = bot_db.InsertBuild(
        'master-paladin', constants.WATERFALL_INTERNAL, _random(),
        'master-paladin', 'bot_hostname')

    build_req_1 = build_requests.BuildRequest(
        id=None, build_id=b_id_1,
        request_build_config='test1-paladin',
        request_build_args='test_build_args_1',
        request_buildbucket_id='test_bb_id_1',
        request_reason=build_requests.REASON_IMPORTANT_CQ_SLAVE,
        timestamp=None)
    build_req_2 = build_requests.BuildRequest(
        id=None, build_id=b_id_1,
        request_build_config='test2-paladin',
        request_build_args='test_build_args_2',
        request_buildbucket_id='test_bb_id_2',
        request_reason=build_requests.REASON_IMPORTANT_CQ_SLAVE,
        timestamp=None)
    build_req_3 = build_requests.BuildRequest(
        id=None, build_id=b_id_1,
        request_build_config='test3-paladin',
        request_build_args='test_build_args_3',
        request_buildbucket_id='test_bb_id_3',
        request_reason=build_requests.REASON_EXPERIMENTAL_CQ_SLAVE,
        timestamp=None)
    bot_db.InsertBuildRequests([build_req_1, build_req_2, build_req_3])

    requests = bot_db.GetBuildRequestsForRequesterBuild(b_id_1)
    self.assertItemsEqual([r.request_build_config for r in requests],
                          ['test1-paladin', 'test2-paladin', 'test3-paladin'])

    requests = bot_db.GetBuildRequestsForRequesterBuild(
        b_id_1, request_reason=build_requests.REASON_IMPORTANT_CQ_SLAVE)
    self.assertItemsEqual([r.request_build_config for r in requests],
                          ['test1-paladin', 'test2-paladin'])

    requests = bot_db.GetBuildRequestsForRequesterBuild(
        b_id_1, request_reason=build_requests.REASON_EXPERIMENTAL_CQ_SLAVE)
    self.assertItemsEqual([r.request_build_config for r in requests],
                          ['test3-paladin'])


class CLActionTableTest(CIDBIntegrationTest):
  """Tests for CLActionTable."""

  def setUp(self):
    self._patch_factory = patch_unittest.MockPatchFactory()
    self.changes = self._patch_factory.GetPatches(how_many=3)
    self.actions = [
        clactions.CLAction.FromGerritPatchAndAction(
            self.changes[0], constants.CL_ACTION_PICKED_UP),
        clactions.CLAction.FromGerritPatchAndAction(
            self.changes[1], constants.CL_ACTION_RELEVANT_TO_SLAVE),
        clactions.CLAction.FromGerritPatchAndAction(
            self.changes[2], constants.CL_ACTION_IRRELEVANT_TO_SLAVE)]

    self.build_config_1 = 'build_config_1'
    self.build_config_2 = 'build_config_2'

  def _AssertAction(self, cl_action, build_config, change,
                    status=None, action=None, start_time=None):
    self.assertEqual(cl_action.build_config, build_config)
    self.assertEqual(cl_action.change_number, int(change.gerrit_number))
    self.assertEqual(cl_action.patch_number, int(change.patch_number))
    if status is not None:
      self.assertEqual(cl_action.status, status)
    if action is not None:
      self.assertEqual(cl_action.action, action)
    if start_time is not None:
      self.assertTrue(cl_action.timestamp >= start_time)

  def testGetActionsForChanges(self):
    """Test GetActionsForChanges."""
    self._PrepareDatabase()
    bot_db = self.LocalCIDBConnection(self.CIDB_USER_BOT)

    # b_id_1 in flight status
    b_id_1 = bot_db.InsertBuild(
        self.build_config_1, waterfall.WATERFALL_INTERNAL, _random(),
        self.build_config_1, 'bot_hostname', buildbucket_id='bb_id_1')

    # b_id_2 in pass status
    b_id_2 = bot_db.InsertBuild(
        self.build_config_2, waterfall.WATERFALL_INTERNAL, _random(),
        self.build_config_2, 'bot_hostname', buildbucket_id='bb_id_2')
    bot_db.FinishBuild(b_id_2, status=constants.BUILDER_STATUS_PASSED)


    ts_1 = datetime.datetime.now() - datetime.timedelta(days=3)
    format_ts_1 = ts_1.strftime('%Y-%m-%d %H:%M:%S')
    ts_2 = datetime.datetime.now()
    format_ts_2 = ts_2.strftime('%Y-%m-%d %H:%M:%S')
    bot_db.InsertCLActions(b_id_1, self.actions[0:2], format_ts_1)
    bot_db.InsertCLActions(b_id_1, self.actions[2:3], format_ts_2)
    bot_db.InsertCLActions(b_id_2, self.actions[0:2], format_ts_1)
    bot_db.InsertCLActions(b_id_2, self.actions[2:3], format_ts_2)

    # Test GetActionsForChanges on a single change
    result = bot_db.GetActionsForChanges(self.changes[0:1])
    self.assertEqual(len(result), 2)
    self._AssertAction(result[0], self.build_config_1, self.changes[0])
    self._AssertAction(result[1], self.build_config_2, self.changes[0])

    # Test GetActionsForChanges on multi changes
    result = bot_db.GetActionsForChanges(self.changes[0:2])
    self.assertEqual(len(result), 4)
    self._AssertAction(result[0], self.build_config_1, self.changes[0])
    self._AssertAction(result[1], self.build_config_1, self.changes[1])
    self._AssertAction(result[2], self.build_config_2, self.changes[0])
    self._AssertAction(result[3], self.build_config_2, self.changes[1])

    # Test GetActionsForChanges with ignore_patch_number
    change_1 = self._patch_factory.MockPatch(
        change_id=int(self.changes[0].gerrit_number),
        patch_number=10)
    result = bot_db.GetActionsForChanges([change_1])
    self.assertEqual(len(result), 2)
    result = bot_db.GetActionsForChanges([change_1], ignore_patch_number=False)
    self.assertEqual(len(result), 0)

    # Test GetActionsForChanges with status
    result = bot_db.GetActionsForChanges(
        self.changes[0:1], status=constants.BUILDER_STATUS_PASSED)
    self.assertEqual(len(result), 1)
    self._AssertAction(result[0], self.build_config_2, self.changes[0],
                       status=constants.BUILDER_STATUS_PASSED)

    # Test GetActionsForChanges with action
    result = bot_db.GetActionsForChanges(
        self.changes[0:2], action=constants.CL_ACTION_RELEVANT_TO_SLAVE)
    self.assertEqual(len(result), 2)
    self._AssertAction(result[0], self.build_config_1, self.changes[1],
                       action=constants.CL_ACTION_RELEVANT_TO_SLAVE)
    self._AssertAction(result[1], self.build_config_2, self.changes[1],
                       action=constants.CL_ACTION_RELEVANT_TO_SLAVE)

    # Test GetActionsForChanges with start_time
    now = datetime.datetime.now()
    result = bot_db.GetActionsForChanges(self.changes, start_time=now)
    self.assertEqual(len(result), 0)

    two_days_ago = now - datetime.timedelta(hours=48)
    result = bot_db.GetActionsForChanges(self.changes, start_time=two_days_ago)
    self.assertEqual(len(result), 2)
    self._AssertAction(result[0], self.build_config_1, self.changes[2],
                       start_time=two_days_ago)
    self._AssertAction(result[1], self.build_config_2, self.changes[2],
                       start_time=two_days_ago)

    four_days_ago = now - datetime.timedelta(hours=96)
    result = bot_db.GetActionsForChanges(self.changes, start_time=four_days_ago)
    self.assertEqual(len(result), 6)


class DataSeries1Test(CIDBIntegrationTest):
  """Simulate a single set of canary builds."""

  def runTest(self):
    """Simulate a single set of canary builds with database schema v56."""
    metadatas = GetTestDataSeries(SERIES_1_TEST_DATA_PATH)
    self.assertEqual(len(metadatas), 18, 'Did not load expected amount of '
                                         'test data')

    # Migrate db to specified version. As new schema versions are added,
    # migrations to later version can be applied after the test builds are
    # simulated, to test that db contents are correctly migrated.
    self._PrepareFreshDatabase(56)

    bot_db = self.LocalCIDBConnection(self.CIDB_USER_BOT)

    def is_master(m):
      return m.GetValue('bot-config') == 'master-release'

    master_index = metadatas.index(next(m for m in metadatas if is_master(m)))
    master_metadata = metadatas.pop(master_index)
    self.assertEqual(master_metadata.GetValue('bot-config'), 'master-release')

    master_id = self._simulate_canary(bot_db, master_metadata)

    for m in metadatas:
      self._simulate_canary(bot_db, m, master_id)

    # Verify that expected data was inserted
    num_boards = bot_db._GetEngine().execute(
        'select count(*) from boardPerBuildTable'
        ).fetchall()[0][0]
    self.assertEqual(num_boards, 40)

    main_firmware_versions = bot_db._GetEngine().execute(
        'select count(distinct main_firmware_version) from boardPerBuildTable'
        ).fetchall()[0][0]
    self.assertEqual(main_firmware_versions, 29)

    # For all builds, finish_time should equal last_updated.
    mismatching_times = bot_db._GetEngine().execute(
        'select count(*) from buildTable where finish_time != last_updated'
        ).fetchall()[0][0]
    self.assertEqual(mismatching_times, 0)

  def _simulate_canary(self, db, metadata, master_build_id=None):
    """Helper method to simulate an individual canary build.

    Args:
      db: cidb instance to use for simulation
      metadata: CBuildbotMetadata instance of build to simulate.
      master_build_id: Optional id of master build.

    Returns:
      build_id of build that was simulated.
    """
    build_id = _SimulateBuildStart(db, metadata, master_build_id)
    metadata_dict = metadata.GetDict()

    # Insert child configs and boards
    for child_config_dict in metadata_dict['child-configs']:
      db.InsertChildConfigPerBuild(build_id, child_config_dict['name'])

    for board in metadata_dict['board-metadata'].keys():
      db.InsertBoardPerBuild(build_id, board)

    for board, bm in metadata_dict['board-metadata'].items():
      db.UpdateBoardPerBuildMetadata(build_id, board, bm)

    db.UpdateMetadata(build_id, metadata)

    status = metadata_dict['status']['status']

    for child_config_dict in metadata_dict['child-configs']:
      # Note, we are not using test data here, because the test data
      # we have predates the existence of child-config status being
      # stored in metadata.json. Instead, we just pretend all child
      # configs had the same status as the main config.
      db.FinishChildConfig(build_id, child_config_dict['name'],
                           status)

    db.FinishBuild(build_id, status)

    return build_id


def _SimulateBuildStart(db, metadata, master_build_id=None, important=None):
  """Returns build_id for the inserted buildTable entry."""
  metadata_dict = metadata.GetDict()
  # TODO(akeshet): We are pretending that all these builds were on the internal
  # waterfall at the moment, for testing purposes. This is because we don't
  # actually save in the metadata.json any way to know which waterfall the
  # build was on.
  wfall = 'chromeos'

  build_id = db.InsertBuild(metadata_dict['builder-name'],
                            wfall,
                            metadata_dict['build-number'],
                            metadata_dict['bot-config'],
                            metadata_dict['bot-hostname'],
                            master_build_id,
                            important=important)

  return build_id


def _SimulateCQBuildFinish(db, metadata, build_id):

  metadata_dict = metadata.GetDict()

  db.InsertCLActions(
      build_id,
      [clactions.CLAction.FromMetadataEntry(e)
       for e in metadata_dict['cl_actions']])

  db.UpdateMetadata(build_id, metadata)

  status = metadata_dict['status']['status']
  # The build summary reported by a real CQ run is more complicated -- it is
  # computed from slave summaries by a master. For sanity checking, we just
  # insert the current builer's summary.
  summary = metadata_dict['status'].get('reason', None)

  db.FinishBuild(build_id, status, summary)


def main(_argv):
  # TODO(akeshet): Allow command line args to specify alternate CIDB instance
  # for testing.
  cros_test_lib.main(module=__name__)
