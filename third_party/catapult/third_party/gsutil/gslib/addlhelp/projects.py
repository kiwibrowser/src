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
"""Additional help about Google Cloud Storage projects."""

from __future__ import absolute_import

from gslib.help_provider import HelpProvider

_DETAILED_HELP_TEXT = ("""
<B>OVERVIEW</B>
  This section discusses how to work with projects in Google Cloud Storage.


<B>PROJECT MEMBERS AND PERMISSIONS</B>
  There are three groups of users associated with each project:

  - Project Owners are allowed to list, create, and delete buckets,
    and can also perform administrative tasks like adding and removing team
    members and changing billing. The project owners group is the owner
    of all buckets within a project, regardless of who may be the original
    bucket creator.

  - Project Editors are allowed to list, create, and delete buckets.

  - Project Viewers are allowed to list buckets within a project.

  These project groups make it easy to set up a bucket and start uploading
  objects with access control appropriate for a project at your company, as
  the three group memberships can be configured by your administrative staff.
  Control over projects and their associated memberships is provided by the
  `Google Cloud Platform Console <https://cloud.google.com/console#/project>`_.


<B>HOW PROJECT MEMBERSHIP IS REFLECTED IN BUCKET ACLS</B>
  When you create a bucket without specifying an ACL the bucket is given a
  "project-private" ACL, which grants the permissions described in the previous
  section. Here's an example of such an ACL:

    [
      {
        "entity": "project-owners-12345",
        "projectTeam": {
          "projectNumber": "12345",
          "team": "owners"
        },
        "role": "OWNER"
      },
      {
        "entity": "project-editors-12345",
        "projectTeam": {
          "projectNumber": "12345",
          "team": "editors"
        },
        "role": "OWNER"
      },
      {
        "entity": "project-viewers-12345",
        "projectTeam": {
          "projectNumber": "12345",
          "team": "viewers"
        },
        "role": "READER"
      }
    ]

  You can edit the bucket ACL if you want to (see "gsutil help acl"),
  but for many cases you'll never need to, and instead can change group
  membership via the
  `Google Cloud Platform Console <https://cloud.google.com/console#/project>`_.


<B>IDENTIFYING PROJECTS WHEN CREATING AND LISTING BUCKETS</B>
  When you create a bucket you need to provide the project ID that will own
  the bucket you want to create, and when you want to list your buckets, you
  need to provide the project ID that you want to list. By default, gsutil uses
  the default_project_id in your ~/.boto configuration file. You can
  instead use the -p option (e.g., "gsutil mb -p <project-id>" or
  "gsutil ls -p <project-id>"). The project ID you use must be either the
  project ID or the project number from the Google Cloud Platform Console
  dashboard.

  Note that the project name is a user-friendly name that you can choose. It is
  not the same thing as the project ID that is required by the gsutil mb and ls
  commands.
""")


class CommandOptions(HelpProvider):
  """Additional help about Google Cloud Storage projects."""

  # Help specification. See help_provider.py for documentation.
  help_spec = HelpProvider.HelpSpec(
      help_name='projects',
      help_name_aliases=[
          'apis console', 'cloud console', 'console', 'dev console', 'project',
          'proj', 'project-id'],
      help_type='additional_help',
      help_one_line_summary='Working With Projects',
      help_text=_DETAILED_HELP_TEXT,
      subcommand_help_text={},
  )
