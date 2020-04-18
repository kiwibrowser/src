#!/bin/bash
# Copyright (c) 2011 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# If you need to add several parameters action should look like "param1 param2",
# i.e parameters should be separated by spaces and enclosed in quotes. This
# quotes should not be used in redirect_table.txt though because Makefile adds
# them automatically.

declare -r usage="$0 from to action"

if (( $# != 3 )); then
  echo "$usage" >&2
  exit 1
fi

if [[ "$3" = "-m32" ]]; then
  cat >"$1" <<ENDSCRIPT
#!/bin/bash
# Put -V argument(s) prior to -m32. Otherwise the GCC driver does not accept it.
program_name="$2"
if [[ "\$1" = "-V" ]]; then
  shift
  OPTV="\$1"
  shift
  exec "\${0%/*}/\$program_name" -V "\$OPTV" -m32 "\$@"
elif [[ "\${1:0:2}" = "-V" ]]; then
  OPTV="\$1"
  shift
  exec "\${0%/*}/\$program_name" "\$OPTV" -m32 "\$@"
else
  exec "\${0%/*}/\$program_name" -m32 "\$@"
fi
ENDSCRIPT
elif [[ "$3" = "" ]]; then
  ln -nfs $2 $1
else
  cat >"$1" <<ENDSCRIPT
#!/bin/bash
exec "\${0%/*}/$2" $3 "\$@"
ENDSCRIPT
fi

chmod 755 "$1"
