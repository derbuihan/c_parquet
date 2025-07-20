cmake_minimum_required(VERSION 3.14)
project(c_parquet C)

set(CMAKE_C_STANDARD 11)

add_executable(c_parquet src/main.c
        src/parquet_reader.c
        src/parquet_reader.h
        src/parquet_metadata.c
        src/parquet_metadata.h
        src/mini_thrift.c
        src/mini_thrift.h)
