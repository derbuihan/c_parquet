#ifndef C_PARQUET_THRIFT_H
#define C_PARQUET_THRIFT_H
#include <stddef.h>
#include <stdint.h>

typedef enum
{
    COMPACT_TYPE_STOP = 0,
    COMPACT_TYPE_BOOL_TRUE = 1,
    COMPACT_TYPE_BOOL_FALSE = 2,
    COMPACT_TYPE_BYTE = 3,
    COMPACT_TYPE_I16 = 4,
    COMPACT_TYPE_I32 = 5,
    COMPACT_TYPE_I64 = 6,
    COMPACT_TYPE_DOUBLE = 7,
    COMPACT_TYPE_STRING = 8,
    COMPACT_TYPE_LIST = 9,
    COMPACT_TYPE_SET = 10,
    COMPACT_TYPE_MAP = 11,
    COMPACT_TYPE_STRUCT = 12
} compact_type_t;

typedef struct
{
    uint8_t type; // compact_type_t
    int16_t field_id;
} thrift_field_header_t;

typedef struct
{
    uint8_t* data;
    size_t size;
    size_t position;
} thrift_reader_t;

int thrift_reader_init(thrift_reader_t* reader, uint8_t* data, size_t size);
int thrift_read_field_header(thrift_reader_t* reader, thrift_field_header_t* header, int16_t* last_field_id);
int thrift_skip_field(thrift_reader_t* reader, uint8_t field_type, int16_t* last_field_id);
int thrift_read_varint32(thrift_reader_t* reader, uint32_t* value);
int thrift_read_varint64(thrift_reader_t* reader, uint64_t* value);
void thrift_print_hex(thrift_reader_t* reader, int bytes);

#endif // C_PARQUET_THRIFT_H
