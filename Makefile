####################################################################################################
## Project Configuration
####################################################################################################
SRC_DIR    := src
INC_DIR    := include
TEST_DIR   := tests
BIN_DIR    := bin
LIB_DIR    := lib
TARGET_DIR := obj
DOC_DIR    := docs
COV_DIR    := coverage_reports
CLEAN_EXCLUDE_DIRS := env .env venv .venv

####################################################################################################
## Platform Detection
####################################################################################################
OS := $(shell uname -s)

# Platform specific library/exe names
ifeq ($(OS),Linux)
    SHARED_LIB_EXT := so
    SHARED_LIB_PREFIX := lib
    STATIC_LIB_EXT := a
    EXE_EXT :=
endif
ifeq ($(OS),Darwin)
    SHARED_LIB_EXT := dylib
    SHARED_LIB_PREFIX := lib
    STATIC_LIB_EXT := a
    EXE_EXT :=
endif

# Default to POSIX-like names
SHARED_LIB_FILENAME ?= $(SHARED_LIB_PREFIX)hsd.$(SHARED_LIB_EXT)
STATIC_LIB_FILENAME ?= $(SHARED_LIB_PREFIX)hsd.$(STATIC_LIB_EXT)
EXE_FILENAME ?= test_runner$(EXE_EXT)

# Target files with platform-aware names
SHARED_LIB := $(LIB_DIR)/$(SHARED_LIB_FILENAME)
STATIC_LIB := $(LIB_DIR)/$(STATIC_LIB_FILENAME)
TEST_RUNNER := $(BIN_DIR)/$(EXE_FILENAME)
TEST_RUNNER_VALGRIND := $(BIN_DIR)/test_runner_valgrind$(EXE_EXT)

####################################################################################################
## Build Configuration
####################################################################################################
CC   := $(shell command -v gcc 2>/dev/null || command -v clang 2>/dev/null)
AR   := ar

SHELL := /bin/bash
.SHELLFLAGS := -e -o pipefail -c
PATH := $(if $(findstring /snap/bin,$(PATH)),$(PATH),/snap/bin:$(PATH))

BUILD_TYPE ?= debug
ifeq ($(BUILD_TYPE),release)
CFLAGS += -O2
else
CFLAGS += -g -O0
CFLAGS += -DHSD_DEBUG
endif

####################################################################################################
## Compiler Flags
####################################################################################################
CFLAGS_BASE := -Wall -Wextra -pedantic -std=c11 -fPIC -Iinclude -march=native
CFLAGS      := $(CFLAGS_BASE)
CFLAGS     += -I$(TEST_DIR)
LDFLAGS     :=
LIBS        := -lm

COMP_FLAGS := -MMD -MP

####################################################################################################
## Source and Object Files
####################################################################################################
LIB_SRC_FILES := $(shell find $(SRC_DIR) -name '*.c')
LIB_OBJ_FILES := $(patsubst $(SRC_DIR)/%.c, $(TARGET_DIR)/%.o, $(LIB_SRC_FILES))
LIB_OBJ_DIRS  := $(sort $(dir $(LIB_OBJ_FILES)))

TEST_SRC_FILES := $(wildcard $(TEST_DIR)/*.c)
TEST_OBJ_FILES := $(patsubst $(TEST_DIR)/%.c, $(TARGET_DIR)/%.o, $(TEST_SRC_FILES))

ALL_OBJ_FILES := $(LIB_OBJ_FILES) $(TEST_OBJ_FILES)
DEP_FILES     := $(ALL_OBJ_FILES:.o=.d)

####################################################################################################
## Zig Configuration
####################################################################################################
ZIG := zig
ZIG_BUILD_FLAGS :=
ifeq ($(BUILD_TYPE),release)
ZIG_BUILD_FLAGS += -Doptimize=Release
else
ZIG_BUILD_FLAGS += -Doptimize=Debug
endif
ZIG_BUILD_FLAGS += -Dcpu=native

# Base Zig build command
ZIG_CMD := $(ZIG) build $(ZIG_BUILD_FLAGS)

# Zig output directories
ZIG_OUT_BIN := zig-out/bin
ZIG_OUT_LIB := zig-out/lib

####################################################################################################
## Python Configuration
####################################################################################################
PYTHON_DIST_DIR := dist
PYTHON_BUILD_DIR := build
PYTHON_EGG_INFO := $(shell find . -maxdepth 2 -type d -name '*.egg-info') UNKNOWN.egg-info

####################################################################################################
## Directory Creation Rules
####################################################################################################
$(BIN_DIR) $(LIB_DIR) $(DOC_DIR) $(COV_DIR) $(TARGET_DIR) $(LIB_OBJ_DIRS):
	mkdir -p $@

####################################################################################################
## Default Target
####################################################################################################
.DEFAULT_GOAL := help

.PHONY: help
help: ## Show help message for each target
	@grep -h -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*?## "}; \
 	{printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'

####################################################################################################
## Primary Build Targets
####################################################################################################
.PHONY: all
all: static shared ## Build static and shared libraries
	@echo "Building libraries..."

.PHONY: rebuild
rebuild: clean all ## Clean and rebuild libraries
	@echo "Rebuilding libraries..."

####################################################################################################
## Library Build Rules
####################################################################################################
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

####################################################################################################
## Object Compilation Rules
####################################################################################################
$(LIB_OBJ_FILES): $(TARGET_DIR)/%.o: $(SRC_DIR)/%.c | $(TARGET_DIR) $(INC_DIR)/hsdlib.h $(LIB_OBJ_DIRS)
	@echo "Compiling library source $< with $(CC)..."
	$(CC) $(CFLAGS) $(COMP_FLAGS) -c $< -o $@

$(TEST_OBJ_FILES): $(TARGET_DIR)/%.o: $(TEST_DIR)/%.c | $(TARGET_DIR) $(INC_DIR)/hsdlib.h $(wildcard $(TEST_DIR)/test_common.h)
	@echo "Compiling test source $<..."
	$(CC) $(CFLAGS) $(COMP_FLAGS) -c $< -o $@

####################################################################################################
## Testing Targets
####################################################################################################
.PHONY: test
test: $(TEST_RUNNER) ## Compile and run the test runner binary
	@echo "Running tests..."
	./$(TEST_RUNNER)

$(TEST_RUNNER): $(TEST_OBJ_FILES) $(LIB_OBJ_FILES) | $(BIN_DIR)
	@echo "Linking test runner $@"
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS)

####################################################################################################
## Code Coverage
####################################################################################################
.PHONY: coverage-clean
coverage-clean: ## Clean object files for coverage build
	@echo "Cleaning object files for coverage build..."
	rm -f $(TARGET_DIR)/*.o $(TARGET_DIR)/*/*.o # Clean subdirs too

coverage: CFLAGS += -fprofile-arcs -ftest-coverage
coverage: LIBS += -lgcov
.PHONY: coverage
coverage: coverage-clean $(TEST_RUNNER) ## Generate code coverage report for the C library
	@echo "Running tests for coverage data generation..."
	./$(TEST_RUNNER)
	@echo "Generating gcov files for src/ directory and subdirectories..."
	@for obj_file in $(LIB_OBJ_FILES); do \
		obj_dir=$$(dirname $$obj_file); \
		src_file=$$(echo $$obj_file | sed 's|$(TARGET_DIR)|$(SRC_DIR)|' | sed 's|\.o$$|.c|'); \
		echo "Processing coverage for $$src_file"; \
		(cd $$obj_dir && gcov --preserve-paths --object-directory . $$src_file); \
	done
	@find $(TARGET_DIR) -name "*.gcov" -exec mv {} . \;
	@find . -maxdepth 1 -name '#usr#*.gcov' -delete -o -name '#opt#*.gcov' -delete

####################################################################################################
## Zig Build Targets
####################################################################################################
.PHONY: zig-shared
zig-shared: | $(LIB_DIR) ## Build shared library using Zig build system
	@echo "Building shared library with Zig $(ZIG_BUILD_FLAGS)..."
	$(ZIG_CMD)
	@echo "Copying Zig output $(ZIG_OUT_LIB)/$(SHARED_LIB_FILENAME) to $(SHARED_LIB)..."
	mkdir -p $(LIB_DIR)
	cp "$(ZIG_OUT_LIB)/$(SHARED_LIB_FILENAME)" "$(SHARED_LIB)"

.PHONY: zig-static
zig-static: | $(LIB_DIR) ## Build static library using Zig build system
	@echo "Building static library with Zig $(ZIG_BUILD_FLAGS)..."
	$(ZIG_CMD)
	@echo "Copying Zig output $(ZIG_OUT_LIB)/$(STATIC_LIB_FILENAME) to $(STATIC_LIB)..."
	mkdir -p $(LIB_DIR)
	cp "$(ZIG_OUT_LIB)/$(STATIC_LIB_FILENAME)" "$(STATIC_LIB)"

.PHONY: zig-lib
zig-lib: zig-shared zig-static ## Build the static abd shared libraries using Zig build system
	@echo "Zig libraries built at $(SHARED_LIB) and $(STATIC_LIB)"

.PHONY: zig-test-c
zig-test-c: | $(BIN_DIR) ## Compile and run C tests using Zig build system
	@echo "Building C tests with Zig $(ZIG_BUILD_FLAGS)..."
	$(ZIG_CMD)
	@echo "Copying test runner to $(TEST_RUNNER)..."
	cp "$(ZIG_OUT_BIN)/test_runner" "$(TEST_RUNNER)"
	@echo "Running tests..."
	./$(TEST_RUNNER)

.PHONY: zig-clean
zig-clean: ## Remove Zig build artifacts
	@echo "Cleaning Zig build artifacts..."
	rm -rf zig-cache .zig-cache zig-out

####################################################################################################
## Python Targets
####################################################################################################
.PHONY: python-build
python-build: zig-shared ## Build the wheel file
	@echo "Copying shared library into Python package..."
	@echo "Building wheel using python -m build..."
	python -m build --wheel --outdir $(PYTHON_DIST_DIR)

.PHONY: python-install
python-install: python-build ## Build the wheel file and install it
	@echo "Finding the wheel file in $(PYTHON_DIST_DIR)..."
	$(eval WHEEL_FILE := $(shell find $(PYTHON_DIST_DIR) -type f -name '*.whl' | head -n 1))
	@if [ -z "$(WHEEL_FILE)" ]; then \
	   echo "ERROR: No wheel file found in $(PYTHON_DIST_DIR)."; exit 1; \
	fi
	@echo "Installing the wheel: $(WHEEL_FILE) using uv..."
	uv pip install --force-reinstall --no-deps "$(WHEEL_FILE)"

.PHONY: python-test
python-test: zig-shared ## Run Python tests
	@echo "Running Python tests..."
	uv run pytest python/tests --tb=short --disable-warnings --cov=python/hsdpy --cov-branch --cov-report=xml

.PHONY: python-clean
python-clean: ## Remove Python build artifacts and copied libs
	@echo "Cleaning Python build artifacts..."
	rm -rf $(PYTHON_DIST_DIR) $(PYTHON_BUILD_DIR) $(PYTHON_EGG_INFO) site_packages*
	@echo "Removing packaged shared libs from python/hsdpy..."
	rm -f python/hsdpy/libhsd.so python/hsdpy/libhsd.dylib python/hsdpy/hsd.dll
	@echo "Clearing Python bytecode caches..."
	find python -type d -name '__pycache__' -exec rm -rf {} +

####################################################################################################
## Development Tools
####################################################################################################
.PHONY: install-deps
install-deps: ## Install development dependencies (for Debian-based systems)
	@echo "Installing system dependencies..."
	sudo apt-get update && sudo apt-get install -y \
	   gcc clang llvm gdb clang-format clang-tools cppcheck graphviz \
	   doxygen python3-pip snapd
	@echo "Installing Python dependencies..."
	pip3 install -U uv
	@echo "Installing Zig..."
	snap install zig --classic --beta

.PHONY: format
format: ## Format C files
	@echo "Formatting code..."
	find $(SRC_DIR) $(INC_DIR) $(TEST_DIR) \( -name '*.c' -o -name '*.h' \) -exec clang-format -i {} +

.PHONY: lint
lint: ## Run linter checks
	@echo "Running linter checks..."
	cppcheck --enable=all --inconclusive --quiet --force --std=c11 -I$(INC_DIR) -I$(TEST_DIR) \
			 --suppress=missingIncludeSystem \
			 --suppress=unusedFunction \
			 --suppress=constVariable \
			 $(shell find $(SRC_DIR) -name '*.c') $(INC_DIR) $(TEST_DIR)

.PHONY: doc
doc: ## Generate documentation using Doxygen
	@echo "Generating documentation..."
	@test -f Doxyfile || { echo "Error: Doxyfile not found."; exit 1; }
	doxygen Doxyfile

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
## Cleaning
####################################################################################################
.PHONY: clean
clean: python-clean zig-clean ## Remove all build artifacts and temporary files
	@echo "Cleaning up C build artifacts..."
	rm -rf $(BIN_DIR) $(TARGET_DIR) $(LIB_DIR) $(COV_DIR)
	@echo "Searching for and removing intermediate and output files (excluding $(CLEAN_EXCLUDE_DIRS))..."
	$(eval EXCLUDE_PRUNE_SEQUENCE := $(foreach dir,$(CLEAN_EXCLUDE_DIRS),-path ./$(dir) -prune -o ))
	$(eval DELETE_COMMAND := \( -name '*.gcda' -o -name '*.gcno' -o -name '*.gcov' -o -name '*.d' \
	                            -o -name '*.o' -o -name '*.a' -o -name '*.so' \
	                         \) -exec rm -f {} +)
	find . $(EXCLUDE_PRUNE_SEQUENCE) $(DELETE_COMMAND)
	@echo "Cleaning Doxygen artifacts..."
	rm -rf Doxyfile.bak $(DOC_DIR)/html $(DOC_DIR)/latex

# Include dependency files
-include $(DEP_FILES)
