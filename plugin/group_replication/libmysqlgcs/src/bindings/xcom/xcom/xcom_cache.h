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

#ifndef XCOM_CACHE_H
#define XCOM_CACHE_H

#include <stddef.h>

#include "xcom/simset.h"
#include "xcom/xcom_profile.h"
#include "xcom/task_debug.h"
#include "xdr_gen/xcom_vp.h"

#define is_cached(x) (hash_get(x) != NULL)

struct lru_machine;
typedef struct lru_machine lru_machine;

struct stack_machine;
typedef struct stack_machine stack_machine;

struct pax_machine;
typedef struct pax_machine pax_machine;

/* Definition of a Paxos instance */
struct pax_machine {
  linkage hash_link;
  stack_machine *stack_link;
  lru_machine *lru;
  synode_no synode;
  double last_modified; /* Start time */
  linkage rv; /* Tasks may sleep on this until something interesting happens */

  struct {
    ballot bal;            /* The current ballot we are working on */
    bit_set *prep_nodeset; /* Nodes which have answered my prepare */
    ballot sent_prop;
    bit_set *prop_nodeset; /* Nodes which have answered my propose */
    pax_msg *msg;          /* The value we are trying to push */
    ballot sent_learn;
  } proposer;

  struct {
    ballot promise; /* Promise to not accept any proposals less than this */
    pax_msg *msg;   /* The value we have accepted */
  } acceptor;

  struct {
    pax_msg *msg; /* The value we have learned */
  } learner;
  int lock; /* Busy ? */
  pax_op op;
  int force_delivery;
  int enforcer;

#ifndef XCOM_STANDALONE
  char is_instrumented;
#endif
};

pax_machine *init_pax_machine(pax_machine *p, lru_machine *lru,
                              synode_no synode);
int is_busy_machine(pax_machine *p);
int lock_pax_machine(pax_machine *p);
pax_machine *get_cache_no_touch(synode_no synode, bool_t force);
pax_machine *get_cache(synode_no synode);
pax_machine *force_get_cache(synode_no synode);
pax_machine *hash_get(synode_no synode);
void init_cache();
void deinit_cache();
void unlock_pax_machine(pax_machine *p);
void xcom_cache_var_init();
size_t shrink_cache();
size_t pax_machine_size(pax_machine const *p);
synode_no cache_get_last_removed();

void init_cache_size();
uint64_t add_cache_size(pax_machine *p);
uint64_t sub_cache_size(pax_machine *p);
int above_cache_limit();
uint64_t set_max_cache_size(uint64_t x);
void set_max_cache_mode(int x);
int was_removed_from_cache(synode_no x);
uint16_t check_decrease();
void do_cache_maintenance();

/* Unit testing */
#define DEC_THRESHOLD_LENGTH MAX_CACHE_SIZE / 2
#define MIN_TARGET_OCCUPATION 0.7F
#define DEC_THRESHOLD_SIZE 0.95F
#define MIN_LENGTH_THRESHOLD 0.9F

#ifndef XCOM_STANDALONE
void psi_set_cache_resetting(int is_resetting);
void psi_report_cache_shutdown();
void psi_report_mem_free(size_t size, int is_instrumented);
int psi_report_mem_alloc(size_t size);
#else
#define psi_set_cache_resetting(x) \
  do {                             \
  } while (0)
#define psi_report_cache_shutdown() \
  do {                              \
  } while (0)
#define psi_report_mem_free(x) \
  do {                         \
  } while (0)
#define psi_report_mem_alloc(x) \
  do {                          \
  } while (0)
#endif

enum {
  CACHE_SHRINK_OK = 0,
  CACHE_TOO_SMALL = 1,
  CACHE_HASH_NOTEMPTY = 2,
  CACHE_HIGH_OCCUPATION = 3,
  CACHE_RESULT_LOW = 4,
  CACHE_INCREASING = 5
};

#endif
