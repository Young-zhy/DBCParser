cmake_minimum_required(VERSION 3.19)
project(DBCParserServer LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets WebSockets)
find_package(Protobuf CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

qt_standard_project_setup()

qt_add_executable(DBCParserServer
    WIN32 MACOSX_BUNDLE
    src/main.cpp
    src/gui/mainwindow.cpp
    src/gui/mainwindow.h
    src/gui/mainwindow.ui
    src/websocketserver/websocketserver.h
    src/websocketserver/websocketserver.cpp
    src/logger/logger.cpp
    src/logger/logger.h
    src/logger/QtTextEditSink.h
)

target_include_directories(DBCParserServer PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(DBCParserServer
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::WebSockets
        protobuf::libprotobuf
        spdlog::spdlog
)

install(TARGETS DBCParserServer
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET DBCParserServer
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})

# 自动生成 .pb.cc/.pb.h
protobuf_generate(
    TARGET DBCParserServer
    PROTOS src/protobuf/can_frame.proto
)
