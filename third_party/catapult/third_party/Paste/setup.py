# Procedure to release a new version:
#
# - run tests: run tox
# - update version in setup.py (__version__)
# - update tag_build in setup.cfg
# - check that "python setup.py sdist" contains all files tracked by
#   the SCM (Mercurial): update MANIFEST.in if needed
# - update changelog: docs/news.txt
#
# - hg ci
# - hg tag VERSION
# - hg push
# - python2 setup.py register sdist bdist_wheel upload
# - python3 setup.py bdist_wheel upload
#
# - increment version in setup.py (__version__)
# - hg ci && hg push

__version__ = '2.0.2'

from setuptools import setup, find_packages
import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__),
                                'paste', 'util'))
import finddata

with open("README.rst") as fp:
    README = fp.read()

setup(name="Paste",
      version=__version__,
      description="Tools for using a Web Server Gateway Interface stack",
      long_description=README,
      classifiers=[
        "Development Status :: 5 - Production/Stable",
        "Intended Audience :: Developers",
        "License :: OSI Approved :: MIT License",
        "Programming Language :: Python",
        "Programming Language :: Python :: 3",
        "Topic :: Internet :: WWW/HTTP",
        "Topic :: Internet :: WWW/HTTP :: Dynamic Content",
        "Topic :: Software Development :: Libraries :: Python Modules",
        "Topic :: Internet :: WWW/HTTP :: WSGI",
        "Topic :: Internet :: WWW/HTTP :: WSGI :: Application",
        "Topic :: Internet :: WWW/HTTP :: WSGI :: Middleware",
        "Topic :: Internet :: WWW/HTTP :: WSGI :: Server",
        "Framework :: Paste",
        ],
      keywords='web application server wsgi',
      author="Ian Bicking",
      author_email="ianb@colorstudy.com",
      url="http://pythonpaste.org",
      license="MIT",
      packages=find_packages(exclude=['ez_setup', 'examples', 'packages', 'tests*']),
      package_data=finddata.find_package_data(
          exclude_directories=finddata.standard_exclude_directories + ('tests',)),
      namespace_packages=['paste'],
      zip_safe=False,
      test_suite='nose.collector',
      install_requires=['six'],
      tests_require=['nose>=0.11'],
      extras_require={
        'subprocess': [],
        'hotshot': [],
        'Flup': ['flup'],
        'Paste': [],
        'openid': ['python-openid'],
        },
      entry_points="""
      [paste.app_factory]
      cgi = paste.cgiapp:make_cgi_application [subprocess]
      static = paste.urlparser:make_static
      pkg_resources = paste.urlparser:make_pkg_resources
      urlparser = paste.urlparser:make_url_parser
      proxy = paste.proxy:make_proxy
      test = paste.debug.debugapp:make_test_app
      test_slow = paste.debug.debugapp:make_slow_app
      transparent_proxy = paste.proxy:make_transparent_proxy
      watch_threads = paste.debug.watchthreads:make_watch_threads

      [paste.composite_factory]
      urlmap = paste.urlmap:urlmap_factory
      cascade = paste.cascade:make_cascade

      [paste.filter_app_factory]
      error_catcher = paste.exceptions.errormiddleware:make_error_middleware
      cgitb = paste.cgitb_catcher:make_cgitb_middleware
      flup_session = paste.flup_session:make_session_middleware [Flup]
      gzip = paste.gzipper:make_gzip_middleware
      httpexceptions = paste.httpexceptions:make_middleware
      lint = paste.lint:make_middleware
      printdebug = paste.debug.prints:PrintDebugMiddleware
      profile = paste.debug.profile:make_profile_middleware [hotshot]
      recursive = paste.recursive:make_recursive_middleware
      # This isn't good enough to deserve the name egg:Paste#session:
      paste_session = paste.session:make_session_middleware
      wdg_validate = paste.debug.wdg_validate:make_wdg_validate_middleware [subprocess]
      evalerror = paste.evalexception.middleware:make_eval_exception
      auth_tkt = paste.auth.auth_tkt:make_auth_tkt_middleware
      auth_basic = paste.auth.basic:make_basic
      auth_digest = paste.auth.digest:make_digest
      auth_form = paste.auth.form:make_form
      grantip = paste.auth.grantip:make_grantip
      openid = paste.auth.open_id:make_open_id_middleware [openid]
      pony = paste.pony:make_pony
      cowbell = paste.cowbell:make_cowbell
      errordocument = paste.errordocument:make_errordocument
      auth_cookie = paste.auth.cookie:make_auth_cookie
      translogger = paste.translogger:make_filter
      config = paste.config:make_config_filter
      registry = paste.registry:make_registry_manager

      [paste.server_runner]
      http = paste.httpserver:server_runner
      """,
      )
