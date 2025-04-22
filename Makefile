####################################################################################################
## Default Target
####################################################################################################
.DEFAULT_GOAL := help

.PHONY: help
help: ## Show this help message with all available targets and their descriptions
	@echo "Hsdlib Makefile Help"
	@echo "===================="
	@grep -h -E '^[[:space:]]*[a-zA-Z0-9_-]+[[:space:]]*:[^=]*##' $(MAKEFILE_LIST) \
	| sed 's/^[[:space:]]*//g' \
	| sort \
	| awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'

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
CLEAN_EXCLUDE_DIRS := env .env venv .venv
BINDINGS_DIR := bindings

####################################################################################################
## Platform Detection
####################################################################################################
ARCH := $(shell uname -m)
OS := $(shell uname -s)

ifeq ($(OS),Linux)
    SHARED_LIB_EXT     := so
    SHARED_LIB_PREFIX  := lib
    STATIC_LIB_EXT     := a
    EXE_EXT            :=
    IS_DEBIAN := $(shell command -v apt-get >/dev/null 2>&1 && echo 1 || echo 0)
endif
ifeq ($(OS),Darwin)
    SHARED_LIB_EXT     := dylib
    SHARED_LIB_PREFIX  := lib
    STATIC_LIB_EXT     := a
    EXE_EXT            :=
endif
ifneq (,$(filter Windows_NT MINGW%,$(OS)))
    SHARED_LIB_EXT     := dll
    SHARED_LIB_PREFIX  :=
    STATIC_LIB_EXT     := lib
    EXE_EXT            := .exe
endif

SHARED_LIB_FILENAME ?= $(SHARED_LIB_PREFIX)hsd.$(SHARED_LIB_EXT)
STATIC_LIB_FILENAME ?= $(SHARED_LIB_PREFIX)hsd.$(STATIC_LIB_EXT)
EXE_FILENAME        ?= test_runner$(EXE_EXT)

SHARED_LIB           := $(LIB_DIR)/$(SHARED_LIB_FILENAME)
STATIC_LIB           := $(LIB_DIR)/$(STATIC_LIB_FILENAME)
TEST_RUNNER          := $(BIN_DIR)/$(EXE_FILENAME)

####################################################################################################
## Backend Configuration for Testing
####################################################################################################
# HSD_TEST_FORCE_BACKEND - Set this environment variable to force a specific backend
# Examples: HSD_TEST_FORCE_BACKEND=AVX2 ./bin/test_runner

AMD64_TARGETS   := AUTO SCALAR AVX AVX2 AVX512F AVX512BW AVX512DQ AVX512VPOPCNTDQ
AARCH64_TARGETS := AUTO SCALAR NEON SVE

####################################################################################################
## Build Configuration
####################################################################################################
CC   := $(shell command -v gcc 2>/dev/null || command -v clang 2>/dev/null)
AR   := ar

SHELL := /bin/bash
.SHELLFLAGS := -e -o pipefail -c

BUILD_TYPE ?= debug

####################################################################################################
## Compiler Flags
####################################################################################################
CFLAGS_COMMON := -Wall -Wextra -pedantic -fPIC -Iinclude -march=native

# Conditional flags based on architecture (AMD64 or AArch64)
ifeq ($(ARCH),x86_64)
CFLAGS_COMMON += -march=native -mfma
else
CFLAGS_COMMON += -march=native
endif

LIB_CFLAGS := $(CFLAGS_COMMON) -std=c11
TEST_CFLAGS := $(CFLAGS_COMMON) -std=gnu2x -I$(TEST_DIR)

ifeq ($(BUILD_TYPE),release)
  LIB_CFLAGS += -O2 # or -O3 (O2 is usually sufficient and more stable)
  TEST_CFLAGS += -O2 # or -O3
else
  LIB_CFLAGS += -g -O0 -DHSD_DEBUG
  TEST_CFLAGS += -g -O0 -DHSD_DEBUG
endif

LDFLAGS :=
LIBS := -lm
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
## Directory Creation Rules
####################################################################################################
$(BIN_DIR) $(LIB_DIR) $(DOC_DIR) $(TARGET_DIR) $(LIB_OBJ_DIRS):
	@echo "Creating directory $@..."
	@mkdir -p $@

####################################################################################################
## Primary Build Targets
####################################################################################################
.PHONY: build
build: $(STATIC_LIB) $(SHARED_LIB) ## Build both static and shared libraries (C compilation method)
	@echo "Build parameters:"
	@echo "  - Build type: $(BUILD_TYPE)"
	@echo "  - Compiler: $(CC)"
	@echo "  - Static library: $(STATIC_LIB)"
	@echo "  - Shared library: $(SHARED_LIB)"
	@echo "  - OS: $(OS)"
	@echo "  - Architecture: $(ARCH)"
	@echo "Build complete: static and shared libraries ready"

.PHONY: rebuild
rebuild: clean build ## Perform a complete rebuild: clean all artifacts and rebuild from scratch
	@echo "Rebuild complete"

.PHONY: build-release
build-release: ## Build the library in release mode (optimized; without debug symbols)
	@echo "Building library in release mode for benchmarks..."
	@$(MAKE) rebuild BUILD_TYPE=release

####################################################################################################
## Library Build Rules
####################################################################################################
$(STATIC_LIB): $(LIB_OBJ_FILES) | $(LIB_DIR)
	@echo "Creating static library $(STATIC_LIB)..."
	@$(AR) rcs $@ $^

$(SHARED_LIB): $(LIB_OBJ_FILES) | $(LIB_DIR)
	@echo "Creating shared library $(SHARED_LIB)..."
	@$(CC) $(LIB_CFLAGS) -shared -o $@ $^ $(LDFLAGS) $(LIBS)

$(LIB_OBJ_FILES): $(TARGET_DIR)/%.o: $(SRC_DIR)/%.c | $(TARGET_DIR) $(INC_DIR)/hsdlib.h $(LIB_OBJ_DIRS)
	@echo "Compiling $<"
	@$(CC) $(LIB_CFLAGS) $(COMP_FLAGS) -c $< -o $@

$(TEST_OBJ_FILES): $(TARGET_DIR)/%.o: $(TEST_DIR)/%.c | $(TARGET_DIR) $(INC_DIR)/hsdlib.h $(wildcard $(TEST_DIR)/test_common.h)
	@echo "Compiling $<"
	@$(CC) $(TEST_CFLAGS) $(COMP_FLAGS) -c $< -o $@

####################################################################################################
## Testing Targets
####################################################################################################
.PHONY: test
test: $(TEST_RUNNER) ## Build and run C test suite (uses AUTO backend by default)
	@echo "Running tests (Backend: AUTO or as set by HSD_TEST_FORCE_BACKEND)..."
	@./$(TEST_RUNNER)
	@echo "Tests completed"

$(TEST_RUNNER): $(TEST_OBJ_FILES) $(LIB_OBJ_FILES) | $(BIN_DIR)
	@echo "Linking test runner: $@"
	@$(CC) $(TEST_CFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS)

####################################################################################################
## Sweep Tests by Backend (Tests Manual Override)
####################################################################################################
.PHONY: test-amd64
test-amd64: $(TEST_RUNNER) ## Test all AMD64 backends (AUTO, SCALAR, AVX, AVX2, AVX512*) via ENV var
	@echo "=== Testing AMD64 (x86_64) backends via Manual Override ==="
	@# Check if this machine *is* x86_64 before running AMD64 tests
ifeq ($(shell uname -m),x86_64)
	@for t in $(AMD64_TARGETS); do \
	  echo; \
	  echo ">>> Forcing $$t backend via ENV and running tests"; \
	  HSD_TEST_FORCE_BACKEND=$$t ./$(TEST_RUNNER); \
	done
else
	@echo "Skipping AMD64 tests - not on an AMD64 machine."
endif
	@echo "=== AMD64 backend tests completed ==="

.PHONY: test-aarch64
test-aarch64: $(TEST_RUNNER) ## Test all AArch64 backends (AUTO, SCALAR, NEON, SVE) via ENV var
	@echo "=== Testing AArch64 backends via Manual Override ==="
	@# Check if this machine *is* aarch64 before running ARM tests
ifeq ($(shell uname -m),aarch64)
	@for t in $(AARCH64_TARGETS); do \
	  echo; \
	  echo ">>> Forcing $$t backend via ENV and running tests"; \
	  HSD_TEST_FORCE_BACKEND=$$t ./$(TEST_RUNNER); \
	done
else
	@echo "Skipping AArch64 tests - not on an aarch64 machine."
endif
	@echo "=== AArch64 backend tests completed ==="

####################################################################################################
## Code Coverage
####################################################################################################
.PHONY: cov
cov: LIB_CFLAGS += -fprofile-arcs -ftest-coverage
cov: TEST_CFLAGS += -fprofile-arcs -ftest-coverage
cov: LIBS += -lgcov
cov: clean $(TEST_RUNNER) ## Generate code coverage report for C tests
	@echo "Running tests for coverage..."
	@./$(TEST_RUNNER)
	@echo "Generating gcov files..."
	@mkdir -p $(DOC_DIR)/coverage
	@for obj_file in $(LIB_OBJ_FILES); do \
		obj_dir=$$(dirname $$obj_file); \
		src_file=$$(echo $$obj_file | sed 's|$(TARGET_DIR)|$(SRC_DIR)|; s|\.o$$|.c|'); \
		echo "Processing $$src_file"; \
		gcov -r -o $$obj_dir $$src_file; \
	done
	@find . -name "*.gcov" -exec mv {} $(DOC_DIR)/coverage/ \; 2>/dev/null || true
	@echo "Coverage analysis complete. Reports in $(DOC_DIR)/coverage/"

####################################################################################################
## Python Configuration
####################################################################################################
PYTHON_DIST_DIR := dist
PYTHON_BUILD_DIR := build
PYTHON_EGG_INFO := $(shell find . -maxdepth 3 -type d -name '*.egg-info') UNKNOWN.egg-info

####################################################################################################
## Python Targets
####################################################################################################
.PHONY: python-setup
python-setup: ## Set up Python environment for building and testing
	@echo "Setting up Python environment..."
	@command -v python3 >/dev/null 2>&1 || { echo "Error: 'python3' command not found. Please install Python 3."; exit 1; }
	@command -v pip3 >/dev/null 2>&1 || { echo "Error: 'pip3' command not found. Please install pip for Python 3."; exit 1; }
	@pip3 install -U uv
	@uv sync
	@uv pip install -e ".[dev]"
	@echo "Python environment setup complete"

.PHONY: python-build
python-build: rebuild ## Build Python wheel package for distribution
	@echo "Using shared lib: $(SHARED_LIB)"
	@echo "Copying shared lib into Python package..."
	@mkdir -p $(BINDINGS_DIR)/python/hsdpy
	@cp "$(LIB_DIR)/$(SHARED_LIB_FILENAME)" $(BINDINGS_DIR)/python/hsdpy/
	@uv build --wheel --out-dir $(PYTHON_DIST_DIR)
	@echo "Python wheel build complete"

.PHONY: python-install
python-install: ## Install the Python wheel package locally
	@echo "Installing Python wheel..."
	@command -v uv >/dev/null 2>&1 || { echo "Error: 'uv' command not found. Please install with 'pip install -U uv'."; exit 1; }
	$(eval WHEEL_FILE := $(shell find $(PYTHON_DIST_DIR) -type f -name '*.whl' | head -n 1))
	@if [ -z "$(WHEEL_FILE)" ]; then \
	  echo "ERROR: No wheel found"; exit 1; \
	fi
	@uv pip install --force-reinstall "$(WHEEL_FILE)"
	@echo "Python wheel installed successfully"

.PHONY: python-test
python-test: ## Run Python test suite (with code coverage)
	@echo "Running Python tests..."
	@command -v uv >/dev/null 2>&1 || { echo "Error: 'uv' command not found. Please install with 'pip install -U uv'."; exit 1; }
	@uv run pytest $(BINDINGS_DIR)/python/tests --tb=short --disable-warnings --cov=$(BINDINGS_DIR)/python/hsdpy --cov-branch --cov-report=xml
	@echo "Python tests complete"

.PHONY: python-clean
python-clean: ## Clean Python-specific build artifacts and caches
	@echo "Cleaning Python build artifacts..."
	@rm -rf $(PYTHON_DIST_DIR) $(PYTHON_BUILD_DIR) $(PYTHON_EGG_INFO) .pytest_cache
	@find . -type d -name '__pycache__' -exec rm -rf {} + 2>/dev/null || true
	@find $(BINDINGS_DIR)/python/hsdpy -maxdepth 1 \( -name '*.so' -o -name '*.dylib' -o -name '*.dll' \) -delete 2>/dev/null || true
	@rm -f '.coverage' 'coverage.xml'
	@echo "Python clean complete"

.PHONY: python-publish
python-publish: ## Publish wheel package to PyPI (needs PYPI_TOKEN environment variable)
	@echo "Publishing Python wheel to PyPI..."
	@if [ -z "$$PYPI_TOKEN" ]; then \
		echo "ERROR: PYPI_TOKEN environment variable is not set"; \
		echo "Please set it with: export PYPI_TOKEN=your_token"; \
		exit 1; \
	fi
	$(eval WHEEL_FILE := $(shell find $(PYTHON_DIST_DIR) -type f -name '*.whl' | head -n 1))
	@if [ -z "$(WHEEL_FILE)" ]; then \
		echo "ERROR: No wheel found in $(PYTHON_DIST_DIR)"; \
		exit 1; \
	fi
	@echo "Found wheel: $(WHEEL_FILE)"
	@uv publish --token "$$PYPI_TOKEN" "$(WHEEL_FILE)"
	@echo "Package published to PyPI successfully"

####################################################################################################
## Development Tools
####################################################################################################
.PHONY: install-deps
install-deps: ## Install all required dependencies for development
	@echo "Installing system dependencies..."
ifeq ($(OS),Linux)
ifeq ($(IS_DEBIAN),1)
	@echo "Detected Debian-based system, using apt-get..."
	@sudo apt-get update
	@sudo apt-get install -y gcc clang llvm gdb clang-format cppcheck graphviz doxygen python3-pip
else
	@echo "Non-Debian Linux detected. Please install the following packages manually:"
	@echo "- gcc, clang, llvm, gdb, clang-format, cppcheck, graphviz, doxygen, python3-pip"
endif
else ifeq ($(OS),Darwin)
	@echo "macOS detected, install using brew (assumes brew is installed):"
	@echo "brew install gcc llvm gdb clang-format cppcheck graphviz doxygen python3"
else
	@echo "Unsupported OS for automatic dependency installation"
	@echo "Please install required tools manually"
endif

.PHONY: format
format: ## Format all C source and header files
	@echo "Formatting C code..."
	@find $(SRC_DIR) $(INC_DIR) $(TEST_DIR) -type f \( -name '*.c' -o -name '*.h' \) -exec clang-format -i {} +
	@echo "Code formatting complete"

.PHONY: lint
lint: ## Run static analysis on C code
	@echo "Linting C code..."
	@cppcheck --enable=all --inconclusive --quiet --force --std=c11 -I$(INC_DIR) -I$(TEST_DIR) \
	  --suppress=missingIncludeSystem --suppress=unusedFunction --suppress=constVariable \
	  $(shell find $(SRC_DIR) -name '*.c') $(INC_DIR) $(TEST_DIR)
	@echo "Linting complete"

.PHONY: doc
doc: ## Generate documentation for the library using Doxygen
	@echo "Generating documentation..."
	@test -f Doxyfile || { echo "Error: Doxyfile not found."; exit 1; }
	@doxygen Doxyfile
	@echo "Documentation generated in $(DOC_DIR)/html"

####################################################################################################
## Example Target
####################################################################################################
EXAMPLE_DIR := examples
EXAMPLE_SRC := $(EXAMPLE_DIR)/hsdlib_example.c
EXAMPLE_BIN := $(BIN_DIR)/hsdlib_example

.PHONY: example
example: ## Build and run the examples (for C and Python)
	@echo "Building example in release mode..."
	@$(MAKE) $(EXAMPLE_BIN) BUILD_TYPE=release
	@echo "Running Hsdlib examples..."
	@$(EXAMPLE_BIN)
	@echo "Running HsdPy examples..."
	@$(MAKE) python-build BUILD_TYPE=release
	@$(MAKE) python-install > /dev/null
	@uv run python3 $(EXAMPLE_DIR)/hsdpy_example.py
	@echo "Example execution complete"

$(EXAMPLE_BIN): $(EXAMPLE_SRC) $(STATIC_LIB) | $(BIN_DIR)
	@echo "Building example: $@"
	@$(CC) $(LIB_CFLAGS) -o $@ $< $(STATIC_LIB) $(LDFLAGS) $(LIBS) -std=gnu2x

####################################################################################################
## Benchmarks: build into bin/ with a single binary per benchmark
####################################################################################################
BENCH_DIR    := benches/c
BENCH_SRCS   := $(wildcard $(BENCH_DIR)/bench_*.c)

# Generate single binary names for each benchmark
BENCH_BINS   := $(foreach src,$(BENCH_SRCS),$(BIN_DIR)/$(basename $(notdir $(src))))

# Benchmark-specific flags: optimize and disable debugging
BENCH_CFLAGS := $(filter-out -g -O0 -DHSD_DEBUG,$(CFLAGS_COMMON)) -O3 -DNDEBUG

# Build a single binary for each benchmark
$(BIN_DIR)/bench_%: $(BENCH_DIR)/bench_%.c build-release | $(BIN_DIR)
	@echo "Building $@"
	@$(CC) $(BENCH_CFLAGS) \
		-D_POSIX_C_SOURCE=200809L \
		-std=c11 -Iinclude -I$(BENCH_DIR) \
		-o $@ $< $(STATIC_LIB) -lm

.PHONY: bench bench-amd64 bench-aarch64 bench-clean

bench: ## Run benchmarks for detected CPU architecture
	@case "$(ARCH)" in \
		x86_64)  $(MAKE) bench-amd64 ;; \
		aarch64) $(MAKE) bench-aarch64 ;; \
		*) echo "Unsupported architecture: $(ARCH). Supported: AMD64, AArch64" ;; \
	esac

bench-amd64: $(BENCH_BINS) ## Run AMD64 backend benchmarks
	@echo
	@if [ "$(ARCH)" = "x86_64" ]; then \
		echo "=== AMD64 Benchmarks (Total time in sec) ==="; \
		printf "| Backend "; \
		for b in $(sort $(foreach src,$(BENCH_SRCS),$(basename $(notdir $(src))))); do \
			printf "| %s " $$b; \
		done; \
		printf "|\n"; \
		printf "|---"; \
		for b in $(sort $(foreach src,$(BENCH_SRCS),$(basename $(notdir $(src))))); do \
			printf "|---"; \
		done; \
		printf "|\n"; \
		for t in $(AMD64_TARGETS); do \
			printf "| %s " $$t; \
			for b in $(sort $(foreach src,$(BENCH_SRCS),$(basename $(notdir $(src))))); do \
				tt=$$(HSD_BENCH_FORCE_BACKEND=$$t $(BIN_DIR)/$$b 2>/dev/null | awk '/Total time:/ {print $$3}'); \
				printf "| %s " $${tt:-N/A}; \
			done; \
			printf "|\n"; \
		done \
	else \
		echo "Skipping AMD64 benchmarks: not on AMD64"; \
	fi

bench-aarch64: $(BENCH_BINS) ## Run AArch64 backend benchmarks
	@echo
	@if [ "$(ARCH)" = "aarch64" ]; then \
		echo "=== AArch64 Benchmarks (Total time in sec) ==="; \
		printf "| Backend "; \
		for b in $(sort $(foreach src,$(BENCH_SRCS),$(basename $(notdir $(src))))); do \
			printf "| %s " $$b; \
		done; \
		printf "|\n"; \
		printf "|---"; \
		for b in $(sort $(foreach src,$(BENCH_SRCS),$(basename $(notdir $(src))))); do \
			printf "|---"; \
		done; \
		printf "|\n"; \
		for t in $(AARCH64_TARGETS); do \
			printf "| %s " $$t; \
			for b in $(sort $(foreach src,$(BENCH_SRCS),$(basename $(notdir $(src))))); do \
				tt=$$(HSD_BENCH_FORCE_BACKEND=$$t $(BIN_DIR)/$$b 2>/dev/null | awk '/Total time:/ {print $$3}'); \
				printf "| %s " $${tt:-N/A}; \
			done; \
			printf "|\n"; \
		done \
	else \
		echo "Skipping AArch64 benchmarks: not on AArch64"; \
	fi

bench-clean: ## Remove benchmark binaries
	@echo "Cleaning benchmark binaries..."
	@rm -f $(BENCH_BINS)

####################################################################################################
## Cleaning
####################################################################################################
.PHONY: clean
clean: python-clean ## Remove all build artifacts and temporary files
	@echo "Cleaning build artifacts..."
	@rm -rf $(BIN_DIR) $(TARGET_DIR) $(LIB_DIR)
	$(eval EXCLUDE_PRUNE := $(foreach d,$(CLEAN_EXCLUDE_DIRS),-path ./$(d) -prune -o ))
	@find . $(EXCLUDE_PRUNE) \( -name '*.gcda' -o -name '*.gcno' -o -name '*.gcov' -o -name '*.d' -o -name '*.o' -o -name '*.a' -o -name '*.so' \) -exec rm -f {} + 2>/dev/null || true
	@rm -rf Doxyfile.bak $(DOC_DIR)/html $(DOC_DIR)/latex
	@echo "Clean complete"

.PHONY: all
all: rebuild test ## Build and run tests

# Include dependency files
-include $(DEP_FILES)
