#include "BulletArray.h"

struct BulletInfo {
    D3DXVECTOR3 pos;
    float rotation;
    float life;
};
static int const max_bullet_count = 1'000'000;
static float const bullet_speed = 2500.0f;
struct BulletArray {
    bool initialized;

    int count;
    BulletInfo * items;
};
size_t
BulletArray_CalcRequiredSize () {
    return sizeof(BulletArray) + max_bullet_count * sizeof(BulletInfo);
}
BulletArray *
BulletArray_Init (BYTE * memory) {
    BulletArray * ret = nullptr;
    if (memory) {
        ret = reinterpret_cast<BulletArray *>(memory);
        ret->items = reinterpret_cast<BulletInfo *>(memory + sizeof(BulletArray));

        ret->count = 0;
        ret->initialized = true;
    }
    return ret;
}
void
BulletArray_Deinit (BulletArray * bullets) {
    bullets->count = 0;
    bullets->items = nullptr;
    bullets->initialized = false;
}
void
BulletArray_AddItem (BulletArray * bullets, D3DXVECTOR3 pos, float rotation, float life) {
    if (bullets->initialized) {
        _ASSERT_EXPR(bullets->count < max_bullet_count, _T("out of bounds"));
        BulletInfo bullet = {
            .pos = pos,
            .rotation = rotation,
            .life = life,
        };
        bullets->items[bullets->count++] = bullet;
    }
}
void
BulletArray_RemoveItem (BulletArray * bullets, int index) {
    if (bullets->initialized && (index >= 0)) {
        _ASSERT_EXPR(index < bullets->count, _T("out of bounds"));
        if (index != (bullets->count - 1))
            bullets->items[index] = bullets->items[bullets->count - 1];
        --bullets->count;
    }
}
int
BulletArray_Count (BulletArray * bullets) {
    return bullets->count;
}
void
BulletArray_UpdateItemPos (BulletArray * bullets, int index, float dt) {
    if (bullets->initialized) {
        _ASSERT_EXPR(index < bullets->count, _T("out of bounds"));
                // Accumualte the time the bullet has lived.
        bullets->items[index].life += dt;

        // If the bullet has lived for two seconds, kill it.  By now the
        // bullet should have flown off the screen and cannot be seen.
        // Otherwise, update its position by moving along its directional
        // path.  Code similar to how we move the ship--but no drag.
        if (bullets->items[index].life >= 2.0f) {
            BulletArray_RemoveItem(bullets, index);
        } else {
            D3DXVECTOR3 dir(
                -sinf(bullets->items[index].rotation),
                cosf(bullets->items[index].rotation),
                0.0f
            );
            bullets->items[index].pos += dir * bullet_speed * dt;
        }
    }
}
D3DXVECTOR3
BulletArray_GetItemPos (BulletArray * bullets, int index) {
    _ASSERT_EXPR(index < bullets->count, _T("out of bounds"));
    return bullets->items[index].pos;
}
float
BulletArray_GetItemRotation (BulletArray * bullets, int index) {
    _ASSERT_EXPR(index < bullets->count, _T("out of bounds"));
    return bullets->items[index].rotation;
}
