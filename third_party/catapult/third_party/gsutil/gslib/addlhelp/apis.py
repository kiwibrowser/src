# -*- coding: utf-8 -*-
# Copyright 2014 Google Inc. All Rights Reserved.
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
"""Additional help about gsutil's interaction with Cloud Storage APIs."""

from gslib.help_provider import HelpProvider

_DETAILED_HELP_TEXT = ("""
<B>OVERVIEW</B>
  Google Cloud Storage offers two APIs: an XML and a JSON API. Gsutil can
  interact with both APIs. By default, gsutil versions starting with 4.0
  interact with the JSON API. If it is not possible to perform a command using
  one of the APIs (for example, the notification command is not supported in
  the XML API), gsutil will silently fall back to using the other API. Also,
  gsutil will automatically fall back to using the XML API when interacting
  with cloud storage providers that only support that API.

<B>CONFIGURING WHICH API IS USED</B>
  To use a certain API for interacting with Google Cloud Storage, you can set
  the 'prefer_api' variable in the "GSUtil" section of .boto config file to
  'xml' or 'json' like so:

    prefer_api = json

  This will cause gsutil to use that API where possible (falling back to the
  other API in cases as noted above). This applies to the gsutil test command
  as well; it will run integration tests against the preferred API.

<B>PERFORMANCE AND COST DIFFERENCES BETWEEN APIS</B>
  The XML API uses the boto framework.  This framework re-reads downloaded files
  to compute an MD5 hash if one is not present. For objects that do not
  include MD5 hashes in their metadata (for example Google Cloud Storage
  composite objects), this doubles the bandwidth consumed and elapsed time
  needed by the download. Therefore, if you are working with composite objects,
  it is recommended that you use the default value for prefer_api.

  The XML API also requires separate calls to get different object and bucket
  metadata fields, such as ACLs or bucket configuration. Thus, using the JSON
  API when possible uses fewer operations (and thus has a lower cost).
""")


class CommandOptions(HelpProvider):
  """Additional help about gsutil's interaction with Cloud Storage APIs."""

  # Help specification. See help_provider.py for documentation.
  help_spec = HelpProvider.HelpSpec(
      help_name='apis',
      help_name_aliases=['XML', 'JSON', 'api', 'force_api', 'prefer_api'],
      help_type='additional_help',
      help_one_line_summary='Cloud Storage APIs',
      help_text=_DETAILED_HELP_TEXT,
      subcommand_help_text={},
  )

