#include <stdlib.h>

#include "raft_wire.h"
#include "raft_util.h"
#include "raft_log.h"

void raft_dealloc_envelope(raft_envelope_t* p_envelope) {
  free(p_envelope->p_message);
  memset(p_envelope, 0, sizeof(*p_envelope));
}

/**
 * The following describes the wire format for raft messages.
 * All multibyte values are encoded in network (big-endian) order.
 *
 * Bytes | Semantics
 * =============================================================================
 *   0-2 | Magic Number                   -| Message
 *     3 | Message Type                    | Header
 *   4-7 | Message Size (from byte 0)     -|
 * - - - - - - - - - - - - - - - - - - - - -
 *   8-* | Constant-length message fields -|
 *       | (Type-dependent)               -|
 * ========================================
 *       | Entry 0 Metadata               -|
 * - - - - - - - - - - - - - - - - - - - - | Log entry metadata
 *       | Entry 1 Metadata                | 8-byte entries
 * - - - - - - - - - - - - - - - - - - - - | (entry type, size,
 *         ...                             |  and unique id)
 * - - - - - - - - - - - - - - - - - - - - |
 *       | Entry n Metadata                |
 * ========================================|
 *       | Entry 0 data                    | Variable-length
 *       |     ...                         | log entry data
 * - - - - - - - - - - - - - - - - - - - - |
 *       | Entry 1 data                    |
 *       |     ...                         |
 * - - - - - - - - - - - - - - - - - - - - |
 *       | Entry n data                    |
 *       |     ...                        -|
 * =============================================================================
 */

#define RAFT_MSG_HEADER_SIZE 8

#define RAFT_MSG_VERSION 0x000001
#define RAFT_MSG_VERSION_BYTE(idx) RAFT_LSBYTE(RAFT_MSG_VERSION, idx)

#define MESSAGE_BUFFER_SIZE_ALIGN 0x100

static uint8_t* write(uint8_t* p_b, uint32_t v) {
  (*p_b++) = (v >> 24) & 0xfF;
  (*p_b++) = (v >> 16) & 0xfF;
  (*p_b++) = (v >> 8)  & 0xfF;
  (*p_b++) = (v >> 0)  & 0xfF;
  return p_b;
}

static uint8_t const* read(uint32_t* p_v, uint8_t const* p_b) {
  *p_v = 0;
  *p_v |= (*p_b++) << 24;
  *p_v |= (*p_b++) << 16;
  *p_v |= (*p_b++) <<  8;
  *p_v |= (*p_b++) <<  0;
  return p_b;
}

static uint32_t a_message_sizes[] = {
  0, /* UNKNOWN */
  32, /* MSG_TYPE_APPEND_ENTRIES */
  0, /* UNKNOWN */
  24, /* MSG_TYPE_REQUEST_VOTE */
  20, /* MSG_TYPE_REQUEST_VOTE_RESPONSE */
};

#define MESSAGE_SIZE(_type) a_message_sizes[(_type)]

/* WM: write to message */
#define WM_SETUP(_type, _dynamic_data_size)                             \
  uint8_t* p_buf = p_env->p_message;                                    \
  do {                                                                  \
    p_env->recipient_id = recipient_id;                                 \
    if (p_buf == NULL ||                                                \
        p_env->buffer_capacity < MESSAGE_SIZE(_type)) {                 \
      uint32_t capacity = RAFT_ALIGN_UP(MESSAGE_SIZE(_type),            \
                                        MESSAGE_BUFFER_SIZE_ALIGN);     \
      p_env->p_message = p_buf = realloc(p_buf, capacity);              \
      if (p_buf == NULL)                                                \
        return RAFT_STATUS_OUT_OF_MEMORY;                               \
      p_env->buffer_capacity = capacity;                                \
    }                                                                   \
    p_env->message_size = MESSAGE_SIZE(_type) + (_dynamic_data_size);   \
    (*p_buf++) = RAFT_MSG_VERSION_BYTE(2);                              \
    (*p_buf++) = RAFT_MSG_VERSION_BYTE(1);                              \
    (*p_buf++) = RAFT_MSG_VERSION_BYTE(0);                              \
    (*p_buf++) = _type;                                                 \
    (*p_buf++) = RAFT_LSBYTE(p_env->message_size, 3);                   \
    (*p_buf++) = RAFT_LSBYTE(p_env->message_size, 2);                   \
    (*p_buf++) = RAFT_LSBYTE(p_env->message_size, 1);                   \
    (*p_buf++) = RAFT_LSBYTE(p_env->message_size, 0);                   \
  } while (0)

#define WM_IMMU32(_value)                                               \
  do {                                                                  \
    RAFT_ASSERT(p_buf - p_env->p_message <= p_env->message_size - 4);   \
    p_buf = write(p_buf, (_value));                                     \
  } while (0)

#define WM(_member_name)                                                \
  do {                                                                  \
    uint8_t _unused[sizeof(p_args->_member_name) == sizeof(uint32_t) ?  \
                    1 : -1];                                            \
    (void)_unused;                                                      \
    WM_IMMU32(p_args->_member_name);                                    \
  } while (0)

#define WM_BOOL(_member_name)                                           \
  do {                                                                  \
    uint8_t _unused[sizeof(p_args->_member_name) ==                     \
                    sizeof(raft_bool_t) ? 1 : -1];                      \
    (void)_unused;                                                      \
    uint32_t v = (p_args->_member_name) ? 1 : 0;                        \
    WM_IMMU32(v);                                                       \
  } while (0)

#define WM_BYTES(_p_bytes, _size)                                       \
  do {                                                                  \
    RAFT_ASSERT(p_buf - p_env->p_message <=                             \
                p_env->message_size - (_size));                         \
    memcpy(p_buf, (_p_bytes), (_size));                                 \
    p_buf = p_buf + (_size);                                            \
  } while (0)

/* RM: Read from message */
#define RM_SETUP uint8_t const* p_buf = p_message_bytes + RAFT_MSG_HEADER_SIZE

#define RM_U32(_p_value)                                                \
  do {                                                                  \
    if (message_size < 4) {                                             \
      return RAFT_STATUS_INVALID_MESSAGE;                               \
    }                                                                   \
    p_buf = read((_p_value), p_buf);                                    \
    message_size -= 4;                                                  \
  } while (0)

#define RM(_member_name)                                                \
  do {                                                                  \
    if (message_size <= 3) {                                            \
      return RAFT_STATUS_INVALID_MESSAGE;                               \
    }                                                                   \
    p_buf = read(&p_args->_member_name, p_buf);                         \
    message_size -= 4;                                                  \
  } while (0)

#define RM_BOOL(_member_name)                                           \
  do {                                                                  \
    if (message_size <= 3) {                                            \
      return RAFT_STATUS_INVALID_MESSAGE;                               \
    }                                                                   \
    uint32_t v;                                                         \
    p_buf = read(&v, p_buf);                                            \
    p_args->_member_name = v ? RAFT_TRUE : RAFT_FALSE;                  \
    message_size -= 4;                                                  \
  } while (0)

#define RM_BYTES(_p_bytes, _size)                                       \
  do {                                                                  \
    RAFT_ASSERT(message_size >= (_size));                               \
    memcpy((_p_bytes), p_buf, (_size));                                 \
    p_buf = p_buf + (_size);                                            \
  } while (0)


/*******************************************************************************
 *******************************************************************************
 ******************************************************************************/

static uint32_t raft_log_message_byte_count(raft_log_entry_t const* p_entries,
                                            uint32_t num_entries) {
  if (p_entries == NULL)
    return 0;

  uint32_t result = 0;
  for (uint32_t ii = 0; ii < num_entries; ++ii) {
    result += 12 + p_entries[ii].data_size;
  }
  return result;
}

raft_status_t raft_write_append_entries_envelope(
    raft_envelope_t* p_env,
    raft_nodeid_t recipient_id,
    raft_append_entries_args_t const* p_args) {
  raft_log_entry_t const* p_entries = p_args->p_log_entries;
  uint32_t const num_entries = p_args->num_entries;

  WM_SETUP(MSG_TYPE_APPEND_ENTRIES, raft_log_message_byte_count(p_entries,
                                                                num_entries));
  WM(term);
  WM(leader_id);
  WM(prev_log_index);
  WM(prev_log_term);
  WM_IMMU32(num_entries);
  WM(leader_commit);

  // TODO: Defer to the client about how to marshall the log entry data.
  //       For now, just copy the bytes directly.
  for (uint32_t ii = 0; ii < num_entries; ++ii) {
    raft_log_entry_t const* p_entry = &p_entries[ii];
    uint32_t size_and_type = p_entry->data_size;
    size_and_type |= p_entry->type << 31;
    WM_IMMU32(size_and_type);
    WM_IMMU32(p_entry->unique_id);
    WM_IMMU32(p_entry->term);
  }

  for (uint32_t ii = 0; ii < num_entries; ++ii) {
    raft_log_entry_t const* p_entry = &p_entries[ii];
    WM_BYTES(p_entry->p_data, p_entry->data_size);
  }

  return RAFT_STATUS_OK;
}

raft_status_t raft_write_request_vote_envelope(
    raft_envelope_t* p_env,
    raft_nodeid_t recipient_id,
    raft_request_vote_args_t const* p_args) {
  WM_SETUP(MSG_TYPE_REQUEST_VOTE, 0);
  WM(term);
  WM(candidate_id);
  WM(last_log_index);
  WM(last_log_term);

  return RAFT_STATUS_OK;
}

raft_status_t raft_write_request_vote_response_envelope(
    raft_envelope_t* p_env,
    raft_nodeid_t recipient_id,
    raft_request_vote_response_args_t const* p_args) {
  WM_SETUP(MSG_TYPE_REQUEST_VOTE_RESPONSE, 0);
  WM(follower_id);
  WM(term);
  WM_BOOL(vote_granted);

  return RAFT_STATUS_OK;
}

/*******************************************************************************
 *******************************************************************************
 ******************************************************************************/

raft_message_type_t raft_message_type(void* p_message_bytes) {
  uint32_t v;
  read(&v, p_message_bytes);
  v &= 0xff;
  if (v >= 1 && v <= MSG_TYPE_REQUEST_VOTE_RESPONSE) {
    return v;
  }

  // TODO: log bad input

  return 0;
}

raft_status_t raft_read_append_entries_args(raft_append_entries_args_t* p_args,
                                            void* p_message_bytes,
                                            uint32_t message_size) {
  uint32_t num_entries = 0;

  RM_SETUP;
  RM(term);
  RM(leader_id);
  RM(prev_log_index);
  RM(prev_log_term);
  RM_U32(&num_entries);
  RM(leader_commit);

  raft_log_entry_t* p_entries = calloc(num_entries, sizeof(raft_log_entry_t));
  if (p_entries == NULL) {
    goto fail_oom;
  }

  for (uint32_t ii = 0; ii < num_entries; ++ii) {
    uint32_t size_and_type;
    RM_U32(&size_and_type);
    RM_U32(&p_entries[ii].unique_id);
    RM_U32(&p_entries[ii].term);
    p_entries[ii].type = size_and_type >> 31;
    uint32_t data_size = p_entries[ii].data_size = size_and_type & 0x7fffffff;
    if (data_size) {
      p_entries[ii].p_data = malloc(p_entries[ii].data_size);
      if (p_entries[ii].p_data == NULL) {
        goto fail_oom;
      }
      p_entries[ii].data_size = data_size;
    }
  }

  for (uint32_t ii = 0; ii < num_entries; ++ii) {
    raft_log_entry_t* p_entry = &p_entries[ii];
    RM_BYTES(p_entry->p_data, p_entry->data_size);
  }

  p_args->p_log_entries = p_entries;
  p_args->num_entries = num_entries;
  return RAFT_STATUS_OK;;

fail_oom:
  if (p_entries)  {
    for (uint32_t ii = 0; ii < num_entries; ++ii) {
      if (p_entries[ii].p_data) {
        free(p_entries[ii].p_data);
        p_entries[ii].p_data = NULL;
      }
    }
    free(p_entries);
    p_entries = NULL;
  }

  return RAFT_STATUS_OUT_OF_MEMORY;
}

raft_status_t raft_read_request_vote_args(raft_request_vote_args_t* p_args,
                                          void* p_message_bytes,
                                          uint32_t message_size) {
  RM_SETUP;
  RM(term);
  RM(candidate_id);
  RM(last_log_index);
  RM(last_log_term);

  return RAFT_STATUS_OK;
}

raft_status_t raft_read_request_vote_response_args(
    raft_request_vote_response_args_t* p_args,
    void* p_message_bytes,
    uint32_t message_size) {
  RM_SETUP;
  RM(follower_id);
  RM(term);
  RM_BOOL(vote_granted);

  return RAFT_STATUS_OK;
}
