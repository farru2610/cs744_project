#!/bin/bash

NUM_CLIENTS=$1
LOG_FILE="server_performance.log"
TASK_LOG_FILE="client_output.log"

echo "Starting performance monitoring..." | tee -a $LOG_FILE

START_TIME=$(date +%s.%N)

echo "Simulating $NUM_CLIENTS clients..."
./loadgen.sh $NUM_CLIENTS > $TASK_LOG_FILE

END_TIME=$(date +%s.%N)
ELAPSED_TIME=$(echo "$END_TIME - $START_TIME" | bc)

if (( $(echo "$ELAPSED_TIME == 0" | bc -l) )); then
    ELAPSED_TIME=0.001 
fi

COMPLETED_TASKS=$(grep -c "Result Recieved from Scheduler" $TASK_LOG_FILE)

if [[ $COMPLETED_TASKS -eq 0 ]]; then
    echo "No tasks completed yet." | tee -a $LOG_FILE
    THROUGHPUT=0
else
    echo "$COMPLETED_TASKS tasks completed successfully." | tee -a $LOG_FILE
    THROUGHPUT=$(echo "scale=2; $COMPLETED_TASKS / $ELAPSED_TIME" | bc)
fi

echo "Time Taken: $ELAPSED_TIME seconds" | tee -a $LOG_FILE
echo "Client Throughput: $THROUGHPUT requests per second" | tee -a $LOG_FILE

echo "Performance metrics logged to $LOG_FILE."
