#pragma once

#include <string>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

using namespace std;

#ifndef UTILS_H
#define UTILS_H

static bool is_valid_uuid(const string& str, boost::uuids::uuid& result);

#endif //UTILS_H
