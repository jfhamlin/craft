#ifndef __RAFT_UTIL_H__
#define __RAFT_UTIL_H__

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "raft_types.h"

#define RAFT_ASSERT_STR(exp, str, ...)                                  \
  do {                                                                  \
    if (!(exp)) {                                                       \
      printf("ASSERTION FAILURE[%s:%d] '%s'",                           \
             __FILE__, __LINE__, #exp);                                 \
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
    fprintf(stderr, "%u: ", _p_state->p.self);        \
    fprintf(stderr, _str, ##__VA_ARGS__);               \
    fprintf(stderr, "\n");                              \
  } while (0)

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define RAFT_LSBYTE(val, idx) (((val) >> (8*(idx))) % 0xff)

#define RAFT_ALIGN_UP(val, align) (((val) + ((align) - 1)) & (~((align) - 1)))

void raft_sleep(uint32_t ms);

#define ARRAY_ELEMENT_COUNT(_arr) (sizeof(_arr) / sizeof(_arr[0]))

#endif
