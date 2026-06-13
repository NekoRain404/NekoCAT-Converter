cmake_minimum_required(VERSION 3.16)
project(NekoECATConverter VERSION 0.2.0 LANGUAGES CXX)

# ═══ C++ 标准与自动处理 ══════════════════════════════════════
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)   # 自动处理 Q_OBJECT 宏
set(CMAKE_AUTORCC ON)   # 自动处理 .qrc 资源文件
set(CMAKE_AUTOUIC ON)   # 自动处理 .ui 文件

# ═══ Qt5 依赖 ═══════════════════════════════════════════════
find_package(Qt5 REQUIRED COMPONENTS Widgets Core Gui)

# ═══ 源文件列表 ══════════════════════════════════════════════
# 采用分层结构:
#   core/  — 主题、样式、常量 (不依赖 UI 类)
#   pages/ — 各功能页面 (可独立使用)
#   根目录 — 主窗口、工作线程、入口

set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/worker.cpp
    src/core/theme.cpp
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
    src/core/theme.h
    src/pages/fileselectpage.h
    src/pages/identitypage.h
    src/pages/rulepage.h
    src/pages/objecttablepage.h
    src/pages/pdopage.h
    src/pages/generatepage.h
)

# ═══ 构建目标 ════════════════════════════════════════════════
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt5::Widgets
    Qt5::Core
    Qt5::Gui
)

# 包含目录: src/ 下的子目录可直接 #include
target_include_directories(${PROJECT_NAME} PRIVATE
    src
    src/core
    src/pages
)

# ═══ 安装 ════════════════════════════════════════════════════
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
