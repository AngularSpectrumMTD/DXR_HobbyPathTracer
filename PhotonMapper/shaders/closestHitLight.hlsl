#include "common.hlsli"

[shader("closesthit")]
void lightClosestHit(inout Payload payload, TriangleIntersectionAttributes attrib)
{
    if (payload.recursive > 0 || isReachedRecursiveLimitPayload(payload))
    {
        return;
    }
    payload.color = float3(1, 0.5, 0);
    payload.energy = 0.xxx;
}