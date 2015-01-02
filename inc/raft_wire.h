#ifndef __RAFT_WIRE_H__
#define __RAFT_WIRE_H__

#include "raft_types.h"
#include "raft_rpc.h"

/**
 *
 */
// void raft_write_append_entries_envelope(
//     raft_envelope_t* p_envelope,
//     raft_nodeid_t node_id,
//     raft_append_entries_args_t const* p_args);
// void raft_write_append_entries_response(raft_envelope_t* p_envelope,
//                          raft_nodeid_t node_id,
//                          raft_append_entries_response_args_t const* p_args);
void raft_write_request_vote_envelope(
    raft_envelope_t* p_envelope,
    raft_nodeid_t node_id,
    raft_request_vote_args_t const* p_args);
void raft_write_request_vote_responseenvelope(
    raft_envelope_t* p_envelope,
    raft_nodeid_t node_id,
    raft_request_vote_response_args_t const* p_args);

#endif
