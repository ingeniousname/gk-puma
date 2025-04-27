#include "Puma.h"
#include <array>
#include "mesh.h"

using namespace mini;
using namespace gk2;
using namespace DirectX;
using namespace std;
const XMFLOAT4 Puma::LIGHT_POS = { 2.0f, 3.0f, 3.0f, 1.0f };

Puma::Puma(HINSTANCE appInstance)
	: DxApplication(appInstance, 1280, 720, L"Pokój"),
	//Constant Buffers
	m_cbWorldMtx(m_device.CreateConstantBuffer<XMFLOAT4X4>()),
	m_cbProjMtx(m_device.CreateConstantBuffer<XMFLOAT4X4>()),
	m_cbViewMtx(m_device.CreateConstantBuffer<XMFLOAT4X4, 2>()),
	m_cbSurfaceColor(m_device.CreateConstantBuffer<XMFLOAT4>()),
	m_cbLightPos(m_device.CreateConstantBuffer<XMFLOAT4, 2>()),
	m_cbShadowControl(m_device.CreateConstantBuffer<XMINT4>()),
	m_vbParticleSystem(m_device.CreateVertexBuffer<ParticleVertex>(ParticleSystem::MAX_PARTICLES)),
	m_particleTexture(m_device.CreateShaderResourceView(L"resources/textures/particle.png"))
{
	//Projection matrix
	auto s = m_window.getClientSize();
	auto ar = static_cast<float>(s.cx) / s.cy;
	DirectX::XMStoreFloat4x4(&m_projMtx, XMMatrixPerspectiveFovLH(XM_PIDIV4, ar, 0.01f, 100.0f));
	UpdateBuffer(m_cbProjMtx, m_projMtx);
	UpdateCameraCB();

	//Meshes
	vector<VertexPositionNormal> vertices;
	vector<unsigned short> indices;
	for (int i = 0; i < 6; i++)
	{
		m_manipulator[i] = SMMesh::LoadMesh(m_device, L"resources/meshes/mesh" + std::to_wstring(i + 1) + L".txt");
		XMStoreFloat4x4(&m_manipulatorMtx[i], XMMatrixIdentity());
	}

	m_cylinder = Mesh::Cylinder(m_device, 100, 100, 3.f, 0.5f);
	m_box = Mesh::ShadedBox(m_device, 5.f);
	m_mirror = Mesh::DoubleRect(m_device, 1.5f, 1.f);
	XMStoreFloat4x4(&m_mirrorMtx, XMMatrixRotationY(XM_PIDIV2) * XMMatrixRotationZ(XM_PIDIV4) * XMMatrixTranslation(-1.5f, 0.25f, -0.5f));

	//Constant buffers content
	UpdateBuffer(m_cbLightPos, LIGHT_POS);

	//Render states
	RasterizerDescription rsDesc;
	rsDesc.FrontCounterClockwise = true;
	m_rsCCW = m_device.CreateRasterizerState(rsDesc);
	rsDesc.FrontCounterClockwise = false;
	rsDesc.CullMode = D3D11_CULL_FRONT;
	m_rsCullFront = m_device.CreateRasterizerState(rsDesc);
	rsDesc.CullMode = D3D11_CULL_BACK;
	m_rsCullBack = m_device.CreateRasterizerState(rsDesc);

	m_bsAdd = m_device.CreateBlendState(BlendDescription::AdditiveBlendDescription());
	m_bsAlpha = m_device.CreateBlendState(BlendDescription::AlphaBlendDescription());
	m_bsNoColor = m_device.CreateBlendState(BlendDescription::NoColorBlendDescription());
	DepthStencilDescription dssDesc;
	dssDesc.DepthEnable = true;
	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	m_dssNoWrite = m_device.CreateDepthStencilState(dssDesc);

	dssDesc.StencilWriteMask = 0xff;
	dssDesc.StencilReadMask = 0xff;
	dssDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	dssDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	dssDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dssDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dssDesc.DepthEnable = true;
	dssDesc.StencilEnable = true;
	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;

	m_dssStencilWrite = m_device.CreateDepthStencilState(dssDesc);

	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dssDesc.StencilEnable = true;
	dssDesc.DepthEnable = true;
	dssDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
	dssDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dssDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dssDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	m_dssStencilTest = m_device.CreateDepthStencilState(dssDesc);



	DepthStencilDescription desc;
	desc.DepthEnable = true;
	desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	desc.DepthFunc = D3D11_COMPARISON_LESS;

	desc.StencilEnable = true;
	desc.StencilReadMask = 0xFF;
	desc.StencilWriteMask = 0xFF;

	// Front faces: increment stencil on depth fail
	desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;

	// Back faces: decrement stencil on depth fail
	desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;

	m_dssStencilShadowVolume = m_device.CreateDepthStencilState(desc);




	//Textures
	auto vsCode = m_device.LoadByteCode(L"phongVS.cso");
	auto psCode = m_device.LoadByteCode(L"phongPS.cso");
	m_phongVS = m_device.CreateVertexShader(vsCode);
	m_phongPS = m_device.CreatePixelShader(psCode);
	m_inputlayout = m_device.CreateInputLayout(VertexPositionNormal::Layout, vsCode);

	vsCode = m_device.LoadByteCode(L"texturedVS.cso");
	psCode = m_device.LoadByteCode(L"texturedPS.cso");
	m_textureVS = m_device.CreateVertexShader(vsCode);
	m_texturePS = m_device.CreatePixelShader(psCode);
	psCode = m_device.LoadByteCode(L"colorTexPS.cso");
	m_colorTexPS = m_device.CreatePixelShader(psCode);

	vsCode = m_device.LoadByteCode(L"multiTexVS.cso");
	psCode = m_device.LoadByteCode(L"multiTexPS.cso");
	m_multiTexVS = m_device.CreateVertexShader(vsCode);
	m_multiTexPS = m_device.CreatePixelShader(psCode);

	vsCode = m_device.LoadByteCode(L"particleVS.cso");
	psCode = m_device.LoadByteCode(L"particlePS.cso");
	auto gsCode = m_device.LoadByteCode(L"particleGS.cso");
	m_particleVS = m_device.CreateVertexShader(vsCode);
	m_particlePS = m_device.CreatePixelShader(psCode);
	m_particleGS = m_device.CreateGeometryShader(gsCode);
	m_particleLayout = m_device.CreateInputLayout<ParticleVertex>(vsCode);

	m_device.context()->IASetInputLayout(m_inputlayout.get());
	m_device.context()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//We have to make sure all shaders use constant buffers in the same slots!
	//Not all slots will be use by each shader
	ID3D11Buffer* vsb[] = { m_cbWorldMtx.get(),  m_cbViewMtx.get(), m_cbProjMtx.get() };
	m_device.context()->VSSetConstantBuffers(0, 3, vsb); //Vertex Shaders - 0: worldMtx, 1: viewMtx,invViewMtx, 2: projMtx, 3: tex1Mtx, 4: tex2Mtx
	m_device.context()->GSSetConstantBuffers(0, 1, vsb + 2); //Geometry Shaders - 0: projMtx
	ID3D11Buffer* psb[] = { m_cbSurfaceColor.get(), m_cbLightPos.get(), m_cbShadowControl.get() };
	m_device.context()->PSSetConstantBuffers(0, 3, psb); //Pixel Shaders - 0: surfaceColor, 1: lightPos, 2: shadowControl
}

void Puma::UpdateCameraCB(XMMATRIX viewMtx)
{
	XMVECTOR det;
	XMMATRIX invViewMtx = XMMatrixInverse(&det, viewMtx);
	XMFLOAT4X4 view[2];
	DirectX::XMStoreFloat4x4(view, viewMtx);
	DirectX::XMStoreFloat4x4(view + 1, invViewMtx);
	UpdateBuffer(m_cbViewMtx, view);
}

void Puma::HandleManipulatorInput(double dt)
{
	KeyboardState keyboard;
	float speed = 0.5f;
	if (!m_keyboard.GetState(keyboard))
		return;
	if (keyboard.isKeyDown(DIK_C)) {
		m_animation = !m_animation;
	}

	if (keyboard.isKeyDown(DIK_F))
	{
		m_manipulatorAngle[0] += speed * dt;
		m_animation = false;
	}
	if (keyboard.isKeyDown(DIK_R))
	{
		m_manipulatorAngle[0] -= speed * dt;
		m_animation = false;
	}
	if (keyboard.isKeyDown(DIK_G))
	{
		m_manipulatorAngle[1] += speed * dt;
		m_animation = false;
	}
	if (keyboard.isKeyDown(DIK_T))
	{
		m_manipulatorAngle[1] -= speed * dt;
		m_animation = false;
	}
	if (keyboard.isKeyDown(DIK_H))
	{
		m_manipulatorAngle[2] += speed * dt;
		m_animation = false;
	}
	if (keyboard.isKeyDown(DIK_Y))
	{
		m_manipulatorAngle[2] -= speed * dt;
		m_animation = false;
	}
	if (keyboard.isKeyDown(DIK_J))
	{
		m_manipulatorAngle[3] += speed * dt;
		m_animation = false;
	}
	if (keyboard.isKeyDown(DIK_U))
	{
		m_manipulatorAngle[3] -= speed * dt;
		m_animation = false;
	}
	if (keyboard.isKeyDown(DIK_K))
	{
		m_manipulatorAngle[4] += speed * dt;
		m_animation = false;
	}
	if (keyboard.isKeyDown(DIK_I))
	{
		m_manipulatorAngle[4] -= speed * dt;
		m_animation = false;
	}

	UpdateManipulatorMtx();
}

void Puma::ManipulatorAnimation(double dt)
{
	const float r = 0.4f;
	static float t = 0.f;
	t += static_cast<float>(dt);
	XMFLOAT3 pos = { r * cosf(t), r * sinf(t), 0.f };
	XMFLOAT3 normal = { 0.f, 0.f, -1.f };

	XMVECTOR p = XMLoadFloat3(&pos);
	XMVECTOR n = XMLoadFloat3(&normal);

	XMMATRIX mirror = XMLoadFloat4x4(&m_mirrorMtx);
	XMMATRIX invmirror = XMMatrixInverse(nullptr, mirror);

	XMVECTOR p3 = XMVector3TransformCoord(p, mirror);
	XMVECTOR n3 = XMVector3TransformNormal(n, invmirror);

	XMStoreFloat3(&pos, p3);
	XMStoreFloat3(&normal, n3);

	InverseKinematics(pos, normal);
	UpdateManipulatorMtx();
	m_particleSystem.SetEmitterPosition(pos);
}

void Puma::InverseKinematics(XMFLOAT3 pos, XMFLOAT3 normal)
{
	float l1 = .91f;
	float l2 = .81f;
	float l3 = .33f;
	float dy = .27f;
	float dz = .26f;

	XMVECTOR nor = XMLoadFloat3(&normal);
	XMVECTOR posV = XMLoadFloat3(&pos);
	nor = XMVector3Normalize(nor);
	XMVECTOR p1 = XMVectorAdd(posV, XMVectorScale(nor, l3));
	XMFLOAT3 p1f;
	XMStoreFloat3(&p1f, p1);

	float e = sqrtf(p1f.x * p1f.x + p1f.z * p1f.z - dz * dz);
	m_manipulatorAngle[0] = atan2f(p1f.z, -p1f.x) + atan2f(dz, e);

	XMFLOAT3 pos2 = XMFLOAT3(e, p1f.y - dy, 0.0f);

	float dot = pos2.x * pos2.x + pos2.y * pos2.y + pos2.z * pos2.z - l1 * l1 - l2 * l2;
	float denom = 2.0f * l1 * l2;
	m_manipulatorAngle[2] = -acosf(min(1.0f, dot / denom));

	float k = l1 + l2 * cosf(m_manipulatorAngle[2]);
	float l = l2 * sinf(m_manipulatorAngle[2]);
	m_manipulatorAngle[1] = -atan2f(pos2.y, sqrtf(pos2.x * pos2.x + pos2.z * pos2.z)) - atan2f(l, k);

	XMVECTOR normal1 = XMVector3TransformNormal(nor, XMMatrixRotationY(-m_manipulatorAngle[0]));
	normal1 = XMVector3TransformNormal(normal1, XMMatrixRotationZ(-(m_manipulatorAngle[1] + m_manipulatorAngle[2])));

	XMFLOAT3 n1;
	XMStoreFloat3(&n1, normal1);
	m_manipulatorAngle[4] = acosf(n1.x);
	m_manipulatorAngle[3] = atan2f(n1.z, n1.y);
}

void mini::gk2::Puma::UpdateManipulatorMtx()
{
	XMMATRIX mtx[6];
	mtx[0] = XMMatrixIdentity();
	mtx[1] = XMMatrixRotationY(m_manipulatorAngle[0]);
	mtx[2] = XMMatrixTranslation(0, -0.27f, 0) * XMMatrixRotationZ(m_manipulatorAngle[1]) * XMMatrixTranslation(0, 0.27f, 0) * mtx[1];
	mtx[3] = XMMatrixTranslation(0.91f, -0.27f, 0) * XMMatrixRotationZ(m_manipulatorAngle[2]) * XMMatrixTranslation(-0.91f, 0.27f, 0) * mtx[2];
	mtx[4] = XMMatrixTranslation(0, -0.27f, 0.26f) * XMMatrixRotationX(m_manipulatorAngle[3]) * XMMatrixTranslation(0, 0.27f, -0.26f) * mtx[3];
	mtx[5] = XMMatrixTranslation(1.72f, -0.27, 0) * XMMatrixRotationZ(m_manipulatorAngle[4]) * XMMatrixTranslation(-1.72f, 0.27f, 0) * mtx[4];

	XMStoreFloat4x4(&m_manipulatorMtx[0], mtx[0]);
	XMStoreFloat4x4(&m_manipulatorMtx[1], mtx[1]);
	XMStoreFloat4x4(&m_manipulatorMtx[2], mtx[2]);
	XMStoreFloat4x4(&m_manipulatorMtx[3], mtx[3]);
	XMStoreFloat4x4(&m_manipulatorMtx[4], mtx[4]);
	XMStoreFloat4x4(&m_manipulatorMtx[5], mtx[5]);
}

void mini::gk2::Puma::UpdateParticleSystem(double dt)
{
	auto verts = m_particleSystem.Update(static_cast<float>(dt), m_camera.getCameraPosition());
	UpdateBuffer(m_vbParticleSystem, verts);
}


void Puma::Update(const Clock& c)
{
	double dt = c.getFrameTime();
	HandleCameraInput(dt);
	HandleManipulatorInput(dt);
	if (m_animation)
	{
		ManipulatorAnimation(dt);
		UpdateParticleSystem(dt);
	}

	UpdateCameraCB();
}

void Puma::SetWorldMtx(DirectX::XMFLOAT4X4 mtx)
{
	UpdateBuffer(m_cbWorldMtx, mtx);
}

void Puma::SetSurfaceColor(DirectX::XMFLOAT4 color)
{
	UpdateBuffer(m_cbSurfaceColor, color);
}

void mini::gk2::Puma::SetShaders(const dx_ptr<ID3D11VertexShader>& vs, const dx_ptr<ID3D11PixelShader>& ps)
{
	m_device.context()->VSSetShader(vs.get(), nullptr, 0);
	m_device.context()->PSSetShader(ps.get(), nullptr, 0);
}

void mini::gk2::Puma::SetTextures(std::initializer_list<ID3D11ShaderResourceView*> resList, const dx_ptr<ID3D11SamplerState>& sampler)
{
	m_device.context()->PSSetShaderResources(0, resList.size(), resList.begin());
	auto s_ptr = sampler.get();
	m_device.context()->PSSetSamplers(0, 1, &s_ptr);
}

void Puma::DrawMesh(const Mesh& m, DirectX::XMFLOAT4X4 worldMtx)
{
	SetWorldMtx(worldMtx);
	m.Render(m_device.context());
}

void Puma::DrawMirroredWorld()
{
	// write mirror to stencil buffer
	m_device.context()->OMSetDepthStencilState(m_dssStencilWrite.get(), 1);
	DrawMirror();
	m_device.context()->OMSetDepthStencilState(m_dssStencilTest.get(), 1);
	XMMATRIX mirror = XMMatrixScaling(1.f, 1.f, -1.f);
	XMMATRIX model = XMLoadFloat4x4(&m_mirrorMtx);
	XMMATRIX inv = XMMatrixInverse(nullptr, model);
	XMMATRIX mirrorRef = inv * mirror * model;

	m_device.context()->RSSetState(m_rsCCW.get());
	mirrorRef = mirrorRef * m_camera.getViewMatrix();
	UpdateCameraCB(mirrorRef);
	DrawBox();
	DrawManipulators();
	DrawCylinder();
	//DrawParticleSystem();

	m_device.context()->RSSetState(nullptr);
	m_device.context()->OMSetDepthStencilState(nullptr, 0);
	m_device.context()->OMSetBlendState(m_bsAlpha.get(), nullptr, 0xFFFFFFFF);
	DrawMirror();
	m_device.context()->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
}

void mini::gk2::Puma::DrawMirror()
{
	SetSurfaceColor({ 0.1f, 0.1f, 0.1f, 0.5f });
	DrawMesh(m_mirror, m_mirrorMtx);
}

void mini::gk2::Puma::DrawManipulators()
{
	SetSurfaceColor({ 0.75f, 0.75f, 0.75f, 1.f });
	XMFLOAT4X4 mtx;
	XMStoreFloat4x4(&mtx, DirectX::XMMatrixIdentity());
	for (int i = 0; i < 1; i++)
	{
		DrawMesh(m_manipulator[i], m_manipulatorMtx[i]);
	}
}

void Puma::DrawMesh(const SMMesh& m, DirectX::XMFLOAT4X4 worldMtx)
{
	SetWorldMtx(worldMtx);
	m.Render(m_device.context());
}

void mini::gk2::Puma::DrawShadowVolume(const SMMesh& m, DirectX::XMFLOAT4X4 worldMtx)
{
	SetWorldMtx(worldMtx);
	m.RenderShadowVolume(m_device.context());
}

void mini::gk2::Puma::DrawCylinder()
{
	XMFLOAT4X4 mtx;
	SetSurfaceColor({ 0.f, 0.75f, 0.f, 1.f });
	XMStoreFloat4x4(&mtx, XMMatrixRotationZ(XM_PIDIV2) * XMMatrixTranslation(0.f, -1.f, -1.5f));
	DrawMesh(m_cylinder, mtx);
}

void mini::gk2::Puma::DrawBox()
{
	XMFLOAT4X4 mtx;
	XMStoreFloat4x4(&mtx, XMMatrixTranslation(0.f, 1.5f, 0.f));
	SetSurfaceColor({ 214.f / 255.f, 212.f / 255.f, 67.f / 255.f, 1.f });
	DrawMesh(m_box, mtx);
}

void Puma::DrawParticleSystem()
{
	if (m_particleSystem.particlesCount() == 0)
		return;
	//Set input layout, primitive topology, shaders, vertex buffer, and draw particles
	//m_device.context()->OMSetDepthStencilState(m_dssNoWrite.get(), 0);
	SetTextures({ m_particleTexture.get() }, m_samplerWrap);
	m_device.context()->OMSetBlendState(m_bsAdd.get(), nullptr, 0xFFFFFFFF);
	m_device.context()->IASetInputLayout(m_particleLayout.get());
	SetShaders(m_particleVS, m_particlePS);
	m_device.context()->GSSetShader(m_particleGS.get(), nullptr, 0);
	m_device.context()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	unsigned int stride = sizeof(ParticleVertex);
	unsigned int offset = 0;
	auto vb = m_vbParticleSystem.get();
	m_device.context()->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
	m_device.context()->Draw(m_particleSystem.particlesCount(), 0);

	//Reset layout, primitive topology and geometry shader
	m_device.context()->GSSetShader(nullptr, nullptr, 0);
	m_device.context()->IASetInputLayout(m_inputlayout.get());
	m_device.context()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_device.context()->OMSetDepthStencilState(nullptr, 0);
	m_device.context()->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
}

void Puma::DrawScene()
{
	//DrawParticleSystem();
	SetShaders(m_phongVS, m_phongPS);
	//DrawMirroredWorld();

	DrawManipulators();
	DrawCylinder();
	DrawBox();
}

void Puma::Render()
{
	Base::Render();

	ResetRenderTarget();
	UpdateBuffer(m_cbProjMtx, m_projMtx);

	UpdateBuffer(m_cbShadowControl, XMINT4(1, 1, 1, 1));
	DrawScene();

	m_device.context()->OMSetDepthStencilState(m_dssStencilShadowVolume.get(), 1);
	m_device.context()->OMSetBlendState(m_bsNoColor.get(), nullptr, 0xFFFFFFFF);
	m_device.context()->RSSetState(m_rsCullFront.get());

	XMFLOAT4X4 mtx;
	XMStoreFloat4x4(&mtx, DirectX::XMMatrixIdentity());
	for (int i = 0; i < 1; i++)
	{
		m_manipulator[i].GenerateShadowVolume(m_device, { LIGHT_POS.x, LIGHT_POS.y, LIGHT_POS.z }, m_manipulatorMtx[i], 10.f);
		DrawShadowVolume(m_manipulator[i], mtx);
	}


	UpdateBuffer(m_cbShadowControl, XMINT4(0, 0, 0, 0));
	m_device.context()->RSSetState(m_rsCullBack.get());
	for (int i = 0; i < 1; i++)
	{
		DrawShadowVolume(m_manipulator[i], mtx);
	}

	// Third pass: render lit areas
	m_device.context()->OMSetDepthStencilState(m_dssStencilTest.get(), 0);
	m_device.context()->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
	m_device.context()->ClearDepthStencilView(m_depthBuffer.get(),
		D3D11_CLEAR_DEPTH, 1.0f, 0);
	DrawScene(); // render light contribution where stencil == 0

	// Clean up
	m_device.context()->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
	m_device.context()->OMSetDepthStencilState(nullptr, 0);
}