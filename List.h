#ifndef LIST_H
#define	LIST_H

#ifdef	__cplusplus
extern "C" {
#endif

    //I was tired of reprogramming lists all over the place and created this
    //simple data structure that can take care of the most common list operations
    
    //A warning: Data is never freed by any destructive operations. Except in
    //LST_Clear. However, LST_Clear needs a RemovalRoutine attached with
    //LST_AttachRemovalRoutine


    typedef struct ListElement{
        void* Data;
        struct ListElement *Next;
    }ListElement;    
    
    typedef struct __LinkedList{
        ListElement* Head;
        ListElement* Tail;
        //The comparison routine is used to determine whether Data1 is greater than,
        //less than or equal to Data2.
        //It must return 0 if equal, negative if less than and positive if greater than
        int (*ComparisonRoutine)(void* Data1, void*Data2);
        
        void (*RemovalRoutine)(void*Data);
        
        //This is used by the iterator function
        ListElement* TraversalPointer;
    } __LinkedList;

    //Creates a new List instance and prepares it for use
    __LinkedList*       LST_New(int (*ComparisonRoutine)(void* Data1, void* Data2));
    
    //Attaches a removal routine that's needed for LST_Clear
    void        LST_AttachRemovalRoutine(__LinkedList* L, void (*RemovalRoutine)(void* Data1));        
    
    //Searches for Data in L and returns a pointer to it. If not found returns NULL.
    void*       LST_Search(__LinkedList* L, void* Data);
    
    //Inserts Data in L. It does not guarantee any particular insertion order
    void        LST_Insert(__LinkedList* L, void* Data);
    
    //Removes Data from L. Returns a pointer to data or NULL, otherwise.
    void*       LST_Remove(__LinkedList* L, void* Data);
    
    //Pushes Data onto Stack L.
    void        LST_Push(__LinkedList* L, void* Data);
    
    //Pops Data from Stack L. Returns a pointer to Data.
    void*       LST_Pop(__LinkedList* L);
    
    //Enqueues Data into Queue L.
    void        LST_Enqueue(__LinkedList* L, void* Data);
    
    //Dequeues Data from Queue L and returns a pointer to it.
    void*       LST_Dequeue(__LinkedList* L);
    
    //Clears all elements from the list. A removal routine must be set with
    //LST_AttachRemovalRoutine
    void        LST_Clear(__LinkedList* L);
    
    //Destroys a list. Does NOT destroy any data.
    void        LST_Destroy(__LinkedList* L);
    
    //Returns the next element in the list. LST_Rewind should be called
    //to reset this function. Useful for traversing a list without removing
    //any elements
    void*       LST_Next(__LinkedList *L);
    
    //Rewinds the list to the first element.
    void        LST_Rewind(__LinkedList *L);
    
#ifdef	__cplusplus
}
#endif

#endif	/* LIST_H */

