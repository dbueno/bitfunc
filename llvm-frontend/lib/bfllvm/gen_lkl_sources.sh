#!/bin/sh

INCLUDE_DIR="../../include"

DERIVED_HEADERS="${INCLUDE_DIR}/linklist_bbs.h ${INCLUDE_DIR}/linklist_literal.h"
DERIVED_SOURCES="linklist_bbs.cpp linklist_literal.c"

if test -n "$1"; then
    if test "$1" = "clean"; then
        for f in "$DERIVED_SOURCES" "$DERIVED_HEADERS"; do
            set -x
            rm -f "$f"
            set +x
        done
        exit 0
    else
        echo "$0: Unrecognized argument '$1'; try 'clean'"
        exit 1
    fi
fi

set -x
echo '#include "llvm/Instructions.h"' | cat -  linklist.h.in | sed 's/@TYPE@/llvm::BasicBlock*/g' | sed 's/@SHORTTYPE@/bbs/g' >"${INCLUDE_DIR}/linklist_bbs.h"
echo '#include "funcsat/system.h"' | cat -  linklist.h.in | sed 's/@TYPE@/literal/g' | sed 's/@SHORTTYPE@/literal/g' >"${INCLUDE_DIR}/linklist_literal.h"
sed 's/@TYPE@/llvm::BasicBlock*/g' linklist.c.in | sed 's/@SHORTTYPE@/bbs/g' >"linklist_bbs.cpp"
sed 's/@TYPE@/literal/g' linklist.c.in | sed 's/@SHORTTYPE@/literal/g' >"linklist_literal.c"

