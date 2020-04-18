# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from api_data_source import APIDataSource
from api_list_data_source import APIListDataSource
from data_source import DataSource
from manifest_data_source import ManifestDataSource
from owners_data_source import OwnersDataSource
from permissions_data_source import PermissionsDataSource
from samples_data_source import SamplesDataSource
from sidenav_data_source import SidenavDataSource
from strings_data_source import StringsDataSource
from template_data_source import (
    ArticleDataSource, IntroDataSource, PartialDataSource)
from whats_new_data_source import WhatsNewDataSource


_all_data_sources = {
  'apis': APIDataSource,
  'api_list': APIListDataSource,
  'articles': ArticleDataSource,
  'intros': IntroDataSource,
  'manifest_source': ManifestDataSource,
  'owners': OwnersDataSource,
  'partials': PartialDataSource,
  'permissions': PermissionsDataSource,
  'samples': SamplesDataSource,
  'sidenavs': SidenavDataSource,
  'strings': StringsDataSource,
  'whatsNew' : WhatsNewDataSource
}


assert all(issubclass(cls, DataSource)
           for cls in _all_data_sources.itervalues())


def GetDataSourceNames():
  return _all_data_sources.keys()


def CreateDataSource(name, server_instance, request=None):
  '''Create a single DataSource by name.'''
  assert name in _all_data_sources
  return _all_data_sources[name](server_instance, request)


def CreateDataSources(server_instance, request=None):
  '''Create a dictionary of initialized DataSources. DataSources are
  initialized with |server_instance| and |request|. If the DataSources are
  going to be used for Refresh, |request| should be omitted.

  The key of each DataSource is the name the template system will use to access
  the DataSource.
  '''
  return dict((name, cls(server_instance, request))
              for name, cls in _all_data_sources.iteritems())
