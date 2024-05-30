#pragma once

// Structure for the linked list
struct LinkedList {
    void* data;
    struct LinkedList *next;
};

// Predicate to find the data in the linked list
typedef int (*Predicate)(void* inList, void* data);

// Function to clean the data
typedef void (*CleanFunc)(void* data);

// Initialize a linked list
struct LinkedList* Init_linked_list();

// Add data to the linked list
void* Add_to_linked_list_if(struct LinkedList* meta, void* data, Predicate predicate);

// Delete data from the linked list
void* Delete_from_linked_list_if(struct LinkedList* meta, void* data, Predicate predicate);

// Free the linked list
void Free_linked_list(struct LinkedList* head, CleanFunc clean_data);
