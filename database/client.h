#pragma once

#include <sqlite3.h>
#include <string>
#include <vector>

using namespace std;

namespace database::client
{
    vector<vector<std::string>> query(const string& query, const vector<string>& params);
    sqlite3* open_connection();
    void close_connection(sqlite3* db);
    vector<vector<string>> transactional_query(sqlite3* db, const string& query, const vector<string>& params);
}
