cmake_minimum_required(VERSION 3.14)
project(ecewo_cookie VERSION 0.2.0 LANGUAGES C)

add_library(ecewo_cookie
  src/ecewo-cookie.c
)

add_library(ecewo::cookie ALIAS ecewo_cookie)

include(GenerateExportHeader)
generate_export_header(ecewo_cookie
  BASE_NAME ECEWO_COOKIE
  EXPORT_FILE_NAME "${CMAKE_CURRENT_BINARY_DIR}/include/ecewo-cookie-export.h"
)

target_include_directories(ecewo_cookie PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

if (PROJECT_IS_TOP_LEVEL AND NOT TARGET ecewo::ecewo)
  include(FetchContent)

  FetchContent_Declare(
    ecewo
    GIT_REPOSITORY https://github.com/ecewo/ecewo.git
    GIT_TAG v4.0.0
    GIT_SHALLOW TRUE
  )

  FetchContent_MakeAvailable(ecewo)
endif()

if (PROJECT_IS_TOP_LEVEL AND TARGET ecewo::ecewo)
  ecewo_add(mock@dev)
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

target_link_libraries(ecewo_cookie PUBLIC ecewo::ecewo)
