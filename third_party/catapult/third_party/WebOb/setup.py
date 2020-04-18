from setuptools import setup

version = '1.5.0a0'

testing_extras = ['nose', 'coverage']

docs_extras = ['Sphinx']

setup(
    name='WebOb',
    version=version,
    description="WSGI request and response object",
    long_description="""\
WebOb provides wrappers around the WSGI request environment, and an
object to help create WSGI responses.

The objects map much of the specified behavior of HTTP, including
header parsing and accessors for other standard parts of the
environment.

You may install the `in-development version of WebOb
<https://github.com/Pylons/webob/zipball/master#egg=WebOb-dev>`_ with
``pip install WebOb==dev`` (or ``easy_install WebOb==dev``).

* `WebOb reference <http://docs.webob.org/en/latest/reference.html>`_
* `Bug tracker <https://github.com/Pylons/webob/issues>`_
* `Browse source code <https://github.com/Pylons/webob>`_
* `Mailing list <http://bit.ly/paste-users>`_
* `Release news <http://docs.webob.org/en/latest/news.html>`_
* `Detailed changelog <https://github.com/Pylons/webob/commits/master>`_
""",
    classifiers=[
        "Development Status :: 6 - Mature",
        "Intended Audience :: Developers",
        "License :: OSI Approved :: MIT License",
        "Topic :: Internet :: WWW/HTTP :: WSGI",
        "Topic :: Internet :: WWW/HTTP :: WSGI :: Application",
        "Topic :: Internet :: WWW/HTTP :: WSGI :: Middleware",
        "Programming Language :: Python :: 2.6",
        "Programming Language :: Python :: 2.7",
        "Programming Language :: Python :: 3.2",
        "Programming Language :: Python :: 3.3",
        "Programming Language :: Python :: 3.4",
        "Programming Language :: Python :: Implementation :: CPython",
        "Programming Language :: Python :: Implementation :: PyPy",
    ],
    keywords='wsgi request web http',
    author='Ian Bicking',
    author_email='ianb@colorstudy.com',
    maintainer='Pylons Project',
    url='http://webob.org/',
    license='MIT',
    packages=['webob'],
    zip_safe=True,
    test_suite='nose.collector',
    tests_require=['nose'],
    extras_require = {
        'testing':testing_extras,
        'docs':docs_extras,
        },
)
