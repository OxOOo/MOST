#!/bin/bash

set -x

while true
do
    DATETIME=$(date '+%Y%m%d-%H%M%S')
    LOG_FILE=/root/most/data/log/ultimate1-${DATETIME}.log
    mkdir -p $(dirname $LOG_FILE)

    ./build/ultimate 1 ultimate1_info.txt &>$LOG_FILE

    sleep 1
done
