#!/bin/bash
set -euo pipefail
IFS=$'\n\t'
# ensure new enough bash version
major=${BASH_VERSINFO[0]}
minor=${BASH_VERSINFO[1]}
if ! { [ "$major" -gt "4" ] || { [ "$major" -eq "4" ] && [ "$minor" -ge "2" ]; }; }; then
    # see: https://stackoverflow.com/a/13219811/
    echo "error> bash v4.2 or newer required"
    exit 1
fi
# ensure charm path correctly specified
basedir=${CHARM_HOME:-}
if [ -z "$basedir" ] || [ ! -d "$basedir" ]; then
    echo "error> set CHARM_HOME=/path/to/charm"
    exit 1
else
    set +u
    # source machine options to learn build configuration
    pushd "$basedir/include/"
    source "conv-mach-opt.sh"
    popd
    set -u
fi
# detect the number of processors to run with
nproc=${CMK_NUM_PES:-}
if [ -z "$nproc" ]; then
    if [ "$OSTYPE" = "linux-gnu" ]; then
        nproc=$(grep ^cpu\\scores /proc/cpuinfo | uniq |  awk '{print $4}')
    else
        nproc=$(nproc)
    fi
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
args["histogram"]="$nproc 4 4 4"
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
