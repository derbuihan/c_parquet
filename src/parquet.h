#ifndef C_PARQUET_PARQUET_H
#define C_PARQUET_PARQUET_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "thrift.h"

#define PARQUET_MAGIC "PAR1"
#define PARQUET_MAGIC_SIZE 4

typedef struct {
  FILE* file;
  size_t file_size;
  uint8_t* buffer;
} parquet_reader_t;

typedef struct {
  // 4: required string name;
  char* name;
} parquet_schema_element_t;

typedef struct {
} parquet_column_chunk_t;

typedef struct {
  // 1: required list<ColumnChunk> columns
  size_t column_count;
  parquet_column_chunk_t** columns;

  // 2: required i64 total_byte_size
  int64_t total_byte_size;

  // 3: required i64 num_rows
  int64_t num_rows;
} parquet_row_group_t;

typedef struct {
  // 1: required string key
  char* key;

  // 2: optional string value
  char* value;
} parquet_key_value_t;

typedef struct {
  // 1: required i32 version
  int32_t version;

  // 2: required list<SchemaElement> schema;
  size_t schema_count;
  parquet_schema_element_t** schema;

  // 3: required i64 num_rows
  int64_t num_rows;

  // 4: required list<RowGroup> row_groups
  size_t row_group_count;
  parquet_row_group_t** row_groups;

  // 5: optional list<KeyValue> key_value_metadata
  size_t key_value_metadata_count;
  parquet_key_value_t** key_value_metadata;

  // 6: optional string created_by
  char* created_by;
} parquet_file_metadata_t;

parquet_reader_t* parquet_open(const char* filename);
int parquet_close(parquet_reader_t* reader);

int parquet_struct_read_schema(thrift_struct_t* struct_val, uint16_t field_id,
                               size_t* schema_count,
                               parquet_schema_element_t** schema);
int parquet_struct_read_metadata(thrift_struct_t* struct_val,
                                 parquet_file_metadata_t* metadata);
parquet_file_metadata_t* parquet_read_metadata(parquet_reader_t* reader);

void print_metadata(const parquet_file_metadata_t* metadata);

#endif  // C_PARQUET_PARQUET_H
