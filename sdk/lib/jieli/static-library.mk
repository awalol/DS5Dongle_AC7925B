# Convenience entry point; CMake owns source lists and the LTO archive rules.
CMAKE ?= cmake
BUILD_DIR ?= build
JOBS ?= 4
CMAKE_ARGS ?=

.PHONY: all configure clean install-sdk
all: configure
	+$(CMAKE) --build "$(BUILD_DIR)" --parallel $(JOBS)

configure:
	$(CMAKE) -S . -B "$(BUILD_DIR)" $(CMAKE_ARGS)

# Only clean this CMake build's outputs; keep downloaded upstream sources.
clean:
	@if test -f "$(BUILD_DIR)/CMakeCache.txt"; then \
		$(CMAKE) --build "$(BUILD_DIR)" --target clean; \
	fi

install-sdk: all
	+$(CMAKE) --build "$(BUILD_DIR)" --target install-sdk
