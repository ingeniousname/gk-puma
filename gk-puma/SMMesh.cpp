#include "SMMesh.h"
#include <fstream>

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
	const std::vector<VertexPositionNormal>& worldVertices,
	const XMVECTOR& lightPos,
	float extrusionDistance,
	std::vector<VertexPositionNormal>& shadowVertices,
	std::vector<unsigned short>& shadowIndices
) {
	int baseIndex = shadowVertices.size();

	// Load vertex positions
	XMVECTOR p0 = XMLoadFloat3(&worldVertices[edge.v0].position);
	XMVECTOR p1 = XMLoadFloat3(&worldVertices[edge.v1].position);

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
	shadowIndices.push_back(baseIndex + 1);
	shadowIndices.push_back(baseIndex + 2);
	shadowIndices.push_back(baseIndex + 0);
	shadowIndices.push_back(baseIndex + 2);
	shadowIndices.push_back(baseIndex + 3);

	// bad doubling
	shadowIndices.push_back(baseIndex + 0);
	shadowIndices.push_back(baseIndex + 2);
	shadowIndices.push_back(baseIndex + 1);
	shadowIndices.push_back(baseIndex + 0);
	shadowIndices.push_back(baseIndex + 3);
	shadowIndices.push_back(baseIndex + 2);
}

void SMMesh::GenerateShadowVolume(const DxDevice& device, XMFLOAT3 lightPos, XMFLOAT4X4 worldMtx, float extrusionDistance)
{
	std::vector<VertexPositionNormal> worldVertices(vertices.size());

	std::vector<VertexPositionNormal> shadowVerticies;
	std::vector<unsigned short> shadowIndicies;

	XMMATRIX m = XMLoadFloat4x4(&worldMtx);
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
			generateExtrudedQuadForEdgeWithCaps(edge, worldVertices, lightPosV, extrusionDistance, shadowVerticies, shadowIndicies);
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
			// reverse winding order 
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
	vector<XMFLOAT3> positions(k);
	for (int i = 0; i < k; ++i)
		input >> positions[i].x >> positions[i].y >> positions[i].z;

	input >> l;
	mesh.vertices.resize(l);
	for (int i = 0; i < l; ++i) {
		int posIndex;
		XMFLOAT3 normal;
		input >> posIndex >> normal.x >> normal.y >> normal.z;
		mesh.vertices[i].position = positions[posIndex];
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
