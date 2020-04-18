# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unittests for cidb."""

from __future__ import print_function

import exceptions
import mock
import os
import sqlalchemy

from chromite.lib import constants
from chromite.lib import cidb
from chromite.lib import cros_test_lib
from chromite.lib import factory
from chromite.lib import osutils


class RetryableOperationalError(exceptions.EnvironmentError):
  """An operational error with retryable error code."""

  def __init__(self):
    super(RetryableOperationalError, self).__init__(1053, 'retryable')


class FatalOperationalError(exceptions.EnvironmentError):
  """An operational error with fatal error code."""

  def __init__(self):
    super(FatalOperationalError, self).__init__(9999, 'fatal')


class UnknownError(Exception):
  """An error that's not an OperationalError."""


class HelperFunctionsTest(cros_test_lib.TestCase):
  """Test (private) helper functions in the module."""

  def _WrapError(self, error):
    return sqlalchemy.exc.OperationalError(
        statement=None, params=None, orig=error)

  # pylint: disable=protected-access
  def testIsRetryableExceptionMatch(self):
    self.assertTrue(cidb._IsRetryableException(RetryableOperationalError()))
    self.assertFalse(cidb._IsRetryableException(FatalOperationalError()))
    self.assertFalse(cidb._IsRetryableException(UnknownError()))

    self.assertTrue(cidb._IsRetryableException(self._WrapError(
        RetryableOperationalError())))
    self.assertFalse(cidb._IsRetryableException(self._WrapError(
        FatalOperationalError())))
    self.assertFalse(cidb._IsRetryableException(self._WrapError(
        UnknownError())))


class CIDBConnectionFactoryTest(cros_test_lib.MockTestCase):
  """Test that CIDBConnectionFactory behaves as expected."""

  def setUp(self):
    # Ensure that we do not create any live connections in this unit test.
    self.connection_mock = self.PatchObject(cidb, 'CIDBConnection')
    # pylint: disable=W0212
    cidb.CIDBConnectionFactory._ClearCIDBSetup()

  def tearDown(self):
    # pylint: disable=protected-access
    cidb.CIDBConnectionFactory._ClearCIDBSetup()

  def testGetConnectionBeforeSetup(self):
    """Calling GetConnection before Setup should raise exception."""
    self.assertRaises(factory.ObjectFactoryIllegalOperation,
                      cidb.CIDBConnectionFactory.GetCIDBConnectionForBuilder)

  def testSetupProd(self):
    """Test that SetupProd behaves as expected."""
    cidb.CIDBConnectionFactory.SetupProdCidb()
    cidb.CIDBConnectionFactory.GetCIDBConnectionForBuilder()

    # Expected constructor call
    self.connection_mock.assert_called_once_with(constants.CIDB_PROD_BOT_CREDS)
    self.assertTrue(cidb.CIDBConnectionFactory.IsCIDBSetup())
    self.assertRaises(factory.ObjectFactoryIllegalOperation,
                      cidb.CIDBConnectionFactory.SetupProdCidb)
    self.assertRaises(factory.ObjectFactoryIllegalOperation,
                      cidb.CIDBConnectionFactory.SetupDebugCidb)
    self.assertRaises(factory.ObjectFactoryIllegalOperation,
                      cidb.CIDBConnectionFactory.SetupMockCidb)
    self.assertRaises(factory.ObjectFactoryIllegalOperation,
                      cidb.CIDBConnectionFactory.SetupNoCidb)

  def testSetupDebug(self):
    """Test that SetupDebug behaves as expected."""
    cidb.CIDBConnectionFactory.SetupDebugCidb()
    cidb.CIDBConnectionFactory.GetCIDBConnectionForBuilder()

    # Expected constructor call
    self.connection_mock.assert_called_once_with(constants.CIDB_DEBUG_BOT_CREDS)
    self.assertTrue(cidb.CIDBConnectionFactory.IsCIDBSetup())
    self.assertRaises(factory.ObjectFactoryIllegalOperation,
                      cidb.CIDBConnectionFactory.SetupProdCidb)
    self.assertRaises(factory.ObjectFactoryIllegalOperation,
                      cidb.CIDBConnectionFactory.SetupDebugCidb)
    self.assertRaises(factory.ObjectFactoryIllegalOperation,
                      cidb.CIDBConnectionFactory.SetupMockCidb)
    self.assertRaises(factory.ObjectFactoryIllegalOperation,
                      cidb.CIDBConnectionFactory.SetupNoCidb)

  def testInvalidateSetup(self):
    """Test that cidb connection can be invalidated."""
    cidb.CIDBConnectionFactory.SetupProdCidb()
    cidb.CIDBConnectionFactory.InvalidateCIDBSetup()
    self.assertRaises(AssertionError,
                      cidb.CIDBConnectionFactory.GetCIDBConnectionForBuilder)

  def testSetupMock(self):
    """Test that SetupMock behaves as expected."""
    # Set the CIDB to mock mode, but without supplying a mock
    cidb.CIDBConnectionFactory.SetupMockCidb()

    # Calls to non-mock Setup methods should fail.
    self.assertRaises(factory.ObjectFactoryIllegalOperation,
                      cidb.CIDBConnectionFactory.SetupProdCidb)
    self.assertRaises(factory.ObjectFactoryIllegalOperation,
                      cidb.CIDBConnectionFactory.SetupDebugCidb)

    # Now supply a mock.
    a = object()
    cidb.CIDBConnectionFactory.SetupMockCidb(a)
    self.assertTrue(cidb.CIDBConnectionFactory.IsCIDBSetup())
    self.assertEqual(cidb.CIDBConnectionFactory.GetCIDBConnectionForBuilder(),
                     a)

    # Mock object can be changed by future SetupMockCidb call.
    b = object()
    cidb.CIDBConnectionFactory.SetupMockCidb(b)
    self.assertEqual(cidb.CIDBConnectionFactory.GetCIDBConnectionForBuilder(),
                     b)

    # Mock object can be cleared by future ClearMock call.
    cidb.CIDBConnectionFactory.ClearMock()

    # Calls to non-mock Setup methods should still fail.
    self.assertRaises(factory.ObjectFactoryIllegalOperation,
                      cidb.CIDBConnectionFactory.SetupProdCidb)
    self.assertRaises(factory.ObjectFactoryIllegalOperation,
                      cidb.CIDBConnectionFactory.SetupDebugCidb)

  def testSetupNo(self):
    """Test that SetupNoCidb behaves as expected."""
    cidb.CIDBConnectionFactory.SetupMockCidb()
    cidb.CIDBConnectionFactory.SetupNoCidb()
    cidb.CIDBConnectionFactory.SetupNoCidb()
    self.assertTrue(cidb.CIDBConnectionFactory.IsCIDBSetup())
    self.assertEqual(cidb.CIDBConnectionFactory.GetCIDBConnectionForBuilder(),
                     None)
    self.assertRaises(factory.ObjectFactoryIllegalOperation,
                      cidb.CIDBConnectionFactory.SetupProdCidb)
    self.assertRaises(factory.ObjectFactoryIllegalOperation,
                      cidb.CIDBConnectionFactory.SetupDebugCidb)
    self.assertRaises(factory.ObjectFactoryIllegalOperation,
                      cidb.CIDBConnectionFactory.SetupMockCidb)


# pylint: disable=protected-access
class SchemaVersionedMySQLConnectionTest(cros_test_lib.MockTempDirTestCase):
  """Test for SchemaVersionedMySQLConnection."""

  def setUp(self):
    self.db_name = 'cidb'
    mock_engine = mock.Mock()
    self.PatchObject(sqlalchemy, 'create_engine', return_value=mock_engine)
    user_path = os.path.join(self.tempdir, 'user.txt')
    osutils.WriteFile(user_path, 'user')
    password_path = os.path.join(self.tempdir, 'password.txt')
    osutils.WriteFile(password_path, 'password')

    mock_result = mock.Mock()
    mock_result.fetchall.return_value = [self.db_name]
    self.PatchObject(cidb.SchemaVersionedMySQLConnection,
                     '_ExecuteWithEngine', return_value=mock_result)

  def testConnectionWithIP(self):
    """Test connection with IP."""
    host_path = os.path.join(self.tempdir, 'host.txt')
    osutils.WriteFile(host_path, '127.0.0.1')
    port_path = os.path.join(self.tempdir, 'port.txt')
    osutils.WriteFile(port_path, '3306')
    conn = cidb.SchemaVersionedMySQLConnection(
        self.db_name, cidb.CIDB_MIGRATIONS_DIR, self.tempdir)

    self.assertEqual(
        str(conn._connect_url),
        'mysql://user:password@127.0.0.1:3306/cidb')

  def testConnectionWithUnixSocket(self):
    """Test Connection with Unix Socket."""
    # No unix_socket.txt found.
    conn = cidb.SchemaVersionedMySQLConnection(
        self.db_name, cidb.CIDB_MIGRATIONS_DIR, self.tempdir,
        for_service=True)
    self.assertEqual(
        str(conn._connect_url),
        'mysql+pymysql://user:password@/cidb')

    unix_socket_path = os.path.join(self.tempdir, 'unix_socket.txt')
    osutils.WriteFile(unix_socket_path, '/cloudsql/CLOUDSQL_CONNECTION_NAME')

    conn = cidb.SchemaVersionedMySQLConnection(
        self.db_name, cidb.CIDB_MIGRATIONS_DIR, self.tempdir,
        for_service=True)

    self.assertEqual(
        str(conn._connect_url),
        'mysql+pymysql://user:password@/cidb?unix_socket='
        '/cloudsql/CLOUDSQL_CONNECTION_NAME')
