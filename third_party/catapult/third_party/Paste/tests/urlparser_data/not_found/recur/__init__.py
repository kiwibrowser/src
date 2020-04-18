def not_found_hook(environ, start_response):
    urlparser = environ['paste.urlparser.not_found_parser']
    path = environ.get('PATH_INFO', '')
    if not path:
        return urlparser.not_found(environ, start_response)
    # Strip off leading _'s
    path = '/' + path.lstrip('/').lstrip('_')
    environ['PATH_INFO'] = path
    return urlparser(environ, start_response)
