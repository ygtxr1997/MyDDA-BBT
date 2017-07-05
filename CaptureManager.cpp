#include "CaptureManager.h"

#include <string>

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////// �ȴ����� ���� //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//
// �ȴ�����
//
typedef struct
{
	UINT WaitTime;
	UINT WaitCount;
} WAIT_BAND;

#define WAIT_BAND_COUNT	3
#define WAIT_BAND_STOP		0

class DYNAMIC_WAIT
{
public:
	DYNAMIC_WAIT();
	~DYNAMIC_WAIT();
	void Wait();

private:
	static const WAIT_BAND m_WaitBands[WAIT_BAND_COUNT];

	static const UINT m_WaitSequenceTimeInSeconds = 2;

	UINT m_CurrentWaitBandIdx;
	UINT m_WaitCountInCurrentBand;
	LARGE_INTEGER m_QPCFrequency;
	LARGE_INTEGER m_LastWakeUpTime;
	BOOL m_QPCValid;
};
const WAIT_BAND DYNAMIC_WAIT::m_WaitBands[WAIT_BAND_COUNT] = {
		{ 250, 20 },
		{ 2000, 200 },
		{ 5000, WAIT_BAND_STOP }
};

DYNAMIC_WAIT::DYNAMIC_WAIT() : m_CurrentWaitBandIdx(0), m_WaitCountInCurrentBand(0)
{
	m_QPCValid = QueryPerformanceFrequency(&m_QPCFrequency);	// ��ȡ��ȷƵ��
	m_LastWakeUpTime.QuadPart = 0L; // ����
}

DYNAMIC_WAIT::~DYNAMIC_WAIT()
{
}

void DYNAMIC_WAIT::Wait()
{
	LARGE_INTEGER CurrentQPC = { 0 };
	QueryPerformanceFrequency(&CurrentQPC);	// ��ȡ��׼ʱ��

	if (m_QPCValid && (CurrentQPC.QuadPart <= (m_LastWakeUpTime.QuadPart + (m_QPCFrequency.QuadPart * m_WaitSequenceTimeInSeconds)))) // δ��ʱ
	{
		if ((m_WaitBands[m_CurrentWaitBandIdx].WaitCount != WAIT_BAND_STOP) && (m_WaitCountInCurrentBand > m_WaitBands[m_CurrentWaitBandIdx].WaitCount))
		{
			m_CurrentWaitBandIdx++;
			m_WaitCountInCurrentBand = 0;
		}
	}
	else	// ��ʱ, ������
	{
		m_WaitCountInCurrentBand = 0;
		m_CurrentWaitBandIdx = 0;
	}

	// Sleep��Ӧ��ʱ��
	Sleep(m_WaitBands[m_CurrentWaitBandIdx].WaitTime);

	// ��¼����ʱ��
	QueryPerformanceCounter(&m_LastWakeUpTime);
	m_WaitCountInCurrentBand++;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//
// ��ǰ������
//
LRESULT CALLBACK	 WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

//
// ȫ�ֱ���
//
// static OUTPUTMANAGER* OutMgr = nullptr;

//
// ��ʼ���б���
//
CAPTUREMANAGER::CAPTUREMANAGER() : m_CaptureMode(CAPTURE_MODE_STOP),
										m_CaptureSetting(nullptr),
										m_Callback(new ICALLBACK),
										m_OutMgr()
{
	// OutMgr = &m_OutMgr;
}

//
// ����
//
CAPTUREMANAGER::~CAPTUREMANAGER()
{
}

//
// ���ûص�
//
CA_RETURN CAPTUREMANAGER::SetCallback(ICALLBACK** pCallback)
{
	pCallback = &m_Callback;
	return CA_RETURN_SUCCESS;
}

//
// ���� �����������
// ����0
CA_RETURN	 CAPTUREMANAGER::SetCaptureSetting(UINT64 uOffsetTimeStamp, UINT FPS, bool IsDisplay)
{
	if (uOffsetTimeStamp)
	{
		m_CaptureSetting->OffsetTimeStamp = uOffsetTimeStamp;
	}

	if (FPS && FPS <= MAX_FPS)
	{
		m_CaptureSetting->FPS = FPS;
	}

	m_CaptureSetting->IsDisplay = IsDisplay;
	
	return CA_RETURN_SUCCESS;
}

//
// ���� �����������
// ����1
CA_RETURN		CAPTUREMANAGER::SetCaptureSetting(HWND WinHandle, RECT TargetRect)
{
	if (IsWindow(WinHandle))
	{
		m_CaptureSetting->WinHandle = WinHandle;
	}
	else
	{
		OutputDebugString(L"���ڲ�����\n");
		return CA_RETURN_ERROR_EXPECTED;
	}

	if (TargetRect.top >= 0 && TargetRect.left >= 0 && TargetRect.bottom <= GetSystemMetrics(SM_CYSCREEN) && TargetRect.right <= GetSystemMetrics(SM_CXSCREEN))
	{
		m_CaptureSetting->TargetRect = TargetRect;
	}
	else
	{
		OutputDebugString(L"����λ�����ò��Ϸ�\n");
		return CA_RETURN_ERROR_EXPECTED;
	}

	return CA_RETURN_SUCCESS;
}

//
// ���� �����������
// ����2
CA_RETURN		CAPTUREMANAGER::SetCaptureSetting(POINT Anchor, bool IsFollowCursor)
{
	m_CaptureSetting->Anchor = Anchor;
	m_CaptureSetting->IsFollowCursor = IsFollowCursor;

	return CA_RETURN_SUCCESS;
}

//
// ȫ������
CA_RETURN CAPTUREMANAGER::StartCaptureFullScreen()
{
	// ѡ����ʵ�ģʽ
	m_CaptureMode = CAPTURE_MODE_UNKNOWN_FULLSCREEN;
	ChooseWay();

	// ����ģʽѡ����Ӧ����
	if (m_CaptureMode == CAPTURE_MODE_DDA_FULLSCREEN)
	{
		HINSTANCE test = GetModuleHandle(0);
		DDACapture(GetModuleHandle(0), NULL, nullptr, SW_SHOWDEFAULT);
	}
	else if (m_CaptureMode == CAPTURE_MODE_BBT_FULLSCREEN)
	{
		BBTCapture();
	}
	else
	{
		OutputDebugString(L"ģʽѡ���쳣");
		return CA_RETURN_ERROR_UNEXPECTED;
	}

	return CA_RETURN_SUCCESS;
}

//
// ���ڲ���
CA_RETURN CAPTUREMANAGER::StartCaptureWindow()
{
	// ѡ����ʵ�ģʽ
	m_CaptureMode = CAPTURE_MODE_UNKNOWN_WIN;
	ChooseWay();

	// ����ģʽѡ����Ӧ����
	switch (m_CaptureMode)
	{
		case CAPTURE_MODE_DDA_WINHANDLE:
		{
			
			break;
		}
		case CAPTURE_MODE_DDA_WINRECT:
		{

			break;
		}
		case CAPTURE_MODE_BBT_WINHANDLE:
		{

			break;
		}
		case CAPTURE_MODE_BBT_WINRECT:
		{

			break;
		}
		default : 
		{
			OutputDebugString(L"ģʽѡ���쳣");
			return CA_RETURN_ERROR_UNEXPECTED;
		}
	}

	return CA_RETURN_SUCCESS;
}


///////////////////////////////////////////////////////////////////////////////
//////////////////////////// private ���� /////////////////////////////////////

//
// ѡ����ʵķ�ʽ���в���
// 
CA_RETURN CAPTUREMANAGER::ChooseWay()
{
	if (!m_CaptureMode)
	{
		return CA_RETURN_ERROR_EXPECTED;
	}

	// ȫ��
	if (m_CaptureMode == CAPTURE_MODE_UNKNOWN_FULLSCREEN)
	{
		if (IsWindows8OrGreater())
		{
			m_CaptureMode = CAPTURE_MODE_DDA_FULLSCREEN;
		}
		else
		{
			m_CaptureMode = CAPTURE_MODE_BBT_FULLSCREEN;
		}
	}

	// ����
	if (m_CaptureMode == CAPTURE_MODE_UNKNOWN_WIN)
	{
		// Ĭ��ʹ�þ����ȡ��ͼ, ���ʧ������ת��������ʽ
		m_CaptureMode = CAPTURE_MODE_BBT_WINHANDLE;
	}

	return CA_RETURN_SUCCESS;
}

//
// DDA ���񷽰�
CA_RETURN CAPTUREMANAGER::DDACapture(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ INT nCmdShow)
{
	DDAProc(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
	return CA_RETURN_SUCCESS;
}

//
// BBT ���񷽰�
CA_RETURN CAPTUREMANAGER::BBTCapture()
{
	return CA_RETURN_SUCCESS;
}

//
// DDA ����
DWORD WINAPI CAPTUREMANAGER::DDAProc(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ INT nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);	// UNREFERENCED_PARAMETER()�궨��, ������������������ں������г���
	UNREFERENCED_PARAMETER(lpCmdLine);

	// ------------------- �жϰ汾��
	BOOL useD3D = IsWindows8OrGreater();
	if (!useD3D) {
		Sleep(2000);
		return -1;
	}

	INT SingleOutput = -1;

	// ��ֹ�߳��¼�
	HANDLE TerminateThreadsEvent = nullptr;
	TerminateThreadsEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

	// ���ھ��
	HWND WindowHandle = nullptr;

	// Ϊ���ڼ������
	HCURSOR Cursor = nullptr;
	Cursor = LoadCursor(nullptr, IDC_ARROW);

	// ע�ᴰ������
	WNDCLASSEXW Wc;
	Wc.cbSize = sizeof(WNDCLASSEXW);
	Wc.style = CS_HREDRAW | CS_VREDRAW;		// ˮƽ����ֱ�仯��Ҫ�ػ洰��
	Wc.lpfnWndProc = WndProc;			// ���ڽ���
	Wc.cbClsExtra = 0;
	Wc.cbWndExtra = 0;
	Wc.hInstance = hInstance;
	Wc.hIcon = nullptr;
	Wc.hCursor = Cursor;
	Wc.hbrBackground = nullptr;
	Wc.lpszMenuName = nullptr;
	Wc.lpszClassName = L"ddasample";
	Wc.hIconSm = nullptr;
	RegisterClassExW(&Wc);

	// ��������
	RECT WindowRect = { 0, 0, 800, 600 };
	AdjustWindowRect(&WindowRect, WS_OVERLAPPEDWINDOW, FALSE);
	WindowHandle = CreateWindowW(L"ddasample", L"Try Desktop Duplication", WS_OVERLAPPEDWINDOW, 0, 0, WindowRect.right - WindowRect.left, WindowRect.bottom - WindowRect.top, nullptr, nullptr, hInstance, nullptr);

	DestroyCursor(Cursor);

	ShowWindow(WindowHandle, nCmdShow);
	UpdateWindow(WindowHandle);
	// ADD: ���ش���
	ShowWindow(WindowHandle, SW_HIDE);

	THREADMANAGER ThreadMgr;	////////////////////////////////////////
	RECT DeskBounds;
	UINT OutputCount;

	// ��Ϣѭ��
	MSG msg = { 0 };
	bool FirstTime = true;
	bool Occluded = true;
	DYNAMIC_WAIT DynamicWait;

	while (WM_QUIT != msg.message)
	{
		DUPL_RETURN Ret = DUPL_RETURN_SUCCESS;
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == OCCLUSION_STATUS_MSG)	// δ������ͻ
				Occluded = false;
			else {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else if (FirstTime)		// ��һ�λ��߳����쳣
		{
			if (!FirstTime)
			{
			}
			else
				FirstTime = false;

			// ���³�ʼ��OutMgr
			Ret = m_OutMgr.InitOutput(WindowHandle, SingleOutput, &OutputCount, &DeskBounds);
			if (Ret == DUPL_RETURN_SUCCESS)
			{
				HANDLE SharedHandle = m_OutMgr.GetSharedHandle();
				if (SharedHandle)
				{
					Ret = ThreadMgr.Initialize(SingleOutput, OutputCount, nullptr, nullptr, TerminateThreadsEvent, SharedHandle, &DeskBounds);
				}
				else
				{
					Ret = DUPL_RETURN_ERROR_UNEXPECTED;
				}
			}

			Occluded = true;
		}
		else
		{
			if (!Occluded)		// ������OutMgr���´�����
			{
				Ret = m_OutMgr.UpdateApplicationWindow(ThreadMgr.GetPointerInfo(), &Occluded, &m_Callback->m_FrameData->pData, &m_Callback->m_FrameData->uSize, &m_Callback->m_FrameData->BytesPerLine);	//ADD: bitmapAlloc, bitmapSize
			}
		}
	} // һ����Ϣѭ������

	// ȷ���߳�ȫ���˳�
	if (SetEvent(TerminateThreadsEvent))
	{
		ThreadMgr.WaitForThreadTermination();
	}

	// ���
	CloseHandle(TerminateThreadsEvent);

	// �˳�
	if (msg.message == WM_QUIT)
	{
		return static_cast<INT>(msg.wParam);
	}

	return 0;
}

//
// ������Ϣ������
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		break;
	}
	case WM_SIZE:
	{
		// �ߴ�ı�, ֪ͨOutMgr�ػ洰��
		// OutMgr->WindowResize();
		break;
	}
	default:
		return DefWindowProc(hWnd, message, wParam, lParam); // �������򲻹��ĵ���Ϣ
	}
	return 0;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////// ICallback ���� //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//
// ��ʼ���б���
//
ICALLBACK::ICALLBACK() : m_FrameData(new CAPTURE_FRAMEDATA),
m_TimeStamp(0),
m_MousePoint(nullptr)
{
}

//
// ����
//
ICALLBACK::~ICALLBACK()
{
}

///////
//////
////

DWORD WINAPI DDProc(_In_ void* Param)
{
	// ------------------- �жϰ汾��
	BOOL useD3D = IsWindows8OrGreater();
	if (!useD3D)
		return -1;

	// �Զ�����
	DISPLAYMANAGER DispMgr;
	DUPLICATIONMANAGER DuplMgr;

	// D3D����(�����߳�)
	ID3D11Texture2D* SharedSurf = nullptr;
	IDXGIKeyedMutex* KeyMutex = nullptr;

	// �߳�Ԫ����
	THREAD_DATA* TData = reinterpret_cast<THREAD_DATA*>(Param);

	// ��ȡ������
	DUPL_RETURN Ret;
	HDESK CurrentDesktop = nullptr;
	CurrentDesktop = OpenInputDesktop(0, FALSE, GENERIC_ALL);

	// ���������ӵ����߳�
	bool DesktopAttached = SetThreadDesktop(CurrentDesktop) != 0;
	CloseDesktop(CurrentDesktop);
	CurrentDesktop = nullptr;

	// ��ʼ��DisplayManager
	DispMgr.InitD3D(&TData->DxRes);

	// ͬ�� Shared Surface
	HRESULT hr = TData->DxRes.Device->OpenSharedResource(TData->TexSharedHandle, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&SharedSurf));

	// ��ȡ������
	hr = SharedSurf->QueryInterface(__uuidof(IDXGIKeyedMutex), reinterpret_cast<void**>(&KeyMutex));

	// ��ʼ�� Duplication Manager
	Ret = DuplMgr.InitDupl(TData->DxRes.Device, TData->Output);

	// ��ȡ�������
	DXGI_OUTPUT_DESC DesktopDesc;
	RtlZeroMemory(&DesktopDesc, sizeof(DXGI_OUTPUT_DESC));
	DuplMgr.GetOutputDesc(&DesktopDesc);

	// ����ѭ����ʼ
	bool WaitToProcessCurrentFrame = false;
	FRAME_DATA CurrentData;

	while ((WaitForSingleObjectEx(TData->TerminateThreadsEvent, 0, FALSE) == WAIT_TIMEOUT))
	{
		if (!WaitToProcessCurrentFrame)
		{
			// ��ȡ��֡
			bool TimeOut;
			Ret = DuplMgr.GetFrame(&CurrentData, &TimeOut);
			if (Ret != DUPL_RETURN_SUCCESS)
			{
				break;
			}

			if (TimeOut)
			{
				continue;
			}
		}

		// �õ�����֡, ��Ҫ�������д���
		// ѯ�ʻ������Է��� Shared Surface
		hr = KeyMutex->AcquireSync(0, 1000);
		if (hr == static_cast<HRESULT>(WAIT_TIMEOUT))
		{
			// �����ܷ��� Shared Surface
			WaitToProcessCurrentFrame = true;
			continue;
		}
		else if (FAILED(hr))
		{
			break;
		}

		// ���Է��� Shared Surface
		WaitToProcessCurrentFrame = false;

		// ��ȡ�����Ϣ, ����TData->PtrInfo
		Ret = DuplMgr.GetMouse(TData->PtrInfo, &(CurrentData.FrameInfo), TData->OffsetX, TData->OffestY);

		// ����֡, ���� SharedSurf
		Ret = DispMgr.ProcessFrame(&CurrentData, SharedSurf, TData->OffsetX, TData->OffestY, &DesktopDesc);

		// �ͷ���
		hr = KeyMutex->ReleaseSync(1);

		// �ͷ�֡
		Ret = DuplMgr.DoneWithFrame();

		static int testNum = 0;
		testNum++;
		char str[6];
		_itoa_s(testNum, str, 10);
		std::string out;
		for (int j = 0; j < 6; j++)
		{
			out.push_back(str[j]);
		}
		OutputDebugString(L"֡��");
		OutputDebugStringA(out.data());
		OutputDebugString(L"\n");
	}

Exit:
	if (SharedSurf)
	{
		SharedSurf->Release();
		SharedSurf = nullptr;
	}

	if (KeyMutex)
	{
		KeyMutex->Release();
		KeyMutex = nullptr;
	}

	return 0;
}