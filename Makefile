####################################################################################################
## Variables
####################################################################################################
# Compiler and archiver
CC       := gcc # Change to `clang` if needed
AR       := ar

# Build configuration: set BUILD_TYPE=release for an optimized build.
BUILD_TYPE ?= debug
ifeq ($(BUILD_TYPE),release)
    CFLAGS += -O2
else
    CFLAGS += -g -O0
endif

# Common flags, including automatic dependency generation (-MMD -MP)
CFLAGS   += -Wall -Wextra -pedantic -std=c11 -fPIC -Iinclude -MMD -MP
LDFLAGS  :=
LIBS     :=

# Directories
SRC_DIR   := src
INC_DIR   := include
TEST_DIR  := test
BIN_DIR   := bin
LIB_DIR   := lib
TARGET_DIR:= obj
DOC_DIR   := docs

# Names and Files
BINARY_NAME   := main
BINARY        := $(BIN_DIR)/$(BINARY_NAME)
TEST_BINARY   := $(BIN_DIR)/test_$(BINARY_NAME)
SRC_FILES     := $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES     := $(patsubst $(SRC_DIR)/%.c, $(TARGET_DIR)/%.o, $(SRC_FILES))
DEP_FILES     := $(OBJ_FILES:.o=.d)
TEST_FILES    := $(wildcard $(TEST_DIR)/*.c)
STATIC_LIB    := $(LIB_DIR)/libproject.a
SHARED_LIB    := $(LIB_DIR)/libproject.so

# Adjust PATH if necessary (append /snap/bin if not present)
PATH := $(if $(findstring /snap/bin,$(PATH)),$(PATH),/snap/bin:$(PATH))

SHELL := /bin/bash
.SHELLFLAGS := -e -o pipefail -c

# Create directories as needed
$(BIN_DIR) $(TARGET_DIR) $(LIB_DIR):
	mkdir -p $@

####################################################################################################
## C Targets
####################################################################################################

.DEFAULT_GOAL := help

.PHONY: help
help: ## Show help message for each target (try: make -j4 help for parallel build suggestion)
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'

.PHONY: all
all: build static shared ## Build everything
	@echo "Building all targets..."

.PHONY: build
build: $(BINARY) ## Build the main binary
	@echo "Main binary built at $(BINARY)"

$(BINARY): $(OBJ_FILES) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS)

# Build object files with dependency generation
$(TARGET_DIR)/%.o: $(SRC_DIR)/%.c | $(TARGET_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: rebuild
rebuild: clean all ## Clean and rebuild everything
	@echo "Rebuilding all targets..."

.PHONY: run
run: build ## Run the main binary
	@echo "Running main binary..."
	./$(BINARY)

.PHONY: test
test: $(TEST_BINARY) ## Run the tests
	@echo "Running tests..."
	./$(TEST_BINARY)

$(TEST_BINARY): $(TEST_FILES) $(filter-out $(TARGET_DIR)/main.o, $(OBJ_FILES)) | $(BIN_DIR)
	@echo "Building test binary..."
	$(CC) $(CFLAGS) $(TEST_FILES) $(filter-out $(TARGET_DIR)/main.o, $(OBJ_FILES)) -o $@

.PHONY: test-valgrind
test-valgrind: $(TEST_BINARY) ## Run tests with Valgrind using suppressions
	@echo "Running tests with Valgrind..."
	valgrind --leak-check=full --suppressions=valgrind.supp ./$(TEST_BINARY)

.PHONY: static
static: $(STATIC_LIB) ## Build static library
	@echo "Static library built at $(STATIC_LIB)"

$(STATIC_LIB): $(OBJ_FILES) | $(LIB_DIR)
	$(AR) rcs $@ $^

.PHONY: shared
shared: $(SHARED_LIB) ## Build shared library
	@echo "Shared library built at $(SHARED_LIB)"

$(SHARED_LIB): $(OBJ_FILES) | $(LIB_DIR)
	$(CC) -shared -o $@ $^

.PHONY: install
install: all ## Install binary, headers, and libs (to /usr/local)
	@echo "Installing..."
	install -d /usr/local/bin /usr/local/include /usr/local/lib
	install -m 0755 $(BINARY) /usr/local/bin/
	cp -r $(INC_DIR)/*.h /usr/local/include/
	install -m 0644 $(STATIC_LIB) /usr/local/lib/
	install -m 0755 $(SHARED_LIB) /usr/local/lib/

.PHONY: uninstall
uninstall: ## Uninstall everything (from /usr/local)
	@echo "Uninstalling..."
	rm -f /usr/local/bin/$(BINARY_NAME)
	rm -f /usr/local/include/*.h
	rm -f /usr/local/lib/libproject.a
	rm -f /usr/local/lib/libproject.so

.PHONY: format
format: ## Format code with clang-format (requires a .clang-format file)
	@echo "Formatting code..."
	clang-format -i $(SRC_FILES) $(wildcard $(INC_DIR)/*.h) $(TEST_FILES)

.PHONY: lint
lint: ## Run cppcheck and clang-tidy
	cppcheck --enable=all --inconclusive --quiet --std=c11 -I$(INC_DIR) --suppress=missingIncludeSystem $(SRC_DIR) $(INC_DIR) $(TEST_DIR)
	@if command -v clang-tidy &> /dev/null; then \
		clang-tidy $(SRC_FILES) -- $(CFLAGS); \
	else \
		echo "clang-tidy not found. Skipping."; \
	fi

.PHONY: docs
docs: ## Generate docs with Doxygen
	@echo "Generating documentation..."
	doxygen Doxyfile

.PHONY: coverage
coverage: CFLAGS += -fprofile-arcs -ftest-coverage -fprofile-prefix-map=$(PWD)=.
coverage: LDFLAGS += -lgcov
coverage: test ## Generate code coverage report
	@echo "Generating code coverage report..."
	gcov -o $(TARGET_DIR) $(filter-out $(SRC_DIR)/main.c, $(SRC_FILES))

.PHONY: clean
clean: ## Remove all build artifacts
	@echo "Cleaning up..."
	rm -rf $(BIN_DIR) $(TARGET_DIR) $(LIB_DIR) *.gcno *.gcda *.gcov *.out *.o *.a *.so *.d
	rm -rf $(DOC_DIR)/html $(DOC_DIR)/latex Doxyfile.bak

.PHONY: install-deps
install-deps: ## Install system and development dependencies (for Debian-based OSes)
	@echo "Installing system dependencies..."
	sudo apt-get update
	sudo apt-get install -y gcc clang clang-format clang-tidy doxygen cppcheck valgrind gdb

# Include dependency files, if they exist.
-include $(DEP_FILES)
