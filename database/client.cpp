#include "client.h"

#include <format>
#include <iostream>
#include <sqlite3.h>

using namespace std;

namespace database::client
{
    vector<vector<string>> executeQuery(sqlite3* db, const string& query)
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

    string prepare_query(const string& query, const vector<string>& params)
    {
        string prepared_query = query;
        for (int i = 0; i < params.size(); i++)
        {
            const string& param = params[i];
            if (const size_t pos = prepared_query.find(format("${}", i + 1)); pos != string::npos)
            {
                prepared_query.replace(pos, 2, "'" + param + "'");
            }
        }
        return prepared_query;
    }

    // Executes a query on the database and returns the results.
    // WARNING: This function is not safe from SQL injection attacks.
    bool query(const string& query, const vector<string>& params, vector<vector<string>>& results)
    {
        try
        {
            const string prepared_query = prepare_query(query, params);

            sqlite3* db;
            if (sqlite3_open("database.db", &db))
            {
                cerr << "Can't open database: " << sqlite3_errmsg(db) << endl;
                exit(EXIT_FAILURE);
            }
            results = executeQuery(db, prepared_query);
            sqlite3_close(db);

            return true;
        }
        catch (const exception& e)
        {
            cerr << "Error executing query: " << e.what() << endl;
            return false;
        }
    }

    //transactions

    sqlite3* open_connection()
    {
        sqlite3* db;
        if (sqlite3_open("database.db", &db))
        {
            cerr << "Can't open database: " << sqlite3_errmsg(db) << endl;
            exit(EXIT_FAILURE);
        }
        return db;
    }

    void close_connection(sqlite3* db)
    {
        sqlite3_close(db);
    }

    vector<vector<string>> transactional_query(sqlite3* db, const string& query, const vector<string>& params)
    {
        const string prepared_query = prepare_query(query, params);
        return executeQuery(db, prepared_query);
    }
}
