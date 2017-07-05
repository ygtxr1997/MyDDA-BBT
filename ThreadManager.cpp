#include "ThreadManager.h"

//
// ��ǰ�����ĺ���
//
DWORD WINAPI DDProc(_In_ void* Param);	

//
// ��ʼ���б���
//
THREADMANAGER::THREADMANAGER() : m_ThreadCount(0),
m_ThreadHandles(nullptr),
m_ThreadData(nullptr)
{
	RtlZeroMemory(&m_PtrInfo, sizeof(m_PtrInfo));
}

//
// �����Դ������
//
THREADMANAGER::~THREADMANAGER()
{
	Clean();
}

//
// ������Դ(�̺߳��ڴ�)
//
void THREADMANAGER::Clean()
{
	if (m_PtrInfo.PtrShapeBuffer)
	{
		delete[] m_PtrInfo.PtrShapeBuffer;
		m_PtrInfo.PtrShapeBuffer = nullptr;
	}
	RtlZeroMemory(&m_PtrInfo, sizeof(m_PtrInfo));	// ** ���m_PtrInfo, ���ָ����Ϣ

	if (m_ThreadHandles)
	{
		for (UINT i = 0; i < m_ThreadCount; ++i)
		{
			if (m_ThreadHandles[i])
			{
				CloseHandle(m_ThreadHandles[i]);			// �رյ����߳̾��
			}
		}
		delete[] m_ThreadHandles;
		m_ThreadHandles = nullptr;
	}																			// ** ���m_ThreadHandles, �߳̾��

	if (m_ThreadData)
	{
		for (UINT i = 0; i < m_ThreadCount; ++i)
		{
			CleanDx(&m_ThreadData[i].DxRes);			// ThreadManager::CleanDx()�������̵߳�D3D��Դ��CleanDx()�����涨��
		}
		delete[] m_ThreadData;
		m_ThreadData = nullptr;
	}																			// ** ���m_ThreadData, �߳�����

	m_ThreadCount = 0;											// ** �߳�����0
}

//
// �������̵߳�������Դ
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
// ��ʼ���߳�
// ���� :	INT				SingleOutput,					---- �������
//				UINT			OutputCount,					---- �����, �߳���
//				HANDLE		UnexpextedErrorEvent,	---- ��Ԥ���쳣
//				HANDLE		ExpetcedErrorEvent,		---- Ԥ���쳣
//				HANDLE		TerminateThreadsEvent,	---- �߳���ֹ
//				HANDLE		SharedHandle,					---- ������, �������� m_ThreadData[].TexSharedHandle
//				_In_ RECT*	DesktopDim					---- �������(left, top, width, height)
DUPL_RETURN THREADMANAGER::Initialize(INT SingleOutput, UINT OutputCount, HANDLE UnexpectedErrorEvent, HANDLE ExpectedErrorEvent, HANDLE TerminateThreadsEvent, HANDLE SharedHandle, _In_ RECT* DesktopDim)
{
	// m_ThreadCount / Handles / Data �������ʼ��
	m_ThreadCount = OutputCount;
	m_ThreadHandles = new (std::nothrow) HANDLE[m_ThreadCount];			// std::nothrow��new�ڴ治��ʱ���׳��쳣���ǽ�ָ��ֵNULL
	m_ThreadData = new (std::nothrow) THREAD_DATA[m_ThreadCount];		// ** ��ʼ���ڴ�

	// ѭ��
	DUPL_RETURN Ret = DUPL_RETURN_SUCCESS;
	for (UINT i = 0; i < m_ThreadCount; ++i)
	{
		// m_ThreadData[]����
		m_ThreadData[i].Output = (SingleOutput < 0) ? i : SingleOutput;
		m_ThreadData[i].TerminateThreadsEvent = TerminateThreadsEvent;
		m_ThreadData[i].TexSharedHandle = SharedHandle;
		m_ThreadData[i].OffsetX = DesktopDim->left;
		m_ThreadData[i].OffestY = DesktopDim->top;
		m_ThreadData[i].PtrInfo = &m_PtrInfo;

		// ��ʼ�� D3D ��Դ
		RtlZeroMemory(&m_ThreadData[i].DxRes, sizeof(DX_RESOURCES));
		Ret = InitializeDx(&m_ThreadData[i].DxRes);		// ThreadManager::InitializeDx(), ��ʼ��D3D��Դ

		// m_ThreadHandles[]����
		DWORD ThreadId;
		m_ThreadHandles[i] = CreateThread(nullptr, 0, DDProc, &m_ThreadData[i], 0, &ThreadId);	// �����߳�
	}

	return Ret;
}

//
// ��ʼ�� D3D ��Դ
// --- ���� ��	�����豸��������ɫ�������벼�֡�������ɫ����������
DUPL_RETURN THREADMANAGER::InitializeDx(_Out_ DX_RESOURCES* Data)
{
	HRESULT hr = S_OK;

	// ֧�ֵ���������
	D3D_DRIVER_TYPE DriverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT NumDriverTypes = ARRAYSIZE(DriverTypes);

	// D3D�ļ��ݰ汾
	D3D_FEATURE_LEVEL FeatureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_1
	};
	UINT NumFeatureLevels = ARRAYSIZE(FeatureLevels);

	D3D_FEATURE_LEVEL FeatureLevel;

	// �����豸
	for (UINT DriverTypeIndex = 0; DriverTypeIndex < NumDriverTypes; ++DriverTypeIndex)
	{
		hr = D3D11CreateDevice(nullptr, DriverTypes[DriverTypeIndex], nullptr, 0, FeatureLevels, NumFeatureLevels,
			D3D11_SDK_VERSION, &Data->Device, &FeatureLevel, &Data->Context);		// �����豸

		if (SUCCEEDED(hr))
		{
			// �豸�Ѿ�ƥ�����������ͺ�D3D�汾���������ѭ��
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
// ��ȡ���ָ����Ϣ
//
PTR_INFO* THREADMANAGER::GetPointerInfo()
{
	return &m_PtrInfo;
}

//
// �������߳���ֹ
//
void THREADMANAGER::WaitForThreadTermination()
{
	if (m_ThreadCount != 0)
	{
		WaitForMultipleObjectsEx(m_ThreadCount, m_ThreadHandles, TRUE, INFINITE, FALSE);
	}
}


