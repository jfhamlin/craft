#ifndef __RAFT_UTIL_H__
#define __RAFT_UTIL_H__

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "raft_types.h"

#define RAFT_ASSERT_STR(exp, str, ...)                                  \
  do {                                                                  \
    if (!(exp)) {                                                       \
      printf("ASSERTION FAILURE[%s:%d] '%s'", __FILE__, __LINE__, #exp); \
      if (strlen(str)) {                                                \
        printf(" - ");                                                  \
        printf((str), ##__VA_ARGS__);                                   \
      }                                                                 \
      printf("\n");                                                     \
      assert(0);                                                        \
    }                                                                   \
  } while (0)

#define RAFT_ASSERT(exp) RAFT_ASSERT_STR(exp, "")

#define RAFT_LOG(_p_state, _str, ...)                   \
  do {                                                  \
    fprintf(stderr, "%llu: ", _p_state->p.self);        \
    fprintf(stderr, _str, ##__VA_ARGS__);               \
    fprintf(stderr, "\n");                              \
  } while (0)

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define RAFT_LSBYTE(val, idx) (((val) >> (8*(idx))) % 0xff)

void raft_sleep(uint32_t ms);

#endif
