#!/usr/bin/env python2

#===- subzero/wasm-run-torture-tests.py - Subzero WASM Torture Test Driver ===//
#
#                        The Subzero Code Generator
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===-----------------------------------------------------------------------===//

from __future__ import print_function
import argparse
import glob
import multiprocessing
import os
import Queue
import shutil
import StringIO
import sys
import threading

IGNORED_TESTS = set([
  # The remaining tests are known waterfall failures

  '20010122-1.c.wasm',
  '20031003-1.c.wasm',
  '20071018-1.c.wasm',
  '20071120-1.c.wasm',
  '20071220-1.c.wasm',
  '20071220-2.c.wasm',
  '20101011-1.c.wasm',
  'alloca-1.c.wasm',
  'bitfld-3.c.wasm',
  'bitfld-5.c.wasm',
  'builtin-bitops-1.c.wasm',
  'conversion.c.wasm',
  'eeprof-1.c.wasm',
  'frame-address.c.wasm',
  'pr17377.c.wasm',
  'pr32244-1.c.wasm',
  'pr34971.c.wasm',
  'pr36765.c.wasm',
  'pr39228.c.wasm',
  'pr43008.c.wasm',
  'pr47237.c.wasm',
  'pr60960.c.wasm',
  'va-arg-pack-1.c.wasm',

  '20000717-5.c.wasm', # abort() (also works without emcc)
  '20001203-2.c.wasm', # assert fail (works without emcc)
  '20040811-1.c.wasm', # OOB trap
  '20070824-1.c.wasm', # abort() (also works without emcc)
  'arith-rand-ll.c.wasm', # abort() (works without emcc)
  'arith-rand.c.wasm', # abort() (works without emcc)
  'pr23135.c.wasm', # OOB trap (works without emcc)
  'pr34415.c.wasm', # (empty output?)
  'pr36339.c.wasm', # abort() (works without emcc)
  'pr38048-2.c.wasm', # abort() (works without emcc)
  'pr42691.c.wasm', # abort() (works without emcc)
  'pr43220.c.wasm', # OOB trap (works without emcc)
  'pr43269.c.wasm', # abort() (works without emcc)
  'vla-dealloc-1.c.wasm', # OOB trap (works without emcc)
  '20051012-1.c.wasm', # error reading binary
  '921208-2.c.wasm', # error reading binary
  '920501-1.c.wasm', # error reading binary
  'call-trap-1.c.wasm', # error reading binary
  'pr44942.c.wasm', # error reading binary

  '920625-1.c.wasm', # abort() (also fails without emcc)
  '931004-10.c.wasm', # abort() (also fails without emcc)
  '931004-12.c.wasm', # abort() (also fails without emcc)
  '931004-14.c.wasm', # abort() (also fails without emcc)
  '931004-6.c.wasm', # abort() (also fails without emcc)
  'pr38051.c.wasm', # (empty output?) (fails without emcc)
  'pr38151.c.wasm', # abort() (fails without emcc)
  'pr44575.c.wasm', # abort() (fails without emcc)
  'strct-stdarg-1.c.wasm', # abort() (fails without emcc)
  'strct-varg-1.c.wasm', # abort() (fails without emcc)
  'va-arg-22.c.wasm', # abort() (fails without emcc)
  'stdarg-3.c.wasm', # abort() (fails without emcc)
  'pr56982.c.wasm', # missing setjmp (wasm.js check did not catch)

  '20010605-2.c.wasm', # missing __netf2
  '20020413-1.c.wasm', # missing __lttf2
  '20030914-1.c.wasm', # missing __floatsitf
  '20040709-1.c.wasm', # missing __netf2
  '20040709-2.c.wasm', # missing __netf2
  '20050121-1.c.wasm', # missing __floatsitf
  '20080502-1.c.wasm', # missing __eqtf2
  '920501-8.c.wasm', # missing __extenddftf2
  '930513-1.c.wasm', # missing __extenddftf2
  '930622-2.c.wasm', # missing __floatditf
  '960215-1.c.wasm', # missing __addtf3
  '960405-1.c.wasm', # missing __eqtf2
  '960513-1.c.wasm', # missing __subtf3
  'align-2.c.wasm', # missing __eqtf2
  'complex-6.c.wasm', # missing __subtf3
  'complex-7.c.wasm', # missing __netf2
  'pr49218.c.wasm', # missing __fixsfti
  'pr54471.c.wasm', # missing __multi3
  'regstack-1.c.wasm', # missing __addtf3
  'stdarg-1.c.wasm', # missing __netf2
  'stdarg-2.c.wasm', # missing __floatsitf
  'va-arg-5.c.wasm', # missing __eqtf2
  'va-arg-6.c.wasm', # missing __eqtf2
  'struct-ret-1.c.wasm', # missing __extenddftf2
])

parser = argparse.ArgumentParser()
parser.add_argument('-v', '--verbose', action='store_true')
parser.add_argument('--translate-only', action='store_true')
parser.add_argument('tests', nargs='*')
args = parser.parse_args()

OUT_DIR = "./build/wasm-torture"

results_lock = threading.Lock()

compile_count = 0
compile_failures = []

run_count = 0
run_failures = []

def run_test(test_file, verbose=False):
  global args
  global compile_count
  global compile_failures
  global results_lock
  global run_count
  global run_failures
  global OUT_DIR
  global IGNORED_TESTS

  run_test = not args.translate_only

  test_name = os.path.basename(test_file)
  obj_file = os.path.join(OUT_DIR, test_name + ".o")
  exe_file = os.path.join(OUT_DIR, test_name + ".exe")

  if not verbose and test_name in IGNORED_TESTS:
    print("\033[1;34mSkipping {}\033[1;m".format(test_file))
    return

  cmd = """LD_LIBRARY_PATH=../../../../v8/out/native/lib.target ./pnacl-sz \
               -filetype=obj -target=x8632 {} -threads=0 -O2 \
               -verbose=wasm -o {}""".format(test_file, obj_file)

  if not verbose:
    cmd += " &> /dev/null"

  out = StringIO.StringIO()

  out.write(test_file + " ...");
  status = os.system(cmd);
  if status != 0:
    print('\033[1;31m[compile fail]\033[1;m', file=out)
    with results_lock:
      compile_failures.append(test_file)
  else:
    compile_count += 1

    # Try to link and run the program.
    cmd = "clang -g -m32 {} -o {} " + \
          "./runtime/szrt.c ./runtime/wasm-runtime.cpp -lm -lstdc++"
    cmd = cmd.format(obj_file, exe_file)

    if not run_test or os.system(cmd) == 0:
      if not run_test or os.system(exe_file) == 0:
        with results_lock:
          run_count += 1
        print('\033[1;32m[ok]\033[1;m', file=out)
      else:
        with results_lock:
          run_failures.append(test_file)
        print('\033[1;33m[run fail]\033[1;m', file=out)
    else:
      with results_lock:
        run_failures.append(test_file)
      print('\033[1;33m[run fail]\033[1;m', file=out)

  sys.stdout.write(out.getvalue())

verbose = args.verbose

if len(args.tests) > 0:
  test_files = args.tests
else:
  test_files = glob.glob("./emwasm-torture-out/*.wasm")

if os.path.exists(OUT_DIR):
  shutil.rmtree(OUT_DIR)
os.mkdir(OUT_DIR)

tasks = Queue.Queue()

def worker():
  while True:
    run_test(tasks.get(), verbose)
    tasks.task_done()

for i in range(multiprocessing.cpu_count()):
  t = threading.Thread(target=worker)
  t.daemon = True
  t.start()

for test_file in test_files:
  tasks.put(test_file)

tasks.join()

if len(compile_failures) > 0:
  print()
  print("Compilation failures:")
  print("=====================\n")
  for f in compile_failures:
    print("    \033[1;31m" + f + "\033[1;m")

if len(run_failures) > 0:
  print()
  print("Run failures:")
  print("=============\n")
  for f in run_failures:
    print("    \033[1;33m" + f + "\033[1;m")

print("\n\033[1;32m{}\033[1;m / \033[1;33m{}\033[1;m / {} tests passed"
      .format(run_count, compile_count - run_count,
              run_count + len(compile_failures) + len(run_failures)))
