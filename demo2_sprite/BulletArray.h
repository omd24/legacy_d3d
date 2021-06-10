#pragma once

#include "Common.h"

struct BulletArray;

size_t
BulletArray_CalcRequiredSize ();
BulletArray *
BulletArray_Init (BYTE * memory);
void
BulletArray_Deinit (BulletArray * bullets);
void
BulletArray_AddItem (BulletArray * bullets, D3DXVECTOR3 pos, float rotation, float life);
void
BulletArray_RemoveItem (BulletArray * bullets, int index);
int
BulletArray_Count (BulletArray * bullets);
void
BulletArray_UpdateItemPos (BulletArray * bullets, int index, float dt);
D3DXVECTOR3
BulletArray_GetItemPos (BulletArray * bullets, int index);
float
BulletArray_GetItemRotation (BulletArray * bullets, int index);
