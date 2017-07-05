#include "ThreadManager.h"

//
// 提前声明的函数
//
DWORD WINAPI DDProc(_In_ void* Param);	

//
// 初始化列表构造
//
THREADMANAGER::THREADMANAGER() : m_ThreadCount(0),
m_ThreadHandles(nullptr),
m_ThreadData(nullptr)
{
	RtlZeroMemory(&m_PtrInfo, sizeof(m_PtrInfo));
}

//
// 清空资源以析构
//
THREADMANAGER::~THREADMANAGER()
{
	Clean();
}

//
// 清理资源(线程和内存)
//
void THREADMANAGER::Clean()
{
	if (m_PtrInfo.PtrShapeBuffer)
	{
		delete[] m_PtrInfo.PtrShapeBuffer;
		m_PtrInfo.PtrShapeBuffer = nullptr;
	}
	RtlZeroMemory(&m_PtrInfo, sizeof(m_PtrInfo));	// ** 清空m_PtrInfo, 鼠标指针信息

	if (m_ThreadHandles)
	{
		for (UINT i = 0; i < m_ThreadCount; ++i)
		{
			if (m_ThreadHandles[i])
			{
				CloseHandle(m_ThreadHandles[i]);			// 关闭单个线程句柄
			}
		}
		delete[] m_ThreadHandles;
		m_ThreadHandles = nullptr;
	}																			// ** 清空m_ThreadHandles, 线程句柄

	if (m_ThreadData)
	{
		for (UINT i = 0; i < m_ThreadCount; ++i)
		{
			CleanDx(&m_ThreadData[i].DxRes);			// ThreadManager::CleanDx()清理单个线程的D3D资源，CleanDx()在下面定义
		}
		delete[] m_ThreadData;
		m_ThreadData = nullptr;
	}																			// ** 清空m_ThreadData, 线程数据

	m_ThreadCount = 0;											// ** 线程数置0
}

//
// 清理单个线程的所有资源
//
void THREADMANAGER::CleanDx(_Inout_ DX_RESOURCES* Data)
{
	if (Data->Device)
	{
		Data->Device->Release();
		Data->Device = nullptr;
	}

	if (Data->Context)
	{
		Data->Context->Release();
		Data->Context = nullptr;
	}

	if (Data->VertexShader)
	{
		Data->VertexShader->Release();
		Data->VertexShader = nullptr;
	}

	if (Data->PixelShader)
	{
		Data->PixelShader->Release();
		Data->PixelShader = nullptr;
	}

	if (Data->InputLayout)
	{
		Data->InputLayout->Release();
		Data->InputLayout = nullptr;
	}

	if (Data->SamplerLinear)
	{
		Data->SamplerLinear->Release();
		Data->SamplerLinear = nullptr;
	}
}

//
// 初始化线程
// 输入 :	INT				SingleOutput,					---- 单输出？
//				UINT			OutputCount,					---- 输出数, 线程数
//				HANDLE		UnexpextedErrorEvent,	---- 非预期异常
//				HANDLE		ExpetcedErrorEvent,		---- 预期异常
//				HANDLE		TerminateThreadsEvent,	---- 线程终止
//				HANDLE		SharedHandle,					---- 共享句柄, 用来设置 m_ThreadData[].TexSharedHandle
//				_In_ RECT*	DesktopDim					---- 桌面矩形(left, top, width, height)
DUPL_RETURN THREADMANAGER::Initialize(INT SingleOutput, UINT OutputCount, HANDLE UnexpectedErrorEvent, HANDLE ExpectedErrorEvent, HANDLE TerminateThreadsEvent, HANDLE SharedHandle, _In_ RECT* DesktopDim)
{
	// m_ThreadCount / Handles / Data 声明与初始化
	m_ThreadCount = OutputCount;
	m_ThreadHandles = new (std::nothrow) HANDLE[m_ThreadCount];			// std::nothrow在new内存不足时不抛出异常而是将指针值NULL
	m_ThreadData = new (std::nothrow) THREAD_DATA[m_ThreadCount];		// ** 初始化内存

	// 循环
	DUPL_RETURN Ret = DUPL_RETURN_SUCCESS;
	for (UINT i = 0; i < m_ThreadCount; ++i)
	{
		// m_ThreadData[]数组
		m_ThreadData[i].Output = (SingleOutput < 0) ? i : SingleOutput;
		m_ThreadData[i].TerminateThreadsEvent = TerminateThreadsEvent;
		m_ThreadData[i].TexSharedHandle = SharedHandle;
		m_ThreadData[i].OffsetX = DesktopDim->left;
		m_ThreadData[i].OffestY = DesktopDim->top;
		m_ThreadData[i].PtrInfo = &m_PtrInfo;

		// 初始化 D3D 资源
		RtlZeroMemory(&m_ThreadData[i].DxRes, sizeof(DX_RESOURCES));
		Ret = InitializeDx(&m_ThreadData[i].DxRes);		// ThreadManager::InitializeDx(), 初始化D3D资源

		// m_ThreadHandles[]数组
		DWORD ThreadId;
		m_ThreadHandles[i] = CreateThread(nullptr, 0, DDProc, &m_ThreadData[i], 0, &ThreadId);	// 创建线程
	}

	return Ret;
}

//
// 初始化 D3D 资源
// --- 功能 ：	创建设备、顶点着色器、输入布局、像素着色器、采样器
DUPL_RETURN THREADMANAGER::InitializeDx(_Out_ DX_RESOURCES* Data)
{
	HRESULT hr = S_OK;

	// 支持的驱动类型
	D3D_DRIVER_TYPE DriverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT NumDriverTypes = ARRAYSIZE(DriverTypes);

	// D3D的兼容版本
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
			D3D11_SDK_VERSION, &Data->Device, &FeatureLevel, &Data->Context);		// 创建设备

		if (SUCCEEDED(hr))
		{
			// 设备已经匹配了驱动类型和D3D版本，无需继续循环
			break;
		}
	}

	// VS
	UINT Size = ARRAYSIZE(g_VS);
	hr = Data->Device->CreateVertexShader(g_VS, Size, nullptr, &Data->VertexShader);

	// Layout
	D3D11_INPUT_ELEMENT_DESC Layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	UINT NumElements = ARRAYSIZE(Layout);
	hr = Data->Device->CreateInputLayout(Layout, NumElements, g_VS, Size, &Data->InputLayout);

	Data->Context->IASetInputLayout(Data->InputLayout);

	// PS
	Size = ARRAYSIZE(g_PS);
	hr = Data->Device->CreatePixelShader(g_PS, Size, nullptr, &Data->PixelShader);

	// Sampler
	D3D11_SAMPLER_DESC SampDesc;
	RtlZeroMemory(&SampDesc, sizeof(SampDesc));
	SampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	SampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	SampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	SampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	SampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	SampDesc.MinLOD = 0;
	SampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = Data->Device->CreateSamplerState(&SampDesc, &Data->SamplerLinear);

	return DUPL_RETURN_SUCCESS;
}

//
// 获取鼠标指针信息
//
PTR_INFO* THREADMANAGER::GetPointerInfo()
{
	return &m_PtrInfo;
}

//
// 等所有线程终止
//
void THREADMANAGER::WaitForThreadTermination()
{
	if (m_ThreadCount != 0)
	{
		WaitForMultipleObjectsEx(m_ThreadCount, m_ThreadHandles, TRUE, INFINITE, FALSE);
	}
}


