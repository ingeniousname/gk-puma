#pragma once
#include "dxApplication.h"
#include "mesh.h"
#include "SMMesh.h"
#include "environmentMapper.h"
#include "particleSystem.h"

namespace mini::gk2
{
	class Puma : public DxApplication
	{
	public:
		using Base = DxApplication;

		explicit Puma(HINSTANCE appInstance);

	protected:
		void Update(const Clock& dt) override;
		void Render() override;

	private:
#pragma region CONSTANTS
		static const DirectX::XMFLOAT4 LIGHT_POS;
#pragma endregion
		dx_ptr<ID3D11Buffer> m_cbWorldMtx, //vertex shader constant buffer slot 0
			m_cbProjMtx;	//vertex shader constant buffer slot 2 & geometry shader constant buffer slot 0
		dx_ptr<ID3D11Buffer> m_cbViewMtx; //vertex shader constant buffer slot 1
		dx_ptr<ID3D11Buffer> m_cbSurfaceColor;	//pixel shader constant buffer slot 0
		dx_ptr<ID3D11Buffer> m_cbLightPos; //pixel shader constant buffer slot 1
		dx_ptr<ID3D11Buffer> m_cbShadowControl; //pixel shader constant buffer slot 2

		dx_ptr<ID3D11Buffer> m_vbParticleSystem;
		dx_ptr<ID3D11ShaderResourceView> m_particleTexture;
		
		SMMesh m_manipulator[6];
		SMMesh m_cylinder;
		Mesh m_box;
		SMMesh m_mirror;

		DirectX::XMFLOAT4X4 m_projMtx;
		DirectX::XMFLOAT4X4 m_mirrorMtx;

		DirectX::XMFLOAT4X4 m_manipulatorMtx[6];
		DirectX::XMFLOAT4X4 m_cylinderMtx;
		float m_manipulatorAngle[5];
		bool m_animation;

		ParticleSystem m_particleSystem;

		dx_ptr<ID3D11RasterizerState> m_rsCullFront;
		dx_ptr<ID3D11RasterizerState> m_rsCullBack;
		dx_ptr<ID3D11RasterizerState> m_rsCCW;
		dx_ptr<ID3D11BlendState> m_bsAlpha;
		dx_ptr<ID3D11BlendState> m_bsNoColor;
		dx_ptr<ID3D11BlendState> m_bsAdd;
		dx_ptr<ID3D11DepthStencilState> m_dssNoWrite;
		dx_ptr<ID3D11DepthStencilState> m_dssDepthWriteOnly;
		dx_ptr<ID3D11DepthStencilState> m_dssStencilWrite;
		dx_ptr<ID3D11DepthStencilState> m_dssStencilTest;
		dx_ptr<ID3D11DepthStencilState> m_dssStencilShadowVolume;
		dx_ptr<ID3D11SamplerState> m_samplerWrap;

		dx_ptr<ID3D11InputLayout> m_inputlayout, m_particleLayout;

		dx_ptr<ID3D11VertexShader> m_phongVS, m_textureVS, m_multiTexVS, m_particleVS;
		dx_ptr<ID3D11GeometryShader> m_particleGS;
		dx_ptr<ID3D11PixelShader> m_phongPS, m_texturePS, m_colorTexPS, m_multiTexPS, m_particlePS;

		void UpdateCameraCB(DirectX::XMMATRIX viewMtx);
		void UpdateCameraCB() { UpdateCameraCB(m_camera.getViewMatrix()); }
		void HandleManipulatorInput(double dt);
		void ManipulatorAnimation(double dt);
		void InverseKinematics(DirectX::XMFLOAT3 pos, DirectX::XMFLOAT3 normal);
		void UpdateManipulatorMtx();
		void UpdateParticleSystem(double dt);

		void GenerateShadowVolumes();
		void DrawShadowVolumes();

		void DrawMesh(const Mesh& m, DirectX::XMFLOAT4X4 worldMtx);
		void DrawMirroredWorld();
		void DrawMirror();
		void DrawManipulators();
		void DrawCylinder();
		void DrawBox();
		void DrawParticleSystem();
		void DrawMesh(const SMMesh& m, DirectX::XMFLOAT4X4 worldMtx);
		void DrawShadowVolume(const SMMesh& m, DirectX::XMFLOAT4X4 worldMtx);

		void SetWorldMtx(DirectX::XMFLOAT4X4 mtx);
		void SetSurfaceColor(DirectX::XMFLOAT4 color);
		void SetShaders(const dx_ptr<ID3D11VertexShader>& vs, const dx_ptr<ID3D11PixelShader>& ps);
		void SetTextures(std::initializer_list<ID3D11ShaderResourceView*> resList, const dx_ptr<ID3D11SamplerState>& sampler);
		void SetTextures(std::initializer_list<ID3D11ShaderResourceView*> resList) { SetTextures(std::move(resList), m_samplerWrap); }


		void DrawScene();
	};
}