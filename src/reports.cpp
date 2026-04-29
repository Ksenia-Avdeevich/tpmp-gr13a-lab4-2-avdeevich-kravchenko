#include "reports.h"
#include "db.h"
#include <iostream>
#include <iomanip>
#include <string>

// ── Запрос 1: все компакты — продано и остаток по убыванию разницы ──────────
void report_sold_remaining_all() {
    const char* sql = R"(
SELECT c.code, c.company,
  COALESCE(SUM(CASE WHEN o.op_type='receive' THEN o.quantity ELSE 0 END),0) AS recv,
  COALESCE(SUM(CASE WHEN o.op_type='sell'    THEN o.quantity ELSE 0 END),0) AS sold
FROM compacts c LEFT JOIN operations o ON c.id=o.compact_id
GROUP BY c.id
ORDER BY (recv - sold) DESC;
)";
    sqlite3_stmt* st = nullptr;
    sqlite3_prepare_v2(g_db, sql, -1, &st, nullptr);
    std::cout << "\n"
              << std::left
              << std::setw(9)  << "Код"
              << std::setw(22) << "Компания"
              << std::setw(12) << "Поступило"
              << std::setw(12) << "Продано"
              << std::setw(12) << "Остаток" << "\n"
              << std::string(67, '-') << "\n";
    while (sqlite3_step(st) == SQLITE_ROW) {
        int recv = sqlite3_column_int(st, 2);
        int sold = sqlite3_column_int(st, 3);
        std::cout << std::left
                  << std::setw(9)  << sqlite3_column_text(st, 0)
                  << std::setw(22) << sqlite3_column_text(st, 1)
                  << std::setw(12) << recv
                  << std::setw(12) << sold
                  << std::setw(12) << (recv - sold) << "\n";
    }
    sqlite3_finalize(st);
}

// ── Запрос 2: по указанному компакту — кол-во и стоимость за период ──────────
void report_sold_by_compact_period() {
    int id = 0;
    std::string d1, d2;
    std::cout << "ID компакта: ";               std::cin >> id;
    std::cout << "Дата начала (YYYY-MM-DD): ";  std::cin >> d1;
    std::cout << "Дата конца  (YYYY-MM-DD): ";  std::cin >> d2;

    const char* sql = R"(
SELECT c.code,
       SUM(o.quantity)           AS qty,
       SUM(o.quantity * c.price) AS total
FROM operations o JOIN compacts c ON c.id=o.compact_id
WHERE o.compact_id=? AND o.op_type='sell'
  AND o.op_date BETWEEN ? AND ?;
)";
    sqlite3_stmt* st = nullptr;
    sqlite3_prepare_v2(g_db, sql, -1, &st, nullptr);
    sqlite3_bind_int(st,  1, id);
    sqlite3_bind_text(st, 2, d1.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 3, d2.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(st) == SQLITE_ROW && sqlite3_column_text(st, 0)) {
        std::cout << "\nКомпакт: "  << sqlite3_column_text(st, 0)
                  << " | Продано: " << sqlite3_column_int(st, 1)
                  << " шт. | Сумма: "
                  << std::fixed << std::setprecision(2)
                  << sqlite3_column_double(st, 2) << " руб.\n";
    } else {
        std::cout << "Данных не найдено.\n";
    }
    sqlite3_finalize(st);
}

// ── Запрос 3: топ-компакт + все его треки ────────────────────────────────────
void report_top_compact() {
    const char* sql = R"(
SELECT c.id, c.code, c.manufacture_date, c.company, c.price,
       SUM(o.quantity) AS total_sold
FROM compacts c JOIN operations o ON c.id=o.compact_id
WHERE o.op_type='sell'
GROUP BY c.id
ORDER BY total_sold DESC LIMIT 1;
)";
    sqlite3_stmt* st = nullptr;
    sqlite3_prepare_v2(g_db, sql, -1, &st, nullptr);
    if (sqlite3_step(st) == SQLITE_ROW) {
        int cid = sqlite3_column_int(st, 0);
        std::cout << "\nТоп компакт: "
                  << sqlite3_column_text(st, 1) << " | "
                  << sqlite3_column_text(st, 2) << " | "
                  << sqlite3_column_text(st, 3) << " | "
                  << std::fixed << std::setprecision(2)
                  << sqlite3_column_double(st, 4) << " руб."
                  << " | Продано: " << sqlite3_column_int(st, 5) << "\n";
        sqlite3_finalize(st);

        sqlite3_stmt* ts = nullptr;
        const char* tsql =
            "SELECT title,author,performer FROM tracks WHERE compact_id=?;";
        sqlite3_prepare_v2(g_db, tsql, -1, &ts, nullptr);
        sqlite3_bind_int(ts, 1, cid);
        std::cout << "  Треки:\n";
        while (sqlite3_step(ts) == SQLITE_ROW)
            std::cout << "  - " << sqlite3_column_text(ts, 0)
                      << " / "  << sqlite3_column_text(ts, 1)
                      << " / "  << sqlite3_column_text(ts, 2) << "\n";
        sqlite3_finalize(ts);
    } else {
        std::cout << "Данных нет.\n";
        sqlite3_finalize(st);
    }
}

// ── Запрос 4: наиболее популярный исполнитель ─────────────────────────────────
void report_popular_performer() {
    const char* sql = R"(
SELECT t.performer, SUM(o.quantity) AS sold
FROM tracks t
JOIN operations o ON o.compact_id=t.compact_id AND o.op_type='sell'
GROUP BY t.performer ORDER BY sold DESC LIMIT 1;
)";
    sqlite3_stmt* st = nullptr;
    sqlite3_prepare_v2(g_db, sql, -1, &st, nullptr);
    if (sqlite3_step(st) == SQLITE_ROW)
        std::cout << "\nНаиболее популярный: "
                  << sqlite3_column_text(st, 0)
                  << " | Продано компактов: "
                  << sqlite3_column_int(st, 1) << "\n";
    else
        std::cout << "Данных нет.\n";
    sqlite3_finalize(st);
}

// ── Запрос 5: по каждому автору — кол-во и выручка ───────────────────────────
void report_by_author() {
    const char* sql = R"(
SELECT t.author,
       SUM(o.quantity)           AS sold,
       SUM(o.quantity * c.price) AS revenue
FROM tracks t
JOIN operations o ON o.compact_id=t.compact_id AND o.op_type='sell'
JOIN compacts c   ON c.id=t.compact_id
GROUP BY t.author ORDER BY revenue DESC;
)";
    sqlite3_stmt* st = nullptr;
    sqlite3_prepare_v2(g_db, sql, -1, &st, nullptr);
    std::cout << "\n"
              << std::left
              << std::setw(28) << "Автор"
              << std::setw(12) << "Продано"
              << std::setw(16) << "Выручка (руб.)" << "\n"
              << std::string(56, '-') << "\n";
    while (sqlite3_step(st) == SQLITE_ROW)
        std::cout << std::left
                  << std::setw(28) << sqlite3_column_text(st, 0)
                  << std::setw(12) << sqlite3_column_int(st, 1)
                  << std::fixed << std::setprecision(2)
                  << sqlite3_column_double(st, 2) << "\n";
    sqlite3_finalize(st);
}

// ── Функция: периодический отчёт → таблица period_report ─────────────────────
void report_period_to_table() {
    std::string d1, d2;
    std::cout << "Начало периода (YYYY-MM-DD): "; std::cin >> d1;
    std::cout << "Конец  периода (YYYY-MM-DD): "; std::cin >> d2;

    const char* sql = R"(
INSERT INTO period_report(date_from, date_to, compact_id, received_qty, sold_qty)
SELECT ?, ?, c.id,
  COALESCE(SUM(CASE WHEN o.op_type='receive' AND o.op_date BETWEEN ? AND ?
                    THEN o.quantity ELSE 0 END), 0),
  COALESCE(SUM(CASE WHEN o.op_type='sell'    AND o.op_date BETWEEN ? AND ?
                    THEN o.quantity ELSE 0 END), 0)
FROM compacts c LEFT JOIN operations o ON c.id=o.compact_id
GROUP BY c.id;
)";
    sqlite3_stmt* st = nullptr;
    sqlite3_prepare_v2(g_db, sql, -1, &st, nullptr);
    sqlite3_bind_text(st, 1, d1.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 2, d2.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 3, d1.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 4, d2.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 5, d1.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 6, d2.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(st) == SQLITE_DONE)
        std::cout << "Отчёт за период " << d1 << " — " << d2
                  << " записан. Строк: " << sqlite3_changes(g_db) << "\n";
    else
        std::cout << "Ошибка: " << sqlite3_errmsg(g_db) << "\n";
    sqlite3_finalize(st);
}

// ── Функция: продажи по коду компакта за период ──────────────────────────────
void report_sales_by_compact_period() {
    int id = 0;
    std::string d1, d2;
    std::cout << "ID компакта: ";                std::cin >> id;
    std::cout << "Начало периода (YYYY-MM-DD): "; std::cin >> d1;
    std::cout << "Конец  периода (YYYY-MM-DD): "; std::cin >> d2;

    const char* sql = R"(
SELECT o.op_date, o.quantity, o.quantity * c.price AS revenue
FROM operations o JOIN compacts c ON c.id=o.compact_id
WHERE o.compact_id=? AND o.op_type='sell'
  AND o.op_date BETWEEN ? AND ?
ORDER BY o.op_date;
)";
    sqlite3_stmt* st = nullptr;
    sqlite3_prepare_v2(g_db, sql, -1, &st, nullptr);
    sqlite3_bind_int(st,  1, id);
    sqlite3_bind_text(st, 2, d1.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 3, d2.c_str(), -1, SQLITE_TRANSIENT);

    std::cout << "\n"
              << std::left
              << std::setw(14) << "Дата"
              << std::setw(10) << "Кол-во"
              << std::setw(14) << "Сумма" << "\n"
              << std::string(38, '-') << "\n";
    while (sqlite3_step(st) == SQLITE_ROW)
        std::cout << std::left
                  << std::setw(14) << sqlite3_column_text(st, 0)
                  << std::setw(10) << sqlite3_column_int(st, 1)
                  << std::fixed << std::setprecision(2)
                  << sqlite3_column_double(st, 2) << "\n";
    sqlite3_finalize(st);
}
