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

#include <stdlib.h>
#include <stddef.h>

#include "queue.h"

queue_ctx_t* queue_ini()
{
    queue_ctx_t* ctx = (queue_ctx_t*) malloc(sizeof(queue_ctx_t));

    if (ctx)
    {
        ctx->size = 0;
        ctx->head = NULL;
        ctx->tail = NULL;
    }

    return ctx;
}

void queue_rel(queue_ctx_t* ctx)
{
    if (ctx == NULL) return;

    queue_t* head = ctx->head;
    queue_t* elem = NULL;

    while (head != NULL)
    {
        elem = head;
        head = head->nxt;

        free(elem);
    }

    free(ctx);
}

queue_t*  queue_put(queue_ctx_t* ctx, void*  ptr)
{

    if (ctx == NULL) return NULL;

    queue_t*    elem = (queue_t*)malloc(sizeof(queue_t));
    if (!elem) return NULL;

    elem->ptr = ptr;
    elem->nxt = NULL;

    if (ctx->head != NULL)
    {
        elem->nxt = ctx->head;
        ctx->head = elem;
	}
    else
    {
        ctx->head = elem;
        ctx->tail = elem;
    }

    ctx->size++;

    return elem;
}

void*  queue_del(queue_ctx_t* ctx, queue_t*  elem)
{

    if (ctx == NULL) return NULL;

    queue_t*    celem   = ctx->head;
    queue_t*    pelem   = NULL;
    void*       ptr     = NULL;

    while (celem)
    {
        if (celem == elem)
        {
            if (pelem != NULL)
                pelem->nxt = celem->nxt;
            else
                ctx->head  = celem->nxt;

            ctx->size--;

            ptr = elem->ptr;
            free(elem);
            break;
        }

        pelem = celem;
        celem = celem->nxt;
    }

    return ptr;
}

queue_t*  queue_fnd_ptr(queue_ctx_t* ctx, void*  ptr)
{

    if (ctx == NULL) return NULL;

    queue_t* elem = ctx->head;

    while (elem)
    {
        if (elem->ptr == ptr) break;
        elem = elem->nxt;
    }

    return elem;
}


void	queue_rel_ptr(queue_ctx_t* ctx)
{
    if (ctx == NULL) return;

    queue_t* head = ctx->head;
    queue_t* elem = NULL;

    while (head != NULL)
    {
        elem = head;
        head = head->nxt;
        if (elem->ptr != NULL) free(elem->ptr);
        free(elem);
    }

    free(ctx);
}

void	queue_del_ptr(queue_ctx_t* ctx, void* ptr)
{
    if (ctx == NULL) return;

    queue_t*    celem   = ctx->head;
    queue_t*    pelem   = NULL;

    while (celem)
    {
        if (celem->ptr == ptr)
        {
            if (pelem != NULL)
                pelem->nxt = celem->nxt;
            else
                ctx->head  = celem->nxt;

            ctx->size--;
            if (celem->ptr != NULL) free(celem->ptr);
            free(celem);
            break;
        }

        pelem = celem;
        celem = celem->nxt;
    }
}



void*     queue_pop_head(queue_ctx_t* ctx)
{

    if (ctx == NULL) return NULL;
    void* head = ctx->head;

    return head;
}

void*     queue_pop_tail(queue_ctx_t* ctx)
{

    if (ctx == NULL) return NULL;

    void* tail = ctx->tail;

    return tail;
}

queue_t*   queue_push_head(queue_ctx_t* ctx, void* ptr)
{

    if (ctx == NULL) return NULL;

    queue_t*    elem = (queue_t*)malloc(sizeof(queue_t));
    if (!elem) return NULL;

    return elem;
}

queue_t*  queue_push_tail(queue_ctx_t* ctx, void* ptr)
{

    if (ctx == NULL) return NULL;

    queue_t*    elem = (queue_t*)malloc(sizeof(queue_t));
    if (!elem) return NULL;

    return elem;
}
