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

// thrift_read_byte();
// thrift_read_bytes();
// thrift_read_string();

int thrift_reader_init(thrift_reader_t* reader, uint8_t* data, size_t size);
int thrift_read_field_header(thrift_reader_t* reader, thrift_field_header_t* header, int16_t* last_field_id);
int thrift_read_varint32(thrift_reader_t* reader, uint32_t* value);
int thrift_read_varint64(thrift_reader_t* reader, uint64_t* value);
int thrift_read_byte(thrift_reader_t* reader, uint8_t* value);
// int thrift_read_bytes(thrift_reader_t* reader, uint8_t* buffer, size_t size);
int thrift_read_string(thrift_reader_t* reader, char** str, uint32_t len);
// int thrift_read_list(thrift_reader_t* reader, thrift_list_t* list);
// int thrift_read_struct(thrift_reader_t* reader, thrift_struct_t* thrift_struct);

int thrift_skip_field(thrift_reader_t* reader, uint8_t field_type, int16_t* last_field_id);
void thrift_print_hex(thrift_reader_t* reader, int bytes);

typedef struct thrift_value thrift_value_t;

typedef struct
{
    thrift_value_t* fields;
    size_t field_count;
    size_t capacity;
} thrift_struct_t;

typedef struct
{
    thrift_value_t* elements;
    size_t count;
    uint8_t element_type;
} thrift_list_t;

struct thrift_value
{
    compact_type_t type;
    int16_t field_id;
    union
    {
        int bool_val;
        int8_t byte_val;
        int16_t i16_val;
        int32_t i32_val;
        int64_t i64_val;
        double double_val;
        struct
        {
            char* data;
            uint32_t len;
        } string_val;
        thrift_list_t list_val;
        thrift_struct_t struct_val;
    };
};


// thrift_skip_bool()
// thrift_skip_byte()
// thrift_skip_i16()
// thrift_skip_i32()
// thrift_skip_i64()
// thrift_skip_double()
// thrift_skip_string()
// thrift_skip_list()
// thrift_skip_struct()
// thrift_skip_set_or_map()

// thrift_parse_bool()
// thrift_parse_byte()
// thrift_parse_i16()
// thrift_parse_i32()
// thrift_parse_i64()
// thrift_parse_double()
// thrift_parse_string()
// thrift_parse_list()


int thrift_parse_value(thrift_reader_t* reader, thrift_value_t* value);
int thrift_parse_struct(thrift_reader_t* reader, thrift_struct_t* thrift_struct);
thrift_value_t* thrift_struct_get_field(thrift_struct_t* thrift_struct, int16_t field_id);

#endif // C_PARQUET_THRIFT_H
