import six

def application(environ, start_response):
    start_response('200 OK', [('Content-type', 'text/plain')])
    body = 'user: %s' % environ.get('app.user')
    if six.PY3:
        body = body.encode('ascii')
    return [body]
