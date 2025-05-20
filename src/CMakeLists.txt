
include_directories(${CMAKE_SOURCE_DIR}/lib/include)

add_subdirectory(router)

add_library (utils OBJECT 
	utils.cxx
)


# add_executable(cli http_server.cxx)
# target_link_libraries(cli ${LIBRARIES})
# set_property(TARGET cli PROPERTY CXX_STANDARD 23)

# add_executable(server-fast http_server_fast.cpp)
# target_link_libraries(server-fast ${LIBRARIES})

add_executable(server http_server.cxx)
# target_include_directories(server PRIVATE ${CMAKE_SOURCE_DIR}/lib/boost/include)
target_link_libraries(server utils ${LIBRARIES})
set_target_properties(server PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

