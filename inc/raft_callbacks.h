#ifndef __RAFT_CALLBACKS_H__
#define __RAFT_CALLBACKS_H__

#include "raft_rpc.h"

typedef raft_status_t raft_send_message_f(
    raft_nodeid_t recipient_id,
    void* p_msg,
    uint32_t message_size
);

typedef raft_status_t raft_append_entries_rpc_f(
    raft_nodeid_t,
    raft_append_entries_args_t*
);

typedef raft_status_t raft_append_entries_response_rpc_f(
    raft_nodeid_t,
    raft_append_entries_response_args_t*
);

typedef raft_status_t raft_request_vote_rpc_f(
    raft_nodeid_t,
    raft_request_vote_args_t*
);

typedef raft_status_t raft_request_vote_response_rpc_f(
    raft_nodeid_t,
    raft_request_vote_response_args_t*
);

typedef struct {
  /**
   *
   */
  raft_send_message_f* pf_send_message;

  raft_append_entries_rpc_f*          pf_append_entries_rpc;
  raft_append_entries_response_rpc_f* pf_append_entries_response_rpc;

  raft_request_vote_rpc_f*          pf_request_vote_rpc;
  raft_request_vote_response_rpc_f* pf_request_vote_response_rpc;
} raft_callbacks_t;

#endif
