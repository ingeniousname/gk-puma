#include "Puma.h"
#include <array>
#include "mesh.h"

using namespace mini;
using namespace gk2;
using namespace DirectX;
using namespace std;
const XMFLOAT4 Puma::LIGHT_POS = {2.0f, 3.0f, 3.0f, 1.0f};

Puma::Puma(HINSTANCE appInstance)
	: DxApplication(appInstance, 1280, 720, L"Pokój"), 
	//Constant Buffers
	m_cbWorldMtx(m_device.CreateConstantBuffer<XMFLOAT4X4>()),
	m_cbProjMtx(m_device.CreateConstantBuffer<XMFLOAT4X4>()),
	m_cbViewMtx(m_device.CreateConstantBuffer<XMFLOAT4X4, 2>()),
	m_cbSurfaceColor(m_device.CreateConstantBuffer<XMFLOAT4>()),
	m_cbLightPos(m_device.CreateConstantBuffer<XMFLOAT4, 2>())
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
		m_manipulator[i] = Mesh::LoadMesh(m_device, L"resources/meshes/mesh" + std::to_wstring(i + 1) + L".txt");
	}
	
	m_cylinder = Mesh::Cylinder(m_device, 100, 100, 3.f, 0.5f);
	m_box = Mesh::ShadedBox(m_device, 5.f);
	m_mirror = Mesh::DoubleRect(m_device, 1.5f, 1.f);

	//World matrix of all objects
	auto temp = XMMatrixTranslation(0.0f, 0.0f, 2.0f);
	auto a = 0.f;

	//Constant buffers content
	UpdateBuffer(m_cbLightPos, LIGHT_POS);


	//Render states
	RasterizerDescription rsDesc;
	rsDesc.CullMode = D3D11_CULL_FRONT;
	m_rsCullFront = m_device.CreateRasterizerState(rsDesc);
	rsDesc.CullMode = D3D11_CULL_BACK;
	m_rsCullBack = m_device.CreateRasterizerState(rsDesc);

	m_bsAlpha = m_device.CreateBlendState(BlendDescription::AlphaBlendDescription());
	DepthStencilDescription dssDesc;
	dssDesc.DepthEnable = true;
	dssDesc.StencilEnable = true;
	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dssDesc.StencilReadMask = 0xFF;
	dssDesc.StencilWriteMask = 0xFF;
	dssDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	dssDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	m_dssNoWrite = m_device.CreateDepthStencilState(dssDesc);

	dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dssDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
	dssDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dssDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	dssDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	m_dssStencilWrite = m_device.CreateDepthStencilState(dssDesc);

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
	ID3D11Buffer* psb[] = { m_cbSurfaceColor.get(), m_cbLightPos.get() };
	m_device.context()->PSSetConstantBuffers(0, 2, psb); //Pixel Shaders - 0: surfaceColor, 1: lightPos
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
	XMMATRIX mtx[6];
	if (!m_keyboard.GetState(keyboard))
		return;
	if (keyboard.isKeyDown(DIK_F))
	{
		mtx[0] = XMLoadFloat4x4(&m_manipulatorMtx[0]) * XMMatrixRotationY(XM_PIDIV2 * dt);
	}
	if (keyboard.isKeyDown(DIK_R))
	{
		mtx[0] = XMLoadFloat4x4(&m_manipulatorMtx[0]) * XMMatrixRotationY(XM_PIDIV2 * -dt);
	}
	if (keyboard.isKeyDown(DIK_G))
	{
		mtx[1] = XMLoadFloat4x4(&m_manipulatorMtx[1]) * XMMatrixRotationX(XM_PIDIV2 * dt);
	}
	if (keyboard.isKeyDown(DIK_T))
	{
		mtx[1] = XMLoadFloat4x4(&m_manipulatorMtx[1]) * XMMatrixRotationX(XM_PIDIV2 * -dt);
	}

}

void Puma::Update(const Clock& c)
{
	double dt = c.getFrameTime();
	HandleCameraInput(dt);
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

void Puma::DrawMesh(const Mesh& m, DirectX::XMFLOAT4X4 worldMtx)
{
	SetWorldMtx(worldMtx);
	m.Render(m_device.context());
}

void Puma::DrawMirroredWorld()
{
	// write mirror to stencil buffer
	XMFLOAT4X4 mtx;
	XMMATRIX model = XMMatrixRotationY(XM_PIDIV2) * XMMatrixRotationZ(XM_PIDIV4) * XMMatrixTranslation(-1.5f, 0.25f, -0.5f);
	XMStoreFloat4x4(&mtx, model);
	m_device.context()->OMSetDepthStencilState(m_dssNoWrite.get(), 1);
	DrawMesh(m_mirror, mtx);
	m_device.context()->OMSetDepthStencilState(m_dssStencilWrite.get(), 1);
	XMMATRIX mirror = XMMatrixScaling(1.f, 1.f, -1.f);
	XMMATRIX inv = XMMatrixInverse(nullptr, model);
	XMMATRIX mirrorRef = inv * mirror * model;

	m_device.context()->RSSetState(m_rsCullBack.get());
	mirrorRef = mirrorRef * m_camera.getViewMatrix();
	UpdateCameraCB(mirrorRef);

	DrawBox();
	DrawManipulators();
	DrawCylinder();
	m_device.context()->RSSetState(nullptr);
	m_device.context()->OMSetDepthStencilState(nullptr, 0);
	UpdateCameraCB();
}

void mini::gk2::Puma::DrawMirror()
{
	XMFLOAT4X4 mtx;
	SetSurfaceColor({ 1.f, 1.f, 1.f, 1.f });
	XMStoreFloat4x4(&mtx, XMMatrixRotationY(XM_PIDIV2) * XMMatrixRotationZ(XM_PIDIV4) * XMMatrixTranslation(-1.5f, 0.25f, -0.5f));
	DrawMesh(m_mirror, mtx);
}

void mini::gk2::Puma::DrawManipulators()
{
	XMFLOAT4X4 mtx;
	XMStoreFloat4x4(&mtx, XMMatrixIdentity());
	SetSurfaceColor({ 0.75f, 0.75f, 0.75f, 1.f });
	for (int i = 0; i < 6; i++)
		DrawMesh(m_manipulator[i], mtx);
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

void Puma::DrawScene()
{
	SetShaders(m_phongVS, m_phongPS);
	DrawMirroredWorld();

	UpdateCameraCB();
	m_device.context()->OMSetBlendState(m_bsAlpha.get(), nullptr, 0xFFFFFFFF);
	DrawMirror();
	m_device.context()->OMSetBlendState(nullptr, nullptr, 0xFFFFFFFF);
	
	DrawManipulators();
	DrawCylinder();
	DrawBox();
}

void Puma::Render()
{
	Base::Render();

	ResetRenderTarget();
	UpdateBuffer(m_cbProjMtx, m_projMtx);

	DrawScene();
}