cmake_minimum_required(VERSION 3.19)
project(DBCParser LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets WebSockets)
find_package(Protobuf CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

qt_standard_project_setup()

qt_add_executable(DBCParser
    WIN32 MACOSX_BUNDLE
    src/main.cpp
    src/gui/main_window.cpp
    src/gui/main_window.h
    src/gui/main_window.ui
    src/dbc_parser.h src/dbc_parser.cpp
    src/websocket_client.h src/websocket_client.cpp
    src/logger/logger.h src/logger/logger.cpp
    src/logger/QtTextEditSink.h
)

target_include_directories(DBCParser PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(DBCParser
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::WebSockets
        protobuf::libprotobuf
        spdlog::spdlog
)

install(TARGETS DBCParser
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET DBCParser
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)

install(SCRIPT ${deploy_script})

# 自动生成 .pb.cc/.pb.h
protobuf_generate(
    TARGET DBCParser
    PROTOS src/protobuf/can_frame.proto
)

