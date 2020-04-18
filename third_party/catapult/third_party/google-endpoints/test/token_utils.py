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

"""Provides a utility method that generates auth token."""

import json

from jwkest import jws


def generate_auth_token(payload, keys, alg="ES256", kid=None):
  json_web_signature = jws.JWS(json.dumps(payload), alg=alg, kid=kid)
  return json_web_signature.sign_compact(keys=keys)
