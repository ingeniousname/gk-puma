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
	Mesh shadowMesh;
	std::vector<VertexPositionNormal> vertices;
	std::vector<Edge> edges;
	std::vector<Face> faces;
	bool FacingFront(const Face& face, const XMVECTOR& lightPos, const std::vector<VertexPositionNormal>& worldVertices);
	void generateExtrudedQuadForEdgeWithCaps(
		const Edge& edge,
		const std::vector<VertexPositionNormal>& worldVertices,
		const XMVECTOR& lightPos,
		float extrusionDistance,
		std::vector<VertexPositionNormal>& shadowVertices,
		std::vector<unsigned short>& shadowIndices
	);
public:
	void Render(const dx_ptr<ID3D11DeviceContext>& context) const;
	void RenderShadowVolume(const dx_ptr<ID3D11DeviceContext>& context) const;
	void GenerateShadowVolume(const DxDevice& device, XMFLOAT3 lightPos, XMFLOAT4X4 worldMtx, float extrusionDistance);
	static SMMesh LoadMesh(const DxDevice& device, const std::wstring& meshPath);
	
};


