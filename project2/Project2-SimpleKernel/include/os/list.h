/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * * * * * * * * * * Copyright (C) 2018 Institute of Computing
 * Technology, CAS Author : Han Shukai (email :
 * hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * * * * * * * * * * Changelog: 2019-8 Reimplement queue.h.
 * Provide Linux-style doube-linked list instead of original
 * unextendable Queue implementation. Luming
 * Wang(wangluming@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * * * * * * * * * * * */

#ifndef INCLUDE_LIST_H_
#define INCLUDE_LIST_H_

#include <type.h>

// double-linked list
typedef struct list_node
{
    struct list_node *next, *prev;
    priority_t priority;
} list_node_t;

typedef list_node_t list_head;

// LIST_HEAD is used to define the head of a list.
#define LIST_HEAD(name) struct list_node name = {&(name), &(name)}

static inline void init_list_head(list_head *list)
{
    list->next = list;
    list->prev = list;
    list->priority = 0;
}

static inline void list_add(list_node_t *node, list_node_t *head)
{
    list_node_t *tail = head->prev;
    tail->next = node;
    node->prev = tail;
    node->next = head;
    head->prev = node;
}

static inline void list_del(list_node_t *node)
{
    list_node_t *prev = node->prev;
    list_node_t *next = node->next;
    next->prev = prev;
    prev->next = next;
    node->prev = NULL;
    node->next = NULL;
}

static inline list_node_t *get_head_node(list_head *head)
{
    list_node_t *temp = head->next;
    return temp;
}

static inline int list_empty(list_head *head)
{
    return head->next == head;
}

static inline void list_add_node(list_node_t *node, list_node_t *prev, list_node_t *next)
{
    next->prev = node;
    node->next = next;
    node->prev = prev;
    prev->next = node;
}

static inline void list_add_priority(list_node_t *node, list_node_t *head)
{
    list_node_t *tail_node = head->prev;
    list_node_t *sort_node = head;
    if (head->next == head)
    {
        list_add_node(node, tail_node, head);
        return;
    }
    while (tail_node != head)
    {
        if (node->priority <= tail_node->priority)
        {
            list_add_node(node, tail_node, sort_node);
            return;
        }
        tail_node = tail_node->prev;
        sort_node = sort_node->prev;
    }
    list_add_node(node, head, head->next);
}

#endif
