#include "raft_wire.h"
#include "raft_util.h"

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

static uint8_t* write(uint8_t* p_b, uint32_t v) {
  (*p_b++) = (v >> 24) & 0xfF;
  (*p_b++) = (v >> 16) & 0xfF;
  (*p_b++) = (v >> 8)  & 0xfF;
  (*p_b++) = (v >> 0)  & 0xfF;
  return p_b;
}

#define RAFT_MSG_VERSION 0x000001
#define RAFT_MSG_VERSION_BYTE(idx) RAFT_LSBYTE(RAFT_MSG_VERSION, idx)

/* AM: append to message */
#define AM_SETUP(_type)                         \
  uint8_t* p_buf = p_env->p_message;            \
  do {                                          \
    (*p_buf++) = RAFT_MSG_VERSION_BYTE(2);      \
    (*p_buf++) = RAFT_MSG_VERSION_BYTE(1);      \
    (*p_buf++) = RAFT_MSG_VERSION_BYTE(0);      \
    (*p_buf++) = _type;                         \
  } while (0)

#define AM(_member_name)                                                \
  do {                                                                  \
    p_buf = write(p_buf, p_args->_member_name);                         \
  } while (0)

enum {
  MSG_TYPE_APPEND_ENTRIES = 0x1,
  MSG_TYPE_APPEND_ENTRIES_RESPONSE,
  MSG_TYPE_REQUEST_VOTE,
  MSG_TYPE_REQUEST_VOTE_RESPONSE,
};

void raft_write_request_vote_envelope(raft_envelope_t* p_env,
                                      raft_nodeid_t recipient_id,
                                      raft_request_vote_args_t const* p_args) {
  p_env->recipient_id = recipient_id;

  AM_SETUP(MSG_TYPE_REQUEST_VOTE);
  AM(term);
  AM(candidate_id);
  AM(last_log_index);
  AM(last_log_term);
}

void raft_write_request_vote_response_envelope(
    raft_envelope_t* p_env,
    raft_nodeid_t recipient_id,
    raft_request_vote_response_args_t const* p_args) {
  p_env->recipient_id = recipient_id;

  AM_SETUP(MSG_TYPE_REQUEST_VOTE_RESPONSE);
  AM(follower_id);
  AM(term);
  AM(vote_granted);
}
