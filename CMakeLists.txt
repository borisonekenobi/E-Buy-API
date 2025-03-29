cmake_minimum_required(VERSION 3.30)
project(E-Buy-API)

set(CMAKE_CXX_STANDARD 20)

find_package(Boost REQUIRED COMPONENTS asio beast)
find_package(nlohmann_json REQUIRED)

set(SOURCE
        main.cpp
        routers/api.cpp
        routers/api.h
        controllers/test.cpp
        controllers/test.h
)

add_executable(E-Buy-API ${SOURCE})
target_link_libraries(E-Buy-API Boost::asio Boost::beast)
target_link_libraries(nlohmann_json::nlohmann_json)

if (WIN32)
    target_link_libraries(E-Buy-API ws2_32 wsock32)
endif ()
