# MyDDA-BBT
DDA and BBT ways to capture the screen

## 使用说明
//
// ADD: 窗口应用程序入口
//
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ INT nCmdShow)
{
	CAPTUREMANAGER CaptureMgr;
	ICALLBACK* pCallback = nullptr;
	CaptureMgr.SetCallback(&pCallback);

	CaptureMgr.StartCaptureFullScreen(hInstance, hPrevInstance, lpCmdLine, nCmdShow);

	while (1) {
		Sleep(1000);
	}
	return 0;
}
## 接口部分
CaptureManager.h/.cpp

## DDA 部分
DisplayManager.h/.cpp
DuplicationManager.h/.cpp
OutputManager.h/.cpp
ThreadManager.h/.cpp

## BBT 部分
