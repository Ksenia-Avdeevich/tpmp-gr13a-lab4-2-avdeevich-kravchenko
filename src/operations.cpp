#include "operations.h"
#include "auth.h"
#include "db.h"
#include <iostream>
#include <string>

void operation_register() {
    if (!auth_is_manager()) {
        std::cout << "Нет прав. Требуется роль менеджера.\n";
        return;
    }
    int compact_id = 0, qty = 0;
    std::string op_type, date;
    std::cout << "ID компакта: ";                    std::cin >> compact_id;
    std::cout << "Тип операции (receive/sell): ";    std::cin >> op_type;
    std::cout << "Дата (YYYY-MM-DD): ";              std::cin >> date;
    std::cout << "Количество: ";                     std::cin >> qty;

    const char* sql =
        "INSERT INTO operations(op_date,op_type,compact_id,quantity,user_id)"
        " VALUES(?,?,?,?,?);";
    sqlite3_stmt* st = nullptr;
    sqlite3_prepare_v2(g_db, sql, -1, &st, nullptr);
    sqlite3_bind_text(st, 1, date.c_str(),    -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 2, op_type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(st,  3, compact_id);
    sqlite3_bind_int(st,  4, qty);
    sqlite3_bind_int(st,  5, g_session.id);

    if (sqlite3_step(st) == SQLITE_DONE)
        std::cout << "Операция зарегистрирована.\n";
    else
        std::cout << "Ошибка: " << sqlite3_errmsg(g_db) << "\n";
    sqlite3_finalize(st);
}
