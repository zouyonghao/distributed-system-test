../../build/fuzz/fuzzer -b -i input -o output -M fuzzer1 -f init_random.txt \
    ../../build/etcd_test/etcd_test_main \
    --node_count=5 \
    --normal_count=5 --critic_count=3 \
    --no_fuzz_normal_events --no_fuzz_time \
    --check_after_fuzz=true
