#include <operator/dst_default_client_operator.hpp>

std::string get_last_output(boost::process::ipstream &pipe_stream)
{
    /* the last output should be the result of read */
    std::string last_output;
    std::string tmp;
    while (pipe_stream && std::getline(pipe_stream, tmp) && !tmp.empty())
    {
        // std::cout << tmp << "\n";
        last_output = tmp;
    }
    return last_output;
}

bool DefaultClientOperator::_do()
{
    int random_thread_id = random() % INT_MAX;

    std::string command;
    uint32_t random_num1;
    uint32_t random_num2;
    std::string invoke_string =
        "{:process " + std::to_string(random_thread_id) + ", :type :invoke, :f :" + OP_NAME_STR[op_name] + ", :value ";
    switch (op_name)
    {
    case OP_READ:
        command = configuration_generator->get_read_configure_string(node_count);
        invoke_string += "nil";
        break;
    case OP_WRITE:
        random_num1 = __dst_get_random_uint32();
        command = configuration_generator->get_write_configure_string(node_count, random_num1);
        invoke_string += std::to_string(random_num1);
        break;
    case OP_CAS:
        random_num1 = __dst_get_random_uint32();
        random_num2 = __dst_get_random_uint32();
        command = configuration_generator->get_cas_configure_string(node_count, random_num1, random_num2);
        invoke_string += "[" + std::to_string(random_num1) + " " + std::to_string(random_num2) + "]";
        break;
    }
    invoke_string += "}";
    __dst_event_record(invoke_string.c_str());

    std::cerr << command << "\n";

    try
    {
        boost::process::ipstream pipe_stream;
        boost::process::child c(command, boost::process::std_out > pipe_stream);
        c.wait();
        int result = c.exit_code();
        /* timeout, killed or force killed */
        if (result == 124 || result == 143 || result == 137)
        {
            return result;
        }

        std::string result_string = "{:process " + std::to_string(random_thread_id) + ", :type " +
                                    std::string(result == 0 ? ":ok" : ":fail") + ", :f :" + OP_NAME_STR[op_name] +
                                    ", :value ";
        switch (op_name)
        {
        case OP_READ:
            result_string += std::to_string(std::stoll(get_last_output(pipe_stream)));
            break;
        case OP_WRITE:
            result_string += std::to_string(random_num1);
            break;
        case OP_CAS:
            result_string += "[" + std::to_string(random_num1) + " " + std::to_string(random_num2) + "]";
            break;
        }
        result_string += "}";
        __dst_event_record(result_string.c_str());
        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Fail to spawn a child process." << '\n';
        std::cerr << e.what() << '\n';
        return false;
    }
}