#include "DuplicationManager.h"


//
// ��ʼ���б��캯��
//
DUPLICATIONMANAGER::DUPLICATIONMANAGER() : m_DeskDupl(nullptr),
																		m_AcquiredDesktopImage(nullptr),
																		m_MetaDataBuffer(nullptr),
																		m_MetaDataSize(0),
																		m_OutputNumber(0),
																		m_Device(nullptr)
{
	RtlZeroMemory(&m_OutputDesc, sizeof(m_OutputDesc));
}


//
// �����о���ʹ��Release()�ͷ�COM���
//
DUPLICATIONMANAGER::~DUPLICATIONMANAGER()
{
	if (m_DeskDupl)
	{
		m_DeskDupl->Release();
		m_DeskDupl = nullptr;
	}

	if (m_AcquiredDesktopImage)
	{
		m_AcquiredDesktopImage->Release();
		m_AcquiredDesktopImage = nullptr;
	}

	if (m_MetaDataBuffer)
	{
		delete[] m_MetaDataBuffer;
		m_MetaDataBuffer = nullptr;
	}

	if (m_Device)
	{
		m_Device->Release();
		m_Device = nullptr;
	}
}

//
// ���� Device ��ʼ�� DXGI ���
// ���� : ��ʼ�� m_Device, m_OutputDesc, m_DeskDupl, m_OutputNumber
// �����Ҫ :	m_Device ---QI--> IDXGIDevice ---GP--> IDXGIAdapter ---EO-->
//					IDXGIOutput ---QI--> IDXGIOutput1 ---::--> DuplicateOutput()
DUPL_RETURN DUPLICATIONMANAGER::InitDupl(_In_ ID3D11Device* Device, UINT Output)
{
	m_OutputNumber = Output;

	// ����Device, ������m_Device�����ü���
	m_Device = Device;
	m_Device->AddRef();

	// ��ȡDXGI�豸
	IDXGIDevice* DxgiDevice = nullptr;
	HRESULT hr = m_Device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&DxgiDevice));

	// ��ȡDXGI������
	IDXGIAdapter* DxgiAdapter = nullptr;
	hr = DxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&DxgiAdapter));
	DxgiDevice->Release();
	DxgiDevice = nullptr;

	// ��ȡDXGI���
	IDXGIOutput* DxgiOutput = nullptr;
	hr = DxgiAdapter->EnumOutputs(Output, &DxgiOutput);
	DxgiAdapter->Release();
	DxgiAdapter = nullptr;

	// ��DXGI���, ���浽m_OutputDesc
	DxgiOutput->GetDesc(&m_OutputDesc);

	// ��ȡDXGI���1.1�汾
	IDXGIOutput1* DxgiOutput1 = nullptr;
	hr = DxgiOutput->QueryInterface(__uuidof(DxgiOutput1), reinterpret_cast<void**>(&DxgiOutput1));
	DxgiOutput->Release();
	DxgiOutput = nullptr;

	// �������渴���� duplication, ���浽m_DeskDupl
	hr = DxgiOutput1->DuplicateOutput(m_Device, &m_DeskDupl);
	DxgiOutput1->Release();
	DxgiOutput1 = nullptr;

	return DUPL_RETURN_SUCCESS;
}

//
// ��ȡ��һ֡��д�� FRAME_DATA
// ���� : ��ȡ��һ������֡, ��д�� FRAME_DATA
// �����Ҫ :	1. m_DeskDupl->AcquireNextFrame(), ��� *IDXGIResource �� DXGI_OUTDUPL_FRAME_INFO
//					2. ��ʱ���˳�, ˵��������Դ��ռ��; �������
//					3. �ͷ� m_AcquiredDesktopImage, Ȼ�� IDXGIResource ---QI--> ID3D11Texture2D
//					4. ��ȡԪ���� (�ƶ����κ������) ���׵�ַ m_MetaDataBuffer �ʹ�С m_MetaDataSize
//						ʹ�� IDXGIOutputDuplication::GetFrameMoveRects()/GetFrameDirtyRects()
//					5. ��������ݴ��� FRAME_DATA
DUPL_RETURN DUPLICATIONMANAGER::GetFrame(_Out_ FRAME_DATA* Data, _Out_ bool* Timeout)
{
	IDXGIResource* DesktopResource = nullptr;
	DXGI_OUTDUPL_FRAME_INFO FrameInfo;			// DXGI֡��Ϣ

	// AcquireNextFrame()��ȡ��֡
	HRESULT hr = m_DeskDupl->AcquireNextFrame(500, &FrameInfo, &DesktopResource);

	// �Ƿ�ʱ
	if (hr == DXGI_ERROR_WAIT_TIMEOUT)
	{
		*Timeout = true;
		return DUPL_RETURN_SUCCESS;
	}
	*Timeout = false;

	// ����m_AcquiredDesktopImage
	if (m_AcquiredDesktopImage)
	{
		m_AcquiredDesktopImage->Release();
		m_AcquiredDesktopImage = nullptr;
	}

	// ��ѯDesktopResource��ID3D11Texture2D�ӿ�, ����m_AcquiredDekstopImage
	hr = DesktopResource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void **>(&m_AcquiredDesktopImage));
	DesktopResource->Release();
	DesktopResource = nullptr;

	// ��ȡԪ����(�ƶ����κ������), �׵�ַ�ʹ�С
	if (FrameInfo.TotalMetadataBufferSize)
	{
		// �Ƿ���Ҫ���·���ռ�
		if (FrameInfo.TotalMetadataBufferSize > m_MetaDataSize)
		{
			if (m_MetaDataBuffer)
			{
				delete[] m_MetaDataBuffer;
				m_MetaDataBuffer = nullptr;
			}
			m_MetaDataBuffer = new (std::nothrow) BYTE[FrameInfo.TotalMetadataBufferSize];
			if (!m_MetaDataBuffer)
			{
				m_MetaDataSize = 0;
				Data->MoveCount = 0;
				Data->DirtyCount = 0;
				return DUPL_RETURN_ERROR_EXPECTED;
			}
			m_MetaDataSize = FrameInfo.TotalMetadataBufferSize;
		}

		// ͨ��dxgi��ȡ�ƶ����θ�����Ԫ���ݻ������׵�ַ
		UINT BufSize = FrameInfo.TotalMetadataBufferSize;

		hr = m_DeskDupl->GetFrameMoveRects(BufSize, reinterpret_cast<DXGI_OUTDUPL_MOVE_RECT*>(m_MetaDataBuffer), &BufSize);
		Data->MoveCount = BufSize / sizeof(DXGI_OUTDUPL_MOVE_RECT);

		// ͨ��dxgi��ȡ�����
		BYTE* DirtyRects = m_MetaDataBuffer + BufSize;
		BufSize = FrameInfo.TotalMetadataBufferSize - BufSize;

		hr = m_DeskDupl->GetFrameDirtyRects(BufSize, reinterpret_cast<RECT*>(DirtyRects), &BufSize);
		Data->DirtyCount = BufSize / sizeof(RECT);

		// �����׵�ַ
		Data->MetaData = m_MetaDataBuffer;
	}

	Data->Frame = m_AcquiredDesktopImage;
	Data->FrameInfo = FrameInfo;

	return DUPL_RETURN_SUCCESS;
}

//
// ���������Ϣ, ������ PTR_INFO
// ����: ���ݸ����� DXGI_OUTDUPL_FRAME_INFO ���� PTR_INFO
// �����Ҫ:
//		1- ���� UpdatePosition �ж��Ƿ���Ҫ�������λ��
//		2- �Ƿ���Ҫ���������״
//		3- ��� PtrInfo ��������С, ��Ҫ���¿��ٿռ�, ������BufferSize��ֵ
//		4- �� m_DeskDupl ��ȡ��״��Ϣ, ��FrameInfo �� PtrInfo (IDXGIOutputDuplication::GetFramePointShape())
DUPL_RETURN DUPLICATIONMANAGER::GetMouse(_Inout_ PTR_INFO* PtrInfo, _In_ DXGI_OUTDUPL_FRAME_INFO* FrameInfo, INT OffsetX, INT OffsetY)
{
	// ʱ�����Ϊ0�Ÿ���
	if (FrameInfo->LastMouseUpdateTime.QuadPart == 0)
		return DUPL_RETURN_SUCCESS;

	bool UpdatePosition = true;

	// ĳЩ����²���Ҫ�������λ��
	if (!FrameInfo->PointerPosition.Visible && (PtrInfo->WhoUpdatedPositionLast != m_OutputNumber))
		UpdatePosition = false;

	if (FrameInfo->PointerPosition.Visible && PtrInfo->Visible && (PtrInfo->WhoUpdatedPositionLast != m_OutputNumber) && (PtrInfo->LastTimeStamp.QuadPart > FrameInfo->LastMouseUpdateTime.QuadPart))
		UpdatePosition = false;

	//  ����UpdatePosition�ж��Ƿ���Ҫ�������λ��
	if (UpdatePosition)
	{
		PtrInfo->Position.x = FrameInfo->PointerPosition.Position.x + m_OutputDesc.DesktopCoordinates.left - OffsetX;
		PtrInfo->Position.y = FrameInfo->PointerPosition.Position.y + m_OutputDesc.DesktopCoordinates.top - OffsetY;
		PtrInfo->WhoUpdatedPositionLast = m_OutputNumber;
		PtrInfo->LastTimeStamp = FrameInfo->LastMouseUpdateTime;
		PtrInfo->Visible = FrameInfo->PointerPosition.Visible != 0;
	}

	// �Ƿ���Ҫ���������״
	if (FrameInfo->PointerShapeBufferSize == 0)
		return DUPL_RETURN_SUCCESS;

	// ���PtrInfo��������С, ��Ҫ���¿��ٿռ�, ������BufferSize��ֵ
	if (FrameInfo->PointerShapeBufferSize > PtrInfo->BufferSize)
	{
		if (PtrInfo->PtrShapeBuffer)
		{
			delete[] PtrInfo->PtrShapeBuffer;
			PtrInfo->PtrShapeBuffer = nullptr;
		}
		PtrInfo->PtrShapeBuffer = new (std::nothrow) BYTE[FrameInfo->PointerShapeBufferSize];	// std::nothrow, ��newʧ��ʱ����NULL�������׳��쳣
		if (!PtrInfo->PtrShapeBuffer)
		{
			PtrInfo->BufferSize = 0;
			return DUPL_RETURN_ERROR_EXPECTED;
		}
		PtrInfo->BufferSize = FrameInfo->PointerShapeBufferSize;
	}

	// ��m_DeskDupl��ȡ��״��Ϣ, ��FrameInfo �� PtrInfo
	UINT BufferSizeRequired;
	HRESULT hr = m_DeskDupl->GetFramePointerShape(FrameInfo->PointerShapeBufferSize, reinterpret_cast<VOID**>(PtrInfo->PtrShapeBuffer), &BufferSizeRequired, &(PtrInfo->ShapeInfo));

	return DUPL_RETURN_SUCCESS;
}

//
// �ͷ�֡
//
DUPL_RETURN DUPLICATIONMANAGER::DoneWithFrame()
{
	HRESULT hr = m_DeskDupl->ReleaseFrame();
	if (m_AcquiredDesktopImage)
	{
		m_AcquiredDesktopImage->Release();
		m_AcquiredDesktopImage = nullptr;
	}

	return DUPL_RETURN_SUCCESS;
}

//
// ��ȡDXGI_OUTPUT_DESC
//
void DUPLICATIONMANAGER::GetOutputDesc(_Out_ DXGI_OUTPUT_DESC* DescPtr)
{
	*DescPtr = m_OutputDesc;
}