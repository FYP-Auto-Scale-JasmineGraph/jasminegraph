#include "K8sWorkerController.h"

#include <utility>
#include <stdexcept>
#include "../util/logger/Logger.h"
#include "../util/Conts.h"

Logger controller_logger;

K8sWorkerController::K8sWorkerController(std::string masterIp, int numberOfWorkers, SQLiteDBInterface *metadb) {
    this->masterIp = std::move(masterIp);
    this->numberOfWorkers = numberOfWorkers;
    this->interface = new K8sInterface();
    this->metadb = *metadb;

    // Delete all the workers from the database
    metadb->runUpdate("DELETE FROM worker");
    int workersAttached = this->attachExistingWorkers();
    for(int i = workersAttached; i < numberOfWorkers; i++) {
        this->spawnWorker(i);
    }
}

K8sWorkerController::~K8sWorkerController() {
    delete this->interface;
}

void K8sWorkerController::spawnWorker(int workerId) {
    v1_deployment_t *deployment = this->interface->createJasmineGraphWorkerDeployment(workerId, this->masterIp);
    if (deployment != nullptr && deployment->metadata != nullptr && deployment->metadata->name != nullptr) {
        controller_logger.info("Worker " + std::to_string(workerId) + " deployment created successfully");

    } else {
        throw std::runtime_error("Worker " + std::to_string(workerId) + " deployment creation failed");
    }

    v1_service_t *service = this->interface->createJasmineGraphWorkerService(workerId);
    if (service != nullptr && service->metadata != nullptr && service->metadata->name != nullptr) {
        controller_logger.info("Worker " + std::to_string(workerId) + " service created successfully");
    } else {
        throw std::runtime_error("Worker " + std::to_string(workerId) + " service creation failed");
    }

    std::string insertQuery = "INSERT INTO worker (host_idhost, server_port, server_data_port, name, ip, idworker) "
                              "VALUES (0, " + std::to_string(Conts::JASMINEGRAPH_FRONTEND_PORT) + ", "
                              + std::to_string(Conts::JASMINEGRAPH_BACKEND_PORT) + ", " +
                              "'" + std::string(service->metadata->name) + "', " +
                              "'" + std::string(service->spec->cluster_ip) + "', " +
                              std::to_string(workerId) + ")";
    int status = metadb.runInsert(insertQuery);
    if (status == -1) {
        controller_logger.error("Worker " + std::to_string(workerId) + " database insertion failed");
    }
}

void K8sWorkerController::deleteWorker(int workerId) {
    v1_status_t *status = this->interface->deleteJasmineGraphWorkerDeployment(workerId);
    if (status != nullptr && status->code == 0) {
        controller_logger.info("Worker " + std::to_string(workerId) + " deployment deleted successfully");
    } else {
        controller_logger.error("Worker " + std::to_string(workerId) + " deployment deletion failed");
    }

    std::string deleteQuery = "DELETE FROM worker WHERE idworker = " + std::to_string(workerId);
    metadb.runUpdate(deleteQuery);
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
                    v1_service_list_t *service_list = this->interface->getServiceList(
                            strdup(("service=jasminegraph-worker,workerId="
                                    + std::to_string(workerId)).c_str()));

                    if (!service_list || service_list->items->count == 0) {
                        service = this->interface->createJasmineGraphWorkerService(workerId);
                    } else {
                        service = static_cast<v1_service_t *>(service_list->items->firstEntry->data);
                    }

                    std::string insertQuery =
                            "INSERT INTO worker (host_idhost, server_port, server_data_port, name, ip, idworker) "
                            "VALUES ( 0, " + std::to_string(Conts::JASMINEGRAPH_FRONTEND_PORT) + ", "
                            + std::to_string(Conts::JASMINEGRAPH_BACKEND_PORT) + ", " +
                            "'" + std::string(service->metadata->name) + "', " +
                            "'" + std::string(service->spec->cluster_ip) + "', " +
                            std::to_string(workerId) + ")";
                    int status = metadb.runInsert(insertQuery);
                    if (status == -1) {
                        controller_logger.error("Worker " + std::to_string(workerId) + " database insertion failed");
                    }
                    break;
                }
            }
        }
        return deployment_list->items->count;
    } else {
        return 0;
    }
}
