#!/bin/bash

application='./build/Release/AlgoraApp'

# Array of test files
test_files=(
    # "c_answers.e")
     "test.txt" "test2.txt" "test3.txt" "test4.txt" "test5.txt")

# Debug file to store program output
debug_file="debug.txt"

# Clear the debug file at the start
> $debug_file

# Loop through each test file
for test_file in "${test_files[@]}"; do
    echo "Running tests for $test_file..."

    # Generate output file names
    hh_output="hh_${test_file%.txt}.csv"
    eh_output="eh_${test_file%.txt}.csv"
    esh_output="esh_${test_file%.txt}.csv"
    he_output="he_${test_file%.txt}.csv"
    ee_output="ee_${test_file%.txt}.csv"
    ese_output="ese_${test_file%.txt}.csv"
    static_output="static_${test_file%.txt}.csv"

    # Run hhh + epstab
    $application -i tests/$test_file -a hhh -p epstab -e 0.33 --all --graph_stats -g -o $he_output >> $debug_file 2>&1
    if [ $? -ne 0 ]; then
        echo "Error: Command failed for hhh/epstab with $test_file. Check $debug_file for details."
        exit 1
    fi

    # Run hhh + hindex
    $application -i tests/$test_file -a hhh -p hindex -e 0.33 --all --graph_stats -g -o $hh_output >> $debug_file 2>&1
    if [ $? -ne 0 ]; then
        echo "$application -i tests/$test_file -a hhh -p hindex -e 0.33 --all --graph_stats -g -o $hh_output >> $debug_file 2>&1"
        echo "Error: Command failed for hhh/hindex with $test_file. Check $debug_file for details."
        exit 1
    fi

    # Run egst + hindex
    $application -i tests/$test_file -a egst -p hindex -e 0.33 --all --graph_stats -g -o $eh_output >> $debug_file 2>&1
    if [ $? -ne 0 ]; then
        echo "Error: Command failed for egst/hindex with $test_file. Check $debug_file for details."
        exit 1
    fi

     # Run egst + epstab
    $application -i tests/$test_file -a egst -p epstab -e 0.33 --all --graph_stats -g -o $ee_output >> $debug_file 2>&1
    if [ $? -ne 0 ]; then
        echo "Error: Command failed for egst/epstab with $test_file. Check $debug_file for details."
        exit 1
    fi

    # Run egst_soft + hindex
    $application -i tests/$test_file -a egst -p hindex -e 0.33 --all --graph_stats -g -o $esh_output --egst-direct --egst-highAnchorsOnly >> $debug_file 2>&1
    if [ $? -ne 0 ]; then
        echo "Error: Command failed for egst/hindex with $test_file. Check $debug_file for details."
        exit 1
    fi

     # Run  egst_soft + epstab
    $application -i tests/$test_file -a egst -p epstab -e 0.33 --all --graph_stats -g -o $ese_output --egst-direct --egst-highAnchorsOnly >> $debug_file 2>&1
    if [ $? -ne 0 ]; then
        echo "Error: Command failed for egst/epstab with $test_file. Check $debug_file for details."
        exit 1
    fi

    # Run static
    $application -i tests/$test_file -a oba -p none --all --graph_stats -g -o $static_output >> $debug_file 2>&1
    if [ $? -ne 0 ]; then
        echo "Error: Command failed for oba/none with $test_file. Check $debug_file for details."
        exit 1
    fi

    # Compare the two output files and redirect diff output to debug file
    diff $he_output $eh_output >> $debug_file 2>&1
    if [ $? -ne 0 ]; then
        echo "Error: Output files $he_output and $eh_output differ for $test_file. Check $debug_file for details."
        exit 1
    fi

    # Compare the two output files and redirect diff output to debug file
    diff $eh_output $ee_output >> $debug_file 2>&1
    if [ $? -ne 0 ]; then
        echo "Error: Output files $eh_output and $eh_output differ for $test_file. Check $debug_file for details."
        exit 1
    fi

    # Compare the two output files and redirect diff output to debug file
    diff $he_output $hh_output >> $debug_file 2>&1
    if [ $? -ne 0 ]; then
        echo "Error: Output files $he_output and $hh_output differ for $test_file. Check $debug_file for details."
        exit 1
    fi

    # Compare the two output files and redirect diff output to debug file
    diff $he_output $static_output >> $debug_file 2>&1
    if [ $? -ne 0 ]; then
        echo "Error: Output files $he_output and $static_output differ for $test_file. Check $debug_file for details."
        exit 1
    fi

    # Compare the two output files and redirect diff output to debug file
    diff $he_output $esh_output >> $debug_file 2>&1
    if [ $? -ne 0 ]; then
        echo "Error: Output files $he_output and $esh_output differ for $test_file. Check $debug_file for details."
        exit 1
    fi

     # Compare the two output files and redirect diff output to debug file
    diff $he_output $ese_output >> $debug_file 2>&1
    if [ $? -ne 0 ]; then
        echo "Error: Output files $he_output and $ese_output differ for $test_file. Check $debug_file for details."
        exit 1
    fi

    # Remove the output files if no differences are found
    rm -f $he_output $eh_output $ee_output $hh_output $static_output $ese_output $esh_output $debug_file
    echo "Test passed for $test_file: No differences found, output files removed."
done

echo "All tests passed successfully, and all output files have been removed!"