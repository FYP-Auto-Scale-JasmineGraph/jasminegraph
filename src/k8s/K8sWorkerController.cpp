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

#include "K8sWorkerController.h"

#include <stdlib.h>

#include <stdexcept>
#include <utility>

Logger controller_logger;

std::vector<JasmineGraphServer::worker> K8sWorkerController::workerList = {};
static int TIME_OUT = 900;
static std::vector<int> activeWorkerIds = {};
std::mutex workerIdMutex;
std::mutex k8sSpawnMutex;
static volatile int nextWorkerId = 0;

static inline int getNextWorkerId(int count) {
    const std::lock_guard<std::mutex> lock(workerIdMutex);
    int returnId = nextWorkerId;
    nextWorkerId += count;
    return returnId;
}

K8sWorkerController::K8sWorkerController(std::string masterIp, int numberOfWorkers, SQLiteDBInterface *metadb) {
    this->masterIp = std::move(masterIp);
    this->numberOfWorkers = 0;
    apiClient_setupGlobalEnv();
    this->interface = new K8sInterface();
    this->metadb = *metadb;
}

K8sWorkerController::~K8sWorkerController() {
    delete this->interface;
    apiClient_unsetupGlobalEnv();
}

static K8sWorkerController *instance = nullptr;

K8sWorkerController *K8sWorkerController::getInstance() {
    if (instance == nullptr) {
        controller_logger.error("K8sWorkerController is not instantiated");
        throw std::runtime_error("K8sWorkerController is not instantiated");
    }
    return instance;
}

static std::mutex instanceMutex;
K8sWorkerController *K8sWorkerController::getInstance(std::string masterIp, int numberOfWorkers,
                                                      SQLiteDBInterface *metadb) {
    if (instance == nullptr) {
        instanceMutex.lock();
        if (instance == nullptr) {  // double-checking lock
            instance = new K8sWorkerController(masterIp, numberOfWorkers, metadb);
            try {
                instance->maxWorkers = stoi(instance->interface->getJasmineGraphConfig("max_worker_count"));
            } catch (std::invalid_argument &e) {
                controller_logger.error("Invalid max_worker_count value. Defaulted to 4");
                instance->maxWorkers = 4;
            }

            // Delete all the workers from the database
            metadb->runUpdate("DELETE FROM worker");
            int workersAttached = instance->attachExistingWorkers();
            if (numberOfWorkers - workersAttached > 0) {
                instance->scaleUp(numberOfWorkers - workersAttached);
            }
        }
        instanceMutex.unlock();
    }
    return instance;
}

std::string K8sWorkerController::spawnWorker(int workerId) {
    k8sSpawnMutex.lock();
    controller_logger.info("Spawning worker " + to_string(workerId));
    auto volume = this->interface->createJasmineGraphPersistentVolume(workerId);
    if (volume != nullptr && volume->metadata != nullptr && volume->metadata->name != nullptr) {
        controller_logger.info("Worker " + std::to_string(workerId) + " persistent volume created successfully");
    } else {
        controller_logger.error("Worker " + std::to_string(workerId) + " persistent volume creation failed");
        throw std::runtime_error("Worker " + std::to_string(workerId) + " persistent volume creation failed");
    }

    auto claim = this->interface->createJasmineGraphPersistentVolumeClaim(workerId);
    if (claim != nullptr && claim->metadata != nullptr && claim->metadata->name != nullptr) {
        controller_logger.info("Worker " + std::to_string(workerId) + " persistent volume claim created successfully");
    } else {
        controller_logger.error("Worker " + std::to_string(workerId) + " persistent volume claim creation failed");
        throw std::runtime_error("Worker " + std::to_string(workerId) + " persistent volume claim creation failed");
    }

    v1_service_t *service = this->interface->createJasmineGraphWorkerService(workerId);
    if (service != nullptr && service->metadata != nullptr && service->metadata->name != nullptr) {
        controller_logger.info("Worker " + std::to_string(workerId) + " service created successfully");
    } else {
        controller_logger.error("Worker " + std::to_string(workerId) + " service creation failed");
        throw std::runtime_error("Worker " + std::to_string(workerId) + " service creation failed");
    }

    std::string ip(service->spec->cluster_ip);

    v1_deployment_t *deployment = this->interface->createJasmineGraphWorkerDeployment(workerId, ip, this->masterIp);
    if (deployment != nullptr && deployment->metadata != nullptr && deployment->metadata->name != nullptr) {
        controller_logger.info("Worker " + std::to_string(workerId) + " deployment created successfully");
    } else {
        controller_logger.error("Worker " + std::to_string(workerId) + " deployment creation failed");
        throw std::runtime_error("Worker " + std::to_string(workerId) + " deployment creation failed");
    }
    k8sSpawnMutex.unlock();
    controller_logger.info("Waiting for worker " + to_string(workerId) + " to respond");
    int waiting = 0;
    while (true) {
        int sockfd;
        struct sockaddr_in serv_addr;
        struct hostent *server;

        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            controller_logger.error("Cannot create socket");
        }

        server = gethostbyname(ip.c_str());
        if (server == NULL) {
            controller_logger.error("ERROR, no host named " + ip);
        }

        bzero((char *)&serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
        serv_addr.sin_port = htons(Conts::JASMINEGRAPH_INSTANCE_PORT);

        if (Utils::connect_wrapper(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            waiting += 30;  // Added overhead in connection retry attempts
            sleep(10);
        } else {
            Utils::send_str_wrapper(sockfd, JasmineGraphInstanceProtocol::CLOSE);
            close(sockfd);
            break;
        }

        if (waiting >= TIME_OUT) {
            controller_logger.error("Error in spawning new worker");
            deleteWorker(workerId);
            close(sockfd);
            return "";
        }
    }
    controller_logger.info("Worker " + to_string(workerId) + " responded");

    JasmineGraphServer::worker worker = {
        .hostname = ip, .port = Conts::JASMINEGRAPH_INSTANCE_PORT, .dataPort = Conts::JASMINEGRAPH_INSTANCE_DATA_PORT};
    K8sWorkerController::workerList.push_back(worker);
    std::string insertQuery =
        "INSERT INTO worker (host_idhost, server_port, server_data_port, name, ip, idworker) "
        "VALUES ( -1, " +
        std::to_string(Conts::JASMINEGRAPH_INSTANCE_PORT) + ", " +
        std::to_string(Conts::JASMINEGRAPH_INSTANCE_DATA_PORT) + ", " + "'" + std::string(service->metadata->name) +
        "', " + "'" + ip + "', " + std::to_string(workerId) + ")";
    int status = metadb.runInsert(insertQuery);
    if (status == -1) {
        controller_logger.error("Worker " + std::to_string(workerId) + " database insertion failed");
    }
    activeWorkerIds.push_back(workerId);
    return ip + ":" + to_string(Conts::JASMINEGRAPH_INSTANCE_PORT);
}

void K8sWorkerController::deleteWorker(int workerId) {
    std::string selectQuery = "SELECT ip, server_port FROM worker WHERE idworker = " + std::to_string(workerId);
    auto result = metadb.runSelect(selectQuery);
    if (result.size() == 0) {
        controller_logger.error("Worker " + std::to_string(workerId) + " not found in the database");
        return;
    }
    std::string ip = result.at(0).at(0).second;
    int port = atoi(result.at(0).at(1).second.c_str());

    int response = JasmineGraphServer::shutdown_worker(ip, port);
    if (response == -1) {
        controller_logger.error("Worker " + std::to_string(workerId) + " graceful shutdown failed");
    }

    v1_status_t *status = this->interface->deleteJasmineGraphWorkerDeployment(workerId);
    if (status != nullptr && status->code == 0) {
        controller_logger.info("Worker " + std::to_string(workerId) + " deployment deleted successfully");
    } else {
        controller_logger.error("Worker " + std::to_string(workerId) + " deployment deletion failed");
    }

    v1_service_t *service = this->interface->deleteJasmineGraphWorkerService(workerId);
    if (service != nullptr && service->metadata != nullptr && service->metadata->name != nullptr) {
        controller_logger.info("Worker " + std::to_string(workerId) + " service deleted successfully");
    } else {
        controller_logger.error("Worker " + std::to_string(workerId) + " service deletion failed");
    }

    auto volume = this->interface->deleteJasmineGraphPersistentVolume(workerId);
    if (volume != nullptr && volume->metadata != nullptr && volume->metadata->name != nullptr) {
        controller_logger.info("Worker " + std::to_string(workerId) + " persistent volume deleted successfully");
    } else {
        controller_logger.error("Worker " + std::to_string(workerId) + " persistent volume deletion failed");
    }

    auto claim = this->interface->deleteJasmineGraphPersistentVolumeClaim(workerId);
    if (claim != nullptr && claim->metadata != nullptr && claim->metadata->name != nullptr) {
        controller_logger.info("Worker " + std::to_string(workerId) + " persistent volume claim deleted successfully");
    } else {
        controller_logger.error("Worker " + std::to_string(workerId) + " persistent volume claim deletion failed");
    }

    std::string deleteQuery = "DELETE FROM worker WHERE idworker = " + std::to_string(workerId);
    metadb.runUpdate(deleteQuery);
    deleteQuery = "DELETE FROM worker_has_partition WHERE worker_idworker = " + std::to_string(workerId);
    metadb.runUpdate(deleteQuery);
    std::remove(activeWorkerIds.begin(), activeWorkerIds.end(), workerId);
}

int K8sWorkerController::attachExistingWorkers() {
    v1_deployment_list_t *deployment_list =
        this->interface->getDeploymentList(strdup("deployment=jasminegraph-worker"));

    if (deployment_list && deployment_list->items->count > 0) {
        listEntry_t *listEntry = NULL;
        v1_deployment_t *deployment = NULL;
        list_ForEach(listEntry, deployment_list->items) {
            deployment = static_cast<v1_deployment_t *>(listEntry->data);
            list_t *labels = deployment->metadata->labels;
            listEntry_t *label = NULL;

            list_ForEach(label, labels) {
                auto *pair = static_cast<keyValuePair_t *>(label->data);
                v1_service_t *service;
                if (strcmp(pair->key, "workerId") == 0) {
                    int workerId = std::stoi(static_cast<char *>(pair->value));
                    workerIdMutex.lock();
                    nextWorkerId = workerId + 1;
                    workerIdMutex.unlock();
                    v1_service_list_t *service_list = this->interface->getServiceList(
                        strdup(("service=jasminegraph-worker,workerId=" + std::to_string(workerId)).c_str()));

                    if (!service_list || service_list->items->count == 0) {
                        service = this->interface->createJasmineGraphWorkerService(workerId);
                    } else {
                        service = static_cast<v1_service_t *>(service_list->items->firstEntry->data);
                    }

                    std::string ip(service->spec->cluster_ip);
                    JasmineGraphServer::worker worker = {.hostname = ip,
                                                         .port = Conts::JASMINEGRAPH_INSTANCE_PORT,
                                                         .dataPort = Conts::JASMINEGRAPH_INSTANCE_DATA_PORT};
                    K8sWorkerController::workerList.push_back(worker);
                    std::string insertQuery =
                        "INSERT INTO worker (host_idhost, server_port, server_data_port, name, ip, idworker) "
                        "VALUES ( -1, " +
                        std::to_string(Conts::JASMINEGRAPH_INSTANCE_PORT) + ", " +
                        std::to_string(Conts::JASMINEGRAPH_INSTANCE_DATA_PORT) + ", " + "'" +
                        std::string(service->metadata->name) + "', " + "'" + ip + "', " + std::to_string(workerId) +
                        ")";
                    int status = metadb.runInsert(insertQuery);
                    if (status == -1) {
                        controller_logger.error("Worker " + std::to_string(workerId) + " database insertion failed");
                    }
                    activeWorkerIds.push_back(workerId);
                    break;
                }
            }
        }
        return deployment_list->items->count;
    } else {
        return 0;
    }
}

std::string K8sWorkerController::getMasterIp() const { return this->masterIp; }

int K8sWorkerController::getNumberOfWorkers() const { return numberOfWorkers; }

/*
 * @deprecated
 */
void K8sWorkerController::setNumberOfWorkers(int newNumberOfWorkers) {
    if (newNumberOfWorkers > this->numberOfWorkers) {
        for (int i = this->numberOfWorkers; i < newNumberOfWorkers; i++) {
            this->spawnWorker(i);
        }
    } else if (newNumberOfWorkers < this->numberOfWorkers) {
        for (int i = newNumberOfWorkers; i < this->numberOfWorkers; i++) {
            this->deleteWorker(i);
        }
    }
    this->numberOfWorkers = newNumberOfWorkers;
}

std::map<string, string> K8sWorkerController::scaleUp(int count) {
    if (this->numberOfWorkers + count > this->maxWorkers) {
        count = this->maxWorkers - this->numberOfWorkers;
    }
    std::map<string, string> workers;
    if (count <= 0) return workers;
    controller_logger.info("Scale up with " + to_string(count) + " new workers");
    std::future<string> asyncCalls[count];
    int nextWorkerId = getNextWorkerId(count);
    for (int i = 0; i < count; i++) {
        asyncCalls[i] = std::async(&K8sWorkerController::spawnWorker, this, nextWorkerId + i);
    }

    int success = 0;
    for (int i = 0; i < count; i++) {
        std::string result = asyncCalls[i].get();
        if (!result.empty()) {
            success++;
            workers.insert(std::make_pair(to_string(nextWorkerId + i), result));
        }
    }
    this->numberOfWorkers += success;
    return workers;
}

void K8sWorkerController::scaleDown(const set<int> workerIds) {
    size_t count = workerIds.size();
    controller_logger.info("Scale down with " + to_string(count) + " workers");
    std::thread threads[count];
    int ind = 0;
    for (auto it = workerIds.begin(); it != workerIds.end(); it++) {
        threads[ind++] = std::thread(&K8sWorkerController::deleteWorker, this, *it);
    }

    for (int i = 0; i < count; i++) {
        threads[i].join();
    }
}
