/* Generated by the protocol buffer compiler.  DO NOT EDIT! */
/* Generated from: models2.proto */

/* Do not generate deprecated warnings for self */
#ifndef PROTOBUF_C__NO_DEPRECATED
#define PROTOBUF_C__NO_DEPRECATED
#endif

#include "models2.pb-c.h"
void   fcws__para2__pca__init
                     (FCWS__Para2__Pca         *message)
{
  static const FCWS__Para2__Pca init_value = FCWS__PARA2__PCA__INIT;
  *message = init_value;
}
void   fcws__para2__ica__init
                     (FCWS__Para2__Ica         *message)
{
  static const FCWS__Para2__Ica init_value = FCWS__PARA2__ICA__INIT;
  *message = init_value;
}
void   fcws__para2__init
                     (FCWS__Para2         *message)
{
  static const FCWS__Para2 init_value = FCWS__PARA2__INIT;
  *message = init_value;
}
size_t fcws__para2__get_packed_size
                     (const FCWS__Para2 *message)
{
  assert(message->base.descriptor == &fcws__para2__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t fcws__para2__pack
                     (const FCWS__Para2 *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &fcws__para2__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t fcws__para2__pack_to_buffer
                     (const FCWS__Para2 *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &fcws__para2__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
FCWS__Para2 *
       fcws__para2__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (FCWS__Para2 *)
     protobuf_c_message_unpack (&fcws__para2__descriptor,
                                allocator, len, data);
}
void   fcws__para2__free_unpacked
                     (FCWS__Para2 *message,
                      ProtobufCAllocator *allocator)
{
  if(!message)
    return;
  assert(message->base.descriptor == &fcws__para2__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
void   fcws__para__init
                     (FCWS__Para         *message)
{
  static const FCWS__Para init_value = FCWS__PARA__INIT;
  *message = init_value;
}
size_t fcws__para__get_packed_size
                     (const FCWS__Para *message)
{
  assert(message->base.descriptor == &fcws__para__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t fcws__para__pack
                     (const FCWS__Para *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &fcws__para__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t fcws__para__pack_to_buffer
                     (const FCWS__Para *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &fcws__para__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
FCWS__Para *
       fcws__para__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (FCWS__Para *)
     protobuf_c_message_unpack (&fcws__para__descriptor,
                                allocator, len, data);
}
void   fcws__para__free_unpacked
                     (FCWS__Para *message,
                      ProtobufCAllocator *allocator)
{
  if(!message)
    return;
  assert(message->base.descriptor == &fcws__para__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
void   fcws__local__init
                     (FCWS__Local         *message)
{
  static const FCWS__Local init_value = FCWS__LOCAL__INIT;
  *message = init_value;
}
size_t fcws__local__get_packed_size
                     (const FCWS__Local *message)
{
  assert(message->base.descriptor == &fcws__local__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t fcws__local__pack
                     (const FCWS__Local *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &fcws__local__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t fcws__local__pack_to_buffer
                     (const FCWS__Local *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &fcws__local__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
FCWS__Local *
       fcws__local__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (FCWS__Local *)
     protobuf_c_message_unpack (&fcws__local__descriptor,
                                allocator, len, data);
}
void   fcws__local__free_unpacked
                     (FCWS__Local *message,
                      ProtobufCAllocator *allocator)
{
  if(!message)
    return;
  assert(message->base.descriptor == &fcws__local__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
void   fcws__vehicle__model__init
                     (FCWS__VehicleModel         *message)
{
  static const FCWS__VehicleModel init_value = FCWS__VEHICLE__MODEL__INIT;
  *message = init_value;
}
size_t fcws__vehicle__model__get_packed_size
                     (const FCWS__VehicleModel *message)
{
  assert(message->base.descriptor == &fcws__vehicle__model__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t fcws__vehicle__model__pack
                     (const FCWS__VehicleModel *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &fcws__vehicle__model__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t fcws__vehicle__model__pack_to_buffer
                     (const FCWS__VehicleModel *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &fcws__vehicle__model__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
FCWS__VehicleModel *
       fcws__vehicle__model__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (FCWS__VehicleModel *)
     protobuf_c_message_unpack (&fcws__vehicle__model__descriptor,
                                allocator, len, data);
}
void   fcws__vehicle__model__free_unpacked
                     (FCWS__VehicleModel *message,
                      ProtobufCAllocator *allocator)
{
  if(!message)
    return;
  assert(message->base.descriptor == &fcws__vehicle__model__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
void   fcws__models__init
                     (FCWS__Models         *message)
{
  static const FCWS__Models init_value = FCWS__MODELS__INIT;
  *message = init_value;
}
size_t fcws__models__get_packed_size
                     (const FCWS__Models *message)
{
  assert(message->base.descriptor == &fcws__models__descriptor);
  return protobuf_c_message_get_packed_size ((const ProtobufCMessage*)(message));
}
size_t fcws__models__pack
                     (const FCWS__Models *message,
                      uint8_t       *out)
{
  assert(message->base.descriptor == &fcws__models__descriptor);
  return protobuf_c_message_pack ((const ProtobufCMessage*)message, out);
}
size_t fcws__models__pack_to_buffer
                     (const FCWS__Models *message,
                      ProtobufCBuffer *buffer)
{
  assert(message->base.descriptor == &fcws__models__descriptor);
  return protobuf_c_message_pack_to_buffer ((const ProtobufCMessage*)message, buffer);
}
FCWS__Models *
       fcws__models__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data)
{
  return (FCWS__Models *)
     protobuf_c_message_unpack (&fcws__models__descriptor,
                                allocator, len, data);
}
void   fcws__models__free_unpacked
                     (FCWS__Models *message,
                      ProtobufCAllocator *allocator)
{
  if(!message)
    return;
  assert(message->base.descriptor == &fcws__models__descriptor);
  protobuf_c_message_free_unpacked ((ProtobufCMessage*)message, allocator);
}
static const ProtobufCFieldDescriptor fcws__para2__pca__field_descriptors[7] =
{
  {
    "mean_size",
    1,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_INT32,
    0,   /* quantifier_offset */
    offsetof(FCWS__Para2__Pca, mean_size),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "mean_data",
    2,
    PROTOBUF_C_LABEL_REPEATED,
    PROTOBUF_C_TYPE_DOUBLE,
    offsetof(FCWS__Para2__Pca, n_mean_data),
    offsetof(FCWS__Para2__Pca, mean_data),
    NULL,
    NULL,
    0 | PROTOBUF_C_FIELD_FLAG_PACKED,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "eval_size",
    3,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_INT32,
    0,   /* quantifier_offset */
    offsetof(FCWS__Para2__Pca, eval_size),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "eval_data",
    4,
    PROTOBUF_C_LABEL_REPEATED,
    PROTOBUF_C_TYPE_DOUBLE,
    offsetof(FCWS__Para2__Pca, n_eval_data),
    offsetof(FCWS__Para2__Pca, eval_data),
    NULL,
    NULL,
    0 | PROTOBUF_C_FIELD_FLAG_PACKED,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "evec_size1",
    5,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_INT32,
    0,   /* quantifier_offset */
    offsetof(FCWS__Para2__Pca, evec_size1),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "evec_size2",
    6,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_INT32,
    0,   /* quantifier_offset */
    offsetof(FCWS__Para2__Pca, evec_size2),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "evec_data",
    7,
    PROTOBUF_C_LABEL_REPEATED,
    PROTOBUF_C_TYPE_DOUBLE,
    offsetof(FCWS__Para2__Pca, n_evec_data),
    offsetof(FCWS__Para2__Pca, evec_data),
    NULL,
    NULL,
    0 | PROTOBUF_C_FIELD_FLAG_PACKED,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned fcws__para2__pca__field_indices_by_name[] = {
  3,   /* field[3] = eval_data */
  2,   /* field[2] = eval_size */
  6,   /* field[6] = evec_data */
  4,   /* field[4] = evec_size1 */
  5,   /* field[5] = evec_size2 */
  1,   /* field[1] = mean_data */
  0,   /* field[0] = mean_size */
};
static const ProtobufCIntRange fcws__para2__pca__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 7 }
};
const ProtobufCMessageDescriptor fcws__para2__pca__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "FCWS.Para2.Pca",
  "Pca",
  "FCWS__Para2__Pca",
  "FCWS",
  sizeof(FCWS__Para2__Pca),
  7,
  fcws__para2__pca__field_descriptors,
  fcws__para2__pca__field_indices_by_name,
  1,  fcws__para2__pca__number_ranges,
  (ProtobufCMessageInit) fcws__para2__pca__init,
  NULL,NULL,NULL    /* reserved[123] */
};
static const ProtobufCFieldDescriptor fcws__para2__ica__field_descriptors[1] =
{
  {
    "test",
    1,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_INT32,
    0,   /* quantifier_offset */
    offsetof(FCWS__Para2__Ica, test),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned fcws__para2__ica__field_indices_by_name[] = {
  0,   /* field[0] = test */
};
static const ProtobufCIntRange fcws__para2__ica__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 1 }
};
const ProtobufCMessageDescriptor fcws__para2__ica__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "FCWS.Para2.Ica",
  "Ica",
  "FCWS__Para2__Ica",
  "FCWS",
  sizeof(FCWS__Para2__Ica),
  1,
  fcws__para2__ica__field_descriptors,
  fcws__para2__ica__field_indices_by_name,
  1,  fcws__para2__ica__number_ranges,
  (ProtobufCMessageInit) fcws__para2__ica__init,
  NULL,NULL,NULL    /* reserved[123] */
};
static const ProtobufCFieldDescriptor fcws__para2__field_descriptors[3] =
{
  {
    "pca",
    1,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_MESSAGE,
    0,   /* quantifier_offset */
    offsetof(FCWS__Para2, pca),
    &fcws__para2__pca__descriptor,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "pca2",
    2,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_MESSAGE,
    0,   /* quantifier_offset */
    offsetof(FCWS__Para2, pca2),
    &fcws__para2__pca__descriptor,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "ica",
    3,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_MESSAGE,
    0,   /* quantifier_offset */
    offsetof(FCWS__Para2, ica),
    &fcws__para2__ica__descriptor,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned fcws__para2__field_indices_by_name[] = {
  2,   /* field[2] = ica */
  0,   /* field[0] = pca */
  1,   /* field[1] = pca2 */
};
static const ProtobufCIntRange fcws__para2__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 3 }
};
const ProtobufCMessageDescriptor fcws__para2__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "FCWS.Para2",
  "Para2",
  "FCWS__Para2",
  "FCWS",
  sizeof(FCWS__Para2),
  3,
  fcws__para2__field_descriptors,
  fcws__para2__field_indices_by_name,
  1,  fcws__para2__number_ranges,
  (ProtobufCMessageInit) fcws__para2__init,
  NULL,NULL,NULL    /* reserved[123] */
};
static const ProtobufCEnumValue fcws__para__type__enum_values_by_number[5] =
{
  { "UnKonwn", "FCWS__PARA__TYPE__UnKonwn", -1 },
  { "PCA", "FCWS__PARA__TYPE__PCA", 0 },
  { "PCA2", "FCWS__PARA__TYPE__PCA2", 1 },
  { "ICA", "FCWS__PARA__TYPE__ICA", 2 },
  { "TOTAL", "FCWS__PARA__TYPE__TOTAL", 3 },
};
static const ProtobufCIntRange fcws__para__type__value_ranges[] = {
{-1, 0},{0, 5}
};
static const ProtobufCEnumValueIndex fcws__para__type__enum_values_by_name[5] =
{
  { "ICA", 3 },
  { "PCA", 1 },
  { "PCA2", 2 },
  { "TOTAL", 4 },
  { "UnKonwn", 0 },
};
const ProtobufCEnumDescriptor fcws__para__type__descriptor =
{
  PROTOBUF_C__ENUM_DESCRIPTOR_MAGIC,
  "FCWS.Para.Type",
  "Type",
  "FCWS__Para__Type",
  "FCWS",
  5,
  fcws__para__type__enum_values_by_number,
  5,
  fcws__para__type__enum_values_by_name,
  1,
  fcws__para__type__value_ranges,
  NULL,NULL,NULL,NULL   /* reserved[1234] */
};
static const FCWS__Para__Type fcws__para__type__default_value = FCWS__PARA__TYPE__UnKonwn;
static const ProtobufCFieldDescriptor fcws__para__field_descriptors[4] =
{
  {
    "type",
    1,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_ENUM,
    0,   /* quantifier_offset */
    offsetof(FCWS__Para, type),
    &fcws__para__type__descriptor,
    &fcws__para__type__default_value,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "rows",
    2,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_INT32,
    0,   /* quantifier_offset */
    offsetof(FCWS__Para, rows),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "cols",
    3,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_INT32,
    0,   /* quantifier_offset */
    offsetof(FCWS__Para, cols),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "data",
    4,
    PROTOBUF_C_LABEL_REPEATED,
    PROTOBUF_C_TYPE_DOUBLE,
    offsetof(FCWS__Para, n_data),
    offsetof(FCWS__Para, data),
    NULL,
    NULL,
    0 | PROTOBUF_C_FIELD_FLAG_PACKED,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned fcws__para__field_indices_by_name[] = {
  2,   /* field[2] = cols */
  3,   /* field[3] = data */
  1,   /* field[1] = rows */
  0,   /* field[0] = type */
};
static const ProtobufCIntRange fcws__para__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 4 }
};
const ProtobufCMessageDescriptor fcws__para__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "FCWS.Para",
  "Para",
  "FCWS__Para",
  "FCWS",
  sizeof(FCWS__Para),
  4,
  fcws__para__field_descriptors,
  fcws__para__field_indices_by_name,
  1,  fcws__para__number_ranges,
  (ProtobufCMessageInit) fcws__para__init,
  NULL,NULL,NULL    /* reserved[123] */
};
static const ProtobufCEnumValue fcws__local__type__enum_values_by_number[6] =
{
  { "UnKonwn", "FCWS__LOCAL__TYPE__UnKonwn", -1 },
  { "LEFT", "FCWS__LOCAL__TYPE__LEFT", 0 },
  { "RIGHT", "FCWS__LOCAL__TYPE__RIGHT", 1 },
  { "CENTER", "FCWS__LOCAL__TYPE__CENTER", 2 },
  { "GARBAGE", "FCWS__LOCAL__TYPE__GARBAGE", 3 },
  { "TOTAL", "FCWS__LOCAL__TYPE__TOTAL", 4 },
};
static const ProtobufCIntRange fcws__local__type__value_ranges[] = {
{-1, 0},{0, 6}
};
static const ProtobufCEnumValueIndex fcws__local__type__enum_values_by_name[6] =
{
  { "CENTER", 3 },
  { "GARBAGE", 4 },
  { "LEFT", 1 },
  { "RIGHT", 2 },
  { "TOTAL", 5 },
  { "UnKonwn", 0 },
};
const ProtobufCEnumDescriptor fcws__local__type__descriptor =
{
  PROTOBUF_C__ENUM_DESCRIPTOR_MAGIC,
  "FCWS.Local.Type",
  "Type",
  "FCWS__Local__Type",
  "FCWS",
  6,
  fcws__local__type__enum_values_by_number,
  6,
  fcws__local__type__enum_values_by_name,
  1,
  fcws__local__type__value_ranges,
  NULL,NULL,NULL,NULL   /* reserved[1234] */
};
static const FCWS__Local__Type fcws__local__local__default_value = FCWS__LOCAL__TYPE__UnKonwn;
static const ProtobufCFieldDescriptor fcws__local__field_descriptors[6] =
{
  {
    "local",
    1,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_ENUM,
    0,   /* quantifier_offset */
    offsetof(FCWS__Local, local),
    &fcws__local__type__descriptor,
    &fcws__local__local__default_value,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "para",
    2,
    PROTOBUF_C_LABEL_REPEATED,
    PROTOBUF_C_TYPE_MESSAGE,
    offsetof(FCWS__Local, n_para),
    offsetof(FCWS__Local, para),
    &fcws__para__descriptor,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "para2",
    3,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_MESSAGE,
    0,   /* quantifier_offset */
    offsetof(FCWS__Local, para2),
    &fcws__para2__descriptor,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "sw_w",
    4,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_INT32,
    0,   /* quantifier_offset */
    offsetof(FCWS__Local, sw_w),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "sw_h",
    5,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_INT32,
    0,   /* quantifier_offset */
    offsetof(FCWS__Local, sw_h),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "need_thread",
    6,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_BOOL,
    0,   /* quantifier_offset */
    offsetof(FCWS__Local, need_thread),
    NULL,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned fcws__local__field_indices_by_name[] = {
  0,   /* field[0] = local */
  5,   /* field[5] = need_thread */
  1,   /* field[1] = para */
  2,   /* field[2] = para2 */
  4,   /* field[4] = sw_h */
  3,   /* field[3] = sw_w */
};
static const ProtobufCIntRange fcws__local__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 6 }
};
const ProtobufCMessageDescriptor fcws__local__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "FCWS.Local",
  "Local",
  "FCWS__Local",
  "FCWS",
  sizeof(FCWS__Local),
  6,
  fcws__local__field_descriptors,
  fcws__local__field_indices_by_name,
  1,  fcws__local__number_ranges,
  (ProtobufCMessageInit) fcws__local__init,
  NULL,NULL,NULL    /* reserved[123] */
};
static const ProtobufCEnumValue fcws__vehicle__model__type__enum_values_by_number[10] =
{
  { "UnKonwn", "FCWS__VEHICLE__MODEL__TYPE__UnKonwn", -1 },
  { "Compact", "FCWS__VEHICLE__MODEL__TYPE__Compact", 0 },
  { "Midsize", "FCWS__VEHICLE__MODEL__TYPE__Midsize", 1 },
  { "Large", "FCWS__VEHICLE__MODEL__TYPE__Large", 2 },
  { "SUV", "FCWS__VEHICLE__MODEL__TYPE__SUV", 3 },
  { "BUS", "FCWS__VEHICLE__MODEL__TYPE__BUS", 4 },
  { "TRUCK", "FCWS__VEHICLE__MODEL__TYPE__TRUCK", 5 },
  { "MOTOCYCLE", "FCWS__VEHICLE__MODEL__TYPE__MOTOCYCLE", 6 },
  { "BYCYCLE", "FCWS__VEHICLE__MODEL__TYPE__BYCYCLE", 7 },
  { "TOTAL", "FCWS__VEHICLE__MODEL__TYPE__TOTAL", 8 },
};
static const ProtobufCIntRange fcws__vehicle__model__type__value_ranges[] = {
{-1, 0},{0, 10}
};
static const ProtobufCEnumValueIndex fcws__vehicle__model__type__enum_values_by_name[10] =
{
  { "BUS", 5 },
  { "BYCYCLE", 8 },
  { "Compact", 1 },
  { "Large", 3 },
  { "MOTOCYCLE", 7 },
  { "Midsize", 2 },
  { "SUV", 4 },
  { "TOTAL", 9 },
  { "TRUCK", 6 },
  { "UnKonwn", 0 },
};
const ProtobufCEnumDescriptor fcws__vehicle__model__type__descriptor =
{
  PROTOBUF_C__ENUM_DESCRIPTOR_MAGIC,
  "FCWS.Vehicle_Model.Type",
  "Type",
  "FCWS__VehicleModel__Type",
  "FCWS",
  10,
  fcws__vehicle__model__type__enum_values_by_number,
  10,
  fcws__vehicle__model__type__enum_values_by_name,
  1,
  fcws__vehicle__model__type__value_ranges,
  NULL,NULL,NULL,NULL   /* reserved[1234] */
};
static const FCWS__VehicleModel__Type fcws__vehicle__model__type__default_value = FCWS__VEHICLE__MODEL__TYPE__UnKonwn;
static const ProtobufCFieldDescriptor fcws__vehicle__model__field_descriptors[2] =
{
  {
    "type",
    1,
    PROTOBUF_C_LABEL_REQUIRED,
    PROTOBUF_C_TYPE_ENUM,
    0,   /* quantifier_offset */
    offsetof(FCWS__VehicleModel, type),
    &fcws__vehicle__model__type__descriptor,
    &fcws__vehicle__model__type__default_value,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
  {
    "local",
    2,
    PROTOBUF_C_LABEL_REPEATED,
    PROTOBUF_C_TYPE_MESSAGE,
    offsetof(FCWS__VehicleModel, n_local),
    offsetof(FCWS__VehicleModel, local),
    &fcws__local__descriptor,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned fcws__vehicle__model__field_indices_by_name[] = {
  1,   /* field[1] = local */
  0,   /* field[0] = type */
};
static const ProtobufCIntRange fcws__vehicle__model__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 2 }
};
const ProtobufCMessageDescriptor fcws__vehicle__model__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "FCWS.Vehicle_Model",
  "VehicleModel",
  "FCWS__VehicleModel",
  "FCWS",
  sizeof(FCWS__VehicleModel),
  2,
  fcws__vehicle__model__field_descriptors,
  fcws__vehicle__model__field_indices_by_name,
  1,  fcws__vehicle__model__number_ranges,
  (ProtobufCMessageInit) fcws__vehicle__model__init,
  NULL,NULL,NULL    /* reserved[123] */
};
static const ProtobufCFieldDescriptor fcws__models__field_descriptors[1] =
{
  {
    "vm",
    1,
    PROTOBUF_C_LABEL_REPEATED,
    PROTOBUF_C_TYPE_MESSAGE,
    offsetof(FCWS__Models, n_vm),
    offsetof(FCWS__Models, vm),
    &fcws__vehicle__model__descriptor,
    NULL,
    0,             /* flags */
    0,NULL,NULL    /* reserved1,reserved2, etc */
  },
};
static const unsigned fcws__models__field_indices_by_name[] = {
  0,   /* field[0] = vm */
};
static const ProtobufCIntRange fcws__models__number_ranges[1 + 1] =
{
  { 1, 0 },
  { 0, 1 }
};
const ProtobufCMessageDescriptor fcws__models__descriptor =
{
  PROTOBUF_C__MESSAGE_DESCRIPTOR_MAGIC,
  "FCWS.Models",
  "Models",
  "FCWS__Models",
  "FCWS",
  sizeof(FCWS__Models),
  1,
  fcws__models__field_descriptors,
  fcws__models__field_indices_by_name,
  1,  fcws__models__number_ranges,
  (ProtobufCMessageInit) fcws__models__init,
  NULL,NULL,NULL    /* reserved[123] */
};
