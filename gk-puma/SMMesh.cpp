#include "SMMesh.h"
#include <fstream>

using namespace std;

void SMMesh::Render(const dx_ptr<ID3D11DeviceContext>& context) const
{
	mesh.Render(context);
}

SMMesh SMMesh::loadMesh(const DxDevice& device, const std::wstring& meshPath)
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
