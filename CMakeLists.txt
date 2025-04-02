cmake_minimum_required(VERSION 3.30)
project(E-Buy-API)

set(CMAKE_CXX_STANDARD 20)

configure_file(.env .env COPYONLY)
configure_file(database/database.db database.db COPYONLY)

find_package(Boost REQUIRED COMPONENTS asio beast uuid)
find_package(jwt-cpp REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(SQLite3 REQUIRED)

set(SOURCE
        main.cpp
        authentication-functions.cpp
        authentication-functions.h
        utils.cpp
        utils.h
        routers/api.cpp
        routers/api.h
        controllers/test.cpp
        controllers/test.h
        controllers/post.cpp
        controllers/post.h
        database/client.cpp
        database/client.h
)

add_executable(E-Buy-API ${SOURCE})
target_link_libraries(E-Buy-API Boost::asio Boost::beast Boost::uuid)
target_link_libraries(E-Buy-API jwt-cpp::jwt-cpp)
target_link_libraries(E-Buy-API nlohmann_json::nlohmann_json)
target_link_libraries(E-Buy-API SQLite::SQLite3)

if (WIN32)
    target_link_libraries(E-Buy-API ws2_32 wsock32)
endif ()
