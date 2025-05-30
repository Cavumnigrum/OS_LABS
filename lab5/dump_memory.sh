#!/bin/bash

MAPS_DIR="/home/vinogradov/OS_vinogradov/labs/OS_LABS/lab5/maps"

PID=$(pgrep memory_test)

if [ ! -z "$PID" ]; then
    /home/vinogradov/OS_vinogradov/labs/OS_LABS/lab5/memory_map "$PID" "$MAPS_DIR"
else
    echo "memory_test not running"
fi
