/**
Copyright 2018 JasminGraph Team
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

#ifndef JASMINEGRAPH_PERFORMANCESQLITEDBINTERFACE_H
#define JASMINEGRAPH_PERFORMANCESQLITEDBINTERFACE_H

#include <map>
#include <vector>

#include "../util/sqlite3/sqlite3.h"

class PerformanceSQLiteDBInterface {
 private:
  sqlite3 *database;

 public:
  int init();

  int finalize();

  std::vector<std::vector<std::pair<std::string, std::string>>> runSelect(
      std::string);

  int runInsert(std::string);

  void runUpdate(std::string);

  void runInsertNoIDReturn(std::string);

  int RunSqlNoCallback(const char *zSql);

  PerformanceSQLiteDBInterface();
};

#endif  // JASMINEGRAPH_PERFORMANCESQLITEDBINTERFACE_H
