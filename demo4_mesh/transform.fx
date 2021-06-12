uniform extern float4x4 g_wvp;

struct OutputVS {
    float4 pos_h : POSITION0;
};

OutputVS
TransformVS (float3 pos_l : POSITION0) {
    OutputVS vout = (OutputVS)0;
    
    vout.pos_h = mul(float4(pos_l, 1.0f), g_wvp);
    
    return vout;
}
float4
TransformPS () : COLOR{
    return float4(0.0f, 0.0f, 0.0f, 1.0f);
}

technique transform_tech {
    pass p0 {
        VertexShader = compile vs_2_0 TransformVS();
        PixelShader = compile ps_2_0 TransformPS();

        fillmode = WireFrame;
    }
}


