CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g -Iincludes
LDFLAGS  = -lsqlite3
COVFLAGS = --coverage -fprofile-arcs -ftest-coverage

SRC  = src/db.cpp src/auth.cpp src/compact.cpp src/operations.cpp src/reports.cpp
OBJ  = $(SRC:src/%.cpp=build/%.o)

.PHONY: all clean test coverage

# ── Основной бинарник ──────────────────────────────────────────────────────────
all: bin/music_salon

bin/music_salon: $(OBJ) build/main.o | bin
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

build/main.o: src/main.cpp | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

build/%.o: src/%.cpp | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

bin:
	mkdir -p bin

build:
	mkdir -p build

db:
	mkdir -p db

# ── Тесты ──────────────────────────────────────────────────────────────────────
TEST_SRCS = tests/test_auth.cpp tests/test_compact.cpp tests/test_operations.cpp
TEST_BINS = $(TEST_SRCS:tests/%.cpp=bin/%)

test: $(TEST_BINS)
	@echo "=== Запуск всех тестов ==="
	@for t in $(TEST_BINS); do \
	    echo "--- $$t ---"; \
	    ./$$t || exit 1; \
	done
	@echo "=== Все тесты пройдены ==="

bin/test_%: tests/test_%.cpp $(SRC) | bin build db
	$(CXX) $(CXXFLAGS) $(COVFLAGS) $^ -o $@ $(LDFLAGS)

# ── Покрытие ───────────────────────────────────────────────────────────────────
coverage: test
	@echo "--- Генерация gcov отчёта ---"
	gcov -o bin/ tests/test_auth.cpp tests/test_compact.cpp tests/test_operations.cpp 2>/dev/null || true
	@echo "--- Генерация lcov + HTML ---"
	lcov --capture \
	     --directory bin/ \
	     --output-file coverage.info \
	     --ignore-errors mismatch 2>/dev/null || true
	lcov --remove coverage.info '/usr/*' '*/tests/*' \
	     --output-file coverage_filtered.info 2>/dev/null || true
	genhtml coverage_filtered.info \
	     --output-directory coverage_html 2>/dev/null || true
	@echo "HTML-отчёт: coverage_html/index.html"
	gcovr --root . --exclude tests/ --print-summary 2>/dev/null || true

# ── Очистка ────────────────────────────────────────────────────────────────────
clean:
	rm -rf bin build db/*.db *.gcov *.gcda *.gcno src/*.gcda src/*.gcno
