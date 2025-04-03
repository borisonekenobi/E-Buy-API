#pragma once

#include <string>
#include <vector>
#include <sqlite3.h>

using namespace std;

namespace database::client
{
    vector<vector<string>> query(const string& query, const vector<string>& params);

    sqlite3* open_connection();

    // Close the shared connection
    void close_connection(sqlite3* db);

    // Query using an existing connection (shared transaction context)
    vector<vector<string>> transactional_query(sqlite3* db, const string& query, const vector<string>& params);
}
