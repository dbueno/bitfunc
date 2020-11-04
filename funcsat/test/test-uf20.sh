#!/bin/bash

SAT=10
UNSAT=20
UNK=0

set -x
files=`ls -1 uf20/*.cnf`
funcsat=../funcsat

for f in $files; do
    $funcsat --quiet --live=no "$f" >solution.txt
    if test \! $? -eq $SAT; then
        echo "!!! error on $f"
        exit 1
    fi
    ../check_solution solution.txt "$f"
    if test \! $? -eq 0; then
        echo "!!! error validating solution on $f"
        exit 1
    fi
done
