
#ifndef LIST_H
#define LIST_H

#ifndef NULL
#define NULL 0
#endif

struct list_head {
    struct list_head *prev, *next;
};

typedef struct list_head list_head;

#define INIT_LIST_HEAD(ptr) do{ \
    list_head *_ptr = (list_head*)ptr;  \
    (_ptr)->prev = (_ptr);  \
    (_ptr)->next = (_ptr);  \
}while(0)

static inline void __list_add(list_head* _new, list_head* prev, list_head* next)
{
    _new->prev = prev;
    prev->next = _new;
    _new->next = next;
    next->prev = _new;
}

static inline void list_add(list_head* _new, list_head* head)
{
    __list_add(_new, head, head->next);
}

static inline void list_add_tail(list_head* _new, list_head* head)
{
    __list_add(_new, head->prev, head);
}

static inline void __list_del(list_head* prev, list_head* next)
{
    prev->next = next;
    next->prev = prev;
}

static inline void list_del(list_head* entry)
{
    __list_del(entry->prev, entry->next);
}

static inline int list_empty(list_head* head)
{
    return (head->next == head) && (head->prev == head);
}

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t)&((TYPE *) 0)->MEMBER)
#endif  /* offsetof */

#define container_of(ptr, type, member) ({  \
    const typeof( ((type*)0)->member )* _ptr = (&ptr); \
    (type*) ((char*)_ptr - offsetof(type, member));    \
})

#define list_entry(ptr, type, member)  container_of(ptr, type, member)

#define list_for_each(pos, head)    \
    for(pos = (head)->next; pos != head; pos = (pos)->next)

#endif  /* LIST_H */
