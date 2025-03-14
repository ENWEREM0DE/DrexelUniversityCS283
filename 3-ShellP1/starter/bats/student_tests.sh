#!/usr/bin/env bats

# File: student_tests.sh
# 
# Create your unit tests suit in this file

# Start server before each test
setup() {
    # Start server in background
    ./dsh -s -i 0.0.0.0 -p 5678 > server.log 2>&1 &
    SERVER_PID=$!
    
    # Give server time to start
    sleep 1
    
    # Check if server is running
    if ! ps -p $SERVER_PID > /dev/null; then
        echo "Server failed to start"
        return 1
    fi
}

# Clean up after each test
teardown() {
    # Kill server if it's still running
    if [ -n "$SERVER_PID" ]; then
        kill $SERVER_PID 2>/dev/null || true
    fi
    rm -f server.log
}

@test "Basic command execution" {
    run ./dsh -c -i 127.0.0.1 -p 5678 <<EOF
ls
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" =~ "dshlib.c" ]]
}

@test "Command with pipe" {
    run ./dsh -c -p 5678 <<EOF
ls | grep dshlib.c
exit
EOF
    [ "$status" -eq 0 ]
    [[ "${output}" =~ "dshlib.c" ]]
}

@test "Multiple pipe commands" {
    run ./dsh -c -p 5678 <<EOF
ls | grep .c | wc -l
exit
EOF
    [ "$status" -eq 0 ]
    [[ "${lines[1]}" =~ [0-9]+ ]]  # Should output a number
}

@test "Built-in cd command" {
    run ./dsh -c -p 5678 <<EOF
cd /tmp
pwd
exit
EOF
    [ "$status" -eq 0 ]
    [[ "${output}" =~ "/tmp" ]]
}

@test "Exit command" {
    run ./dsh -c -p 5678 <<EOF
exit
EOF
    [ "$status" -eq 0 ]
}

@test "Stop-server command" {
    # First client sends stop-server
    run ./dsh -c -i 127.0.0.1 -p 5678 <<EOF
stop-server
EOF
    [ "$status" -eq 0 ]

    # Give server time to fully stop
    sleep 2

    # Try to connect again - should fail as server is stopped
    run ./dsh -c -i 127.0.0.1 -p 5678 <<EOF
echo test
EOF
    
    # Print the actual status for debugging
    echo "Actual status: $status"
    
    # Check that it's a non-zero value (error)
    [ "$status" -ne 0 ]
}

@test "Command not found" {
    run ./dsh -c -p 5678 <<EOF
nonexistentcommand
exit
EOF
    [ "$status" -eq 0 ]
    [[ "${output}" =~ "not found" ]]
}

@test "Empty command" {
    run ./dsh -c -i 127.0.0.1 -p 5678 <<EOF

exit
EOF
    [ "$status" -eq 0 ]
    [[ "${output}" =~ "warning: no commands provided" ]]
}

@test "Long output handling" {
    run ./dsh -c -p 5678 <<EOF
yes "test" | head -1000
exit
EOF
    [ "$status" -eq 0 ]
    # Count number of "test" occurrences
    [[ $(echo "${output}" | grep -c "test") -eq 1000 ]]
}

@test "Multiple clients sequential" {
    # First client
    run ./dsh -c -p 5678 <<EOF
echo "client1"
exit
EOF
    [ "$status" -eq 0 ]
    [[ "${output}" =~ "client1" ]]

    # Second client
    run ./dsh -c -p 5678 <<EOF
echo "client2"
exit
EOF
    [ "$status" -eq 0 ]
    [[ "${output}" =~ "client2" ]]
}

@test "Return code command" {
    run ./dsh -c -p 5678 <<EOF
false
rc
exit
EOF
    [ "$status" -eq 0 ]
    [[ "${output}" =~ "1" ]]  # false command returns 1
}