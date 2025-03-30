#include <iostream>

#include "client.h"

using namespace std;

namespace database
{
    vector<vector<string>> client::executeQuery(sqlite3* db, const string& query)
    {
        char* errMsg = nullptr;
        vector<vector<string>> results;

        if (sqlite3_exec(db, query.c_str(), [](void* data, const int argc, char** argv, char** colName) -> int
        {
            auto* res = static_cast<vector<vector<string>>*>(data);
            vector<string> row;
            row.reserve(argc);
            for (int i = 0; i < argc; i++)
            {
                row.emplace_back(argv[i] ? argv[i] : "NULL");
            }
            res->push_back(row);
            return 0;
        }, &results, &errMsg) != SQLITE_OK)
        {
            cerr << "Error executing query: " << errMsg << endl;
            sqlite3_free(errMsg);
        }

        return results;
    }

    vector<vector<string>> client::query(const string& query)
    {
        sqlite3* db;
        if (sqlite3_open("database.db", &db))
        {
            cerr << "Can't open database: " << sqlite3_errmsg(db) << endl;
            exit(EXIT_FAILURE);
        }
        const vector<vector<string>> query_results = executeQuery(db, query);
        sqlite3_close(db);

        return query_results;
    }
}
