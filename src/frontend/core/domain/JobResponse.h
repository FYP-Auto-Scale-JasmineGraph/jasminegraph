/**
Copyright 2021 JasmineGraph Team
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

#ifndef JASMINEGRAPH_JOBRESPONSE_H
#define JASMINEGRAPH_JOBRESPONSE_H

#include <map>
#include <string>
#include <chrono>

class JobResponse {
 private:
    std::string jobId;
    std::map<std::string, std::string> responseParams;
    std::chrono::time_point<std::chrono::system_clock> begin;
    std::chrono::time_point<std::chrono::system_clock> end;

 public:
    std::string getJobId();
    void setJobId(std::string inputJobId);
    void addParameter(std::string key, std::string value);
    std::string getParameter(std::string key);
    std::chrono::time_point<std::chrono::system_clock> getEndTime();
    void setEndTime(std::chrono::time_point<std::chrono::system_clock> end);
};

#endif  // JASMINEGRAPH_JOBRESPONSE_H
