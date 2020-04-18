from paste import request

def not_found_hook(environ, start_response):
    urlparser = environ['paste.urlparser.not_found_parser']
    first, rest = request.path_info_split(environ.get('PATH_INFO', ''))
    if not first:
        # No username
        return
    environ['app.user'] = first
    environ['SCRIPT_NAME'] += '/' + first
    environ['PATH_INFO'] = rest
    return urlparser(environ, start_response)
