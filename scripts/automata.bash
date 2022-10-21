#!/bin/bash
while true; do (ulimit -v 16777216 -t 43200; ../automata --algorithm noUL >> automata_${1}.log); done
