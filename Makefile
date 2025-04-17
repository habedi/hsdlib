####################################################################################################
## Project Configuration
####################################################################################################
# Project directories
SRC_DIR   := src
INC_DIR   := include
TEST_DIR  := tests
BIN_DIR   := bin
LIB_DIR   := lib
TARGET_DIR:= obj
DOC_DIR   := docs
COV_DIR   := coverage_reports

# Target names
TEST_RUNNER := $(BIN_DIR)/test_runner
STATIC_LIB  := $(LIB_DIR)/libhsd.a
SHARED_LIB  := $(LIB_DIR)/libhsd.so

####################################################################################################
## Build Configuration
####################################################################################################
# Compiler and archiver
CC   := gcc # Change to `clang` if needed
# NVCC := nvcc # REMOVED CUDA compiler
AR   := ar

# Shell configuration
SHELL := /bin/bash
.SHELLFLAGS := -e -o pipefail -c
PATH := $(if $(findstring /snap/bin,$(PATH)),$(PATH),/snap/bin:$(PATH))

# Debug/Release configuration
BUILD_TYPE ?= debug
ifeq ($(BUILD_TYPE),release)
CFLAGS += -O2
else
CFLAGS += -g -O0
CFLAGS += -DHSD_DEBUG # Define HSD_DEBUG for debug builds automatically
endif

# REMOVED GPU backend selection variables (USE_CUDA, USE_OPENCL)

####################################################################################################
## Compiler Flags
####################################################################################################
# Base C flags
CFLAGS_BASE := -Wall -Wextra -pedantic -std=c11 -fPIC -Iinclude -march=native
CFLAGS      := $(CFLAGS_BASE)
CFLAGS     += -I$(TEST_DIR) # Add tests dir to include path for test_common.h etc.
LDFLAGS     :=
LIBS        := -lm # Link math library by default

# Dependency generation flags
COMP_FLAGS := -MMD -MP

# REMOVED CUDA flags (NVCCFLAGS)
# REMOVED CUDA configuration block
# REMOVED OpenCL configuration block

####################################################################################################
## Source and Object Files
####################################################################################################
# Library source files
LIB_SRC_FILES := $(wildcard $(SRC_DIR)/*.c)
LIB_OBJ_FILES := $(patsubst $(SRC_DIR)/%.c, $(TARGET_DIR)/%.o, $(LIB_SRC_FILES))

# Test source files
TEST_SRC_FILES := $(wildcard $(TEST_DIR)/*.c)
TEST_OBJ_FILES := $(patsubst $(TEST_DIR)/%.c, $(TARGET_DIR)/%.o, $(TEST_SRC_FILES))

# All object files for dependency generation
ALL_OBJ_FILES := $(LIB_OBJ_FILES) $(TEST_OBJ_FILES)
DEP_FILES     := $(ALL_OBJ_FILES:.o=.d)

# REMOVED CUDA_SRC_FILES definition

####################################################################################################
## Directory Creation Rules
####################################################################################################
$(BIN_DIR) $(TARGET_DIR) $(LIB_DIR) $(COV_DIR):
	mkdir -p $@

####################################################################################################
## Default Target
####################################################################################################
.DEFAULT_GOAL := help

.PHONY: help
help: ## Show help message for each target
	@grep -h -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'

####################################################################################################
## Library Build Rules
####################################################################################################
.PHONY: all
all: static shared ## Build static and shared libraries
	@echo "Building libraries..."

.PHONY: static
static: $(STATIC_LIB) ## Build static library
	@echo "Static library built at $(STATIC_LIB)"

$(STATIC_LIB): $(LIB_OBJ_FILES) | $(LIB_DIR)
	$(AR) rcs $@ $^

.PHONY: shared
shared: $(SHARED_LIB) ## Build shared library
	@echo "Shared library built at $(SHARED_LIB)"

$(SHARED_LIB): $(LIB_OBJ_FILES) | $(LIB_DIR)
	$(CC) $(CFLAGS_BASE) -shared -o $@ $^ $(LDFLAGS) $(LIBS)

.PHONY: rebuild
rebuild: clean all ## Clean and rebuild libraries
	@echo "Rebuilding libraries..."

####################################################################################################
## Object Compilation Rules
####################################################################################################
# Library object compilation (Simplified - No CUDA check)
$(LIB_OBJ_FILES): $(TARGET_DIR)/%.o: $(SRC_DIR)/%.c | $(TARGET_DIR) $(INC_DIR)/hsdlib.h
	@echo "Compiling library source $< with $(CC)..."
	$(CC) $(CFLAGS) $(COMP_FLAGS) -c $< -o $@

# Test object compilation
$(TEST_OBJ_FILES): $(TARGET_DIR)/%.o: $(TEST_DIR)/%.c | $(TARGET_DIR) $(INC_DIR)/hsdlib.h $(wildcard $(TEST_DIR)/test_common.h)
	@echo "Compiling test source $<..."
	$(CC) $(CFLAGS) $(COMP_FLAGS) -c $< -o $@

####################################################################################################
## Testing Targets
####################################################################################################
.PHONY: test
test: $(TEST_RUNNER) ## Compile and run the single test runner binary
	@echo "Running tests..."
	./$(TEST_RUNNER)

$(TEST_RUNNER): $(TEST_OBJ_FILES) $(LIB_OBJ_FILES) | $(BIN_DIR)
	@echo "Linking test runner $@"
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS)

# Valgrind-compatible test target (disables AVX512)
TEST_RUNNER_VALGRIND := $(BIN_DIR)/test_runner_valgrind
.PHONY: test-valgrind
test-valgrind: CFLAGS += -DNO_AVX512 -march=haswell
test-valgrind: $(TEST_RUNNER_VALGRIND) ## Run tests with Valgrind (AVX512 disabled)
	@echo "Running tests with Valgrind..."
	valgrind --leak-check=full --error-exitcode=1 --suppressions=valgrind.supp ./$(TEST_RUNNER_VALGRIND)

$(TEST_RUNNER_VALGRIND): $(TEST_OBJ_FILES:$(TARGET_DIR)/%.o=$(TARGET_DIR)/%.valgrind.o) $(LIB_OBJ_FILES:$(TARGET_DIR)/%.o=$(TARGET_DIR)/%.valgrind.o) | $(BIN_DIR)
	@echo "Linking Valgrind-compatible test runner $@"
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS)

# Valgrind-compatible object files
$(TARGET_DIR)/%.valgrind.o: $(SRC_DIR)/%.c | $(TARGET_DIR) $(INC_DIR)/hsdlib.h
	@echo "Compiling library source $< for Valgrind compatibility..."
	$(CC) $(CFLAGS) $(COMP_FLAGS) -c $< -o $@

$(TARGET_DIR)/%.valgrind.o: $(TEST_DIR)/%.c | $(TARGET_DIR) $(INC_DIR)/hsdlib.h $(wildcard $(TEST_DIR)/test_common.h)
	@echo "Compiling test source $< for Valgrind compatibility..."
	$(CC) $(CFLAGS) $(COMP_FLAGS) -c $< -o $@

####################################################################################################
## Code Coverage
####################################################################################################
.PHONY: coverage-clean
coverage-clean:
	@echo "Cleaning object files for coverage build..."
	rm -f $(TARGET_DIR)/*.o

coverage: CFLAGS += -fprofile-arcs -ftest-coverage
coverage: LIBS += -lgcov
.PHONY: coverage
coverage: coverage-clean $(TEST_RUNNER) ## Generate code coverage report for src/ files
	@echo "Running tests for coverage data generation..."
	./$(TEST_RUNNER)
	@echo "Generating gcov files for src/ directory only..."
	gcov --preserve-paths $(SRC_DIR)/*.c -o $(TARGET_DIR)
	find . -maxdepth 1 -name '#usr#*.gcov' -delete -o -name '#opt#*.gcov' -delete

####################################################################################################
## Installation Targets
####################################################################################################
.PHONY: install
install: static shared ## Build and install headers and libs (to /usr/local)
	@echo "Installing libraries and headers..."
	install -d /usr/local/include /usr/local/lib
	install -m 0644 $(INC_DIR)/*.h /usr/local/include/
	install -m 0644 $(STATIC_LIB) /usr/local/lib/
	install -m 0755 $(SHARED_LIB) /usr/local/lib/
	ldconfig || true

.PHONY: uninstall
uninstall: ## Uninstall headers and libs (from /usr/local)
	@echo "Uninstalling libraries and headers..."
	@for header in $(wildcard $(INC_DIR)/*.h); do \
	   rm -f /usr/local/include/$$(basename $$header); \
	done
	rm -f /usr/local/lib/libhsd.a
	rm -f /usr/local/lib/libhsd.so
	ldconfig || true

####################################################################################################
## Development Tools
####################################################################################################
.PHONY: install-deps
install-deps: ## Install development dependencies (for Debian-based systems)
	@echo "Installing development dependencies..."
	sudo apt-get update && sudo apt-get install -y \
		gcc llvm gdb clang clang-format clang-tools cppcheck valgrind graphviz \
		kcachegrind graphviz linux-tools-common linux-tools-generic linux-tools-$(shell uname -r)

.PHONY: format
format: ## Format code with clang-format (requires a .clang-format file)
	@echo "Formatting code..."
	clang-format -i $(LIB_SRC_FILES) $(wildcard $(INC_DIR)/*.h) $(TEST_SRC_FILES)

.PHONY: lint
lint: ## Run linter checks
	cppcheck --enable=all --inconclusive --quiet --force --std=c11 -I$(INC_DIR) -I$(TEST_DIR) \
	         --suppress=missingIncludeSystem \
	         --suppress=unusedFunction \
	         --suppress=constVariable \
	         $(SRC_DIR) $(INC_DIR) $(TEST_DIR)

.PHONY: docs
docs: ## Generate docs with Doxygen
	@echo "Generating documentation..."
	@test -f Doxyfile || { echo "Error: Doxyfile not found."; exit 1; }
	doxygen Doxyfile

.PHONY: clean
clean: ## Remove all build artifacts, including coverage reports and test runner
	@echo "Cleaning up..."
	rm -rf $(BIN_DIR) $(TARGET_DIR) $(LIB_DIR) $(COV_DIR)
	find . \( -name '*.gcda' -o -name '*.gcno' -o -name '*.gcov' -o -name '*.d' -o -name '*.o' -o -name '*.a' -o -name '*.so' \) -delete
	rm -rf Doxyfile.bak $(DOC_DIR)/html $(DOC_DIR)/latex

# Include dependency files
-include $(DEP_FILES)
