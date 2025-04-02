#include <boost/uuid/string_generator.hpp>

#include "utils.h"

using namespace std;

bool is_valid_uuid(const string& str, boost::uuids::uuid& result)
{
    try
    {
        result = boost::uuids::string_generator()(str);
        return result.version() != boost::uuids::uuid::version_unknown;
    }
    catch (...)
    {
        return false;
    }
}
