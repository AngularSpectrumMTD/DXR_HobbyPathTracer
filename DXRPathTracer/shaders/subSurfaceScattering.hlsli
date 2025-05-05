#ifndef __SUBSURFACE_SCATTERING_HLSLI__
#define __SUBSURFACE_SCATTERING_HLSLI__

#define BSSRDF_R_MIN 1e-6f
#define BSSRDF_C 2.5715f
#define BSSRDF_PMF_AXIS 0.5f, 0.25f, 0.25f
#define BSSRDF_CHANNEL_PMF 1.0f / 3.0f
#define BSSRDF_EPS 0.01f
#define BSSRDF_TEST_IOR 1.4f

struct BSSRDFSample
{
    float3 position;
    float3 direction;
    float Tmin;
    float Tmax;
    uint axisID; //012 xyz
};

//https://www.pbr-book.org/3ed-2018/Volume_Scattering/The_BSSRDF

//=========================================================================
//BSSRDF
//=========================================================================
//https://graphics.stanford.edu/papers/fast_bssrdf/fast_bssrdf.pdf
float Fdr(float IoR)
{
    return -1.440 / IoR / IoR + 0.710 / IoR + 0.668 + 0.0636 * IoR;
}
//https://graphics.stanford.edu/papers/fast_bssrdf/fast_bssrdf.pdf
float termA(float IoR)
{
    return (1 + Fdr(IoR)) / (1 - Fdr(IoR));
}

//https://thisistian.github.io/publication/spvg_xie_2020_I3D_small.pdf
//l is mean free path
float d_length(float l, float IoR)
{
    const float A = termA(IoR);
    //return l / (1.85 - A + 7 * abs(A - 0.8) * abs(A - 0.8) * abs(A - 0.8));//case 1
    //return l / (1.9 - A + 3.5 * (A - 0.8) * (A - 0.8));//case 2
    return l / (3.5 - A + 100 * abs(A - 0.33) * abs(A - 0.33) * abs(A - 0.33) * abs(A - 0.33));//case 3
}

float3 computeBSSRDF(float r, float3 d, float IoR)
{
    r = max(r, BSSRDF_R_MIN);
    return termA(IoR) * (exp(-r / d) + exp(-r / (3 * d))) / (8 * PI * d * r);
}

//https://thisistian.github.io/publication/spvg_xie_2020_I3D_small.pdf
float sample_r_BSSRDF(float xi, float d)
{
    return d * ((2 - BSSRDF_C) * xi - 2) * log(1 - xi + BSSRDF_EPS);
}

void swapFloat3(inout float3 v0, inout float3 v1)
{
    float3 tmp = v0;
    v0 = v1;
    v1 = tmp;
}

BSSRDFSample sampleBSSRDF(float3 position, float3 normal, float3 d, float rMax, inout uint randomSeed)
{
    float u0 = rand(randomSeed);
    float u1 = rand(randomSeed);

    float axisSelectionPMF[3] = { BSSRDF_PMF_AXIS };
    float3 axis[3];
    axis[0] = normal;
    ONB(axis[0], axis[1], axis[2]);

    uint axisID;
    if(u0 < axisSelectionPMF[0])
    {
        axisID = 0;
    }
    else if(u0 < axisSelectionPMF[0] + axisSelectionPMF[1])
    {
        swapFloat3(axis[0], axis[1]);
        axisID = 1;
    }
    else
    {
        swapFloat3(axis[0], axis[2]);
        axisID = 2;
    }

    float selectedD;
    if(u1 < BSSRDF_CHANNEL_PMF)
    {
        selectedD = d.x;
    }
    else if(u1 < 2 * BSSRDF_CHANNEL_PMF)
    {
        selectedD = d.y;
    }
    else
    {
        selectedD = d.z;
    }

    float phi = 2 * PI * rand(randomSeed);
    float r = sample_r_BSSRDF(rand(randomSeed), selectedD);
    float x = r * cos(phi);
    float y = r * sin(phi);
    float3 offset = rMax * axis[0] + x * axis[1] + y * axis[2];

    float B = rMax;
    float C = dot(offset, offset) - rMax * rMax;
    float sqrtD = sqrt(max(0, B * B - C));

    BSSRDFSample bssrdfSample;
    bssrdfSample.position = position + offset;
    bssrdfSample.direction = -axis[0];
    bssrdfSample.axisID = axisID;
    bssrdfSample.Tmin = rMax - sqrtD;
    bssrdfSample.Tmax = rMax + sqrtD;

    return bssrdfSample;
}

float BSSRDF_PDF(float3 toHitPosition, float3 normal, float3 hitNormal, float3 d, uint hitCount)
{
    float pmf_axis[3] = { BSSRDF_PMF_AXIS };
    float3 axis[3];
    axis[0] = normal;
    ONB(axis[0], axis[1], axis[2]);

    float pdf = 0;
    for(int i = 0; i < 3; i++)
    {
        float NdotV = abs(dot(hitNormal, axis[i]));
        float projectedR = length(toHitPosition - dot(toHitPosition , axis[i]) * axis[i]);
        float scaling = NdotV * pmf_axis[i] * BSSRDF_CHANNEL_PMF / (1.0f * hitCount);
        float3 bssrdfPDF = computeBSSRDF(projectedR, d, BSSRDF_TEST_IOR);
        pdf += bssrdfPDF.x * scaling;
        pdf += bssrdfPDF.y * scaling;
        pdf += bssrdfPDF.z * scaling;
    }

    return pdf;
}

//BSSRDF based SSS manner
//1. Sample BSDF at exit point (and perform NEE)
//---------SSS
//2. Sample position at incident point (if not sss material, exit point is equal to incident point)
//3. Sample inner direction
//---------SSS
//4. Sample BSDF at incident point (and perform NEE)
bool computeSSSPosition(inout Payload payload, inout float3 scatterPosition, inout float3 surfaceNormal, in float3 geometryNormal)
{
    float3 normal = normalize(WorldRayDirection());
    //meanFreePath must be 1[mm] - 10[mm]
    //float3 d = float3(meanFreePath(), meanFreePath(), meanFreePath());
    //float d_length(meanFreePath(), 1.4);
    float dlen = d_length(meanFreePath(), BSSRDF_TEST_IOR);
    float3 d = float3(dlen, dlen, dlen);
    //float3 d = float3(0.01, 0.01, 0.01);//test
    float rMax = sample_r_BSSRDF(1.0f, max(d.x, max(d.y, d.z)));
    
    BSSRDFSample bssrdfSample;
    bssrdfSample = sampleBSSRDF(scatterPosition, normal, d, rMax, payload.randomSeed);

    Payload sssPayload;
    sssPayload.flags = 0;
    sssPayload.flags |= PAYLOAD_BIT_MASK_IS_SSS_RAY;
    sssPayload.hitCount = 0;
    sssPayload.T = -1;
    sssPayload.SSSnormal = float3(0, 0, 0);

    RayDesc sssRay;
    sssRay.TMin = bssrdfSample.Tmin;
    sssRay.TMax = bssrdfSample.Tmax;
    sssRay.Direction = bssrdfSample.direction;
    sssRay.Origin = bssrdfSample.position;

    RAY_FLAG flags = RAY_FLAG_NONE;

    uint rayMask = ~(LIGHT_INSTANCE_MASK);

    TraceDefaultRay(flags, rayMask, sssRay, sssPayload);

    if (sssPayload.hitCount > 0)
    {
        sssRay.Origin += normalize(sssRay.Direction) * sssPayload.T;
        const float3 toHitPosition = sssRay.Origin - scatterPosition;
        const float hitCount = sssPayload.hitCount;
        const float pdf = BSSRDF_PDF(toHitPosition, normal, surfaceNormal, d, sssPayload.hitCount);

        float3 W = computeBSSRDF(length(toHitPosition), d, BSSRDF_TEST_IOR);

        //Since the normal sampled R is very large compared to d, the exp term is almost zero.... ex) R 1 vs d 0.0000001
        payload.updateThroughputByMulitiplicationF3(W / pdf);

        //debug
        if(payload.recursive == 1)
        {
            gDebugTexture[DispatchRaysIndex().xy] = float4(0.5 * (1.xxx + sssPayload.SSSnormal), abs(dot(sssPayload.SSSnormal, surfaceNormal)));
        }

        scatterPosition = sssRay.Origin;
        surfaceNormal = sssPayload.SSSnormal;
        return true;
    }
    return false;
}

#endif//__SUBSURFACE_SCATTERING_HLSLI__