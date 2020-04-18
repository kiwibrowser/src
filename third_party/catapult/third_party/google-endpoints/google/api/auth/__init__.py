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

from dogpile import cache

from google.api.auth import suppliers
from google.api.auth import tokens


cache.register_backend("lru_cache", "google.api.auth.caches", "LruBackend")


def create_authenticator(issuers_to_provider_ids, issuer_uri_configs):
  key_uri_supplier = suppliers.KeyUriSupplier(issuer_uri_configs)
  jwks_supplier = suppliers.JwksSupplier(key_uri_supplier)
  return tokens.Authenticator(issuers_to_provider_ids, jwks_supplier)
