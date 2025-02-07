#ifndef KEEPER_CONFIGURATION_GENERATOR_HEADER
#define KEEPER_CONFIGURATION_GENERATOR_HEADER

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <cstdarg>
#include <dst_node_manager.hpp>
#include <operator/dst_default_client_operator.hpp>

#define BIN_PATH "/usr/bin/clickhouse keeper"
#define BASE_ZK_PORT 9181
#define BASE_INNER_PORT 44444
#define BASE_XML_PATH "./enable_keeper.xml"
#define IP "127.0.1.1"

/* The ServerConfigurationGenerator for clickhouse-keeper, the example is as follows:
 * clickhouse-keeper --config enable_keeper1.xml
 */
class KeeperConfigurationGenerator : public ServerConfigurationGenerator
{
public:
    void init_xml_file(uint32_t node_id, uint32_t node_count)
    {
        namespace pt = boost::property_tree;
        pt::ptree tree;
        pt::read_xml(BASE_XML_PATH, tree);
        tree.put("yandex.logger.log", "run/run_log" + std::to_string(node_id));
        tree.put("yandex.logger.errorlog", "run/err_log" + std::to_string(node_id));
        tree.put("yandex.keeper_server.tcp_port", std::to_string(BASE_ZK_PORT + node_id));
        tree.put("yandex.keeper_server.server_id", std::to_string(node_id));
        tree.put("yandex.keeper_server.log_storage_path", "run/log" + std::to_string(node_id));
        tree.put("yandex.keeper_server.snapshot_storage_path", "run/snapshot" + std::to_string(node_id));

        pt::ptree servers;
        for (uint32_t i = 0; i < node_count; i++)
        {
            pt::ptree server;
            server.put("id", std::to_string(i));
            server.put("hostname", IP);
            server.put("port", std::to_string(BASE_INNER_PORT + i));
            server.put("can_become_leader", "true");
            server.put("priority", "1");
            servers.push_back(std::make_pair("server", server));
        }
        tree.put_child("yandex.keeper_server.raft_configuration", servers);
        pt::write_xml("enable_keeper" + std::to_string(node_id) + ".xml", tree);
    }
    std::string get_configure_string(uint32_t node_id, uint32_t node_count) override
    {
        std::string config = BIN_PATH " --config enable_keeper" + std::to_string(node_id) + ".xml";
        init_xml_file(node_id, node_count);
        return config;
    }

    std::string get_no_fault_files(uint32_t node_id) override { return ".xml,run/log,run/err_log"; }
};

/* The ClientConfigurationGenerator for clickhouse-keeper, the example is as follows:
 * /usr/share/zookeeper/bin/zkCli.sh -server 127.0.1.1:9181,127.0.1.1:9182,127.0.1.1:9183 get /a
 * /usr/share/zookeeper/bin/zkCli.sh -server 127.0.1.1:9181,127.0.1.1:9182,127.0.1.1:9183 set /a 1
 */
class KeeperClientConfigurationGenerator : public ClientConfigurationGenerator
{
public:
    std::string get_configure_string(OP_NAME op_name, uint32_t node_count, ...)
    {
        std::string configure_string = "timeout 3 /usr/share/zookeeper/bin/zkCli.sh -server ";
        for (uint32_t i = 0; i < node_count; i++)
        {
            configure_string += IP ":" + std::to_string(BASE_ZK_PORT + i);
            if (i < node_count - 1)
            {
                configure_string += ",";
            }
        }
        configure_string += " ";
        va_list random_nums;
        va_start(random_nums, node_count);
        switch (op_name)
        {
        case OP_READ:
            configure_string += "get /a";
            break;
        case OP_WRITE:
        {
            uint32_t value = va_arg(random_nums, uint32_t);
            configure_string += "set /a " + std::to_string(value);
            break;
        }
        case OP_CAS:
        {
            break;
        }
        default:
            break;
        }
        va_end(random_nums);
        sleep(1);
        return configure_string;
    }
};

class KeeperClientWriteWithVersionConfigurationGenerator : public ClientConfigurationGenerator
{
public:
    std::string get_configure_string(OP_NAME op_name, uint32_t node_count, ...)
    {
        std::string configure_string = "timeout 3 /usr/share/zookeeper/bin/zkCli.sh -server ";
        for (uint32_t i = 0; i < node_count; i++)
        {
            configure_string += IP ":" + std::to_string(BASE_ZK_PORT + i);
            if (i < node_count - 1)
            {
                configure_string += ",";
            }
        }
        configure_string += " ";
        va_list random_nums;
        va_start(random_nums, node_count);
        switch (op_name)
        {
        case OP_WRITE:
        {
            uint32_t value = va_arg(random_nums, uint32_t);
            /**
             * The version should not be too large
             * and can't be 0?
             */
            uint32_t version = __dst_get_random_uint8_t() % 3 + 1;
            configure_string += "set /a " + std::to_string(value) + " " + std::to_string(version);
            break;
        }
        default:
            break;
        }
        va_end(random_nums);
        sleep(1);
        return configure_string;
    }
};

#endif // KEEPER_CONFIGURATION_GENERATOR_HEADER