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

####################################################################################################
## Force CPU backend and iteration lists
####################################################################################################
# pick one: AUTO, SCALAR, AVX, AVX2, AVX512, NEON, SVE
HSD_TARGET ?= AUTO

# Available backends for each architecture
AMD64_TARGETS   := AUTO SCALAR AVX AVX2 AVX512
AARCH64_TARGETS := AUTO SCALAR NEON SVE

####################################################################################################
## Map HSD_TARGET to defines and ISA flags
####################################################################################################
ifeq ($(HSD_TARGET),SCALAR)
  TARGET_DEFS := -DHSD_TARGET_SCALAR
  TARGET_ISA  :=
else ifeq ($(HSD_TARGET),AVX)
  TARGET_DEFS := -DHSD_TARGET_AVX -D__AVX__
  TARGET_ISA  := -mavx
else ifeq ($(HSD_TARGET),AVX2)
  TARGET_DEFS := -DHSD_TARGET_AVX2 -D__AVX2__
  TARGET_ISA  := -mavx2
else ifeq ($(HSD_TARGET),AVX512)
  TARGET_DEFS := -DHSD_TARGET_AVX512VPOPCNTDQ -D__AVX512F__ -D__AVX512BW__ -D__AVX512DQ__ -D__AVX512VPOPCNTDQ__
  TARGET_ISA  := -mavx512f -mavx512bw -mavx512dq -mavx512vpopcntdq
else ifeq ($(HSD_TARGET),NEON)
  TARGET_DEFS := -DHSD_TARGET_NEON -D__ARM_NEON
  TARGET_ISA  := -mfpu=neon
else ifeq ($(HSD_TARGET),SVE)
  TARGET_DEFS := -DHSD_TARGET_SVE -D__ARM_FEATURE_SVE
  TARGET_ISA  := -march=armv8.2-a+sve
else
  TARGET_DEFS :=
  TARGET_ISA  :=
endif

####################################################################################################
## Platform Detection
####################################################################################################
OS := $(shell uname -s)

ifeq ($(OS),Linux)
    SHARED_LIB_EXT     := so
    SHARED_LIB_PREFIX  := lib
    STATIC_LIB_EXT     := a
    EXE_EXT            :=
endif
ifeq ($(OS),Darwin)
    SHARED_LIB_EXT     := dylib
    SHARED_LIB_PREFIX  := lib
    STATIC_LIB_EXT     := a
    EXE_EXT            :=
endif

SHARED_LIB_FILENAME ?= $(SHARED_LIB_PREFIX)hsd.$(SHARED_LIB_EXT)
STATIC_LIB_FILENAME ?= $(SHARED_LIB_PREFIX)hsd.$(STATIC_LIB_EXT)
EXE_FILENAME        ?= test_runner$(EXE_EXT)

SHARED_LIB           := $(LIB_DIR)/$(SHARED_LIB_FILENAME)
STATIC_LIB           := $(LIB_DIR)/$(STATIC_LIB_FILENAME)
TEST_RUNNER          := $(BIN_DIR)/$(EXE_FILENAME)

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
CFLAGS_COMMON := -Wall -Wextra -pedantic -fPIC -Iinclude $(TARGET_DEFS) $(TARGET_ISA)

ifeq ($(HSD_TARGET),AUTO)
  CFLAGS_COMMON += -march=native
endif

LIB_CFLAGS := $(CFLAGS_COMMON) -std=c99
TEST_CFLAGS := $(CFLAGS_COMMON) -std=gnu2x -I$(TEST_DIR)

ifeq ($(BUILD_TYPE),release)
  LIB_CFLAGS += -O2 # or -O3
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

ZIG_CMD     := $(ZIG) build $(ZIG_BUILD_FLAGS)
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
$(BIN_DIR) $(LIB_DIR) $(DOC_DIR) $(TARGET_DIR) $(LIB_OBJ_DIRS):
	@echo "Creating directory $@..."
	@mkdir -p $@

####################################################################################################
## Default Target
####################################################################################################
.DEFAULT_GOAL := help

.PHONY: help
help: ## Show help message for each target
	@echo "HSDLib Makefile Help"
	@echo "===================="
	@grep -h -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) \
	| awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'

####################################################################################################
## Primary Build Targets
####################################################################################################
.PHONY: all
all: static shared ## Build static and shared libraries
	@echo "Build complete: static and shared libraries ready"

.PHONY: rebuild
rebuild: clean all ## Clean and rebuild libraries
	@echo "Rebuild complete"

####################################################################################################
## Library Build Rules
####################################################################################################
.PHONY: static
static: $(STATIC_LIB) ## Build static library
	@echo "Static library built successfully: $(STATIC_LIB)"

$(STATIC_LIB): $(LIB_OBJ_FILES) | $(LIB_DIR)
	@echo "Creating static library $(STATIC_LIB)..."
	@$(AR) rcs $@ $^

.PHONY: shared
shared: $(SHARED_LIB) ## Build shared library
	@echo "Shared library built successfully: $(SHARED_LIB)"

$(SHARED_LIB): $(LIB_OBJ_FILES) | $(LIB_DIR)
	@echo "Creating shared library $(SHARED_LIB)..."
	@$(CC) $(LIB_CFLAGS) -shared -o $@ $^ $(LDFLAGS) $(LIBS)

####################################################################################################
## Object Compilation Rules
####################################################################################################
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
test: $(TEST_RUNNER) ## Run C tests
	@echo "Running tests..."
	@./$(TEST_RUNNER)
	@echo "Tests completed"

$(TEST_RUNNER): $(TEST_OBJ_FILES) $(LIB_OBJ_FILES) | $(BIN_DIR)
	@echo "Linking test runner: $@"
	@$(CC) $(TEST_CFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS)

####################################################################################################
## Sweep Tests by Backend
####################################################################################################
.PHONY: test-all
test-all: ## Run tests for all backends (AMD64, AArch64)
	@echo "=== Running tests for all backends ==="
	@echo "=== Running tests for AMD64 ==="
	@$(MAKE) test-amd64
	@echo "=== Running tests for AArch64 ==="
	@$(MAKE) test-aarch64
	@echo "=== All backend tests completed ==="

.PHONY: test-amd64
test-amd64: ## Run tests for AMD64 backends (AUTO, SCALAR, AVX, AVX2, AVX512)
	@echo "=== Testing AMD64 (x86_64) backends ==="
	@for t in $(AMD64_TARGETS); do \
	  echo; \
	  echo ">>> Testing $$t backend"; \
	  $(MAKE) clean HSD_TARGET=$$t all test; \
	done
	@echo "=== AMD64 backend tests completed ==="

.PHONY: test-aarch64
test-aarch64: ## Run tests for AArch64 backends (AUTO, SCALAR, NEON, SVE)
	@echo "=== Testing AArch64 backends ==="
	@for t in $(AARCH64_TARGETS); do \
	  echo; \
	  echo ">>> Testing $$t backend"; \
	  $(MAKE) clean HSD_TARGET=$$t all test; \
	done
	@echo "=== AArch64 backend tests completed ==="

####################################################################################################
## Code Coverage
####################################################################################################
.PHONY: coverage-clean
coverage-clean: ## Clean object files for coverage build
	@echo "Cleaning object files for coverage build..."
	@rm -f $(TARGET_DIR)/*.o $(TARGET_DIR)/*/*.o

coverage: TEST_CFLAGS += -fprofile-arcs -ftest-coverage
coverage: LIBS += -lgcov
.PHONY: coverage
coverage: coverage-clean $(TEST_RUNNER) ## Generate coverage report
	@echo "Running tests for coverage..."
	@./$(TEST_RUNNER)
	@echo "Generating gcov files..."
	@for obj_file in $(LIB_OBJ_FILES); do \
	  obj_dir=$$(dirname $$obj_file); \
	  src_file=$$(echo $$obj_file | sed 's|$(TARGET_DIR)|$(SRC_DIR)|; s|\.o$$|.c|'); \
	  echo "Processing $$src_file"; \
	  (cd $$obj_dir && gcov --preserve-paths --object-directory . $$src_file); \
	done
	@find $(TARGET_DIR) -name "*.gcov" -exec mv {} . \;
	@find . -maxdepth 1 -name '#usr#*.gcov' -delete -o -name '#opt#*.gcov' -delete
	@echo "Coverage analysis complete"

####################################################################################################
## Zig Build Targets
####################################################################################################
.PHONY: zig-shared
zig-shared: | $(LIB_DIR) ## Build shared library with Zig
	@echo "Building shared lib via Zig..."
	@$(ZIG_CMD)
	@cp "$(ZIG_OUT_LIB)/$(SHARED_LIB_FILENAME)" "$(SHARED_LIB)"
	@echo "Zig shared library build complete: $(SHARED_LIB)"

.PHONY: zig-static
zig-static: | $(LIB_DIR) ## Build static library with Zig
	@echo "Building static lib via Zig..."
	@$(ZIG_CMD)
	@cp "$(ZIG_OUT_LIB)/$(STATIC_LIB_FILENAME)" "$(STATIC_LIB)"
	@echo "Zig static library build complete: $(STATIC_LIB)"

.PHONY: zig-lib
zig-lib: zig-shared zig-static ## Build both via Zig
	@echo "Zig library builds complete"

.PHONY: zig-test-c
zig-test-c: | $(BIN_DIR) ## Run C tests via Zig
	@echo "Building tests via Zig..."
	@$(ZIG_CMD)
	@cp "$(ZIG_OUT_BIN)/test_runner" "$(TEST_RUNNER)"
	@echo "Running Zig-built tests..."
	@./$(TEST_RUNNER)
	@echo "Zig test execution complete"

.PHONY: zig-clean
zig-clean: ## Remove Zig build artifacts
	@echo "Cleaning Zig build artifacts..."
	@rm -rf zig-cache .zig-cache zig-out
	@echo "Zig clean complete"

####################################################################################################
## Python Targets
####################################################################################################
.PHONY: python-build
python-build: ## Build Python wheel
	@echo "Building Python wheel..."
	@echo "Copying shared lib into Python package..."
	@mkdir -p python/hsdpy
	@cp "$(LIB_DIR)/$(SHARED_LIB_FILENAME)" python/hsdpy/
	@python -m build --wheel --outdir $(PYTHON_DIST_DIR)
	@echo "Python wheel build complete"

.PHONY: python-install
python-install: python-build ## Install wheel
	@echo "Installing Python wheel..."
	$(eval WHEEL_FILE := $(shell find $(PYTHON_DIST_DIR) -type f -name '*.whl' | head -n 1))
	@if [ -z "$(WHEEL_FILE)" ]; then \
	  echo "ERROR: No wheel found"; exit 1; \
	fi
	@uv pip install --force-reinstall --no-deps "$(WHEEL_FILE)"
	@echo "Python wheel installed successfully"

.PHONY: python-test
python-test: zig-shared ## Run Python tests
	@echo "Running Python tests..."
	@uv run pytest python/tests --tb=short --disable-warnings --cov=python/hsdpy --cov-branch --cov-report=xml
	@echo "Python tests complete"

.PHONY: python-clean
python-clean: ## Clean Python build artifacts
	@echo "Cleaning Python build artifacts..."
	@rm -rf $(PYTHON_DIST_DIR) $(PYTHON_BUILD_DIR) $(PYTHON_EGG_INFO) site_packages*
	@find python -type d -name '__pycache__' -exec rm -rf {} +
	@find python/hsdpy -maxdepth 1 \( -name '*.so' -o -name '*.dylib' -o -name '*.dll' \) -delete
	@rm '.coverage' 'coverage.xml'
	@echo "Python clean complete"

####################################################################################################
## Development Tools
####################################################################################################
.PHONY: install-deps
install-deps: ## Install system & Python deps (for Debian-based systems)
	@echo "Installing system dependencies..."
	@sudo apt-get update && sudo apt-get install -y gcc clang llvm gdb clang-format cppcheck graphviz doxygen python3-pip snapd
	@echo "Installing Python dependencies..."
	@pip3 install -U uv
	@echo "Installing Zig..."
	@sudo snap install zig --classic --beta
	@echo "All dependencies installed"

.PHONY: format
format: ## Format C code
	@echo "Formatting C code..."
	@find $(SRC_DIR) $(INC_DIR) $(TEST_DIR) -type f \( -name '*.c' -o -name '*.h' \) -exec clang-format -i {} +
	@echo "Code formatting complete"

.PHONY: lint
lint: ## Lint C code
	@echo "Linting C code..."
	@cppcheck --enable=all --inconclusive --quiet --force --std=c11 -I$(INC_DIR) -I$(TEST_DIR) \
	  --suppress=missingIncludeSystem --suppress=unusedFunction --suppress=constVariable \
	  $(shell find $(SRC_DIR) -name '*.c') $(INC_DIR) $(TEST_DIR)
	@echo "Linting complete"

.PHONY: doc
doc: ## Generate docs via Doxygen
	@echo "Generating documentation..."
	@test -f Doxyfile || { echo "Error: Doxyfile not found."; exit 1; }
	@doxygen Doxyfile
	@echo "Documentation generated in $(DOC_DIR)/html"

####################################################################################################
## Installation Targets
####################################################################################################
.PHONY: install
install: static shared ## Install to /usr/local
	@echo "Installing libraries and headers to /usr/local..."
	@install -d /usr/local/include /usr/local/lib
	@install -m 0644 $(INC_DIR)/*.h /usr/local/include/
	@install -m 0644 $(STATIC_LIB) /usr/local/lib/
	@install -m 0755 $(SHARED_LIB) /usr/local/lib/
	@ldconfig || true
	@echo "Installation complete"

.PHONY: uninstall
uninstall: ## Remove from /usr/local
	@echo "Uninstalling from /usr/local..."
	@for header in $(shell ls $(INC_DIR)/*.h); do rm -f /usr/local/include/$$(basename $$header); done
	@rm -f /usr/local/lib/libhsd.a /usr/local/lib/libhsd.so
	@ldconfig || true
	@echo "Uninstallation complete"

####################################################################################################
## Cleaning
####################################################################################################
.PHONY: clean
clean: python-clean zig-clean ## Remove all build artifacts
	@echo "Cleaning build artifacts..."
	@rm -rf $(BIN_DIR) $(TARGET_DIR) $(LIB_DIR)
	$(eval EXCLUDE_PRUNE := $(foreach d,$(CLEAN_EXCLUDE_DIRS),-path ./$(d) -prune -o ))
	@find . $(EXCLUDE_PRUNE) \( -name '*.gcda' -o -name '*.gcno' -o -name '*.gcov' -o -name '*.d' -o -name '*.o' -o -name '*.a' -o -name '*.so' \) -exec rm -f {} +
	@rm -rf Doxyfile.bak $(DOC_DIR)/html $(DOC_DIR)/latex
	@echo "Clean complete"

# Include dependency files
-include $(DEP_FILES)
