#!/bin/bash

# Define the list of run numbers
run_numbers=(30 31 32 33 34 35 36 37)

# Define a function for parallel to use
run_exec() {
    ../build/data_inspection -r "$1"
}

# Export the function
export -f run_exec

# Use parallel to run the function
echo "${run_numbers[@]}" | tr ' ' '\n' | parallel -j4 run_exec
