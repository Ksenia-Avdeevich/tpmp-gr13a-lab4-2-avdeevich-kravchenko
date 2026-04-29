#include <iostream>
#include "db.h"

static int run = 0, passed = 0;
#define TEST(name, expr) do { \
    ++run; \
    if (expr) { std::cout << "[PASS] " << name << "\n"; ++passed; } \
    else      { std::cout << "[FAIL] " << name << " (line " << __LINE__ << ")\n"; } \
} while(0)

static int exec_rc(const char* sql) {
    char* err = nullptr;
    int rc = sqlite3_exec(g_db, sql, nullptr, nullptr, &err);
    if (err) sqlite3_free(err);
    return rc;
}

static int query_int(const char* sql) {
    sqlite3_stmt* st = nullptr; int v = 0;
    if (sqlite3_prepare_v2(g_db, sql, -1, &st, nullptr) == SQLITE_OK) {
        if (sqlite3_step(st) == SQLITE_ROW) v = sqlite3_column_int(st, 0);
        sqlite3_finalize(st);
    }
    return v;
}

void test_receive_ok() {
    TEST("операция receive вставляется",
         exec_rc("INSERT INTO operations(op_date,op_type,compact_id,quantity,user_id)"
                 " VALUES('2024-06-01','receive',1,100,1);") == SQLITE_OK);
}

void test_sell_within_stock_ok() {
    exec_rc("INSERT INTO operations(op_date,op_type,compact_id,quantity,user_id)"
            " VALUES('2024-06-01','receive',2,200,1);");
    TEST("продажа в пределах остатка разрешена",
         exec_rc("INSERT INTO operations(op_date,op_type,compact_id,quantity,user_id)"
                 " VALUES('2024-06-02','sell',2,5,1);") == SQLITE_OK);
}

void test_sell_exceeds_stock_blocked() {
    TEST("триггер блокирует продажу сверх остатка",
         exec_rc("INSERT INTO operations(op_date,op_type,compact_id,quantity,user_id)"
                 " VALUES('2024-06-03','sell',1,999999,1);") != SQLITE_OK);
}

void test_invalid_op_type_blocked() {
    TEST("CHECK блокирует неверный op_type",
         exec_rc("INSERT INTO operations(op_date,op_type,compact_id,quantity,user_id)"
                 " VALUES('2024-06-01','refund',1,1,1);") != SQLITE_OK);
}

void test_zero_quantity_blocked() {
    TEST("CHECK блокирует quantity=0",
         exec_rc("INSERT INTO operations(op_date,op_type,compact_id,quantity,user_id)"
                 " VALUES('2024-06-01','receive',1,0,1);") != SQLITE_OK);
}

void test_operations_count() {
    TEST("количество операций > 0",
         query_int("SELECT COUNT(*) FROM operations;") > 0);
}

int main() {
    std::cout << "=== Тесты: operations + триггеры ===\n\n";
    if (!db_open(":memory:"))  { std::cout << "Не получилось открыть БД\n"; return 1; }
    if (!db_init_schema())     { std::cout << "Ошибка схемы\n"; db_close(); return 1; }

    test_receive_ok();
    test_sell_within_stock_ok();
    test_sell_exceeds_stock_blocked();
    test_invalid_op_type_blocked();
    test_zero_quantity_blocked();
    test_operations_count();

    std::cout << "\nРезультат: " << passed << "/" << run << " тестов пройдено.\n";
    db_close();
    return (passed == run) ? 0 : 1;
}
