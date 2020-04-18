#!/bin/bash
#
# sel_ldr.bash
# A simple shell script for:
# 1)  Invoking the sel_ldr with the setuid sandbox
# 2)  Starting up sel_ldr with an enhanced
#     library path, for Linux. This allows us to install libraries in
#     a user directory rather than having to put them someplace like
#     /usr/lib
#
LD_LIBRARY_PATH=$HOME/.mozilla/plugins:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH

safe_dir=/usr/local/google/plugins/

if [ -f $safe_dir/sandboxme ]; then
  echo "Running sandboxed sel_ldr"
  found_module=0
  args=''
  for i in "$@"
  do
    if [ $found_module = 1 ]
    then
      module="$i"
      args="$args '$safe_dir/module.nexe'"
      cp -- "$i" "$safe_dir/module.nexe"
      chmod ugo+r "$safe_dir/module.nexe"
      found_module=0
    else
      i=$(echo "$i" | sed "s/'/'\\\\''/g")
      args="$args '$i'"
    fi
    case "$i" in
      -f)
        found_module=1
        ;;
    esac
  done
  if [ "$module" = '' ]
  then
    echo "Error:  No NaCl module specified." >&2
    exit 1
  fi
  cp $HOME/.mozilla/plugins/sel_ldr_bin "$safe_dir/"
  eval SBD_X=4 exec $safe_dir/sandboxme -c0 -u0 -- $safe_dir/sel_ldr_bin $args
else
  echo "Warning: Running non-sandboxed sel_ldr"
  exec $HOME/.mozilla/plugins/sel_ldr_bin "$@"
fi
