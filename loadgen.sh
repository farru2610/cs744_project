#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <number_of_clients>"
    exit 1
fi

NUM_CLIENTS=$1

for (( i=1; i<=NUM_CLIENTS; i++ ))
do
    RANDOM_NUM=$((RANDOM % 10 + 1))
    
    ./client $RANDOM_NUM &
done

wait

echo "All clients have finished."
