#include <stdlib.h>

#include "raft_wire.h"
#include "raft_util.h"

void raft_dealloc_envelope(raft_envelope_t* p_envelope) {
  free(p_envelope->p_message);
  memset(p_envelope, 0, sizeof(*p_envelope));
}

/**
 * The following describes the wire format for raft messages.
 * All multibyte values are encoded in network (big-endian) order.
 *
 * Bytes | Semantics
 * =======================================
 *   0-2 | Magic Number                   -| Message
 *     3 | Message Type                    | Header
 *   4-7 | Message Size (from byte 0)     -|
 * - - - - - - - - - - - - - - - - - - - - -
 *   8-* | Constant-length message fields
 *       | (Type-dependent)
 * - - - - - - - - - - - - - - - - - - - - -
 *    *  | Log entry data (AppendEntries only)
 * =======================================
 */

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
  0, /* UNKNOWN */
  0, /* UNKNOWN */
  24, /* MSG_TYPE_REQUEST_VOTE */
  20, /* MSG_TYPE_REQUEST_VOTE_RESPONSE */
};

#define MESSAGE_SIZE(_type) a_message_sizes[(_type)]

/* AM: append to message */
#define AM_SETUP(_type)                                                 \
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
    p_env->message_size = MESSAGE_SIZE(_type);                          \
    (*p_buf++) = RAFT_MSG_VERSION_BYTE(2);                              \
    (*p_buf++) = RAFT_MSG_VERSION_BYTE(1);                              \
    (*p_buf++) = RAFT_MSG_VERSION_BYTE(0);                              \
    (*p_buf++) = _type;                                                 \
    (*p_buf++) = RAFT_LSBYTE(p_env->message_size, 3);                   \
    (*p_buf++) = RAFT_LSBYTE(p_env->message_size, 2);                   \
    (*p_buf++) = RAFT_LSBYTE(p_env->message_size, 1);                   \
    (*p_buf++) = RAFT_LSBYTE(p_env->message_size, 0);                   \
  } while (0)

#define AM(_member_name)                                                \
  do {                                                                  \
    p_buf = write(p_buf, p_args->_member_name);                         \
  } while (0)

/* AM: Read message */
#define RM_SETUP uint8_t const* p_buf = p_message_bytes + 8

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

/*******************************************************************************
 *******************************************************************************
 ******************************************************************************/

raft_status_t raft_write_request_vote_envelope(
    raft_envelope_t* p_env,
    raft_nodeid_t recipient_id,
    raft_request_vote_args_t const* p_args) {
  AM_SETUP(MSG_TYPE_REQUEST_VOTE);
  AM(term);
  AM(candidate_id);
  AM(last_log_index);
  AM(last_log_term);

  return RAFT_STATUS_OK;
}

raft_status_t raft_write_request_vote_response_envelope(
    raft_envelope_t* p_env,
    raft_nodeid_t recipient_id,
    raft_request_vote_response_args_t const* p_args) {
  AM_SETUP(MSG_TYPE_REQUEST_VOTE_RESPONSE);
  AM(follower_id);
  AM(term);
  AM(vote_granted);

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
