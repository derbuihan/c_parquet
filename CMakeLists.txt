cmake_minimum_required(VERSION 3.14)
project(c_parquet C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(c_parquet src/main.c
        src/parquet.c
        src/parquet.h
        src/thrift.c
        src/thrift.h
)
