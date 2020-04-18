import doctest
from paste.util.import_string import simple_import
import os

filenames = [
    'tests/test_template.txt',
    ]

modules = [
    'paste.util.template',
    'paste.util.looper',
    # This one opens up httpserver, which is bad:
    #'paste.auth.cookie',
    #'paste.auth.multi',
    #'paste.auth.digest',
    #'paste.auth.basic',
    #'paste.auth.form',
    #'paste.progress',
    'paste.exceptions.serial_number_generator',
    'paste.evalexception.evalcontext',
    'paste.util.dateinterval',
    'paste.util.quoting',
    'paste.wsgilib',
    'paste.url',
    'paste.request',
    ]

options = doctest.ELLIPSIS|doctest.REPORT_ONLY_FIRST_FAILURE

def test_doctests():
    for filename in filenames:
        filename = os.path.join(
            os.path.dirname(os.path.dirname(__file__)),
            filename)
        yield do_doctest, filename

def do_doctest(filename):
    failure, total = doctest.testfile(
        filename, module_relative=False,
        optionflags=options)
    assert not failure, "Failure in %r" % filename

def test_doctest_mods():
    for module in modules:
        yield do_doctest_mod, module

def do_doctest_mod(module):
    module = simple_import(module)
    failure, total = doctest.testmod(
        module, optionflags=options)
    assert not failure, "Failure in %r" % module

if __name__ == '__main__':
    import sys
    import doctest
    args = sys.argv[1:]
    if not args:
        args = filenames
    for filename in args:
        doctest.testfile(filename, module_relative=False)
