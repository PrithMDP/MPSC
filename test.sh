#!/bin/bash
num=1
i="0"
while [ $num -gt 0 ]
do
num=$(./a.out > dump.txt && tail -1 dump.txt | grep 100000 | wc -l)
echo 'running ' $i
i=$[$i+1]
done
