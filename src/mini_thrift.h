#ifndef MINI_THRIFT_H
#define MINI_THRIFT_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define MT_STOP 0
#define MT_I32 8
#define MT_I64 10
#define MT_STRING 11

typedef struct
{
    uint8_t* data; // Pointer to the Thrift data
    size_t size; // Total size of the Thrift data
    size_t pos; // Current position in the Thrift data
} MiniThrift;


void mt_init(MiniThrift* mt, const uint8_t* data, size_t size);
bool mt_read_field(MiniThrift* mt, uint8_t* type, int16_t* field_id);
int32_t mt_read_i32(MiniThrift* mt);
int64_t mt_read_i64(MiniThrift* mt);
char* mt_read_string(MiniThrift* mt);

#endif // MINI_THRIFT_H
