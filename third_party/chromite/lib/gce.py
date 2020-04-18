# -*- coding: utf-8 -*-
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A convinient wrapper of the GCE python API.

Public methods in class GceContext raise HttpError when the underlining call to
Google API fails, or gce.Error on other failures.
"""

from __future__ import print_function

import httplib2

from chromite.lib import cros_logging as logging
from chromite.lib import timeout_util
from googleapiclient.discovery import build
from googleapiclient.errors import HttpError
from googleapiclient.http import HttpRequest
from oauth2client.client import GoogleCredentials


class Error(Exception):
  """Base exception for this module."""


class ResourceNotFoundError(Error):
  """Exceptions raised when requested GCE resource was not found."""


class RetryOnServerErrorHttpRequest(HttpRequest):
  """A HttpRequest that will be retried on server errors automatically."""

  def __init__(self, num_retries, *args, **kwargs):
    """Constructor for RetryOnServerErrorHttpRequest."""
    self.num_retries = num_retries
    super(RetryOnServerErrorHttpRequest, self).__init__(*args, **kwargs)

  def execute(self, http=None, num_retries=None):
    """Excutes a RetryOnServerErrorHttpRequest.

    HttpRequest.execute() has the option of automatically retrying on server
    errors, i.e., 500 status codes. Call it with a non-zero value of
    |num_retries| will cause failed requests to be retried.

    Args:
      http: The httplib2.http to send this request through.
      num_retries: Number of retries. Class default value will be used if
          omitted.

    Returns:
      A deserialized object model of the response body as determined
          by the postproc. See HttpRequest.execute().
    """
    return super(RetryOnServerErrorHttpRequest, self).execute(
        http=http, num_retries=num_retries or self.num_retries)


def _GetMetdataValue(metadata, key):
  """Finds a value corresponding to a given metadata key.

  Args:
    metadata: metadata object, i.e. a dict containing containing 'items'
      - a list of key-value pairs.
    key: name of the key.

  Returns:
    Corresponding value or None if it was not found.
  """
  for item in metadata['items']:
    if item['key'] == key:
      return item['value']
  return None


def _UpdateMetadataValue(metadata, key, value):
  """Updates a single key-value pair in a metadata object.

  Args:
    metadata: metadata object, i.e. a dict containing containing 'items'
      - a list of key-value pairs.
    key: name of the key.
    value: new value for the key, or None if it should be removed.
  """
  items = metadata.setdefault('items', [])
  for item in items:
    if item['key'] == key:
      if value is None:
        items.remove(item)
      else:
        item['value'] = value
      return

  if value is not None:
    items.append({
        'key': key,
        'value': value,
    })


class GceContext(object):
  """A convinient wrapper around the GCE Python API."""

  # These constants are made public so that users can customize as they need.
  DEFAULT_TIMEOUT_SEC = 5 * 60
  INSTANCE_OPERATIONS_TIMEOUT_SEC = 10 * 60
  IMAGE_OPERATIONS_TIMEOUT_SEC = 10 * 60

  _GCE_SCOPES = (
      'https://www.googleapis.com/auth/compute',  # CreateInstance, CreateImage
      'https://www.googleapis.com/auth/devstorage.full_control', # CreateImage
  )
  _DEFAULT_NETWORK = 'default'
  _DEFAULT_MACHINE_TYPE = 'n1-standard-8'

  # Project default service account and scopes.
  _DEFAULT_SERVICE_ACCOUNT_EMAIL = 'default'
  # The list is in line with what the gcloud cli uses.
  # https://cloud.google.com/sdk/gcloud/reference/compute/instances/create
  _DEFAULT_INSTANCE_SCOPES = [
      'https://www.googleapis.com/auth/cloud.useraccounts.readonly',
      'https://www.googleapis.com/auth/devstorage.read_only',
      'https://www.googleapis.com/auth/logging.write',
  ]

  # This is made public to allow easy customization of the retry behavior.
  RETRIES = 2

  def __init__(self, project, zone, credentials, thread_safe=False):
    """Initializes GceContext.

    Args:
      project: The GCP project to create instances in.
      zone: The default zone to create instances in.
      credentials: The credentials used to call the GCE API.
      thread_safe: Whether the client is expected to be thread safe.
    """
    self.project = project
    self.zone = zone

    def _BuildRequest(http, *args, **kwargs):
      """Custom request builder."""
      return self._BuildRetriableRequest(self.RETRIES, http, thread_safe,
                                         credentials, *args, **kwargs)

    self.gce_client = build('compute', 'v1', credentials=credentials,
                            requestBuilder=_BuildRequest)

    self.region = self.GetZoneRegion(zone)

  @classmethod
  def ForServiceAccount(cls, project, zone, json_key_file):
    """Creates a GceContext using service account credentials.

    About service account:
    https://developers.google.com/api-client-library/python/auth/service-accounts

    Args:
      project: The GCP project to create images and instances in.
      zone: The default zone to create instances in.
      json_key_file: Path to the service account JSON key.

    Returns:
      GceContext.
    """
    credentials = GoogleCredentials.from_stream(json_key_file).create_scoped(
        cls._GCE_SCOPES)
    return GceContext(project, zone, credentials)

  @classmethod
  def ForServiceAccountThreadSafe(cls, project, zone, json_key_file):
    """Creates a thread-safe GceContext using service account credentials.

    About service account:
    https://developers.google.com/api-client-library/python/auth/service-accounts

    Args:
      project: The GCP project to create images and instances in.
      zone: The default zone to create instances in.
      json_key_file: Path to the service account JSON key.

    Returns:
      GceContext.
    """
    credentials = GoogleCredentials.from_stream(json_key_file).create_scoped(
        cls._GCE_SCOPES)
    return GceContext(project, zone, credentials, thread_safe=True)

  def CreateAddress(self, name, region=None):
    """Reserves an external IP address.

    Args:
      name: The name to assign to the address.
      region: Region to reserved the address in.

    Returns:
      The reserved address as a string.
    """
    body = {
        'name': name,
    }
    operation = self.gce_client.addresses().insert(
        project=self.project,
        region=region or self.region,
        body=body).execute()
    self._WaitForRegionOperation(
        operation['name'], region,
        timeout_sec=self.INSTANCE_OPERATIONS_TIMEOUT_SEC)

    address = self.gce_client.addresses().get(
        project=self.project,
        region=region or self.region,
        address=name).execute()

    return address['address']

  def DeleteAddress(self, name, region=None):
    """Frees up an external IP address.

    Args:
      name: The name of the address.
      region: Region of the address.
    """
    operation = self.gce_client.addresses().delete(
        project=self.project,
        region=region or self.region,
        address=name).execute()
    self._WaitForRegionOperation(
        operation['name'], region=region or self.region,
        timeout_sec=self.INSTANCE_OPERATIONS_TIMEOUT_SEC)

  def GetZoneRegion(self, zone=None):
    """Resolves name of the region that a zone belongs to.

    Args:
      zone: The zone to resolve.

    Returns:
      Name of the region corresponding to the zone.
    """
    zone_resource = self.gce_client.zones().get(
        project=self.project,
        zone=zone or self.zone).execute()
    return zone_resource['region'].split('/')[-1]

  def CreateInstance(self, name, image, zone=None, network=None, subnet=None,
                     machine_type=None, default_scopes=True,
                     static_address=None, **kwargs):
    """Creates an instance with the given image and waits until it's ready.

    Args:
      name: Instance name.
      image: Fully spelled URL of the image, e.g., for private images,
          'global/images/my-private-image', or for images from a
          publicly-available project,
          'projects/debian-cloud/global/images/debian-7-wheezy-vYYYYMMDD'.
          Details:
          https://cloud.google.com/compute/docs/reference/latest/instances/insert
      zone: The zone to create the instance in. Default zone will be used if
          omitted.
      network: An existing network to create the instance in. Default network
          will be used if omitted.
      subnet: The subnet to create the instance in.
      machine_type: The machine type to use. Default machine type will be used
          if omitted.
      default_scopes: If true, the default scopes are added to the instances.
      static_address: External IP address to assign to the instance as a string.
          If None an emphemeral address will be used.
      kwargs: Other possible Instance Resource properties.
          https://cloud.google.com/compute/docs/reference/latest/instances#resource
          Note that values from kwargs will overrule properties constructed from
          positinal arguments, i.e., name, image, zone, network and
          machine_type.

    Returns:
      URL to the created instance.
    """
    logging.info('Creating instance "%s" with image "%s" ...', name, image)
    network = 'global/networks/%s' % network or self._DEFAULT_NETWORK
    machine_type = 'zones/%s/machineTypes/%s' % (
        zone or self.zone, machine_type or self._DEFAULT_MACHINE_TYPE)
    service_accounts = (
        {
            'email': self._DEFAULT_SERVICE_ACCOUNT_EMAIL,
            'scopes': self._DEFAULT_INSTANCE_SCOPES,
        },
    ) if default_scopes else ()

    config = {
        'name': name,
        'machineType': machine_type,
        'disks': (
            {
                'boot': True,
                'autoDelete': True,
                'initializeParams': {
                    'sourceImage': image,
                },
            },
        ),
        'networkInterfaces': (
            {
                'network': network,
                'accessConfigs': (
                    {
                        'type': 'ONE_TO_ONE_NAT',
                        'name': 'External NAT',
                    },
                ),
            },
        ),
        'serviceAccounts' : service_accounts,
    }
    config.update(**kwargs)
    if static_address is not None:
      config['networkInterfaces'][0]['accessConfigs'][0]['natIP'] = (
          static_address)
    if subnet is not None:
      region = self.GetZoneRegion(zone)
      config['networkInterfaces'][0]['subnetwork'] = (
          'regions/%s/subnetworks/%s' % (region, subnet)
      )
    operation = self.gce_client.instances().insert(
        project=self.project,
        zone=zone or self.zone,
        body=config).execute()
    self._WaitForZoneOperation(
        operation['name'],
        timeout_sec=self.INSTANCE_OPERATIONS_TIMEOUT_SEC,
        timeout_handler=lambda: self.DeleteInstance(name))
    return operation['targetLink']

  def DeleteInstance(self, name, zone=None):
    """Deletes an instance with the name and waits until it's done.

    Args:
      name: Name of the instance to delete.
      zone: Zone where the instance is in. Default zone will be used if omitted.
    """
    logging.info('Deleting instance "%s" ...', name)
    operation = self.gce_client.instances().delete(
        project=self.project,
        zone=zone or self.zone,
        instance=name).execute()
    self._WaitForZoneOperation(
        operation['name'], timeout_sec=self.INSTANCE_OPERATIONS_TIMEOUT_SEC)

  def StartInstance(self, name, zone=None):
    """Starts an instance with the name and waits until it's done.

    Args:
      name: Name of the instance to start.
      zone: Zone where the instance is in. Default zone will be used if omitted.
    """
    logging.info('Starting instance "%s" ...', name)
    operation = self.gce_client.instances().start(
        project=self.project,
        zone=zone or self.zone,
        instance=name).execute()
    self._WaitForZoneOperation(
        operation['name'], timeout_sec=self.INSTANCE_OPERATIONS_TIMEOUT_SEC)

  def StopInstance(self, name, zone=None):
    """Stops an instance with the name and waits until it's done.

    Args:
      name: Name of the instance to stop.
      zone: Zone where the instance is in. Default zone will be used if omitted.
    """
    logging.info('Stopping instance "%s" ...', name)
    operation = self.gce_client.instances().stop(
        project=self.project,
        zone=zone or self.zone,
        instance=name).execute()
    self._WaitForZoneOperation(
        operation['name'], timeout_sec=self.INSTANCE_OPERATIONS_TIMEOUT_SEC)

  def CreateImage(self, name, source):
    """Creates an image with the given |source|.

    Args:
      name: Name of the image to be created.
      source:
        Google Cloud Storage object of the source disk, e.g.,
        'https://storage.googleapis.com/my-gcs-bucket/test_image.tar.gz'.

    Returns:
      URL to the created image.
    """
    logging.info('Creating image "%s" with "source" %s ...', name, source)
    config = {
        'name': name,
        'rawDisk': {
            'source': source,
        },
    }
    operation = self.gce_client.images().insert(
        project=self.project,
        body=config).execute()
    self._WaitForGlobalOperation(operation['name'],
                                 timeout_sec=self.IMAGE_OPERATIONS_TIMEOUT_SEC,
                                 timeout_handler=lambda: self.DeleteImage(name))
    return operation['targetLink']

  def DeleteImage(self, name):
    """Deletes an image and waits until it's deleted.

    Args:
      name: Name of the image to delete.
    """
    logging.info('Deleting image "%s" ...', name)
    operation = self.gce_client.images().delete(
        project=self.project,
        image=name).execute()
    self._WaitForGlobalOperation(operation['name'],
                                 timeout_sec=self.IMAGE_OPERATIONS_TIMEOUT_SEC)

  def ListInstances(self, zone=None):
    """Lists all instances.

    Args:
      zone: Zone where the instances are in. Default zone will be used if
            omitted.

    Returns:
      A list of Instance Resources if found, or an empty list otherwise.
    """
    result = self.gce_client.instances().list(project=self.project,
                                              zone=zone or self.zone).execute()
    return result.get('items', [])

  def ListImages(self):
    """Lists all images.

    Returns:
      A list of Image Resources if found, or an empty list otherwise.
    """
    result = self.gce_client.images().list(project=self.project).execute()
    return result.get('items', [])

  def GetInstance(self, instance, zone=None):
    """Gets an Instance Resource by name and zone.

    Args:
      instance: Name of the instance.
      zone: Zone where the instance is in. Default zone will be used if omitted.

    Returns:
      An Instance Resource.

    Raises:
      ResourceNotFoundError if instance was not found, or HttpError on other
      HTTP failures.
    """
    try:
      return self.gce_client.instances().get(project=self.project,
                                             zone=zone or self.zone,
                                             instance=instance).execute()
    except HttpError as e:
      if e.resp.status == 404:
        raise ResourceNotFoundError(
            'Instance "%s" for project "%s" in zone "%s" was not found.' %
            (instance, self.project, zone or self.zone))
      else:
        raise

  def GetInstanceIP(self, instance, zone=None):
    """Gets the external IP of an instance.

    Args:
      instance: Name of the instance to get IP for.
      zone: Zone where the instance is in. Default zone will be used if omitted.

    Returns:
      External IP address of the instance.

    Raises:
      Error: Something went wrong when trying to get IP for the instance.
    """
    result = self.GetInstance(instance, zone)
    try:
      return result['networkInterfaces'][0]['accessConfigs'][0]['natIP']
    except (KeyError, IndexError):
      raise Error('Failed to get IP address for instance %s' % instance)

  def GetInstanceInternalIP(self, instance, zone=None):
    """Gets the internal IP of an instance."""
    result = self.GetInstance(instance, zone)
    try:
      return result['networkInterfaces'][0]['networkIP']
    except (KeyError, IndexError):
      raise Error('Failed to get internal IP for instance %s' % instance)

  def GetImage(self, image):
    """Gets an Image Resource by name.

    Args:
      image: Name of the image to look for.

    Returns:
      An Image Resource.

    Raises:
      ResourceNotFoundError: The requested image was not found.
    """
    try:
      return self.gce_client.images().get(project=self.project,
                                          image=image).execute()
    except HttpError as e:
      if e.resp.status == 404:
        raise ResourceNotFoundError('Image "%s" for project "%s" was not found.'
                                    % (image, self.project))
      else:
        raise

  def InstanceExists(self, instance, zone=None):
    """Checks if an instance exists in the current project.

    Args:
      instance: Name of the instance to check existence of.
      zone: Zone where the instance is in. Default zone will be used if omitted.

    Returns:
      True if the instance exists or False otherwise.
    """
    try:
      return self.GetInstance(instance, zone) is not None
    except ResourceNotFoundError:
      return False

  def ImageExists(self, image):
    """Checks if an image exists in the current project.

    Args:
      image: Name of the image to check existence of.

    Returns:
      True if the instance exists or False otherwise.
    """
    try:
      return self.GetImage(image) is not None
    except ResourceNotFoundError:
      return False

  def GetCommonInstanceMetadata(self, key):
    """Looks up a single project metadata value.

    Args:
      key: Metadata key name.

    Returns:
      Metadata value corresponding to the key, or None if it was not found.
    """
    projects_data = self.gce_client.projects().get(
        project=self.project).execute()
    metadata = projects_data['commonInstanceMetadata']
    return _GetMetdataValue(metadata, key)

  def SetCommonInstanceMetadata(self, key, value):
    """Sets a single project metadata value.

    Args:
      key: Metadata key to be set.
      value: New value, or None if the given key should be removed.
    """
    projects_data = self.gce_client.projects().get(
        project=self.project).execute()
    metadata = projects_data['commonInstanceMetadata']
    _UpdateMetadataValue(metadata, key, value)
    operation = self.gce_client.projects().setCommonInstanceMetadata(
        project=self.project,
        body=metadata).execute()
    self._WaitForGlobalOperation(operation['name'])

  def GetInstanceMetadata(self, instance, key):
    """Looks up instance's metadata value.

    Args:
      instance: Name of the instance.
      key: Metadata key name.

    Returns:
      Metadata value corresponding to the key, or None if it was not found.
    """
    instance_data = self.GetInstance(instance)
    metadata = instance_data['metadata']
    return self._GetMetdataValue(metadata, key)

  def SetInstanceMetadata(self, instance, key, value):
    """Sets a single instance metadata value.

    Args:
      instance: Name of the instance.
      key: Metadata key to be set.
      value: New value, or None if the given key should be removed.
    """
    instance_data = self.GetInstance(instance)
    metadata = instance_data['metadata']
    _UpdateMetadataValue(metadata, key, value)
    operation = self.gce_client.instances().setMetadata(
        project=self.project,
        zone=self.zone,
        instance=instance,
        body=metadata).execute()
    self._WaitForZoneOperation(operation['name'])

  def _WaitForZoneOperation(self, operation, zone=None, timeout_sec=None,
                            timeout_handler=None):
    """Waits until a GCE ZoneOperation is finished or timed out.

    Args:
      operation: The GCE operation to wait for.
      zone: The zone that |operation| belongs to.
      timeout_sec: The maximum number of seconds to wait for.
      timeout_handler: A callable to be executed when timeout happens.

    Raises:
      Error when timeout happens or the operation fails.
    """
    get_request = self.gce_client.zoneOperations().get(
        project=self.project, zone=zone or self.zone, operation=operation)
    self._WaitForOperation(operation, get_request, timeout_sec,
                           timeout_handler=timeout_handler)

  def _WaitForRegionOperation(self, operation, region, timeout_sec=None,
                              timeout_handler=None):
    """Waits until a GCE RegionOperation is finished or timed out.

    Args:
      operation: The GCE operation to wait for.
      region: The region that |operation| belongs to.
      timeout_sec: The maximum number of seconds to wait for.
      timeout_handler: A callable to be executed when timeout happens.

    Raises:
      Error when timeout happens or the operation fails.
    """
    get_request = self.gce_client.regionOperations().get(
        project=self.project, region=region or self.region, operation=operation)
    self._WaitForOperation(operation, get_request, timeout_sec,
                           timeout_handler=timeout_handler)

  def _WaitForGlobalOperation(self, operation, timeout_sec=None,
                              timeout_handler=None):
    """Waits until a GCE GlobalOperation is finished or timed out.

    Args:
      operation: The GCE operation to wait for.
      timeout_sec: The maximum number of seconds to wait for.
      timeout_handler: A callable to be executed when timeout happens.

    Raises:
      Error when timeout happens or the operation fails.
    """
    get_request = self.gce_client.globalOperations().get(project=self.project,
                                                         operation=operation)
    self._WaitForOperation(operation, get_request, timeout_sec=timeout_sec,
                           timeout_handler=timeout_handler)

  def _WaitForOperation(self, operation, get_operation_request,
                        timeout_sec=None, timeout_handler=None):
    """Waits until timeout or the request gets a response with a 'DONE' status.

    Args:
      operation: The GCE operation to wait for.
      get_operation_request:
        The HTTP request to get the operation's status.
        This request will be executed periodically until it returns a status
        'DONE'.
      timeout_sec: The maximum number of seconds to wait for.
      timeout_handler: A callable to be executed when times out.

    Raises:
      Error when timeout happens or the operation fails.
    """
    def _IsDone():
      result = get_operation_request.execute()
      if result['status'] == 'DONE':
        if 'error' in result:
          raise Error(result['error'])
        return True
      return False

    try:
      timeout = timeout_sec or self.DEFAULT_TIMEOUT_SEC
      logging.info('Waiting up to %d seconds for operation [%s] to complete...',
                   timeout, operation)
      timeout_util.WaitForReturnTrue(_IsDone, timeout, period=1)
    except timeout_util.TimeoutError:
      if timeout_handler:
        timeout_handler()
      raise Error('Timeout wating for operation [%s] to complete' % operation)

  def _BuildRetriableRequest(self, num_retries, http, thread_safe=False,
                             credentials=None, *args, **kwargs):
    """Builds a request that will be automatically retried on server errors.

    Args:
      num_retries: The maximum number of times to retry until give up.
      http: An httplib2.Http object that this request will be executed through.
      thread_safe: Whether or not the request needs to be thread-safe.
      credentials: Credentials to apply to the request.
      *args: Optional positional arguments.
      **kwargs: Optional keyword arguments.

    Returns:
      RetryOnServerErrorHttpRequest: A request that will automatically retried
          on server errors.
    """
    if thread_safe:
      # Create a new http object for every request.
      http = credentials.authorize(httplib2.Http())
    return RetryOnServerErrorHttpRequest(num_retries, http, *args, **kwargs)
