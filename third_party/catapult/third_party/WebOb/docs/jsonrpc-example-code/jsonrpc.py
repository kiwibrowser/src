# A reaction to: http://aspn.activestate.com/ASPN/Cookbook/Python/Recipe/552751
from webob import Request, Response
from webob import exc
from simplejson import loads, dumps
import traceback
import sys

class JsonRpcApp(object):
    """
    Serve the given object via json-rpc (http://json-rpc.org/)
    """

    def __init__(self, obj):
        self.obj = obj

    def __call__(self, environ, start_response):
        req = Request(environ)
        try:
            resp = self.process(req)
        except ValueError, e:
            resp = exc.HTTPBadRequest(str(e))
        except exc.HTTPException, e:
            resp = e
        return resp(environ, start_response)

    def process(self, req):
        if not req.method == 'POST':
            raise exc.HTTPMethodNotAllowed(
                "Only POST allowed",
                allowed='POST')
        try:
            json = loads(req.body)
        except ValueError, e:
            raise ValueError('Bad JSON: %s' % e)
        try:
            method = json['method']
            params = json['params']
            id = json['id']
        except KeyError, e:
            raise ValueError(
                "JSON body missing parameter: %s" % e)
        if method.startswith('_'):
            raise exc.HTTPForbidden(
                "Bad method name %s: must not start with _" % method)
        if not isinstance(params, list):
            raise ValueError(
                "Bad params %r: must be a list" % params)
        try:
            method = getattr(self.obj, method)
        except AttributeError:
            raise ValueError(
                "No such method %s" % method)
        try:
            result = method(*params)
        except:
            text = traceback.format_exc()
            exc_value = sys.exc_info()[1]
            error_value = dict(
                name='JSONRPCError',
                code=100,
                message=str(exc_value),
                error=text)
            return Response(
                status=500,
                content_type='application/json',
                body=dumps(dict(result=None,
                                error=error_value,
                                id=id)))
        return Response(
            content_type='application/json',
            body=dumps(dict(result=result,
                            error=None,
                            id=id)))


class ServerProxy(object):
    """
    JSON proxy to a remote service.
    """

    def __init__(self, url, proxy=None):
        self._url = url
        if proxy is None:
            from wsgiproxy.exactproxy import proxy_exact_request
            proxy = proxy_exact_request
        self.proxy = proxy

    def __getattr__(self, name):
        if name.startswith('_'):
            raise AttributeError(name)
        return _Method(self, name)

    def __repr__(self):
        return '<%s for %s>' % (
            self.__class__.__name__, self._url)

class _Method(object):

    def __init__(self, parent, name):
        self.parent = parent
        self.name = name

    def __call__(self, *args):
        json = dict(method=self.name,
                    id=None,
                    params=list(args))
        req = Request.blank(self.parent._url)
        req.method = 'POST'
        req.content_type = 'application/json'
        req.body = dumps(json)
        resp = req.get_response(self.parent.proxy)
        if resp.status_code != 200 and not (
            resp.status_code == 500
            and resp.content_type == 'application/json'):
            raise ProxyError(
                "Error from JSON-RPC client %s: %s"
                % (self.parent._url, resp.status),
                resp)
        json = loads(resp.body)
        if json.get('error') is not None:
            e = Fault(
                json['error'].get('message'),
                json['error'].get('code'),
                json['error'].get('error'),
                resp)
            raise e
        return json['result']

class ProxyError(Exception):
    """
    Raised when a request via ServerProxy breaks
    """
    def __init__(self, message, response):
        Exception.__init__(self, message)
        self.response = response

class Fault(Exception):
    """
    Raised when there is a remote error
    """
    def __init__(self, message, code, error, response):
        Exception.__init__(self, message)
        self.code = code
        self.error = error
        self.response = response
    def __str__(self):
        return 'Method error calling %s: %s\n%s' % (
            self.response.request.url,
            self.args[0],
            self.error)

class DemoObject(object):
    """
    Something interesting to attach to
    """
    def add(self, *args):
        return sum(args)
    def average(self, *args):
        return sum(args) / float(len(args))
    def divide(self, a, b):
        return a / b

def make_app(expr):
    module, expression = expr.split(':', 1)
    __import__(module)
    module = sys.modules[module]
    obj = eval(expression, module.__dict__)
    return JsonRpcApp(obj)

def main(args=None):
    import optparse
    from wsgiref import simple_server
    parser = optparse.OptionParser(
        usage='%prog [OPTIONS] MODULE:EXPRESSION')
    parser.add_option(
        '-p', '--port', default='8080',
        help='Port to serve on (default 8080)')
    parser.add_option(
        '-H', '--host', default='127.0.0.1',
        help='Host to serve on (default localhost; 0.0.0.0 to make public)')
    options, args = parser.parse_args()
    if not args or len(args) > 1:
        print 'You must give a single object reference'
        parser.print_help()
        sys.exit(2)
    app = make_app(args[0])
    server = simple_server.make_server(options.host, int(options.port), app)
    print 'Serving on http://%s:%s' % (options.host, options.port)
    server.serve_forever()
    # Try python jsonrpc.py 'jsonrpc:DemoObject()'

if __name__ == '__main__':
    main()
