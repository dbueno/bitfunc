#!/bin/bash

whereami=`dirname $0`

SAT=10

if test -z "$1"; then
    echo "error: need argument directory of CNF files"
    exit 1
fi

funcsat="$whereami/../funcsat"
check_solution="$whereami/../check_solution"
if test \! -x $funcsat; then
    $funcsat=../funcsat
fi
if test \! -x $funcsat; then
    echo "error: can't find funcsat"
    exit 1
fi
if test \! -x $check_solution; then
    $check_solution=../check_solution
fi
if test \! -x $check_solution; then
    echo "$whereami"
    echo "error: can't find check_solution"
    exit 1
fi

files=`ls -1 "$1"/*.cnf*`
set -x
for f in $files; do
    $funcsat --quiet --live=no "$f" >solution.txt
    if test \! $? -eq $SAT; then
        echo "!!! error on $f"
        exit 1
    fi
    $check_solution solution.txt "$f"
    if test \! $? -eq 0; then
        echo "!!! error validating solution on $f"
        exit 1
    fi
done
