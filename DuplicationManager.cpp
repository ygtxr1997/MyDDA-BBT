#include "DuplicationManager.h"


//
// 初始化列表构造函数
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
// 析构中尽量使用Release()释放COM组件
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
// 利用 Device 初始化 DXGI 组件
// 功能 : 初始化 m_Device, m_OutputDesc, m_DeskDupl, m_OutputNumber
// 步骤概要 :	m_Device ---QI--> IDXGIDevice ---GP--> IDXGIAdapter ---EO-->
//					IDXGIOutput ---QI--> IDXGIOutput1 ---::--> DuplicateOutput()
DUPL_RETURN DUPLICATIONMANAGER::InitDupl(_In_ ID3D11Device* Device, UINT Output)
{
	m_OutputNumber = Output;

	// 传入Device, 并增加m_Device的引用计数
	m_Device = Device;
	m_Device->AddRef();

	// 获取DXGI设备
	IDXGIDevice* DxgiDevice = nullptr;
	HRESULT hr = m_Device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&DxgiDevice));

	// 获取DXGI适配器
	IDXGIAdapter* DxgiAdapter = nullptr;
	hr = DxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&DxgiAdapter));
	DxgiDevice->Release();
	DxgiDevice = nullptr;

	// 获取DXGI输出
	IDXGIOutput* DxgiOutput = nullptr;
	hr = DxgiAdapter->EnumOutputs(Output, &DxgiOutput);
	DxgiAdapter->Release();
	DxgiAdapter = nullptr;

	// 将DXGI输出, 保存到m_OutputDesc
	DxgiOutput->GetDesc(&m_OutputDesc);

	// 获取DXGI输出1.1版本
	IDXGIOutput1* DxgiOutput1 = nullptr;
	hr = DxgiOutput->QueryInterface(__uuidof(DxgiOutput1), reinterpret_cast<void**>(&DxgiOutput1));
	DxgiOutput->Release();
	DxgiOutput = nullptr;

	// 创建桌面复制项 duplication, 保存到m_DeskDupl
	hr = DxgiOutput1->DuplicateOutput(m_Device, &m_DeskDupl);
	DxgiOutput1->Release();
	DxgiOutput1 = nullptr;

	return DUPL_RETURN_SUCCESS;
}

//
// 获取下一帧并写入 FRAME_DATA
// 功能 : 获取下一个桌面帧, 并写入 FRAME_DATA
// 步骤概要 :	1. m_DeskDupl->AcquireNextFrame(), 获得 *IDXGIResource 和 DXGI_OUTDUPL_FRAME_INFO
//					2. 超时则退出, 说明桌面资源被占用; 否则继续
//					3. 释放 m_AcquiredDesktopImage, 然后 IDXGIResource ---QI--> ID3D11Texture2D
//					4. 获取元数据 (移动矩形和脏矩形) 的首地址 m_MetaDataBuffer 和大小 m_MetaDataSize
//						使用 IDXGIOutputDuplication::GetFrameMoveRects()/GetFrameDirtyRects()
//					5. 把相关数据存入 FRAME_DATA
DUPL_RETURN DUPLICATIONMANAGER::GetFrame(_Out_ FRAME_DATA* Data, _Out_ bool* Timeout)
{
	IDXGIResource* DesktopResource = nullptr;
	DXGI_OUTDUPL_FRAME_INFO FrameInfo;			// DXGI帧信息

	// AcquireNextFrame()获取新帧
	HRESULT hr = m_DeskDupl->AcquireNextFrame(500, &FrameInfo, &DesktopResource);

	// 是否超时
	if (hr == DXGI_ERROR_WAIT_TIMEOUT)
	{
		*Timeout = true;
		return DUPL_RETURN_SUCCESS;
	}
	*Timeout = false;

	// 重置m_AcquiredDesktopImage
	if (m_AcquiredDesktopImage)
	{
		m_AcquiredDesktopImage->Release();
		m_AcquiredDesktopImage = nullptr;
	}

	// 查询DesktopResource的ID3D11Texture2D接口, 赋给m_AcquiredDekstopImage
	hr = DesktopResource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void **>(&m_AcquiredDesktopImage));
	DesktopResource->Release();
	DesktopResource = nullptr;

	// 获取元数据(移动矩形和脏矩形), 首地址和大小
	if (FrameInfo.TotalMetadataBufferSize)
	{
		// 是否需要重新分配空间
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

		// 通过dxgi获取移动矩形个数和元数据缓冲区首地址
		UINT BufSize = FrameInfo.TotalMetadataBufferSize;

		hr = m_DeskDupl->GetFrameMoveRects(BufSize, reinterpret_cast<DXGI_OUTDUPL_MOVE_RECT*>(m_MetaDataBuffer), &BufSize);
		Data->MoveCount = BufSize / sizeof(DXGI_OUTDUPL_MOVE_RECT);

		// 通过dxgi获取脏矩形
		BYTE* DirtyRects = m_MetaDataBuffer + BufSize;
		BufSize = FrameInfo.TotalMetadataBufferSize - BufSize;

		hr = m_DeskDupl->GetFrameDirtyRects(BufSize, reinterpret_cast<RECT*>(DirtyRects), &BufSize);
		Data->DirtyCount = BufSize / sizeof(RECT);

		// 传递首地址
		Data->MetaData = m_MetaDataBuffer;
	}

	Data->Frame = m_AcquiredDesktopImage;
	Data->FrameInfo = FrameInfo;

	return DUPL_RETURN_SUCCESS;
}

//
// 检索鼠标信息, 并存入 PTR_INFO
// 功能: 根据给定的 DXGI_OUTDUPL_FRAME_INFO 更新 PTR_INFO
// 步骤概要:
//		1- 根据 UpdatePosition 判断是否需要更新鼠标位置
//		2- 是否需要更新鼠标形状
//		3- 如果 PtrInfo 缓冲区过小, 需要重新开辟空间, 并更新BufferSize的值
//		4- 从 m_DeskDupl 获取形状信息, 从FrameInfo 到 PtrInfo (IDXGIOutputDuplication::GetFramePointShape())
DUPL_RETURN DUPLICATIONMANAGER::GetMouse(_Inout_ PTR_INFO* PtrInfo, _In_ DXGI_OUTDUPL_FRAME_INFO* FrameInfo, INT OffsetX, INT OffsetY)
{
	// 时间戳不为0才更新
	if (FrameInfo->LastMouseUpdateTime.QuadPart == 0)
		return DUPL_RETURN_SUCCESS;

	bool UpdatePosition = true;

	// 某些情况下不需要更新鼠标位置
	if (!FrameInfo->PointerPosition.Visible && (PtrInfo->WhoUpdatedPositionLast != m_OutputNumber))
		UpdatePosition = false;

	if (FrameInfo->PointerPosition.Visible && PtrInfo->Visible && (PtrInfo->WhoUpdatedPositionLast != m_OutputNumber) && (PtrInfo->LastTimeStamp.QuadPart > FrameInfo->LastMouseUpdateTime.QuadPart))
		UpdatePosition = false;

	//  根据UpdatePosition判断是否需要更新鼠标位置
	if (UpdatePosition)
	{
		PtrInfo->Position.x = FrameInfo->PointerPosition.Position.x + m_OutputDesc.DesktopCoordinates.left - OffsetX;
		PtrInfo->Position.y = FrameInfo->PointerPosition.Position.y + m_OutputDesc.DesktopCoordinates.top - OffsetY;
		PtrInfo->WhoUpdatedPositionLast = m_OutputNumber;
		PtrInfo->LastTimeStamp = FrameInfo->LastMouseUpdateTime;
		PtrInfo->Visible = FrameInfo->PointerPosition.Visible != 0;
	}

	// 是否需要更新鼠标形状
	if (FrameInfo->PointerShapeBufferSize == 0)
		return DUPL_RETURN_SUCCESS;

	// 如果PtrInfo缓冲区过小, 需要重新开辟空间, 并更新BufferSize的值
	if (FrameInfo->PointerShapeBufferSize > PtrInfo->BufferSize)
	{
		if (PtrInfo->PtrShapeBuffer)
		{
			delete[] PtrInfo->PtrShapeBuffer;
			PtrInfo->PtrShapeBuffer = nullptr;
		}
		PtrInfo->PtrShapeBuffer = new (std::nothrow) BYTE[FrameInfo->PointerShapeBufferSize];	// std::nothrow, 在new失败时返回NULL而不是抛出异常
		if (!PtrInfo->PtrShapeBuffer)
		{
			PtrInfo->BufferSize = 0;
			return DUPL_RETURN_ERROR_EXPECTED;
		}
		PtrInfo->BufferSize = FrameInfo->PointerShapeBufferSize;
	}

	// 从m_DeskDupl获取形状信息, 从FrameInfo 到 PtrInfo
	UINT BufferSizeRequired;
	HRESULT hr = m_DeskDupl->GetFramePointerShape(FrameInfo->PointerShapeBufferSize, reinterpret_cast<VOID**>(PtrInfo->PtrShapeBuffer), &BufferSizeRequired, &(PtrInfo->ShapeInfo));

	return DUPL_RETURN_SUCCESS;
}

//
// 释放帧
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
// 获取DXGI_OUTPUT_DESC
//
void DUPLICATIONMANAGER::GetOutputDesc(_Out_ DXGI_OUTPUT_DESC* DescPtr)
{
	*DescPtr = m_OutputDesc;
}