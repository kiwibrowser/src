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
"""Additional help about object metadata."""

from __future__ import absolute_import

from gslib.help_provider import HelpProvider

_DETAILED_HELP_TEXT = ("""
<B>OVERVIEW OF METADATA</B>
  Objects can have associated metadata, which control aspects of how
  GET requests are handled, including Content-Type, Cache-Control,
  Content-Disposition, and Content-Encoding (discussed in more detail in
  the subsections below). In addition, you can set custom metadata that
  can be used by applications (e.g., tagging that particular objects possess
  some property).

  There are two ways to set metadata on objects:

  - At upload time you can specify one or more metadata properties to
    associate with objects, using the gsutil -h option.  For example, the
    following command would cause gsutil to set the Content-Type and
    Cache-Control for each of the files being uploaded:

      gsutil -h "Content-Type:text/html" \\
             -h "Cache-Control:public, max-age=3600" cp -r images \\
             gs://bucket/images

    Note that -h is an option on the gsutil command, not the cp sub-command.

  - You can set or remove metadata fields from already uploaded objects using
    the gsutil setmeta command. See "gsutil help setmeta".

  More details about specific pieces of metadata are discussed below.


<B>CONTENT-TYPE</B>
  The most commonly set metadata is Content-Type (also known as MIME type),
  which allows browsers to render the object properly. gsutil sets the
  Content-Type automatically at upload time, based on each filename extension.
  For example, uploading files with names ending in .txt will set Content-Type
  to text/plain. If you're running gsutil on Linux or MacOS and would prefer to
  have content type set based on naming plus content examination, see the
  use_magicfile configuration variable in the .boto configuration file (See
  also "gsutil help config"). In general, using use_magicfile is more robust
  and configurable, but is not available on Windows.

  If you specify Content-Type with -h when uploading content (like the
  example gsutil command given in the previous section), it overrides the
  Content-Type that would have been set based on filename extension or content.
  This can be useful if the Content-Type detection algorithm doesn't work as
  desired for some of your files.


<B>CACHE-CONTROL</B>
  Another commonly set piece of metadata is Cache-Control, which allows
  you to control whether and for how long browser and Internet caches are
  allowed to cache your objects. Cache-Control only applies to objects with
  a public-read ACL. Non-public data are not cacheable.

  Here's an example of uploading a set of objects to allow caching:

    gsutil -h "Cache-Control:public,max-age=3600" cp -a public-read \\
           -r html gs://bucket/html

  This command would upload all files in the html directory (and subdirectories)
  and make them publicly readable and cacheable, with cache expiration of
  one hour.

  Note that if you allow caching, at download time you may see older versions
  of objects after uploading a newer replacement object. Note also that because
  objects can be cached at various places on the Internet there is no way to
  force a cached object to expire globally (unlike the way you can force your
  browser to refresh its cache). If you want to prevent caching of publicly
  readable objects you should set a Cache-Control:private header on the object.
  You can do this with a command such as:

    gsutil -h Cache-Control:private cp -a public-read file.png gs://your-bucket

  Another use of Cache-Control is through the "no-transform" value,
  which instructs Google Cloud Storage to not apply any content transformations
  based on specifics of a download request, such as removing gzip
  content-encoding for incompatible clients.  Note that this parameter is only
  respected by the XML API. The Google Cloud Storage JSON API respects only the
  no-cache and max-age Cache-Control parameters.

  For details about how to set the Cache-Control header see
  "gsutil help setmeta".


<B>CONTENT-ENCODING</B>
  You can specify a Content-Encoding to indicate that an object is compressed
  (for example, with gzip compression) while maintaining its Content-Type.
  You will need to ensure that the files have been compressed using the
  specified Content-Encoding before using gsutil to upload them. Consider the
  following example for Linux:

    echo "Highly compressible text" | gzip > foo.txt
    gsutil -h "Content-Encoding:gzip" \\
           -h "Content-Type:text/plain" \\
           cp foo.txt gs://bucket/compressed

  Note that this is different from uploading a gzipped object foo.txt.gz with
  Content-Type: application/x-gzip because most browsers are able to
  dynamically decompress and process objects served with Content-Encoding: gzip
  based on the underlying Content-Type.

  For compressible content, using Content-Encoding: gzip saves network and
  storage costs, and improves content serving performance. However, for content
  that is already inherently compressed (archives and many media formats, for
  instance) applying another level of compression via Content-Encoding is
  typically detrimental to both object size and performance and should be
  avoided.

  Note also that gsutil provides an easy way to cause content to be compressed
  and stored with Content-Encoding: gzip: see the -z and -Z options in
  "gsutil help cp".


<B>CONTENT-DISPOSITION</B>
  You can set Content-Disposition on your objects, to specify presentation
  information about the data being transmitted. Here's an example:

    gsutil -h 'Content-Disposition:attachment; filename=filename.ext' \\
           cp -r attachments gs://bucket/attachments

  Setting the Content-Disposition allows you to control presentation style
  of the content, for example determining whether an attachment should be
  automatically displayed vs should require some form of action from the user to
  open it.  See https://tools.ietf.org/html/rfc6266
  for more details about the meaning of Content-Disposition.


<B>CUSTOM METADATA</B>
  You can add your own custom metadata (e.g,. for use by your application)
  to a Google Cloud Storage object by using "x-goog-meta" with -h. For example:

    gsutil -h x-goog-meta-reviewer:jane cp mycode.java gs://bucket/reviews

  You can add multiple differently-named custom metadata fields to each object.


<B>SETTABLE FIELDS; FIELD VALUES</B>
  You can't set some metadata fields, such as ETag and Content-Length. The
  fields you can set are:

  - Cache-Control
  - Content-Disposition
  - Content-Encoding
  - Content-Language
  - Content-Type
  - Custom metadata

  Field names are case-insensitive.

  All fields and their values must consist only of ASCII characters, with the
  exception of values for x-goog-meta- fields, which may contain arbitrary
  Unicode values. Note that when setting metadata using the XML API, which sends
  custom metadata as HTTP headers, Unicode characters will be encoded using
  UTF-8, then url-encoded to ASCII. For example:

    gsutil setmeta -h "x-goog-meta-foo: ã" gs://bucket/object

  would store the custom metadata key-value pair of "foo" and "%C3%A3".
  Subsequently, running "ls -L" using the JSON API to list the object's metadata
  would print "%C3%A3", while "ls -L" using the XML API would url-decode this
  value automatically, printing the character "ã".


<B>VIEWING CURRENTLY SET METADATA</B>
  You can see what metadata is currently set on an object by using:

    gsutil ls -L gs://the_bucket/the_object
""")


class CommandOptions(HelpProvider):
  """Additional help about object metadata."""

  # Help specification. See help_provider.py for documentation.
  help_spec = HelpProvider.HelpSpec(
      help_name='metadata',
      help_name_aliases=[
          'cache-control', 'caching', 'content type', 'mime type', 'mime',
          'type'],
      help_type='additional_help',
      help_one_line_summary='Working With Object Metadata',
      help_text=_DETAILED_HELP_TEXT,
      subcommand_help_text={},
  )
