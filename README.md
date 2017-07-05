# MyDDA-BBT
DDA and BBT ways to capture the screen

## ʹ��˵��
//
// ADD: ����Ӧ�ó������
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
## �ӿڲ���
CaptureManager.h/.cpp

## DDA ����
DisplayManager.h/.cpp
DuplicationManager.h/.cpp
OutputManager.h/.cpp
ThreadManager.h/.cpp

## BBT ����
