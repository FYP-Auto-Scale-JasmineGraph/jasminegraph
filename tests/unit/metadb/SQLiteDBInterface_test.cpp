
#include "gtest/gtest.h"

#include "../../../src/metadb/SQLiteDBInterface.h"

class SQLiteDBInterfaceTest: public testing::Test {
protected:
    SQLiteDBInterface* metadb;
    void SetUp() override {
        metadb = new SQLiteDBInterface(TEST_RESOURCE_DIR"temp/jasminegraph_metadb.db");
        metadb->init();
    }

    void TearDown() override {
        free(metadb);

    }
};

TEST_F(SQLiteDBInterfaceTest, TestInsertHost) {
    auto* host = new SQLiteDBInterface::host();
    host->idhost = 1;
    host->name = "localhost";
    host->ip = "127.0.0.1";
    metadb->insertHost(host);

    auto hosts = metadb->runSelect("SELECT idhost, name, ip, is_public FROM host");
    ASSERT_EQ(hosts.size(), 1);
    ASSERT_EQ(atoi(hosts[0][0].second.c_str()), host->idhost);
    ASSERT_EQ(hosts[0][1].second, host->name);
    ASSERT_EQ(hosts[0][2].second, host->ip);
    ASSERT_EQ(hosts[0][3].second, "false");
}