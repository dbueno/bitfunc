#!/bin/sh

SAT=10
UNSAT=20
UNK=0

../funcsat --quiet --assume="1, 2" --assume="-3" --assume="-4" --assume="-5"--assume=" -6" --assume="-7" --assume="8" --assume="-9" --assume="-10" --assume="-11" --assume="12" --assume="-13" --assume="-14" --assume="-15" --assume="-16" --assume="17" --assume="18" --assume="19" --assume="20" --assume="-21" --assume="-22" --assume="-23" --assume="24" --assume="25" --assume="-26" --assume="-27" --assume="28" --assume="-29" --assume="-30" --assume="-31" --assume="-32" --assume="-33" --assume="-34" --assume="-35" --assume="36" --assume="37" --assume="-38" --assume="-39" --assume="-40" --assume="-41" --assume="-42" --assume="43" --assume="44" --assume="-45" --assume="-46" --assume="-47" --assume="48" --assume="-49" --assume="-50" --assume="-51" --assume="-52" --assume="53" --assume="54" --assume="-55" --assume="56" --assume="-57" --assume="-58" --assume="-59" --assume="60" --assume="-61" --assume="62" --assume="63" --assume="-64" --assume="65" --assume="-66" --assume="67" --assume="68" ${srcdir-.}/trans.cnf.gz
test $? -eq $SAT
