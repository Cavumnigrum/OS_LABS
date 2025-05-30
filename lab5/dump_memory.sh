#!/bin/bash

MAPS_DIR="/home/stasyuk/labs/lab5/maps"

PID=$(pgrep memory_test)

if [ ! -z "$PID" ]; then
    /home/stasyuk/labs/lab5/memory_map "$PID" "$MAPS_DIR"
else
    echo "memory_test not running"
fi
