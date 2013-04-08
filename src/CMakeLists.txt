install(FILES
        Myasquo.h
        DBQueue.h
        DESTINATION ${INCLUDE_INSTALL_DIR}/myasquo COMPONENT Devel
)



set (myasquo_SRCS Myasquo.cpp DBQueue.cpp)

add_library (myasquo STATIC ${myasquo_SRCS})
target_link_libraries(myasquo ${Boost_LIBRARIES})
target_link_libraries(myasquo ${MYSQL_LIBRARIES})

set_target_properties(myasquo PROPERTIES
                      VERSION ${MYASQUO_LIB_MAJOR_VERSION}.${MYASQUO_LIB_MINOR_VERSION}.${MYASQUO_LIB_PATCH_VERSION}
                      SOVERSION ${MYASQUO_LIB_MAJOR_VERSION}
                      DEFINE_SYMBOL MYASQUO_MAKEDLL
                      )

INSTALL(TARGETS myasquo
   LIBRARY DESTINATION ${LIB_INSTALL_DIR}
   RUNTIME DESTINATION bin
   ARCHIVE DESTINATION  ${LIB_INSTALL_DIR}
)


add_executable(example examples/example.cpp)
target_link_libraries(example myasquo)

install(FILES
        examples/example.cpp
        DESTINATION share/doc/libmyasquo/examples
)
