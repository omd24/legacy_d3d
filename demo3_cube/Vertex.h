#pragma once

#include <d3dx9.h>

struct VertexPos {
    D3DXVECTOR3 pos;
    IDirect3DVertexDeclaration9 * decl;
};

inline void
InitAllVertexDeclarations (
    IDirect3DDevice9 * device,
    VertexPos * vertex_pos
) {
    //===============================================================
    // VertexPos

    D3DVERTEXELEMENT9 VertexPosElements [] =
    {
        {0, 0,  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
        D3DDECL_END()
    };
    device->CreateVertexDeclaration(VertexPosElements, &vertex_pos->decl);
}

