/**
Copyright 2024 JasmineGraph Team
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

#include <map>
#include <mutex>

#include "../metadb/SQLiteDBInterface.h"

extern std::mutex schedulerMutex;
extern std::map<std::string, int> used_workers;  // { worker_id => use_count }

extern void start_scale_down(SQLiteDBInterface *sqliteInterface);

extern void stop_scale_down();
