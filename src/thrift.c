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

int thrift_read_list(thrift_reader_t* reader, thrift_list_t* list_val) {
  if (reader->position >= reader->size) return -1;  // Out of bounds

  uint8_t first_byte = reader->data[reader->position++];
  list_val->type = first_byte & 0x0F;
  uint8_t size_part = (first_byte & 0xF0) >> 4;
  if (size_part < 15) {
    list_val->count = size_part;
  } else {
    if (thrift_read_varint32(reader, &list_val->count) != 0)
      return -1;  // Failed to read size
  }

  list_val->elements = malloc(list_val->count * sizeof(thrift_data_t));
  if (!list_val->elements) return -1;  // Memory allocation failed

  for (uint32_t i = 0; i < list_val->count; i++) {
    thrift_field_t field;
    field.type = list_val->type;
    field.field_id = i;  // Assign a field ID for the list element
    if (thrift_read_field(reader, &field) != 0) {
      free(list_val->elements);
      return -1;  // Failed to read list element
    }
    list_val->elements[i] = field.value;
  }
  return 0;  // Successfully read list
}

int thrift_read_struct(thrift_reader_t* reader, thrift_struct_t* struct_val) {
  struct_val->field_count = 0;
  struct_val->fields =
      malloc(10 * sizeof(thrift_field_t*));  // Initial capacity
  struct_val->fields[0] = malloc(sizeof(thrift_field_t));

  int16_t last_field_id = 0;
  while (thrift_read_field_header(reader,
                                  struct_val->fields[struct_val->field_count],
                                  &last_field_id) == 0 &&
         struct_val->fields[struct_val->field_count]->type !=
             COMPACT_TYPE_STOP) {
    if (thrift_read_field(reader,
                          struct_val->fields[struct_val->field_count]) != 0) {
      free(struct_val->fields);
      return -1;  // Failed to read field
    }
    struct_val->field_count++;
    if (struct_val->field_count >= 10) {  // Resize if needed
      struct_val->fields =
          realloc(struct_val->fields,
                  (struct_val->field_count + 10) * sizeof(thrift_field_t*));
      if (!struct_val->fields) return -1;  // Memory allocation failed
    }
    struct_val->fields[struct_val->field_count] =
        malloc(sizeof(thrift_field_t));
  }
  return 0;  // Successfully read root struct
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
    field->field_id = reader->data[reader->position] |
                      (reader->data[reader->position + 1] << 8);
    reader->position += 2;
  } else {
    field->field_id = *last_field_id + delta;
  }

  *last_field_id = field->field_id;
  return 0;
}

int thrift_read_field(thrift_reader_t* reader, thrift_field_t* field) {
  if (reader->position >= reader->size) return -1;  // Out of bounds
  field->value = malloc(sizeof(thrift_data_t));
  if (!field->value) return -1;  // Memory allocation failed

  switch (field->type) {
    case COMPACT_TYPE_STOP:
      break;
    case COMPACT_TYPE_I32:
      if (thrift_read_zigzag32(reader, &field->value->i32_val) != 0) {
        free(field->value);
        return -1;  // Failed to read I32 value
      }
      break;
    case COMPACT_TYPE_I64:
      if (thrift_read_zigzag64(reader, &field->value->i64_val) != 0) {
        free(field->value);
        return -1;  // Failed to read I64 value
      }
      break;
    case COMPACT_TYPE_STRING:
      if (thrift_read_string(reader, &field->value->binary_val) != 0) {
        free(field->value);
        return -1;  // Failed to read string value
      }
      break;
    case COMPACT_TYPE_LIST:
      if (thrift_read_list(reader, &field->value->list_val) != 0) {
        free(field->value);
        return -1;  // Failed to read list
      }
      break;
    case COMPACT_TYPE_STRUCT:
      if (thrift_read_struct(reader, &field->value->struct_val) != 0) {
        free(field->value);
        return -1;  // Failed to read struct
      }
      break;
    default:
      printf("Unknown type %d at position %zu, field_id: %d, skipping 1 byte\n",
             field->type, reader->position, field->field_id);
      return -1;  // Unknown type, skip 1 byte
  }

  return 0;
}

thrift_data_t* thrift_get_field_value(thrift_struct_t* struct_val,
                                      uint16_t field_id) {
  for (size_t i = 0; i < struct_val->field_count; i++) {
    if (struct_val->fields[i]->field_id == field_id) {
      return struct_val->fields[i]->value;
    }
  }
  return NULL;  // Field not found
}

void thrift_print_struct(const thrift_struct_t* struct_val, int indent) {
  if (!struct_val) return;
  for (int i = 0; i < indent; i++) printf("  ");
  printf("Thrift Struct with %u fields:\n", struct_val->field_count);

  for (size_t i = 0; i < struct_val->field_count; i++) {
    const thrift_field_t* field = struct_val->fields[i];
    for (int j = 0; j < indent; j++) printf("  ");
    printf("Field ID: %d, Type: %d, ", field->field_id, field->type);
    switch (field->type) {
      case COMPACT_TYPE_I32:
        printf("Value: %d\n", field->value->i32_val);
        break;
      case COMPACT_TYPE_I64:
        printf("Value: %lld\n", field->value->i64_val);
        break;
      case COMPACT_TYPE_STRING:
        printf("Value: '%s'\n", field->value->binary_val.data);
        break;
      case COMPACT_TYPE_LIST:
        printf("List with %u elements of type %d\n",
               field->value->list_val.count, field->value->list_val.type);
        for (uint32_t j = 0; j < field->value->list_val.count; j++) {
          if (field->value->list_val.type == COMPACT_TYPE_STRUCT)
            thrift_print_struct(&field->value->list_val.elements[j]->struct_val,
                                indent + 1);
        }
        break;
      case COMPACT_TYPE_STRUCT:
        printf("Struct with %u fields\n", field->value->struct_val.field_count);
        thrift_print_struct(&field->value->struct_val, indent + 1);
        break;
      default:
        printf("Unknown type\n");
        break;
    }
  }
}

void thrift_print_root(const thrift_struct_t* root) {
  if (!root) return;
  printf("Thrift Root Struct:\n");
  thrift_print_struct(root, 1);
}
