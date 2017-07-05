#include <limits.h>

#include "DisplayManager.h"
#include "DuplicationManager.h"
#include "OutputManager.h"
#include "ThreadManager.h"

//
// 全局变量
//
OUTPUTMANAGER OutMgr;

//
// 函数声明
//
DWORD WINAPI DDProc(_In_ void* Param);
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


//
// 等待队列
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
	m_QPCValid = QueryPerformanceFrequency(&m_QPCFrequency);	// 获取精确频率
	m_LastWakeUpTime.QuadPart = 0L; // 清零
}

DYNAMIC_WAIT::~DYNAMIC_WAIT()
{
}

void DYNAMIC_WAIT::Wait()
{
	LARGE_INTEGER CurrentQPC = { 0 };
	QueryPerformanceFrequency(&CurrentQPC);	// 获取精准时间

	if (m_QPCValid && (CurrentQPC.QuadPart <= (m_LastWakeUpTime.QuadPart + (m_QPCFrequency.QuadPart * m_WaitSequenceTimeInSeconds)))) // 未超时
	{
		if ((m_WaitBands[m_CurrentWaitBandIdx].WaitCount != WAIT_BAND_STOP) && (m_WaitCountInCurrentBand > m_WaitBands[m_CurrentWaitBandIdx].WaitCount))
		{
			m_CurrentWaitBandIdx++;
			m_WaitCountInCurrentBand = 0;
		}
	}
	else	// 超时, 则重置
	{
		m_WaitCountInCurrentBand = 0;
		m_CurrentWaitBandIdx = 0;
	}

	// Sleep对应的时间
	Sleep(m_WaitBands[m_CurrentWaitBandIdx].WaitTime);

	// 记录唤醒时间
	QueryPerformanceCounter(&m_LastWakeUpTime);
	m_WaitCountInCurrentBand++;
}


//
// 窗口应用程序入口
//
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ INT nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);	// UNREFERENCED_PARAMETER()宏定义, 代表这个参数不会再在函数体中出现
	UNREFERENCED_PARAMETER(lpCmdLine);

	// ------------------- 判断版本号
	BOOL useD3D = IsWindows8OrGreater();
	if (!useD3D) {
		Sleep(2000);
		return -1;
	}

	INT SingleOutput = -1;

	// 终止线程事件
	HANDLE TerminateThreadsEvent = nullptr;
	TerminateThreadsEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

	// 窗口句柄
	HWND WindowHandle = nullptr;

	// 为窗口加载鼠标
	HCURSOR Cursor = nullptr;
	Cursor = LoadCursor(nullptr, IDC_ARROW);

	// 注册窗口属性
	WNDCLASSEXW Wc;
	Wc.cbSize = sizeof(WNDCLASSEXW);
	Wc.style = CS_HREDRAW | CS_VREDRAW;		// 水平、垂直变化都要重绘窗口
	Wc.lpfnWndProc = WndProc;			// 窗口进程
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

	// 创建窗口
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

	// 消息循环
	MSG msg = { 0 };
	bool FirstTime = true;
	bool Occluded = true;
	DYNAMIC_WAIT DynamicWait;

	while (WM_QUIT != msg.message)
	{
		DUPL_RETURN Ret = DUPL_RETURN_SUCCESS;
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == OCCLUSION_STATUS_MSG)	// 未发生冲突
				Occluded = false;
			else {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else if (FirstTime)		// 第一次或者出现异常
		{
			if (!FirstTime)
			{
			}
			else
				FirstTime = false;

			// 重新初始化OutMgr
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
			if (!Occluded)		// 可以用OutMgr更新窗口了
			{
				Ret = OutMgr.UpdateApplicationWindow(ThreadMgr.GetPointerInfo(), &Occluded);
			}
		}
	} // 一次消息循环结束

	// 确保线程全部退出
	if (SetEvent(TerminateThreadsEvent))
	{
		ThreadMgr.WaitForThreadTermination();
	}

	// 清空
	CloseHandle(TerminateThreadsEvent);

	// 退出
	if (msg.message == WM_QUIT)
	{
		return static_cast<INT>(msg.wParam);
	}

	return 0;
}

//
// 窗口消息处理者
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
		// 尺寸改变, 通知OutMgr重绘窗口
		OutMgr.WindowResize();
		break;
	}
	default:
		return DefWindowProc(hWnd, message, wParam, lParam); // 处理本程序不关心的消息
	}
	return 0;
}

//
// Desktop Duplication 线程入口
// 步骤概要：
//		0- 初始化THREAD_DATA对象指针TData
//		1- 获取桌面
//		2- 把桌面句柄附加到线程
//		3- 初始化DisplayManager
//		4- 指定TData->TexSharedHandle为共享表面(Shared Surface)
//		5- 获取共享表面的互斥锁KeyMutex
//		5- 初始化DuplicationManager
//		6- 获取输出描述DXGI_OUTPUT_DESC
//		7- 开始Duplication的循环
//			7.1- 如果没有新帧, 调用DuplicationManager获取新帧并放入CurrentData中
//			7.2- 如果获得了新帧, 需要对它进行处理
//			7.3- 请求锁, 若还未打开, 则直接进入下一次循环
//			7.4- 若锁成功打开, 调用DuplicationManager获取鼠标并放入TData中
//			7.5- 调用DisplayManager处理帧(脏矩形算法)
//			7.6- 释放锁, 释放帧
//		8- 清空
//
DWORD WINAPI DDProc(_In_ void* Param)
{
	// ------------------- 判断版本号
	BOOL useD3D = IsWindows8OrGreater();
	if (!useD3D)
		return -1;

	// 自定义类
	DISPLAYMANAGER DispMgr;
	DUPLICATIONMANAGER DuplMgr;

	// D3D对象(用于线程)
	ID3D11Texture2D* SharedSurf = nullptr;
	IDXGIKeyedMutex* KeyMutex = nullptr;

	// 线程元数据
	THREAD_DATA* TData = reinterpret_cast<THREAD_DATA*>(Param);

	// 获取桌面句柄
	DUPL_RETURN Ret;
	HDESK CurrentDesktop = nullptr;
	CurrentDesktop = OpenInputDesktop(0, FALSE, GENERIC_ALL);

	// 桌面句柄附加到本线程
	bool DesktopAttached = SetThreadDesktop(CurrentDesktop) != 0;
	CloseDesktop(CurrentDesktop);
	CurrentDesktop = nullptr;

	// 初始化DisplayManager
	DispMgr.InitD3D(&TData->DxRes);

	// 同步 Shared Surface
	HRESULT hr = TData->DxRes.Device->OpenSharedResource(TData->TexSharedHandle, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&SharedSurf));

	// 获取互斥锁
	hr = SharedSurf->QueryInterface(__uuidof(IDXGIKeyedMutex), reinterpret_cast<void**>(&KeyMutex));

	// 初始化 Duplication Manager
	Ret = DuplMgr.InitDupl(TData->DxRes.Device, TData->Output);

	// 获取输出描述
	DXGI_OUTPUT_DESC DesktopDesc;
	RtlZeroMemory(&DesktopDesc, sizeof(DXGI_OUTPUT_DESC));
	DuplMgr.GetOutputDesc(&DesktopDesc);

	// 复制循环开始
	bool WaitToProcessCurrentFrame = false;
	FRAME_DATA CurrentData;

	while ((WaitForSingleObjectEx(TData->TerminateThreadsEvent, 0, FALSE) == WAIT_TIMEOUT))
	{
		if (!WaitToProcessCurrentFrame)
		{
			// 获取新帧
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

		// 得到了新帧, 需要对它进行处理
		// 询问互斥锁以访问 Shared Surface
		hr = KeyMutex->AcquireSync(0, 1000);
		if (hr == static_cast<HRESULT>(WAIT_TIMEOUT))
		{
			// 还不能访问 Shared Surface
			WaitToProcessCurrentFrame = true;
			continue;
		}
		else if (FAILED(hr))
		{
			break;
		}

		// 可以访问 Shared Surface
		WaitToProcessCurrentFrame = false;

		// 获取鼠标信息, 存入TData->PtrInfo
		Ret = DuplMgr.GetMouse(TData->PtrInfo, &(CurrentData.FrameInfo), TData->OffsetX, TData->OffestY);

		// 处理帧, 存入 SharedSurf
		Ret = DispMgr.ProcessFrame(&CurrentData, SharedSurf, TData->OffsetX, TData->OffestY, &DesktopDesc);

		// 释放锁
		hr = KeyMutex->ReleaseSync(1);

		// 释放帧
		Ret = DuplMgr.DoneWithFrame();

		static int testNum = 0;
		OutputDebugString(L"成");
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

