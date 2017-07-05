#pragma once

#include "DuplicationManager.h"
#include "DisplayManager.h"
#include "OutputManager.h"
#include "ThreadManager.h"

#include <VersionHelpers.h>

class ICALLBACK;

// ���������
// ���� :	�ṩ�������õĽӿڡ�ѡ����ʵķ�ʽ( DDA �� BBT )���в���
class CAPTUREMANAGER
{
public:
	CAPTUREMANAGER();
	~CAPTUREMANAGER();
	CA_RETURN		SetCallback(ICALLBACK** pCallback);		// ���ûص�

	CA_RETURN		SetCaptureSetting(UINT64 uOffsetTimeStamp, UINT FPS, bool IsDisplay);	// ��������1
	CA_RETURN		SetCaptureSetting(HWND WinHandle, RECT TargetRect);								// ��������2
	CA_RETURN		SetCaptureSetting(POINT Anchor, bool IsFollowCursor);								// ��������3

	CA_RETURN		StartCaptureFullScreen();		// ���ݵ�ǰģʽѡ����Ӧ�ĺ�������"ȫ������"
	CA_RETURN		StartCaptureWindow();			// ���ݵ�ǰģʽѡ����Ӧ�ĺ�������"���ڲ���"
	
private:
	// ����
	CA_RETURN		ChooseWay();		// ѡ����ʵ�ģʽ

	CA_RETURN		DDACapture(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ INT nCmdShow);		// DDA ���񷽷�
	CA_RETURN		BBTCapture();		// BBT ���񷽷�

	DWORD	WINAPI	DDAProc(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ INT nCmdShow);			// DDA ����

	// ����
	CAPTURE_MODE			m_CaptureMode;
	CAPTURE_SETTING*	m_CaptureSetting;
	ICALLBACK*					m_Callback;

	OUTPUTMANAGER		m_OutMgr;
};

//
// �ص�ָ��
// ���� :	֡��Ϣ WINCAPTURE_FRAMEDATA��ʱ��������λ��
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

