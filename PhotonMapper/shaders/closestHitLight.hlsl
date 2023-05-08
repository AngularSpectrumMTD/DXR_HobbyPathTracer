#include "common.hlsli"

[shader("closesthit")]
void lightClosestHit(inout Payload payload, TriangleIntersectionAttributes attrib)
{
    if (isReachedRecursiveLimitPayload(payload))
    {
        return;
    }
    payload.color = float3(1, 0.5, 0);
}