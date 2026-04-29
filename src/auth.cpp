#include "auth.h"
#include "db.h"
#include <iostream>
#include <cstring>

Session g_session;

bool auth_login(const std::string& login, const std::string& password) {
    if (!g_db) return false;

    const char* sql =
        "SELECT id, role, password_hash FROM users WHERE login = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_text(stmt, 1, login.c_str(), -1, SQLITE_STATIC);

    bool ok = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string stored(reinterpret_cast<const char*>(
            sqlite3_column_text(stmt, 2)));
        // Учебный режим: сравниваем пароль напрямую с хранимым хэшем
        if (password == stored) {
            g_session.id     = sqlite3_column_int(stmt, 0);
            g_session.role   = reinterpret_cast<const char*>(
                sqlite3_column_text(stmt, 1));
            g_session.login  = login;
            g_session.active = true;
            ok = true;
        }
    }
    sqlite3_finalize(stmt);
    return ok;
}

void auth_logout() {
    g_session = Session{};
}

bool auth_is_manager() {
    return g_session.active && g_session.role == "manager";
}
