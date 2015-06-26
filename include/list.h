/**
 *
 * @file: list.h
 * @author: Alvin Jiang
 * @mail: celsius.j@gmail.com
 * @created time: 2015-05-04
 *
 */


#ifndef __LIST_H__
#define __LIST_H__

#include "type.h"
#include "stddef.h"

/*
    doubly linked list
    ref: <linux/list.h>
*/
struct list_head {
    struct list_head *next, *prev;
};

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
    struct list_head name = LIST_HEAD_INIT(name)

#define INIT_LIST_HEAD(ptr) do { \
    (ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)

static inline void __list_add(struct list_head *new,
                  struct list_head *prev,
                  struct list_head *next)
{
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

static inline void list_add(struct list_head *new, struct list_head *head)
{
    __list_add(new, head, head->next);
}

static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
    __list_add(new, head->prev, head);
}

static inline void __list_del(struct list_head * prev, struct list_head * next)
{
    next->prev = prev;
    prev->next = next;
}

static inline void list_del(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
    // entry->next = LIST_POISON1;
    // entry->prev = LIST_POISON2;
}

static inline int list_empty(const struct list_head *head)
{
    return head->next == head;
}

#define list_entry(ptr, type, member) \
    container_of(ptr, type, member)

#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); \
        pos = pos->next)
#define list_for_each_entry(pos, head, member) \
    for (pos = list_entry((head)->next, typeof(*pos), member); \
         &pos->member != (head); \
         pos = list_entry(pos->member.next, typeof(*pos), member))
/* safe against removal of list entry */
#define list_for_each_entry_safe(pos, n, head, member) \
    for (pos = list_entry((head)->next, typeof(*pos), member), \
        n = list_entry(pos->member.next, typeof(*pos), member); \
        &pos->member != (head); \
        pos = n, n = list_entry(n->member.next, typeof(*n), member))

/*
    doubly linked list, optimized memory usage for hash table
    ref: <linux/list.h>
*/
struct hlist_head {
    struct hlist_node *first;
};
struct hlist_node {
    struct hlist_node *next, **pprev;
};

#define HLIST_HEAD_INIT { .first = NULL } 
#define HLIST_HEAD(name) struct hlist_head name = {  .first = NULL }
#define INIT_HLIST_HEAD(ptr) ((ptr)->first = NULL) 
#define INIT_HLIST_NODE(ptr) ((ptr)->next = NULL, (ptr)->pprev = NULL)

static inline int hlist_empty(const struct hlist_head *head)
{
    return !head->first;
}

static inline void hlist_del(struct hlist_node *node)
{
    struct hlist_node *next = node->next;
    struct hlist_node **pprev = node->pprev;
    *pprev = next;
    if (next)
        next->pprev = pprev;
}

static inline void hlist_add_head(struct hlist_node *new, struct hlist_head *head)
{
    struct hlist_node *first = head->first;
    new->pprev = &(head->first);
    new->next = first;
    if (first)
        first->pprev = &(new->next);
    head->first = new;
}

/* next must be != NULL */
static inline void hlist_add_before(struct hlist_node *new, struct hlist_node *node)
{
    new->pprev = node->pprev;
    new->next = node;
    node->pprev = &(new->next);
    *(new->pprev) = new;
}

/* next must be != NULL */
/* WARNING: this is different from linux's implementation */
static inline void hlist_add_after(struct hlist_node *new, struct hlist_node *node)
{
    struct hlist_node *next = node->next;
    new->pprev = &(node->next);
    new->next = next;
    if (next)
        next->pprev = &(new->next);
    node->next = new;
}

#define hlist_entry(ptr, type, member) \
    container_of(ptr,type,member)
#define hlist_for_each(pos, head) \
    for (pos = (head)->first; \
        pos; \
        pos = pos->next)
#define hlist_for_each_entry(tpos, pos, head, member) \
    for (pos = (head)->first; \
         pos && ({ tpos = hlist_entry(pos, typeof(*tpos), member); 1;}); \
         pos = pos->next)
#define hlist_for_each_entry_safe(tpos, pos, n, head, member)        \
    for (pos = (head)->first;                    \
         pos && ({ n = pos->next; 1; }) &&               \
        ({ tpos = hlist_entry(pos, typeof(*tpos), member); 1;}); \
         pos = n)

#endif

