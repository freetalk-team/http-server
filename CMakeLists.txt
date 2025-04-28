cmake_minimum_required(VERSION 3.20)

project (HttpServer VERSION 1.0.1 LANGUAGES CXX)

set (Boost_USE_STATIC_LIBS ON)
find_package (Boost 1.83.0 COMPONENTS context filesystem system thread url iostreams)

# Find JSON (header-only)
find_package (nlohmann_json REQUIRED)

# Find Threads
find_package (Threads REQUIRED)

find_package (OpenSSL REQUIRED)
# set (OPENSSL_LIBRARIES ${OPENSSL_CRYPTO_LIBRARY} ${OPENSSL_SSL_LIBRARY})

find_package (SQLite3 REQUIRED)
find_package (PostgreSQL REQUIRED)

# option (BUILD_SOCI "Build libsoci" ON)

set (CMAKE_CXX_STANDARD 23)

# VFALCO I want static but "b2 stage" builds a minimal set which excludes static
add_definitions (-DBOOST_ALL_STATIC_LINK=1)

add_definitions (-DBOOST_ASIO_NO_DEPRECATED=1)
add_definitions (-DBOOST_ASIO_DISABLE_BOOST_ARRAY=1)
add_definitions (-DBOOST_ASIO_DISABLE_BOOST_BIND=1)
add_definitions (-DBOOST_ASIO_DISABLE_BOOST_DATE_TIME=1)
add_definitions (-DBOOST_ASIO_DISABLE_BOOST_REGEX=1)
add_definitions (-DBOOST_COROUTINES_NO_DEPRECATION_WARNING=1)


include_directories(
	${CMAKE_BINARY_DIR}/include
)

add_subdirectory(lib/tinyjs)
add_subdirectory(lib/soci)

set(LIBRARIES 
	router
	${Boost_LIBRARIES}
	${SQLite3_LIBRARIES}
	${PostgreSQL_LIBRARIES}
	${OPENSSL_LIBRARIES}
	nlohmann_json::nlohmann_json
	Threads::Threads
	stdc++fs
	z
)

add_subdirectory(src)
