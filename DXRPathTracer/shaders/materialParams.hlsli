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
    uint materialBits;
};

struct Surface
{
    float3 position;
    float3 normal;
    float3 geomNormal;
    float3 interpolatedNormal;
    MaterialParams material;
    bool isIgnoreHit;
};

bool isTransparentMaterial(in MaterialParams material)
{
    return (material.transRatio > 0);
}

bool isSpecularMaterial(in MaterialParams material)
{
    return (material.metallic > 0.5) && (material.roughness < 0.5);
}

bool hasDiffuseTex(in MaterialParams material)
{
    return (material.materialBits >> 0) & 1;
}

bool hasAlphaMask(in MaterialParams material)
{
    return (material.materialBits >> 1) & 1;
}

bool hasBumpMap(in MaterialParams material)
{
    return (material.materialBits >> 2) & 1;
}

bool hasNormalMap(in MaterialParams material)
{
    return (material.materialBits >> 3) & 1;
}

bool hasRoughnessMap(in MaterialParams material)
{
    return (material.materialBits >> 4) & 1;
}

bool hasMetallnessMap(in MaterialParams material)
{
    return (material.materialBits >> 5) & 1;
}

bool isSSSExecutable(in MaterialParams material)
{
    return material.isSSSExecutable && (material.transRatio < 0.1f) && (material.roughness > 0.2f);
}

bool isNEEExecutable(in MaterialParams material)
{
    return (material.roughness > 0.1f) && !isTransparentMaterial(material);
}

struct CompressedMaterialParams
{
    uint albedoU32;
    float metallic;
    float roughness;
    float specular;
    float transRatio;
    uint transColorU32;
    uint emissionU32;
    uint isSSSExecutable;
};

MaterialParams decompressMaterialParams(in CompressedMaterialParams compressed)
{
    MaterialParams decompressed = (MaterialParams)0;
    decompressed.albedo = float4(decompressU32asRGB(compressed.albedoU32), 0);
    decompressed.metallic = compressed.metallic;
    decompressed.roughness = compressed.roughness;
    decompressed.specular = compressed.specular;
    decompressed.transRatio = compressed.transRatio;
    decompressed.transColor = float4(decompressU32asRGB(compressed.transColorU32), 0);
    decompressed.emission = float4(decompressU32asRGB(compressed.emissionU32), 0);
    decompressed.isSSSExecutable = compressed.isSSSExecutable;

    return decompressed;
}

CompressedMaterialParams compressMaterialParams(in MaterialParams original)
{
    CompressedMaterialParams compressed = (CompressedMaterialParams)0;
    compressed.albedoU32 = compressRGBasU32(original.albedo.xyz);
    compressed.metallic = original.metallic;
    compressed.roughness = original.roughness;
    compressed.specular = original.specular;
    compressed.transRatio = original.transRatio;
    compressed.transColorU32 = compressRGBasU32(original.transColor.xyz);
    compressed.emissionU32 = compressRGBasU32(original.emission.xyz);
    compressed.isSSSExecutable = original.isSSSExecutable;

    return compressed;
}

float3 modulatedAlbedo(in MaterialParams material)
{
    float3 modulatedColor = 1.xxx;
    float3 matAlbedo = material.albedo.xyz;
    if(matAlbedo.r > 0)
    {
        modulatedColor.r = matAlbedo.r;
    }
    if(matAlbedo.g > 0)
    {
        modulatedColor.g = matAlbedo.g;
    }
    if(matAlbedo.b > 0)
    {
        modulatedColor.b = matAlbedo.b;
    }
    return modulatedColor;
}

#endif//__MATERIAL_PARAMS_HLSLI__