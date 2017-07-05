#include <limits.h>

#include "DisplayManager.h"
#include "DuplicationManager.h"
#include "OutputManager.h"
#include "ThreadManager.h"

//
// ȫ�ֱ���
//
OUTPUTMANAGER OutMgr;

//
// ��������
//
DWORD WINAPI DDProc(_In_ void* Param);
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


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


//
// ����Ӧ�ó������
//
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ INT nCmdShow)
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
	// ShowWindow(WindowHandle, SW_HIDE);

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
			Ret = OutMgr.InitOutput(WindowHandle, SingleOutput, &OutputCount, &DeskBounds);
			if (Ret == DUPL_RETURN_SUCCESS)
			{
				HANDLE SharedHandle = OutMgr.GetSharedHandle();
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
				Ret = OutMgr.UpdateApplicationWindow(ThreadMgr.GetPointerInfo(), &Occluded);
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
		OutMgr.WindowResize();
		break;
	}
	default:
		return DefWindowProc(hWnd, message, wParam, lParam); // �������򲻹��ĵ���Ϣ
	}
	return 0;
}

//
// Desktop Duplication �߳����
// �����Ҫ��
//		0- ��ʼ��THREAD_DATA����ָ��TData
//		1- ��ȡ����
//		2- �����������ӵ��߳�
//		3- ��ʼ��DisplayManager
//		4- ָ��TData->TexSharedHandleΪ�������(Shared Surface)
//		5- ��ȡ�������Ļ�����KeyMutex
//		5- ��ʼ��DuplicationManager
//		6- ��ȡ�������DXGI_OUTPUT_DESC
//		7- ��ʼDuplication��ѭ��
//			7.1- ���û����֡, ����DuplicationManager��ȡ��֡������CurrentData��
//			7.2- ����������֡, ��Ҫ�������д���
//			7.3- ������, ����δ��, ��ֱ�ӽ�����һ��ѭ��
//			7.4- �����ɹ���, ����DuplicationManager��ȡ��겢����TData��
//			7.5- ����DisplayManager����֡(������㷨)
//			7.6- �ͷ���, �ͷ�֡
//		8- ���
//
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
		OutputDebugString(L"��");
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

