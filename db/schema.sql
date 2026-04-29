-- ============================================================
--  Схема БД «Музыкальный салон» (SQLite 3)
--  Вариант 5, группа 13
-- ============================================================

PRAGMA foreign_keys = ON;

-- ── Пользователи системы ──────────────────────────────────────
CREATE TABLE IF NOT EXISTS users (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    login         TEXT    NOT NULL UNIQUE,
    password_hash TEXT    NOT NULL,
    role          TEXT    NOT NULL DEFAULT 'buyer'
                          CHECK(role IN ('buyer','manager'))
);

-- ── Компакт-диски ─────────────────────────────────────────────
CREATE TABLE IF NOT EXISTS compacts (
    id               INTEGER PRIMARY KEY AUTOINCREMENT,
    code             TEXT    NOT NULL UNIQUE,
    manufacture_date TEXT    NOT NULL,   -- формат YYYY-MM-DD
    company          TEXT    NOT NULL,
    price            REAL    NOT NULL CHECK(price >= 0)
);

-- ── Музыкальные произведения ──────────────────────────────────
CREATE TABLE IF NOT EXISTS tracks (
    id         INTEGER PRIMARY KEY AUTOINCREMENT,
    title      TEXT    NOT NULL,
    author     TEXT    NOT NULL,
    performer  TEXT    NOT NULL,
    compact_id INTEGER NOT NULL
               REFERENCES compacts(id) ON DELETE CASCADE
);

-- ── Операции прихода и продажи ────────────────────────────────
CREATE TABLE IF NOT EXISTS operations (
    id         INTEGER PRIMARY KEY AUTOINCREMENT,
    op_date    TEXT    NOT NULL,
    op_type    TEXT    NOT NULL CHECK(op_type IN ('receive','sell')),
    compact_id INTEGER NOT NULL REFERENCES compacts(id),
    quantity   INTEGER NOT NULL CHECK(quantity > 0),
    user_id    INTEGER REFERENCES users(id)
);

-- ── Периодический отчёт ───────────────────────────────────────
CREATE TABLE IF NOT EXISTS period_report (
    id           INTEGER PRIMARY KEY AUTOINCREMENT,
    date_from    TEXT    NOT NULL,
    date_to      TEXT    NOT NULL,
    compact_id   INTEGER NOT NULL REFERENCES compacts(id),
    received_qty INTEGER NOT NULL DEFAULT 0,
    sold_qty     INTEGER NOT NULL DEFAULT 0
);

-- ── Триггер: запрет продажи больше чем поступило ─────────────
CREATE TRIGGER IF NOT EXISTS trg_check_sell
BEFORE INSERT ON operations
WHEN NEW.op_type = 'sell'
BEGIN
    SELECT CASE
        WHEN (
            SELECT COALESCE(SUM(quantity), 0)
            FROM operations
            WHERE compact_id = NEW.compact_id AND op_type = 'sell'
        ) + NEW.quantity
        > (
            SELECT COALESCE(SUM(quantity), 0)
            FROM operations
            WHERE compact_id = NEW.compact_id AND op_type = 'receive'
        )
        THEN RAISE(ABORT, 'Недостаточно компактов на складе')
    END;
END;

-- ── Тестовые данные ───────────────────────────────────────────
INSERT INTO users(login, password_hash, role) VALUES
    ('admin',  '8c6976e5b5410415bde908bd4dee15dfb167a9c873fc4bb8a81f6f2ab448a918', 'manager'),
    ('buyer1', 'a665a45920422f9d417e4867efdc4fb8a04a1f3fff1fa07e998e86f7f7a27ae3', 'buyer');

INSERT INTO compacts(code, manufacture_date, company, price) VALUES
    ('CD001', '2023-01-15', 'SonyMusic',   450.00),
    ('CD002', '2023-03-20', 'WarnerMusic', 520.00),
    ('CD003', '2022-11-10', 'Universal',   390.00);

INSERT INTO tracks(title, author, performer, compact_id) VALUES
    ('Bohemian Rhapsody', 'Freddie Mercury',   'Queen',           1),
    ('We Will Rock You',  'Brian May',          'Queen',           1),
    ('Thriller',          'Michael Jackson',    'Michael Jackson', 2),
    ('Beat It',           'Michael Jackson',    'Michael Jackson', 2),
    ('Hotel California',  'Don Felder',         'Eagles',          3);

INSERT INTO operations(op_date, op_type, compact_id, quantity, user_id) VALUES
    ('2024-01-10', 'receive', 1, 50, 1),
    ('2024-01-10', 'receive', 2, 30, 1),
    ('2024-01-10', 'receive', 3, 40, 1),
    ('2024-02-01', 'sell',    1, 10, 1),
    ('2024-02-05', 'sell',    2,  8, 1),
    ('2024-02-10', 'sell',    3, 15, 1),
    ('2024-03-01', 'sell',    1,  5, 1),
    ('2024-03-15', 'sell',    2, 12, 1);
