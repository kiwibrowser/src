from paste.util.import_string import *
import sys
import os

def test_simple():
    for func in eval_import, simple_import:
        assert func('sys') is sys
        assert func('sys.version') is sys.version
        assert func('os.path.join') is os.path.join

def test_complex():
    assert eval_import('sys:version') is sys.version
    assert eval_import('os:getcwd()') == os.getcwd()
    assert (eval_import('sys:version.split()[0]') ==
            sys.version.split()[0])

