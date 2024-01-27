/**
Copyright 2020-2024 JasmineGraph Team
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
**/

#include "StreamingSQLiteDBInterface.h"

#include <string>

#include "../util/Utils.h"
#include "../util/logger/Logger.h"

using namespace std;
Logger streamdb_logger;

int StreamingSQLiteDBInterface::init() {
    if (!Utils::fileExists(this->databaseLocation.c_str())) {
        if (Utils::createDatabaseFromDDL(this->databaseLocation.c_str(),
                                         ROOT_DIR "src/streamingdb/ddl.sql") != 0) {
            streamdb_logger.error("Cannot create database: " + databaseLocation);
            return -1;
        }
    }

    int rc = sqlite3_open(this->databaseLocation.c_str(), &database);
    if (rc) {
        streamdb_logger.error("Cannot open database: " + string(sqlite3_errmsg(database)));
        return (-1);
    } else {
        streamdb_logger.info("Database opened successfully : " + databaseLocation);
        return 0;
    }
}

int StreamingSQLiteDBInterface::finalize() { return sqlite3_close(database); }

StreamingSQLiteDBInterface::StreamingSQLiteDBInterface() {
    this->databaseLocation = Utils::getJasmineGraphProperty("org.jasminegraph.streaming.db.location");
}

StreamingSQLiteDBInterface::StreamingSQLiteDBInterface(string databaseLocation) {
    this->databaseLocation = databaseLocation;
}

typedef vector<vector<pair<string, string>>> table_type;

static int callback(void *ptr, int argc, char **argv, char **azColName) {
    int i;
    table_type *dbResults = static_cast<table_type *>(ptr);
    vector<pair<string, string>> results;

    for (i = 0; i < argc; i++) {
        results.push_back(make_pair(azColName[i], argv[i] ? argv[i] : "NULL"));
        // printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    dbResults->push_back(results);
    return 0;
}

vector<vector<pair<string, string>>> StreamingSQLiteDBInterface::runSelect(std::string query) {
    char *zErrMsg = 0;
    int rc;
    vector<vector<pair<string, string>>> dbResults;

    rc = sqlite3_exec(database, query.c_str(), callback, &dbResults, &zErrMsg);

    if (rc != SQLITE_OK) {
        streamdb_logger.error("SQL Error: " + string(zErrMsg));
        sqlite3_free(zErrMsg);
    } else {
        streamdb_logger.info("Operation done successfully");
        return dbResults;
    }
}

// This function inserts a new row to the DB and returns the last inserted row id
int StreamingSQLiteDBInterface::runInsert(std::string query) {
    char *zErrMsg = 0;
    int rc = sqlite3_exec(database, query.c_str(), NULL, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        streamdb_logger.error("SQL Error: " + string(zErrMsg));
        sqlite3_free(zErrMsg);
        return -1;
    } else {
        streamdb_logger.info("Insert operation done successfully");
        vector<vector<pair<string, string>>> dbResults;
        string q2 = "SELECT last_insert_rowid();";

        int rc2 = sqlite3_exec(database, q2.c_str(), callback, &dbResults, &zErrMsg);

        if (rc2 != SQLITE_OK) {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
            return -1;
        } else {
            return std::stoi(dbResults[0][0].second);
        }
    }
}

// This function inserts one or more rows of the DB and nothing is returned
// This is used for inserting tables which do not have primary IDs
void StreamingSQLiteDBInterface::runInsertNoIDReturn(std::string query) {
    char *zErrMsg = 0;
    int rc = sqlite3_exec(database, query.c_str(), NULL, NULL, &zErrMsg);
    if (rc != SQLITE_OK) {
        streamdb_logger.error("SQL Error: " + string(zErrMsg));
        sqlite3_free(zErrMsg);
    } else {
        streamdb_logger.info("Insert operation done successfully");
    }
}

// This function updates one or more rows of the DB
void StreamingSQLiteDBInterface::runUpdate(std::string query) {
    char *zErrMsg = 0;

    int rc = sqlite3_exec(database, query.c_str(), NULL, NULL, &zErrMsg);

    if (rc != SQLITE_OK) {
        streamdb_logger.error("SQL Error: " + string(zErrMsg));
        sqlite3_free(zErrMsg);
    } else {
        streamdb_logger.info("Update operation done successfully");
    }
}

int StreamingSQLiteDBInterface::RunSqlNoCallback(const char *zSql) {
    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(database, zSql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return rc;

    int rowCount = 0;
    rc = sqlite3_step(stmt);
    while (rc != SQLITE_DONE && rc != SQLITE_OK) {
        rowCount++;
        int colCount = sqlite3_column_count(stmt);
        for (int colIndex = 0; colIndex < colCount; colIndex++) {
            int type = sqlite3_column_type(stmt, colIndex);
            const char *columnName = sqlite3_column_name(stmt, colIndex);
            if (type == SQLITE_INTEGER) {
                int valInt = sqlite3_column_int(stmt, colIndex);
                printf("columnName = %s, Integer val = %d", columnName, valInt);
            } else if (type == SQLITE_FLOAT) {
                double valDouble = sqlite3_column_double(stmt, colIndex);
                printf("columnName = %s,Double val = %f", columnName, valDouble);
            } else if (type == SQLITE_TEXT) {
                const unsigned char *valChar = sqlite3_column_text(stmt, colIndex);
                printf("columnName = %s,Text val = %s", columnName, valChar);
                // free((void *) valChar);
            } else if (type == SQLITE_BLOB) {
                printf("columnName = %s,BLOB", columnName);
            } else if (type == SQLITE_NULL) {
                printf("columnName = %s,NULL", columnName);
            }
        }
        printf("Line %d, rowCount = %d", rowCount, colCount);

        rc = sqlite3_step(stmt);
    }

    rc = sqlite3_finalize(stmt);

    return rc;
}