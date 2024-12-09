#!/bin/bash

executable="./bus"
output_file="result.txt"

> "$output_file"

for i in $(seq 1 100); do
    echo "Run #$i" >> "$output_file"
    "$executable" >> "$output_file"
    echo "" >> "$output_file"
done

echo "saved all 100 result to $output_file"