#pragma once

#include <string>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/random_generator.hpp>

using namespace std;

#ifndef UTILS_H
#define UTILS_H

inline boost::uuids::random_generator gen_uuid;
bool is_valid_uuid(const string& str, boost::uuids::uuid& result);

#endif //UTILS_H
