cmake_minimum_required(VERSION 3.10)
project(mio_progetto)

set(CMAKE_CXX_STANDARD 20)

add_executable(Mammuthc
    src/main.cpp
    src/ast.h
    src/debug.h
    src/driver.cpp
    src/driver.h
    src/interpreter.cpp
    src/interpreter.h
    src/lexer.cpp
    src/lexer.h
    src/main.cpp
    src/parser.cpp
    src/parser.h
    src/range.h
    src/scope.h
    src/utf8.h
    src/value.h
)

