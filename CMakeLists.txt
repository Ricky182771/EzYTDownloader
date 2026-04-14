cmake_minimum_required(VERSION 3.20)
project(EzYTDownloader VERSION 1.0.0 LANGUAGES CXX)

# ── C++20 ────────────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Qt6 auto‑generators ─────────────────────────────────────────────────────
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC OFF)

# ── Find Qt6 ─────────────────────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS Widgets Core Concurrent Network)

# ── Sources ──────────────────────────────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/Application.cpp
    src/MainWindow.cpp
    src/DependencyChecker.cpp
    src/DependencyDialog.cpp
    src/DownloadManager.cpp
    src/FetchWorker.cpp
    src/ConvertWorker.cpp
    src/SettingsManager.cpp
    src/Utils.cpp
)

set(HEADERS
    src/Application.h
    src/MainWindow.h
    src/DependencyChecker.h
    src/DependencyDialog.h
    src/DownloadManager.h
    src/FetchWorker.h
    src/ConvertWorker.h
    src/SettingsManager.h
    src/StreamInfo.h
    src/Utils.h
)

set(RESOURCES
    resources/resources.qrc
)

# ── Executable ───────────────────────────────────────────────────────────────
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${RESOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets Qt6::Core Qt6::Concurrent Qt6::Network)

# ── Compile definitions: project source dir for dev‑mode path resolution ────
target_compile_definitions(${PROJECT_NAME} PRIVATE
    APP_VERSION="${PROJECT_VERSION}"
    SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

# ── Install rules ────────────────────────────────────────────────────────────
include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_BINDIR})

install(FILES python/fetch.py
    DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/python
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                GROUP_READ GROUP_EXECUTE
                WORLD_READ WORLD_EXECUTE)

install(FILES scripts/install_deps.sh
    DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/scripts
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
                GROUP_READ GROUP_EXECUTE
                WORLD_READ WORLD_EXECUTE)

install(FILES scripts/ezytdownloader.desktop 
        DESTINATION share/applications)

install(FILES resources/icons/app_icon.png 
        DESTINATION share/icons/hicolor/256x256/apps 
        RENAME ezytdownloader.png)