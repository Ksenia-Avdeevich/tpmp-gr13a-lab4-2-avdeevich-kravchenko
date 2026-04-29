#pragma once
#include <string>

struct Session {
    int         id   = 0;
    std::string login;
    std::string role;
    bool        active = false;
};

extern Session g_session;

bool auth_login(const std::string& login, const std::string& password);
void auth_logout();
bool auth_is_manager();
