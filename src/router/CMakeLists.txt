
include_directories(
	${CMAKE_SOURCE_DIR}/lib/tinyjs
	${CMAKE_SOURCE_DIR}/lib/soci/include
)


add_library(router STATIC
	functions.cxx
	router.cxx
	config.cxx
	ejs.cxx
	context.cxx
	cache.cxx
)

target_link_libraries(router tinyjs SOCI::soci)

# add_subdirectory(test)
