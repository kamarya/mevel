/*
 *   Copyright 2018 Behrooz Kamary
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct queue_s {
    void*           ptr;
    struct queue_s* nxt;
} queue_t;

typedef struct {
    queue_t*    head;
    queue_t*    tail;
    size_t      size;
} queue_ctx_t;

queue_ctx_t*    queue_ini();
void	        queue_rel(queue_ctx_t*);
queue_t*        queue_put(queue_ctx_t*, void*);
void*           queue_del(queue_ctx_t*, queue_t*);

queue_t*        queue_fnd_ptr(queue_ctx_t*, void*);
void	        queue_rel_ptr(queue_ctx_t*);
void            queue_del_ptr(queue_ctx_t*, void*);

void*           queue_pop_head(queue_ctx_t*);
void*           queue_pop_tail(queue_ctx_t*);
queue_t*        queue_push_head(queue_ctx_t*, void*);
queue_t*        queue_push_tail(queue_ctx_t*, void*);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __QUEUE_H__
