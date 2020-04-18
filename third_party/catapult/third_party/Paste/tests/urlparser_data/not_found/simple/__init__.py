def not_found_hook(environ, start_response):
    start_response('200 OK', [('Content-type', 'text/plain')])
    return [b'not found']
