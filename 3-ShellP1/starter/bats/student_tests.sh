#!/usr/bin/env bats

# File: student_tests.sh
# 
# Create your unit tests suit in this file

@test "Example: check ls runs without errors" {
    run ./dsh <<EOF                
ls
EOF

    # Assertions
    [ "$status" -eq 0 ]
}

@test "Simple pipe test: ls | grep dsh" {
    run ./dsh <<EOF
ls | grep dsh
exit
EOF

    # Assertions
    [ "$status" -eq 0 ]
    [[ "$output" == *"dsh"* ]]
}

@test "Multiple pipe test: ls | grep dsh | wc -l" {
    run ./dsh <<EOF
ls | grep dsh | wc -l
exit
EOF

    # Assertions
    [ "$status" -eq 0 ]
    [[ "$output" =~ [0-9]+ ]]
}

@test "Error handling: invalid command in pipe" {
    run ./dsh <<EOF
ls | nonexistentcommand | wc -l
exit
EOF

    # Assertions
    [ "$status" -eq 0 ]
    [[ "$output" == *"command not found"* ]]
}

@test "Too many pipes error" {
    run ./dsh <<EOF
cmd1 | cmd2 | cmd3 | cmd4 | cmd5 | cmd6 | cmd7 | cmd8 | cmd9
exit
EOF

    # Assertions
    [ "$status" -eq 0 ]
    [[ "$output" == *"piping limited to"* ]]
}

@test "Built-in command with pipe should execute as normal command" {
    run ./dsh <<EOF
echo test | cd /
exit
EOF

    # Assertions
    [ "$status" -eq 0 ]
    [[ "$output" == *"test"* ]]
}

@test "Input redirection test" {
    echo "hello world" > test_input.txt
    run ./dsh <<EOF
cat < test_input.txt
exit
EOF

    # Assertions
    [ "$status" -eq 0 ]
    [[ "$output" == *"hello world"* ]]
    rm -f test_input.txt
}

@test "Output redirection test" {
    run ./dsh <<EOF
echo "hello, class" > test_output.txt
cat test_output.txt
exit
EOF

    # Assertions
    [ "$status" -eq 0 ]
    [[ "$output" == *"hello, class"* ]]
    rm -f test_output.txt
}

@test "Combined pipe and redirection test" {
    run ./dsh <<EOF
echo "hello world" | grep "hello" > test_combined.txt
cat test_combined.txt
exit
EOF

    # Assertions
    [ "$status" -eq 0 ]
    [[ "$output" == *"hello world"* ]]
    rm -f test_combined.txt
}

@test "Append redirection test" {
    run ./dsh <<EOF
echo "hello, class" > test_append.txt
echo "this is line 2" >> test_append.txt
cat test_append.txt
exit
EOF

    # Assertions
    [ "$status" -eq 0 ]
    [[ "$output" == *"hello, class"* ]]
    [[ "$output" == *"this is line 2"* ]]
    rm -f test_append.txt
}