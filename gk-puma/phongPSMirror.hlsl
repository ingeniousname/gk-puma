cbuffer cbSurfaceColor : register(b0)
{
    float4 surfaceColor;
};

cbuffer cbLights : register(b1)
{
    float4 lightPos;
};

cbuffer cbShadowControl : register(b2)
{
    int4 isInShadow;
};

struct PSInput
{
    float4 pos : SV_POSITION;
    float3 worldPos : POSITION0;
    float3 norm : NORMAL0;
    float3 viewVec : TEXCOORD0;
    float clipDist : SV_ClipDistance0;
};

static const float3 ambientColor = float3(0.2f, 0.2f, 0.2f);
static const float3 lightColor = float3(1.0f, 1.0f, 1.0f);
static const float kd = 0.5, ks = 0.2f, m = 100.0f;

float4 main(PSInput i) : SV_TARGET
{
    //clip(-i.clipDist);
    //float3 newColor;
    //if(i.clipDist >= 0)
        //newColor = float3(1.f, 0.f, 0.f);
    //else newColor = float3(0.f, 1.f, 0.f);


    
    float3 viewVec = normalize(i.viewVec);
    float3 normal = normalize(i.norm);
    float3 color = surfaceColor.rgb * ambientColor;
    //float3 color = newColor * ambientColor;

    if (!isInShadow.x)
    {
        float3 lightPosition = lightPos.xyz;
        float3 lightVec = normalize(lightPosition - i.worldPos);
        float3 halfVec = normalize(viewVec + lightVec);
        color += lightColor * surfaceColor.rgb * kd * saturate(dot(normal, lightVec)); //diffuse color
        //color += lightColor * newColor * kd * saturate(dot(normal, lightVec)); //diffuse color
        float nh = dot(normal, halfVec);
        nh = saturate(nh);
        nh = pow(nh, m);
        nh *= ks;
        color += lightColor * nh;
    }

    return float4(saturate(color), surfaceColor.a);
}