# -*- coding: utf-8 -*-
# Copyright 2012 Google Inc. All Rights Reserved.
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
"""Additional help text for anonymous access."""

from __future__ import absolute_import

from gslib.help_provider import HelpProvider

_DETAILED_HELP_TEXT = ("""
<B>OVERVIEW</B>
  gsutil users can access publicly readable data without obtaining
  credentials. For example, the gs://uspto-pair bucket contains a number
  of publicly readable objects, so any user can run the following command
  without first obtaining credentials:

    gsutil ls gs://uspto-pair/applications/0800401*

  Users can similarly download objects they find via the above gsutil ls
  command.

  See "gsutil help acls" for more details about data protection.

<B>Configuring/Using Credentials via Cloud SDK Distribution of gsutil</B>
  If a user without credentials attempts to access protected data using gsutil,
  they will be prompted to run gcloud init to obtain credentials.

<B>Configuring/Using Credentials via Standalone gsutil Distribution</B>
  If a user without credentials attempts to access protected data using gsutil,
  they will be prompted to run gsutil config to obtain credentials.
""")


class CommandOptions(HelpProvider):
  """Additional help text for anonymous access."""

  # Help specification. See help_provider.py for documentation.
  help_spec = HelpProvider.HelpSpec(
      help_name='anon',
      help_name_aliases=['anonymous', 'public'],
      help_type='additional_help',
      help_one_line_summary='Accessing Public Data Without Credentials',
      help_text=_DETAILED_HELP_TEXT,
      subcommand_help_text={},
  )
