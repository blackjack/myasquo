CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

PROJECT(myasquo CXX)
string(TOUPPER ${PROJECT_NAME} PROJECT_NAME_UPPER)

set(LIB_MAJOR_VERSION "0")
set(LIB_MINOR_VERSION "1")
set(LIB_PATCH_VERSION "3")

set(LIB_VERSION_STRING "${LIB_MAJOR_VERSION}.${LIB_MINOR_VERSION}.${LIB_PATCH_VERSION}")

include(GNUInstallDirs)

# compile in release with debug info mode by default
IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

IF("${CMAKE_BUILD_TYPE}" MATCHES "^Rel.*")
  ADD_DEFINITIONS("-DQT_NO_DEBUG_OUTPUT")
  ADD_DEFINITIONS("-DQT_NO_DEBUG_OUTPUT")
ENDIF("${CMAKE_BUILD_TYPE}" MATCHES "^Rel.*")


OPTION(MYASQUO_BUILD_SHARED "Build myasquo as a shared library" ON)
OPTION(MYASQUO_VERBOSE_DEBUG_OUTPUT "Enable additional verbose debug output" OFF)
OPTION(MYASQUO_BUILD_CXX11 "Build myasquo with C++11" ON)
OPTION(MYASQUO_BUILD_EXAMPLES "Build myasquo examples" ON)


# Select to build shared or static library
IF(MYASQUO_BUILD_SHARED)
    SET(MYASQUO_LIBRARY_TYPE SHARED)
else()
    SET(MYASQUO_LIBRARY_TYPE STATIC)
ENDIF()

IF(MYASQUO_BUILD_CXX11)
    ADD_DEFINITIONS("-std=c++11")
ENDIF()


# Ability to disable verbose debug output
IF(MYASQUO_VERBOSE_DEBUG_OUTPUT)
  ADD_DEFINITIONS("-DMYASQUO_VERBOSE_DEBUG_OUTPUT")
endif(MYASQUO_VERBOSE_DEBUG_OUTPUT)


include_directories(include)
install(DIRECTORY include/${PROJECT_NAME}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    COMPONENT dev)

# Subdirs
add_subdirectory(src)
add_subdirectory(examples)

EXPORT( PACKAGE ${PROJECT_NAME} )

# CMake config to provide library detection on target system. New policy
# requires to expand target name at run-time and not at configure time, thus
# requiring to run file(GENERATE) after configure_file() in some cases.

# See https://cmake.org/pipermail/cmake/2015-April/060350.html for more
# details.

SET(CMAKE_INSTALL_CMAKECONF ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}-config.cmake.in
               ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake.pre
               @ONLY)
FILE(GENERATE
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
    INPUT  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake.pre)

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}-config-version.cmake.in
               ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
               @ONLY)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
              DESTINATION ${CMAKE_INSTALL_CMAKECONF})


# pkg-config
IF (NOT WIN32)
    CONFIGURE_FILE (${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.pc.in
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc
        @ONLY)
    INSTALL (FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
ENDIF (NOT WIN32)

# Custom target to uninstall library and supplementary files
CONFIGURE_FILE(
  ${CMAKE_MODULE_PATH}/cmake_uninstall.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
  IMMEDIATE @ONLY)

ADD_CUSTOM_TARGET(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")
