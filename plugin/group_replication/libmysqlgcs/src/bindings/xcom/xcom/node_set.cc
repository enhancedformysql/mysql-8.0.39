/* Copyright (c) 2015, 2024, Oracle and/or its affiliates.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License, version 2.0,
   as published by the Free Software Foundation.

   This program is designed to work with certain software (including
   but not limited to OpenSSL) that is licensed under separate terms,
   as designated in a particular file or component or in included license
   documentation.  The authors of MySQL hereby grant you an additional
   permission to link the program and your derivative works with the
   separately licensed software that they have either included with
   the program or referenced in the documentation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License, version 2.0, for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

#ifdef _MSC_VER
#include <stdint.h>
#endif
#include <rpc/rpc.h>
#include <stdlib.h>

#include "xcom/bitset.h"
#include "xcom/xcom_profile.h"
#ifndef XCOM_STANDALONE
#include "my_compiler.h"
#endif
#include "xcom/node_set.h"
#include "xcom/simset.h"
#include "xcom/task.h"
#include "xcom/task_debug.h"
#include "xcom/x_platform.h"
#include "xcom/xcom_common.h"
#include "xcom/xcom_memory.h"
#include "xdr_gen/xcom_vp.h"

node_set *alloc_node_set(node_set *set, u_int n) {
  set->node_set_val = (int *)calloc((size_t)n, sizeof(bool_t));
  set->node_set_len = n;
  return set;
}

node_set *realloc_node_set(node_set *set, u_int n) {
  u_int old_n = set->node_set_len;
  bool_t *old_p = set->node_set_val;
  u_int i;

  set->node_set_val = (int *)realloc(old_p, n * sizeof(bool_t));
  set->node_set_len = n;
  for (i = old_n; i < n; i++) {
    set->node_set_val[i] = 0;
  }
  return set;
}

/* Copy node set. Reallocate if mismatch */

void copy_node_set(node_set const *from, node_set *to) {
  if (from->node_set_len > 0) {
    u_int i;
    if (to->node_set_val == nullptr || from->node_set_len != to->node_set_len) {
      init_node_set(to, from->node_set_len);
    }
    for (i = 0; i < from->node_set_len; i++) {
      to->node_set_val[i] = from->node_set_val[i];
    }
  }
}

/* Initialize node set. Free first if necessary */

node_set *init_node_set(node_set *set, u_int n) {
  if (set) {
    free_node_set(set);
    alloc_node_set(set, n);
  }
  return set;
}

/* Free node set contents */

void free_node_set(node_set *set) {
  if (set) {
    if (set->node_set_val != nullptr) X_FREE(set->node_set_val);
    set->node_set_len = 0;
  }
}

/* Clone set. Used when sending messages */

node_set clone_node_set(node_set set) {
  node_set new_set;
  new_set.node_set_len = 0;
  new_set.node_set_val = nullptr;
  copy_node_set(&set, &new_set);
  return new_set;
}

node_set *set_node_set(node_set *set) {
  u_int i;
  for (i = 0; set && i < set->node_set_len; i++) {
    set->node_set_val[i] = TRUE;
  }
  return set;
}

/* Count number of nodes in set */

u_int node_count(node_set set) {
  u_int count = 0;
  u_int i;
  for (i = 0; i < set.node_set_len; i++) {
    if (set.node_set_val[i]) count++;
  }
  return count;
}

/* Return true if empty node set */

bool_t is_empty_node_set(node_set set) {
  u_int i;
  for (i = 0; i < set.node_set_len; i++) {
    if (set.node_set_val[i]) return FALSE;
  }
  return TRUE;
}

/* Return true if full node set */

bool_t is_full_node_set(node_set set) {
  u_int i;
  for (i = 0; i < set.node_set_len; i++) {
    if (!set.node_set_val[i]) return FALSE;
  }
  return TRUE;
}

/* Return true if equal node sets */

bool_t equal_node_set(node_set x, node_set y) {
  u_int i;
  if (x.node_set_len != y.node_set_len) return FALSE;
  for (i = 0; i < x.node_set_len; i++) {
    if (x.node_set_val[i] != y.node_set_val[i]) return FALSE;
  }
  return TRUE;
}
/* purecov: end */

/* Return true if node i is in set */

bool_t is_set(node_set set, node_no i) {
  if (i < set.node_set_len) {
    return set.node_set_val[i];
  } else {
    return FALSE;
  }
}

/* Remove node from set */
void remove_node(node_set set, node_no node) {
  if (node < set.node_set_len) {
    set.node_set_val[node] = FALSE;
  }
}

/* AND operation, return result in x */
void and_node_set(node_set *x, node_set const *y) {
  u_int i;
  for (i = 0; i < x->node_set_len && i < y->node_set_len; i++) {
    x->node_set_val[i] = x->node_set_val[i] && y->node_set_val[i];
  }
}

/* OR operation, return result in x */
void or_node_set(node_set *x, node_set const *y) {
  u_int i;
  for (i = 0; i < x->node_set_len && i < y->node_set_len; i++) {
    x->node_set_val[i] = x->node_set_val[i] || y->node_set_val[i];
  }
}

/* XOR operation, return result in x */
void xor_node_set(node_set *x, node_set const *y) {
  u_int i;
  for (i = 0; i < x->node_set_len && i < y->node_set_len; i++) {
    x->node_set_val[i] =
        x->node_set_val[i] ^ y->node_set_val[i]; /* Beware of the bitwise xor */
  }
}

/* NOT operation, return result in x */
void not_node_set(node_set *x, node_set const *y) {
  u_int i;
  for (i = 0; i < x->node_set_len && i < y->node_set_len; i++) {
    x->node_set_val[i] = (y->node_set_val[i] == TRUE ? FALSE : TRUE);
  }
}

