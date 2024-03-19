#ifndef __SPECTRAL_RENDERING_HELPER_HLSLI__
#define __SPECTRAL_RENDERING_HELPER_HLSLI__

//=========================================================================
//Spectral Rendering Helper
//=========================================================================
#define REFLECTANCE_BOOST 4
#define LANBDA_INF_NM 770
#define LAMBDA_VIO_NM 380
#define LAMBDA_NUM 40
#define LAMBDA_STEP 10

float luminanceFromRGB(const float3 rgb)
{
    return 0.2126 * rgb.r + 0.7152 * rgb.g + 0.0722 * rgb.b;
}

//http://nalab.mind.meiji.ac.jp/2017/2018-suzuki.pdf
static float3 XYZ380to770_10nmTbl[LAMBDA_NUM] =
{
    float3(0.0014, 0, 0.0065),
    float3(0.0042, 0.0001, 0.0201),
    float3(0.0143, 0.0004, 0.0679),
    float3(0.0435, 0.0012, 0.2074),
    float3(0.1344, 0.004, 0.6456),
    float3(0.2839, 0.0116, 1.3856),
    float3(0.3483, 0.023, 1.7471),
    float3(0.3362, 0.038, 1.7721),
    float3(0.2908, 0.06, 1.6692),
    float3(0.1954, 0.091, 1.2876),
    float3(0.0956, 0.139, 0.813),
    float3(0.032, 0.208, 0.4652),
    float3(0.0049, 0.323, 0.272),
    float3(0.0093, 0.503, 0.1582),
    float3(0.0633, 0.71, 0.0782),
    float3(0.1655, 0.862, 0.0422),
    float3(0.294, 0.954, 0.0203),
    float3(0.4334, 0.995, 0.0087),
    float3(0.5945, 0.995, 0.0039),
    float3(0.7621, 0.952, 0.0021),
    float3(0.9163, 0.87, 0.0017),
    float3(1.0263, 0.757, 0.0011),
    float3(1.0622, 0.631, 0.0008),
    float3(1.0026, 0.503, 0.0003),
    float3(0.8544, 0.381, 0.0002),
    float3(0.6424, 0.265, 0),
    float3(0.4479, 0.175, 0),
    float3(0.2835, 0.107, 0),
    float3(0.1649, 0.061, 0),
    float3(0.0874, 0.032, 0),
    float3(0.0468, 0.017, 0),
    float3(0.0227, 0.0082, 0),
    float3(0.0114, 0.0041, 0),
    float3(0.0058, 0.0021, 0),
    float3(0.0029, 0.001, 0),
    float3(0.0014, 0.0005, 0),
    float3(0.0007, 0.0003, 0),
    float3(0.0003, 0.0001, 0),
    float3(0.0002, 0.0001, 0),
    float3(0.0001, 0, 0)
};
static float Rmin = -0.925239563;
static float Gmin = -0.221841574;
static float Bmin = -0.169706330;

static float Rmax = 2.47556806;
static float Gmax = 1.50557864;
static float Bmax = 1.88461602;

float3 lambda2XYZ(float lambdaNM)
{
    float fid = clamp((lambdaNM - LAMBDA_VIO_NM) / LAMBDA_STEP, 0, LAMBDA_NUM);
    int baseID = int(fid + 0.5);
    float t = fid - baseID;
    float3 XYZ0 = XYZ380to770_10nmTbl[baseID];
    float3 XYZ1 = XYZ380to770_10nmTbl[baseID + 1];
    return lerp(XYZ0, XYZ1, t);
}

float gamma(float val)
{
    if (val <= 0.0031308)
    {
        return val * 12.92;
    }
    else
    {
        return pow(val, 1.0 / 2.4) * 1.055 - 0.055;
    }
}

static float3x3 XYZtoRGB =
{
    +3.240479, -1.537150, -0.498535,
    -0.969256, +1.875992, +0.041556,
    +0.055648, -0.204043, +1.057311
};

static float3x3 XYZtoRGB2 =
{
    +2.3706743, -0.51388850, +0.0052982,
    -0.9000405, +1.4253036, -0.0146949,
    -0.470638, +0.0885814, +1.0093968
};

float3 integralColor()
{
    int i = 0;
    float3 XYZ = 0.xxx;
    for (i = 0; i < LAMBDA_NUM; i++)
    {
        XYZ += XYZ380to770_10nmTbl[i];
    }

    return mul(XYZ, XYZtoRGB2);
}

float3 lambda2sRGB_D65_BT709(float lambdaNM)
{
    float3 XYZ = lambda2XYZ(lambdaNM);

    float3 RGB = mul(XYZtoRGB, XYZ);

    RGB.r = gamma((RGB.r - Rmin) / (Rmax - Rmin));
    RGB.g = gamma((RGB.g - Gmin) / (Gmax - Gmin));
    RGB.b = gamma((RGB.b - Bmin) / (Bmax - Bmin));

    return RGB;
}

float3 lambda2RGB(float lambda)
{
    float b = 0.2 * (lerp(1, 1, (lambda - 380) / (490 - 380)) + lerp(1, 0, (lambda - 490) / (510 - 490)));
    if (lambda == 490)
        b *= 0.5;
    float g = 0.3 * (lerp(0, 1, (lambda - 440) / (490 - 440)) + lerp(1, 1, (lambda - 490) / (580 - 490)) + lerp(1, 0, (lambda - 580) / (645 - 580)));
    if (lambda == 490 || lambda == 580)
        g *= 0.5;
    float r = lerp(0, 1, (lambda - 350) / (440 - 350)) + lerp(0, 1, (lambda - 510) / (580 - 510)) + lerp(1, 0, (lambda - 580) / (700 - 580));
    if (lambda == 580)
        r *= 0.5;

    return 0.3 * float3(r, g, b);
}

float3 lambda2RGB_Fluorescent(float lambda)
{
    float b = lerp(0, 1, (lambda - 380) / (460 - 400)) + lerp(1, 0, (lambda - 460) / (500 - 460));
    if (lambda == 460)
        b *= 0.5;
    float g = 0.6 * (lerp(0, 1, (lambda - 460) / (500 - 460)) + lerp(1, 0, (lambda - 500) / (530 - 500)));
    if (lambda == 500)
        g *= 0.5;
    float r = 0.5 * (lerp(0, 1, (lambda - 520) / (580 - 520)) + lerp(1, 0, (lambda - 580) / (700 - 580)));
    if (lambda == 580)
        r *= 0.5;

    return 0.3 * float3(r, g, b);
}

float3 lambda2RGB_IncandescentBulb(float lambda)
{
    float b = 0.1 * (lerp(0, 1, (lambda - 380) / (460 - 400)) + lerp(1, 0, (lambda - 460) / (500 - 460)));
    if (lambda == 460)
        b *= 0.5;
    float g = 0.3 * (lerp(0, 1, (lambda - 460) / (530 - 460)) + lerp(1, 0, (lambda - 530) / (560 - 530)));
    if (lambda == 530)
        g *= 0.5;
    float r = lerp(0, 1, (lambda - 500) / (780 - 500));

    return 0.6 * float3(r, g, b);
}

float3 getBaseLightColor(float lambda)
{
    float3 color = 0.xxx;
    switch (getSpectrumMode())
    {
        case 0:
            color = lambda2sRGB_D65_BT709(lambda);
            break;
        case 1:
            color = lambda2RGB(lambda);
            break;
        case 2:
            color = lambda2RGB_Fluorescent(lambda);
            break;
        case 3:
            color = lambda2RGB_IncandescentBulb(lambda);
            break;
        default:
            color = 0.xxx;
            break;
    }
    return color;
}

float3 getBaseLightXYZ(float lambda)
{
    return lambda2XYZ(lambda);
}

#endif//__SPECTRAL_RENDERING_HELPER_HLSLI__