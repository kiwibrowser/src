# Copyright 2016 Google Inc.  All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from django.http import HttpResponse

from oauth2client.contrib.django_util import decorators


def index(request):
    return HttpResponse("Hello world!")


@decorators.oauth_required
def get_profile_required(request):
    resp, content = request.oauth.http.request(
        'https://www.googleapis.com/plus/v1/people/me')
    return HttpResponse(content)


@decorators.oauth_enabled
def get_profile_optional(request):
    if request.oauth.has_credentials():
        # this could be passed into a view
        # request.oauth.http is also initialized
        return HttpResponse('User email: {}'.format(
            request.oauth.credentials.id_token['email']))
    else:
        return HttpResponse(
            'Here is an OAuth Authorize link:<a href="{}">Authorize</a>'
            .format(request.oauth.get_authorize_redirect()))
