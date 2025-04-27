#pragma once
#include "DirectXMath.h"
#include "mesh.h"
#include "vertexTypes.h"
#include <map> 

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
	std::vector<XMFLOAT3> positions;
	std::vector<VertexPositionNormal> vertices;
	std::vector<Edge> edges;
	std::vector<Face> faces;
	bool FacingFront(const Face& face, const XMVECTOR& lightPos, const std::vector<VertexPositionNormal>& worldVertices);
	bool isEdgeOriented(unsigned v0, unsigned v1, const Face& face);
	bool SameFloat3(XMFLOAT3 v1, XMFLOAT3 v2);
	void generateExtrudedQuadForEdge(
		const Edge& edge,
		const std::vector<XMFLOAT3>& worldPositions,
		const XMVECTOR& lightPos,
		float extrusionDistance,
		std::vector<VertexPositionNormal>& shadowVertices,
		std::vector<unsigned short>& shadowIndices
	);

	void AddEdge(std::map<std::pair<unsigned short, unsigned short>, Edge>& edgeMap, unsigned short v0, unsigned short v1, unsigned short face);

	void CylinderPositions(unsigned int stacks, unsigned int slices, float height, float radius);
	std::vector<unsigned short> CylinderVerts(unsigned int stacks, unsigned int slices, float height, float radius);
	std::vector<unsigned short> CylinderIdx(unsigned int stacks, unsigned int slices, const std::vector<unsigned short>& vertexPositionMapping);

	void DoubleRectPositions(float width, float height);
	std::vector<unsigned short> DoubleRectVerts();
	std::vector<unsigned short> DoubleRectIdx(const std::vector<unsigned short>& vertexPositionMapping);
public:
public:
	void Render(const dx_ptr<ID3D11DeviceContext>& context) const;
	void RenderShadowVolume(const dx_ptr<ID3D11DeviceContext>& context) const;
	void GenerateShadowVolume(const DxDevice& device, XMFLOAT3 lightPos, XMFLOAT4X4 worldMtx, float extrusionDistance);
	static SMMesh LoadMesh(const DxDevice& device, const std::wstring& meshPath);

	static SMMesh Cylinder(const DxDevice& device, unsigned int stacks, unsigned int slices, float height, float radius);
	static SMMesh DoubleRect(const DxDevice& device, float width, float height);
	
};


