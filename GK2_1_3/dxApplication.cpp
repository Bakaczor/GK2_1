#include "dxApplication.h"
#include <algorithm>

#define ZADANIE_1 1
#define ZADANIE_2 1
#define ZADANIE_3 1

using namespace mini;

DxApplication::DxApplication(HINSTANCE hInstance): WindowApplication(hInstance), m_device(m_window), m_angle(0.0f), m_isLeftButtonDown(false), m_isRightButtonDown(false) {
	ID3D11Texture2D *temp;
	dx_ptr<ID3D11Texture2D> backTexture;
	m_device.swapChain()->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&temp));
	backTexture.reset(temp);

	m_backBuffer = m_device.CreateRenderTargetView(backTexture);

	SIZE wndSize = m_window.getClientSize();
	m_depthBuffer = m_device.CreateDepthStencilView(wndSize);
	auto backBuffer = m_backBuffer.get();
	m_device.context()->OMSetRenderTargets(1, &backBuffer, m_depthBuffer.get());
	Viewport viewport{ wndSize };
	m_device.context()->RSSetViewports(1, &viewport);

	const auto vsBytes = DxDevice::LoadByteCode(L"vs.cso");
	const auto psBytes = DxDevice::LoadByteCode(L"ps.cso");
	m_vertexShader = m_device.CreateVertexShader(vsBytes);
	m_pixelShader = m_device.CreatePixelShader(psBytes);

	const auto vertices = CreateCubeVertices();
	m_vertexBuffer = m_device.CreateVertexBuffer(vertices);
	const auto indices = CreateCubeIndices();
	m_indexBuffer = m_device.CreateIndexBuffer(indices);

	std::vector<D3D11_INPUT_ELEMENT_DESC> elements {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(VertexPositionColor, color), D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	m_layout = m_device.CreateInputLayout(elements, vsBytes);

	XMStoreFloat4x4(&m_modelMtx1, DirectX::XMMatrixIdentity());

	// Zadanie 3
	XMStoreFloat4x4(&m_modelMtx2, DirectX::XMMatrixIdentity());
	
	// Zadanie 2
	m_cameraPitch = m_defaultPitch; 
	m_cameraDistance = m_defaultDistance;
	UpdateViewMatrix();

	XMStoreFloat4x4(&m_projMtx, DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(45), static_cast<float>(wndSize.cx) / wndSize.cy, 0.1f, 100.0f));
	m_cbMVP = m_device.CreateConstantBuffer<DirectX::XMFLOAT4X4>();

	// Zadanie 1
	QueryPerformanceFrequency(&m_frequency);
	QueryPerformanceCounter(&m_previousTime);
}

int DxApplication::MainLoop() {
	MSG msg;
	//PeekMessage doesn't change MSG if there are no messages to be recieved.
	//However unlikely the case may be, that the first call to PeekMessage
	//doesn't find any messages, msg is zeroed out to make sure loop condition
	//isn't reading unitialized values.
	ZeroMemory(&msg, sizeof msg);
	do {
		if (PeekMessage(&msg, nullptr, 0,0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		} else {
			Update();
			Render();
			m_device.swapChain()->Present(0, 0);
		}
	} while (msg.message != WM_QUIT);
	return static_cast<int>(msg.wParam);
}

void DxApplication::Update() {
#if ZADANIE_2
	UpdateViewMatrix();
#endif

#if !ZADANIE_3
#if ZADANIE_1
	ComputeAngle();
	XMStoreFloat4x4(&m_modelMtx1, DirectX::XMMatrixRotationY(m_angle));
	D3D11_MAPPED_SUBRESOURCE res;
	m_device.context()->Map(m_cbMVP.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
	DirectX::XMMATRIX mvp = XMLoadFloat4x4(&m_modelMtx1) * XMLoadFloat4x4(&m_viewMtx) * XMLoadFloat4x4(&m_projMtx);
	memcpy(res.pData, &mvp, sizeof(DirectX::XMMATRIX));
	m_device.context()->Unmap(m_cbMVP.get(), 0);
#else
	XMStoreFloat4x4(&m_modelMtx1, XMLoadFloat4x4(&m_modelMtx1) * DirectX::XMMatrixRotationY(0.0001f));
	D3D11_MAPPED_SUBRESOURCE res;
	m_device.context()->Map(m_cbMVP.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
	DirectX::XMMATRIX mvp = XMLoadFloat4x4(&m_modelMtx1) * XMLoadFloat4x4(&m_viewMtx) * XMLoadFloat4x4(&m_projMtx);
	memcpy(res.pData, &mvp, sizeof(DirectX::XMMATRIX));
	m_device.context()->Unmap(m_cbMVP.get(), 0);
#endif
#endif
}

void DxApplication::ComputeAngle() {
	LARGE_INTEGER currentTime;
	QueryPerformanceCounter(&currentTime);

	float deltaTime = static_cast<float>(currentTime.QuadPart - m_previousTime.QuadPart) / static_cast<float>(m_frequency.QuadPart);
	m_previousTime = currentTime;
	m_angle += m_angularVelocity * deltaTime;
	if (m_angle > DirectX::XM_2PI) {
		m_angle -= DirectX::XM_2PI;
	}
}

void DxApplication::Render() {
	float clearColor[] = { 0.5f, 0.5f, 1.0f, 1.0f };
	m_device.context()->ClearRenderTargetView(m_backBuffer.get(), clearColor);
	m_device.context()->ClearDepthStencilView(m_depthBuffer.get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	m_device.context()->VSSetShader(m_vertexShader.get(), nullptr, 0);
	m_device.context()->PSSetShader(m_pixelShader.get(), nullptr, 0);
	m_device.context()->IASetInputLayout(m_layout.get());
	m_device.context()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	ID3D11Buffer* vbs[] = { m_vertexBuffer.get() };
	UINT strides[] = { sizeof(VertexPositionColor) };
	UINT offsets[] = { 0 };
	m_device.context()->IASetVertexBuffers(0, 1, vbs, strides, offsets);
	m_device.context()->IASetIndexBuffer(m_indexBuffer.get(), DXGI_FORMAT_R16_UINT, 0);

#if ZADANIE_3
	ComputeAngle();
	XMStoreFloat4x4(&m_modelMtx1, DirectX::XMMatrixRotationY(m_angle));
	D3D11_MAPPED_SUBRESOURCE res;
	m_device.context()->Map(m_cbMVP.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
	DirectX::XMMATRIX mvp = XMLoadFloat4x4(&m_modelMtx1) * XMLoadFloat4x4(&m_viewMtx) * XMLoadFloat4x4(&m_projMtx);
	memcpy(res.pData, &mvp, sizeof(DirectX::XMMATRIX));
	m_device.context()->Unmap(m_cbMVP.get(), 0);
#endif
	std::array<ID3D11Buffer*, 1> cbs = { m_cbMVP.get() };
	m_device.context()->VSSetConstantBuffers(0, 1, cbs.data());
	m_device.context()->DrawIndexed(36, 0, 0);
#if ZADANIE_3
	XMStoreFloat4x4(&m_modelMtx2, DirectX::XMMatrixTranslation(m_translation[0], m_translation[1], m_translation[2]));
	m_device.context()->Map(m_cbMVP.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &res);
	mvp = XMLoadFloat4x4(&m_modelMtx2) * XMLoadFloat4x4(&m_viewMtx) * XMLoadFloat4x4(&m_projMtx);
	memcpy(res.pData, &mvp, sizeof(DirectX::XMMATRIX));
	m_device.context()->Unmap(m_cbMVP.get(), 0);

	cbs = { m_cbMVP.get() };
	m_device.context()->VSSetConstantBuffers(0, 1, cbs.data());
	m_device.context()->DrawIndexed(36, 0, 0);
#endif
}

std::vector<DxApplication::VertexPositionColor> DxApplication::CreateCubeVertices() {
	return {
		// Front Face
		{ { -0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f } }, // 0
		{ { +0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f } }, // 1
		{ { +0.5f, +0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f } }, // 2
		{ { -0.5f, +0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f } }, // 3
		// Back Face
		{ { -0.5f, -0.5f, +0.5f }, { 1.0f, 1.0f, 0.0f } }, // 4
		{ { +0.5f, -0.5f, +0.5f }, { 1.0f, 1.0f, 0.0f } }, // 5
		{ { +0.5f, +0.5f, +0.5f }, { 1.0f, 1.0f, 0.0f } }, // 6
		{ { -0.5f, +0.5f, +0.5f }, { 1.0f, 1.0f, 0.0f } }, // 7
		// Left Face
		{ { -0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f } }, // 8
		{ { -0.5f, -0.5f, +0.5f }, { 0.0f, 1.0f, 0.0f } }, // 9
		{ { -0.5f, +0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f } }, // 10
		{ { -0.5f, +0.5f, +0.5f }, { 0.0f, 1.0f, 0.0f } }, // 11
		// Right Face
		{ { +0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, 1.0f } }, // 12
		{ { +0.5f, -0.5f, +0.5f }, { 0.0f, 1.0f, 1.0f } }, // 13
		{ { +0.5f, +0.5f, -0.5f }, { 0.0f, 1.0f, 1.0f } }, // 14
		{ { +0.5f, +0.5f, +0.5f }, { 0.0f, 1.0f, 1.0f } }, // 15
		// Up Face
		{ { -0.5f, +0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f } }, // 16
		{ { +0.5f, +0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f } }, // 17
		{ { +0.5f, +0.5f, +0.5f }, { 0.0f, 0.0f, 1.0f } }, // 18
		{ { -0.5f, +0.5f, +0.5f }, { 0.0f, 0.0f, 1.0f } }, // 19
		// Down Face
		{ { -0.5f, -0.5f, +0.5f }, { 1.0f, 0.0f, 1.0f } }, // 20
		{ { +0.5f, -0.5f, +0.5f }, { 1.0f, 0.0f, 1.0f } }, // 21
		{ { +0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 1.0f } }, // 22
		{ { -0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 1.0f } }, // 23
	};
}

std::vector<unsigned short> DxApplication::CreateCubeIndices() {
	return {
		// Front Face
		0,2,1, 0,3,2,
		// Back Face
		5,7,4, 5,6,7,
		// Left Face
		9,10,8, 9,11,10,
		// Right Face
		12,15,13, 12,14,15,
		// Top Face
		16,18,17, 16,19,18,
		// Bottom Face
		20,22,21, 20,23,22
	};
}

void DxApplication::UpdateViewMatrix() {
	XMStoreFloat4x4(&m_viewMtx, DirectX::XMMatrixRotationX(m_cameraPitch) * DirectX::XMMatrixTranslation(0.0f, 0.0f, m_cameraDistance));
}

bool DxApplication::ProcessMessage(WindowMessage& msg) {
#if ZADANIE_2
	msg.result = 0;
	switch (msg.message) {
		case WM_LBUTTONDOWN: {
			m_isLeftButtonDown = true;
			m_prevMousePos.x = LOWORD(msg.lParam);
			m_prevMousePos.y = HIWORD(msg.lParam);
			break;
		}
		case WM_LBUTTONUP: {
			m_isLeftButtonDown = false;
			break;
		}
		case WM_RBUTTONDOWN: {
			m_isRightButtonDown = true;
			m_prevMousePos.x = LOWORD(msg.lParam);
			m_prevMousePos.y = HIWORD(msg.lParam); 
			break;
		}
		case WM_RBUTTONUP: {
			m_isRightButtonDown = false;
			break;
		}
		case WM_MOUSEMOVE: {
			int currentX = LOWORD(msg.lParam); 
			int currentY = HIWORD(msg.lParam); 

			if (m_isLeftButtonDown) {
				int deltaY = currentY - m_prevMousePos.y;
				m_cameraPitch += deltaY * 0.01f;
				m_cameraPitch = std::clamp(m_cameraPitch, -DirectX::XM_PI, DirectX::XM_PI);
				UpdateViewMatrix();
			}
			if (m_isRightButtonDown) {
				int deltaY = currentY - m_prevMousePos.y;
				m_cameraDistance += deltaY * 0.1f; 
				m_cameraDistance = std::clamp(m_cameraDistance, 0.0f, 50.0f);
				UpdateViewMatrix();
			}
			m_prevMousePos.x = currentX;
			m_prevMousePos.y = currentY;
			break;
		}
		case WM_DESTROY:{
			PostQuitMessage(0);
			break;
		}
		case WM_CLOSE: {
			DestroyWindow(m_window.getHandle());
			break;
		}
		default: {
			msg.result = DefWindowProc(m_window.getHandle(), msg.message, msg.wParam, msg.lParam);
			break;
		}
	}
	return (msg.result == 0);
#else
	return false;
#endif
}
