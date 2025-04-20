cbuffer cbView : register(b1) //Vertex Shader constant buffer slot 1
{
	matrix viewMatrix;
};

struct VSInput
{
	float3 pos : POSITION0;
	float3 prevpos : POSITION1;
	float age : TEXCOORD0;
	float size : TEXCOORD1;
};

struct GSInput
{
	float4 pos : POSITION0;
	float4 prevpos : POSITION1;
	float age : TEXCOORD0;
	float size : TEXCOORD1;
};

GSInput main(VSInput i)
{
	GSInput o = (GSInput)0;
	o.pos = float4(i.pos, 1.0f);
	o.pos = mul(viewMatrix, o.pos);
	o.prevpos = float4(i.prevpos, 1.0f);
	o.prevpos = mul(viewMatrix, o.prevpos);
	o.age = i.age;
	o.size = i.size;
	return o;
}