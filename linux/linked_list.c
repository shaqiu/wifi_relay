#include "linked_list.h"
#include <stdlib.h>

// This function new a linked list node
struct LinkedList* _new_linked_list_node() {
    struct LinkedList* new_node = (struct LinkedList*)malloc(sizeof(struct LinkedList));
    if (!new_node)
    {
        return NULL;
    }
    
    new_node->next = NULL;
    
    return new_node;
}

// This function initializes a linked list and returns a pointer to the meta head of the list
struct LinkedList* Init_linked_list() {
    struct LinkedList* meta = _new_linked_list_node();
    if (!meta)
    {
        return NULL;
    }
    
    meta->next = NULL;
    meta->data = NULL;
    
    return meta;
}

// search for a node that satisfies the predicate, return the previous node
struct LinkedList* _search_if(struct LinkedList* meta, Predicate predicate, void* data) {
    if (meta == NULL)
    {
        return NULL;
    }
    
    struct LinkedList* prev = meta;
    while (prev->next != NULL) {
        if(predicate(prev->next->data, data))   // arg1: data in list; arg2: data out of list
        {
            return prev;
        }
        prev = prev->next;
    }

    return NULL;
}

// add new_node to the linked list if the predicate is satisfied, or replace the node that satisfies the predicate with data
void* Add_to_linked_list_if(struct LinkedList* meta, void* data, Predicate predicate) {
    if(!meta)
    {
        return NULL;
    }
    
    // create a new node with generic data
    struct LinkedList* new_node = _new_linked_list_node();
    if(!new_node)
    {
        return NULL;
    }
    new_node->data = data;

    struct LinkedList* prev = NULL;
    prev = (predicate == NULL ? NULL : _search_if(meta, predicate, data));
    // if predicate is NULL or search fails, new node is added as the first node of the list
    if (prev == NULL)
    {
        new_node->next = meta->next;
        meta->next = new_node;

        return NULL;
    }

    // add new node and free the duplicate node
    struct LinkedList* to_delete = prev->next;
    new_node->next = to_delete->next;
    prev->next = new_node;

    void* old_data = to_delete->data;
    // free the duplicate node found with predicate
    free(to_delete);

    return old_data;
;
}

void* Delete_from_linked_list_if(struct LinkedList* meta, void* data, Predicate predicate) {
    if (meta == NULL)
    {
        return NULL;
    }

    struct LinkedList* to_delete = NULL;
    struct LinkedList* prev = meta;
    void* old_data = NULL;

    while (prev->next != NULL) {
        struct LinkedList* new_prev = _search_if(prev, predicate, data);
        if(new_prev == NULL)
        {
            break;
        }

        to_delete = new_prev->next;

        new_prev->next = to_delete->next;
        
        old_data = to_delete->data;
        free(to_delete);

        break;
    }

    return old_data;
}


void Free_linked_list(struct LinkedList* meta, CleanFunc clean_data) {
    struct LinkedList* curr = meta;
    struct LinkedList* next = NULL;
    
    while(curr)
    {
        next = curr->next;

        clean_data(curr->data);
        free(curr);

        curr = next;
    }
}