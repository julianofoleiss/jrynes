#ifndef DYNARRAY_H
#define	DYNARRAY_H

#include <stdint.h>

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct DynArray{
    uint32_t ElementSize;
    uint32_t IncreaseStep;
    uint32_t Pos;
    uint32_t Size;
    void* Data;
}DynArray;

//Returns a new Dynamic Array with IncreaseStep elements, each with ElementSize bytes
DynArray* DynArray_New(uint32_t ElementSize, uint32_t IncreaseStep);

//Returns the address of Position
void*    DynArray_At(DynArray* DA, uint32_t Position);

//Sets the element at Position
void     DynArray_Set(DynArray* DA, uint32_t Position, void* Element);

//Pushes an element to the end of the array. 
//This is the only function resizes the array accordingly.
void     DynArray_Push(DynArray* DA, void* Element);

//Pops the last pushed element from the array. Returns the address to it.
void*    DynArray_Pop(DynArray* DA);

//Gets the current stack offset
uint32_t DynArray_GetPos(DynArray* DA);

//Gets the current dynamic array size (in bytes)
uint32_t DynArray_GetSize(DynArray* DA);

//Clears the memory used by the dynamic array data.
void     DynArray_Clear(DynArray* DA);

#ifdef	__cplusplus
}
#endif

#endif	/* DYNARRAY_H */

