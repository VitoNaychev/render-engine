cmake_minimum_required(VERSION 3.14)
project(Engine LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(PROJECT_SOURCES
    engine/engine.cpp
    app/app.cpp
    app/window.cpp
    app/main.cpp
    app/viewport.cpp
)

qt_add_executable(EngineApp ${PROJECT_SOURCES})
target_link_libraries(EngineApp PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

# --- DXC + shader header generation ----
find_program(DXC_EXECUTABLE dxc REQUIRED)

set(SHADER_DIR ${CMAKE_SOURCE_DIR}/engine/shaders)
set(GEN_DIR ${CMAKE_BINARY_DIR}/generated_shaders)
file(MAKE_DIRECTORY ${GEN_DIR})

function(compile_hlsl_header TARGET SHADER ENTRY PROFILE HEADER_OUT SYMBOL_NAME)
    add_custom_command(
        OUTPUT ${HEADER_OUT}
        COMMAND ${DXC_EXECUTABLE}
                -T ${PROFILE}
                -E ${ENTRY}
                -Fh ${HEADER_OUT}
                -Vn ${SYMBOL_NAME}
                ${SHADER}
        DEPENDS ${SHADER}
        COMMENT "Generating shader header ${HEADER_OUT}"
        VERBATIM
    )

    # Register generated file with the target (nice for IDEs)
    target_sources(${TARGET} PRIVATE ${HEADER_OUT})
endfunction()

compile_hlsl_header(
    EngineApp
    ${SHADER_DIR}/ConstColorVS.hlsl
    VSMain
    vs_6_0
    ${GEN_DIR}/const_color_vs.h
    g_const_color_vs
)

compile_hlsl_header(
    EngineApp
    ${SHADER_DIR}/ConstColor.hlsl
    PSMain
    ps_6_0
    ${GEN_DIR}/const_color_ps.h
    g_const_color_ps
)

add_custom_target(CompileShaders
    DEPENDS
        ${GEN_DIR}/const_color_vs.h
        ${GEN_DIR}/const_color_ps.h
)

# If you want shaders to build when EngineApp builds:
add_dependencies(EngineApp CompileShaders)

target_include_directories(EngineApp PRIVATE ${GEN_DIR}  ${CMAKE_CURRENT_SOURCE_DIR}/external)
