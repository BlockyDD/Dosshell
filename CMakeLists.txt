cmake_minimum_required(VERSION 3.20)
project(Dosshell VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    add_definitions(-DNOMINMAX -DWIN32_LEAN_AND_MEAN -D_CRT_SECURE_NO_WARNINGS)
endif()

set(SOURCES
    src/main.cpp
    src/shell.cpp
    src/parser.cpp
    src/pipeline.cpp
    src/builtins.cpp
    src/console.cpp
)

set(HEADERS
    src/shell.h
    src/parser.h
    src/pipeline.h
    src/builtins.h
    src/console.h
)

add_executable(dosshell ${SOURCES} ${HEADERS})

if(MSVC)
    target_compile_options(dosshell PRIVATE /W4 /utf-8)
endif()
