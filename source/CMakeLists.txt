cmake_minimum_required(VERSION 3.5)

add_executable(
    material_editor main.cpp
    data/document.h data/document.cpp
    rendering/renderer.h rendering/renderer.cpp
    rendering/resources.h rendering/resources.cpp
    rendering/document.h rendering/document.cpp
    rendering/shader.h rendering/shader.cpp

    ../third_party/SPIRV-Reflect/spirv_reflect.c
)

target_include_directories(
    material_editor
    PRIVATE
    ../third_party/SPIRV-Reflect
)

target_compile_definitions(material_editor PRIVATE _GLIBCXX_DEBUG)
target_compile_options(material_editor PRIVATE -Wall -Werror)
