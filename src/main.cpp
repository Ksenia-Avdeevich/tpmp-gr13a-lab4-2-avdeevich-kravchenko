#include <iostream>
#include <string>
#include "db.h"
#include "auth.h"
#include "compact.h"
#include "operations.h"
#include "reports.h"

static constexpr const char* DB_PATH = "db/music_salon.db";

static void menu_buyer() {
    int choice = -1;
    while (choice != 0) {
        std::cout << "\n=== Меню покупателя [" << g_session.login << "] ===\n"
                  << "1.  Каталог компактов\n"
                  << "2.  Продажи и остатки по всем CD\n"
                  << "3.  Топ-компакт (макс. продаж) + треки\n"
                  << "4.  Наиболее популярный исполнитель\n"
                  << "5.  Статистика по авторам\n"
                  << "6.  Продажи по CD за период\n"
                  << "0.  Выйти из аккаунта\n"
                  << "Выбор: ";
        std::cin >> choice;
        switch (choice) {
            case 1: compact_list_all();              break;
            case 2: report_sold_remaining_all();     break;
            case 3: report_top_compact();            break;
            case 4: report_popular_performer();      break;
            case 5: report_by_author();              break;
            case 6: report_sales_by_compact_period();break;
            case 0: break;
            default: std::cout << "Неверный выбор.\n";
        }
    }
}

static void menu_manager() {
    int choice = -1;
    while (choice != 0) {
        std::cout << "\n=== Меню менеджера [" << g_session.login << "] ===\n"
                  << "--- Каталог ---\n"
                  << "1.  Список компактов\n"
                  << "2.  Добавить компакт\n"
                  << "3.  Изменить цену компакта\n"
                  << "4.  Удалить компакт\n"
                  << "--- Операции ---\n"
                  << "5.  Зарегистрировать операцию (приход/продажа)\n"
                  << "--- Отчёты ---\n"
                  << "6.  Продажи и остатки по всем CD\n"
                  << "7.  Продажи по CD за период\n"
                  << "8.  Топ-компакт + треки\n"
                  << "9.  Наиболее популярный исполнитель\n"
                  << "10. Статистика по авторам\n"
                  << "11. Сохранить периодический отчёт в таблицу\n"
                  << "12. Продажи по коду компакта за период\n"
                  << "0.  Выйти из аккаунта\n"
                  << "Выбор: ";
        std::cin >> choice;
        switch (choice) {
            case 1:  compact_list_all();               break;
            case 2:  compact_add();                    break;
            case 3:  compact_update();                 break;
            case 4:  compact_delete();                 break;
            case 5:  operation_register();             break;
            case 6:  report_sold_remaining_all();      break;
            case 7:  report_sold_by_compact_period();  break;
            case 8:  report_top_compact();             break;
            case 9:  report_popular_performer();       break;
            case 10: report_by_author();               break;
            case 11: report_period_to_table();         break;
            case 12: report_sales_by_compact_period(); break;
            case 0: break;
            default: std::cout << "Неверный выбор.\n";
        }
    }
}

int main() {
    std::cout << "=== Музыкальный салон ===\n";

    if (!db_open(DB_PATH))      return 1;
    if (!db_init_schema()) {
        std::cerr << "Не получилось инициализировать схему БД.\n";
        db_close();
        return 1;
    }

    int attempts = 0;
    while (true) {
        if (attempts >= 3) {
            std::cout << "Превышено количество попыток входа.\n";
            break;
        }
        std::string login, password;
        std::cout << "\nЛогин: ";   std::cin >> login;
        std::cout << "Пароль: ";   std::cin >> password;

        if (auth_login(login, password)) {
            std::cout << "Добро пожаловать, " << g_session.login
                      << "! (роль: " << g_session.role << ")\n";

            if (auth_is_manager()) menu_manager();
            else                   menu_buyer();

            auth_logout();
            attempts = 0;
            std::cout << "Вы вышли из системы.\n";

            std::string again;
            std::cout << "Войти снова? (да/нет): "; std::cin >> again;
            if (again != "да") break;
        } else {
            std::cout << "Неверный логин или пароль.\n";
            ++attempts;
        }
    }

    db_close();
    std::cout << "До свидания!\n";
    return 0;
}
