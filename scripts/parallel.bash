#!/bin/bash
for i in $(seq 1 $1);
do
  ./automata.bash $i &
done
