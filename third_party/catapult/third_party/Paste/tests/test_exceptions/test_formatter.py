from paste.exceptions import formatter
from paste.exceptions import collector
import sys
import os
import difflib

class Mock(object):
    def __init__(self, **kw):
        for name, value in kw.items():
            setattr(self, name, value)

class Supplement(Mock):

    object = 'test_object'
    source_url = 'http://whatever.com'
    info = 'This is some supplemental information'
    args = ()
    def getInfo(self):
        return self.info

    def __call__(self, *args):
        self.args = args
        return self

class BadSupplement(Supplement):

    def getInfo(self):
        raise ValueError("This supplemental info is buggy")

def call_error(sup):
    1 + 2
    __traceback_supplement__ = (sup, ())
    assert 0, "I am an error"

def raise_error(sup='default'):
    if sup == 'default':
        sup = Supplement()
    for i in range(10):
        __traceback_info__ = i
        if i == 5:
            call_error(sup=sup)

def hide(t, inner, *args, **kw):
    __traceback_hide__ = t
    return inner(*args, **kw)

def pass_through(info, inner, *args, **kw):
    """
    To add another frame to the call; detectable because
    __tracback_info__ is set to `info`
    """
    __traceback_info__ = info
    return inner(*args, **kw)

def format(type='html', **ops):
    data = collector.collect_exception(*sys.exc_info())
    report = getattr(formatter, 'format_' + type)(data, **ops)
    return report

formats = ('text', 'html')

def test_excersize():
    for f in formats:
        try:
            raise_error()
        except:
            format(f)

def test_content():
    for f in formats:
        try:
            raise_error()
        except:
            result = format(f)
            print(result)
            assert 'test_object' in result
            assert 'http://whatever.com' in result
            assert 'This is some supplemental information' in result
            assert 'raise_error' in result
            assert 'call_error' in result
            assert '5' in result
            assert 'test_content' in result
        else:
            assert 0

def test_trim():
    current = os.path.abspath(os.getcwd())
    for f in formats:
        try:
            raise_error()
        except:
            result = format(f, trim_source_paths=[(current, '.')])
            assert current not in result
            assert ('%stest_formatter.py' % os.sep) in result, ValueError(repr(result))
        else:
            assert 0

def test_hide():
    for f in formats:
        try:
            hide(True, raise_error)
        except:
            result = format(f)
            print(result)
            assert 'in hide_inner' not in result
            assert 'inner(*args, **kw)' not in result
        else:
            assert 0

def print_diff(s1, s2):
    differ = difflib.Differ()
    result = list(differ.compare(s1.splitlines(), s2.splitlines()))
    print('\n'.join(result))

def test_hide_supppressed():
    """
    When an error occurs and __traceback_stop__ is true for the
    erroneous frame, then that setting should be ignored.
    """
    for f in ['html']: #formats:
        results = []
        for hide_value in (False, 'after'):
            try:
                pass_through(
                    'a',
                    hide,
                    hide_value,
                    pass_through,
                    'b',
                    raise_error)
            except:
                results.append(format(f))
            else:
                assert 0
        if results[0] != results[1]:
            print_diff(results[0], results[1])
            assert 0

def test_hide_after():
    for f in formats:
        try:
            pass_through(
                'AABB',
                hide, 'after',
                pass_through, 'CCDD',
                # A little whitespace to keep this line out of the
                # content part of the report


                hide, 'reset',
                raise_error)
        except:
            result = format(f)
            assert 'AABB' in result
            assert 'CCDD' not in result
            assert 'raise_error' in result
        else:
            assert 0

def test_hide_before():
    for f in formats:
        try:
            pass_through(
                'AABB',
                hide, 'before',
                raise_error)
        except:
            result = format(f)
            print(result)
            assert 'AABB' not in result
            assert 'raise_error' in result
        else:
            assert 0

def test_make_wrappable():
    assert '<wbr>' in formatter.make_wrappable('x'*1000)
    # I'm just going to test that this doesn't excede the stack limit:
    formatter.make_wrappable(';'*2000)
    assert (formatter.make_wrappable('this that the other')
            == 'this that the other')
    assert (formatter.make_wrappable('this that ' + ('x'*50) + ';' + ('y'*50) + ' and the other')
            == 'this that '+('x'*50) + ';<wbr>' + ('y'*50) + ' and the other')

