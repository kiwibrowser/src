# (c) 2005 Ben Bangert
# This module is part of the Python Paste Project and is released under
# the MIT License: http://www.opensource.org/licenses/mit-license.php
from nose.tools import assert_raises

from paste.fixture import *
from paste.registry import *
from paste.registry import Registry
from paste.evalexception.middleware import EvalException

regobj = StackedObjectProxy()
secondobj = StackedObjectProxy(default=dict(hi='people'))

def simpleapp(environ, start_response):
    status = '200 OK'
    response_headers = [('Content-type','text/plain')]
    start_response(status, response_headers)
    return [b'Hello world!\n']

def simpleapp_withregistry(environ, start_response):
    status = '200 OK'
    response_headers = [('Content-type','text/plain')]
    start_response(status, response_headers)
    body = 'Hello world!Value is %s\n' % regobj.keys()
    if six.PY3:
        body = body.encode('utf8')
    return [body]

def simpleapp_withregistry_default(environ, start_response):
    status = '200 OK'
    response_headers = [('Content-type','text/plain')]
    start_response(status, response_headers)
    body = 'Hello world!Value is %s\n' % secondobj
    if six.PY3:
        body = body.encode('utf8')
    return [body]


class RegistryUsingApp(object):
    def __init__(self, var, value, raise_exc=False):
        self.var = var
        self.value = value
        self.raise_exc = raise_exc

    def __call__(self, environ, start_response):
        if 'paste.registry' in environ:
            environ['paste.registry'].register(self.var, self.value)
        if self.raise_exc:
            raise self.raise_exc
        status = '200 OK'
        response_headers = [('Content-type','text/plain')]
        start_response(status, response_headers)
        body = 'Hello world!\nThe variable is %s' % str(regobj)
        if six.PY3:
            body = body.encode('utf8')
        return [body]

class RegistryUsingIteratorApp(object):
    def __init__(self, var, value):
        self.var = var
        self.value = value

    def __call__(self, environ, start_response):
        if 'paste.registry' in environ:
            environ['paste.registry'].register(self.var, self.value)
        status = '200 OK'
        response_headers = [('Content-type','text/plain')]
        start_response(status, response_headers)
        body = 'Hello world!\nThe variable is %s' % str(regobj)
        if six.PY3:
            body = body.encode('utf8')
        return iter([body])

class RegistryMiddleMan(object):
    def __init__(self, app, var, value, depth):
        self.app = app
        self.var = var
        self.value = value
        self.depth = depth

    def __call__(self, environ, start_response):
        if 'paste.registry' in environ:
            environ['paste.registry'].register(self.var, self.value)
        line = ('\nInserted by middleware!\nInsertValue at depth %s is %s'
                % (self.depth, str(regobj)))
        if six.PY3:
            line = line.encode('utf8')
        app_response = [line]
        app_iter = None
        app_iter = self.app(environ, start_response)
        if type(app_iter) in (list, tuple):
            app_response.extend(app_iter)
        else:
            response = []
            for line in app_iter:
                response.append(line)
            if hasattr(app_iter, 'close'):
                app_iter.close()
            app_response.extend(response)
        line = ('\nAppended by middleware!\nAppendValue at \
                depth %s is %s' % (self.depth, str(regobj)))
        if six.PY3:
            line = line.encode('utf8')
        app_response.append(line)
        return app_response


def test_simple():
    app = TestApp(simpleapp)
    response = app.get('/')
    assert 'Hello world' in response

def test_solo_registry():
    obj = {'hi':'people'}
    wsgiapp = RegistryUsingApp(regobj, obj)
    wsgiapp = RegistryManager(wsgiapp)
    app = TestApp(wsgiapp)
    res = app.get('/')
    assert 'Hello world' in res
    assert 'The variable is' in res
    assert "{'hi': 'people'}" in res

def test_registry_no_object_error():
    app = TestApp(simpleapp_withregistry)
    assert_raises(TypeError, app.get, '/')

def test_with_default_object():
    app = TestApp(simpleapp_withregistry_default)
    res = app.get('/')
    print(res)
    assert 'Hello world' in res
    assert "Value is {'hi': 'people'}" in res

def test_double_registry():
    obj = {'hi':'people'}
    secondobj = {'bye':'friends'}
    wsgiapp = RegistryUsingApp(regobj, obj)
    wsgiapp = RegistryManager(wsgiapp)
    wsgiapp = RegistryMiddleMan(wsgiapp, regobj, secondobj, 0)
    wsgiapp = RegistryManager(wsgiapp)
    app = TestApp(wsgiapp)
    res = app.get('/')
    assert 'Hello world' in res
    assert 'The variable is' in res
    assert "{'hi': 'people'}" in res
    assert "InsertValue at depth 0 is {'bye': 'friends'}" in res
    assert "AppendValue at depth 0 is {'bye': 'friends'}" in res

def test_really_deep_registry():
    keylist = ['fred', 'wilma', 'barney', 'homer', 'marge', 'bart', 'lisa',
        'maggie']
    valuelist = range(0, len(keylist))
    obj = {'hi':'people'}
    wsgiapp = RegistryUsingApp(regobj, obj)
    wsgiapp = RegistryManager(wsgiapp)
    for depth in valuelist:
        newobj = {keylist[depth]: depth}
        wsgiapp = RegistryMiddleMan(wsgiapp, regobj, newobj, depth)
        wsgiapp = RegistryManager(wsgiapp)
    app = TestApp(wsgiapp)
    res = app.get('/')
    assert 'Hello world' in res
    assert 'The variable is' in res
    assert "{'hi': 'people'}" in res
    for depth in valuelist:
        assert "InsertValue at depth %s is {'%s': %s}" % \
            (depth, keylist[depth], depth) in res
    for depth in valuelist:
        assert "AppendValue at depth %s is {'%s': %s}" % \
            (depth, keylist[depth], depth) in res

def test_iterating_response():
    obj = {'hi':'people'}
    secondobj = {'bye':'friends'}
    wsgiapp = RegistryUsingIteratorApp(regobj, obj)
    wsgiapp = RegistryManager(wsgiapp)
    wsgiapp = RegistryMiddleMan(wsgiapp, regobj, secondobj, 0)
    wsgiapp = RegistryManager(wsgiapp)
    app = TestApp(wsgiapp)
    res = app.get('/')
    assert 'Hello world' in res
    assert 'The variable is' in res
    assert "{'hi': 'people'}" in res
    assert "InsertValue at depth 0 is {'bye': 'friends'}" in res
    assert "AppendValue at depth 0 is {'bye': 'friends'}" in res

def _test_restorer(stack, data):
    # We need to test the request's specific Registry. Initialize it here so we
    # can use it later (RegistryManager will re-use one preexisting in the
    # environ)
    registry = Registry()
    extra_environ={'paste.throw_errors': False,
                   'paste.registry': registry}
    request_id = restorer.get_request_id(extra_environ)
    app = TestApp(stack)
    res = app.get('/', extra_environ=extra_environ, expect_errors=True)

    # Ensure all the StackedObjectProxies are empty after the RegistryUsingApp
    # raises an Exception
    for stacked, proxied_obj, test_cleanup in data:
        only_key = list(proxied_obj.keys())[0]
        try:
            assert only_key not in stacked
            assert False
        except TypeError:
            # Definitely empty
            pass

    # Ensure the StackedObjectProxies & Registry 'work' in the simulated
    # EvalException context
    replace = {'replace': 'dict'}
    new = {'new': 'object'}
    restorer.restoration_begin(request_id)
    try:
        for stacked, proxied_obj, test_cleanup in data:
            # Ensure our original data magically re-appears in this context
            only_key, only_val = list(proxied_obj.items())[0]
            assert only_key in stacked and stacked[only_key] == only_val

            # Ensure the Registry still works
            registry.prepare()
            registry.register(stacked, new)
            assert 'new' in stacked and stacked['new'] == 'object'
            registry.cleanup()

            # Back to the original (pre-prepare())
            assert only_key in stacked and stacked[only_key] == only_val

            registry.replace(stacked, replace)
            assert 'replace' in stacked and stacked['replace'] == 'dict'

            if test_cleanup:
                registry.cleanup()
                try:
                    stacked._current_obj()
                    assert False
                except TypeError:
                    # Definitely empty
                    pass
    finally:
        restorer.restoration_end()

def _restorer_data():
    S = StackedObjectProxy
    d = [[S(name='first'), dict(top='of the registry stack'), False],
         [S(name='second'), dict(middle='of the stack'), False],
         [S(name='third'), dict(bottom='of the STACK.'), False]]
    return d

def _set_cleanup_test(data):
    """Instruct _test_restorer to check registry cleanup at this level of the stack
    """
    data[2] = True

def test_restorer_basic():
    data = _restorer_data()[0]
    wsgiapp = RegistryUsingApp(data[0], data[1], raise_exc=Exception())
    wsgiapp = RegistryManager(wsgiapp)
    _set_cleanup_test(data)
    wsgiapp = EvalException(wsgiapp)
    _test_restorer(wsgiapp, [data])

def test_restorer_basic_manager_outside():
    data = _restorer_data()[0]
    wsgiapp = RegistryUsingApp(data[0], data[1], raise_exc=Exception())
    wsgiapp = EvalException(wsgiapp)
    wsgiapp = RegistryManager(wsgiapp)
    _set_cleanup_test(data)
    _test_restorer(wsgiapp, [data])

def test_restorer_middleman_nested_evalexception():
    data = _restorer_data()[:2]
    wsgiapp = RegistryUsingApp(data[0][0], data[0][1], raise_exc=Exception())
    wsgiapp = EvalException(wsgiapp)
    wsgiapp = RegistryMiddleMan(wsgiapp, data[1][0], data[1][1], 0)
    wsgiapp = RegistryManager(wsgiapp)
    _set_cleanup_test(data[1])
    _test_restorer(wsgiapp, data)

def test_restorer_nested_middleman():
    data = _restorer_data()[:2]
    wsgiapp = RegistryUsingApp(data[0][0], data[0][1], raise_exc=Exception())
    wsgiapp = RegistryManager(wsgiapp)
    _set_cleanup_test(data[0])
    wsgiapp = RegistryMiddleMan(wsgiapp, data[1][0], data[1][1], 0)
    wsgiapp = EvalException(wsgiapp)
    wsgiapp = RegistryManager(wsgiapp)
    _set_cleanup_test(data[1])
    _test_restorer(wsgiapp, data)

def test_restorer_middlemen_nested_evalexception():
    data = _restorer_data()
    wsgiapp = RegistryUsingApp(data[0][0], data[0][1], raise_exc=Exception())
    wsgiapp = RegistryManager(wsgiapp)
    _set_cleanup_test(data[0])
    wsgiapp = EvalException(wsgiapp)
    wsgiapp = RegistryMiddleMan(wsgiapp, data[1][0], data[1][1], 0)
    wsgiapp = RegistryManager(wsgiapp)
    _set_cleanup_test(data[1])
    wsgiapp = RegistryMiddleMan(wsgiapp, data[2][0], data[2][1], 1)
    wsgiapp = RegistryManager(wsgiapp)
    _set_cleanup_test(data[2])
    _test_restorer(wsgiapp, data)

def test_restorer_disabled():
    # Ensure restoration_begin/end work safely when there's no Registry
    wsgiapp = TestApp(simpleapp)
    wsgiapp.get('/')
    try:
        restorer.restoration_begin(1)
    finally:
        restorer.restoration_end()
        # A second call should do nothing
        restorer.restoration_end()
