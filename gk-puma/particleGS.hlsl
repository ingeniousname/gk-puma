cbuffer cbProj : register(b0) //Geometry Shader constant buffer slot 0
{
    matrix projMatrix;
};

struct GSInput
{
    float4 pos : POSITION0;
    float4 prevpos : POSITION1;
    float age : TEXCOORD0;
    float size : TEXCOORD1;
};

struct PSInput
{
    float4 pos : SV_POSITION;
    float2 tex1 : TEXCOORD0;
    float2 tex2 : TEXCOORD1;
};


static const float MinSize = 0.05f;
static const float TimeToLive = .5f;
[maxvertexcount(4)]
void main(point GSInput inArray[1], inout TriangleStream<PSInput> ostream)
{
    GSInput i = inArray[0];
    PSInput o = (PSInput) 0;

    float3 curr = i.pos.xyz;
    float3 prev = i.prevpos.xyz;

    float3 dir = normalize(curr - prev);
    prev = curr - (dir * MinSize);
    
    float3 camToTrail = normalize(-curr);

    float3 side = normalize(cross(dir, camToTrail));

    float halfWidth = 0.005f;

    float3 offsets[2] =
    {
       -side * halfWidth,
        side * halfWidth
    };

    // Order: prev+left, prev+right, curr+left, curr+right
    float3 basePos[2] = { prev, curr };
    float2 texcoords[2] = { float2(0, 0), float2(1, 0) };

    for (int j = 0; j < 2; ++j) // prev, curr
    {
        for (int k = 0; k < 2; ++k) // left, right
        {
            float3 worldPos = basePos[j] + offsets[k];
            o.pos = mul(projMatrix, float4(worldPos, 1.0f));
            o.tex1 = float2(texcoords[j].x, k);
            o.tex2 = float2(i.age / TimeToLive, i.age / TimeToLive);
            ostream.Append(o);
        }
    }

    ostream.RestartStrip();
}

