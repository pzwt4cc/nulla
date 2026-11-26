cmake_minimum_required(VERSION 3.21)

project(Nulla VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Quick QuickControls2)

set(NULLA_SOURCES
    src/main.cpp
    src/editor_backend.cpp
    src/tab_session.cpp
    src/highlighter.cpp
    src/gutter.cpp
)

set(NULLA_HEADERS
    include/editor_backend.h
    include/tab_session.h
    include/highlighter.h
    include/gutter.h
)

set(NULLA_RESOURCES
    resources.qrc
)

if(WIN32)
    list(APPEND NULLA_RESOURCES package/windows/nulla.rc)
endif()

add_executable(nulla
    ${NULLA_SOURCES}
    ${NULLA_HEADERS}
    ${NULLA_RESOURCES}
)

target_include_directories(nulla PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(nulla PRIVATE
    Qt6::Widgets
    Qt6::QuickControls2
    Qt6::Quick
    Qt6::Gui
    Qt6::Core
)

if(WIN32)
    set_target_properties(nulla PROPERTIES WIN32_EXECUTABLE TRUE)
    if(MSVC)
        target_compile_options(nulla PRIVATE /W4 /utf-8)
        target_link_options(nulla PRIVATE /ENTRY:mainCRTStartup)
    else()
        target_compile_options(nulla PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    install(TARGETS nulla RUNTIME DESTINATION .)
else()
    target_compile_options(nulla PRIVATE -Wall -Wextra -Werror -Wpedantic)
    target_link_libraries(nulla PRIVATE ${CMAKE_DL_LIBS})
    include(GNUInstallDirs)
    install(TARGETS nulla RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(FILES assets/nulla.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES assets/nulla.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
    install(FILES assets/nulla.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/512x512/apps)
endif()
