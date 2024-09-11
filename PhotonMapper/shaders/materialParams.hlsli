#ifndef __MATERIAL_PARAMS_HLSLI__
#define __MATERIAL_PARAMS_HLSLI__

struct MaterialParams
{
    float4 albedo;
    float metallic;
    float roughness;
    float specular;
    float transRatio;
    float4 transColor;
    float4 emission;
    uint isSSSExecutable;
};

bool isSSSExecutable(in MaterialParams material)
{
    return material.isSSSExecutable && (material.transRatio < 0.1f) && (material.roughness > 0.2f);
}

bool isNEEExecutable(in MaterialParams material)
{
    return (material.roughness > 0.1f) && (material.transRatio == 0);
}

struct CompressedMaterialParams
{
    uint albedo;
    float metallic;
    float roughness;
    float specular;
    float transRatio;
    uint transColor;
    uint emission;
    uint isSSSExecutable;
};

MaterialParams decompressMaterialParams(in CompressedMaterialParams compressed)
{
    MaterialParams decompressed = (MaterialParams)0;
    decompressed.albedo = float4(U32toF32x3(compressed.albedo), 0);
    decompressed.metallic = compressed.metallic;
    decompressed.roughness = compressed.roughness;
    decompressed.specular = compressed.specular;
    decompressed.transRatio = compressed.transRatio;
    decompressed.transColor = float4(U32toF32x3(compressed.transColor), 0);
    decompressed.emission = float4(U32toF32x3(compressed.emission), 0);
    decompressed.isSSSExecutable = compressed.isSSSExecutable;

    return decompressed;
}

CompressedMaterialParams compressMaterialParams(in MaterialParams original)
{
    CompressedMaterialParams compressed = (CompressedMaterialParams)0;
    compressed.albedo = F32x3toU32(original.albedo.xyz);
    compressed.metallic = original.metallic;
    compressed.roughness = original.roughness;
    compressed.specular = original.specular;
    compressed.transRatio = original.transRatio;
    compressed.transColor = F32x3toU32(original.transColor.xyz);
    compressed.emission = F32x3toU32(original.emission.xyz);
    compressed.isSSSExecutable = original.isSSSExecutable;

    return compressed;
}

#endif//__MATERIAL_PARAMS_HLSLI__