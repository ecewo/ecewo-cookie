cmake_minimum_required(VERSION 3.16)
project(ecewo_cookie VERSION 0.1.0 LANGUAGES C)

add_library(ecewo_cookie
  src/ecewo-cookie.c
)

add_library(ecewo::cookie ALIAS ecewo_cookie)

target_include_directories(ecewo_cookie PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

if (PROJECT_IS_TOP_LEVEL AND NOT TARGET ecewo::ecewo)
  include(FetchContent)

  FetchContent_Declare(
    ecewo
    GIT_REPOSITORY https://github.com/savashn/ecewo.git
    GIT_TAG        dev
    GIT_SHALLOW    TRUE
  )

  FetchContent_MakeAvailable(ecewo)
endif()

target_link_libraries(ecewo_cookie PUBLIC ecewo::ecewo)

target_compile_definitions(ecewo_cookie PRIVATE _GNU_SOURCE)

if (PROJECT_IS_TOP_LEVEL AND TARGET ecewo::ecewo)

  ecewo_add_plugin(mock)

  option(ECEWO_COOKIE_BUILD_TESTS "Build tests" ON)

  if (ECEWO_COOKIE_BUILD_TESTS)
    enable_testing()

    add_executable(ecewo_cookie_test
      tests/test.c
      tests/runner.c
    )

    target_include_directories(ecewo_cookie_test
      PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )

    target_link_libraries(ecewo_cookie_test
      PRIVATE
        ecewo::ecewo
        ecewo::cookie
        ecewo::mock
    )

    if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
      target_compile_options(ecewo_cookie_test PRIVATE
        -Wall
        -Wextra
        -Werror
        -Wstrict-prototypes
        -Wincompatible-pointer-types
        -Wno-unused-parameter
      )
    endif()

    add_test(
      NAME ecewo_cookie_all
      COMMAND ecewo_cookie_test
    )
  endif()
endif()
