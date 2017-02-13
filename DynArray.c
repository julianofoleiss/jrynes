#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "DynArray.h"

#define DA_GET_ELEMENT_ADDRESS(Position) (((uint8_t*)DA->Data) + Position * (DA->ElementSize))

DynArray* DynArray_New(uint32_t ElementSize, uint32_t IncreaseStep){
    DynArray *New;
    
    New = malloc(sizeof(DynArray));
    
    New->ElementSize = ElementSize;
    New->Data = calloc(IncreaseStep, ElementSize);
    New->Pos = 0;
    New->Size = IncreaseStep;
    New->IncreaseStep = IncreaseStep;
    
    return New;
}

void* DynArray_At(DynArray* DA, uint32_t Position){
    if(Position < DA->Size)
        return DA_GET_ELEMENT_ADDRESS(Position);
    else
        return NULL;
}

void DynArray_Set(DynArray* DA, uint32_t Position, void* Element){
    if(Position < DA->Size)
        memcpy(DA_GET_ELEMENT_ADDRESS(Position), Element, DA->ElementSize);
}

void DynArray_Push(DynArray* DA, void* Element){
    DynArray_Set(DA, DA->Pos, Element);
    DA->Pos++;
    if(DA->Pos >= DA->Size){
        DA->Size += DA->IncreaseStep;
        DA->Data = realloc(DA->Data, DA->Size);
    }
}

void* DynArray_Pop(DynArray* DA){
    if(DA->Pos)
        DA->Pos--;
    
    return DynArray_At(DA, DA->Pos);
}

uint32_t DynArray_GetPos(DynArray* DA){
    return DA->Pos;
}

uint32_t DynArray_GetSize(DynArray* DA){
    return DA->Size;
}

void DynArray_Clear(DynArray* DA){
    free(DA->Data);
}
