#ifndef __LIST_H__
#define __LIST_H__

#include <stdbool.h>

// macro
#define List(type) type##List // type-name of list
#define ListEntry(type) type##ListEntry // type-name of entry of list
#define new_list(type) new_##type##List // function name to make a new list
#define new_list_entry(type) new_##type##ListEntry // function name to make a new entry of list
#define get_length(type) get_length_##type // function name to get the length of list
#define get_element(type) get_element_##type // function name to get the element of an entry of list
#define get_first_entry(type) get_first_entry_##type // function name to get the first entry of list
#define get_first_element(type) get_first_element_##type // function name to get the element of the first entry of list
#define get_last_entry(type) get_last_entry_##type // function name to get the last entry of list
#define set_first_entry(type) set_first_entry_##type // function name to set the first entry of list
#define end_iteration(type) end_iteration_##type // function name to check if iteration is over
#define add_list_entry_head(type) add_list_entry_head_##type // function name to add an entry at the head of list
#define add_list_entry_tail(type) add_list_entry_tail_##type // function name to add an entry at the tail of list
#define concatenate_list(type) concatenate_list_##type // function name to concatenate lists
#define prev_entry(type, entry) ((entry)->prev) // get the previous entry of list
#define next_entry(type, entry) ((entry)->next) // get the next entry of list
#define for_each_entry(type, cursor, list) for(ListEntry(type) *cursor = get_first_entry(type)(list); !end_iteration(type)(list, cursor); cursor = next_entry(type, cursor)) // iterator over list
#define for_each_entry_reversed(type, cursor, list) for(ListEntry(type) *cursor = get_last_entry(type)(list); !end_iteration(type)(list, cursor); cursor = prev_entry(type, cursor)) // iterator over list in reverse order

#define define_list(type) \
/* definition of structure */\
typedef struct List(type) List(type);\
typedef struct ListEntry(type) ListEntry(type);\
struct List(type) {\
    ListEntry(type) *head; /* first entry of list */\
    ListEntry(type) *tail; /* last entry of list */\
};\
struct ListEntry(type) {\
    ListEntry(type) *prev; /* pointer to the previous element */\
    ListEntry(type) *next; /* pointer to the next element */\
    type *element;         /* element of list */\
};\
/* function prototypes */\
List(type) *new_list(type)(void);\
ListEntry(type) *new_list_entry(type)(type *element);\
size_t get_length(type)(const List(type) *list);\
type *get_element(type)(const ListEntry(type) *entry);\
ListEntry(type) *get_first_entry(type)(const List(type) *list);\
type *get_first_element(type)(const List(type) *list);\
ListEntry(type) *get_last_entry(type)(const List(type) *list);\
void set_first_entry(type)(List(type) *list, ListEntry(type) *entry);\
bool end_iteration(type)(const List(type) *list, const ListEntry(type) *entry);\
ListEntry(type) *add_list_entry_head(type)(List(type) *list, type *element);\
ListEntry(type) *add_list_entry_tail(type)(List(type) *list, type *element);\
List(type) *concatenate_list(type)(List(type) *self, List(type) *other);\

#define define_list_operations(type) \
/* make a new list */\
List(type) *new_list(type)(void)\
{\
    List(type) *list = calloc(1, sizeof(List(type)));\
    ListEntry(type) *dummy_entry = new_list_entry(type)(NULL);\
    dummy_entry->next = dummy_entry;\
    dummy_entry->prev = dummy_entry;\
    list->head = dummy_entry;\
    list->tail = dummy_entry;\
\
    return list;\
}\
\
\
/* make a new entry of list */\
ListEntry(type) *new_list_entry(type)(type *element)\
{\
    ListEntry(type) *entry = calloc(1, sizeof(ListEntry(type)));\
    entry->next = NULL;\
    entry->element = element;\
\
    return entry;\
}\
\
\
/* get the length of list */\
size_t get_length(type)(const List(type) *list)\
{\
    size_t len = 0;\
    for_each_entry(type, cursor, list)\
    {\
        len++;\
    }\
\
    return len;\
}\
\
\
/* get the element of an entry of list */\
type *get_element(type)(const ListEntry(type) *entry)\
{\
    return entry->element;\
}\
\
\
/* get the first entry of list */\
ListEntry(type) *get_first_entry(type)(const List(type) *list)\
{\
    return list->head->next;\
}\
\
\
/* get the element of the first entry of list */\
type *get_first_element(type)(const List(type) *list)\
{\
    return get_element(type)(get_first_entry(type)(list));\
}\
\
\
/* get the last entry of list */\
ListEntry(type) *get_last_entry(type)(const List(type) *list)\
{\
    return list->tail->prev;\
}\
\
\
/* set the first entry of list */\
void set_first_entry(type)(List(type) *list, ListEntry(type) *entry)\
{\
    list->head->next = entry;\
    entry->prev = list->head;\
}\
\
\
/* check if iteration is over */\
bool end_iteration(type)(const List(type) *list, const ListEntry(type) *entry)\
{\
    return list->tail == entry;\
}\
\
\
/* add an entry at the head of list */\
ListEntry(type) *add_list_entry_head(type)(List(type) *list, type *element)\
{\
    ListEntry(type) *head = new_list_entry(type)(element);\
    head->prev = list->head;\
    head->next = list->head->next;\
    list->head->next->prev = head;\
    list->head->next = head;\
\
    return head;\
}\
\
\
/* add an entry at the tail of list */\
ListEntry(type) *add_list_entry_tail(type)(List(type) *list, type *element)\
{\
    ListEntry(type) *tail = new_list_entry(type)(element);\
    tail->prev = list->tail->prev;\
    tail->next = list->tail;\
    list->tail->prev->next = tail;\
    list->tail->prev = tail;\
\
    return tail;\
}\
\
\
/* concatenate lists */\
List(type) *concatenate_list(type)(List(type) *self, List(type) *other)\
{\
    ListEntry(type) *self_last = get_last_entry(type)(self);\
    ListEntry(type) *other_first = get_first_entry(type)(other);\
    ListEntry(type) *other_last = get_last_entry(type)(other);\
\
    self_last->next = other_first;\
    other_first->prev = self_last;\
    other_last->next = self->tail;\
    self->tail->prev = other_last;\
\
    return self;\
}\

#endif /* !__LIST_H__ */
