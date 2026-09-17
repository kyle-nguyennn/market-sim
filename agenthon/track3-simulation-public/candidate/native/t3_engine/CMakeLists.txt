cmake_minimum_required(VERSION 3.20)

project(
    t3_engine
    VERSION 0.1.0
    DESCRIPTION "Native engine components for the Agenthon Track 3 simulator"
    LANGUAGES CXX
)

option(T3_ENGINE_BUILD_PYTHON "Build the pybind11 extension" ON)
option(T3_ENGINE_BUILD_TESTS "Build native smoke tests" ON)
option(T3_ENGINE_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

add_library(
    t3_orderbook_core
    STATIC
    src/orderbook/order_book.cpp
)
add_library(t3::orderbook ALIAS t3_orderbook_core)

target_compile_features(t3_orderbook_core PUBLIC cxx_std_20)
set_target_properties(t3_orderbook_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(
    t3_orderbook_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if(MSVC)
    target_compile_options(t3_orderbook_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(t3_orderbook_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(T3_ENGINE_ENABLE_SANITIZERS)
    if(MSVC)
        message(FATAL_ERROR "T3_ENGINE_ENABLE_SANITIZERS is not configured for MSVC")
    endif()
    target_compile_options(
        t3_orderbook_core
        PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer
    )
    target_link_options(
        t3_orderbook_core
        PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer
    )
endif()

if(T3_ENGINE_BUILD_PYTHON)
    find_package(pybind11 CONFIG REQUIRED)
    pybind11_add_module(_native bindings/python.cpp)
    target_link_libraries(_native PRIVATE t3::orderbook)
    target_compile_features(_native PRIVATE cxx_std_20)
    install(TARGETS _native DESTINATION t3_fast_orderbook)
endif()

if(T3_ENGINE_BUILD_TESTS)
    enable_testing()
    add_executable(
        t3_orderbook_smoke
        tests/cpp/order_book_smoke.cpp
    )
    target_link_libraries(t3_orderbook_smoke PRIVATE t3::orderbook)
    add_test(NAME t3_orderbook_smoke COMMAND t3_orderbook_smoke)
endif()
