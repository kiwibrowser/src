#!/usr/bin/env python
#-------------------------------------------------------------------------------
# test/run_examples_test.py
#
# Run the examples and compare their output to a reference
#
# Eli Bendersky (eliben@gmail.com)
# This code is in the public domain
#-------------------------------------------------------------------------------
import os, sys
import logging
from utils import setup_syspath; setup_syspath()
from utils import run_exe, is_in_rootdir, dump_output_to_temp_files


# Create a global logger object
#
testlog = logging.getLogger('run_examples_test')
testlog.setLevel(logging.DEBUG)
testlog.addHandler(logging.StreamHandler(sys.stdout))


def discover_examples():
    """ Return paths to all example scripts. Assume we're in the root source
        dir of pyelftools.
    """
    root = './examples'
    for filename in os.listdir(root):
        if os.path.splitext(filename)[1] == '.py':
            yield os.path.join(root, filename)


def reference_output_path(example_path):
    """ Compute the reference output path from a given example path.
    """
    examples_root, example_name = os.path.split(example_path)
    example_noext, _ = os.path.splitext(example_name)
    return os.path.join(examples_root, 'reference_output', example_noext + '.out')


def run_example_and_compare(example_path):
    testlog.info("Example '%s'" % example_path)

    reference_path = reference_output_path(example_path)
    ref_str = ''
    try:
        with open(reference_path) as ref_f:
            ref_str = ref_f.read()
    except (IOError, OSError) as e:
        testlog.info('.......ERROR - reference output cannot be read! - %s' % e)
        return False

    rc, example_out = run_exe(example_path, ['./examples/sample_exe64.elf'])
    if rc != 0:
        testlog.info('.......ERROR - example returned error code %s' % rc)
        return False

    # Comparison is done as lists of lines, to avoid EOL problems
    if example_out.split() == ref_str.split():
        return True
    else:
        testlog.info('.......FAIL comparison')
        dump_output_to_temp_files(testlog, example_out)
        return False


def main():
    if not is_in_rootdir():
        testlog.error('Error: Please run me from the root dir of pyelftools!')
        return 1

    success = True
    for example_path in discover_examples():
        if success:
            success = success and run_example_and_compare(example_path)

    if success:
        testlog.info('\nConclusion: SUCCESS')
        return 0
    else:
        testlog.info('\nConclusion: FAIL')
        return 1


if __name__ == '__main__':
    sys.exit(main())

