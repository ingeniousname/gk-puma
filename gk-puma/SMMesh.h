#pragma once
#include "DirectXMath.h"
#include "mesh.h"
#include "vertexTypes.h"

using namespace DirectX;
using namespace mini;

struct Edge {
	unsigned v0, v1;
	unsigned face0, face1;
	
	Edge() {}

	Edge(unsigned v0, unsigned v1, unsigned face0, unsigned face1)
		: v0(v0), v1(v1), face0(face0), face1(face1)
	{
	}
};

struct Face
{
	unsigned indices[3];
	Face() {}
	Face(unsigned v0, unsigned v1, unsigned v2)
	{
		indices[0] = v0;
		indices[1] = v1;
		indices[2] = v2;
	}
};

class SMMesh
{
	Mesh mesh;
	std::vector<VertexPositionNormal> vertices;
	std::vector<Edge> edges;
	std::vector<Face> faces;
public:
	void Render(const dx_ptr<ID3D11DeviceContext>& context) const;
	static SMMesh loadMesh(const DxDevice& device, const std::wstring& meshPath);
};


