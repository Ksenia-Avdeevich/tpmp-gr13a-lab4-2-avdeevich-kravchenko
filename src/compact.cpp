#include "compact.h"
#include "auth.h"
#include "db.h"
#include <iostream>
#include <iomanip>
#include <string>

void compact_list_all() {
    const char* sql = R"(
SELECT c.id, c.code, c.manufacture_date, c.company, c.price,
  COALESCE((SELECT SUM(quantity) FROM operations
            WHERE compact_id=c.id AND op_type='receive'),0) -
  COALESCE((SELECT SUM(quantity) FROM operations
            WHERE compact_id=c.id AND op_type='sell'),0) AS remaining
FROM compacts c ORDER BY c.code;
)";
    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(g_db, sql, -1, &st, nullptr) != SQLITE_OK) {
        std::cout << "Ошибка запроса.\n";
        return;
    }
    std::cout << "\n"
              << std::left
              << std::setw(5)  << "ID"
              << std::setw(9)  << "Код"
              << std::setw(14) << "Дата изг."
              << std::setw(22) << "Компания"
              << std::setw(10) << "Цена"
              << std::setw(10) << "Остаток" << "\n"
              << std::string(70, '-') << "\n";

    while (sqlite3_step(st) == SQLITE_ROW) {
        std::cout << std::left
                  << std::setw(5)  << sqlite3_column_int(st, 0)
                  << std::setw(9)  << sqlite3_column_text(st, 1)
                  << std::setw(14) << sqlite3_column_text(st, 2)
                  << std::setw(22) << sqlite3_column_text(st, 3)
                  << std::setw(10) << std::fixed << std::setprecision(2)
                                   << sqlite3_column_double(st, 4)
                  << std::setw(10) << sqlite3_column_int(st, 5) << "\n";
    }
    sqlite3_finalize(st);
}

void compact_add() {
    if (!auth_is_manager()) {
        std::cout << "Нет прав. Требуется роль менеджера.\n";
        return;
    }
    std::string code, date, company;
    double price = 0.0;
    std::cout << "Код компакта: ";       std::cin >> code;
    std::cout << "Дата (YYYY-MM-DD): ";  std::cin >> date;
    std::cout << "Компания: ";           std::cin >> company;
    std::cout << "Цена: ";               std::cin >> price;

    const char* sql =
        "INSERT INTO compacts(code,manufacture_date,company,price)"
        " VALUES(?,?,?,?);";
    sqlite3_stmt* st = nullptr;
    sqlite3_prepare_v2(g_db, sql, -1, &st, nullptr);
    sqlite3_bind_text(st, 1, code.c_str(),    -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 2, date.c_str(),    -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 3, company.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(st, 4, price);

    if (sqlite3_step(st) == SQLITE_DONE)
        std::cout << "Компакт добавлен.\n";
    else
        std::cout << "Ошибка: " << sqlite3_errmsg(g_db) << "\n";
    sqlite3_finalize(st);
}

void compact_update() {
    if (!auth_is_manager()) {
        std::cout << "Нет прав.\n"; return;
    }
    int id = 0; double price = 0.0;
    std::cout << "ID компакта для изменения цены: "; std::cin >> id;
    std::cout << "Новая цена: "; std::cin >> price;

    const char* sql = "UPDATE compacts SET price=? WHERE id=?;";
    sqlite3_stmt* st = nullptr;
    sqlite3_prepare_v2(g_db, sql, -1, &st, nullptr);
    sqlite3_bind_double(st, 1, price);
    sqlite3_bind_int(st,   2, id);
    sqlite3_step(st);
    std::cout << "Обновлено строк: " << sqlite3_changes(g_db) << "\n";
    sqlite3_finalize(st);
}

void compact_delete() {
    if (!auth_is_manager()) {
        std::cout << "Нет прав.\n"; return;
    }
    int id = 0;
    std::cout << "ID компакта для удаления: "; std::cin >> id;

    const char* sql = "DELETE FROM compacts WHERE id=?;";
    sqlite3_stmt* st = nullptr;
    sqlite3_prepare_v2(g_db, sql, -1, &st, nullptr);
    sqlite3_bind_int(st, 1, id);
    sqlite3_step(st);
    std::cout << "Удалено строк: " << sqlite3_changes(g_db) << "\n";
    sqlite3_finalize(st);
}
