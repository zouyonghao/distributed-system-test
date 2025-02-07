#include <aerospike/aerospike.h>
#include <aerospike/aerospike_key.h>

#include <operator/dst_default_client_operator.hpp>
#include <utils/dst_common_util.hpp>

#include <dst_event.h>
#include <dst_random.h>

#define SERVICE_BASE_PORT 2000

static bool example_log_callback(as_log_level level, const char *func, const char *file, uint32_t line, const char *fmt,
                                 ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    return true;
}

class AerospikeClient : public NormalOperator
{
public:
    OP_NAME op_name;

    as_config config;
    aerospike as;
    as_key key;

    bool is_initialized = false;

    AerospikeClient(OP_NAME _op_name) : op_name(_op_name) {}

    void init()
    {
        as_log_set_level(AS_LOG_LEVEL_DEBUG);
        as_log_set_callback(example_log_callback);
        as_config_init(&config);
        for (int i = 0; i < node_count; i++)
        {
            as_config_add_host(&config, "127.0.1.1", SERVICE_BASE_PORT + i);
        }
        aerospike_init(&as, &config);
        as_key_init_str(&key, "test", "test-set", "test-key");
        is_initialized = true;
    }

    bool _do() override
    {
        if (!is_initialized)
        {
            init();
        }

        int random_thread_id = random() % INT_MAX;
        as_error err;
        if (aerospike_connect(&as, &err) != AEROSPIKE_OK)
        {
            fprintf(stderr, "err(%d) %s at [%s:%d]\n", err.code, err.message, err.file, err.line);
            return false;
        }

        fprintf(stderr, "connect success!\n");

        std::string invoke_record_string = "{:process " + std::to_string(random_thread_id) +
                                           ", :type :invoke, :f :" + OP_NAME_STR[op_name] + ", :value ";

        std::string result_record_string = "{:process " + std::to_string(random_thread_id) + ", :type " + ":ok" +
                                           ", :f :" + OP_NAME_STR[op_name] + ", :value ";
        switch (op_name)
        {
        case OP_READ:
        {

            invoke_record_string = invoke_record_string + "nil" + "}";
            __dst_event_record(invoke_record_string.c_str());

            as_record *p_rec = NULL;
            static const char *bins[] = {"test-bin", NULL};
            if (aerospike_key_select(&as, &err, NULL, &key, bins, &p_rec) != AEROSPIKE_OK || p_rec == NULL)
            {
                fprintf(stderr, "aerospike_key_get() returned %d - %s\n", err.code, err.message);
                // __dst_event_record(get_result_record(op_name, op_vector, -1, "FAIL", random_thread_id).c_str());
                return false;
            }
            char *val_as_str = as_val_tostring(as_bin_get_value(p_rec->bins.entries));
            printf("read value is %s\n", val_as_str);

            result_record_string = result_record_string + val_as_str + "}";
            __dst_event_record(result_record_string.c_str());

            free(val_as_str);
            as_record_destroy(p_rec);
            break;
        }
        case OP_WRITE:
        {
            uint32_t random = __dst_get_random_uint32_t();

            as_record rec;
            as_record_inita(&rec, 1);
            printf("write value is %d\n", random);
            as_record_set_int64(&rec, "test-bin", random);

            as_policy_write wpol;
            as_policy_write_init(&wpol);
            wpol.exists = AS_POLICY_EXISTS_CREATE_OR_REPLACE;

            invoke_record_string = invoke_record_string + std::to_string(random) + "}";
            __dst_event_record(invoke_record_string.c_str());

            if (aerospike_key_put(&as, &err, &wpol, &key, &rec) != AEROSPIKE_OK)
            {
                fprintf(stderr, "err(%d) %s at [%s:%d]\n", err.code, err.message, err.file, err.line);

                if (err.code != AEROSPIKE_ERR_TIMEOUT)
                {
                    printf("write failed!\n");
                    // __dst_event_record(get_result_record(op_name, op_vector, -1, "FAIL", random_thread_id).c_str());
                }
                else
                {
                    printf("write timeout!\n");
                }
                return false;
            }

            result_record_string = result_record_string + std::to_string(random) + "}";
            __dst_event_record(result_record_string.c_str());

            as_record_destroy(&rec);
            break;
        }
        case OP_CAS:
            // TODO
            break;
        }

        sleep(1);
        return true;
    }

    ~AerospikeClient() { aerospike_destroy(&as); }
};

/** The InitOperator will try to set until the read is success */
class AerospikeInitOperator : public AerospikeClient
{
public:
    int MAX_TRY_COUNT = 5;
    AerospikeInitOperator() : AerospikeClient(OP_WRITE) {}
    bool _do() override
    {
        if (!is_initialized)
        {
            init();
        }

        std::string invoke_record_string = "{:process 0, :type :invoke, :f :" + OP_NAME_STR[OP_WRITE] + ", :value 0}";
        __dst_event_record(invoke_record_string.c_str());

        int count = 0;
        while (count < MAX_TRY_COUNT)
        {
            as_error err;
            if (aerospike_connect(&as, &err) != AEROSPIKE_OK)
            {
                fprintf(stderr, "err(%d) %s at [%s:%d]\n", err.code, err.message, err.file, err.line);
                count++;
                usleep(500 * 1e3); // 500ms
                continue;
            }
            as_record rec;
            as_record_inita(&rec, 1);
            as_record_set_int64(&rec, "test-bin", 0);
            as_policy_write wpol;
            as_policy_write_init(&wpol);
            wpol.exists = AS_POLICY_EXISTS_CREATE_OR_REPLACE;
            wpol.base.total_timeout = 200; // 200ms
            bool success = true;
            if (aerospike_key_put(&as, &err, &wpol, &key, &rec) != AEROSPIKE_OK)
            {
                fprintf(stderr, "err(%d) %s at [%s:%d]\n", err.code, err.message, err.file, err.line);

                if (err.code != AEROSPIKE_ERR_TIMEOUT)
                {
                    printf("init failed!\n");
                }
                else
                {
                    printf("init timeout!\n");
                }
                success = false;
            }
            as_record_destroy(&rec);
            if (success)
            {
                break;
            }
            count++;
            usleep(500 * 1e3); // 500ms
        }
        if (count >= MAX_TRY_COUNT)
        {
            std::cerr << "Run init failed!\n";
            return false;
        }

        std::string result_record_string =
            "{:process 0, :type :ok, :f :" + OP_NAME_STR[OP_WRITE] + ", :value 0}";
        __dst_event_record(result_record_string.c_str());

        return true;
    }
};