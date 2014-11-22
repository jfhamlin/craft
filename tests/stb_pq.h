#ifndef __STB_PQ_H__
#define __STB_PQ_H__

#include "stb.h"

/* PRIORITY QUEUE */

#define stb_pq_len stb_arr_len
#define stb_pq_min(arr) ((arr)[0])

#define stb_pq__par_idx(idx) (((idx) - 1) / 2)
#define stb_pq__par(arr, idx) ((arr)[stb_pq__par_idx(idx)])
#define stb_pq__cld(arr, idx, n) ((arr)[2*idx + n + 1])

#define stb_pq_push(arr, val, cmp)                              \
  do {                                                          \
    uint32_t idx = stb_pq_len(arr);                             \
    stb_arr_add(arr);                                           \
    while (idx > 0 && cmp(&val, &stb_pq__par(arr, idx)) < 0) {  \
      (arr)[idx] = stb_pq__par(arr, idx);                       \
      idx = stb_pq__par_idx(idx);                               \
    }                                                           \
    (arr)[idx] = val;                                           \
  } while (0)

#define stb_pq_pop(arr, cmp)                                            \
  do {                                                                  \
    uint32_t len = stb_pq_len(arr) - 1;                                 \
    uint32_t idx = 0;                                                   \
    while (2*idx + 1 < len) {                                           \
      if (2*idx + 2 >= len ||                                           \
          cmp(&stb_pq__cld(arr, idx, 0),                                \
              &stb_pq__cld(arr, idx, 1)) < 0) {                         \
        if (cmp(&(arr)[len], &stb_pq__cld(arr, idx, 0)) < 0) break;     \
        (arr)[idx] = stb_pq__cld(arr, idx, 0);                          \
        idx = 2*idx+1;                                                  \
      } else {                                                          \
        if (cmp(&(arr)[len], &stb_pq__cld(arr, idx, 1)) < 0) break;     \
        (arr)[idx] = stb_pq__cld(arr, idx, 1);                          \
        idx = 2*idx + 2;                                                \
      }                                                                 \
    }                                                                   \
    (arr)[idx] = (arr)[len];                                            \
    stb_arr_pop(arr);                                                   \
  } while (0)

#endif
