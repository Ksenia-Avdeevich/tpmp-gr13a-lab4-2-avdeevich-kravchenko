#pragma once
#include <sqlite3.h>
#include <string>

extern sqlite3* g_db;

bool db_open(const std::string& path);
void db_close();
bool db_init_schema();
