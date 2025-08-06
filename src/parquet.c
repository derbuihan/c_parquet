#include "parquet.h"

#include <stdlib.h>
#include <string.h>

#include "thrift.h"

int parquet_validate_magic(parquet_reader_t* reader) {
  if (memcmp(reader->buffer, PARQUET_MAGIC, PARQUET_MAGIC_SIZE) != 0) return -1;

  size_t footer_offset = reader->file_size - PARQUET_MAGIC_SIZE;
  if (memcmp(reader->buffer + footer_offset, PARQUET_MAGIC,
             PARQUET_MAGIC_SIZE) != 0)
    return -1;

  return 0;
}

parquet_reader_t* parquet_open(const char* filename) {
  parquet_reader_t* reader = malloc(sizeof(parquet_reader_t));
  if (!reader) return NULL;  // Memory allocation failed

  reader->file = fopen(filename, "rb");
  if (!reader->file) {
    free(reader);
    return NULL;  // File open failed
  }

  fseek(reader->file, 0, SEEK_END);
  reader->file_size = ftell(reader->file);
  fseek(reader->file, 0, SEEK_SET);

  reader->buffer = malloc(reader->file_size);
  if (!reader->buffer) {
    fclose(reader->file);
    free(reader);
    return NULL;  // Memory allocation failed
  }
  fread(reader->buffer, 1, reader->file_size, reader->file);

  if (parquet_validate_magic(reader) != 0) {
    fclose(reader->file);
    free(reader->buffer);
    free(reader);
    return NULL;  // Invalid Parquet file magic
  }

  return reader;
}

int parquet_close(parquet_reader_t* reader) {
  if (reader->file) fclose(reader->file);
  if (reader->buffer) free(reader->buffer);
  free(reader);
  return 0;
}

thrift_reader_t* parquet_read_footer(parquet_reader_t* reader) {
  size_t footer_size_pos = reader->file_size - PARQUET_MAGIC_SIZE - 4;
  uint8_t* pos = reader->buffer + footer_size_pos;

  uint32_t footer_length =
      pos[0] | (pos[1] << 8) | (pos[2] << 16) | (pos[3] << 24);
  size_t metadata_pos = footer_size_pos - footer_length;

  thrift_reader_t* thrift_reader =
      thrift_reader_init(reader->buffer + metadata_pos, footer_length);
  return thrift_reader;
}

int parquet_struct_read_schema(thrift_struct_t* struct_val, uint16_t field_id,
                               size_t* schema_count,
                               parquet_schema_element_t** schema) {
  if (!struct_val || !schema) return -1;  // Invalid input

  thrift_list_t* list_val = malloc(sizeof(thrift_list_t));
  if (thrift_struct_get_list(struct_val, field_id, list_val) != 0)
    return -1;  // Failed to read schema list

  return 0;
}

int parquet_struct_read_metadata(thrift_struct_t* struct_val,
                                 parquet_file_metadata_t* metadata) {
  if (!struct_val || !metadata) return -1;  // Invalid input

  if (thrift_struct_get_i32(struct_val, 1, &metadata->version) != 0) return -1;
  if (parquet_struct_read_schema(struct_val, 2, &metadata->schema_count,
                                 metadata->schema) != 0)
    return -1;  // Failed to read schema

  if (thrift_struct_get_i64(struct_val, 3, &metadata->num_rows) != 0) return -1;
  if (thrift_struct_get_string(struct_val, 6, &metadata->created_by) != 0)
    return -1;
  metadata->schema_count = 0;
  metadata->schema = NULL;

  return 0;
}

parquet_file_metadata_t* parquet_read_metadata(parquet_reader_t* reader) {
  thrift_reader_t* thrift_reader = parquet_read_footer(reader);
  if (!thrift_reader) return NULL;  // Failed to read footer

  thrift_struct_t* root = malloc(sizeof(thrift_struct_t));
  if (thrift_read_struct(thrift_reader, root) != 0) {
    thrift_reader_free(thrift_reader);
    return NULL;  // Failed to read root struct
  }
  thrift_print_root(root);

  parquet_file_metadata_t* metadata = malloc(sizeof(parquet_file_metadata_t));
  if (!metadata) {
    free(thrift_reader);
    return NULL;  // Memory allocation failed
  }

  if (parquet_struct_read_metadata(root, metadata) != 0) {
    free(metadata);
    free(thrift_reader);
    return NULL;  // Failed to read metadata
  }

  thrift_reader_free(thrift_reader);
  return metadata;
}

void print_metadata(const parquet_file_metadata_t* metadata) {
  if (!metadata) return;

  printf("Parquet Metadata:\n");
  printf("  Version: %d\n", metadata->version);
  printf("  Number of Rows: %lld\n", metadata->num_rows);
  printf("  Created By: %s\n", metadata->created_by);

  // Add more fields as needed
}
