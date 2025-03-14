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

@test "Concurrent clients (threaded mode)" {
    # Kill any existing server and start a new one in threaded mode
    if [ -n "$SERVER_PID" ]; then
        kill $SERVER_PID 2>/dev/null || true
    fi
    
    # Start server in threaded mode
    ./dsh -s -i 0.0.0.0 -p 5678 -x > server.log 2>&1 &
    SERVER_PID=$!
    sleep 1

    # Start first client that sleeps
    ./dsh -c -i 127.0.0.1 -p 5678 > client1.out 2>&1 <<EOF &
sleep 3
echo "client1 done"
exit
EOF
    CLIENT1_PID=$!

    # Start second client immediately
    ./dsh -c -i 127.0.0.1 -p 5678 > client2.out 2>&1 <<EOF
echo "client2 done"
exit
EOF

    # Wait for first client to finish
    wait $CLIENT1_PID

    # Check that client2 finished before client1 (concurrent execution)
    grep -q "client2 done" client2.out
    grep -q "client1 done" client1.out

    # Verify timestamps to ensure concurrent execution
    TIMESTAMP1=$(stat -f "%m" client1.out)
    TIMESTAMP2=$(stat -f "%m" client2.out)
    [ $TIMESTAMP2 -lt $TIMESTAMP1 ]

    # Clean up
    rm -f client1.out client2.out
}

@test "Multiple concurrent clients (threaded mode)" {
    # Kill any existing server and start a new one in threaded mode
    if [ -n "$SERVER_PID" ]; then
        kill $SERVER_PID 2>/dev/null || true
    fi
    
    # Start server in threaded mode
    ./dsh -s -i 0.0.0.0 -p 5678 -x > server.log 2>&1 &
    SERVER_PID=$!
    sleep 1

    # Array to store client PIDs
    CLIENT_PIDS=()

    # Start multiple clients simultaneously
    for i in {1..5}; do
        ./dsh -c -i 127.0.0.1 -p 5678 > "client${i}.out" 2>&1 <<EOF &
echo "client${i} running"
sleep $((1 + RANDOM % 2))
echo "client${i} done"
exit
EOF
        # Store the PID of the background process
        CLIENT_PIDS+=($!)
    done

    # Wait for all clients with a timeout
    TIMEOUT=10
    START_TIME=$SECONDS
    while true; do
        # Check if all processes are done
        ALL_DONE=true
        for pid in "${CLIENT_PIDS[@]}"; do
            if kill -0 $pid 2>/dev/null; then
                ALL_DONE=false
                break
            fi
        done

        if $ALL_DONE; then
            break
        fi

        # Check for timeout
        if [ $((SECONDS - START_TIME)) -gt $TIMEOUT ]; then
            echo "Timeout waiting for clients"
            # Kill remaining processes
            for pid in "${CLIENT_PIDS[@]}"; do
                kill $pid 2>/dev/null || true
            done
            return 1
        fi

        sleep 0.1
    done

    # Check all clients completed successfully
    for i in {1..5}; do
        if [ ! -f "client${i}.out" ]; then
            echo "client${i}.out does not exist"
            return 1
        fi
        if ! grep -q "client${i} running" "client${i}.out"; then
            echo "client${i} did not start"
            return 1
        fi
        if ! grep -q "client${i} done" "client${i}.out"; then
            echo "client${i} did not finish"
            return 1
        fi
    done

    # Clean up
    rm -f client*.out
}