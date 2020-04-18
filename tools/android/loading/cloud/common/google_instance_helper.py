# Copyright 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import time

from googleapiclient import (discovery, errors)

import common.google_error_helper as google_error_helper


class GoogleInstanceHelper(object):
  """Helper class for the Google Compute API, allowing to manage groups of
  instances more easily. Groups of instances are identified by a tag."""
  _COMPUTE_API_ROOT = 'https://www.googleapis.com/compute/v1/projects/'

  def __init__(self, credentials, project, logger):
    self._compute_api = discovery.build('compute','v1', credentials=credentials)
    self._project = project
    self._project_api_url = self._COMPUTE_API_ROOT + project
    self._region = 'europe-west1'
    self._zone = 'europe-west1-c'
    self._logger = logger

  def _ExecuteApiRequest(self, request, retry_count=3):
    """ Executes a Compute API request and returns True on success.

    Returns:
      (True, Response) in case of success, or (False, error_content) otherwise.
    """
    self._logger.info('Compute API request:\n' + request.to_json())
    try:
      response = request.execute()
      self._logger.info('Compute API response:\n' + str(response))
      return (True, response)
    except errors.HttpError as err:
      error_content = google_error_helper.GetErrorContent(err)
      error_reason = google_error_helper.GetErrorReason(error_content)
      if error_reason == 'resourceNotReady' and retry_count > 0:
        # Retry after a delay
        delay_seconds = 1
        self._logger.info(
            'Resource not ready, retrying in %i seconds.' % delay_seconds)
        time.sleep(delay_seconds)
        return self._ExecuteApiRequest(request, retry_count - 1)
      else:
        self._logger.error('Compute API error (reason: "%s"):\n%s' % (
            error_reason, err))
        if error_content:
          self._logger.error('Error details:\n%s' % error_content)
        return (False, error_content)

  def _GetTemplateName(self, tag):
    """Returns the name of the instance template associated with tag."""
    return 'template-' + tag

  def _GetInstanceGroupName(self, tag):
    """Returns the name of the instance group associated with tag."""
    return 'group-' + tag

  def CreateTemplate(self, tag, bucket, task_dir):
    """Creates an instance template for instances identified by tag.

    Args:
      tag: (string) Tag associated to a task.
      bucket: (string) Root bucket where the deployment is located.
      task_dir: (string) Subdirectory of |bucket| where task data is read and
                         written.

    Returns:
      boolean: True if successful.
    """
    image_url = self._COMPUTE_API_ROOT + \
                'ubuntu-os-cloud/global/images/ubuntu-1404-trusty-v20160406'
    request_body = {
        'name': self._GetTemplateName(tag),
        'properties': {
            'machineType': 'n1-standard-1',
            'networkInterfaces': [{
                'network': self._project_api_url + '/global/networks/default',
                'accessConfigs': [{
                    'name': 'external-IP',
                    'type': 'ONE_TO_ONE_NAT'
            }]}],
            'disks': [{
                'type': 'PERSISTENT',
                'boot': True,
                'autoDelete': True,
                'mode': 'READ_WRITE',
                'initializeParams': {'sourceImage': image_url}}],
            'canIpForward': False,
            'scheduling': {
                'automaticRestart': True,
                'onHostMaintenance': 'MIGRATE',
                'preemptible': False},
            'serviceAccounts': [{
                'scopes': [
                    'https://www.googleapis.com/auth/cloud-platform',
                    'https://www.googleapis.com/auth/cloud-taskqueue'],
                'email': 'default'}],
            'metadata': { 'items': [
                {'key': 'cloud-storage-path',
                 'value': bucket},
                {'key': 'task-dir',
                 'value': task_dir},
                {'key': 'startup-script-url',
                 'value': 'gs://%s/deployment/startup-script.sh' % bucket},
                {'key': 'taskqueue-tag', 'value': tag}]}}}
    request = self._compute_api.instanceTemplates().insert(
        project=self._project, body=request_body)
    return self._ExecuteApiRequest(request)[0]

  def DeleteTemplate(self, tag):
    """Deletes the instance template associated with tag. Returns True if
    successful.
    """
    template_name = self._GetTemplateName(tag)
    request = self._compute_api.instanceTemplates().delete(
        project=self._project, instanceTemplate=template_name)
    (success, result) = self._ExecuteApiRequest(request)
    if success:
      return True
    if google_error_helper.GetErrorReason(result) == \
        google_error_helper.REASON_NOT_FOUND:
      # The template does not exist, nothing to do.
      self._logger.warning('Template not found: ' + template_name)
      return True
    return False

  def CreateInstances(self, tag, instance_count):
    """Creates an instance group associated with tag. The instance template must
    exist for this to succeed. Returns True if successful.
    """
    template_url = '%s/global/instanceTemplates/%s' % (
        self._project_api_url, self._GetTemplateName(tag))
    request_body = {
        'zone': self._zone, 'targetSize': instance_count,
        'baseInstanceName': 'instance-' + tag,
        'instanceTemplate': template_url,
        'name': self._GetInstanceGroupName(tag)}
    request = self._compute_api.instanceGroupManagers().insert(
        project=self._project, zone=self._zone,
        body=request_body)
    return self._ExecuteApiRequest(request)[0]

  def DeleteInstance(self, tag, instance_hostname):
    """Deletes one instance from the instance group identified by tag. Returns
    True if successful.
    """
    # The instance hostname may be of the form <name>.c.<project>.internal but
    # only the <name> part should be passed to the compute API.
    name = instance_hostname.split('.')[0]
    instance_url = self._project_api_url + (
        "/zones/%s/instances/%s" % (self._zone, name))
    request = self._compute_api.instanceGroupManagers().deleteInstances(
        project=self._project, zone=self._zone,
        instanceGroupManager=self._GetInstanceGroupName(tag),
        body={'instances': [instance_url]})
    return self._ExecuteApiRequest(request)[0]

  def DeleteInstanceGroup(self, tag):
    """Deletes the instance group identified by tag. If instances are still
    running in this group, they are deleted as well.
    """
    group_name = self._GetInstanceGroupName(tag)
    request = self._compute_api.instanceGroupManagers().delete(
        project=self._project, zone=self._zone,
        instanceGroupManager=group_name)
    (success, result) = self._ExecuteApiRequest(request)
    if success:
      return True
    if google_error_helper.GetErrorReason(result) == \
        google_error_helper.REASON_NOT_FOUND:
      # The group does not exist, nothing to do.
      self._logger.warning('Instance group not found: ' + group_name)
      return True
    return False

  def GetInstanceCount(self, tag):
    """Returns the number of instances in the instance group identified by
    tag, or -1 in case of failure.
    """
    request = self._compute_api.instanceGroupManagers().listManagedInstances(
        project=self._project, zone=self._zone,
        instanceGroupManager=self._GetInstanceGroupName(tag))
    (success, response) = self._ExecuteApiRequest(request)
    if not success:
      return -1
    return len(response.get('managedInstances', []))


  def GetAvailableInstanceCount(self):
    """Returns the number of instances that can be created, according to the
    ComputeEngine quotas, or -1 on failure.
    """
    request = self._compute_api.regions().get(project=self._project,
                                              region=self._region)
    (success, response) = self._ExecuteApiRequest(request)
    if not success:
      self._logger.error('Could not get ComputeEngine region information.')
      return -1
    metric_name = 'IN_USE_ADDRESSES'
    for quota in response.get('quotas', []):
      if quota['metric'] == metric_name:
        return quota['limit'] - quota['usage']
    self._logger.error(
        metric_name + ' quota not found in ComputeEngine response.')
    return -1

