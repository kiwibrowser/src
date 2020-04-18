#!/bin/bash
SCRIPTPATH=$(dirname "$0")
export PYTHONPATH=$SCRIPTPATH:$PYTHONPATH
python -m inferno.inferno "$@"
