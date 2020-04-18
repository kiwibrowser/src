.. _devcycle-running:

.. include:: /migration/deprecation.inc

#######
Running
#######


.. contents::
  :local:
  :backlinks: none
  :depth: 2

Introduction
============

This document describes how to run Native Client applications during
development.

The workflow for PNaCl applications is straightfoward and will only be discussed
briefly. For NaCl applications distributed through the web-store, there is a
number of options and these will be discussed more in-depth.

Portable Native Client (PNaCl) applications
===========================================

Running PNaCl applications from the open web is enabled in Chrome version 31 and
above; therefore, no special provisions are required to run and test such
applications locally. An application that uses a PNaCl module can be tested
similarly to any other web application that only consists of HTML, CSS and
JavaScript.

To better simulate a production environment, it's recommended to start a local
web server to serve the application's files. The NaCl SDK comes with a simple
local server built in, and the process of using it to run PNaCl applications is
described in :ref:`the tutorial <tutorial_step_2>`.

Native Client applications and the Chrome Web Store
===================================================

Before reading about how to run Native Client applications, it's important to
understand a little bit about how Native Client applications are distributed.
As explained in :doc:`Distributing Your Application <../distributing>`, Native
Client applications must currently be distributed through the **Chrome Web
Store (CWS)**. Applications in the CWS are one of three types:

* A **hosted application** is an application that you host on a server of your
  choice. To distribute an application as a hosted application, you upload
  application metadata to the CWS. Learn more on the `Chrome App </apps>`_
  documentation page.

* A **packaged application** is an application that is hosted in the CWS and
  downloaded to the user's machine. To distribute an application as a packaged
  application, you upload the entire application, including all application
  assets and metadata, to the CWS. Learn more on the `Chrome App </apps>`_
  documentation page.

* An **extension** is a packaged application that has a tiny UI component
  (extensions are typically used to extend the functionality of the Chrome
  browser). To distribute an application as an extension, you upload the entire
  application, including all application assets and metadata, to the CWS. Learn
  more on the `Chrome extensions </extensions>`_ documentation page.

The web store documentation contains a handy guide to `help you choose which to
use <https://developer.chrome.com/webstore/choosing>`_.

It's clearly not convenient to package and upload files to the Chrome Web Store
every time you want to run a new build of your application, but there are four
alternative techniques you can use to run the application during development.
These techniques are listed in the following table and described in detail
below. Each technique has certain requirements (NaCl flag, web server, and/or
CWS metadata); these are explained in the :ref:`Requirements <requirements>`
section below.


+--------------------------------------------------------+----------+----------+
| Technique                                              | Requires | Requires |
|                                                        | Web      | CWS      |
|                                                        | Server   | Metadata |
+========================================================+==========+==========+
|**1. Local server**                                     | |CHK|    |          |
|                                                        |          |          |
| ..                                                     |          |          |
|                                                        |          |          |
|  Run a local server and simply point your browser to   |          |          |
|  your application on the server.                       |          |          |
|                                                        |          |          |
|  .. Note::                                             |          |          |
|    :class: note                                        |          |          |
|                                                        |          |          |
|    This technique requires the NaCl flag.              |          |          |
+---------------------------------------------+----------+----------+----------+
|**2. Packaged application loaded as an unpacked         |          | |CHK|    |
|extension**                                             |          |          |
|                                                        |          |          |
| ..                                                     |          |          |
|                                                        |          |          |
|  Load your packaged application into Chrome as an      |          |          |
|  unpacked extension and run it without a server. An    |          |          |
|  unpacked extension is an application whose source and |          |          |
|  metadata files are located in an unzipped folder on   |          |          |
|  your development machine. The CWS manifest file       |          |          |
|  (explained below) must specify a local_path field.    |          |          |
+--------------------------------------------------------+----------+----------+
|**3. Hosted application loaded as an unpacked           | |CHK|    | |CHK|    |
|extension**                                             |          |          |
|                                                        |          |          |
| ..                                                     |          |          |
|                                                        |          |          |
|  Load your hosted application into Chrome as an        |          |          |
|  unpacked extension and run it from a server (which can|          |          |
|  be a local server). The CWS manifest file must specify|          |          |
|  a web_url field.                                      |          |          |
+--------------------------------------------------------+----------+----------+
|**4. CWS application with untrusted testers**           |          | |CHK|    |
|                                                        |          |          |
| ..                                                     |          |          |
|                                                        |          |          |
|  The standard technique for distributing a packaged or |          |          |
|  hosted application in the CWS. You can limit the      |          |          |
|  application to trusted testers. This technique        |          |          |
|  requires a server if your application is a hosted     |          |          |
|  application.                                          |          |          |
+--------------------------------------------------------+----------+----------+


.. |CHK| image:: /images/check-red.png

Which of the above techniques you use to run your application during development
is largely a matter of personal preference (i.e., would you rather start a local
server or create CWS metadata?). As a general rule, once you have an idea of how
you plan to distribute your application, you should use the corresponding
technique during development. Choosing a distribution option depends on a number
of factors such as application size, application start-up time, hosting costs,
offline functionality, etc. (see :doc:`Distributing Your Application
<../distributing>` for details), but you don't need to make a decision about how
to distribute your application at the outset.

The next two sections of this document describe a couple of prerequisites for
running applications during development, and explain the three requirements
listed in the table above (NaCl flag, web server, and CWS metadata). The
subsequent sections of the document provide instructions for how to use each of
the four techniques.

Prerequisites
=============

Browser and Pepper versions
---------------------------

Before you run a new build of your application, make sure that you're using the
correct version of Chrome. Each version of Chrome supports a corresponding
version of the Pepper API. You (and your users) must use a version of Chrome
that is equal to or higher than the version of the Pepper API that your
application uses. For example, if you compiled your application using the
``pepper_37`` bundle, your application uses the Pepper 37 API, and you must run
the application in Chrome 37 or higher. To check which version of Chrome you're
using, type ``about:version`` in the Chrome address bar.

.. _cache:

Chrome Cache
------------

Chrome caches resources aggressively. You should disable Chrome's cache whenever
you are developing a Native Client application in order to make sure Chrome
loads new versions of your application. Follow the instructions :ref:`in the
tutorial <tutorial_step_3>`.

.. _requirements:

Requirements
============

.. _flag:

Native Client flag
------------------

Native Client is automatically enabled for applications that are installed from
the Chrome Web Store. To enable Native Client for applications that are not
installed from the Chrome Web Store, you must explicitly turn on the Native
Client flag in Chrome as follows:

#. Type ``about:flags`` in the Chrome address bar.
#. Scroll down to "Native Client".
#. If the link below "Native Client" says "Disable", then Native Client is
   already enabled and you don't need to do anything else.
#. If the link below "Native Client" says "Enable":

   * Click the "Enable" link.
   * Click the "Relaunch Now" button in the bottom of the screen. **Native
     Client will not be enabled until you relaunch your browser**. All browser
     windows will restart when you relaunch Chrome.

If you enable the Native Client flag and still can't run applications from
outside the Chrome Web Store, you may need to enable the Native Client plugin:

#. Type ``about:plugins`` in the Chrome address bar.
#. Scroll down to "Native Client".
#. If the link below "Native Client" says "Enable", click the link to enable
   the Native Client plugin. You do not need to relaunch Chrome after enabling
   the Native Client plugin.

.. _web_server:

Web server
----------

For security reasons, Native Client applications must come from a server (you
can't simply drag HTML files into your browser). The Native Client SDK comes
with a lightweight Python web server that you can run to serve your application
locally. The server can be invoked from a Makefile. Here is how to run the
server:

.. naclcode::
  :prettyprint: 0

  $ cd examples
  $ make serve

By default, the server listens for requests on port 5103. You can use the server
to run most applications under the ``examples`` directory where you started the
server. For example, to run the ``flock`` example in the SDK, start the server
and point your browser to ``http://localhost:5103/demo/flock/``.

Some of the applications need special flags to Chrome, and must be run with the
``make run`` command. See :ref:`running_the_sdk_examples` for more details.

.. _metadata:

Chrome Web Store metadata
~~~~~~~~~~~~~~~~~~~~~~~~~

Applications published in the Chrome Web Store must be accompanied by CWS
metadata; specifically, a Chrome Web Store manifest file named
``manifest.json``, and at least one icon.

Below is an example of a CWS manifest file for a **hosted application**:

.. naclcode::

  {
    "name": "My NaCl App",
    "description": "Simple game implemented using Native Client",
    "version": "0.1",
    "icons": {
      "128": "icon128.png"
    },
    "app": {
      "urls": [
        "http://mysubdomain.example.com/"
      ],
      "launch": {
        "web_url": "http://mysubdomain.example.com/my_app_main_page.html"
      }
    }
  }


For a **packaged application**, you can omit the urls field, and replace the
``web_url`` field with a ``local_path`` field, as shown below:

.. naclcode::

  {
    "name": "My NaCl App",
    "description": "Simple game implemented using Native Client",
    "version": "0.1",
    "icons": {
      "16": "icon16.png",
      "128": "icon128.png"
    },
    "app": {
      "launch": {
        "local_path": "my_app_main_page.html"
      }
    }
  }

You must put the ``manifest.json`` file in the same directory as your
application's main HTML page.

If you don't have icons for your application, you can use the following icons as
placeholders:

|ICON16|

|ICON128|

.. |ICON16| image:: /images/icon16.png
.. |ICON128| image:: /images/icon128.png

Put the icons in the same directory as the CWS manifest file. For more
information about CWS manifest files and application icons, see:

* `Chrome Web Store Tutorial: Getting Started </webstore/get_started_simple>`_
* `Chrome Web Store Formats: Manifest Files </extensions/manifest>`_

Technique 1: Local server
=========================

To run your application from a local server:

* Enable the :ref:`Native Client flag <flag>` in Chrome.
* Start a :ref:`local web server <web_server>`.
* Put your application under the examples directory in the SDK bundle you are
  using (for example, in the directory ``pepper_35/examples/my_app``).
* Access your application on the local server by typing the location of its
  HTML file in Chrome, for example:
  ``http://localhost:5103/my_app/my_app_main_page.html``.

.. Note::
  :class: note

  **Note:** You don't have to use a local web server---you can use another
  server if you already have one running. You must still enable the Native
  Client flag in order to run your application from the server.

Technique 2: Packaged application loaded as an unpacked extension
=================================================================

For development purposes, Chrome lets you load a packaged application as an
unpacked extension. To load and run your packaged application as an unpacked
extension:

#. Create a Chrome Web Store manifest file and one or more icons for your
   application.

   * Follow the instructions above under Chrome Web Store metadata to create
     these files.
   * Note that the CWS manifest file should contain the ``local_path`` field
     rather than the ``web_url`` field.
#. Put the CWS manifest file and the application icon(s) in the same directory
   as your application's main HTML page.
#. Load the application as an unpacked extension in Chrome:

   * Bring up the extensions management page in Chrome by clicking the menu
     icon |menu-icon| and choosing **Tools > Extensions**.
   * Check the box for **Developer mode** and then click the **Load unpacked
     extension** button:
     |extensions|
   * In the file dialog that appears, select your application directory. Unless
     you get an error dialog, you've now installed your app in Chrome.
#. Open a new tab in Chrome and click the **Apps** link at the bottom of the
   page to show your installed apps:
   |new-tab-apps|
#. The icon for your newly installed app should appear on the New Tab page.
   Click the icon to launch the app.

For additional information about how to create CWS metadata and load your
application into Chrome (including troubleshooting information), see the
`Chrome Web Store Tutorial: Getting Started </webstore/get_started_simple>`_.

See also :ref:`run_sdk_examples_as_packaged`.

Technique 3: Hosted application loaded as an unpacked extension
===============================================================

For development purposes, Chrome lets you load a hosted application as an
unpacked extension. To load and run your hosted application as an unpacked
extension:

#. Start a web server to serve your application.

   * You can use the :ref:`local web server <web_server>` included with the
     Native Client SDK if you want.
#. Upload your application (.html, .nmf, .nexe, .css, .js, image files, etc.)
   to the server.

   * If you're using the local server included with the Native Client SDK,
     simply put your application under the ``examples`` directory in the SDK
     bundle you are using (e.g., in the directory
     ``pepper_37/examples/my_app``).
#. Create a Chrome Web Store manifest file and one or more icons for your
   application.

   * Follow the instructions above under :ref:`Chrome Web Store metadata
     <metadata>` to create these files.
   * In the CWS manifest file, the ``web_url`` field should specify the
     location of your application on your server. If you're using the local
     server included with the SDK, the ``web_url`` field should look something
     like ``http://localhost:5103/my_app/my_app_main_page.html``.
#. Put the CWS manifest file and the application icon(s) in the same directory
   as your application's main HTML page.
#. Load the application as an unpacked extension in Chrome:

   * Bring up the extensions management page in Chrome by clicking the menu
     icon |menu-icon| and choosing **Tools > Extensions**.
   * Check the box for **Developer mode** and then click the **Load unpacked
     extension** button:
     |extensions|
   * In the file dialog that appears, select your application directory. Unless
     you get an error dialog, you've now installed your app in Chrome.
#. Open a new tab in Chrome and click the **Apps** link at the bottom of the
   page to show your installed apps:
   |new-tab-apps|
#. The icon for your newly installed app should appear on the New Tab page.
   Click the icon to launch the app.

For additional information about how to create CWS metadata and load your
application into Chrome (including troubleshooting information), see the
`Chrome Web Store Tutorial: Getting Started </webstore/get_started_simple>`_.

Technique 4: Chrome Web Store application with trusted testers
==============================================================

When you're ready to test your application more broadly, you can upload the
application to the Chrome Web Store and let some trusted testers run it. Here
is how to do so:

#. Create the Chrome Web Store metadata required to publish your application:

   * First, create a Chrome Web Store manifest file and one or more icons for
     your application, as described above under :ref:`Chrome Web Store metadata
     <metadata>`. Note that packaged applications must have at least two icons
     (a 16x16 icon and a 128x128 icon).
   * You also need to create the following additional assets before you can
     publish your application:

     * a screenshot (size must be 640x400 or 1280x800)
     * a promotional image called a "small tile" (size must be 440x280)

#. For a **packaged application**:

   * Create a zip file with the CWS manifest file, the application icons, and
     all your application files (.html, .nmf, .nexe, .css, .js, image files,
     etc.)

#. For a **hosted application**:

   * Create a zip file with the CWS manifest file and the application icon(s).
   * Upload the application files (.html, .nmf, .nexe, .css, .js, image files,
     etc.) to the server on which the application is being hosted.
   * Use `Google Webmaster Tools <http://www.google.com/webmasters/tools/>`_ to
     verify ownership of the website on which the application runs.

#. Log in to the `Chrome Web Store Developer Dashboard
   <https://chrome.google.com/webstore/developer/dashboard>`_.

   * The first time you log in, click the "Add new item" button to display the
     Google Chrome Web Store Developer Agreement. Review and accept the
     agreement and then return to the `Developer Dashboard
     <https://chrome.google.com/webstore/developer/dashboard>`_.

#. Click "Edit your tester accounts" at the bottom of the Developer Dashboard.
#. Enter a series of email addresses for your testers (separated by commas or
   whitespace), and click the "Save Changes" button.
#. Click the "Add new item" button to add your application to the Chrome Web
   Store.
#. Click the "Choose file" button and select the zip file you created earlier.
#. Click the "Upload" button; this uploads your zip file and opens the "Edit
   item" page.
#. Edit the following required fields on the "Edit item" page:

   * Upload an application icon.
   * Upload a screenshot.
   * Upload a small tile.
   * Select a category for your application (accounting application, action
     game, etc.).
   * Select a language for your application.
#. If you are an owner or manager of a Google Group, you can select that group
   in the "Trusted testers" field.

   * You may want to create a Google Group specifically for your testers. When
     you add a group to the "Trusted testers" field, all group members will be
     able to test the application, in addition to the individuals you added to
     the "trusted tester accounts" field on the Developer Dashboard.
#. Click the "Publish to test accounts" button at the bottom of the page and
   click "OK".
#. A page comes up that shows your application's listing in the Chrome Web
   Store. Copy the URL and mail it to your trusted testers.

   * When you publish an application to test accounts, the application's CWS
     listing is visible only to you and to people who are logged into those
     accounts. Your application won't appear in search results, so you need to
     give testers a direct link to your application's CWS listing. Users won't
     be able to find the application by searching in the CWS.

To publish an application to the world after publishing it to test accounts,
you must first unpublish the application. For additional information see
`Publishing Your App </webstore/docs/publish>`_, and in particular `Publishing
to test accounts </webstore/publish#testaccounts>`_.

.. |menu-icon| image:: /images/menu-icon.png
.. |extensions| image:: /images/extensions-management.png
.. |new-tab-apps| image:: /images/new-tab-apps.png
