#include "thrift.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

thrift_reader_t* thrift_reader_init(uint8_t* data, size_t size) {
  thrift_reader_t* reader = malloc(sizeof(thrift_reader_t));
  if (!reader) return NULL;  // Memory allocation failed
  reader->data = data;
  reader->size = size;
  reader->position = 0;
  return reader;
}

void thrift_reader_free(thrift_reader_t* reader) {
  if (reader) {
    free(reader);
  }
}

int thrift_read_varint32(thrift_reader_t* reader, uint32_t* value) {
  *value = 0;
  uint8_t shift = 0;

  while (reader->position < reader->size) {
    uint8_t byte = reader->data[reader->position++];
    *value |= (byte & 0x7F) << shift;
    if ((byte & 0x80) == 0) return 0;
    shift += 7;
    if (shift >= 32) return -1;
  }
  return -1;
}

int thrift_read_zigzag32(thrift_reader_t* reader, int32_t* value) {
  uint32_t varint;
  if (thrift_read_varint32(reader, &varint) != 0) return -1;
  *value = (int32_t)((varint >> 1) ^ -(int32_t)(varint & 1));
  return 0;
}

int thrift_read_varint64(thrift_reader_t* reader, uint64_t* value) {
  *value = 0;
  uint8_t shift = 0;

  while (reader->position < reader->size) {
    uint8_t byte = reader->data[reader->position++];
    *value |= (uint64_t)(byte & 0x7F) << shift;
    if ((byte & 0x80) == 0) return 0;
    shift += 7;
    if (shift >= 64) return -1;
  }
  return -1;
}

int thrift_read_zigzag64(thrift_reader_t* reader, int64_t* value) {
  uint64_t varint;
  if (thrift_read_varint64(reader, &varint) != 0) return -1;
  *value = (int64_t)((varint >> 1) ^ -(int64_t)(varint & 1));
  return 0;
}

int thrift_read_string(thrift_reader_t* reader, thrift_binary_t* binary) {
  uint32_t len;
  if (thrift_read_varint32(reader, &len) != 0)
    return -1;  // Failed to read length

  binary->data = malloc(len + 1);  // +1 for null terminator
  if (!binary->data) return -1;    // Memory allocation failed

  if (reader->position + len > reader->size) {
    free(binary->data);
    return -1;  // Out of bounds
  }

  memcpy(binary->data, reader->data + reader->position, len);
  binary->data[len] = '\0';  // Null-terminate the string
  binary->len = len;
  reader->position += len;

  return 0;  // Successfully read string
}

int thrift_read_list(thrift_reader_t* reader, thrift_list_t* list) {
  if (reader->position >= reader->size) return -1;  // Out of bounds

  uint8_t first_byte = reader->data[reader->position++];
  list->type = first_byte & 0x0F;
  uint8_t size_part = (first_byte & 0xF0) >> 4;
  if (size_part < 15) {
    list->count = size_part;
  } else {
    if (thrift_read_varint32(reader, &list->count) != 0)
      return -1;  // Failed to read size
  }

  list->elements = malloc(list->count * sizeof(thrift_data_t));
  if (!list->elements) return -1;  // Memory allocation failed

  int16_t last_field_id = 0;
  for (uint32_t i = 0; i < list->count; i++) {
    thrift_field_t field;
    if (thrift_read_field(reader, &field, &last_field_id) != 0) {
      free(list->elements);
      return -1;  // Failed to read list element
    }
    list->elements[i] = *field.value;  // Copy the value
  }
  return 0;  // Successfully read list
}

int thrift_read_field_header(thrift_reader_t* reader, thrift_field_t* field,
                             int16_t* last_field_id) {
  if (reader->position >= reader->size) return -1;  // Out of bounds

  uint8_t first_byte = reader->data[reader->position++];
  field->type = first_byte & 0x0F;
  int8_t delta = (first_byte & 0xF0) >> 4;

  if (field->type == COMPACT_TYPE_STOP) {
    field->field_id = 0;
    return 0;
  }

  if (delta == 0) {
    if (reader->position + 1 >= reader->size) return -1;  // Out of bounds
    // field->field_id = reader->data[reader->position] |
    //                   (reader->data[reader->position + 1] << 8);
    field->field_id = (reader->data[reader->position] << 8) |
                      reader->data[reader->position + 1];
    reader->position += 2;
  } else {
    field->field_id = *last_field_id + delta;
  }

  *last_field_id = field->field_id;

  printf("Field ID: %d, Type: %d\n", field->field_id, field->type);

  return 0;
}

int thrift_read_field(thrift_reader_t* reader, thrift_field_t* field,
                      int16_t* last_field_id) {
  if (thrift_read_field_header(reader, field, last_field_id) != 0) {
    return -1;  // Failed to read field header
  }
  field->value = malloc(sizeof(thrift_data_t));
  if (!field->value) return -1;  // Memory allocation failed

  switch (field->type) {
    case COMPACT_TYPE_I32:
      if (thrift_read_zigzag32(reader, &field->value->i32_val) != 0) {
        free(field->value);
        return -1;  // Failed to read I32 value
      }
      //   printf("I32 value: %d, field_id: %d, type: %d\n",
      //   field->value->i32_val,
      //          field->field_id, field->type);
      break;
    case COMPACT_TYPE_STRING:
      if (thrift_read_string(reader, &field->value->binary) != 0) {
        free(field->value);
        return -1;  // Failed to read string value
      }
      //   printf("String value: '%s', field_id: %d, type: %d\n",
      //          field->value->binary.data, field->field_id, field->type);
      break;
    case COMPACT_TYPE_LIST:
      if (thrift_read_list(reader, &field->value->list) != 0) {
        free(field->value);
        return -1;  // Failed to read list
      }
      //   printf("List with %u elements, field_id: %d, type: %d\n",
      //          field->value->list.count, field->field_id, field->type);
      break;
    default:
      break;
  }

  return 0;
}

int thrift_read_root(thrift_reader_t* reader, thrift_struct_t* root) {
  root->field_count = 0;
  root->fields = malloc(10 * sizeof(thrift_field_t));  // Initial capacity
  if (!root->fields) return -1;  // Memory allocation failed
  int16_t last_field_id = 0;
  thrift_field_t field;
  while (thrift_read_field(reader, &field, &last_field_id) == 0 &&
         field.type != COMPACT_TYPE_STOP) {
    if (root->field_count >= 10) {  // Resize if needed
      root->fields = realloc(root->fields,
                             (root->field_count + 10) * sizeof(thrift_field_t));
      if (!root->fields) return -1;  // Memory allocation failed
    }
    root->fields[root->field_count++] = field;
  }

  return 0;  // Successfully read root struct
}

/*

int thrift_read_field_header(thrift_reader_t* reader, thrift_field_header_t*
header, int16_t* last_field_id)
{
    if (reader->position >= reader->size)
        return -1;

    uint8_t first_byte = reader->data[reader->position++];
    header->type = first_byte & 0x0F;
    int8_t delta = (first_byte & 0xF0) >> 4;

    if (header->type == COMPACT_TYPE_STOP)
    {
        header->field_id = 0;
        return 0;
    }

    if (delta == 0)
    {
        if (reader->position + 1 > reader->size)
            return -1;
        header->field_id = reader->data[reader->position] |
(reader->data[reader->position + 1] << 8); reader->position += 2;
    }
    else
    {
        header->field_id = *last_field_id + delta;
    }

    *last_field_id = header->field_id;
    return 0;
}


int thrift_read_varint64(thrift_reader_t* reader, uint64_t* value)
{
    *value = 0;
    uint8_t shift = 0;
    while (reader->position < reader->size)
    {
        uint8_t byte = reader->data[reader->position++];
        *value |= (uint64_t)(byte & 0x7F) << shift;
        if ((byte & 0x80) == 0)
            break;
        shift += 7;
        if (shift >= 64)
            return -1; // Overflow
    }
    return 0;
}

int thrift_read_byte(thrift_reader_t* reader, uint8_t* value)
{
    if (reader->position >= reader->size)
        return -1; // Out of bounds
    *value = reader->data[reader->position++];
    return 0;
}

// int thrift_read_bytes(thrift_reader_t* reader, uint8_t* buffer, size_t
size);

int thrift_read_string(thrift_reader_t* reader, char** str, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++)
    {
        char c = reader->data[reader->position + i];
        if (c < 32 || c > 126)
            c = '.';
        (*str)[i] = c;
    }
    (*str)[len] = '\0'; // Null-terminate the string
    reader->position += len;

    return 0;
}

int32_t thrift_zigzag_decode32(uint32_t n) { return (int32_t)((n >> 1) ^
-(int32_t)(n & 1)); }

int64_t thrift_zigzag_decode64(uint64_t n) { return (int64_t)((n >> 1) ^
-(int64_t)(n & 1)); }

int thrift_skip_field(thrift_reader_t* reader, uint8_t field_type, int16_t*
last_field_id)
{
    switch (field_type)
    {
    case COMPACT_TYPE_STOP:
        break;
    case COMPACT_TYPE_BOOL_TRUE:
        printf("  → BOOL value: true\n");
        break;
    case COMPACT_TYPE_BOOL_FALSE:
        printf("  → BOOL value: false\n");
        break;
    case COMPACT_TYPE_BYTE:
        uint8_t byte_value;
        thrift_read_byte(reader, &byte_value);
        printf("  → BYTE value: %d\n", byte_value);
        break;
    case COMPACT_TYPE_I16:
        uint32_t varint16;
        thrift_read_varint32(reader, &varint16);
        int16_t decoded_i16 = thrift_zigzag_decode32(varint16);
        printf("  → I16 value: %d\n", decoded_i16);
        break;
    case COMPACT_TYPE_I32:
        uint32_t varint32;
        thrift_read_varint32(reader, &varint32);
        int32_t decoded_i32 = thrift_zigzag_decode32(varint32);
        printf("  → I32 value: %u\n", decoded_i32);
        break;
    case COMPACT_TYPE_I64:
        uint64_t varint64;
        thrift_read_varint64(reader, &varint64);
        int64_t decoded_i64 = thrift_zigzag_decode64(varint64);
        printf("  → I64 value: %llu\n", (long long)decoded_i64);
        break;
    case COMPACT_TYPE_DOUBLE:
        reader->position += 8;
        printf("  → DOUBLE (8 bytes)\n");
        break;
    case COMPACT_TYPE_STRING:
        uint32_t len;
        thrift_read_varint32(reader, &len);
        printf("  → STRING length: %u\n", len);

        char* str = malloc(len + 1);
        thrift_read_string(reader, &str, len);
        printf("  → STRING value: '%s'\n", str);
        break;
    case COMPACT_TYPE_LIST:
        if (reader->position >= reader->size)
            return -1;

        uint8_t list_header = reader->data[reader->position++];
        uint8_t element_type = list_header & 0x0F;
        uint8_t size_part = (list_header & 0xF0) >> 4;
        uint32_t size;

        if (size_part < 15)
        {
            size = size_part;
        }
        else
        {
            if (thrift_read_varint32(reader, &size) != 0)
                return -1;
        }

        printf("  → LIST: size=%u, element_type=%d\n", size, element_type);

        for (uint32_t i = 0; i < size; i++)
        {
            printf("    [%u] ", i);
            int16_t element_last_field_id = 0;
            if (thrift_skip_field(reader, element_type,
&element_last_field_id)
!= 0)
            {
                return -1;
            }
        }
        break;
    case COMPACT_TYPE_STRUCT:
        int16_t struct_last_field_id = 0;
        thrift_field_header_t nested_header;

        printf("  → STRUCT {\n");
        while (thrift_read_field_header(reader, &nested_header,
&struct_last_field_id) == 0 && nested_header.type != COMPACT_TYPE_STOP)
        {
            printf("    Field ID:%d Type:%d ", nested_header.field_id,
nested_header.type);

            if (thrift_skip_field(reader, nested_header.type,
&struct_last_field_id) != 0)
            {
                return -1;
            }
        }
        printf("  → } STRUCT END\n");
        break;
    case COMPACT_TYPE_SET:
    case COMPACT_TYPE_MAP:
        printf("  → SET/MAP (not implemented, skipping 1 byte)\n");
        reader->position += 1;
        break;
    default:
        printf("  → Unknown type %d at position %zu, skipping 1 byte\n",
field_type, reader->position); reader->position += 1; break;
    }
    return 0;
}

void thrift_print_hex(thrift_reader_t* reader, int bytes)
{
    printf("Position: %zu: ", reader->position);
    for (int i = 0; i < bytes && reader->position + i < reader->size; i++)
    {
        printf("%02x ", reader->data[reader->position + i]);
    }
    printf("\n");
}


// typedef enum
// {
//     COMPACT_TYPE_STOP = 0,
//     COMPACT_TYPE_BOOL_TRUE = 1,
//     COMPACT_TYPE_BOOL_FALSE = 2,
//     COMPACT_TYPE_BYTE = 3,
//     COMPACT_TYPE_I16 = 4,
//     COMPACT_TYPE_I32 = 5,
//     COMPACT_TYPE_I64 = 6,
//     COMPACT_TYPE_DOUBLE = 7,
//     COMPACT_TYPE_STRING = 8,
//     COMPACT_TYPE_LIST = 9,
//     COMPACT_TYPE_SET = 10,
//     COMPACT_TYPE_MAP = 11,
//     COMPACT_TYPE_STRUCT = 12
// } compact_type_t;
//

// int thrift_parse_value(thrift_reader_t* reader, thrift_value_t* value)
// {
//     switch (value->type)
//     {
//     case COMPACT_TYPE_BOOL_TRUE:
//         value->bool_val = 1;
//         break;
//     case COMPACT_TYPE_BOOL_FALSE:
//         value->bool_val = 0;
//         break;
//     case COMPACT_TYPE_BYTE:
//         value->byte_val = reader->data[reader->position++];
//         break;
//     case COMPACT_TYPE_I16:
//         uint32_t varint16;
//         if (thrift_read_varint32(reader, &varint16) != 0)
//             return -1;
//         value->i16_val = thrift_zigzag_decode32(varint16);
//         break;
//     case COMPACT_TYPE_I32:
//         uint32_t varint32;
//         if (thrift_read_varint32(reader, &varint32) != 0)
//             return -1;
//         value->i32_val = thrift_zigzag_decode32(varint32);
//         break;
//     case COMPACT_TYPE_I64:
//         uint64_t varint64;
//         if (thrift_read_varint64(reader, &varint64) != 0)
//             return -1;
//         value->i64_val = thrift_zigzag_decode64(varint64);
//         break;
//     case COMPACT_TYPE_DOUBLE:
//         reader->position += 8; // Skip 8 bytes for double
//         printf("Not implemented: COMPACT_TYPE_DOUBLE\n");
//         break;
//     case COMPACT_TYPE_STRING:
//         uint32_t len;
//         if (thrift_read_varint32(reader, &len) != 0)
//             return -1;
//
//         char* str = malloc(len + 1);
//         if (!str)
//             return -1; // Memory allocation failed
//         thrift_read_string(reader, str, len);
//         str[len] = '\0'; // Null-terminate the string
//         value->string_val.data = str;
//         value->string_val.len = len;
//         break;
//     case COMPACT_TYPE_LIST:
//         uint8_t list_header = reader->data[reader->position++];
//         uint8_t element_type = list_header & 0x0F;
//         uint8_t size_part = (list_header & 0xF0) >> 4;
//         uint32_t size;
//         if (size_part < 15)
//         {
//             size = size_part;
//         }
//         else
//         {
//             if (thrift_read_varint32(reader, &size) != 0)
//                 return -1;
//         }
//         value->list_val.count = size;
//         value->list_val.element_type = element_type;
//         value->list_val.elements = malloc(size * sizeof(thrift_value_t));
//         if (!value->list_val.elements)
//             return -1; // Memory allocation failed
//
//         thrift_read_list(reader, &value->list_val, element_type);
//         break;
//     }
// }

*/
