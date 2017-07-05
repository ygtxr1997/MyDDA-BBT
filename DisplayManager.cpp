#include "DisplayManager.h"

//
// ��Ա�б���
//
DISPLAYMANAGER::DISPLAYMANAGER() : m_Device(nullptr),
m_DeviceContext(nullptr),
m_MoveSurf(nullptr),
m_VertexShader(nullptr),
m_PixelShader(nullptr),
m_InputLayout(nullptr),
m_RTV(nullptr),
m_SamplerLinear(nullptr),
m_DirtyVertexBufferAlloc(nullptr),
m_DirtyVertexBufferAllocSize(0)
{
}

//
// ����
//
DISPLAYMANAGER::~DISPLAYMANAGER()
{
	CleanRefs();

	if (m_DirtyVertexBufferAlloc)
	{
		delete[] m_DirtyVertexBufferAlloc;
		m_DirtyVertexBufferAlloc = nullptr;
	}
}

//
// ���� DX_RESOURCES ����Ϣ��ʼ��˽�� D3D ��Ա
// ���� : ��ʼ�� m_Device, m_DeviceContext, m_VertexShader, m_PixelShader, m_InputLayout, m_SamplerLinear
void DISPLAYMANAGER::InitD3D(DX_RESOURCES* Data)
{
	m_Device = Data->Device;
	m_DeviceContext = Data->Context;
	m_VertexShader = Data->VertexShader;
	m_PixelShader = Data->PixelShader;
	m_InputLayout = Data->InputLayout;
	m_SamplerLinear = Data->SamplerLinear;

	// �������ü���
	m_Device->AddRef();
	m_DeviceContext->AddRef();
	m_VertexShader->AddRef();
	m_PixelShader->AddRef();
	m_InputLayout->AddRef();
	m_SamplerLinear->AddRef();
}

// 
//	���� FRAME_DATA��DXGI_OUTDUPL_FRAME_INFO �� �����Ԫ���ݴ���һ֡, ת��Ϊ ID3D11Texture2D
// �����Ҫ :	1. ���Ԫ���ݻ�������С��Ϊ0, �����; �����˳�
//					2. ��� MoveCount != 0 (һ�㶼Ϊ0), �� DISPLAYMANAGER::CopyMove() 
//					3. ��� DirtyCount != 0, �� DISPLAYMANAGER::CopyDirty()
DUPL_RETURN DISPLAYMANAGER::ProcessFrame(_In_ FRAME_DATA* Data, _Inout_ ID3D11Texture2D* SharedSurf, INT OffsetX, INT OffsetY, _In_ DXGI_OUTPUT_DESC* DeskDesc)
{
	DUPL_RETURN Ret = DUPL_RETURN_SUCCESS;

	// ��������κ��ƶ�����
	if (Data->FrameInfo.TotalMetadataBufferSize)
	{
		D3D11_TEXTURE2D_DESC Desc;
		Data->Frame->GetDesc(&Desc);

		if (Data->MoveCount)
		{
			Ret = CopyMove(SharedSurf, reinterpret_cast<DXGI_OUTDUPL_MOVE_RECT*>(Data->MetaData), Data->MoveCount, OffsetX, OffsetY, DeskDesc, Desc.Width, Desc.Height);
			if (Ret != DUPL_RETURN_SUCCESS)
			{
				return Ret;
			}
		}

		if (Data->DirtyCount)
		{
			Ret = CopyDirty(Data->Frame, SharedSurf, reinterpret_cast<RECT*>(Data->MetaData + (Data->MoveCount * sizeof(DXGI_OUTDUPL_MOVE_RECT))), Data->DirtyCount, OffsetX, OffsetY, DeskDesc);
		}
	}

	return Ret;
}

//
// ��������ʹ�õ�D3D�豸
//
ID3D11Device* DISPLAYMANAGER::GetDevice()
{
	return m_Device;
}

//
// ������ü���, �ͷ���Դ
//
void DISPLAYMANAGER::CleanRefs()
{
	if (m_DeviceContext)
	{
		m_DeviceContext->Release();
		m_DeviceContext = nullptr;
	}

	if (m_Device)
	{
		m_Device->Release();
		m_Device = nullptr;
	}

	if (m_MoveSurf)
	{
		m_MoveSurf->Release();
		m_MoveSurf = nullptr;
	}

	if (m_VertexShader)
	{
		m_VertexShader->Release();
		m_VertexShader = nullptr;
	}

	if (m_PixelShader)
	{
		m_PixelShader->Release();
		m_PixelShader = nullptr;
	}

	if (m_InputLayout)
	{
		m_InputLayout->Release();
		m_InputLayout = nullptr;
	}

	if (m_SamplerLinear)
	{
		m_SamplerLinear->Release();
		m_SamplerLinear = nullptr;
	}

	if (m_RTV)
	{
		m_RTV->Release();
		m_RTV = nullptr;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// private ���� ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

//
// �����ƶ�����(һ�㲻�����е�����)
//
void DISPLAYMANAGER::SetMoveRect(_Out_ RECT* SrcRect, _Out_ RECT* DestRect, _In_ DXGI_OUTPUT_DESC* DeskDesc, _In_ DXGI_OUTDUPL_MOVE_RECT* MoveRect, INT TexWIdth, INT TexHeight)
{
	switch (DeskDesc->Rotation)
	{
		case DXGI_MODE_ROTATION_UNSPECIFIED:
		case DXGI_MODE_ROTATION_IDENTITY:
		{
			SrcRect->left = MoveRect->SourcePoint.x;
			SrcRect->top = MoveRect->SourcePoint.y;
			SrcRect->right = MoveRect->SourcePoint.x + MoveRect->DestinationRect.right - MoveRect->DestinationRect.left;
			SrcRect->bottom = MoveRect->SourcePoint.y + MoveRect->DestinationRect.bottom - MoveRect->DestinationRect.top;

			*DestRect = MoveRect->DestinationRect;
			break;
		}
		case DXGI_MODE_ROTATION_ROTATE90:
		{
			break;
		}
		case DXGI_MODE_ROTATION_ROTATE180:
		{
			break;
		}
		case DXGI_MODE_ROTATION_ROTATE270:
		{
			break;
		}
		default:
		{
			RtlZeroMemory(DestRect, sizeof(RECT));
			RtlZeroMemory(SrcRect, sizeof(RECT));
			break;
		}
	}
}

//
// �����ƶ�����(һ�㲻�����е�����)
//
DUPL_RETURN DISPLAYMANAGER::CopyMove(_Inout_ ID3D11Texture2D* SharedSurf, _In_reads_(MoveCoutn) DXGI_OUTDUPL_MOVE_RECT* MoveBuffer, UINT MoveCount, INT OffsetX, INT OffsetY, _In_ DXGI_OUTPUT_DESC* DeskDesc, INT TexWidth, INT TexHeight)
{
	D3D11_TEXTURE2D_DESC FullDesc;
	SharedSurf->GetDesc(&FullDesc);	// FullDesc����SharedSurf��������Ϣ(����)

	// Make new intermediate surface to copy into for moving
	// Ϊ�����ƶ����� �����µ��м��
	if (!m_MoveSurf)
	{
		D3D11_TEXTURE2D_DESC MoveDesc;
		MoveDesc = FullDesc;
		MoveDesc.Width = DeskDesc->DesktopCoordinates.right - DeskDesc->DesktopCoordinates.left;
		MoveDesc.Height = DeskDesc->DesktopCoordinates.bottom - DeskDesc->DesktopCoordinates.top;
		MoveDesc.BindFlags = D3D11_BIND_RENDER_TARGET;	// D3D11_BIND_FLAG, ����󶨵��Կ����ߵķ�ʽ, �ο�"�Կ���Ⱦ��ˮ��"
		MoveDesc.MiscFlags = 0;
		HRESULT hr = m_Device->CreateTexture2D(&MoveDesc, nullptr, &m_MoveSurf);	// ID3D11Device::CreateTexture2D(), ����Texture2D��m_MoveSurf

		if (FAILED(hr))
		{
		}
	}

	// ��ʼ����
	for (UINT i = 0; i < MoveCount; ++i)
	{
		RECT SrcRect;
		RECT DestRect;

		SetMoveRect(&SrcRect, &DestRect, DeskDesc, &(MoveBuffer[i]), TexWidth, TexHeight);	// DISPLAYMANAGER::SetMoveRect(), ��MoveBuffer������DeskDesc��ʼ��SrcRect��DestRect

		// Copy rect out of shared surface
		// �ӹ�������и���rect
		D3D11_BOX Box;		// D3D11����ģ��
		Box.left = SrcRect.left + DeskDesc->DesktopCoordinates.left - OffsetX;
		Box.top = SrcRect.top + DeskDesc->DesktopCoordinates.top - OffsetY;
		Box.front = 0;
		Box.right = SrcRect.right + DeskDesc->DesktopCoordinates.left - OffsetX;
		Box.bottom = SrcRect.bottom + DeskDesc->DesktopCoordinates.top - OffsetY;
		Box.back = 1;
		m_DeviceContext->CopySubresourceRegion(m_MoveSurf, 0, SrcRect.left, SrcRect.top, 0, SharedSurf, 0, &Box);		// ID3D11DeviceContext::CopySubresourceRegion(), ����Դ��Դ��Ŀ����Դ������, ������Ϊ_in_
		// --- ����˵��:		m_MoveSurf				Ŀ����Դ, �ƶ�����(�м��)
		//							0								Ŀ������Դ���Ϊ0
		//							SrcRect.left				Ŀ�������x����
		//							SrcRect.top				Ŀ�������y����
		//							0								Ŀ�������z����
		//							SharedSurf				Դ��Դ, �������
		//							0								Դ����Դ�ı��Ϊ0
		//							&Box						�����Ƶķ�Χ����Box�ķ�Χ

		// Copy back to shared surface
		// ���ƻع������
		Box.left = SrcRect.left;
		Box.top = SrcRect.top;
		Box.front = 0;
		Box.right = SrcRect.right;
		Box.bottom = SrcRect.bottom;
		Box.back = 1;
		m_DeviceContext->CopySubresourceRegion(SharedSurf, 0, DestRect.left + DeskDesc->DesktopCoordinates.left - OffsetX, DestRect.top + DeskDesc->DesktopCoordinates.top - OffsetY, 0, m_MoveSurf, 0, &Box);
	}

	return DUPL_RETURN_SUCCESS;
}

//
// �����ඥ��
// ���� :	��������Ρ�DXGI_OUTPUT_DESC��D3D11_TEXTURE2D_DESC (Full and This) �����ඥ�� VERTEX
// �����Ҫ :	1. ��ȡ�������� (��ߡ�����λ��)
//					2. ���ݳ������� VERTEX �Ķ�ά����
//					3. ���� VERTEX ����ά����
void DISPLAYMANAGER::SetDirtyVert(_Out_writes_(NUMVERTICES) VERTEX* Vertices, _In_ RECT* Dirty, INT OffsetX, INT OffsetY, _In_ DXGI_OUTPUT_DESC* DeskDesc, _In_ D3D11_TEXTURE2D_DESC* FullDesc, _In_ D3D11_TEXTURE2D_DESC* ThisDesc)
{
	// 1. ��ȡ��������
	INT CenterX = FullDesc->Width / 2;
	INT CenterY = FullDesc->Height / 2;

	INT Width = DeskDesc->DesktopCoordinates.right - DeskDesc->DesktopCoordinates.left;
	INT Height = DeskDesc->DesktopCoordinates.bottom - DeskDesc->DesktopCoordinates.top;

	RECT DestDirty = *Dirty;

	// 2. ���ݳ������ö�ά����
	//		Vertices[0] :	left, bottom
	//		Vertices[1] : left, top
	//		Vertices[2] : right, bottom
	//		Vertices[5] : right, top
	//		����ͼ��ʾ :
	//		(4) 1 ---------------------- 5
	//		      |									  |
	//		      |									  |
	//		      |									  |
	//		     0 ---------------------  2 (3)
	switch (DeskDesc->Rotation)
	{
	case DXGI_MODE_ROTATION_ROTATE90:
		break;
	case DXGI_MODE_ROTATION_ROTATE180:
		break;
	case DXGI_MODE_ROTATION_ROTATE270:
		break;
	case DXGI_MODE_ROTATION_UNSPECIFIED:
		break;
	case DXGI_MODE_ROTATION_IDENTITY:
	{
		Vertices[0].TexCoord = DirectX::XMFLOAT2(Dirty->left / static_cast<FLOAT>(ThisDesc->Width), Dirty->bottom / static_cast<FLOAT>(ThisDesc->Height));
		Vertices[1].TexCoord = DirectX::XMFLOAT2(Dirty->left / static_cast<FLOAT>(ThisDesc->Width), Dirty->top / static_cast<FLOAT>(ThisDesc->Height));
		Vertices[2].TexCoord = DirectX::XMFLOAT2(Dirty->right / static_cast<FLOAT>(ThisDesc->Width), Dirty->bottom / static_cast<FLOAT>(ThisDesc->Height));
		Vertices[5].TexCoord = DirectX::XMFLOAT2(Dirty->right / static_cast<FLOAT>(ThisDesc->Width), Dirty->top / static_cast<FLOAT>(ThisDesc->Height));
		break;
	}
	}

	// 3. ���� VERTEX ����ά����
	//		����ͼ��ʾ : (z = 0 ��ƽ��)
	//
	//
	//															   ^
	//																|  y��
	//																|
	//		--------------------------------------------------------------------------------
	//		|														|															|
	//		|			1-----5									|															|
	//		|			 |		  |	 Dirty						|															|
	//		|			0-----2									|															|
	//		|														|															|
	//		| ---------------------------------Center------------------------------------- |-----> x��
	//		|														|															|
	//		|														|															|
	//		|														|															|
	//		|														|															|
	//		|														|															|
	//		--------------------------------------------------------------------------------
	//
	//		(1 = 4, 2 = 3)
	//
	Vertices[0].Pos = DirectX::XMFLOAT3((DestDirty.left + DeskDesc->DesktopCoordinates.left - OffsetX - CenterX) / static_cast<FLOAT>(CenterX),
		-1 * (DestDirty.bottom + DeskDesc->DesktopCoordinates.top - OffsetY - CenterY) / static_cast<FLOAT>(CenterY),
		0.0f);
	Vertices[1].Pos = DirectX::XMFLOAT3((DestDirty.left + DeskDesc->DesktopCoordinates.left - OffsetX - CenterX) / static_cast<FLOAT>(CenterX),
		-1 * (DestDirty.top + DeskDesc->DesktopCoordinates.top - OffsetY - CenterY) / static_cast<FLOAT>(CenterY),
		0.0f);
	Vertices[2].Pos = DirectX::XMFLOAT3((DestDirty.right + DeskDesc->DesktopCoordinates.left - OffsetX - CenterX) / static_cast<FLOAT>(CenterX),
		-1 * (DestDirty.bottom + DeskDesc->DesktopCoordinates.top - OffsetY - CenterY) / static_cast<FLOAT>(CenterY),
		0.0f);
	Vertices[3].Pos = Vertices[2].Pos;
	Vertices[4].Pos = Vertices[1].Pos;
	Vertices[5].Pos = DirectX::XMFLOAT3((DestDirty.right + DeskDesc->DesktopCoordinates.left - OffsetX - CenterX) / static_cast<FLOAT>(CenterX),
		-1 * (DestDirty.top + DeskDesc->DesktopCoordinates.top - OffsetY - CenterY) / static_cast<FLOAT>(CenterY),
		0.0f);

	Vertices[3].TexCoord = Vertices[2].TexCoord;
	Vertices[4].TexCoord = Vertices[1].TexCoord;
}

//
// ���������
// ���� :	���� ����Ρ�DXGI_OUTPUT_DESC �� SrcSurface��SharedSurf, ���µ�֡���Ƶ� m_RTV
// �����Ҫ :	1. �ֱ��ȡ SharedSurf �� SrcSurface ��������Ϣ
//					2. m_Device->CreateShaderResourceView(DESC)
//					3. m_DeviceContext->OM/VS/PS/IA
//					4. ������Ҫ���¿��� DirtyVertex �Ŀռ�
//					5. ���� DirtyVertex, DISPLAYMANAGER::SetDirtyVert
//					6. m_Device->CreateBuffer(DESC) & m_DeviceContext->IA
//					7. m_DeviceContext->RSSetViewPorts()
//					8. m_DeviceContext->Draw()
DUPL_RETURN DISPLAYMANAGER::CopyDirty(_In_ ID3D11Texture2D* SrcSurface, _Inout_ ID3D11Texture2D* SharedSurf, _In_reads_(DirtyCount) RECT* DirtyBuffer, UINT DirtyCount, INT OffsetX, INT OffsetY, _In_ DXGI_OUTPUT_DESC* DeskDesc)
{
	HRESULT hr;

	D3D11_TEXTURE2D_DESC FullDesc;
	SharedSurf->GetDesc(&FullDesc);

	D3D11_TEXTURE2D_DESC ThisDesc;
	SrcSurface->GetDesc(&ThisDesc);

	if (!m_RTV)
	{
		hr = m_Device->CreateRenderTargetView(SharedSurf, nullptr, &m_RTV);
	}

	// ��ɫ������
	D3D11_SHADER_RESOURCE_VIEW_DESC ShaderDesc;
	ShaderDesc.Format = ThisDesc.Format;
	ShaderDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	ShaderDesc.Texture2D.MostDetailedMip = ThisDesc.MipLevels - 1;
	ShaderDesc.Texture2D.MipLevels = ThisDesc.MipLevels;

	// �����µ���ɫ����Դ��ͼ
	ID3D11ShaderResourceView* ShaderResource = nullptr;
	hr = m_Device->CreateShaderResourceView(SrcSurface, &ShaderDesc, &ShaderResource);

	// DirectX��׼����
	FLOAT BlendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
	m_DeviceContext->OMSetBlendState(nullptr, BlendFactor, 0xFFFFFFFF);
	m_DeviceContext->OMSetRenderTargets(1, &m_RTV, nullptr);
	m_DeviceContext->VSSetShader(m_VertexShader, nullptr, 0);
	m_DeviceContext->PSSetShader(m_PixelShader, nullptr, 0);
	m_DeviceContext->PSSetShaderResources(0, 1, &ShaderResource);
	m_DeviceContext->PSSetSamplers(0, 1, &m_SamplerLinear);
	m_DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);		// ͼԪ���˲����������б�ģʽ

	// ����ζ���ռ������Ҫ���¿��ٿռ�
	UINT BytesNeeded = sizeof(VERTEX) * NUMVERTICES * DirtyCount;
	if (BytesNeeded > m_DirtyVertexBufferAllocSize)
	{
		if (m_DirtyVertexBufferAlloc)
		{
			delete[] m_DirtyVertexBufferAlloc;
		}

		m_DirtyVertexBufferAlloc = new (std::nothrow) BYTE[BytesNeeded];
		if (!m_DirtyVertexBufferAlloc)
		{
			m_DirtyVertexBufferAllocSize = 0;
		}

		m_DirtyVertexBufferAllocSize = BytesNeeded;
	}

	// ��������Ϣ��ӽ�DirtyVertex
	VERTEX* DirtyVertex = reinterpret_cast<VERTEX*>(m_DirtyVertexBufferAlloc);
	for (UINT i = 0; i < DirtyCount; ++i, DirtyVertex += NUMVERTICES)
	{
		SetDirtyVert(DirtyVertex, &(DirtyBuffer[i]), OffsetX, OffsetY, DeskDesc, &FullDesc, &ThisDesc);
	}

	// �������㻺��, �Լ���Ӧ��������Ϣ
	D3D11_BUFFER_DESC BufferDesc;
	RtlZeroMemory(&BufferDesc, sizeof(BufferDesc));
	BufferDesc.Usage = D3D11_USAGE_DEFAULT;
	BufferDesc.ByteWidth = BytesNeeded;
	BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	BufferDesc.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	RtlZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = m_DirtyVertexBufferAlloc;

	// ����ID3D11Buffer
	ID3D11Buffer* VertBuf = nullptr;
	hr = m_Device->CreateBuffer(&BufferDesc, &InitData, &VertBuf);

	// DirectX��IA��Դ���ض��㻺��
	UINT Stride = sizeof(VERTEX);
	UINT Offset = 0;
	m_DeviceContext->IASetVertexBuffers(0, 1, &VertBuf, &Stride, &Offset);

	// �����ӿ�
	D3D11_VIEWPORT VP;
	VP.Width = static_cast<FLOAT>(FullDesc.Width);
	VP.Height = static_cast<FLOAT>(FullDesc.Height);
	VP.MinDepth = 0.0f;
	VP.MaxDepth = 1.0f;
	VP.TopLeftX = 0.0f;
	VP.TopLeftY = 0.0f;
	m_DeviceContext->RSSetViewports(1, &VP);

	// ��
	m_DeviceContext->Draw(NUMVERTICES * DirtyCount, 0);

	VertBuf->Release();
	VertBuf = nullptr;

	ShaderResource->Release();
	ShaderResource = nullptr;

	return DUPL_RETURN_SUCCESS;
}

