#include <stdlib.h>
#include "List.h"

static inline ListElement* NewElement(void* Data){
    ListElement* New;
    
    New = malloc(sizeof(ListElement));
    New->Data = Data;
    New->Next = NULL;
    
    return New;
}

//Creates a new List instance and prepares it for use
__LinkedList* LST_New(int (*ComparisonRoutine)(void* Data1, void* Data2)){
    __LinkedList* New;
    
    New = malloc(sizeof(__LinkedList));
    
    New->Head = NULL;
    New->Tail = NULL;
    New->RemovalRoutine = NULL;
    New->ComparisonRoutine = ComparisonRoutine;
    New->TraversalPointer = NULL;
    
    return New;
}

//Attaches a removal routine that's needed for LST_Destroy
void LST_AttachRemovalRoutine(__LinkedList* L, void (*RemovalRoutine)(void* Data1)){
    L->RemovalRoutine = RemovalRoutine;
}

//Searches for Data in L and returns a pointer to it. If not found returns NULL.
void* LST_Search(__LinkedList* L, void* Data){
    ListElement* p;
    void* retVal = NULL;
    
    L->TraversalPointer = NULL;
    
    for(p = L->Head; p != NULL; p = p->Next){
        if(!(L->ComparisonRoutine(p->Data, Data))){
            retVal = p->Data;
            L->TraversalPointer = p->Next;
            break;
        }
    }
    return retVal;
}

//Inserts Data in L. It does not guarantee any particular insertion order
void LST_Insert(__LinkedList* L, void* Data){
    ListElement* New;
    
    New = NewElement(Data);
    
    if(!(L->Head))
        L->Head = New, 
        L->Tail = New;
    else{
        L->Tail->Next = New;
        L->Tail = New;
    }
}

//Removes Data from L. Returns a pointer to data or NULL, otherwise.
void* LST_Remove(__LinkedList* L, void* Data){
    ListElement *p, *prev;
    void* retVal = NULL;
    
    prev = NULL;
    
    for(p = L->Head; p != NULL; p = p->Next){
        if(!(L->ComparisonRoutine(p->Data, Data))){
            retVal = p->Data;
            
            if(prev){
                prev->Next = p->Next;
            }
            else{
                L->Head = p->Next;
                
                if(!(p->Next))
                    L->Tail = NULL;
            }
            
            free(p);
            break;
        }
        prev = p;
    }    
    return retVal;
}

//Pushes Data onto Stack L.
void LST_Push(__LinkedList* L, void* Data){
    ListElement* New;
    
    New = NewElement(Data);
    
    New->Next = L->Head;
    L->Head = New;
    
    if(!(L->Tail))
        L->Tail = New;
}

//Pops Data from Stack L. Returns a pointer to Data.
void* LST_Pop(__LinkedList* L){
    ListElement *p;
    void* Data;
    
    p = L->Head;
    Data = NULL;
    
    if(p){
        L->Head = p->Next;
        
        if(!(L->Head))
            L->Tail = NULL;
        
        Data = p->Data;
        free(p);
    }
    
    return Data;
}

//Enqueues Data into Queue L.
void LST_Enqueue(__LinkedList* L, void* Data){
    LST_Insert(L, Data);
}

//Dequeues Data from Queue L and returns a pointer to it.
void* LST_Dequeue(__LinkedList* L){
    return LST_Pop(L);
}

//Clears all elements from the list. A removal routine must be set with
//LST_AttachRemovalRoutine
void LST_Clear(__LinkedList* L){
    ListElement *p, *prev;
    
    if (L->RemovalRoutine){
        prev = NULL;
        
        for(p = L->Head; p != NULL; p = p->Next){
            
            if(prev)
                free(prev);
            
            L->RemovalRoutine(p->Data);
            
            prev = p;
        }   
        
        free(prev);
        
        L->Head = NULL;
        L->Tail = NULL;
    }
    
}

void LST_Destroy(__LinkedList* L){
    if(L){
        while(LST_Pop(L));
        free(L);
    }
}

//Returns the next element in the list. LST_Rewind should be called
//to reset this function. Useful for traversing a list without removing
//any elements
void* LST_Next(__LinkedList *L){
    void* Element = NULL;
    
    if(L->TraversalPointer){
        Element = L->TraversalPointer->Data;
        L->TraversalPointer = L->TraversalPointer->Next;
    }
    
    return Element;
}

void LST_Rewind(__LinkedList *L){
    L->TraversalPointer = L->Head;
}
