/**
Copyright 2018 JasmineGraph Team
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
 */

#ifndef JASMINEGRAPH_SQLITEDBINTERFACE_H
#define JASMINEGRAPH_SQLITEDBINTERFACE_H

#include <sqlite3.h>

#include <map>
#include <string>
#include <vector>

#include "../util/Conts.h"

class SQLiteDBInterface {
 private:
    sqlite3 *database{};
    std::string readDDLFile(const std::string& fileName);
    int createDatabase();
    int runInsert(std::string);

 public:
    int init();

    int finalize();

    std::vector<std::vector<std::pair<std::string, std::string>>> runSelect(std::string);

    void runUpdate(std::string);

    void runInsertNoIDReturn(std::string);

    int RunSqlNoCallback(const char *zSql);

    SQLiteDBInterface();

    struct host {
        int idhost;
        std::string name;
        std::string ip;
        std::string is_public;

        host() {
            is_public = "false";
        }
    };

    struct worker {
        int idworker;
        int host_idhost;
        std::string name;
        std::string ip;
        std::string user;
        std::string is_public;
        int server_port;
        int server_data_port;

        worker() {
            user = "";
            is_public = "false";
            server_port = Conts::JASMINEGRAPH_BACKEND_PORT;
            server_data_port = Conts::JASMINEGRAPH_BACKEND_PORT;
        }
    };

    struct worker_has_partition {
        int partition_idpartition;
        int partition_graph_idgraph;
        int worker_idworker;
    };

    struct graph {
        std::string name;
        std::string upload_path;
        std::string upload_start_time;
        std::string upload_end_time;
        int graph_status_idgraph_status;
        int vertexcount;
        int centralpartitioncount;
        int edgecount;

        graph() {
            upload_end_time = "";
            vertexcount = 0;
            centralpartitioncount = 0;
            edgecount = 0;
        }
    };

    struct model {
        std::string name;
        std::string upload_path;
        std::string upload_time;
        int model_status_idmodel_status;
    };

    int insertHost(host* data);
    int insertWorker(worker* data);
    int insertWorkerHasPartition(worker_has_partition* data);
    int insertGraph(graph* data);
    int insertModel(model* data);
};

#endif  // JASMINEGRAPH_SQLITEDBINTERFACE_H
