cmake_minimum_required(VERSION 3.16)
project(NekoECATConverter VERSION 0.3.0 LANGUAGES CXX)

# ═══ C++ 标准与自动处理 ══════════════════════════════════════
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ═══ Qt5 依赖 ═══════════════════════════════════════════════
find_package(Qt5 REQUIRED COMPONENTS Widgets Core Gui)

# ═══ 源文件列表 ══════════════════════════════════════════════
# 分层结构:
#   core/  — 主题、样式、常量 (不依赖 UI 类)
#   根目录 — 主窗口、工作线程、入口

set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/worker.cpp
    src/core/theme.cpp
)

set(HEADERS
    src/mainwindow.h
    src/worker.h
    src/core/theme.h
)

# ═══ 构建目标 ════════════════════════════════════════════════
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt5::Widgets
    Qt5::Core
    Qt5::Gui
)

target_include_directories(${PROJECT_NAME} PRIVATE
    src
    src/core
)

# ═══ 安装 ════════════════════════════════════════════════════
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
