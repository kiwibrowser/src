#!/usr/bin/env python
# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests exercising chromiumsync and SyncDataModel."""

import pickle
import unittest

import autofill_specifics_pb2
import bookmark_specifics_pb2
import chromiumsync
import managed_user_specifics_pb2
import sync_pb2
import theme_specifics_pb2

class SyncDataModelTest(unittest.TestCase):
  def setUp(self):
    self.model = chromiumsync.SyncDataModel()
    # The Synced Bookmarks folder is not created by default
    self._expect_synced_bookmarks_folder = False

  def AddToModel(self, proto):
    self.model._entries[proto.id_string] = proto

  def GetChangesFromTimestamp(self, requested_types, timestamp):
    message = sync_pb2.GetUpdatesMessage()
    message.from_timestamp = timestamp
    for data_type in requested_types:
      getattr(message.requested_types,
              chromiumsync.SYNC_TYPE_TO_DESCRIPTOR[
                  data_type].name).SetInParent()
    return self.model.GetChanges(
        chromiumsync.UpdateSieve(message, self.model.migration_history))

  def FindMarkerByNumber(self, markers, datatype):
    """Search a list of progress markers and find the one for a datatype."""
    for marker in markers:
      if marker.data_type_id == datatype.number:
        return marker
    self.fail('Required marker not found: %s' % datatype.name)

  def testPermanentItemSpecs(self):
    specs = chromiumsync.SyncDataModel._PERMANENT_ITEM_SPECS

    declared_specs = set(['0'])
    for spec in specs:
      self.assertTrue(spec.parent_tag in declared_specs, 'parent tags must '
                      'be declared before use')
      declared_specs.add(spec.tag)

    unique_datatypes = set([x.sync_type for x in specs])
    self.assertEqual(unique_datatypes,
                     set(chromiumsync.ALL_TYPES[1:]),
                     'Every sync datatype should have a permanent folder '
                     'associated with it')

  def testSaveEntry(self):
    proto = sync_pb2.SyncEntity()
    proto.id_string = 'abcd'
    proto.version = 0
    self.assertFalse(self.model._ItemExists(proto.id_string))
    self.model._SaveEntry(proto)
    self.assertEqual(1, proto.version)
    self.assertTrue(self.model._ItemExists(proto.id_string))
    self.model._SaveEntry(proto)
    self.assertEqual(2, proto.version)
    proto.version = 0
    self.assertTrue(self.model._ItemExists(proto.id_string))
    self.assertEqual(2, self.model._entries[proto.id_string].version)

  def testCreatePermanentItems(self):
    self.model._CreateDefaultPermanentItems(chromiumsync.ALL_TYPES)
    self.assertEqual(len(chromiumsync.ALL_TYPES) + 1,
                     len(self.model._entries))

  def ExpectedPermanentItemCount(self, sync_type):
    if sync_type == chromiumsync.BOOKMARK:
      if self._expect_synced_bookmarks_folder:
        return 4
      else:
        return 3
    else:
      return 1

  def testGetChangesFromTimestampZeroForEachType(self):
    all_types = chromiumsync.ALL_TYPES[1:]
    for sync_type in all_types:
      self.model = chromiumsync.SyncDataModel()
      request_types = [sync_type]

      version, changes, remaining = (
          self.GetChangesFromTimestamp(request_types, 0))

      expected_count = self.ExpectedPermanentItemCount(sync_type)
      self.assertEqual(expected_count, version)
      self.assertEqual(expected_count, len(changes))
      for change in changes:
        self.assertTrue(change.HasField('server_defined_unique_tag'))
        self.assertEqual(change.version, change.sync_timestamp)
        self.assertTrue(change.version <= version)

      # Test idempotence: another GetUpdates from ts=0 shouldn't recreate.
      version, changes, remaining = (
          self.GetChangesFromTimestamp(request_types, 0))
      self.assertEqual(expected_count, version)
      self.assertEqual(expected_count, len(changes))
      self.assertEqual(0, remaining)

      # Doing a wider GetUpdates from timestamp zero shouldn't recreate either.
      new_version, changes, remaining = (
          self.GetChangesFromTimestamp(all_types, 0))
      if self._expect_synced_bookmarks_folder:
        self.assertEqual(len(chromiumsync.SyncDataModel._PERMANENT_ITEM_SPECS),
                         new_version)
      else:
        self.assertEqual(
            len(chromiumsync.SyncDataModel._PERMANENT_ITEM_SPECS) -1,
            new_version)
      self.assertEqual(new_version, len(changes))
      self.assertEqual(0, remaining)
      version, changes, remaining = (
          self.GetChangesFromTimestamp(request_types, 0))
      self.assertEqual(new_version, version)
      self.assertEqual(expected_count, len(changes))
      self.assertEqual(0, remaining)

  def testBatchSize(self):
    for sync_type in chromiumsync.ALL_TYPES[1:]:
      specifics = chromiumsync.GetDefaultEntitySpecifics(sync_type)
      self.model = chromiumsync.SyncDataModel()
      request_types = [sync_type]

      for i in range(self.model._BATCH_SIZE*3):
        entry = sync_pb2.SyncEntity()
        entry.id_string = 'batch test %d' % i
        entry.specifics.CopyFrom(specifics)
        self.model._SaveEntry(entry)
      last_bit = self.ExpectedPermanentItemCount(sync_type)
      version, changes, changes_remaining = (
          self.GetChangesFromTimestamp(request_types, 0))
      self.assertEqual(self.model._BATCH_SIZE, version)
      self.assertEqual(self.model._BATCH_SIZE*2 + last_bit, changes_remaining)
      version, changes, changes_remaining = (
          self.GetChangesFromTimestamp(request_types, version))
      self.assertEqual(self.model._BATCH_SIZE*2, version)
      self.assertEqual(self.model._BATCH_SIZE + last_bit, changes_remaining)
      version, changes, changes_remaining = (
          self.GetChangesFromTimestamp(request_types, version))
      self.assertEqual(self.model._BATCH_SIZE*3, version)
      self.assertEqual(last_bit, changes_remaining)
      version, changes, changes_remaining = (
          self.GetChangesFromTimestamp(request_types, version))
      self.assertEqual(self.model._BATCH_SIZE*3 + last_bit, version)
      self.assertEqual(0, changes_remaining)

      # Now delete a third of the items.
      for i in xrange(self.model._BATCH_SIZE*3 - 1, 0, -3):
        entry = sync_pb2.SyncEntity()
        entry.id_string = 'batch test %d' % i
        entry.deleted = True
        self.model._SaveEntry(entry)

      # The batch counts shouldn't change.
      version, changes, changes_remaining = (
          self.GetChangesFromTimestamp(request_types, 0))
      self.assertEqual(self.model._BATCH_SIZE, len(changes))
      self.assertEqual(self.model._BATCH_SIZE*2 + last_bit, changes_remaining)
      version, changes, changes_remaining = (
          self.GetChangesFromTimestamp(request_types, version))
      self.assertEqual(self.model._BATCH_SIZE, len(changes))
      self.assertEqual(self.model._BATCH_SIZE + last_bit, changes_remaining)
      version, changes, changes_remaining = (
          self.GetChangesFromTimestamp(request_types, version))
      self.assertEqual(self.model._BATCH_SIZE, len(changes))
      self.assertEqual(last_bit, changes_remaining)
      version, changes, changes_remaining = (
          self.GetChangesFromTimestamp(request_types, version))
      self.assertEqual(last_bit, len(changes))
      self.assertEqual(self.model._BATCH_SIZE*4 + last_bit, version)
      self.assertEqual(0, changes_remaining)

  def testCommitEachDataType(self):
    for sync_type in chromiumsync.ALL_TYPES[1:]:
      specifics = chromiumsync.GetDefaultEntitySpecifics(sync_type)
      self.model = chromiumsync.SyncDataModel()
      my_cache_guid = '112358132134'
      parent = 'foobar'
      commit_session = {}

      # Start with a GetUpdates from timestamp 0, to populate permanent items.
      original_version, original_changes, changes_remaining = (
          self.GetChangesFromTimestamp([sync_type], 0))

      def DoCommit(original=None, id_string='', name=None, parent=None,
                   position=0):
        proto = sync_pb2.SyncEntity()
        if original is not None:
          proto.version = original.version
          proto.id_string = original.id_string
          proto.parent_id_string = original.parent_id_string
          proto.name = original.name
        else:
          proto.id_string = id_string
          proto.version = 0
        proto.specifics.CopyFrom(specifics)
        if name is not None:
          proto.name = name
        if parent:
          proto.parent_id_string = parent.id_string
        proto.insert_after_item_id = 'please discard'
        proto.position_in_parent = position
        proto.folder = True
        proto.deleted = False
        result = self.model.CommitEntry(proto, my_cache_guid, commit_session)
        self.assertTrue(result)
        return (proto, result)

      # Commit a new item.
      proto1, result1 = DoCommit(name='namae', id_string='Foo',
                                 parent=original_changes[-1], position=100)
      # Commit an item whose parent is another item (referenced via the
      # pre-commit ID).
      proto2, result2 = DoCommit(name='Secondo', id_string='Bar',
                                 parent=proto1, position=-100)
        # Commit a sibling of the second item.
      proto3, result3 = DoCommit(name='Third!', id_string='Baz',
                                 parent=proto1, position=-50)

      self.assertEqual(3, len(commit_session))
      for p, r in [(proto1, result1), (proto2, result2), (proto3, result3)]:
        self.assertNotEqual(r.id_string, p.id_string)
        self.assertEqual(r.originator_client_item_id, p.id_string)
        self.assertEqual(r.originator_cache_guid, my_cache_guid)
        self.assertTrue(r is not self.model._entries[r.id_string],
                        "Commit result didn't make a defensive copy.")
        self.assertTrue(p is not self.model._entries[r.id_string],
                        "Commit result didn't make a defensive copy.")
        self.assertEqual(commit_session.get(p.id_string), r.id_string)
        self.assertTrue(r.version > original_version)
      self.assertEqual(result1.parent_id_string, proto1.parent_id_string)
      self.assertEqual(result2.parent_id_string, result1.id_string)
      version, changes, remaining = (
          self.GetChangesFromTimestamp([sync_type], original_version))
      self.assertEqual(3, len(changes))
      self.assertEqual(0, remaining)
      self.assertEqual(original_version + 3, version)
      self.assertEqual([result1, result2, result3], changes)
      for c in changes:
        self.assertTrue(c is not self.model._entries[c.id_string],
                        "GetChanges didn't make a defensive copy.")
      self.assertTrue(result2.position_in_parent < result3.position_in_parent)
      self.assertEqual(-100, result2.position_in_parent)

      # Now update the items so that the second item is the parent of the
      # first; with the first sandwiched between two new items (4 and 5).
      # Do this in a new commit session, meaning we'll reference items from
      # the first batch by their post-commit, server IDs.
      commit_session = {}
      old_cache_guid = my_cache_guid
      my_cache_guid = 'A different GUID'
      proto2b, result2b = DoCommit(original=result2,
                                   parent=original_changes[-1])
      proto4, result4 = DoCommit(id_string='ID4', name='Four',
                                 parent=result2, position=-200)
      proto1b, result1b = DoCommit(original=result1,
                                   parent=result2, position=-150)
      proto5, result5 = DoCommit(id_string='ID5', name='Five', parent=result2,
                                 position=150)

      self.assertEqual(2, len(commit_session), 'Only new items in second '
                       'batch should be in the session')
      for p, r, original in [(proto2b, result2b, proto2),
                             (proto4, result4, proto4),
                             (proto1b, result1b, proto1),
                             (proto5, result5, proto5)]:
        self.assertEqual(r.originator_client_item_id, original.id_string)
        if original is not p:
          self.assertEqual(r.id_string, p.id_string,
                           'Ids should be stable after first commit')
          self.assertEqual(r.originator_cache_guid, old_cache_guid)
        else:
          self.assertNotEqual(r.id_string, p.id_string)
          self.assertEqual(r.originator_cache_guid, my_cache_guid)
          self.assertEqual(commit_session.get(p.id_string), r.id_string)
        self.assertTrue(r is not self.model._entries[r.id_string],
                        "Commit result didn't make a defensive copy.")
        self.assertTrue(p is not self.model._entries[r.id_string],
                        "Commit didn't make a defensive copy.")
        self.assertTrue(r.version > p.version)
      version, changes, remaining = (
          self.GetChangesFromTimestamp([sync_type], original_version))
      self.assertEqual(5, len(changes))
      self.assertEqual(0, remaining)
      self.assertEqual(original_version + 7, version)
      self.assertEqual([result3, result2b, result4, result1b, result5], changes)
      for c in changes:
        self.assertTrue(c is not self.model._entries[c.id_string],
                        "GetChanges didn't make a defensive copy.")
      self.assertTrue(result4.parent_id_string ==
                      result1b.parent_id_string ==
                      result5.parent_id_string ==
                      result2b.id_string)
      self.assertTrue(result4.position_in_parent <
                      result1b.position_in_parent <
                      result5.position_in_parent)

  def testUpdateSieve(self):
    # from_timestamp, legacy mode
    autofill = chromiumsync.SYNC_TYPE_FIELDS['autofill']
    theme = chromiumsync.SYNC_TYPE_FIELDS['theme']
    msg = sync_pb2.GetUpdatesMessage()
    msg.from_timestamp = 15412
    msg.requested_types.autofill.SetInParent()
    msg.requested_types.theme.SetInParent()

    sieve = chromiumsync.UpdateSieve(msg)
    self.assertEqual(sieve._state,
        {chromiumsync.TOP_LEVEL: 15412,
         chromiumsync.AUTOFILL: 15412,
         chromiumsync.THEME: 15412})

    response = sync_pb2.GetUpdatesResponse()
    sieve.SaveProgress(15412, response)
    self.assertEqual(0, len(response.new_progress_marker))
    self.assertFalse(response.HasField('new_timestamp'))

    response = sync_pb2.GetUpdatesResponse()
    sieve.SaveProgress(15413, response)
    self.assertEqual(0, len(response.new_progress_marker))
    self.assertTrue(response.HasField('new_timestamp'))
    self.assertEqual(15413, response.new_timestamp)

    # Existing tokens
    msg = sync_pb2.GetUpdatesMessage()
    marker = msg.from_progress_marker.add()
    marker.data_type_id = autofill.number
    marker.token = pickle.dumps((15412, 1))
    marker = msg.from_progress_marker.add()
    marker.data_type_id = theme.number
    marker.token = pickle.dumps((15413, 1))
    sieve = chromiumsync.UpdateSieve(msg)
    self.assertEqual(sieve._state,
        {chromiumsync.TOP_LEVEL: 15412,
         chromiumsync.AUTOFILL: 15412,
         chromiumsync.THEME: 15413})

    response = sync_pb2.GetUpdatesResponse()
    sieve.SaveProgress(15413, response)
    self.assertEqual(1, len(response.new_progress_marker))
    self.assertFalse(response.HasField('new_timestamp'))
    marker = response.new_progress_marker[0]
    self.assertEqual(marker.data_type_id, autofill.number)
    self.assertEqual(pickle.loads(marker.token), (15413, 1))
    self.assertFalse(marker.HasField('timestamp_token_for_migration'))

    # Empty tokens indicating from timestamp = 0
    msg = sync_pb2.GetUpdatesMessage()
    marker = msg.from_progress_marker.add()
    marker.data_type_id = autofill.number
    marker.token = pickle.dumps((412, 1))
    marker = msg.from_progress_marker.add()
    marker.data_type_id = theme.number
    marker.token = ''
    sieve = chromiumsync.UpdateSieve(msg)
    self.assertEqual(sieve._state,
        {chromiumsync.TOP_LEVEL: 0,
         chromiumsync.AUTOFILL: 412,
         chromiumsync.THEME: 0})
    response = sync_pb2.GetUpdatesResponse()
    sieve.SaveProgress(1, response)
    self.assertEqual(1, len(response.new_progress_marker))
    self.assertFalse(response.HasField('new_timestamp'))
    marker = response.new_progress_marker[0]
    self.assertEqual(marker.data_type_id, theme.number)
    self.assertEqual(pickle.loads(marker.token), (1, 1))
    self.assertFalse(marker.HasField('timestamp_token_for_migration'))

    response = sync_pb2.GetUpdatesResponse()
    sieve.SaveProgress(412, response)
    self.assertEqual(1, len(response.new_progress_marker))
    self.assertFalse(response.HasField('new_timestamp'))
    marker = response.new_progress_marker[0]
    self.assertEqual(marker.data_type_id, theme.number)
    self.assertEqual(pickle.loads(marker.token), (412, 1))
    self.assertFalse(marker.HasField('timestamp_token_for_migration'))

    response = sync_pb2.GetUpdatesResponse()
    sieve.SaveProgress(413, response)
    self.assertEqual(2, len(response.new_progress_marker))
    self.assertFalse(response.HasField('new_timestamp'))
    marker = self.FindMarkerByNumber(response.new_progress_marker, theme)
    self.assertEqual(pickle.loads(marker.token), (413, 1))
    self.assertFalse(marker.HasField('timestamp_token_for_migration'))
    marker = self.FindMarkerByNumber(response.new_progress_marker, autofill)
    self.assertEqual(pickle.loads(marker.token), (413, 1))
    self.assertFalse(marker.HasField('timestamp_token_for_migration'))

    # Migration token timestamps (client gives timestamp, server returns token)
    # These are for migrating from the old 'timestamp' protocol to the
    # progressmarker protocol, and have nothing to do with the MIGRATION_DONE
    # error code.
    msg = sync_pb2.GetUpdatesMessage()
    marker = msg.from_progress_marker.add()
    marker.data_type_id = autofill.number
    marker.timestamp_token_for_migration = 15213
    marker = msg.from_progress_marker.add()
    marker.data_type_id = theme.number
    marker.timestamp_token_for_migration = 15211
    sieve = chromiumsync.UpdateSieve(msg)
    self.assertEqual(sieve._state,
        {chromiumsync.TOP_LEVEL: 15211,
         chromiumsync.AUTOFILL: 15213,
         chromiumsync.THEME: 15211})
    response = sync_pb2.GetUpdatesResponse()
    sieve.SaveProgress(16000, response)  # There were updates
    self.assertEqual(2, len(response.new_progress_marker))
    self.assertFalse(response.HasField('new_timestamp'))
    marker = self.FindMarkerByNumber(response.new_progress_marker, theme)
    self.assertEqual(pickle.loads(marker.token), (16000, 1))
    self.assertFalse(marker.HasField('timestamp_token_for_migration'))
    marker = self.FindMarkerByNumber(response.new_progress_marker, autofill)
    self.assertEqual(pickle.loads(marker.token), (16000, 1))
    self.assertFalse(marker.HasField('timestamp_token_for_migration'))

    msg = sync_pb2.GetUpdatesMessage()
    marker = msg.from_progress_marker.add()
    marker.data_type_id = autofill.number
    marker.timestamp_token_for_migration = 3000
    marker = msg.from_progress_marker.add()
    marker.data_type_id = theme.number
    marker.timestamp_token_for_migration = 3000
    sieve = chromiumsync.UpdateSieve(msg)
    self.assertEqual(sieve._state,
        {chromiumsync.TOP_LEVEL: 3000,
         chromiumsync.AUTOFILL: 3000,
         chromiumsync.THEME: 3000})
    response = sync_pb2.GetUpdatesResponse()
    sieve.SaveProgress(3000, response)  # Already up to date
    self.assertEqual(2, len(response.new_progress_marker))
    self.assertFalse(response.HasField('new_timestamp'))
    marker = self.FindMarkerByNumber(response.new_progress_marker, theme)
    self.assertEqual(pickle.loads(marker.token), (3000, 1))
    self.assertFalse(marker.HasField('timestamp_token_for_migration'))
    marker = self.FindMarkerByNumber(response.new_progress_marker, autofill)
    self.assertEqual(pickle.loads(marker.token), (3000, 1))
    self.assertFalse(marker.HasField('timestamp_token_for_migration'))

  def testCheckRaiseTransientError(self):
    testserver = chromiumsync.TestServer()
    http_code, raw_respon = testserver.HandleSetTransientError()
    self.assertEqual(http_code, 200)
    try:
      testserver.CheckTransientError()
      self.fail('Should have raised transient error exception')
    except chromiumsync.TransientError:
      self.assertTrue(testserver.transient_error)

  def testUpdateSieveStoreMigration(self):
    autofill = chromiumsync.SYNC_TYPE_FIELDS['autofill']
    theme = chromiumsync.SYNC_TYPE_FIELDS['theme']
    migrator = chromiumsync.MigrationHistory()
    msg = sync_pb2.GetUpdatesMessage()
    marker = msg.from_progress_marker.add()
    marker.data_type_id = autofill.number
    marker.token = pickle.dumps((15412, 1))
    marker = msg.from_progress_marker.add()
    marker.data_type_id = theme.number
    marker.token = pickle.dumps((15413, 1))
    sieve = chromiumsync.UpdateSieve(msg, migrator)
    sieve.CheckMigrationState()

    migrator.Bump([chromiumsync.BOOKMARK, chromiumsync.PASSWORD])  # v=2
    sieve = chromiumsync.UpdateSieve(msg, migrator)
    sieve.CheckMigrationState()
    self.assertEqual(sieve._state,
        {chromiumsync.TOP_LEVEL: 15412,
         chromiumsync.AUTOFILL: 15412,
         chromiumsync.THEME: 15413})

    migrator.Bump([chromiumsync.AUTOFILL, chromiumsync.PASSWORD])  # v=3
    sieve = chromiumsync.UpdateSieve(msg, migrator)
    try:
      sieve.CheckMigrationState()
      self.fail('Should have raised.')
    except chromiumsync.MigrationDoneError, error:
      # We want this to happen.
      self.assertEqual([chromiumsync.AUTOFILL], error.datatypes)

    msg = sync_pb2.GetUpdatesMessage()
    marker = msg.from_progress_marker.add()
    marker.data_type_id = autofill.number
    marker.token = ''
    marker = msg.from_progress_marker.add()
    marker.data_type_id = theme.number
    marker.token = pickle.dumps((15413, 1))
    sieve = chromiumsync.UpdateSieve(msg, migrator)
    sieve.CheckMigrationState()
    response = sync_pb2.GetUpdatesResponse()
    sieve.SaveProgress(15412, response)  # There were updates
    self.assertEqual(1, len(response.new_progress_marker))
    self.assertFalse(response.HasField('new_timestamp'))
    self.assertFalse(marker.HasField('timestamp_token_for_migration'))
    marker = self.FindMarkerByNumber(response.new_progress_marker, autofill)
    self.assertEqual(pickle.loads(marker.token), (15412, 3))
    self.assertFalse(marker.HasField('timestamp_token_for_migration'))
    msg = sync_pb2.GetUpdatesMessage()
    marker = msg.from_progress_marker.add()
    marker.data_type_id = autofill.number
    marker.token = pickle.dumps((15412, 3))
    marker = msg.from_progress_marker.add()
    marker.data_type_id = theme.number
    marker.token = pickle.dumps((15413, 1))
    sieve = chromiumsync.UpdateSieve(msg, migrator)
    sieve.CheckMigrationState()

    migrator.Bump([chromiumsync.THEME, chromiumsync.AUTOFILL])  # v=4
    migrator.Bump([chromiumsync.AUTOFILL])                      # v=5
    sieve = chromiumsync.UpdateSieve(msg, migrator)
    try:
      sieve.CheckMigrationState()
      self.fail("Should have raised.")
    except chromiumsync.MigrationDoneError, error:
      # We want this to happen.
      self.assertEqual(set([chromiumsync.THEME, chromiumsync.AUTOFILL]),
                       set(error.datatypes))
    msg = sync_pb2.GetUpdatesMessage()
    marker = msg.from_progress_marker.add()
    marker.data_type_id = autofill.number
    marker.token = ''
    marker = msg.from_progress_marker.add()
    marker.data_type_id = theme.number
    marker.token = pickle.dumps((15413, 1))
    sieve = chromiumsync.UpdateSieve(msg, migrator)
    try:
      sieve.CheckMigrationState()
      self.fail("Should have raised.")
    except chromiumsync.MigrationDoneError, error:
      # We want this to happen.
      self.assertEqual([chromiumsync.THEME], error.datatypes)

    msg = sync_pb2.GetUpdatesMessage()
    marker = msg.from_progress_marker.add()
    marker.data_type_id = autofill.number
    marker.token = ''
    marker = msg.from_progress_marker.add()
    marker.data_type_id = theme.number
    marker.token = ''
    sieve = chromiumsync.UpdateSieve(msg, migrator)
    sieve.CheckMigrationState()
    response = sync_pb2.GetUpdatesResponse()
    sieve.SaveProgress(15412, response)  # There were updates
    self.assertEqual(2, len(response.new_progress_marker))
    self.assertFalse(response.HasField('new_timestamp'))
    self.assertFalse(marker.HasField('timestamp_token_for_migration'))
    marker = self.FindMarkerByNumber(response.new_progress_marker, autofill)
    self.assertEqual(pickle.loads(marker.token), (15412, 5))
    self.assertFalse(marker.HasField('timestamp_token_for_migration'))
    marker = self.FindMarkerByNumber(response.new_progress_marker, theme)
    self.assertEqual(pickle.loads(marker.token), (15412, 4))
    self.assertFalse(marker.HasField('timestamp_token_for_migration'))
    msg = sync_pb2.GetUpdatesMessage()
    marker = msg.from_progress_marker.add()
    marker.data_type_id = autofill.number
    marker.token = pickle.dumps((15412, 5))
    marker = msg.from_progress_marker.add()
    marker.data_type_id = theme.number
    marker.token = pickle.dumps((15413, 4))
    sieve = chromiumsync.UpdateSieve(msg, migrator)
    sieve.CheckMigrationState()

  def testCreateSyncedBookmarks(self):
    version1, changes, remaining = (
        self.GetChangesFromTimestamp([chromiumsync.BOOKMARK], 0))
    id_string = self.model._MakeCurrentId(chromiumsync.BOOKMARK,
                                          '<server tag>synced_bookmarks')
    self.assertFalse(self.model._ItemExists(id_string))
    self._expect_synced_bookmarks_folder = True
    self.model.TriggerCreateSyncedBookmarks()
    self.assertTrue(self.model._ItemExists(id_string))

    # Check that the version changed when the folder was created and the only
    # change was the folder creation.
    version2, changes, remaining = (
        self.GetChangesFromTimestamp([chromiumsync.BOOKMARK], version1))
    self.assertEqual(len(changes), 1)
    self.assertEqual(changes[0].id_string, id_string)
    self.assertNotEqual(version1, version2)
    self.assertEqual(
        self.ExpectedPermanentItemCount(chromiumsync.BOOKMARK),
        version2)

    # Ensure getting from timestamp 0 includes the folder.
    version, changes, remaining = (
        self.GetChangesFromTimestamp([chromiumsync.BOOKMARK], 0))
    self.assertEqual(
        self.ExpectedPermanentItemCount(chromiumsync.BOOKMARK),
        len(changes))
    self.assertEqual(version2, version)

  def testAcknowledgeManagedUser(self):
    # Create permanent items.
    self.GetChangesFromTimestamp([chromiumsync.MANAGED_USER], 0)
    proto = sync_pb2.SyncEntity()
    proto.id_string = 'abcd'
    proto.version = 0

    # Make sure the managed_user field exists.
    proto.specifics.managed_user.acknowledged = False
    self.assertTrue(proto.specifics.HasField('managed_user'))
    self.AddToModel(proto)
    version1, changes1, remaining1 = (
        self.GetChangesFromTimestamp([chromiumsync.MANAGED_USER], 0))
    for change in changes1:
      self.assertTrue(not change.specifics.managed_user.acknowledged)

    # Turn on managed user acknowledgement
    self.model.acknowledge_managed_users = True

    version2, changes2, remaining2 = (
        self.GetChangesFromTimestamp([chromiumsync.MANAGED_USER], 0))
    for change in changes2:
      self.assertTrue(change.specifics.managed_user.acknowledged)

  def testGetKey(self):
    [key1] = self.model.GetKeystoreKeys()
    [key2] = self.model.GetKeystoreKeys()
    self.assertTrue(len(key1))
    self.assertEqual(key1, key2)

    # Trigger the rotation. A subsequent GetUpdates should return the nigori
    # node (whose timestamp was bumped by the rotation).
    version1, changes, remaining = (
        self.GetChangesFromTimestamp([chromiumsync.NIGORI], 0))
    self.model.TriggerRotateKeystoreKeys()
    version2, changes, remaining = (
        self.GetChangesFromTimestamp([chromiumsync.NIGORI], version1))
    self.assertNotEqual(version1, version2)
    self.assertEquals(len(changes), 1)
    self.assertEquals(changes[0].name, "Nigori")

    # The current keys should contain the old keys, with the new key appended.
    [key1, key3] = self.model.GetKeystoreKeys()
    self.assertEquals(key1, key2)
    self.assertNotEqual(key1, key3)
    self.assertTrue(len(key3) > 0)

  def testTriggerEnableKeystoreEncryption(self):
    version1, changes, remaining = (
        self.GetChangesFromTimestamp([chromiumsync.EXPERIMENTS], 0))
    keystore_encryption_id_string = (
        self.model._ClientTagToId(
            chromiumsync.EXPERIMENTS,
            chromiumsync.KEYSTORE_ENCRYPTION_EXPERIMENT_TAG))

    self.assertFalse(self.model._ItemExists(keystore_encryption_id_string))
    self.model.TriggerEnableKeystoreEncryption()
    self.assertTrue(self.model._ItemExists(keystore_encryption_id_string))

    # The creation of the experiment should be downloaded on the next
    # GetUpdates.
    version2, changes, remaining = (
        self.GetChangesFromTimestamp([chromiumsync.EXPERIMENTS], version1))
    self.assertEqual(len(changes), 1)
    self.assertEqual(changes[0].id_string, keystore_encryption_id_string)
    self.assertNotEqual(version1, version2)

    # Verify the experiment was created properly and is enabled.
    self.assertEqual(chromiumsync.KEYSTORE_ENCRYPTION_EXPERIMENT_TAG,
                     changes[0].client_defined_unique_tag)
    self.assertTrue(changes[0].HasField("specifics"))
    self.assertTrue(changes[0].specifics.HasField("experiments"))
    self.assertTrue(
        changes[0].specifics.experiments.HasField("keystore_encryption"))
    self.assertTrue(
        changes[0].specifics.experiments.keystore_encryption.enabled)

if __name__ == '__main__':
  unittest.main()
