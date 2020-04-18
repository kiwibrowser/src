import email

from paste.httpserver import WSGIHandler
from six.moves import StringIO


class MockServer(object):
    server_address = ('127.0.0.1', 80)


class MockSocket(object):
    def makefile(self, mode, bufsize):
        return StringIO()


def test_environ():
    mock_socket = MockSocket()
    mock_client_address = '1.2.3.4'
    mock_server = MockServer()

    wsgi_handler = WSGIHandler(mock_socket, mock_client_address, mock_server)
    wsgi_handler.command = 'GET'
    wsgi_handler.path = '/path'
    wsgi_handler.request_version = 'HTTP/1.0'
    wsgi_handler.headers = email.message_from_string('Host: mywebsite')

    wsgi_handler.wsgi_setup()

    assert wsgi_handler.wsgi_environ['HTTP_HOST'] == 'mywebsite'


def test_environ_with_multiple_values():
    mock_socket = MockSocket()
    mock_client_address = '1.2.3.4'
    mock_server = MockServer()

    wsgi_handler = WSGIHandler(mock_socket, mock_client_address, mock_server)
    wsgi_handler.command = 'GET'
    wsgi_handler.path = '/path'
    wsgi_handler.request_version = 'HTTP/1.0'
    wsgi_handler.headers = email.message_from_string('Host: host1\nHost: host2')

    wsgi_handler.wsgi_setup()

    assert wsgi_handler.wsgi_environ['HTTP_HOST'] == 'host1,host2'
