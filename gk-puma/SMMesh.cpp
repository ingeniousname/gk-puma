#include "SMMesh.h"
#include <fstream>
#include <map>

using namespace std;

bool SMMesh::FacingFront(const Face& face, const XMVECTOR& lightPos, const std::vector<VertexPositionNormal>& worldVertices)
{
	XMVECTOR center = XMVectorSet(
		(worldVertices[face.indices[0]].position.x + worldVertices[face.indices[0]].position.x + worldVertices[face.indices[0]].position.x) / 3,
		(worldVertices[face.indices[1]].position.y + worldVertices[face.indices[1]].position.y + worldVertices[face.indices[1]].position.y) / 3,
		(worldVertices[face.indices[2]].position.z + worldVertices[face.indices[2]].position.z + worldVertices[face.indices[2]].position.z) / 3,
		1.f
	);

	XMVECTOR lightDir = XMVector4Normalize(center - lightPos);
	return XMVectorGetX(XMVector3Dot(lightDir, XMLoadFloat3(&worldVertices[face.indices[0]].normal))) > 0.f;

}

bool SMMesh::isEdgeOriented(unsigned v0, unsigned v1, const Face& face)
{
	for (int i = 0; i < 3; ++i) {
		unsigned a = face.indices[i];
		unsigned b = face.indices[(i + 1) % 3];
		if (SameFloat3(vertices[a].position, positions[v0]) && SameFloat3(vertices[b].position, positions[v1]))
			return true; 
		if (SameFloat3(vertices[a].position, positions[v1]) && SameFloat3(vertices[b].position, positions[v0]))
			return false;
	}
	return true;
}

bool SMMesh::SameFloat3(XMFLOAT3 v1, XMFLOAT3 v2)
{
	return std::abs(v1.x - v2.x) + std::abs(v1.y - v2.y) + std::abs(v1.z - v2.z) < 1e-5;
}

void SMMesh::Render(const dx_ptr<ID3D11DeviceContext>& context) const
{
	mesh.Render(context);
}

void SMMesh::RenderShadowVolume(const dx_ptr<ID3D11DeviceContext>& context) const
{
	shadowMesh.Render(context);
}

void SMMesh::generateExtrudedQuadForEdgeWithCaps(
	const Edge& edge,
	const std::vector<XMFLOAT3>& worldPositions,
	const XMVECTOR& lightPos,
	float extrusionDistance,
	std::vector<VertexPositionNormal>& shadowVertices,
	std::vector<unsigned short>& shadowIndices
) {
	int baseIndex = shadowVertices.size();

	// Load vertex positions
	XMVECTOR p0 = XMLoadFloat3(&worldPositions[edge.v0]);
	XMVECTOR p1 = XMLoadFloat3(&worldPositions[edge.v1]);

	// Direction from vertex to light
	XMVECTOR dir0 = XMVector3Normalize(p0 - lightPos);
	XMVECTOR dir1 = XMVector3Normalize(p1 - lightPos);

	// Extruded points
	XMVECTOR p0Extruded = p0 + dir0 * extrusionDistance;
	XMVECTOR p1Extruded = p1 + dir1 * extrusionDistance;

	// Calculate normals (point towards light for top cap, opposite for bottom cap)
	XMVECTOR extrudedNormal = XMVector3Normalize(XMVector3Cross(p1 - p0, p1Extruded - p0)); // Wall normal

	// Add the 4 vertices of the quad (the wall of the shadow volume)
	for (int i = 0; i < 4; i++)
		shadowVertices.push_back({ {},{} });

	XMStoreFloat3(&shadowVertices[baseIndex].position, p0);
	XMStoreFloat3(&shadowVertices[baseIndex].normal, extrudedNormal);

	XMStoreFloat3(&shadowVertices[baseIndex + 1].position, p1);
	XMStoreFloat3(&shadowVertices[baseIndex + 1].normal, extrudedNormal);

	XMStoreFloat3(&shadowVertices[baseIndex + 2].position, p1Extruded);
	XMStoreFloat3(&shadowVertices[baseIndex + 2].normal, extrudedNormal);

	XMStoreFloat3(&shadowVertices[baseIndex + 3].position, p0Extruded);
	XMStoreFloat3(&shadowVertices[baseIndex + 3].normal, extrudedNormal);

	shadowIndices.push_back(baseIndex + 0);
	shadowIndices.push_back(baseIndex + 2);
	shadowIndices.push_back(baseIndex + 1);
	shadowIndices.push_back(baseIndex + 0);
	shadowIndices.push_back(baseIndex + 3);
	shadowIndices.push_back(baseIndex + 2);

}

void SMMesh::AddEdge(std::map<std::pair<unsigned short, unsigned short>, Edge>& edgeMap, unsigned short v0, unsigned short v1, unsigned short face)
{
	auto key = std::minmax(v0, v1); // Always (smaller, larger) to avoid duplicates
	auto it = edgeMap.find(key);
	if (it == edgeMap.end())
	{
		// New edge
		edgeMap[key] = Edge(key.first, key.second, face, UINT_MAX);
	}
	else
	{
		// Existing edge, fill second face
		it->second.face1 = face;
	}
}

void SMMesh::CylinderPositions(unsigned int stacks, unsigned int slices, float height, float radius)
{
	assert(stacks > 0 && slices > 1);

	positions.reserve((stacks + 1) * slices + 2);

	float halfHeight = height / 2.0f;
	float dp = XM_2PI / slices;
	float dy = height / stacks;

	for (unsigned int i = 0; i <= stacks; ++i)
	{
		float y = halfHeight - i * dy;
		for (unsigned int j = 0; j < slices; ++j)
		{
			float sinp, cosp;
			XMScalarSinCos(&sinp, &cosp, j * dp);
			positions.emplace_back(radius * cosp, y, radius * sinp);
		}
	}

	positions.emplace_back(0.0f, halfHeight, 0.0f);

	positions.emplace_back(0.0f, -halfHeight, 0.0f);
}

std::vector<unsigned short> SMMesh::CylinderVerts(unsigned int stacks, unsigned int slices, float height, float radius)
{
	std::vector<unsigned short> vertexPosMapping;
	vertices.reserve(positions.size());


	auto sideVerts = (stacks + 1) * slices;
	auto topCenter = sideVerts;
	auto bottomCenter = sideVerts + 1;

	for (unsigned int i = 0; i < sideVerts; ++i)
	{
		auto& pos = positions[i];

		// Normal is just (x, 0, z) normalized
		XMFLOAT3 normal(pos.x, 0.0f, pos.z);
		XMStoreFloat3(&normal, XMVector3Normalize(XMLoadFloat3(&normal)));

		vertices.push_back({ pos, normal });
		vertexPosMapping.push_back(i);
	}

	for (unsigned int j = 0; j < slices; ++j)
	{
		const auto& pos = positions[j];
		vertices.push_back({ pos, XMFLOAT3(0.0f, 1.0f, 0.0f) });
		vertexPosMapping.push_back(j);
	}

	vertices.push_back({ positions[topCenter], XMFLOAT3(0.0f, 1.0f, 0.0f) });
	vertexPosMapping.push_back(topCenter);

	unsigned int bottomStart = (stacks)*slices;
	for (unsigned int j = 0; j < slices; ++j)
	{
		const auto& pos = positions[bottomStart + j];
		vertices.push_back({ pos, XMFLOAT3(0.0f, -1.0f, 0.0f) });
		vertexPosMapping.push_back(bottomStart + j);
	}

	// Bottom center vertex
	vertices.push_back({ positions[bottomCenter], XMFLOAT3(0.0f, -1.0f, 0.0f) });
	vertexPosMapping.push_back(bottomCenter);

	return vertexPosMapping;
}

std::vector<unsigned short> SMMesh::CylinderIdx(unsigned int stacks, unsigned int slices, const std::vector<unsigned short>& vertexPosMapping)
{
	assert(vertexPosMapping.size() == vertices.size());
	std::vector<unsigned short> indices;
	std::map<std::pair<unsigned short, unsigned short>, Edge> edgeMap;
	unsigned faceIndex = 0;

	unsigned int sideVerts = (stacks + 1) * slices;
	unsigned int topRingStart = sideVerts;
	unsigned int topCenterIndex = topRingStart + slices;
	unsigned int bottomRingStart = topCenterIndex + 1;
	unsigned int bottomCenterIndex = bottomRingStart + slices;

	// === Sides ===
	for (unsigned int i = 0; i < stacks; ++i)
	{
		for (unsigned int j = 0; j < slices; ++j)
		{
			unsigned int next = (j + 1) % slices;
			unsigned int a = i * slices + j;
			unsigned int b = i * slices + next;
			unsigned int c = (i + 1) * slices + j;
			unsigned int d = (i + 1) * slices + next;

			unsigned int va = vertexPosMapping[a];
			unsigned int vb = vertexPosMapping[b];
			unsigned int vc = vertexPosMapping[c];
			unsigned int vd = vertexPosMapping[d];

			indices.push_back(a);
			indices.push_back(b);
			indices.push_back(d);
			faces.emplace_back(a, b, d);
			AddEdge(edgeMap, va, vb, faceIndex);
			AddEdge(edgeMap, vb, vd, faceIndex);
			AddEdge(edgeMap, vd, va, faceIndex);
			++faceIndex;

			indices.push_back(a);
			indices.push_back(d);
			indices.push_back(c);
			faces.emplace_back(a, d, c);
			AddEdge(edgeMap, va, vd, faceIndex);
			AddEdge(edgeMap, vd, vc, faceIndex);
			AddEdge(edgeMap, vc, va, faceIndex);
			++faceIndex;
		}
	}

	// === Top cap ===
	for (unsigned int j = 0; j < slices; ++j)
	{
		unsigned int next = (j + 1) % slices;
		unsigned int v0 = topRingStart + j;
		unsigned int v1 = topCenterIndex;
		unsigned int v2 = topRingStart + next;

		unsigned int va = vertexPosMapping[v0];
		unsigned int vb = vertexPosMapping[v1];
		unsigned int vc = vertexPosMapping[v2];

		indices.push_back(v0);
		indices.push_back(v1);
		indices.push_back(v2);
		faces.emplace_back(v0, v1, v2);
		AddEdge(edgeMap, va, vb, faceIndex);
		AddEdge(edgeMap, vb, vc, faceIndex);
		AddEdge(edgeMap, vc, va, faceIndex);
		++faceIndex;
	}

	// === Bottom cap ===
	for (unsigned int j = 0; j < slices; ++j)
	{
		unsigned int next = (j + 1) % slices;
		unsigned int v0 = bottomRingStart + next;
		unsigned int v1 = bottomCenterIndex;
		unsigned int v2 = bottomRingStart + j;

		unsigned int va = vertexPosMapping[v0];
		unsigned int vb = vertexPosMapping[v1];
		unsigned int vc = vertexPosMapping[v2];

		indices.push_back(v0);
		indices.push_back(v1);
		indices.push_back(v2);
		faces.emplace_back(v0, v1, v2);
		AddEdge(edgeMap, va, vb, faceIndex);
		AddEdge(edgeMap, vb, vc, faceIndex);
		AddEdge(edgeMap, vc, va, faceIndex);
		++faceIndex;
	}

	// === Edges ===
	edges.reserve(edgeMap.size());
	for (auto& [_, edge] : edgeMap)
	{
		edges.push_back(edge);
	}

	return indices;
}

void SMMesh::GenerateShadowVolume(const DxDevice& device, XMFLOAT3 lightPos, XMFLOAT4X4 worldMtx, float extrusionDistance)
{
	std::vector<VertexPositionNormal> worldVertices(vertices.size());
	std::vector<XMFLOAT3> worldPositions(positions.size());

	std::vector<VertexPositionNormal> shadowVerticies;
	std::vector<unsigned short> shadowIndicies;

	XMMATRIX m = XMLoadFloat4x4(&worldMtx);
	for (int i = 0; i < positions.size(); i++)
		XMStoreFloat3(&worldPositions[i], XMVector3Transform(XMLoadFloat3(&positions[i]), m));

	for (int i = 0; i < vertices.size(); i++)
	{
		XMStoreFloat3(&worldVertices[i].position, XMVector3Transform(XMLoadFloat3(&vertices[i].position), m));
		XMStoreFloat3(&worldVertices[i].normal, XMVector3Normalize(XMVector3TransformNormal(XMLoadFloat3(&vertices[i].normal), m)));
	}


	XMVECTOR lightPosV = XMLoadFloat3(&lightPos);

	// extrude edges
	for (Edge& edge : edges) {
		bool f0Front = FacingFront(faces[edge.face0], lightPosV, worldVertices);
		bool f1Front = FacingFront(faces[edge.face1], lightPosV, worldVertices);

		if (f0Front != f1Front) {
			int backFaceIndex = f0Front ? edge.face1 : edge.face0;
			Face& backFace = faces[backFaceIndex];

			unsigned ev0 = edge.v0;
			unsigned ev1 = edge.v1;

			if (!isEdgeOriented(ev0, ev1, backFace)) {
				// Flip to match backface winding
				std::swap(ev0, ev1);
			}

			generateExtrudedQuadForEdgeWithCaps(Edge(ev0, ev1, edge.face0, edge.face1), worldPositions, lightPosV, extrusionDistance, shadowVerticies, shadowIndicies);
		}
	}

	for (size_t i = 0; i < faces.size(); ++i) {
		const Face& face = faces[i];
		// top cap
		uint32_t baseIndex = shadowVerticies.size();
		if (FacingFront(face, lightPosV, worldVertices)) {

			for (int j = 0; j < 3; ++j) {
				shadowVerticies.push_back(worldVertices[face.indices[j]]);
			}

			shadowIndicies.push_back(baseIndex + 2);
			shadowIndicies.push_back(baseIndex + 1);
			shadowIndicies.push_back(baseIndex + 0);
		}
		else
		{
			for (int j = 0; j < 3; ++j) {
				XMVECTOR pos = XMLoadFloat3(&worldVertices[face.indices[j]].position);
				XMVECTOR dir = XMVector3Normalize(pos - lightPosV);
				XMVECTOR extruded = pos + dir * extrusionDistance;
				XMFLOAT3 postmp;
				XMStoreFloat3(&postmp, extruded);
				shadowVerticies.push_back({ postmp, {} });
			}
			shadowIndicies.push_back(baseIndex + 2);
			shadowIndicies.push_back(baseIndex + 1);
			shadowIndicies.push_back(baseIndex + 0);

		}
	}

	shadowMesh = Mesh::SimpleTriMesh(device, shadowVerticies, shadowIndicies);
}



SMMesh SMMesh::LoadMesh(const DxDevice& device, const std::wstring& meshPath)
{
	//File format for VN vertices and IN indices (IN divisible by 3, i.e. IN/3 triangles):
	//VN IN
	//pos.x pos.y pos.z norm.x norm.y norm.z tex.x tex.y [VN times, i.e. for each vertex]
	//t.i1 t.i2 t.i3 [IN/3 times, i.e. for each triangle]

	SMMesh mesh;
	ifstream input;
	// In general we shouldn't throw exceptions on end-of-file,
	// however, in case of this file format if we reach the end
	// of a file before we read all values, the file is
	// ill-formated and we would need to throw an exception anyway
	input.exceptions(ios::badbit | ios::failbit | ios::eofbit);
	input.open(meshPath);

	int k, l, m, n;
	input >> k;
	mesh.positions.resize(k);
	for (int i = 0; i < k; ++i)
		input >> mesh.positions[i].x >> mesh.positions[i].y >> mesh.positions[i].z;

	input >> l;
	mesh.vertices.resize(l);
	for (int i = 0; i < l; ++i) {
		int posIndex;
		XMFLOAT3 normal;
		input >> posIndex >> normal.x >> normal.y >> normal.z;
		mesh.vertices[i].position = mesh.positions[posIndex];
		mesh.vertices[i].normal = normal;
	}

	input >> m;
	vector<unsigned short> indices(m * 3);
	for (int i = 0; i < m; ++i)
	{
		unsigned v0, v1, v2;
		input >> v0 >> v1 >> v2;
		indices[3 * i] = v0;
		indices[3 * i + 1] = v1;
		indices[3 * i + 2] = v2;

		mesh.faces.push_back(Face{ v0, v1, v2 });
	}

	input >> n;
	// KRAWEDZIE
	//getline(input, skipLine); 
	for (int i = 0; i < n; ++i)
	{
		unsigned v0, v1;
		unsigned f0, f1;

		input >> v0 >> v1 >> f0 >> f1;
		mesh.edges.push_back(Edge{ v0, v1, f0, f1 });
	}
	input.close();
	mesh.mesh = Mesh::SimpleTriMesh(device, mesh.vertices, indices);
	return mesh;
}

SMMesh SMMesh::Cylinder(const DxDevice& device, unsigned int stacks, unsigned int slices, float height, float radius)
{
	SMMesh cylinder;
	cylinder.CylinderPositions(stacks, slices, height, radius);
	auto vertexPosMapping = cylinder.CylinderVerts(stacks, slices, height, radius);
	auto indices = cylinder.CylinderIdx(stacks, slices, vertexPosMapping);

	cylinder.mesh = Mesh::SimpleTriMesh(device, cylinder.vertices, indices);

	return cylinder;
}
