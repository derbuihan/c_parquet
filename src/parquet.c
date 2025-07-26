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

parquet_metadata_t* parquet_read_metadata(parquet_reader_t* reader) {
  thrift_reader_t* thrift_reader = parquet_read_footer(reader);
  if (!thrift_reader) return NULL;  // Failed to read footer

  parquet_metadata_t* metadata = malloc(sizeof(parquet_metadata_t));
  if (!metadata) {
    free(thrift_reader);
    return NULL;  // Memory allocation failed
  }

  thrift_field_t field;
  int16_t last_field_id = 0;
  if (thrift_read_field(thrift_reader, &field, &last_field_id) != 0) {
    free(metadata);
    thrift_reader_free(thrift_reader);
    return NULL;  // Failed to read field
  }

  printf("Field ID: %d, Type: %d\n", field.field_id, field.type);
  metadata->version = field.value->i32_val;

  thrift_reader_free(thrift_reader);
  return metadata;
}

void print_metadata(const parquet_metadata_t* metadata) {
  if (!metadata) return;

  printf("Parquet Metadata:\n");
  printf("  Version: %d\n", metadata->version);
  // Add more fields as needed
}
