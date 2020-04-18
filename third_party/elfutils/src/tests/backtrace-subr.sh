# Copyright (C) 2013 Red Hat, Inc.
# This file is part of elfutils.
#
# This file is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# elfutils is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

. $srcdir/test-subr.sh

# Verify one of the backtraced threads contains function 'main'.
check_main()
{
  if grep -w main $1; then
    return
  fi
  echo >&2 $2: no main
  false
}

# Without proper ELF symbols resolution we could get inappropriate weak
# symbol "gsignal" with the same address as the correct symbol "raise".
# It was fixed by GIT commit 78dec228b3cfb2f9300cd0b682ebf416c9674c91 .
# [patch] Improve ELF symbols preference (global > weak)
# https://lists.fedorahosted.org/pipermail/elfutils-devel/2012-October/002624.html
check_gsignal()
{
  if ! grep -w gsignal $1; then
    return
  fi
  echo >&2 $2: found gsignal
  false
}

# Verify the STDERR output does not contain unexpected errors.
# In some cases we cannot reliably find out we got behind _start as some
# operating system do not properly terminate CFI by undefined PC.
# Ignore it here as it is a bug of OS, not a bug of elfutils.
check_err()
{
  if [ $(egrep -v <$1 'dwfl_thread_getframes: (No DWARF information found|no matching address range)$' \
         | wc -c) \
       -eq 0 ]
  then
    return
  fi
  echo >&2 $2: neither empty nor just out of DWARF
  false
}

check_all()
{
  bt=$1
  err=$2
  testname=$3
  check_main $bt $testname
  check_gsignal $bt $testname
  check_err $err $testname
}

check_unsupported()
{
  err=$1
  testname=$2
  if grep -q ': Unwinding not supported for this architecture$' $err; then
    echo >&2 $testname: arch not supported
    exit 77
  fi
}

check_core()
{
  arch=$1
  testfiles backtrace.$arch.{exec,core}
  tempfiles backtrace.$arch.{bt,err}
  echo ./backtrace ./backtrace.$arch.{exec,core}
  testrun ${abs_builddir}/backtrace -e ./backtrace.$arch.exec --core=./backtrace.$arch.core 1>backtrace.$arch.bt 2>backtrace.$arch.err || true
  cat backtrace.$arch.{bt,err}
  check_all backtrace.$arch.{bt,err} backtrace.$arch.core
}

# Backtrace live process.
# Do not abort on non-zero exit code due to some warnings of ./backtrace
# - see function check_err.
check_native()
{
  child=$1
  tempfiles $child.{bt,err}
  (set +ex; testrun ${abs_builddir}/backtrace --backtrace-exec=${abs_builddir}/$child 1>$child.bt 2>$child.err; true)
  cat $child.{bt,err}
  check_unsupported $child.err $child
  check_all $child.{bt,err} $child
}

# Backtrace core file.
check_native_core()
{
  child=$1

  # Disable valgrind while dumping core.
  SAVED_VALGRIND_CMD="$VALGRIND_CMD"
  unset VALGRIND_CMD

  # Skip the test if we cannot adjust core ulimit.
  core="core.`ulimit -c unlimited || exit 77; set +ex; testrun ${abs_builddir}/$child --gencore; true`"

  if [ "x$SAVED_VALGRIND_CMD" != "x" ]; then
    VALGRIND_CMD="$SAVED_VALGRIND_CMD"
    export VALGRIND_CMD
  fi

  # Do not abort on non-zero exit code due to some warnings of ./backtrace
  # - see function check_err.
  tempfiles $core{,.{bt,err}}
  (set +ex; testrun ${abs_builddir}/backtrace -e ${abs_builddir}/$child --core=$core 1>$core.bt 2>$core.err; true)
  cat $core.{bt,err}
  check_unsupported $core.err $child-$core
  check_all $core.{bt,err} $child-$core
}
