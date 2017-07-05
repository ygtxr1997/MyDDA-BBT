#include "OutputManager.h"

using namespace::DirectX;

//
// 初始化列表构造
//
OUTPUTMANAGER::OUTPUTMANAGER() : m_SwapChain(nullptr),
m_Device(nullptr),
m_Factory(nullptr),
m_DeviceContext(nullptr),
m_RTV(nullptr),
m_SamplerLinear(nullptr),
m_BlendState(nullptr),
m_VertexShader(nullptr),
m_PixelShader(nullptr),
m_InputLayout(nullptr),
m_SharedSurf(nullptr),
m_KeyMutex(nullptr),
m_WindowHandle(nullptr),
m_NeedsResize(false),
m_OcclusionCookie(0)
{
}

//
// 清空引用析构
//
OUTPUTMANAGER::~OUTPUTMANAGER()
{
	CleanRefs();
}

//
// 窗口尺寸发生变化
//
void OUTPUTMANAGER::WindowResize()
{
	m_NeedsResize = true;
}

//
// 初始化所有相关变量
// 步骤概要 :
//		1. 窗口句柄
//		2. D3D_DRIVER_TYPE
//		3. D3D_FEATURE_LEVEL
//		4. D3D11CreateDevice()
//		5. m_Device ---QI--> IDXGIDevice
//		6. IDXGIDevice ---GP--> IDXGIAdapter
//		7. IDXGIAdapter ---GP--> IDXGIFactory
//		8. IDXGIFactory->RegisterOcclusionStatusWindow()
//		9. 获取窗口尺寸
//		10. IDXGIFactory->CreateSwapChain(DESC)
//		11. 禁用 ALT+ENTER
//		12. →OutputManager::CreateSharedSurf()
//		13. →OutputManager::MakeRTV()
//		14. →OutputManager::SetViewPort()
//		15. m_Device->CreateSamplerState(DESC)
//		16. m_Device->CreateBlendState(DESC) 
//		17. →OutputManager::InitShaders()
//		18. MoveWindow()
DUPL_RETURN OUTPUTMANAGER::InitOutput(HWND Window, INT SingleOutput, _Out_ UINT* OutCount, _Out_ RECT* DeskBounds)
{
	HRESULT hr;

	// 窗口句柄
	m_WindowHandle = Window;

	// 支持的驱动类型
	D3D_DRIVER_TYPE DriverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE
	};
	UINT NumDriverTypes = ARRAYSIZE(DriverTypes);

	// 支持的版本
	D3D_FEATURE_LEVEL FeatureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_1
	};
	UINT NumFeatureLevels = ARRAYSIZE(FeatureLevels);
	D3D_FEATURE_LEVEL FeatureLevel;

	// 创建设备
	for (UINT DriverTypeIndex = 0; DriverTypeIndex < NumDriverTypes; ++DriverTypeIndex)
	{
		hr = D3D11CreateDevice(nullptr, DriverTypes[DriverTypeIndex], nullptr, 0, FeatureLevels, NumFeatureLevels,
			D3D11_SDK_VERSION, &m_Device, &FeatureLevel, &m_DeviceContext);
		if (SUCCEEDED(hr))
			break;
	}

	// 获取DXGI device
	IDXGIDevice* DxgiDevice = nullptr;
	hr = m_Device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&DxgiDevice));

	// 获取DXGI adapter
	IDXGIAdapter* DxgiAdapter = nullptr;
	hr = DxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&DxgiAdapter));
	DxgiDevice->Release();
	DxgiDevice = nullptr;

	// 获取DXGI factory
	hr = DxgiAdapter->GetParent(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&m_Factory));
	DxgiAdapter->Release();
	DxgiAdapter = nullptr;

	// IDXGI::注册窗口状态
	hr = m_Factory->RegisterOcclusionStatusWindow(Window, OCCLUSION_STATUS_MSG, &m_OcclusionCookie);

	// 获取窗口尺寸
	RECT WindowRect;
	GetClientRect(m_WindowHandle, &WindowRect);
	UINT Width = WindowRect.right - WindowRect.left;
	UINT Height = WindowRect.bottom - WindowRect.top;

	// 创建交换链
	DXGI_SWAP_CHAIN_DESC1 SwapChainDesc;
	RtlZeroMemory(&SwapChainDesc, sizeof(SwapChainDesc));

	SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	SwapChainDesc.BufferCount = 2;
	SwapChainDesc.Width = Width;
	SwapChainDesc.Height = Height;
	SwapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapChainDesc.SampleDesc.Count = 1;
	SwapChainDesc.SampleDesc.Quality = 0;
	hr = m_Factory->CreateSwapChainForHwnd(m_Device, Window, &SwapChainDesc, nullptr, nullptr, &m_SwapChain);


	// 禁用窗口的全屏快捷键
	hr = m_Factory->MakeWindowAssociation(Window, DXGI_MWA_NO_ALT_ENTER);

	// 创建共享纹理, OutputManager::
	DUPL_RETURN Return = CreateSharedSurf(SingleOutput, OutCount, DeskBounds);

	// 创建新的渲染目标视图
	Return = MakeRTV();

	// 设置视口
	SetViewPort(Width, Height);

	// 创建采样器状态
	D3D11_SAMPLER_DESC SampDesc;
	RtlZeroMemory(&SampDesc, sizeof(SampDesc));
	SampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	SampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP; // 寻址范围在[0.0, 1.0]
	SampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	SampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	SampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	SampDesc.MinLOD = 0;
	SampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = m_Device->CreateSamplerState(&SampDesc, &m_SamplerLinear);

	// 创建混色状态
	D3D11_BLEND_DESC BlendStateDesc;
	BlendStateDesc.AlphaToCoverageEnable = FALSE;
	BlendStateDesc.IndependentBlendEnable = FALSE;
	BlendStateDesc.RenderTarget[0].BlendEnable = TRUE;
	BlendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	BlendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	BlendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	BlendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	BlendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	BlendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	BlendStateDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	hr = m_Device->CreateBlendState(&BlendStateDesc, &m_BlendState);

	// 初始化着色器, OutputManager::
	Return = InitShaders();

	// 调整窗口大小
	GetWindowRect(m_WindowHandle, &WindowRect);	// 根据窗口句柄获取窗口矩形
	MoveWindow(m_WindowHandle, WindowRect.left, WindowRect.top, (DeskBounds->right - DeskBounds->left) / 2, (DeskBounds->bottom - DeskBounds->top) / 2, TRUE);	// 移动窗口

	return Return;
}

//
// 创建共享纹理
// 功能 : 创建共享纹理 m_SharedSurf、互斥锁 m_KeyMutex
//		  确定 OutputCount 的值
//		  获取正确的桌面尺寸
DUPL_RETURN OUTPUTMANAGER::CreateSharedSurf(INT SingleOutput, _Out_ UINT* OutCount, _Out_ RECT* DeskBounds)
{
	HRESULT hr;

	// 获取DXGI资源(Device, Adapter, Output)
	// --- Device
	IDXGIDevice* DxgiDevice = nullptr;
	hr = m_Device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&DxgiDevice));

	// --- Adapter
	IDXGIAdapter* DxgiAdapter = nullptr;
	hr = DxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&DxgiAdapter));
	DxgiDevice->Release();
	DxgiDevice = nullptr;

	DeskBounds->left = INT_MAX;
	DeskBounds->right = INT_MIN;
	DeskBounds->top = INT_MAX;
	DeskBounds->bottom = INT_MIN;

	// --- Output
	IDXGIOutput* DxgiOutput = nullptr;

	// 确保尺寸和坐标正确
	UINT OutputCount;
	if (SingleOutput < 0)
	{
		hr = S_OK;
		for (OutputCount = 0; SUCCEEDED(hr); ++OutputCount)
		{
			if (DxgiOutput)
			{
				DxgiOutput->Release();
				DxgiOutput = nullptr;
			}
			hr = DxgiAdapter->EnumOutputs(OutputCount, &DxgiOutput);	// 枚举输出
			if (DxgiOutput && (hr != DXGI_ERROR_NOT_FOUND))
			{
				DXGI_OUTPUT_DESC DesktopDesc;
				DxgiOutput->GetDesc(&DesktopDesc);

				DeskBounds->left = min(DesktopDesc.DesktopCoordinates.left, DeskBounds->left);
				DeskBounds->top = min(DesktopDesc.DesktopCoordinates.top, DeskBounds->top);
				DeskBounds->right = max(DesktopDesc.DesktopCoordinates.right, DeskBounds->right);
				DeskBounds->bottom = max(DesktopDesc.DesktopCoordinates.bottom, DeskBounds->bottom);
			}
		}

		--OutputCount;
	}
	else
	{
		hr = DxgiAdapter->EnumOutputs(SingleOutput, &DxgiOutput);

		DXGI_OUTPUT_DESC DesktopDesc;
		DxgiOutput->GetDesc(&DesktopDesc);
		*DeskBounds = DesktopDesc.DesktopCoordinates;

		DxgiOutput->Release();
		DxgiOutput = nullptr;

		OutputCount = 1;
	}

	DxgiAdapter->Release();
	DxgiAdapter = nullptr;

	*OutCount = OutputCount;

	// 为复制线程创建共享纹理m_SharedSurf
	D3D11_TEXTURE2D_DESC DeskTexD;
	RtlZeroMemory(&DeskTexD, sizeof(D3D11_TEXTURE2D_DESC));
	DeskTexD.Width = DeskBounds->right - DeskBounds->left;
	DeskTexD.Height = DeskBounds->bottom - DeskBounds->top;
	DeskTexD.MipLevels = 1;
	DeskTexD.ArraySize = 1;
	DeskTexD.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	DeskTexD.SampleDesc.Count = 1;
	DeskTexD.Usage = D3D11_USAGE_DEFAULT;
	DeskTexD.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	DeskTexD.CPUAccessFlags = 0;
	DeskTexD.MiscFlags = D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;

	hr = m_Device->CreateTexture2D(&DeskTexD, nullptr, &m_SharedSurf);

	// DXGI互斥锁
	hr = m_SharedSurf->QueryInterface(__uuidof(IDXGIKeyedMutex), reinterpret_cast<void**>(&m_KeyMutex));

	return DUPL_RETURN_SUCCESS;
}

//
// 不断更新窗口
// 步骤概要 :	
//		1. 请求锁
//		2. →DrawFrame()
//		3. →DrawMouse()
//		4. 释放锁
//		5. m_SwapChain->Present()
DUPL_RETURN OUTPUTMANAGER::UpdateApplicationWindow(_In_ PTR_INFO* PointerInfo, _Inout_ bool* Occluded)
{
	// 同步过程
	HRESULT hr = m_KeyMutex->AcquireSync(1, 100);
	if (hr == static_cast<HRESULT>(WAIT_TIMEOUT))
	{
		// 另一个线程还没打开
		return DUPL_RETURN_SUCCESS;
	}

	// 锁打开，可以开始画图
	DUPL_RETURN Ret = DrawFrame(); // OutputManager::
	if (Ret == DUPL_RETURN_SUCCESS)
	{
		if (PointerInfo->Visible)
		{
			// 绘制鼠标
			Ret = DrawMouse(PointerInfo);
		}
	}

	/////////////////
	//TODO: Map
	D3D11_TEXTURE2D_DESC FullDesc;
	m_SharedSurf->GetDesc(&FullDesc);

	D3D11_TEXTURE2D_DESC CopyBufferDesc;
	CopyBufferDesc.Width = FullDesc.Width;
	CopyBufferDesc.Height = FullDesc.Height;
	CopyBufferDesc.MipLevels = 1;
	CopyBufferDesc.ArraySize = 1;
	CopyBufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	CopyBufferDesc.SampleDesc.Count = 1;
	CopyBufferDesc.SampleDesc.Quality = 0;
	CopyBufferDesc.Usage = D3D11_USAGE_STAGING;
	CopyBufferDesc.BindFlags = 0;
	CopyBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	CopyBufferDesc.MiscFlags = 0;

	ID3D11Texture2D* CopyBuffer = nullptr;
	hr = m_Device->CreateTexture2D(&CopyBufferDesc, nullptr, &CopyBuffer);

	m_DeviceContext->CopyResource(CopyBuffer, m_SharedSurf);

	IDXGISurface* DxgiSurface = nullptr;
	hr = CopyBuffer->QueryInterface(__uuidof(IDXGISurface), (void**)&DxgiSurface);
	CopyBuffer->Release();
	CopyBuffer = nullptr;

	DXGI_MAPPED_RECT MappedSurface;
	hr = DxgiSurface->Map(&MappedSurface, DXGI_MAP_READ);

	hr = DxgiSurface->Unmap();
	DxgiSurface->Release();
	DxgiSurface = nullptr;

	//////////////////

	// 释放锁
	hr = m_KeyMutex->ReleaseSync(0);

	// 显示到窗口中
	if (Ret == DUPL_RETURN_SUCCESS)
	{
		hr = m_SwapChain->Present(1, 0);

		if (hr == DXGI_STATUS_OCCLUDED)
			*Occluded = true;
	}

	return Ret;
}

//
// 返回共享句柄
//
HANDLE OUTPUTMANAGER::GetSharedHandle()
{
	HANDLE Hnd = nullptr;

	IDXGIResource* DXGIResource = nullptr;
	HRESULT hr = m_SharedSurf->QueryInterface(__uuidof(IDXGIResource), reinterpret_cast<void**>(&DXGIResource));
	if (SUCCEEDED(hr))
	{
		DXGIResource->GetSharedHandle(&Hnd);
		DXGIResource->Release();
		DXGIResource = nullptr;
	}

	return Hnd;
}

//
// 向 backbuffer 写入帧
// 步骤概要 : 
//		1. →ResizeSwapChain()
//		2. 定义 VERTEX[]
//		3. m_Device->CreateShaderResourceView(DESC)
//		4. m_DeviceContext->OM/VS/PS/IA
//		5. m_DeviceContext->CreateBuffer(DESC) & IA
//		6. m_DeviceContext->Draw()
DUPL_RETURN OUTPUTMANAGER::DrawFrame()
{
	HRESULT hr;

	// 可能需要重新分配swapchain的空间
	if (m_NeedsResize)
	{
		DUPL_RETURN Ret = ResizeSwapChain();	// OutputManager::
		m_NeedsResize = false;
	}

	// 绘制顶点
	VERTEX Vertices[NUMVERTICES] =
	{
		{ XMFLOAT3(-1.0f, -1.0f, 0), XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 0), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 0), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 0), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 0), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 0), XMFLOAT2(1.0f, 0.0f) },
	};

	// 着色器及其描述
	D3D11_TEXTURE2D_DESC FrameDesc;
	m_SharedSurf->GetDesc(&FrameDesc);

	D3D11_SHADER_RESOURCE_VIEW_DESC ShaderDesc;
	ShaderDesc.Format = FrameDesc.Format;
	ShaderDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	ShaderDesc.Texture2D.MostDetailedMip = FrameDesc.MipLevels - 1;
	ShaderDesc.Texture2D.MipLevels = FrameDesc.MipLevels;

	ID3D11ShaderResourceView* ShaderResource = nullptr;
	hr = m_Device->CreateShaderResourceView(m_SharedSurf, &ShaderDesc, &ShaderResource);

	// 设置资源, ID3D11DeviceContext::
	UINT Stride = sizeof(VERTEX);
	UINT Offset = 0;
	FLOAT blendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
	m_DeviceContext->OMSetBlendState(nullptr, blendFactor, 0xffffffff);
	m_DeviceContext->OMSetRenderTargets(1, &m_RTV, nullptr);
	m_DeviceContext->VSSetShader(m_VertexShader, nullptr, 0);
	m_DeviceContext->PSSetShader(m_PixelShader, nullptr, 0);
	m_DeviceContext->PSSetShaderResources(0, 1, &ShaderResource);
	m_DeviceContext->PSSetSamplers(0, 1, &m_SamplerLinear);
	m_DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// 创建顶点缓冲
	D3D11_BUFFER_DESC BufferDesc;
	RtlZeroMemory(&BufferDesc, sizeof(BufferDesc));
	BufferDesc.Usage = D3D11_USAGE_DEFAULT;
	BufferDesc.ByteWidth = sizeof(VERTEX) * NUMVERTICES;
	BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	BufferDesc.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData;
	RtlZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = Vertices;

	ID3D11Buffer* VertexBuffer = nullptr;
	hr = m_Device->CreateBuffer(&BufferDesc, &InitData, &VertexBuffer);

	m_DeviceContext->IASetVertexBuffers(0, 1, &VertexBuffer, &Stride, &Offset);

	// 绘制 texture 到 render target view
	m_DeviceContext->Draw(NUMVERTICES, 0);

	VertexBuffer->Release();
	VertexBuffer = nullptr;

	ShaderResource->Release();
	ShaderResource = nullptr;

	return DUPL_RETURN_SUCCESS;
}

//
// 绘制不同种类的鼠标指针 (_MONOCHROME 或者 _MASKED_COLOR 类型)
// 步骤概要 : 
//		1. 获取坐标, 修正坐标
//		2. m_Device->CreateTexture2D(DESC) 生成 CopyBuffer
//		3. m_DeviceContext->CopySubresourceRegion()
//		4. CopyBuffer ---QI--> IDXGISurface
//		5. IDXGISurface::Map() 生成 MappedSurface
//		6. *InitBuffer32 与 *Desktop32
//		7. 根据是否为 Mono 处理鼠标指针图片?
DUPL_RETURN OUTPUTMANAGER::ProcessMonoMask(bool IsMono, _Inout_ PTR_INFO* PtrInfo, _Out_ INT* PtrWidth, _Out_ INT* PtrHeight, _Out_ INT* PtrLeft, _Out_ INT* PtrTop, _Outptr_result_bytebuffer_(*PtrHeight * *PtrWidth * BPP)BYTE** InitBuffer, _Out_ D3D11_BOX* Box)
{
	// 坐标
	D3D11_TEXTURE2D_DESC FullDesc;
	m_SharedSurf->GetDesc(&FullDesc);
	INT DesktopWidth = FullDesc.Width;
	INT DesktopHeight = FullDesc.Height;

	INT GivenLeft = PtrInfo->Position.x;
	INT GivenTop = PtrInfo->Position.y;

	// 修正坐标
	if (GivenLeft < 0)
	{
		*PtrWidth = GivenLeft + static_cast<INT>(PtrInfo->ShapeInfo.Width); // 左溢
	}
	else if ((GivenLeft + static_cast<INT>(PtrInfo->ShapeInfo.Width)) > DesktopWidth)
	{
		*PtrWidth = static_cast<INT>(PtrInfo->ShapeInfo.Width);					// 右溢
	}
	else
	{
		*PtrWidth = static_cast<INT>(PtrInfo->ShapeInfo.Width);
	}

	if (IsMono)	// 修改
	{
		PtrInfo->ShapeInfo.Height = PtrInfo->ShapeInfo.Height / 2;
	}

	if (GivenTop < 0)
	{
		*PtrHeight = GivenTop + static_cast<INT>(PtrInfo->ShapeInfo.Height);
	}
	else if ((GivenTop + static_cast<INT>(PtrInfo->ShapeInfo.Height)) > DesktopHeight)
	{
		*PtrHeight = DesktopHeight - GivenTop;
	}
	else
	{
		*PtrHeight = static_cast<INT>(PtrInfo->ShapeInfo.Height);
	}

	if (IsMono)	// 还原
	{
		PtrInfo->ShapeInfo.Height = PtrInfo->ShapeInfo.Height * 2;
	}

	*PtrLeft = (GivenLeft < 0) ? 0 : GivenLeft;
	*PtrTop = (GivenTop < 0) ? 0 : GivenTop;

	// Texture2D 描述 与 创建
	D3D11_TEXTURE2D_DESC CopyBufferDesc;
	CopyBufferDesc.Width = *PtrWidth;
	CopyBufferDesc.Height = *PtrHeight;
	CopyBufferDesc.MipLevels = 1;
	CopyBufferDesc.ArraySize = 1;
	CopyBufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	CopyBufferDesc.SampleDesc.Count = 1;
	CopyBufferDesc.SampleDesc.Quality = 0;
	CopyBufferDesc.Usage = D3D11_USAGE_STAGING;
	CopyBufferDesc.BindFlags = 0;
	CopyBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	CopyBufferDesc.MiscFlags = 0;

	ID3D11Texture2D* CopyBuffer = nullptr;
	HRESULT hr = m_Device->CreateTexture2D(&CopyBufferDesc, nullptr, &CopyBuffer);

	// 复制桌面图片需要的部分
	Box->left = *PtrLeft;
	Box->top = *PtrTop;
	Box->right = *PtrLeft + *PtrWidth;
	Box->bottom = *PtrTop + *PtrHeight;
	m_DeviceContext->CopySubresourceRegion(CopyBuffer, 0, 0, 0, 0, m_SharedSurf, 0, Box);

	// IDXGISurface
	IDXGISurface* CopySurface = nullptr;
	hr = CopyBuffer->QueryInterface(__uuidof(IDXGISurface), (void **)&CopySurface);
	CopyBuffer->Release();
	CopyBuffer = nullptr;

	// DXGI_MAPPED_RECT
	DXGI_MAPPED_RECT MappedSurface;
	hr = CopySurface->Map(&MappedSurface, DXGI_MAP_READ);

	// 映射到 InitBuffer
	*InitBuffer = new (std::nothrow) BYTE[*PtrWidth * *PtrHeight * BPP];

	UINT* InitBuffer32 = reinterpret_cast<UINT*>(*InitBuffer);
	UINT* Desktop32 = reinterpret_cast<UINT*>(MappedSurface.pBits);
	UINT  DesktopPitchInPixels = MappedSurface.Pitch / sizeof(UINT);

	UINT SkipX = (GivenLeft < 0) ? (-1 * GivenLeft) : (0);		// 需要跳过的部分
	UINT SkipY = (GivenTop < 0) ? (-1 * GivenTop) : (0);		// 需要跳过的部分

	// 根据是否为 Mono 开始处理指针图片 ?
	if (IsMono)
	{
		for (INT Row = 0; Row < *PtrHeight; ++Row)
		{
			// 设置蒙版
			BYTE Mask = 0x80;
			Mask = Mask >> (SkipX % 8);
			for (INT Col = 0; Col < *PtrWidth; ++Col)
			{
				// 获取合适的蒙版
				BYTE AndMask = PtrInfo->PtrShapeBuffer[((Col + SkipX) / 8 + ((Row + SkipY) * (PtrInfo->ShapeInfo.Pitch)))] & Mask;
				BYTE XorMask = PtrInfo->PtrShapeBuffer[((Col + SkipX) / 8) + ((Row + SkipY + (PtrInfo->ShapeInfo.Height / 2)) * (PtrInfo->ShapeInfo.Pitch))] & Mask;
				UINT AndMask32 = (AndMask) ? 0xFFFFFFFF : 0xFF000000;
				UINT XorMask32 = (XorMask) ? 0x00FFFFFF : 0x00000000;

				// 修改像素
				InitBuffer32[(Row * *PtrWidth) + Col] = (Desktop32[(Row * DesktopPitchInPixels) + Col] & AndMask32) ^ XorMask32;

				// 修改蒙版
				if (Mask == 0x01)
				{
					Mask = 0x80;
				}
				else
				{
					Mask = Mask >> 1;
				}
			}
		}
	}
	else
	{
		UINT* Buffer32 = reinterpret_cast<UINT*>(PtrInfo->PtrShapeBuffer);

		for (INT Row = 0; Row < *PtrHeight; ++Row)
		{
			for (INT Col = 0; Col < *PtrWidth; ++Col)
			{
				// 设置蒙版
				UINT MaskVal = 0xFF000000 & Buffer32[(Col + SkipX) + ((Row + SkipY) * (PtrInfo->ShapeInfo.Pitch / sizeof(UINT)))];
				if (MaskVal)
				{
					// Mask was 0xFF
					InitBuffer32[(Row * *PtrWidth) + Col] = (Desktop32[(Row * DesktopPitchInPixels) + Col] ^ Buffer32[(Col + SkipX) + ((Row + SkipY) * (PtrInfo->ShapeInfo.Pitch / sizeof(UINT)))]) | 0xFF000000;
				}
				else
				{
					// Mask was 0x00
					InitBuffer32[(Row * *PtrWidth) + Col] = Buffer32[(Col + SkipX) + ((Row + SkipY) * (PtrInfo->ShapeInfo.Pitch / sizeof(UINT)))] | 0xFF000000;
				}
			}
		}
	}

	// 清空
	hr = CopySurface->Unmap();
	CopySurface->Release();
	CopySurface = nullptr;

	return DUPL_RETURN_SUCCESS;
}

//
// 绘制鼠标
// 步骤概要 :
//		1. 声明 Texture2D(DESC)、ShaderResourceView(DESC)、Buffer 和 SubresourceData
//		2. 定义 Vertex[]
//		3. 获取坐标尺寸、定义 BYTE 缓冲区
//		4. Texture2D 描述、ShaderResourceView 描述
//		5. →ProcessMonoMask()
//		6. 创建 [0.f, 1.f] 坐标
//		7. 设置 SubResourceData
//		8. m_Device->CreateTexture() / CreateShaderResourceView()
//		9. m_Device->CreateBuffer(DESC)
//		10. m_DeviceContext->IA/OM/VS/PS
//		11. m_DeviceContext->Draw()
DUPL_RETURN OUTPUTMANAGER::DrawMouse(_In_ PTR_INFO* PtrInfo)
{
	// 相关变量
	ID3D11Texture2D* MouseTex = nullptr;
	ID3D11ShaderResourceView* ShaderRes = nullptr;
	ID3D11Buffer* VertexBufferMouse = nullptr;
	D3D11_SUBRESOURCE_DATA InitData;
	D3D11_TEXTURE2D_DESC Desc;
	D3D11_SHADER_RESOURCE_VIEW_DESC SDesc;

	// VERTEX
	VERTEX Vertices[NUMVERTICES] =
	{
		{ XMFLOAT3(-1.0f, -1.0f, 0), XMFLOAT2(0.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 0), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 0), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(1.0f, -1.0f, 0), XMFLOAT2(1.0f, 1.0f) },
		{ XMFLOAT3(-1.0f, 1.0f, 0), XMFLOAT2(0.0f, 0.0f) },
		{ XMFLOAT3(1.0f, 1.0f, 0), XMFLOAT2(1.0f, 0.0f) },
	};

	// 坐标
	D3D11_TEXTURE2D_DESC FullDesc;
	m_SharedSurf->GetDesc(&FullDesc);
	INT DesktopWidth = FullDesc.Width;
	INT DesktopHeight = FullDesc.Height;

	INT CenterX = (DesktopWidth / 2);
	INT CenterY = (DesktopHeight / 2);

	INT PtrWidth = 0;
	INT PtrHeight = 0;
	INT PtrLeft = 0;
	INT PtrTop = 0;

	// BYTE 缓冲区
	BYTE* InitBuffer = nullptr;

	// Texture2D 的描述
	D3D11_BOX Box;
	Box.front = 0;
	Box.back = 1;

	Desc.MipLevels = 1;
	Desc.ArraySize = 1;
	Desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	Desc.SampleDesc.Count = 1;
	Desc.SampleDesc.Quality = 0;
	Desc.Usage = D3D11_USAGE_DEFAULT;
	Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	Desc.CPUAccessFlags = 0;
	Desc.MiscFlags = 0;

	// ShaderResourceView 的描述
	SDesc.Format = Desc.Format;
	SDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	SDesc.Texture2D.MostDetailedMip = Desc.MipLevels - 1;
	SDesc.Texture2D.MipLevels = Desc.MipLevels;

	// 根据鼠标指针类型进行不同的处理
	switch (PtrInfo->ShapeInfo.Type)
	{
	case DXGI_OUTDUPL_POINTER_SHAPE_TYPE_COLOR:
	{
		PtrLeft = PtrInfo->Position.x;
		PtrTop = PtrInfo->Position.y;

		PtrWidth = static_cast<INT>(PtrInfo->ShapeInfo.Width);
		PtrHeight = static_cast<INT>(PtrInfo->ShapeInfo.Height);

		break;
	}

	case DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MONOCHROME:
	{
		ProcessMonoMask(true, PtrInfo, &PtrWidth, &PtrHeight, &PtrLeft, &PtrTop, &InitBuffer, &Box);
		break;
	}

	case DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MASKED_COLOR:
	{
		ProcessMonoMask(false, PtrInfo, &PtrWidth, &PtrHeight, &PtrLeft, &PtrTop, &InitBuffer, &Box);
		break;
	}

	default:
		break;
	}

	// 创建[0.f, 1.f]坐标
	Vertices[0].Pos.x = (PtrLeft - CenterX) / (FLOAT)CenterX;
	Vertices[0].Pos.y = -1 * ((PtrTop + PtrHeight) - CenterY) / (FLOAT)CenterY;
	Vertices[1].Pos.x = (PtrLeft - CenterX) / (FLOAT)CenterX;
	Vertices[1].Pos.y = -1 * (PtrTop - CenterY) / (FLOAT)CenterY;
	Vertices[2].Pos.x = ((PtrLeft + PtrWidth) - CenterX) / (FLOAT)CenterX;
	Vertices[2].Pos.y = -1 * ((PtrTop + PtrHeight) - CenterY) / (FLOAT)CenterY;
	Vertices[3].Pos.x = Vertices[2].Pos.x;
	Vertices[3].Pos.y = Vertices[2].Pos.y;
	Vertices[4].Pos.x = Vertices[1].Pos.x;
	Vertices[4].Pos.y = Vertices[1].Pos.y;
	Vertices[5].Pos.x = ((PtrLeft + PtrWidth) - CenterX) / (FLOAT)CenterX;
	Vertices[5].Pos.y = -1 * (PtrTop - CenterY) / (FLOAT)CenterY;

	Desc.Width = PtrWidth;
	Desc.Height = PtrHeight;

	// 设置 SubResourceData
	InitData.pSysMem = (PtrInfo->ShapeInfo.Type == DXGI_OUTDUPL_POINTER_SHAPE_TYPE_COLOR) ? PtrInfo->PtrShapeBuffer : InitBuffer;
	InitData.SysMemPitch = (PtrInfo->ShapeInfo.Type == DXGI_OUTDUPL_POINTER_SHAPE_TYPE_COLOR) ? PtrInfo->ShapeInfo.Pitch : PtrWidth * BPP;
	InitData.SysMemSlicePitch = 0;

	// 创建 Texture2D / ShaderResourceView
	HRESULT hr = m_Device->CreateTexture2D(&Desc, &InitData, &MouseTex);
	hr = m_Device->CreateShaderResourceView(MouseTex, &SDesc, &ShaderRes);

	// 创建 Buffer (DESC)
	D3D11_BUFFER_DESC BDesc;
	ZeroMemory(&BDesc, sizeof(D3D11_BUFFER_DESC));
	BDesc.Usage = D3D11_USAGE_DEFAULT;
	BDesc.ByteWidth = sizeof(VERTEX) * NUMVERTICES;
	BDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	BDesc.CPUAccessFlags = 0;

	ZeroMemory(&InitData, sizeof(D3D11_SUBRESOURCE_DATA));
	InitData.pSysMem = Vertices;

	hr = m_Device->CreateBuffer(&BDesc, &InitData, &VertexBufferMouse);

	// m_DeviceContext->IA / OM / VS / PS
	FLOAT BlendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
	UINT Stride = sizeof(VERTEX);
	UINT Offset = 0;
	m_DeviceContext->IASetVertexBuffers(0, 1, &VertexBufferMouse, &Stride, &Offset);
	m_DeviceContext->OMSetBlendState(m_BlendState, BlendFactor, 0xFFFFFFFF);
	m_DeviceContext->OMSetRenderTargets(1, &m_RTV, nullptr);
	m_DeviceContext->VSSetShader(m_VertexShader, nullptr, 0);
	m_DeviceContext->PSSetShader(m_PixelShader, nullptr, 0);
	m_DeviceContext->PSSetShaderResources(0, 1, &ShaderRes);
	m_DeviceContext->PSSetSamplers(0, 1, &m_SamplerLinear);

	// m_DeviceContext->Draw()
	m_DeviceContext->Draw(NUMVERTICES, 0);

	// 清空
	if (VertexBufferMouse)
	{
		VertexBufferMouse->Release();
		VertexBufferMouse = nullptr;
	}
	if (ShaderRes)
	{
		ShaderRes->Release();
		ShaderRes = nullptr;
	}
	if (MouseTex)
	{
		MouseTex->Release();
		MouseTex = nullptr;
	}
	if (InitBuffer)
	{
		delete[] InitBuffer;
		InitBuffer = nullptr;
	}

	return DUPL_RETURN_SUCCESS;
}

//
// 初始化着色器
// 步骤概要 : 
//		1. m_Device->CreateVertexShader(g_VS)
//		2. m_Device->CreateInputLayout(g_VS)
//		3. m_Device->CreatePixelShader(g_PS)
DUPL_RETURN OUTPUTMANAGER::InitShaders()
{
	HRESULT hr;

	// VS
	UINT Size = ARRAYSIZE(g_VS);
	hr = m_Device->CreateVertexShader(g_VS, Size, nullptr, &m_VertexShader);

	// Layout
	D3D11_INPUT_ELEMENT_DESC Layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	UINT NumElements = ARRAYSIZE(Layout);
	hr = m_Device->CreateInputLayout(Layout, NumElements, g_VS, Size, &m_InputLayout);
	m_DeviceContext->IASetInputLayout(m_InputLayout);

	// PS
	Size = ARRAYSIZE(g_PS);
	hr = m_Device->CreatePixelShader(g_PS, Size, nullptr, &m_PixelShader);

	return DUPL_RETURN_SUCCESS;
}

//
// 设置 m_RTV
// 步骤概要 : 
//		1. m_SwapChain->GetBuffer()
//		2. m_Device->CreateRenderTargetView()
//		3. m_Device->OMSetRenderTarget()
DUPL_RETURN OUTPUTMANAGER::MakeRTV()
{
	// 获取 backbuffer
	ID3D11Texture2D* BackBuffer = nullptr;
	HRESULT hr = m_SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&BackBuffer));

	// 创建RTV
	hr = m_Device->CreateRenderTargetView(BackBuffer, nullptr, &m_RTV);
	BackBuffer->Release();

	// OM
	m_DeviceContext->OMSetRenderTargets(1, &m_RTV, nullptr);

	return DUPL_RETURN_SUCCESS;
}

//
// 设置视口
//
void OUTPUTMANAGER::SetViewPort(UINT Width, UINT Height)
{
	D3D11_VIEWPORT VP;
	VP.Width = static_cast<FLOAT>(Width);
	VP.Height = static_cast<FLOAT>(Height);
	VP.MinDepth = 0.0f;
	VP.MaxDepth = 1.0f;
	VP.TopLeftX = 0;
	VP.TopLeftY = 0;
	m_DeviceContext->RSSetViewports(1, &VP);
}

//
// Resize swapchain
// 重新分配交换链swapchain的大小
//
DUPL_RETURN OUTPUTMANAGER::ResizeSwapChain()
{
	if (m_RTV)
	{
		m_RTV->Release();
		m_RTV = nullptr;
	}

	RECT WindowRect;
	GetClientRect(m_WindowHandle, &WindowRect);
	UINT Width = WindowRect.right - WindowRect.left;
	UINT Height = WindowRect.bottom - WindowRect.top;

	// Resize swapchain
	DXGI_SWAP_CHAIN_DESC SwapChainDesc;
	m_SwapChain->GetDesc(&SwapChainDesc);
	HRESULT hr = m_SwapChain->ResizeBuffers(SwapChainDesc.BufferCount, Width, Height, SwapChainDesc.BufferDesc.Format, SwapChainDesc.Flags);

	// Make new render target view
	DUPL_RETURN Ret = MakeRTV();
	if (Ret != DUPL_RETURN_SUCCESS)
	{
		return Ret;
	}

	// Set new viewport
	SetViewPort(Width, Height);

	return Ret;
}

//
// Releases all references
// 清空所有引用计数
//
void OUTPUTMANAGER::CleanRefs()
{
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

	if (m_RTV)
	{
		m_RTV->Release();
		m_RTV = nullptr;
	}

	if (m_SamplerLinear)
	{
		m_SamplerLinear->Release();
		m_SamplerLinear = nullptr;
	}

	if (m_BlendState)
	{
		m_BlendState->Release();
		m_BlendState = nullptr;
	}

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

	if (m_SwapChain)
	{
		m_SwapChain->Release();
		m_SwapChain = nullptr;
	}

	if (m_SharedSurf)
	{
		m_SharedSurf->Release();
		m_SharedSurf = nullptr;
	}

	if (m_KeyMutex)
	{
		m_KeyMutex->Release();
		m_KeyMutex = nullptr;
	}

	if (m_Factory)
	{
		if (m_OcclusionCookie)
		{
			m_Factory->UnregisterOcclusionStatus(m_OcclusionCookie);
			m_OcclusionCookie = 0;
		}
		m_Factory->Release();
		m_Factory = nullptr;
	}
}