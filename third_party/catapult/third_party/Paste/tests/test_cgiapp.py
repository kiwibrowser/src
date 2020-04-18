import os
import sys
from nose.tools import assert_raises
from paste.cgiapp import CGIApplication, CGIError
from paste.fixture import *

data_dir = os.path.join(os.path.dirname(__file__), 'cgiapp_data')

# these CGI scripts can't work on Windows or Jython
if sys.platform != 'win32' and not sys.platform.startswith('java'):
    def test_ok():
        app = TestApp(CGIApplication({}, script='ok.cgi', path=[data_dir]))
        res = app.get('')
        assert res.header('content-type') == 'text/html; charset=UTF-8'
        assert res.full_status == '200 Okay'
        assert 'This is the body' in res

    def test_form():
        app = TestApp(CGIApplication({}, script='form.cgi', path=[data_dir]))
        res = app.post('', params={'name': b'joe'},
                       upload_files=[('up', 'file.txt', b'x'*10000)])
        assert 'file.txt' in res
        assert 'joe' in res
        assert 'x'*10000 in res

    def test_error():
        app = TestApp(CGIApplication({}, script='error.cgi', path=[data_dir]))
        assert_raises(CGIError, app.get, '', status=500)

    def test_stderr():
        app = TestApp(CGIApplication({}, script='stderr.cgi', path=[data_dir]))
        res = app.get('', expect_errors=True)
        assert res.status == 500
        assert 'error' in res
        assert b'some data' in res.errors

