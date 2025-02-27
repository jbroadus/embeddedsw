file(READ ${LIBMETAL_ROOT_DIR}/VERSION ver)

string(REGEX MATCH "VERSION_MAJOR = ([0-9]*)" _ ${ver})
set(PROJECT_VERSION_MAJOR ${CMAKE_MATCH_1})

string(REGEX MATCH "VERSION_MINOR = ([0-9]*)" _ ${ver})
set(PROJECT_VERSION_MINOR ${CMAKE_MATCH_1})

string(REGEX MATCH "VERSION_PATCH = ([0-9]*)" _ ${ver})
set(PROJECT_VERSION_PATCH ${CMAKE_MATCH_1})

set(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})

message(STATUS "libmetal version: ${PROJECT_VERSION} (${CMAKE_SOURCE_DIR})")

if (NOT DEFINED CMAKE_BUILD_TYPE)
  set (CMAKE_BUILD_TYPE Debug)
endif (NOT DEFINED CMAKE_BUILD_TYPE)
message ("-- Build type:  ${CMAKE_BUILD_TYPE}")

if (NOT CMAKE_INSTALL_LIBDIR)
  set (CMAKE_INSTALL_LIBDIR "lib")
endif (NOT CMAKE_INSTALL_LIBDIR)

if (NOT CMAKE_INSTALL_BINDIR)
  set (CMAKE_INSTALL_BINDIR "bin")
endif (NOT CMAKE_INSTALL_BINDIR)

set (_host "${CMAKE_HOST_SYSTEM_NAME}/${CMAKE_HOST_SYSTEM_PROCESSOR}")
message ("-- Host:    ${_host}")

set (_target "${CMAKE_SYSTEM_NAME}/${CMAKE_SYSTEM_PROCESSOR}")
message ("-- Target:  ${_target}")

if (NOT DEFINED MACHINE)
  set (MACHINE "Generic")
endif (NOT DEFINED MACHINE)
message ("-- Machine: ${MACHINE}")

# handle if '-' in machine name
string (REPLACE "-" "_" MACHINE ${MACHINE})

if (NOT DEFINED PROJECT_SYSTEM)
  string (TOLOWER ${CMAKE_SYSTEM_NAME}      PROJECT_SYSTEM)
  string (TOUPPER ${CMAKE_SYSTEM_NAME}      PROJECT_SYSTEM_UPPER)
endif (NOT DEFINED PROJECT_SYSTEM)

string (TOLOWER ${CMAKE_SYSTEM_PROCESSOR} PROJECT_PROCESSOR)
string (TOUPPER ${CMAKE_SYSTEM_PROCESSOR} PROJECT_PROCESSOR_UPPER)
string (TOLOWER ${MACHINE}                PROJECT_MACHINE)
string (TOUPPER ${MACHINE}                PROJECT_MACHINE_UPPER)

option (WITH_STATIC_LIB "Build with a static library" ON)

if ("${PROJECT_SYSTEM}" STREQUAL "linux")
  option (WITH_SHARED_LIB "Build with a shared library" ON)
  option (WITH_TESTS      "Install test applications" ON)
endif ("${PROJECT_SYSTEM}" STREQUAL "linux")

if (WITH_TESTS AND (${_host} STREQUAL ${_target}))
  option (WITH_TESTS_EXEC "Run test applications during build" ON)
endif (WITH_TESTS AND (${_host} STREQUAL ${_target}))

if (WITH_ZEPHYR)
  option (WITH_ZEPHYR_LIB "Build libmetal as a zephyr library" OFF)
endif (WITH_ZEPHYR)

option (WITH_DEFAULT_LOGGER "Build with default logger" ON)

option (WITH_DOC "Build with documentation" ON)

option (WITH_VFIO "Build with VFIO enabled" OFF)

set_property (GLOBAL PROPERTY "PROJECT_EC_FLAGS" -Wall -Werror -Wextra)
# vim: expandtab:ts=2:sw=2:smartindent
