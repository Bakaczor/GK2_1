#pragma once
#include "windowApplication.h"
#include "dxDevice.h"
#include <DirectXMath.h>
#include <array>

class DxApplication : public mini::WindowApplication
{
public:
	explicit DxApplication(HINSTANCE hInstance);

	std::vector<DirectX::XMFLOAT2> CreateTriangleVertices() {
		DirectX::XMFLOAT2 x1(-0.5f, -0.5f);
		DirectX::XMFLOAT2 x2(-0.5f, 0.5f);
		DirectX::XMFLOAT2 x3(0.5f, -0.5f);

		return std::vector<DirectX::XMFLOAT2>{ x1, x2, x3 };
	}

protected:
	int MainLoop() override;
	bool ProcessMessage(mini::WindowMessage& msg) override;

private:
	struct VertexPositionColor {
		DirectX::XMFLOAT3 position, color;
	};
	static std::vector<VertexPositionColor> CreateCubeVertices();
	static std::vector<unsigned short> CreateCubeIndices();
	mini::dx_ptr<ID3D11Buffer> m_indexBuffer;

	void Render();
	void Update();

	DxDevice m_device;
	mini::dx_ptr<ID3D11RenderTargetView> m_backBuffer;
	mini::dx_ptr<ID3D11DepthStencilView> m_depthBuffer;
	mini::dx_ptr<ID3D11Buffer> m_vertexBuffer;
	mini::dx_ptr<ID3D11VertexShader> m_vertexShader;
	mini::dx_ptr<ID3D11PixelShader> m_pixelShader;
	mini::dx_ptr<ID3D11InputLayout> m_layout;

	// Zadanie 3
	DirectX::XMFLOAT4X4 m_modelMtx1, m_modelMtx2;
	const std::array<float, 3> m_translation = { -10.0f, 0.0f, 0.0f };

	DirectX::XMFLOAT4X4 m_viewMtx, m_projMtx;
	mini::dx_ptr<ID3D11Buffer> m_cbMVP;

	// Zadanie 1
	float m_angle;
	LARGE_INTEGER m_previousTime;
	LARGE_INTEGER m_frequency;
	const float m_angularVelocity = DirectX::XM_PI / 4.0f;
	void ComputeAngle();

	// Zadanie 2
	float m_cameraPitch;  
	float m_cameraDistance; 
	POINT m_prevMousePos;   
	bool m_isLeftButtonDown; 
	bool m_isRightButtonDown;

	const float m_defaultPitch = DirectX::XMConvertToRadians(-30.0f); 
	const float m_defaultDistance = 10.0f;
	void UpdateViewMatrix();
};
