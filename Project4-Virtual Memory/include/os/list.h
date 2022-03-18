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
#define LIST_HEAD(name) struct list_node name = {&(name), &(name)};
#define offsetof(TYPE, MEMBER) ((size_t) & ((TYPE *)0)->MEMBER)
// #define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#define list_entry(ptr, type, member)                          \
    (                                                          \
        {                                                      \
            const typeof(((type *)0)->member) *__mptr = (ptr); \
            (type *)((char *)__mptr - offsetof(type, member)); \
        })

// TODO: use your own list design!!
typedef struct list_node
{
    struct list_node *next, *prev;
} list_node_t;
typedef list_node_t list_head;

static inline void init_list(list_head *list)
{
    list->next = list;
    list->prev = list;
}

static inline void list_add(list_node_t *node, list_head *head)
{
    node->prev = head;
    node->next = head->next;
    head->next->prev = node;
    head->next = node;
}

/*出队列*/
static inline list_node_t *get_first_node(list_head *head)
{
    list_node_t *del = head->prev;
    del->prev->next = head;
    head->prev = del->prev;
    return del;
}
/*将某个list直接从队列中删除出来*/
static inline void list_del(list_head *node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;
}

/*队列是否为空*/
static inline int list_empty(list_head *head)
{
    return head->next == head ? 1 : 0;
}

#endif
