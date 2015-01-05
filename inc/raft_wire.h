#ifndef __RAFT_WIRE_H__
#define __RAFT_WIRE_H__

#include "raft_types.h"
#include "raft_rpc.h"

typedef enum {
  MSG_TYPE_APPEND_ENTRIES = 0x1,
  MSG_TYPE_APPEND_ENTRIES_RESPONSE,
  MSG_TYPE_REQUEST_VOTE,
  MSG_TYPE_REQUEST_VOTE_RESPONSE,
} raft_message_type_t;

typedef struct {
  raft_nodeid_t recipient_id;
  uint32_t message_size;
  uint32_t buffer_capacity;
  uint8_t* p_message;
} raft_envelope_t;

raft_status_t raft_write_append_entries_envelope(
    raft_envelope_t* p_env,
    raft_nodeid_t recipient_id,
    raft_append_entries_args_t const* p_args);
raft_status_t raft_write_request_vote_envelope(
    raft_envelope_t* p_envelope,
    raft_nodeid_t node_id,
    raft_request_vote_args_t const* p_args);
raft_status_t raft_write_request_vote_response_envelope(
    raft_envelope_t* p_envelope,
    raft_nodeid_t node_id,
    raft_request_vote_response_args_t const* p_args);

void raft_dealloc_envelope(raft_envelope_t* p_envelope);

raft_message_type_t raft_message_type(void* p_message_bytes);

raft_status_t raft_read_append_entries_args(raft_append_entries_args_t* p_args,
                                            void* p_message_bytes,
                                            uint32_t message_size);

raft_status_t raft_read_request_vote_args(raft_request_vote_args_t* p_args,
                                          void* p_message_bytes,
                                          uint32_t message_size);

raft_status_t raft_read_request_vote_response_args(
    raft_request_vote_response_args_t* p_args,
    void* p_message_bytes,
    uint32_t message_size);

#endif
