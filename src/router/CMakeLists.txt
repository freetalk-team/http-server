
include_directories(
	${CMAKE_SOURCE_DIR}/lib/tinyjs
	${CMAKE_SOURCE_DIR}/lib/duktape
	${CMAKE_SOURCE_DIR}/lib/soci/include
)


add_library(router STATIC
	functions.cxx
	router.cxx
	config.cxx
	db.cxx
	context.cxx
	utils.cxx
)

add_subdirectory(executor)

target_link_libraries(router PRIVATE cmark tinyjs duktape SOCI::soci)

# add_subdirectory(test)
