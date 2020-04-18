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

from django.conf import urls
from django.contrib import admin
import django.contrib.auth.views
from polls import views

import oauth2client.contrib.django_util.site as django_util_site


urlpatterns = [
    urls.url(r'^$', views.index),
    urls.url(r'^profile_required$', views.get_profile_required),
    urls.url(r'^profile_enabled$', views.get_profile_optional),
    urls.url(r'^admin/', urls.include(admin.site.urls)),
    urls.url(r'^login', django.contrib.auth.views.login, name="login"),
    urls.url(r'^oauth2/', urls.include(django_util_site.urls)),
]
