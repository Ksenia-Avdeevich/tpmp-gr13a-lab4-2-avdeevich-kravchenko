#include <iostream>
#include "db.h"
#include "auth.h"

static int run = 0, passed = 0;
#define TEST(name, expr) do { \
    ++run; \
    if (expr) { std::cout << "[PASS] " << name << "\n"; ++passed; } \
    else      { std::cout << "[FAIL] " << name << " (line " << __LINE__ << ")\n"; } \
} while(0)

static int query_int(const char* sql) {
    sqlite3_stmt* st = nullptr; int val = 0;
    if (sqlite3_prepare_v2(g_db, sql, -1, &st, nullptr) == SQLITE_OK) {
        if (sqlite3_step(st) == SQLITE_ROW) val = sqlite3_column_int(st, 0);
        sqlite3_finalize(st);
    }
    return val;
}

void test_compacts_exist() {
    TEST("compacts не пустая", query_int("SELECT COUNT(*) FROM compacts;") > 0);
}

void test_tracks_linked_to_compact() {
    TEST("треки компакта 1 существуют",
         query_int("SELECT COUNT(*) FROM tracks WHERE compact_id=1;") > 0);
}

void test_operations_exist() {
    TEST("operations не пустая",
         query_int("SELECT COUNT(*) FROM operations;") > 0);
}

void test_sold_never_exceeds_received() {
    int violations = query_int(
        "SELECT COUNT(*) FROM ("
        "  SELECT compact_id,"
        "    SUM(CASE WHEN op_type='receive' THEN quantity ELSE 0 END) AS recv,"
        "    SUM(CASE WHEN op_type='sell'    THEN quantity ELSE 0 END) AS sold "
        "  FROM operations GROUP BY compact_id"
        ") WHERE sold > recv;");
    TEST("продано <= поступило для всех компактов", violations == 0);
}

void test_top_compact_query() {
    TEST("топ-компакт по продажам существует",
         query_int(
             "SELECT compact_id FROM operations WHERE op_type='sell' "
             "GROUP BY compact_id ORDER BY SUM(quantity) DESC LIMIT 1;") > 0);
}

void test_author_report_not_empty() {
    TEST("отчёт по авторам даёт результат",
         query_int(
             "SELECT COUNT(DISTINCT t.author) FROM tracks t "
             "JOIN operations o ON o.compact_id=t.compact_id "
             "AND o.op_type='sell';") > 0);
}

void test_period_report_insert() {
    sqlite3_exec(g_db,
        "INSERT INTO period_report(date_from,date_to,compact_id,"
        "received_qty,sold_qty) VALUES('2024-01-01','2024-12-31',1,50,15);",
        nullptr, nullptr, nullptr);
    TEST("period_report принимает вставку",
         query_int("SELECT COUNT(*) FROM period_report;") > 0);
}

void test_fk_violation_blocked() {
    char* err = nullptr;
    int rc = sqlite3_exec(g_db,
        "INSERT INTO tracks(title,author,performer,compact_id)"
        " VALUES('X','Y','Z',99999);",
        nullptr, nullptr, &err);
    TEST("FK нарушение при плохом compact_id блокируется", rc != SQLITE_OK);
    if (err) sqlite3_free(err);
}

void test_cascade_delete() {
    sqlite3_exec(g_db,
        "INSERT INTO compacts(id,code,manufacture_date,company,price)"
        " VALUES(999,'TST','2024-01-01','Co',1.0);",
        nullptr, nullptr, nullptr);
    sqlite3_exec(g_db,
        "INSERT INTO tracks(title,author,performer,compact_id)"
        " VALUES('T','A','P',999);",
        nullptr, nullptr, nullptr);
    sqlite3_exec(g_db,
        "DELETE FROM compacts WHERE id=999;",
        nullptr, nullptr, nullptr);
    TEST("каскадное удаление треков при удалении компакта",
         query_int("SELECT COUNT(*) FROM tracks WHERE compact_id=999;") == 0);
}

void test_price_check_constraint() {
    char* err = nullptr;
    int rc = sqlite3_exec(g_db,
        "INSERT INTO compacts(code,manufacture_date,company,price)"
        " VALUES('BAD','2024-01-01','Co',-10.0);",
        nullptr, nullptr, &err);
    TEST("CHECK блокирует отрицательную цену", rc != SQLITE_OK);
    if (err) sqlite3_free(err);
}

int main() {
    std::cout << "=== Тесты: compacts + reports ===\n\n";
    if (!db_open(":memory:"))  { std::cout << "Не получилось открыть БД\n"; return 1; }
    if (!db_init_schema())     { std::cout << "Ошибка схемы\n"; db_close(); return 1; }

    test_compacts_exist();
    test_tracks_linked_to_compact();
    test_operations_exist();
    test_sold_never_exceeds_received();
    test_top_compact_query();
    test_author_report_not_empty();
    test_period_report_insert();
    test_fk_violation_blocked();
    test_cascade_delete();
    test_price_check_constraint();

    std::cout << "\nРезультат: " << passed << "/" << run << " тестов пройдено.\n";
    db_close();
    return (passed == run) ? 0 : 1;
}
