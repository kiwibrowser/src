``webob`` README
================

.. image:: https://pypip.in/version/WebOb/badge.svg?style=flat
    :target: https://pypi.python.org/pypi/WebOb/
    :alt: Latest Version

.. image:: https://travis-ci.org/Pylons/webob.png?branch=master
        :target: https://travis-ci.org/Pylons/webob

.. image:: https://readthedocs.org/projects/webob/badge/?version=latest
        :target: http://docs.pylonsproject.org/projects/webob/en/latest/
        :alt: Documentation Status

WebOb provides objects for HTTP requests and responses.  Specifically
it does this by wrapping the `WSGI <http://wsgi.org>`_ request
environment and response status/headers/app_iter(body).

The request and response objects provide many conveniences for parsing
HTTP request and forming HTTP responses.  Both objects are read/write:
as a result, WebOb is also a nice way to create HTTP requests and
parse HTTP responses.

Please see the `full documentation <http://webob.rtfd.org/>`_ for details.
