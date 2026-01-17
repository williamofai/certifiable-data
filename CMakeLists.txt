cmake_minimum_required(VERSION 3.16)
project(certifiable_data C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Strict compiler flags for safety-critical code
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -pedantic")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wshadow -Wconversion -Wstrict-prototypes")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-common")

# DVM-required flags: no FMA, no fast-math, preserve operation order
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffp-contract=off")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-fast-math")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-associative-math")

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Source files
set(DVM_SOURCES
    src/dvm/primitives.c
    src/dvm/prng.c
)

set(DATA_SOURCES
    src/data/loader.c
    src/data/normalize.c
    src/data/augment.c
    src/data/shuffle.c
    src/data/batch.c
)

set(AUDIT_SOURCES
    src/audit/merkle.c
    src/audit/sha256.c
)

# Build static library
add_library(certifiable_data STATIC
    ${DVM_SOURCES}
    ${DATA_SOURCES}
    ${AUDIT_SOURCES}
)

# Link math library to main library
target_link_libraries(certifiable_data m)

# Enable testing
enable_testing()

# Unit tests
add_executable(test_primitives tests/unit/test_primitives.c)
target_link_libraries(test_primitives certifiable_data m)
add_test(NAME test_primitives COMMAND test_primitives)

add_executable(test_prng tests/unit/test_prng.c)
target_link_libraries(test_prng certifiable_data m)
add_test(NAME test_prng COMMAND test_prng)

add_executable(test_normalize tests/unit/test_normalize.c)
target_link_libraries(test_normalize certifiable_data m)
add_test(NAME test_normalize COMMAND test_normalize)

add_executable(test_augment tests/unit/test_augment.c)
target_link_libraries(test_augment certifiable_data m)
add_test(NAME test_augment COMMAND test_augment)

add_executable(test_shuffle tests/unit/test_shuffle.c)
target_link_libraries(test_shuffle certifiable_data m)
add_test(NAME test_shuffle COMMAND test_shuffle)

add_executable(test_batch tests/unit/test_batch.c)
target_link_libraries(test_batch certifiable_data m)
add_test(NAME test_batch COMMAND test_batch)

add_executable(test_merkle tests/unit/test_merkle.c)
target_link_libraries(test_merkle certifiable_data m)
add_test(NAME test_merkle COMMAND test_merkle)

add_executable(test_bit_identity tests/unit/test_bit_identity.c)
target_link_libraries(test_bit_identity certifiable_data m)
add_test(NAME test_bit_identity COMMAND test_bit_identity)

# Examples (add when ready)
# add_executable(load_csv examples/load_csv.c)
# target_link_libraries(load_csv certifiable_data m)

# add_executable(batch_pipeline examples/batch_pipeline.c)
# target_link_libraries(batch_pipeline certifiable_data m)

# Custom targets
add_custom_target(test-all
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS test_primitives test_prng test_normalize test_augment
            test_shuffle test_batch test_merkle test_bit_identity
)
