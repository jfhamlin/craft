#ifndef __TEST_HELPERS_H__
#define __TEST_HELPERS_H__

#pragma clang diagnostic ignored "-Wunused-function"

#include <stdlib.h>

#include "stb.h"
#include "stb_pq.h"

#include "raft.h"
#include "raft_state.h"
#include "raft_config.h"
#include "raft_util.h"

static raft_state_t* s_node_states[NODE_COUNT] = { 0 };

static raft_state_t* get_node(uint32_t i) {
  if (i >= NODE_COUNT) return NULL;
  return s_node_states[i];
}

static int32_t first_leader() {
  for (int32_t ii = 0; ii < NODE_COUNT; ++ii) {
    if (s_node_states[ii] && s_node_states[ii]->type == RAFT_NODE_TYPE_LEADER)
      return ii;
  }
  return -1;
}

static uint32_t leader_count() {
  uint32_t count = 0;
  for (uint32_t ii = 0; ii < NODE_COUNT; ++ii) {
    if (s_node_states[ii] && s_node_states[ii]->type == RAFT_NODE_TYPE_LEADER)
      ++count;
  }
  return count;
}

static
raft_status_t
append_entries_rpc(raft_nodeid_t id, raft_append_entries_args_t* args) {
  if (!s_node_states[id - 1]) return RAFT_STATUS_OK;
  return raft_recv_append_entries(s_node_states[id - 1], args);
}

static
raft_status_t
append_entries_response_rpc(raft_nodeid_t id,
                            raft_append_entries_response_args_t* args) {
  if (!s_node_states[id - 1]) return RAFT_STATUS_OK;
  return raft_recv_append_entries_response(s_node_states[id - 1], args);
}

static
raft_status_t
request_vote_rpc(raft_nodeid_t id, raft_request_vote_args_t* args) {
  if (!s_node_states[id - 1]) return RAFT_STATUS_OK;
  return raft_recv_request_vote(s_node_states[id - 1], args);
}

static
raft_status_t
request_vote_response_rpc(raft_nodeid_t id,
                          raft_request_vote_response_args_t* args) {
  if (!s_node_states[id - 1]) return RAFT_STATUS_OK;
  return raft_recv_request_vote_response(s_node_states[id - 1], args);
}

static
raft_status_t
send_message_callback(raft_nodeid_t id,
                      void* p_msg,
                      uint32_t message_size) {
  if (!s_node_states[id - 1]) return RAFT_STATUS_OK;
  return raft_recv_message(s_node_states[id - 1], p_msg, message_size);
}

typedef struct {
  raft_nodeid_t node_id;
  uint32_t time;

  enum {
    EVENT_TYPE_TICK
  } type;

  union {
    uint32_t elapsed_ms;
  };
} event_t;

static int event_cmp(event_t* p_a, event_t* p_b) {
  if (p_a->time < p_b->time) return -1;
  else if (p_a->time == p_b->time) return 0;
  else return 1;
}

static event_t* p_events = NULL;

static uint32_t s_time = 0;

static int cmp_int(int* p_a, int* p_b) {
  if (*p_a < *p_b) return -1;
  if (*p_a == *p_b) return 0;
  return 1;
}

/* EVENT QUEUE */

static void schedule_event(raft_nodeid_t node_id,
                           uint32_t ms_from_now,
                           event_t event) {
  //printf("scheduling (id: %llu, ms: %u)...\n", node_id, s_time+ms_from_now);

  event.node_id = node_id;
  event.time = s_time + ms_from_now;

  stb_pq_push(p_events, event, event_cmp);
}

static uint32_t event_count() {
  return stb_pq_len(p_events);
}

static void process_event() {
  event_t next = stb_pq_min(p_events);
  stb_pq_pop(p_events, event_cmp);
  s_time = next.time;

  //printf("processing %llu...\n", next.node_id);

  raft_state_t* p_state = get_node(next.node_id);
  if (p_state == NULL) {
    return;
  }

  switch (next.type) {
    case EVENT_TYPE_TICK:
    {
      uint32_t next_tick_ms;
      raft_tick(&next_tick_ms, p_state, next.elapsed_ms);
      next.elapsed_ms = next_tick_ms;
      schedule_event(next.node_id, next_tick_ms, next);
      break;
    }
    default:
    {
      RAFT_ASSERT(RAFT_FALSE);
      break;
    }
  }
}

static void process_events(uint32_t count) {
  for (uint32_t ii = 0; ii < count; ++ii) {
    if (event_count() == 0)
      return;
    process_event();
  }
}

#define STOP_NODES() do { stop_nodes(); stb_arr_free(p_events); } while (0)

static raft_state_t* make_raft_node(uint32_t id) {
  raft_config_t* p_config = calloc(1, sizeof(raft_config_t));
  p_config->node_count = NODE_COUNT;
  p_config->p_nodeids = calloc(1, (NODE_COUNT - 1) * sizeof(raft_nodeid_t));

  raft_nodeid_t other_id = 1;
  for (uint32_t i = 0; i < NODE_COUNT - 1; ++i) {
    if (other_id == id) ++other_id;
    p_config->p_nodeids[i] = other_id++;
  }

  p_config->selfid = id;

  p_config->leader_ping_interval_ms = 100;

  p_config->election_timeout_max_ms = 1000;
  p_config->election_timeout_min_ms = 500;

  p_config->cb.pf_append_entries_rpc =          &append_entries_rpc;
  p_config->cb.pf_append_entries_response_rpc = &append_entries_response_rpc;
  //p_config->cb.pf_request_vote_rpc =            &request_vote_rpc;
  p_config->cb.pf_request_vote_response_rpc =   &request_vote_response_rpc;

  p_config->cb.pf_send_message = &send_message_callback;

  raft_state_t* p_state = NULL;
  raft_alloc(&p_state, p_config);
  return p_state;
}

static void start_nodes() {
  for (uint32_t i = 0; i < NODE_COUNT; ++i) {
    s_node_states[i] = make_raft_node(i + 1);

    event_t event = {
      .type = EVENT_TYPE_TICK,
      .elapsed_ms = 0
    };
    schedule_event(i, 0, event);
  }
}

static void stop_node(uint32_t i) {
  if (s_node_states[i]) {
    raft_free(s_node_states[i]);
    s_node_states[i] = NULL;
  }
}

static void stop_nodes() {
  for (uint32_t i = 0; i < NODE_COUNT; ++i) {
    stop_node(i);
  }
}

#endif
