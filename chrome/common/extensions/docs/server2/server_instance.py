# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from api_data_source import APIDataSource
from api_list_data_source import APIListDataSource
from branch_utility import BranchUtility
from compiled_file_system import CompiledFileSystem
from content_providers import ContentProviders
from document_renderer import DocumentRenderer
from empty_dir_file_system import EmptyDirFileSystem
from environment import IsDevServer
from gcs_file_system_provider import CloudStorageFileSystemProvider
from host_file_system_iterator import HostFileSystemIterator
from host_file_system_provider import HostFileSystemProvider
from object_store_creator import ObjectStoreCreator
from platform_bundle import PlatformBundle
from samples_data_source import SamplesDataSource
from table_of_contents_renderer import TableOfContentsRenderer
from template_renderer import TemplateRenderer
from test_branch_utility import TestBranchUtility
from test_object_store import TestObjectStore


class ServerInstance(object):

  def __init__(self,
               object_store_creator,
               compiled_fs_factory,
               branch_utility,
               host_file_system_provider,
               gcs_file_system_provider,
               base_path='/'):
    '''
    |object_store_creator|
        The ObjectStoreCreator used to create almost all caches.
    |compiled_fs_factory|
        Factory used to create CompiledFileSystems, a higher-level cache type
        than ObjectStores. This can usually be derived from just
        |object_store_creator| but under special circumstances a different
        implementation needs to be passed in.
    |branch_utility|
        Has knowledge of Chrome branches, channels, and versions.
    |host_file_system_provider|
        Creates FileSystem instances which host the server at alternative
        revisions.
    |base_path|
        The path which all HTML is generated relative to. Usually this is /
        but some servlets need to override this.
    '''
    self.object_store_creator = object_store_creator

    self.compiled_fs_factory = compiled_fs_factory

    self.host_file_system_provider = host_file_system_provider
    host_fs_at_master = host_file_system_provider.GetMaster()

    self.gcs_file_system_provider = gcs_file_system_provider

    assert base_path.startswith('/') and base_path.endswith('/')
    self.base_path = base_path

    self.host_file_system_iterator = HostFileSystemIterator(
        host_file_system_provider,
        branch_utility)

    self.platform_bundle = PlatformBundle(
        branch_utility,
        self.compiled_fs_factory,
        host_fs_at_master,
        self.host_file_system_iterator,
        self.object_store_creator,
        self.base_path)

    self.content_providers = ContentProviders(
        object_store_creator,
        self.compiled_fs_factory,
        host_fs_at_master,
        self.gcs_file_system_provider)

    # TODO(kalman): Move all the remaining DataSources into DataSourceRegistry,
    # then factor out the DataSource creation into a factory method, so that
    # the entire ServerInstance doesn't need to be passed in here.
    self.template_renderer = TemplateRenderer(self)

    # TODO(kalman): It may be better for |document_renderer| to construct a
    # TemplateDataSource itself rather than depending on template_renderer, but
    # for that the above todo should be addressed.
    self.document_renderer = DocumentRenderer(
        TableOfContentsRenderer(host_fs_at_master,
                                compiled_fs_factory,
                                self.template_renderer),
        self.platform_bundle)

  @staticmethod
  def ForTest(file_system=None, file_system_provider=None, base_path='/'):
    object_store_creator = ObjectStoreCreator.ForTest()
    if file_system is None and file_system_provider is None:
      raise ValueError('Either |file_system| or |file_system_provider| '
                       'must be specified')
    if file_system and file_system_provider:
      raise ValueError('Only one of |file_system| and |file_system_provider| '
                       'can be specified')
    if file_system_provider is None:
      file_system_provider = HostFileSystemProvider.ForTest(
          file_system,
          object_store_creator)
    return ServerInstance(object_store_creator,
                          CompiledFileSystem.Factory(object_store_creator),
                          TestBranchUtility.CreateWithCannedData(),
                          file_system_provider,
                          CloudStorageFileSystemProvider(object_store_creator),
                          base_path=base_path)

  @staticmethod
  def ForLocal():
    object_store_creator = ObjectStoreCreator(start_empty=False,
                                              store_type=TestObjectStore)
    host_file_system_provider = HostFileSystemProvider.ForLocal(
        object_store_creator)
    return ServerInstance(
        object_store_creator,
        CompiledFileSystem.Factory(object_store_creator),
        BranchUtility.Create(object_store_creator),
        host_file_system_provider,
        CloudStorageFileSystemProvider(object_store_creator))
