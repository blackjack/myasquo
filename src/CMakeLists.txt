set(myasquo_SRCS Myasquo.cpp DBQueue.cpp)

find_package(Boost COMPONENTS thread system filesystem REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

find_package(MySQL REQUIRED)
include_directories(${MYSQL_INCLUDE_DIR})

add_library(${PROJECT_NAME} ${MYASQUO_LIBRARY_TYPE} ${myasquo_SRCS})
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES} ${MYSQL_LIBRARIES})

set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION "${LIB_VERSION_STRING}"
    SOVERSION "${LIB_MAJOR_VERSION}"
    DEFINE_SYMBOL "MYASQUO_MAKEDLL")

INSTALL(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
   RUNTIME DESTINATION bin
   ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
