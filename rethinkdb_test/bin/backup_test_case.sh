mkdir -p test_cases/$1

pwd

mv log*  ./test_cases/$1
mv rethinkdb_data*  ./test_cases/$1
mv random_node0.txt ./test_cases/$1
mv random_node1.txt ./test_cases/$1
mv random_node2.txt ./test_cases/$1
mv random_proxy.txt ./test_cases/$1
mv random.txt ./test_cases/$1
mv operation_log ./test_cases/$1

# checking the operation log!
{ \
    ./check.sh ${PWD}/test_cases/$1/operation_log > ${PWD}/test_cases/$1/check_log 2>&1; \
    if grep -q ^E ./test_cases/$1/log*; then echo "Find errors!"; \
    elif grep -q Backtrace ./test_cases/$1/log*; then echo "May find error trace!"; \
    elif grep -q "ERROR: AddressSanitizer"; then echo "Find ASan errors!"; \
    elif grep -q true ./test_cases/$1/check_log; then echo "No errors, deleting logs..."; \
        rm -rf ./test_cases/$1; \
    else \
        echo "Find operation log errors!"; \
    fi \
} &
