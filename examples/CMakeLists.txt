if(MYASQUO_BUILD_EXAMPLES)
    include_directories("../src")
    add_executable(example example.cpp)
    target_link_libraries(example ${PROJECT_NAME})
endif()

install(FILES example.cpp
    DESTINATION ${CMAKE_INSTALL_DOCDIR}/examples)
