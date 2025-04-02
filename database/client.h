#pragma once

#include <sqlite3.h>
#include <string>
#include <vector>

using namespace std;

namespace database
{
    class client
    {
        static vector<vector<string>> executeQuery(sqlite3* db, const string& query);
        static string prepare_query(const string& query, const vector<string>& params);

    public:
        static vector<vector<string>> query(const string& query, const vector<string>& params);
    };
}
