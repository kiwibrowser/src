# -*- coding: utf-8 -*-
# (c) 2007 Ian Bicking and Philip Jenvey; written for Paste (http://pythonpaste.org)
# Licensed under the MIT license: http://www.opensource.org/licenses/mit-license.php
import cgi
import six
from six.moves import StringIO

from nose.tools import assert_raises

from paste.util.multidict import MultiDict, UnicodeMultiDict

def test_dict():
    d = MultiDict({'a': 1})
    assert d.items() == [('a', 1)]

    d['b'] = 2
    d['c'] = 3
    assert d.items() == [('a', 1), ('b', 2), ('c', 3)]

    d['b'] = 4
    assert d.items() == [('a', 1), ('c', 3), ('b', 4)]

    d.add('b', 5)
    assert_raises(KeyError, d.getone, "b")
    assert d.getall('b') == [4, 5]
    assert d.items() == [('a', 1), ('c', 3), ('b', 4), ('b', 5)]

    del d['b']
    assert d.items() == [('a', 1), ('c', 3)]
    assert d.pop('xxx', 5) == 5
    assert d.getone('a') == 1
    assert d.popitem() == ('c', 3)
    assert d.items() == [('a', 1)]

    item = []
    assert d.setdefault('z', item) is item
    assert d.items() == [('a', 1), ('z', item)]

    assert d.setdefault('y', 6) == 6

    assert d.mixed() == {'a': 1, 'y': 6, 'z': item}
    assert d.dict_of_lists() == {'a': [1], 'y': [6], 'z': [item]}

    assert 'a' in d
    dcopy = d.copy()
    assert dcopy is not d
    assert dcopy == d
    d['x'] = 'x test'
    assert dcopy != d

    d[(1, None)] = (None, 1)
    assert d.items() == [('a', 1), ('z', []), ('y', 6), ('x', 'x test'),
                         ((1, None), (None, 1))]

def test_unicode_dict():
    _test_unicode_dict()
    _test_unicode_dict(decode_param_names=True)

def _test_unicode_dict(decode_param_names=False):
    d = UnicodeMultiDict(MultiDict({b'a': 'a test'}))
    d.encoding = 'utf-8'
    d.errors = 'ignore'

    if decode_param_names:
        key_str = six.text_type
        k = lambda key: key
        d.decode_keys = True
    else:
        key_str = six.binary_type
        k = lambda key: key.encode()

    def assert_unicode(obj):
        assert isinstance(obj, six.text_type)

    def assert_key_str(obj):
        assert isinstance(obj, key_str)

    def assert_unicode_item(obj):
        key, value = obj
        assert isinstance(key, key_str)
        assert isinstance(value, six.text_type)

    assert d.items() == [(k('a'), u'a test')]
    map(assert_key_str, d.keys())
    map(assert_unicode, d.values())

    d[b'b'] = b'2 test'
    d[b'c'] = b'3 test'
    assert d.items() == [(k('a'), u'a test'), (k('b'), u'2 test'), (k('c'), u'3 test')]
    list(map(assert_unicode_item, d.items()))

    d[k('b')] = b'4 test'
    assert d.items() == [(k('a'), u'a test'), (k('c'), u'3 test'), (k('b'), u'4 test')], d.items()
    list(map(assert_unicode_item, d.items()))

    d.add(k('b'), b'5 test')
    assert_raises(KeyError, d.getone, k("b"))
    assert d.getall(k('b')) == [u'4 test', u'5 test']
    map(assert_unicode, d.getall('b'))
    assert d.items() == [(k('a'), u'a test'), (k('c'), u'3 test'), (k('b'), u'4 test'),
                         (k('b'), u'5 test')]
    list(map(assert_unicode_item, d.items()))

    del d[k('b')]
    assert d.items() == [(k('a'), u'a test'), (k('c'), u'3 test')]
    list(map(assert_unicode_item, d.items()))
    assert d.pop('xxx', u'5 test') == u'5 test'
    assert isinstance(d.pop('xxx', u'5 test'), six.text_type)
    assert d.getone(k('a')) == u'a test'
    assert isinstance(d.getone(k('a')), six.text_type)
    assert d.popitem() == (k('c'), u'3 test')
    d[k('c')] = b'3 test'
    assert_unicode_item(d.popitem())
    assert d.items() == [(k('a'), u'a test')]
    list(map(assert_unicode_item, d.items()))

    item = []
    assert d.setdefault(k('z'), item) is item
    items = d.items()
    assert items == [(k('a'), u'a test'), (k('z'), item)]
    assert isinstance(items[1][0], key_str)
    assert isinstance(items[1][1], list)

    assert isinstance(d.setdefault(k('y'), b'y test'), six.text_type)
    assert isinstance(d[k('y')], six.text_type)

    assert d.mixed() == {k('a'): u'a test', k('y'): u'y test', k('z'): item}
    assert d.dict_of_lists() == {k('a'): [u'a test'], k('y'): [u'y test'],
                                 k('z'): [item]}
    del d[k('z')]
    list(map(assert_unicode_item, six.iteritems(d.mixed())))
    list(map(assert_unicode_item, [(key, value[0]) for \
                                   key, value in six.iteritems(d.dict_of_lists())]))

    assert k('a') in d
    dcopy = d.copy()
    assert dcopy is not d
    assert dcopy == d
    d[k('x')] = 'x test'
    assert dcopy != d

    d[(1, None)] = (None, 1)
    assert d.items() == [(k('a'), u'a test'), (k('y'), u'y test'), (k('x'), u'x test'),
                         ((1, None), (None, 1))]
    item = d.items()[-1]
    assert isinstance(item[0], tuple)
    assert isinstance(item[1], tuple)

    fs = cgi.FieldStorage()
    fs.name = 'thefile'
    fs.filename = 'hello.txt'
    fs.file = StringIO('hello')
    d[k('f')] = fs
    ufs = d[k('f')]
    assert isinstance(ufs, cgi.FieldStorage)
    assert ufs is not fs
    assert ufs.name == fs.name
    assert isinstance(ufs.name, str if six.PY3 else key_str)
    assert ufs.filename == fs.filename
    assert isinstance(ufs.filename, six.text_type)
    assert isinstance(ufs.value, str)
    assert ufs.value == 'hello'
