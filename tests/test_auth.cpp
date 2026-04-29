#include <iostream>
#include <cassert>
#include <cstring>
#include "db.h"
#include "auth.h"

static int run = 0, passed = 0;
#define TEST(name, expr) do { \
    ++run; \
    if (expr) { std::cout << "[PASS] " << name << "\n"; ++passed; } \
    else      { std::cout << "[FAIL] " << name << " (line " << __LINE__ << ")\n"; } \
} while(0)

void test_db_open_memory() {
    TEST("db_open :memory:", db_open(":memory:"));
    TEST("g_db не nullptr", g_db != nullptr);
}

void test_schema_init() {
    TEST("db_init_schema успешен", db_init_schema());
}

void test_login_wrong_password() {
    TEST("неверный пароль → false", !auth_login("admin", "wrongpass"));
}

void test_login_correct() {
    bool ok = auth_login("admin",
        "8c6976e5b5410415bde908bd4dee15dfb167a9c873fc4bb8a81f6f2ab448a918");
    TEST("верный хэш → true", ok);
}

void test_role_manager() {
    auth_login("admin",
        "8c6976e5b5410415bde908bd4dee15dfb167a9c873fc4bb8a81f6f2ab448a918");
    TEST("admin — менеджер", auth_is_manager());
}

void test_logout_clears_session() {
    auth_logout();
    TEST("после logout login пуст",   g_session.login.empty());
    TEST("после logout is_manager=0", !auth_is_manager());
}

void test_buyer_not_manager() {
    auth_login("buyer1",
        "a665a45920422f9d417e4867efdc4fb8a04a1f3fff1fa07e998e86f7f7a27ae3");
    TEST("buyer1 не менеджер", !auth_is_manager());
    auth_logout();
}

void test_trigger_blocks_oversell() {
    char* err = nullptr;
    int rc = sqlite3_exec(g_db,
        "INSERT INTO operations(op_date,op_type,compact_id,quantity,user_id)"
        " VALUES('2099-01-01','sell',1,999999,1);",
        nullptr, nullptr, &err);
    TEST("триггер блокирует продажу сверх остатка", rc != SQLITE_OK);
    if (err) sqlite3_free(err);
}

int main() {
    std::cout << "=== Тесты: auth + db ===\n\n";
    test_db_open_memory();
    test_schema_init();
    test_login_wrong_password();
    test_login_correct();
    test_role_manager();
    test_logout_clears_session();
    test_buyer_not_manager();
    test_trigger_blocks_oversell();
    std::cout << "\nРезультат: " << passed << "/" << run << " тестов пройдено.\n";
    db_close();
    return (passed == run) ? 0 : 1;
}
