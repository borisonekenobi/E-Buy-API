#pragma once

#include <sqlite3.h>
#include <string>
#include <vector>

using namespace std;

namespace database::client
{
    vector<vector<string>> query(const string& query, const vector<string>& params);
}
