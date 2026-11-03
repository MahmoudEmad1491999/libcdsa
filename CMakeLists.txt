cmake_minimum_required(VERSION 3.20)
project(cdsa VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# ── Options ─────────────────────────────────────────────────────────────────
option(CDSA_SANITIZE "Build with AddressSanitizer + UndefinedBehaviorSanitizer" OFF)
option(CDSA_BUILD_TESTS "Build unit tests" ON)
option(CDSA_BUILD_BENCH "Build micro-benchmarks" OFF)
option(CDSA_BUILD_EXAMPLES "Build examples" OFF)

# ── Compiler warnings ────────────────────────────────────────────────────────
add_library(cdsa_warnings INTERFACE)
target_compile_options(cdsa_warnings INTERFACE
    -Wall -Wextra -Wpedantic -Werror
    -Wconversion -Wshadow
    -Wstrict-prototypes -Wmissing-prototypes
)

# ── Sanitizers ───────────────────────────────────────────────────────────────
add_library(cdsa_sanitize INTERFACE)
if(CDSA_SANITIZE)
    target_compile_options(cdsa_sanitize INTERFACE
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
        -fno-sanitize-recover=all
    )
    target_link_options(cdsa_sanitize INTERFACE
        -fsanitize=address,undefined
    )
endif()

# ── Library target ───────────────────────────────────────────────────────────
# Populated by later phases; empty for now so the build graph is valid.
add_library(cdsa STATIC)
target_sources(cdsa PRIVATE
    src/cdsa_placeholder.c
    src/cdsa_status.c
    src/cdsa_alloc.c
    src/cdsa_callbacks.c
    src/cdsa_vec.c
)
target_include_directories(cdsa PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(cdsa PRIVATE cdsa_warnings cdsa_sanitize)

# ── Tests ────────────────────────────────────────────────────────────────────
if(CDSA_BUILD_TESTS)
    include(CTest)   # generates DartConfiguration.tcl needed by ctest -T memcheck
    add_subdirectory(tests)
endif()

# ── Benchmarks ───────────────────────────────────────────────────────────────
if(CDSA_BUILD_BENCH AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bench/CMakeLists.txt")
    add_subdirectory(bench)
endif()

# ── Examples ─────────────────────────────────────────────────────────────────
if(CDSA_BUILD_EXAMPLES AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples/CMakeLists.txt")
    add_subdirectory(examples)
endif()

# ── Documentation ─────────────────────────────────────────────────────────────
find_package(Doxygen OPTIONAL_COMPONENTS dot)
if(DOXYGEN_FOUND)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in"
        "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
        @ONLY
    )
    add_custom_target(docs
        COMMAND "${DOXYGEN_EXECUTABLE}" "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Generating API documentation (docs/html/index.html)"
        VERBATIM
    )
endif()
