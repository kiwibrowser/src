.. _distributing:

.. include:: /migration/deprecation.inc

#############################
Distributing Your Application
#############################

.. contents::
  :local:
  :backlinks: none
  :depth: 2

This document describes how to distribute Portable Native Client applications
on the web, and Native Client applications through the
`Chrome Web Store </webstore>`_ (CWS).

Portable Native Client
======================

Portable Native Client is enabled by default for web pages, so no separate
distribution step is required. Making PNaCl a part of your web application is as
simple as embedding a manifest file that points to a **pexe**. See the
:doc:`technical overview <../overview>` for more details.

.. image:: /images/nacl-in-a-web-app.png

The only constraint for distributing PNaCl modules with a web application is
abiding by the `Same-origin policy
<http://en.wikipedia.org/wiki/Same_origin_policy>`_. The PNaCl manifest and
**pexe** must either be served from the same domain with the HTML, or the `CORS
mechanism <http://en.wikipedia.org/wiki/Cross-origin_resource_sharing>`_ should
be used to safely host them on a different domain.

Non-portable Native Client
==========================

NaCl modules are only allowed for applications distributed through the `Chrome
Web Store (CWS) <https://chrome.google.com/webstore/category/apps>`_
The CWS requirement is in place to prevent the proliferation of Native Client
executables (**nexe**\s) compiled for specific architecures (e.g., x86-32,
x86-64, or ARM).

In general, the considerations and guidelines for distributing applications
through the Chrome Web Store apply to applications that contain NaCl modules as
well. Here are a few pointers to relevant documentation:

* `CWS Overview </webstore>`_
* `Choosing an App Type </webstore/choosing>`_
* `Getting started with Chrome apps </apps>`_
* `Chrome extensions </extensions>`_

In this document, we'll focus only on distribution issues specific to
applications that contain NaCl modules.

.. _distributing_packaged:

Chrome apps
-----------

A Chrome app is a special zip file (with a .crx extension) hosted in the Chrome
Web Store. This file contains all of the application parts: A Chrome Web Store
manifest file (manifest.json), an icon, and all of the regular Native Client
application files. Refer to `Chrome Apps </apps>`_ for more information about
creating a Chrome app.

Reducing the size of the user download package
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. Note::
   :class: note

   **Tip:**
   Packaging an app in a multi-platform zip file can significantly reduce the
   download and storage requirements for the app.

As described above, to upload a Chrome app to the CWS you have to create a zip
file with all the resources that your app needs, including .nexe files for
multiple architectures (x86-64, x86-32, and ARM). Prior to Chrome 28, when users
installed your app they had to download a .crx file from the CWS with all the
included .nexe files.

Starting with Chrome 28, the Chrome Web Store includes a feature called
**multi-platform zip files.** This feature lets you structure your application
directory and zip file in a way that reduces the size of the user download
package.  Here's how this feature works:

* You still include all the .nexe files in the zip file that you upload to
  the CWS, but you designate specific .nexe files (and other files if
  appropriate) for specific architectures.
* The Chrome Web Store re-packages your app, so that users only download
  the files that they need for their specific architecture.

Here is how to use this feature:

#. Create a directory called ``_platform_specific``.
   Put this directory at the same level where your CWS manifest file,
   ``manifest.json``, is located.

#. Create a subdirectory for each specific architecture that you support,
   and add the files for each architecture in the relevant subdirectory.

   Here is a sample app directory structure:

   .. naclcode::
     :prettyprint: 0

     |-- my_app_directory/
     |       |-- manifest.json
     |       |-- my_app.html
     |       |-- my_module.nmf
     |       +-- css/
     |       +-- images/
     |       +-- scripts/
     |       |-- _platform_specific/
     |       |       |-- x86-64/
     |       |       |       |-- my_module_x86_64.nexe
     |       |       |-- x86-32/
     |       |       |       |-- my_module_x86_32.nexe
     |       |       |-- arm/
     |       |       |       |-- my_module_arm.nexe
     |       |       |-- all/
     |       |       |       |-- my_module_x86_64.nexe
     |       |       |       |-- my_module_x86_64.nexe
     |       |       |       |-- my_module_x86_32.nexe

   Please note a few important points about the app directory structure:

   * The architecture-specific subdirectories:

     * can have arbitrary names;
     * must be directly under the ``_platform_specific`` directory; and
     * must be listed in the CWS manifest file (see step 3 below).

   * You can include a fallback subdirectory that provides a download package
     with all the architecture-specific files.  (In the example above this
     is the ``all/`` subdirectory.) This folder is used if the user has an
     earlier version of Chrome (prior to Chrome 28) that does not support
     multi-platform zip files.

   * You cannot include any files directly in the folder
     ``_platform_specific``.  All architecture-specific files
     must be under one of the architecture-specific subdirectories.

   * Files that are not under the ``_platform_specific`` directory are
     included in all download packages.  (In the example above, that
     includes ``my_app.html``, ``my_module.nmf``,
     and the ``css/``, ``images/``, and ``scripts/`` directories.)


#. Modify the CWS manifest file, ``manifest.json``, so that it specifies which
   subdirectory under ``_platform_specific`` corresponds to which architecture.

   The CWS manifest file must include a new name/value pair, where the name
   is ``platforms`` and the value is an array.  The array has an object for
   each Native Client architecture with two name/value pairs:

   +----------------------+---------------------------------------+
   | Name                 | Value                                 |
   +======================+=======================================+
   | ``nacl_arch``        | ``x86-64``, ``x86-32``, or ``arm``    |
   +----------------------+---------------------------------------+
   | ``sub_package_path`` | the path of the directory (starting   |
   |                      | with ``_platform_specific``) that     |
   |                      | contains the files for the designated |
   |                      | NaCl architecture                     |
   +----------------------+---------------------------------------+

   Here is a sample ``manifest.json`` file:

   .. naclcode::
     :prettyprint: 0

     {
       "name": "My Reminder App",
       "description": "A reminder app that syncs across Chrome browsers.",
       "manifest_version": 2,
       "minimum_chrome_version": "28",
       "offline_enabled": true,
       "version": "0.3",
       "permissions": [
         {"fileSystem": ["write"]},
         "alarms",
         "storage"
       ],
       "app": {
         "background": {
           "scripts": ["scripts/background.js"]
         }
       },
       "icons": {
         "16": "images/icon-16x16.png",
         "128": "images/icon-128x128.png"
       },
       "platforms": [
         {
           "nacl_arch": "x86-64",
           "sub_package_path": "_platform_specific/x86-64/"
         },
         {
           "nacl_arch": "x86-32",
           "sub_package_path": "_platform_specific/x86-32/"
         },
         {
           "nacl_arch": "arm",
           "sub_package_path": "_platform_specific/arm/"
         },
         {
           "sub_package_path": "_platform_specific/all/"
         }
       ]
     }

   Note the last entry in the CWS manifest file above, which specifies a
   ``sub_package_path`` without a corresponding ``nacl_arch``. This entry
   identifies the fallback directory, which is included in the download
   package if the user architecture does not match any of the listed NaCl
   architectures, or if the user is using an older version of Chrome that
   does not support multi-platform zip files.

#. Modify your application as necessary so that it uses the files for the
   correct user architecture.

   To reference architecture-specific files, use the JavaScript API
   `chrome.runtime.getPlatformInfo() </extensions/runtime.html#method-getPlatformInfo>`_.
   As an example, if you have architecture-specific files in the directories
   ``x86-64``, ``x86-32``, and ``arm``, you can use the following JavaScript
   code to create a path for the files:

   .. naclcode::

      function getPath(name) {
        return '_platform_specific/' +
          chrome.runtime.getPlatformInfo().nacl_arch +
          '/' + name;
      }

#. Test your app, create a zip file, and upload the app to the CWS as before.

.. _additional_considerations_packaged:

Additional considerations for a Chrome app
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* In the description of your application in the CWS, make sure to mention that
  your application is a Native Client application that only works with the
  Chrome browser. Also make sure to identify the minimum version of Chrome
  that your application requires.
* Hosted and packaged applications have a "launch" parameter in the CWS
  manifest. This parameter is present only in apps (not extensions), and it
  tells Google Chrome what to show when a user starts an installed app. For
  example:

  .. naclcode::
    :prettyprint: 0

    "launch": {
      "web_url": "http://mail.google.com/mail/"
    }

* If you want to write local data using the Pepper
  `FileIO </native-client/peppercpp/classpp_1_1_file_i_o>`_
  API, you must set the 'unlimitedStorage' permission in your Chrome Web
  Store manifest file, just as you would for a JavaScript application that
  uses the HTML5 File API.
* For Chrome apps, you can only use in-app purchases.
* You can place your application in the Google Web Store with access only to
  certain people for testing. See `Publishing to test accounts
  </webstore/publish>`_ for more information.

Extension
---------

The NaCl-specific notes for a :ref:`package application <distributing_packaged>`
apply to extensions as well.

Hosted application
------------------

The .html file, .nmf file (Native Client manifest file), and .nexe files must
be served from the same domain, and the Chrome Web Store manifest file must
specify the correct, verified domain. Other files can be served from the same
or another domain.

In addition, see :ref:`Additional considerations for a Chrome apps
<additional_considerations_packaged>`.

Registering Native Client modules to handle MIME types
------------------------------------------------------

If you want Chrome to use a Native Client module to display a particular type
of content, you can associate the MIME type of that content with the Native
Client module. Use the ``nacl_modules`` attribute in the Chrome Web Store
manifest file to register a Native Client module as the handler for one or more
specific MIME types. For example, the bold code in the snippet below registers
a Native Client module as the content handler for the OpenOffice spreadsheet
MIME type:

.. naclcode::
  :prettyprint: 0

  {
     "name": "My Native Client Spreadsheet Viewer",
     "version": "0.1",
     "description": "Open spreadsheets right in your browser.",
     "nacl_modules": [{
        "path": "SpreadsheetViewer.nmf",
        "mime_type": "application/vnd.oasis.opendocument.spreadsheet"
     }]
  }

The value of "path" is the location of a Native Client manifest file (.nmf)
within the application directory. For more information on Native Client
manifest files, see :ref:`Manifest Files <manifest_file>`.

The value of "mime_type" is a specific MIME type that you want the Native
Client module to handle. Each MIME type can be associated with only one .nmf
file, but a single .nmf file might handle multiple MIME types. The following
example shows an extension with two .nmf files that handle three MIME types.

.. naclcode::
  :prettyprint: 0

  {
     "name": "My Native Client Spreadsheet and Document Viewer",
     "version": "0.1",
     "description": "Open spreadsheets and documents right in your browser.",
     "nacl_modules": [{
       "path": "SpreadsheetViewer.nmf",
       "mime_type": "application/vnd.oasis.opendocument.spreadsheet"
     },
     {
        "path": "SpreadsheetViewer.nmf",
        "mime_type": "application/vnd.oasis.opendocument.spreadsheet-template"
     },
     {
        "path": "DocumentViewer.nmf",
        "mime_type": "application/vnd.oasis.opendocument.text"
     }]
  }

The ``nacl_modules`` attribute is optional---specify this attribute only if
you want Chrome to use a Native Client module to display a particular type of
content.

