#!/bin/bash

SAT=10
UNSAT=20
UNK=0

files=`ls -1 $ ../../funsat/tests/problems/uuf50/*.cnf`

for f in $files; do
    ../funcsat --quiet "$f"
    if test \! $? -eq $UNSAT; then
        echo "!!! error on $f"
        exit 1
    fi
done
