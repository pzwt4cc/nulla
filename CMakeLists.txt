cmake_minimum_required(VERSION 3.21)

project(Nulla VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Quick QuickControls2)
find_program(CARGO_EXECUTABLE cargo REQUIRED)

if(MSVC)
    set(NULLA_CORE_LIB "${CMAKE_CURRENT_SOURCE_DIR}/core/target/release/nulla_core.lib")
else()
    set(NULLA_CORE_LIB "${CMAKE_CURRENT_SOURCE_DIR}/core/target/release/libnulla_core.a")
endif()

file(GLOB_RECURSE NULLA_RUST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/core/src/*.rs")

add_custom_command(
    OUTPUT "${NULLA_CORE_LIB}"
    COMMAND "${CARGO_EXECUTABLE}" build --release --manifest-path "${CMAKE_CURRENT_SOURCE_DIR}/core/Cargo.toml"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/core/Cargo.toml" ${NULLA_RUST_SOURCES}
    COMMENT "Building nulla_core Rust static library"
)

add_custom_target(nulla_core_rust DEPENDS "${NULLA_CORE_LIB}")

set(NULLA_SOURCES
    src/main.cpp
    src/editor_backend.cpp
    src/tab_session.cpp
    src/editor_view.cpp
    src/theme_palette.cpp
)

set(NULLA_HEADERS
    include/editor_backend.h
    include/tab_session.h
    include/editor_view.h
    include/theme_palette.h
    include/nulla_core.h
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

add_dependencies(nulla nulla_core_rust)

target_include_directories(nulla PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(nulla PRIVATE
    Qt6::Widgets
    Qt6::QuickControls2
    Qt6::Quick
    Qt6::Gui
    Qt6::Core
    "${NULLA_CORE_LIB}"
)

if(WIN32)
    set_target_properties(nulla PROPERTIES WIN32_EXECUTABLE TRUE)
    target_link_libraries(nulla PRIVATE ws2_32 userenv ntdll bcrypt advapi32)
    if(MSVC)
        target_compile_options(nulla PRIVATE /W4 /utf-8)
        target_link_options(nulla PRIVATE /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup)
    else()
        target_compile_options(nulla PRIVATE -Wall -Wextra -Wpedantic)
        target_link_options(nulla PRIVATE -mwindows)
    endif()
    install(TARGETS nulla RUNTIME DESTINATION .)
else()
    target_compile_options(nulla PRIVATE -Wall -Wextra -Werror -Wpedantic)
    target_link_libraries(nulla PRIVATE ${CMAKE_DL_LIBS} pthread m)
    include(GNUInstallDirs)
    install(TARGETS nulla RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(FILES assets/nulla.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES assets/nulla.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
    install(FILES assets/nulla.png DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/512x512/apps)
endif()
