#!/bin/bash

set -x

while true
do
    DATETIME=$(date '+%Y%m%d-%H%M%S')
    LOG_FILE=/root/most/data/log/ultimate0-${DATETIME}.log
    mkdir -p $(dirname $LOG_FILE)

    ./build/ultimate 0 ultimate0_info.txt &>$LOG_FILE

    sleep 1
done
