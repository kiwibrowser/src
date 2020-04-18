#!/usr/bin/python
#
# Copyright 2016 Google Inc. All Rights Reserved.
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


"""Google Cloud Endpoints module."""

# pylint: disable=wildcard-import

from api_config import api
from api_config import API_EXPLORER_CLIENT_ID
from api_config import AUTH_LEVEL
from api_config import EMAIL_SCOPE
from api_config import Issuer
from api_config import method
from api_exceptions import *
from apiserving import *
from endpoints_dispatcher import *
import message_parser
from resource_container import ResourceContainer
from users_id_token import get_current_user
from users_id_token import InvalidGetUserCall
from users_id_token import SKIP_CLIENT_ID_CHECK

__version__ = '2.0.0'
