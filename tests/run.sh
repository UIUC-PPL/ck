#!/bin/bash
set -euo pipefail
IFS=$'\n\t'

make clean
make -j

nproc=$(grep ^cpu\\scores /proc/cpuinfo | uniq |  awk '{print $4}')

unset args
declare -A args

args["histogram"]="4 4 4 4"
args["leanmd"]="3 3 3 20"
args["hierarchical_hello"]="4 4 +balancer RotateLB +LBDebug 1"

allargs="++local"

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
