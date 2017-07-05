#pragma once

#include "DuplicationManager.h"
#include "DisplayManager.h"
#include "OutputManager.h"
#include "ThreadManager.h"

#include <VersionHelpers.h>

class ICALLBACK;

// 捕获管理工具
// 功能 :	提供捕获设置的接口、选择合适的方式( DDA 或 BBT )进行捕获
class CAPTUREMANAGER
{
public:
	CAPTUREMANAGER();
	~CAPTUREMANAGER();
	CA_RETURN		SetCallback(ICALLBACK** pCallback);		// 设置回调

	CA_RETURN		SetCaptureSetting(UINT64 uOffsetTimeStamp, UINT FPS, bool IsDisplay);	// 捕获设置1
	CA_RETURN		SetCaptureSetting(HWND WinHandle, RECT TargetRect);								// 捕获设置2
	CA_RETURN		SetCaptureSetting(POINT Anchor, bool IsFollowCursor);								// 捕获设置3

	CA_RETURN		StartCaptureFullScreen();		// 根据当前模式选择相应的函数进行"全屏捕获"
	CA_RETURN		StartCaptureWindow();			// 根据当前模式选择相应的函数进行"窗口捕获"
	
private:
	// 方法
	CA_RETURN		ChooseWay();		// 选择合适的模式

	CA_RETURN		DDACapture(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ INT nCmdShow);		// DDA 捕获方法
	CA_RETURN		BBTCapture();		// BBT 捕获方法

	DWORD	WINAPI	DDAProc(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ INT nCmdShow);			// DDA 进程

	// 变量
	CAPTURE_MODE			m_CaptureMode;
	CAPTURE_SETTING*	m_CaptureSetting;
	ICALLBACK*					m_Callback;

	OUTPUTMANAGER		m_OutMgr;
};

//
// 回调指针
// 包括 :	帧信息 WINCAPTURE_FRAMEDATA、时间戳、鼠标位置
class ICALLBACK
{
public:
	friend CAPTUREMANAGER;
	ICALLBACK();
	~ICALLBACK();
protected:
	CAPTURE_FRAMEDATA* m_FrameData;
	UINT64 m_TimeStamp;
	POINT* m_MousePoint;
};

