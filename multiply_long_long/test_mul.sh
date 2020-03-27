#!/bin/bash
for (( t = 1; t < 1000; t++ ))
do
    python3 random_mul_input.py > inp.txt || break
    ./mul < inp.txt > my_ans.txt || break
    python3 mul.py < inp.txt > corr_ans.txt || break
    if cmp -s my_ans.txt corr_ans.txt ; then
        echo $t OK
    else
        break
    fi
done
