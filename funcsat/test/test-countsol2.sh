#!/bin/sh

../funcsat --quiet --live-st=no --count-solutions="2,6,13,17,20,24,29,33,38,43,46" --count-solutions="2" ${srcdir-.}/trans.cnf.gz | grep '^s 1$'

