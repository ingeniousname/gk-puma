#include "mesh.h"
#include <algorithm>
#include <fstream>

using namespace std;
using namespace mini;
using namespace DirectX;

Mesh::Mesh()
	: m_indexCount(0), m_primitiveType(D3D_PRIMITIVE_TOPOLOGY_UNDEFINED)
{ }

Mesh::Mesh(dx_ptr_vector<ID3D11Buffer>&& vbuffers, vector<unsigned int>&& vstrides, vector<unsigned int>&& voffsets,
	dx_ptr<ID3D11Buffer>&& indices, unsigned int indexCount, D3D_PRIMITIVE_TOPOLOGY primitiveType)
{
	assert(vbuffers.size() == voffsets.size() && vbuffers.size() == vstrides.size());
	m_indexCount = indexCount;
	m_primitiveType = primitiveType;
	m_indexBuffer = move(indices);

	m_vertexBuffers = std::move(vbuffers);
	m_strides = move(vstrides);
	m_offsets = move(voffsets);
}

Mesh::Mesh(Mesh&& right) noexcept
	: m_indexBuffer(move(right.m_indexBuffer)), m_vertexBuffers(move(right.m_vertexBuffers)),
	m_strides(move(right.m_strides)), m_offsets(move(right.m_offsets)),
	m_indexCount(right.m_indexCount), m_primitiveType(right.m_primitiveType)
{
	right.Release();
}

void Mesh::Release()
{
	m_vertexBuffers.clear();
	m_strides.clear();
	m_offsets.clear();
	m_indexBuffer.reset();
	m_indexCount = 0;
	m_primitiveType = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
}

Mesh& Mesh::operator=(Mesh&& right) noexcept
{
	Release();
	m_vertexBuffers = move(right.m_vertexBuffers);
	m_indexBuffer = move(right.m_indexBuffer);
	m_strides = move(right.m_strides);
	m_offsets = move(right.m_offsets);
	m_indexCount = right.m_indexCount;
	m_primitiveType = right.m_primitiveType;
	right.Release();
	return *this;
}

void Mesh::Render(const dx_ptr<ID3D11DeviceContext>& context) const
{
	if (!m_indexBuffer || m_vertexBuffers.empty())
		return;
	context->IASetPrimitiveTopology(m_primitiveType);
	context->IASetIndexBuffer(m_indexBuffer.get(), DXGI_FORMAT_R16_UINT, 0);
	context->IASetVertexBuffers(0, m_vertexBuffers.size(), m_vertexBuffers.data(), m_strides.data(), m_offsets.data());
	context->DrawIndexed(m_indexCount, 0, 0);
}

Mesh::~Mesh()
{
	Release();
}

std::vector<VertexPositionColor> mini::Mesh::ColoredBoxVerts(float width, float height, float depth)
{
	return {
		//Front Face
		{ { -0.5f * width, -0.5f * height, -0.5f * depth }, { 1.0f, 0.0f, 0.0f } },
		{ { +0.5f * width, -0.5f * height, -0.5f * depth }, { 1.0f, 0.0f, 0.0f } },
		{ { +0.5f * width, +0.5f * height, -0.5f * depth }, { 1.0f, 0.0f, 0.0f } },
		{ { -0.5f * width, +0.5f * height, -0.5f * depth }, { 1.0f, 0.0f, 0.0f } },

		//Back Face
		{ { +0.5f * width, -0.5f * height, +0.5f * depth }, { 0.0f, 1.0f, 1.0f } },
		{ { -0.5f * width, -0.5f * height, +0.5f * depth }, { 0.0f, 1.0f, 1.0f } },
		{ { -0.5f * width, +0.5f * height, +0.5f * depth }, { 0.0f, 1.0f, 1.0f } },
		{ { +0.5f * width, +0.5f * height, +0.5f * depth }, { 0.0f, 1.0f, 1.0f } },

		//Left Face
		{ { -0.5f * width, -0.5f * height, +0.5f * depth }, { 0.0f, 1.0f, 0.0f } },
		{ { -0.5f * width, -0.5f * height, -0.5f * depth }, { 0.0f, 1.0f, 0.0f } },
		{ { -0.5f * width, +0.5f * height, -0.5f * depth }, { 0.0f, 1.0f, 0.0f } },
		{ { -0.5f * width, +0.5f * height, +0.5f * depth }, { 0.0f, 1.0f, 0.0f } },

		//Right Face
		{ { +0.5f * width, -0.5f * height, -0.5f * depth }, { 1.0f, 0.0f, 1.0f } },
		{ { +0.5f * width, -0.5f * height, +0.5f * depth }, { 1.0f, 0.0f, 1.0f } },
		{ { +0.5f * width, +0.5f * height, +0.5f * depth }, { 1.0f, 0.0f, 1.0f } },
		{ { +0.5f * width, +0.5f * height, -0.5f * depth }, { 1.0f, 0.0f, 1.0f } },

		//Bottom Face
		{ { -0.5f * width, -0.5f * height, +0.5f * depth }, { 0.0f, 0.0f, 1.0f } },
		{ { +0.5f * width, -0.5f * height, +0.5f * depth }, { 0.0f, 0.0f, 1.0f } },
		{ { +0.5f * width, -0.5f * height, -0.5f * depth }, { 0.0f, 0.0f, 1.0f } },
		{ { -0.5f * width, -0.5f * height, -0.5f * depth }, { 0.0f, 0.0f, 1.0f } },

		//Top Face
		{ { -0.5f * width, +0.5f * height, -0.5f * depth }, { 1.0f, 1.0f, 0.0f } },
		{ { +0.5f * width, +0.5f * height, -0.5f * depth }, { 1.0f, 1.0f, 0.0f } },
		{ { +0.5f * width, +0.5f * height, +0.5f * depth }, { 1.0f, 1.0f, 0.0f } },
		{ { -0.5f * width, +0.5f * height, +0.5f * depth }, { 1.0f, 1.0f, 0.0f } }
	};
}

std::vector<VertexPositionNormal> mini::Mesh::ShadedBoxVerts(float width, float height, float depth)
{
	return {
		//Front face
		{ { -0.5f * width, -0.5f * height, -0.5f * depth }, { 0.0f, 0.0f, 1.0f } },
		{ { +0.5f * width, -0.5f * height, -0.5f * depth }, { 0.0f, 0.0f, 1.0f } },
		{ { +0.5f * width, +0.5f * height, -0.5f * depth }, { 0.0f, 0.0f, 1.0f } },
		{ { -0.5f * width, +0.5f * height, -0.5f * depth }, { 0.0f, 0.0f, 1.0f } },

		//Back face
		{ { +0.5f * width, -0.5f * height, +0.5f * depth }, { 0.0f, 0.0f,  -1.0f } },
		{ { -0.5f * width, -0.5f * height, +0.5f * depth }, { 0.0f, 0.0f,  -1.0f } },
		{ { -0.5f * width, +0.5f * height, +0.5f * depth }, { 0.0f, 0.0f,  -1.0f } },
		{ { +0.5f * width, +0.5f * height, +0.5f * depth }, { 0.0f, 0.0f,  -1.0f } },

		//Left face
		{ { -0.5f * width, -0.5f * height, +0.5f * depth }, { 1.0f, 0.0f, 0.0f } },
		{ { -0.5f * width, -0.5f * height, -0.5f * depth }, { 1.0f, 0.0f, 0.0f } },
		{ { -0.5f * width, +0.5f * height, -0.5f * depth }, { 1.0f, 0.0f, 0.0f } },
		{ { -0.5f * width, +0.5f * height, +0.5f * depth }, { 1.0f, 0.0f, 0.0f } },

		//Right face
		{ { +0.5f * width, -0.5f * height, -0.5f * depth }, {  -1.0f, 0.0f, 0.0f } },
		{ { +0.5f * width, -0.5f * height, +0.5f * depth }, {  -1.0f, 0.0f, 0.0f } },
		{ { +0.5f * width, +0.5f * height, +0.5f * depth }, {  -1.0f, 0.0f, 0.0f } },
		{ { +0.5f * width, +0.5f * height, -0.5f * depth }, {  -1.0f, 0.0f, 0.0f } },

		//Bottom face
		{ { -0.5f * width, -0.5f * height, +0.5f * depth }, { 0.0f, 1.0f, 0.0f } },
		{ { +0.5f * width, -0.5f * height, +0.5f * depth }, { 0.0f, 1.0f, 0.0f } },
		{ { +0.5f * width, -0.5f * height, -0.5f * depth }, { 0.0f, 1.0f, 0.0f } },
		{ { -0.5f * width, -0.5f * height, -0.5f * depth }, { 0.0f, 1.0f, 0.0f } },

		//Top face
		{ { -0.5f * width, +0.5f * height, -0.5f * depth }, { 0.0f,  -1.0f, 0.0f } },
		{ { +0.5f * width, +0.5f * height, -0.5f * depth }, { 0.0f,  -1.0f, 0.0f } },
		{ { +0.5f * width, +0.5f * height, +0.5f * depth }, { 0.0f,  -1.0f, 0.0f } },
		{ { -0.5f * width, +0.5f * height, +0.5f * depth }, { 0.0f,  -1.0f, 0.0f } },
	};
}

std::vector<unsigned short> mini::Mesh::BoxIdxs()
{
	//return {
	//	 0, 2, 1,  0, 3, 2,
	//	 4, 6, 5,  4, 7, 6,
	//	 8,10, 9,  8,11,10,
	//	12,14,13, 12,15,14,
	//	16,18,17, 16,19,18,
	//	20,22,21, 20,23,22
	//};
	return {
	 0, 1, 2,  0, 2, 3,
	 4, 5, 6,  4, 6, 7,
	 8, 9,10,  8,10,11,
	12,13,14, 12,14,15,
	16,17,18, 16,18,19,
	20,21,22, 20,22,23
	};
}

std::vector<VertexPositionNormal> mini::Mesh::PentagonVerts(float radius)
{
	std::vector<VertexPositionNormal> vertices;
	vertices.reserve(5);
	float a = 0, da = XM_2PI / 5.0f;
	for (int i = 0; i < 5; ++i, a -= da)
	{
		float sina, cosa;
		XMScalarSinCos(&sina, &cosa, a);
		vertices.push_back({ { cosa * radius, sina * radius, 0.0f }, { 0.0f, 0.0f, -1.0f } });
	}
	return vertices;
}

std::vector<unsigned short> mini::Mesh::PentagonIdxs()
{
	return { 0, 1, 2, 0, 2, 3, 0, 3, 4 };
}

std::vector<VertexPositionNormal> mini::Mesh::DoubleRectVerts(float width, float height)
{
	return {
		{ { -0.5f * width, -0.5f * height, 0.0f }, { 0.0f, 0.0f,  1.0f } },
		{ { +0.5f * width, -0.5f * height, 0.0f }, { 0.0f, 0.0f,  1.0f } },
		{ { +0.5f * width, +0.5f * height, 0.0f }, { 0.0f, 0.0f,  1.0f } },
		{ { -0.5f * width, +0.5f * height, 0.0f }, { 0.0f, 0.0f,  1.0f } },

		{ { -0.5f * width, -0.5f * height, 0.0f }, { 0.0f, 0.0f, -1.0f } },
		{ { -0.5f * width, +0.5f * height, 0.0f }, { 0.0f, 0.0f, -1.0f } },
		{ { +0.5f * width, +0.5f * height, 0.0f }, { 0.0f, 0.0f, -1.0f } },
		{ { +0.5f * width, -0.5f * height, 0.0f }, { 0.0f, 0.0f, -1.0f } }
	};
}

std::vector<unsigned short> mini::Mesh::DoubleRectIdxs()
{
	return { 0, 1, 2, 0, 2, 3, 4, 5, 6, 4, 6, 7 };
}

std::vector<VertexPositionNormal> mini::Mesh::RectangleVerts(float width, float height)
{
	return {
			{ {-0.5f * width, -0.5f * height, 0.0f}, {0.0f, 0.0f, -1.0f} },
			{ {-0.5f * width, +0.5f * height, 0.0f}, {0.0f, 0.0f, -1.0f} },
			{ {+0.5f * width, +0.5f * height, 0.0f}, {0.0f, 0.0f, -1.0f} },
			{ {+0.5f * width, -0.5f * height, 0.0f}, {0.0f, 0.0f, -1.0f} }
	};
}

std::vector<unsigned short> mini::Mesh::RectangleIdx()
{
	return { 0, 1, 2, 0, 2, 3 };
}

std::vector<DirectX::XMFLOAT3> mini::Mesh::BillboardVerts(float width, float height)
{
	return {
		{ -0.5f * width, -0.5f * height, 0.0f },
		{ -0.5f * width, +0.5f * height, 0.0f },
		{ +0.5f * width, +0.5f * height, 0.0f },
		{ +0.5f * width, -0.5f * height, 0.0f }
	};
}

std::vector<VertexPositionNormal> mini::Mesh::SphereVerts(unsigned int stacks, unsigned int slices, float radius)
{
	assert(stacks > 2 && slices > 1);
	auto n = (stacks - 1U) * slices + 2U;
	vector<VertexPositionNormal> vertices(n);
	vertices[0].position = XMFLOAT3(0.0f, radius, 0.0f);
	vertices[0].normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
	auto dp = XM_PI / stacks;
	auto phi = dp;
	auto k = 1U;
	for (auto i = 0U; i < stacks - 1U; ++i, phi += dp)
	{
		float cosp, sinp;
		XMScalarSinCos(&sinp, &cosp, phi);
		auto thau = 0.0f;
		auto dt = XM_2PI / slices;
		auto stackR = radius * sinp;
		auto stackY = radius * cosp;
		for (auto j = 0U; j < slices; ++j, thau += dt)
		{
			float cost, sint;
			XMScalarSinCos(&sint, &cost, thau);
			vertices[k].position = XMFLOAT3(stackR * cost, stackY, stackR * sint);
			vertices[k++].normal = XMFLOAT3(cost * sinp, cosp, sint * sinp);
		}
	}
	vertices[k].position = XMFLOAT3(0.0f, -radius, 0.0f);
	vertices[k].normal = XMFLOAT3(0.0f, -1.0f, 0.0f);
	return vertices;
}

std::vector<unsigned short> mini::Mesh::SphereIdx(unsigned int stacks, unsigned int slices)
{
	assert(stacks > 2 && slices > 1);
	auto in = (stacks - 1U) * slices * 6U;
	vector<unsigned short> indices(in);
	auto n = (stacks - 1U) * slices + 2U;
	auto k = 0U;
	for (auto j = 0U; j < slices - 1U; ++j)
	{
		indices[k++] = 0U;
		indices[k++] = j + 2;
		indices[k++] = j + 1;
	}
	indices[k++] = 0U;
	indices[k++] = 1U;
	indices[k++] = slices;
	auto i = 0U;
	for (; i < stacks - 2U; ++i)
	{
		auto j = 0U;
		for (; j < slices - 1U; ++j)
		{
			indices[k++] = i * slices + j + 1;
			indices[k++] = i * slices + j + 2;
			indices[k++] = (i + 1) * slices + j + 2;
			indices[k++] = i * slices + j + 1;
			indices[k++] = (i + 1) * slices + j + 2;
			indices[k++] = (i + 1) * slices + j + 1;
		}
		indices[k++] = i * slices + j + 1;
		indices[k++] = i * slices + 1;
		indices[k++] = (i + 1) * slices + 1;
		indices[k++] = i * slices + j + 1;
		indices[k++] = (i + 1) * slices + 1;
		indices[k++] = (i + 1) * slices + j + 1;
	}
	for (auto j = 0U; j < slices - 1U; ++j)
	{
		indices[k++] = i * slices + j + 1;
		indices[k++] = i * slices + j + 2;
		indices[k++] = n - 1;
	}
	indices[k++] = (i + 1) * slices;
	indices[k++] = i * slices + 1;
	indices[k] = n - 1;
	return indices;
}

std::vector<VertexPositionNormal> mini::Mesh::CylinderVerts(unsigned int stacks, unsigned int slices, float height, float radius)
{
	assert(stacks > 0 && slices > 1);
	auto sideVerts = (stacks + 1) * slices;
	auto totalVerts = sideVerts + 2 * slices + 2;
	std::vector<VertexPositionNormal> vertices(totalVerts);
	auto y = height / 2;
	auto dy = height / stacks;
	auto dp = XM_2PI / slices;
	auto k = 0U;

	for (auto i = 0U; i <= stacks; ++i, y -= dy)
	{
		auto phi = 0.0f;
		for (auto j = 0U; j < slices; ++j, phi += dp)
		{
			float sinp, cosp;
			XMScalarSinCos(&sinp, &cosp, phi);
			vertices[k].position = XMFLOAT3(radius * cosp, y, radius * sinp);
			vertices[k++].normal = XMFLOAT3(cosp, 0, sinp);
		}
	}

	auto phi = 0.0f;
	for (auto j = 0U; j < slices; ++j, phi += dp)
	{
		float sinp, cosp;
		XMScalarSinCos(&sinp, &cosp, phi);
		vertices[k].position = XMFLOAT3(radius * cosp, height / 2, radius * sinp);
		vertices[k++].normal = XMFLOAT3(0, 1, 0);
	}

	phi = 0.0f;
	for (auto j = 0U; j < slices; ++j, phi += dp)
	{
		float sinp, cosp;
		XMScalarSinCos(&sinp, &cosp, phi);
		vertices[k].position = XMFLOAT3(radius * cosp, -height / 2, radius * sinp);
		vertices[k++].normal = XMFLOAT3(0, -1, 0);
	}

	vertices[k].position = XMFLOAT3(0, height / 2, 0);
	vertices[k++].normal = XMFLOAT3(0, 1, 0);
	vertices[k].position = XMFLOAT3(0, -height / 2, 0);
	vertices[k++].normal = XMFLOAT3(0, -1, 0);

	return vertices;
}

std::vector<unsigned short> mini::Mesh::CylinderIdx(unsigned int stacks, unsigned int slices)
{
	assert(stacks > 0 && slices > 1);
	auto in = 6 * stacks * slices + 6 * slices;
	std::vector<unsigned short> indices;
	indices.reserve(in);
	auto k = 0U;

	for (auto i = 0U; i < stacks; ++i)
	{
		for (auto j = 0U; j < slices; ++j)
		{
			auto next = (j + 1) % slices;
			auto a = i * slices + j;
			auto b = i * slices + next;
			auto c = (i + 1) * slices + j;
			auto d = (i + 1) * slices + next;

			indices.push_back(a);
			indices.push_back(b);
			indices.push_back(d);
			indices.push_back(a);
			indices.push_back(d);
			indices.push_back(c);
		}
	}

	auto sideVerts = (stacks + 1) * slices;
	auto top = sideVerts;
	auto bot = top + slices;
	auto tc = bot + slices;
	auto bc = tc + 1;

	for (auto j = 0U; j < slices; ++j)
	{
		auto next = (j + 1) % slices;
		indices.push_back(tc);
		indices.push_back(top + next);
		indices.push_back(top + j);
	}

	for (auto j = 0U; j < slices; ++j)
	{
		auto next = (j + 1) % slices;
		indices.push_back(bc);
		indices.push_back(bot + j);
		indices.push_back(bot + next);
	}

	return indices;
}

std::vector<VertexPositionNormal> mini::Mesh::DiskVerts(unsigned int slices, float radius)
{
	assert(slices > 1);
	auto n = slices + 1;
	vector<VertexPositionNormal> vertices(n);
	vertices[0].position = XMFLOAT3(0.0f, 0.0f, 0.0f);
	vertices[0].normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
	auto phi = 0.0f;
	auto dp = XM_2PI / slices;
	auto k = 1;
	for (auto i = 1U; i <= slices; ++i, phi += dp)
	{
		float cosp, sinp;
		XMScalarSinCos(&sinp, &cosp, phi);
		vertices[k].position = XMFLOAT3(radius * cosp, 0.0f, radius * sinp);
		vertices[k++].normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
	}
	return vertices;
}

std::vector<unsigned short> mini::Mesh::DiskIdx(unsigned int slices)
{
	assert(slices > 1);
	auto in = slices * 3;
	vector<unsigned short> indices(in);
	auto k = 0U;
	for (auto i = 0U; i < slices - 1; ++i)
	{
		indices[k++] = 0;
		indices[k++] = i + 2;
		indices[k++] = i + 1;
	}
	indices[k++] = 0;
	indices[k++] = 1;
	indices[k] = slices;
	return indices;
}

Mesh mini::Mesh::LoadMesh(const DxDevice& device, const std::wstring& meshPath)
{
	//File format for VN vertices and IN indices (IN divisible by 3, i.e. IN/3 triangles):
	//VN IN
	//pos.x pos.y pos.z norm.x norm.y norm.z tex.x tex.y [VN times, i.e. for each vertex]
	//t.i1 t.i2 t.i3 [IN/3 times, i.e. for each triangle]

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
	vector<VertexPositionNormal> vertices(l);
	for (int i = 0; i < l; ++i) {
		int posIndex;
		XMFLOAT3 normal;
		input >> posIndex >> normal.x >> normal.y >> normal.z;
		vertices[i].position = positions[posIndex];
		vertices[i].normal = normal;
	}

	input >> m;
	vector<unsigned short> indices(m * 3);
	for (int i = 0; i < m * 3; ++i)
		input >> indices[i];

	input >> n;
	// KRAWEDZIE
	//getline(input, skipLine); 
	//for (int i = 0; i < n; ++i)
	//	getline(input, skipLine);
	input.close();
	return SimpleTriMesh(device, vertices, indices);
}
