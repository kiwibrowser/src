from paste import request

def urlparser_hook(environ):
    first, rest = request.path_info_split(environ.get('PATH_INFO', ''))
    if not first:
        # No username
        return
    environ['app.user'] = first
    environ['SCRIPT_NAME'] += '/' + first
    environ['PATH_INFO'] = rest
