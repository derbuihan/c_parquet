#ifndef C_PARQUET_THRIFT_H
#define C_PARQUET_THRIFT_H
#include <stdbool.h>
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

typedef union thrift_data thrift_data_t;

typedef struct
{
    size_t len;
    uint8_t* data;
} thrift_binary_t;

typedef struct
{
    compact_type_t type;
    size_t count;
    thrift_data_t** elements;
} thrift_list_t;

typedef struct
{
    uint16_t field_id;
    compact_type_t type;
    thrift_data_t* value;
} thrift_field_t;

typedef struct
{
    size_t field_count;
    thrift_field_t** fields;
} thrift_struct_t;

union thrift_data
{
    bool bool_val; // BOOL_TRUE / BOOL_FALSE
    int8_t byte_val; // BYTE
    int16_t i16_val; // I16
    int32_t i32_val; // I32
    int64_t i64_val; // I64
    double double_val; // DOUBLE
    thrift_binary_t binary_val; // STRING
    thrift_list_t list_val; // LIST/SET
    thrift_struct_t struct_val; // STRUCT
};

typedef struct
{
    uint8_t* data;
    size_t size;
    size_t position;
} thrift_reader_t;

thrift_reader_t* thrift_reader_init(uint8_t* data, size_t size);
void thrift_reader_free(thrift_reader_t* reader);

int thrift_read_varint32(thrift_reader_t* reader, uint32_t* value);
int thrift_read_zigzag32(thrift_reader_t* reader, int32_t* value);
int thrift_read_varint64(thrift_reader_t* reader, uint64_t* value);
int thrift_read_zigzag64(thrift_reader_t* reader, int64_t* value);
int thrift_read_struct(thrift_reader_t* reader, thrift_struct_t* struct_val);
int thrift_read_field_header(thrift_reader_t* reader, thrift_field_t* field,
                             int16_t* last_field_id) ;
int thrift_read_field(thrift_reader_t* reader, thrift_field_t* field);

int thrift_struct_get_i32(thrift_struct_t* struct_val, uint16_t field_id,
                             int32_t* value);
int thrift_struct_get_i64(thrift_struct_t* struct_val, uint16_t field_id,
                             int64_t* value);
int thrift_struct_get_string(thrift_struct_t* struct_val, uint16_t field_id,
                             char** value);

void thrift_print_root(const thrift_struct_t* root);

#endif // C_PARQUET_THRIFT_H
