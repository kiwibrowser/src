Paste provides several pieces of "middleware" (or filters) that can be nested
to build web applications.  Each piece of middleware uses the WSGI (`PEP 333`_)
interface, and should be compatible with other middleware based on those
interfaces.

.. _PEP 333: http://www.python.org/dev/peps/pep-0333.html

* `Paste project at Bitbucket (source code, bug tracker)
  <https://bitbucket.org/ianb/paste/>`_
* `Paste on the Python Cheeseshop (PyPI)
  <https://pypi.python.org/pypi/Paste>`_
* `Paste documentation
  <http://pythonpaste.org/>`_

See also:

* `PasteDeploy <http://pythonpaste.org/deploy/>`_
* `PasteScript <http://pythonpaste.org/script/>`_
* `WebTest <http://webtest.pythonpaste.org/>`_
* `WebOb <http://docs.webob.org/>`_

Includes these features...

Testing
-------

* A fixture for testing WSGI applications conveniently and in-process,
  in ``paste.fixture``

* A fixture for testing command-line applications, also in
  ``paste.fixture``

* Check components for WSGI-compliance in ``paste.lint``

Dispatching
-----------

* Chain and cascade WSGI applications (returning the first non-error
  response) in ``paste.cascade``

* Dispatch to several WSGI applications based on URL prefixes, in
  ``paste.urlmap``

* Allow applications to make subrequests and forward requests
  internally, in ``paste.recursive``

Web Application
---------------

* Run CGI programs as WSGI applications in ``paste.cgiapp``

* Traverse files and load WSGI applications from ``.py`` files (or
  static files), in ``paste.urlparser``

* Serve static directories of files, also in ``paste.urlparser``; also
  in that module serving from Egg resources using ``pkg_resources``.

Tools
-----

* Catch HTTP-related exceptions (e.g., ``HTTPNotFound``) and turn them
  into proper responses in ``paste.httpexceptions``

* Several authentication techniques, including HTTP (Basic and
  Digest), signed cookies, and CAS single-signon, in the
  ``paste.auth`` package.

* Create sessions in ``paste.session`` and ``paste.flup_session``

* Gzip responses in ``paste.gzip``

* A wide variety of routines for manipulating WSGI requests and
  producing responses, in ``paste.request``, ``paste.response`` and
  ``paste.wsgilib``

Debugging Filters
-----------------

* Catch (optionally email) errors with extended tracebacks (using
  Zope/ZPT conventions) in ``paste.exceptions``

* Catch errors presenting a `cgitb
  <http://docs.python.org/2/library/cgitb.html>`_-based
  output, in ``paste.cgitb_catcher``.

* Profile each request and append profiling information to the HTML,
  in ``paste.debug.profile``

* Capture ``print`` output and present it in the browser for
  debugging, in ``paste.debug.prints``

* Validate all HTML output from applications using the `WDG Validator
  <http://www.htmlhelp.com/tools/validator/>`_, appending any errors
  or warnings to the page, in ``paste.debug.wdg_validator``

Other Tools
-----------

* A file monitor to allow restarting the server when files have been
  updated (for automatic restarting when editing code) in
  ``paste.reloader``

* A class for generating and traversing URLs, and creating associated
  HTML code, in ``paste.url``

The official development repo is at https://bitbucket.org/ianb/paste.

For the latest changes see the `news file
<http://pythonpaste.org/news.html>`_.
