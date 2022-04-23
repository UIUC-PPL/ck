#!/bin/bash
set -euo pipefail
IFS=$'\n\t'
# ensure charm path correctly specified
basedir=${CHARM_HOME:-}
if [ -z "$basedir" ] || [ ! -d "$basedir" ]; then
    echo "error> set CHARM_HOME=/path/to/charm"
    exit 1
else
    set +u
    # source machine options to learn build configuration
    source "$basedir/include/conv-mach-opt.sh"
    set -u
fi
# detect the number of processors to run with
if [ "$OSTYPE" = "linux-gnu" ]; then
    nproc=$(grep ^cpu\\scores /proc/cpuinfo | uniq |  awk '{print $4}')
else
    nproc=$(nproc)
fi
# detect netlrts builds and set all-test options accordingly
if [ "$CMK_GDIR" = "netlrts" ]; then
    allargs="++local"
else
    allargs=""
fi
# set per-test options
unset args
declare -A args
args["histogram"]="4 4 4 4"
args["leanmd"]="3 3 3 20"
args["hierarchical_hello"]="4 4 +balancer RotateLB +LBDebug 1"
# clean and build all tests 
make clean
make -j
# then get ready to rock n' roll
for pgm in *.out; do
    key="${pgm%%.*}"
    set +u
    if [ -v 'args[$key]' ]; then
        opts="$allargs ${args[$key]}"
    else
        opts="$allargs"
    fi
    set -u
    cmd="charmrun +p$nproc ./$pgm $opts"
    echo "$ $cmd"
    eval "$cmd"
done
