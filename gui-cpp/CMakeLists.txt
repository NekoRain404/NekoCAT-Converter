cmake_minimum_required(VERSION 3.16)
project(NekoECATConverter VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 REQUIRED COMPONENTS Widgets Core Gui)

# 源文件
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/worker.cpp
    src/pages/fileselectpage.cpp
    src/pages/identitypage.cpp
    src/pages/rulepage.cpp
    src/pages/objecttablepage.cpp
    src/pages/pdopage.cpp
    src/pages/generatepage.cpp
)

set(HEADERS
    src/mainwindow.h
    src/worker.h
    src/pages/fileselectpage.h
    src/pages/identitypage.h
    src/pages/rulepage.h
    src/pages/objecttablepage.h
    src/pages/pdopage.h
    src/pages/generatepage.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt5::Widgets
    Qt5::Core
    Qt5::Gui
)

target_include_directories(${PROJECT_NAME} PRIVATE src)

# 安装
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
