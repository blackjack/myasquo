# Find myasquo - async mysql client library
#
# This module defines
#  MYASQUO_FOUND - whether the myasquo library was found
#  MYASQUO_LIBRARIES - the myasquo library
#  MYASQUO_INCLUDE_DIR - the include path of the myasquo library
#

if (MYASQUO_INCLUDE_DIR AND MYASQUO_LIBRARIES)

  # Already in cache
  set (MYASQUO_FOUND TRUE)

else (MYASQUO_INCLUDE_DIR AND MYASQUO_LIBRARIES)

  if (NOT WIN32)
    # use pkg-config to get the values of MYASQUO_INCLUDE_DIRS
    # and MYASQUO_LIBRARY_DIRS to add as hints to the find commands.
    include (FindPkgConfig)
    pkg_check_modules (myasquo REQUIRED myasquo>=0.1)
  endif (NOT WIN32)

  find_library (MYASQUO_LIBRARIES
    NAMES
    myasquo
    PATHS
    ${MYASQUO_LIBRARY_DIRS}
    ${LIB_INSTALL_DIR}
  )

  find_path (MYASQUO_INCLUDE_DIR
    NAMES
    Myasquo.h
    PATH_SUFFIXES
    myasquo
    PATHS
    ${MYASQUO_INCLUDE_DIRS}
    ${INCLUDE_INSTALL_DIR}
  )

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(MYASQUO DEFAULT_MSG MYASQUO_LIBRARIES MYASQUO_INCLUDE_DIR)

endif (MYASQUO_INCLUDE_DIR AND MYASQUO_LIBRARIES)
