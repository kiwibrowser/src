# Copyright 2016 Google Inc. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Generated client library for servicecontrol version v1."""

# NOTE: This file is originally auto-generated using google-apitools then
# style-correcting hand edits were applied.  New behaviour should not provided
# by hand, please re-generate and restyle.
from __future__ import absolute_import

from apitools.base.py import base_api
from . import servicecontrol_v1_messages as messages


class ServicecontrolV1(base_api.BaseApiClient):
    """Generated client library for service servicecontrol version v1."""

    MESSAGES_MODULE = messages

    _PACKAGE = u'servicecontrol'
    _SCOPES = [u'https://www.googleapis.com/auth/cloud-platform',
               u'https://www.googleapis.com/auth/servicecontrol']
    _VERSION = u'v1'
    _CLIENT_CLASS_NAME = u'ServicecontrolV1'
    _URL_VERSION = u'v1'
    _API_KEY = None

    # pylint: disable=too-many-arguments
    def __init__(self, url='', credentials=None,
                 get_credentials=True, http=None, model=None,
                 log_request=False, log_response=False,
                 credentials_args=None, default_global_params=None,
                 additional_http_headers=None):
        """Create a new servicecontrol handle."""
        url = url or u'https://servicecontrol.googleapis.com/'
        super(ServicecontrolV1, self).__init__(
            url, credentials=credentials,
            get_credentials=get_credentials, http=http, model=model,
            log_request=log_request, log_response=log_response,
            credentials_args=credentials_args,
            default_global_params=default_global_params,
            additional_http_headers=additional_http_headers)
        self.services = self.ServicesService(self)

    class ServicesService(base_api.BaseApiService):
        """Service class for the services resource."""

        _NAME = u'services'

        def __init__(self, client):
            super(ServicecontrolV1.ServicesService, self).__init__(client)
            self._method_configs = {
                'check': base_api.ApiMethodInfo(
                    http_method=u'POST',
                    method_id=u'servicecontrol.services.check',
                    ordered_params=[u'serviceName'],
                    path_params=[u'serviceName'],
                    query_params=[],
                    relative_path=u'v1/services/{serviceName}:check',
                    request_field=u'checkRequest',
                    request_type_name=u'ServicecontrolServicesCheckRequest',
                    response_type_name=u'CheckResponse',
                    supports_download=False,
                ),
                'report': base_api.ApiMethodInfo(
                    http_method=u'POST',
                    method_id=u'servicecontrol.services.report',
                    ordered_params=[u'serviceName'],
                    path_params=[u'serviceName'],
                    query_params=[],
                    relative_path=u'v1/services/{serviceName}:report',
                    request_field=u'reportRequest',
                    request_type_name=u'ServicecontrolServicesReportRequest',
                    response_type_name=u'ReportResponse',
                    supports_download=False,
                ),
            }

            self._upload_configs = {
            }

        def check(self, request, global_params=None):
            """Checks quota, abuse status etc. to decide whether the given
            operation. should proceed. It should be called by the service
            before the given operation is executed.
            This method requires the `servicemanagement.services.check`
            permission on the specified service. For more information, see
            [Google Cloud IAM](https://cloud.google.com/iam).
            Args:
              request: (ServicecontrolServicesCheckRequest) input message
              global_params: (StandardQueryParameters, default: None)
                global arguments
            Returns:
              (CheckResponse) The response message.
            """
            config = self.GetMethodConfig('check')
            return self._RunMethod(
                config, request, global_params=global_params)

        def report(self, request, global_params=None):
            """Reports an operation to the service control features such as
            billing, logging, monitoring etc. It should be called by the
            service after the given operation is completed.
            This method requires the `servicemanagement.services.report`
            permission on the specified service. For more information, see
            [Google Cloud IAM](https://cloud.google.com/iam).
            Args:
              request: (ServicecontrolServicesReportRequest) input message
              global_params: (StandardQueryParameters, default: None) global
                arguments
            Returns:
              (ReportResponse) The response message.
            """
            config = self.GetMethodConfig('report')
            return self._RunMethod(
                config, request, global_params=global_params)
