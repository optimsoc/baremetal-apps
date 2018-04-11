#!/bin/bash

echo "l gap_min gap_avg gap_max egops_min egops_avg egops_max egutil_avg os_min os_avg os_max waits_min waits_avg waits_max tp_avg or_min or_avg or_max waitr_min waitr_avg waitr_max lat_min lat_avg lat_max retry_avg act_avg" > results.txt

for i in $(seq 0 8); do
    make clean && make measure SIZE=$i
    ./evaluate.py ctf* $i >> results.txt
done
