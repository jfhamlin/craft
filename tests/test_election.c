
#include <stdio.h>
#include <stdlib.h>

#include "CuTest.h"

#define STB_DEFINE
#include "stb.h"
#include "stb_pq.h"

#include "raft_config.h"
#include "raft.h"
#include "raft_util.h"

#define NODE_COUNT 5

static raft_state_t* s_node_states[NODE_COUNT];

static raft_state_t* first_leader() {
  for (uint32_t ii = 0; ii < NODE_COUNT; ++ii) {
    if (s_node_states[ii]->type == RAFT_NODE_TYPE_LEADER)
      return s_node_states[ii];
  }
  return NULL;
}

static uint32_t leader_count() {
  uint32_t count = 0;
  for (uint32_t ii = 0; ii < NODE_COUNT; ++ii) {
    if (s_node_states[ii]->type == RAFT_NODE_TYPE_LEADER)
      ++count;
  }
  return count;
}

static uint32_t active_node_count() {
  uint32_t count = 0;
  for (uint32_t ii = 0; ii < NODE_COUNT; ++ii) {
    if (s_node_states[ii]->active)
      ++count;
  }
  return count;
}

static
raft_status_t
append_entries_rpc(raft_nodeid_t id, raft_append_entries_args_t* args) {
  if (!s_node_states[id - 1]->active) return RAFT_STATUS_OK;
  return raft_recv_append_entries(s_node_states[id - 1], args);
}

static
raft_status_t
append_entries_response_rpc(raft_nodeid_t id,
                            raft_append_entries_response_args_t* args) {
  if (!s_node_states[id - 1]->active) return RAFT_STATUS_OK;
  return raft_recv_append_entries_response(s_node_states[id - 1], args);
}

static
raft_status_t
request_vote_rpc(raft_nodeid_t id, raft_request_vote_args_t* args) {
  if (!s_node_states[id - 1]->active) return RAFT_STATUS_OK;
  return raft_recv_request_vote(s_node_states[id - 1], args);
}

static
raft_status_t
request_vote_response_rpc(raft_nodeid_t id,
                          raft_request_vote_response_args_t* args) {
  if (!s_node_states[id - 1]->active) return RAFT_STATUS_OK;
  return raft_recv_request_vote_response(s_node_states[id - 1], args);
}

typedef struct {
  raft_state_t* p_state;
  uint32_t time;
  raft_event_type_t type;
} event_t;

int event_cmp(event_t* p_a, event_t* p_b) {
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

void Test_priority_queue(CuTest* tc) {
  int const num_count = 3;
  int* p_pq = NULL;
  for (int i = 0; i < num_count; ++i) {
    stb_pq_push(p_pq, i, cmp_int);
  }
  CuAssertIntEquals(tc, stb_pq_len(p_pq), num_count);

  stb_pq_pop(p_pq, cmp_int);

  CuAssertIntEquals(tc, stb_pq_len(p_pq), num_count-1);

  int v = 0;
  stb_pq_push(p_pq, v, cmp_int);

  CuAssertIntEquals(tc, stb_pq_len(p_pq), num_count);

  for (int i = 0; i < num_count; ++i) {
    CuAssertIntEquals(tc, stb_pq_min(p_pq), i);
    stb_pq_pop(p_pq, cmp_int);
  }
}

/* EVENT QUEUE */

static
raft_status_t
schedule_event(raft_state_t* p_state,
               uint32_t ms_from_now,
               raft_event_type_t event_type) {
  //printf("scheduling (%llu, %u)...\n", p_state, s_time+ms_from_now);
  event_t event = {
    .p_state = p_state,
    .time = s_time + ms_from_now,
    .type = event_type
  };
  stb_pq_push(p_events, event, event_cmp);

  for (uint32_t ii = 0; ii < NODE_COUNT; ++ii) {
    if (!s_node_states[ii]->active) continue;
    raft_bool_t found_heartbeat = RAFT_FALSE;
    for (uint32_t jj = 0; jj < stb_pq_len(p_events); ++jj) {
      if (p_events[jj].p_state == s_node_states[ii])
        found_heartbeat = RAFT_TRUE;
    }
    RAFT_ASSERT_STR(found_heartbeat, "Missing heartbeat for %u", ii+1);
  }

  return RAFT_STATUS_OK;
}

uint32_t event_count() {
  return stb_pq_len(p_events);
}

void process_event() {
  event_t next = stb_pq_min(p_events);
  stb_pq_pop(p_events, event_cmp);
  s_time = next.time;

  //printf("processing %llu...\n", next.p_state);

  if (RAFT_FAILURE(raft_receive_event(next.p_state, next.type))) {
    RAFT_ASSERT_STR(RAFT_FALSE, "Heartbeat failure.");
  }

  RAFT_ASSERT_STR(active_node_count() <= event_count(), "Missing heartbeat");

  for (uint32_t ii = 0; ii < NODE_COUNT; ++ii) {
    if (!s_node_states[ii]->active) continue;
    raft_bool_t found_heartbeat = RAFT_FALSE;
    for (uint32_t jj = 0; jj < stb_pq_len(p_events); ++jj) {
      if (p_events[jj].p_state == s_node_states[ii])
        found_heartbeat = RAFT_TRUE;
    }
    RAFT_ASSERT_STR(found_heartbeat, "Missing heartbeat for %u", ii+1);
  }
}

void process_events(uint32_t count) {
  for (uint32_t ii = 0; ii < count; ++ii) {
    if (event_count() == 0)
      return;
    process_event();
  }
}

raft_state_t* make_raft_node(uint32_t id) {
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

  p_config->cb.p_append_entries_rpc =          &append_entries_rpc;
  p_config->cb.p_append_entries_response_rpc = &append_entries_response_rpc;
  p_config->cb.p_request_vote_rpc =            &request_vote_rpc;
  p_config->cb.p_request_vote_response_rpc =   &request_vote_response_rpc;
  p_config->cb.p_schedule_event =              &schedule_event;

  raft_state_t* p_state = NULL;
  raft_init(&p_state, p_config);
  return p_state;
}

void start_nodes() {
  for (uint32_t i = 0; i < NODE_COUNT; ++i) {
    s_node_states[i] = make_raft_node(i + 1);
  }
  for (uint32_t i = 0; i < NODE_COUNT; ++i) {
    raft_start(s_node_states[i]);
  }
}


void stop_nodes() {
  for (uint32_t i = 0; i < NODE_COUNT; ++i) {
    raft_stop(s_node_states[i]);
    raft_deinit(s_node_states[i]);
    s_node_states[i] = 0;
  }
  stb_arr_free(p_events);
}

/**
 * TESTS BELOW
 */

void Test_election(CuTest* tc) {
  start_nodes();

  CuAssertPtrEquals(tc, first_leader(), NULL);

  process_events(10);

  CuAssertIntEquals(tc, leader_count(), 1);

  stop_nodes();
}

void Test_election_after_leader_failures(CuTest* tc) {
  start_nodes();

  process_events(10);
  CuAssertIntEquals(tc, 1, leader_count());

  CuAssertIntEquals(tc, active_node_count(), event_count());

  raft_stop(first_leader());
  CuAssertIntEquals(tc, 0, leader_count());

  process_events(10);
  CuAssertIntEquals(tc, 1, leader_count());

  raft_stop(first_leader());

  process_events(10);
  CuAssertIntEquals(tc, 1, leader_count());

  CuAssertIntEquals(tc, active_node_count(), event_count());

  raft_state_t* p_leader = first_leader();
  raft_stop(p_leader);

  process_events(10);
  CuAssertIntEquals(tc, 0, leader_count());

  CuAssertIntEquals(tc, active_node_count(), event_count());

  raft_start(p_leader);

  process_events(10);
  CuAssertIntEquals(tc, 1, leader_count());

  stop_nodes();
}
