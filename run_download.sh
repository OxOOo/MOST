#!/bin/bash

set -x

while true
do
    DATETIME=$(date '+%Y%m%d-%H%M%S')
    LOG_FILE=/root/most/data/log/download-${DATETIME}.log
    mkdir -p $(dirname $LOG_FILE)

    ./build/download 2>&1 | tee $LOG_FILE

    sleep 1
done
