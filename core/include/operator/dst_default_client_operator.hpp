#ifndef DST_DEFAULT_CLIENT_OPERATOR_HEADER
#define DST_DEFAULT_CLIENT_OPERATOR_HEADER

#include <dst_configuration_generator.hpp>
#include <dst_event.h>
#include <dst_random.h>
#include <operator/dst_default_operator.hpp>
#include <util/common_uils.hpp>

#include <limits>
#include <map>
#include <sstream>

#include <boost/process.hpp>

enum OP_NAME
{
    OP_READ,
    OP_WRITE,
    OP_CAS
};

static const std::string OP_NAME_STR[] = {"read", "write", "cas"};

/* The ClientConfigurationGenerator for each system
 * return an empty string if some operations are not supported
 */
class ClientConfigurationGenerator
{
public:
    virtual std::string get_configure_string(OP_NAME op_name, uint32_t node_count, ...) = 0;
};

/* The default client operator
 * Usually we support three operations: READ, WRITE, CAS for each system
 */
class DefaultClientOperator : public NormalOperator
{
protected:
    ClientConfigurationGenerator *configuration_generator;
    OP_NAME op_name;

public:
    DefaultClientOperator(OP_NAME _op_name, ClientConfigurationGenerator *_configuration_generator)
        : configuration_generator(_configuration_generator), op_name(_op_name)
    {
    }

    virtual bool _do();
};

#endif // DST_DEFAULT_CLIENT_OPERATOR_HEADER
