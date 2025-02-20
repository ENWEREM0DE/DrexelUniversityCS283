#!/usr/bin/env bats

# File: student_tests.sh
# 
# Create your unit tests suit in this file

# Setup and teardown helpers
setup() {
    TEST_DIR="$(mktemp -d)"
    mkdir -p "$TEST_DIR/subdir"
    ORIG_DIR="$(pwd)"
    DSH_PATH="$ORIG_DIR/dsh"  # Store full path to dsh
    cd "$TEST_DIR"
}

teardown() {
    cd "$ORIG_DIR"
    rm -rf "$TEST_DIR"
}

# Basic command execution tests
@test "execute simple external command" {
    run "$DSH_PATH" <<EOF
ls
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" =~ "subdir" ]]
}

@test "execute command with multiple arguments" {
    run "$DSH_PATH" <<EOF
ls -la subdir
exit
EOF
    [ "$status" -eq 0 ]
}

# Empty input and whitespace tests
@test "handle empty input" {
    run "$DSH_PATH" <<EOF

exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" =~ "warning: no commands provided" ]]
}

@test "handle whitespace-only input" {
    run "$DSH_PATH" <<EOF
   
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" =~ "warning: no commands provided" ]]
}

# Quote handling tests
@test "preserve spaces in quoted strings" {
    run "$DSH_PATH" <<EOF
echo "  hello   world  "
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" =~ "  hello   world  " ]]
}

@test "handle multiple quoted arguments" {
    run "$DSH_PATH" <<EOF
echo "first arg" "second   arg" "third"
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" =~ "first arg second   arg third" ]]
}

# CD command tests
@test "cd with no arguments stays in same directory" {
    run "$DSH_PATH" <<EOF
pwd
cd
pwd
exit
EOF
    [ "$status" -eq 0 ]
    first_pwd=$(echo "$output" | grep "/tmp" | head -n1)
    second_pwd=$(echo "$output" | grep "/tmp" | tail -n1)
    [ "$first_pwd" = "$second_pwd" ]
}

@test "cd to valid directory" {
    run "$DSH_PATH" <<EOF
cd subdir
pwd
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" =~ "subdir" ]]
}

@test "cd to invalid directory" {
    run "$DSH_PATH" <<EOF
cd nonexistent
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" =~ "cd failed" ]]
}

# Error handling tests
@test "handle nonexistent command" {
    run "$DSH_PATH" <<EOF
nonexistentcommand
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" =~ "command not found in PATH" ]]
}

@test "handle command with invalid arguments" {
    run "$DSH_PATH" <<EOF
ls --invalid-flag
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" =~ "unrecognized option" ]]
}

# Exit command test
@test "exit command works" {
    run "$DSH_PATH" <<EOF
exit
EOF
    [ "$status" -eq 0 ]
}

# Multiple space handling
@test "collapse multiple spaces between arguments" {
    run "$DSH_PATH" <<EOF
echo hello     world
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" =~ "hello world" ]]
}

# Complex command tests
@test "handle complex command with quotes and multiple args" {
    run "$DSH_PATH" <<EOF
echo "quoted arg" normal-arg "spaced   arg" last
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" =~ "quoted arg normal-arg spaced   arg last" ]]
}

@test "command not found error handling" {
    run "$DSH_PATH" <<EOF
nonexistentcommand
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" =~ "command not found in PATH" ]]
}

@test "permission denied error handling" {
    touch testfile
    chmod -x testfile
    run "$DSH_PATH" <<EOF
./testfile
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" =~ "permission denied" ]]
    rm testfile
}

@test "rc command shows last return code" {
    run "$DSH_PATH" <<EOF
nonexistentcommand
rc
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" =~ "2" ]]  # ENOENT is usually 2
}

@test "rc command shows success" {
    run "$DSH_PATH" <<EOF
true
rc
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" =~ "0" ]]
}