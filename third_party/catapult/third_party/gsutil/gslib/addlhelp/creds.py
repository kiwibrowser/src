# -*- coding: utf-8 -*-
# Copyright 2013 Google Inc. All Rights Reserved.
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
"""Additional help about types of credentials and authentication."""

from __future__ import absolute_import

from gslib.help_provider import HelpProvider


_DETAILED_HELP_TEXT = ("""
<B>OVERVIEW</B>
  gsutil currently supports several types of credentials/authentication, as
  well as the ability to access public data anonymously (see "gsutil help anon"
  for more on anonymous access). Each of these type of credentials is discussed
  in more detail below, along with information about configuring and using
  credentials via either the Cloud SDK or standalone installations of gsutil.


<B>Configuring/Using Credentials via Cloud SDK Distribution of gsutil</B>
  When gsutil is installed/used via the Cloud SDK ("gcloud"), credentials are
  stored by Cloud SDK in a non-user-editable file located under
  ~/.config/gcloud (any manipulation of credentials should be done via the
  gcloud auth command). If you need to set up multiple credentials (e.g., one
  for an individual user account and a second for a service account), the
  gcloud auth command manages the credentials for you, and you switch between
  credentials using the gcloud auth command as well (for more details see
  https://developers.google.com/cloud/sdk/gcloud/#gcloud.auth).

  Once credentials have been configured via gcloud auth, those credentials will
  be used regardless of whether the user has any boto configuration files (which
  are located at ~/.boto unless a different path is specified in the BOTO_CONFIG
  environment variable). However, gsutil will still look for credentials in the
  boto config file if a type of non-GCS credential is needed that's not stored
  in the gcloud credential store (e.g., an HMAC credential for an S3 account).


<B>Configuring/Using Credentials via Standalone gsutil Distribution</B>
  If you installed a standalone distribution of gsutil (downloaded from
  https://pub.storage.googleapis.com/gsutil.tar.gz,
  https://pub.storage.googleapis.com/gsutil.zip, or PyPi), credentials are
  configured using the gsutil config command, and are stored in the
  user-editable boto config file (located at ~/.boto unless a different path is
  specified in the BOTO_CONFIG environment). In this case if you want to set up
  multiple credentials (e.g., one for an individual user account and a second
  for a service account), you run gsutil config once for each credential, and
  save each of the generated boto config files (e.g., renaming one to
  ~/.boto_user_account and the second to ~/.boto_service_account), and you
  switch between the credentials using the BOTO_CONFIG environment variable
  (e.g., by running BOTO_CONFIG=~/.boto_user_account gsutil ls).

  Note that when using the standalone version of gsutil with the JSON API you
  can configure at most one of the following types of Google Cloud Storage
  credentials in a single boto config file: OAuth2 User Account, OAuth2 Service
  Account. In addition to these, you may also have S3 HMAC credentials
  (necessary for using s3:// URLs) and Google Compute Engine Internal Service
  Account credentials. Google Compute Engine Internal Service Account
  credentials are used only when OAuth2 credentials are not present.


<B>SUPPORTED CREDENTIAL TYPES</B>
  gsutil supports several types of credentials (the specific subset depends on
  which distribution of gsutil you are using; see above discussion).

  OAuth2 User Account:
    This is the preferred type of credentials for authenticating requests on
    behalf of a specific user (which is probably the most common use of gsutil).
    This is the default type of credential that will be created when you run
    "gsutil config" (or "gcloud init" for Cloud SDK installs).
    For more details about OAuth2 authentication, see:
    https://developers.google.com/accounts/docs/OAuth2#scenarios

  HMAC:
    This type of credential can be used by programs that are implemented using
    HMAC authentication, which is an authentication mechanism supported by
    certain other cloud storage service providers. This type of credential can
    also be used for interactive use when moving data to/from service providers
    that support HMAC credentials. This is the type of credential that will be
    created when you run "gsutil config -a".

    Note that it's possible to set up HMAC credentials for both Google Cloud
    Storage and another service provider; or to set up OAuth2 user account
    credentials for Google Cloud Storage and HMAC credentials for another
    service provider. To do so, after you run the "gsutil config" command (or
    "gcloud init" for Cloud SDK installs), you can edit the generated ~/.boto
    config file and look for comments for where other credentials can be added.

    For more details about HMAC authentication, see:
      https://developers.google.com/storage/docs/reference/v1/getting-startedv1#keys

  OAuth2 Service Account:
    This is the preferred type of credential to use when authenticating on
    behalf of a service or application (as opposed to a user). For example, if
    you will run gsutil out of a nightly cron job to upload/download data,
    using a service account allows the cron job not to depend on credentials of
    an individual employee at your company. This is the type of credential that
    will be configured when you run "gsutil config -e". To configure service
    account credentials when installed via the Cloud SDK, run "gcloud auth
    activate-service-account".

    It is important to note that a service account is considered an Editor by
    default for the purposes of API access, rather than an Owner. In particular,
    the fact that Editors have OWNER access in the default object and
    bucket ACLs, but the canned ACL options remove OWNER access from
    Editors, can lead to unexpected results. The solution to this problem is to
    use "gsutil acl ch" instead of "gsutil acl set <canned-ACL>" to change
    permissions on a bucket.

    To set up a service account for use with "gsutil config -e" or "gcloud auth
    activate-service-account", see:
     https://cloud.google.com/storage/docs/authentication#generating-a-private-key 

    For more details about OAuth2 service accounts, see:
      https://developers.google.com/accounts/docs/OAuth2ServiceAccount

    For further information about account roles, see:
      https://developers.google.com/console/help/#DifferentRoles

  Google Compute Engine Internal Service Account:
    This is the type of service account used for accounts hosted by App Engine
    or Google Compute Engine. Such credentials are created automatically for
    you on Google Compute Engine when you run the gcloud compute instances
    creates command and the credentials can be controlled with the --scopes
    flag.

    For more details about Google Compute Engine service accounts, see:
      https://developers.google.com/compute/docs/authentication;

    For more details about App Engine service accounts, see:
      https://developers.google.com/appengine/docs/python/appidentity/overview
""")


class CommandOptions(HelpProvider):
  """Additional help about types of credentials and authentication."""

  # Help specification. See help_provider.py for documentation.
  help_spec = HelpProvider.HelpSpec(
      help_name='creds',
      help_name_aliases=['credentials', 'authentication', 'auth', 'gcloud'],
      help_type='additional_help',
      help_one_line_summary='Credential Types Supporting Various Use Cases',
      help_text=_DETAILED_HELP_TEXT,
      subcommand_help_text={},
  )
