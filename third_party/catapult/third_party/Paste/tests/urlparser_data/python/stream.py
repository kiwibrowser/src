def stream():
    def app(environ, start_response):
        writer = start_response('200 OK', [('Content-type', 'text/html')])
        writer(b'te')
        writer(b'st')
        return [b'2']
    return app
