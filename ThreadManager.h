#pragma once

#include "CommonTypes.h"

//
// �̹߳����ߣ������̳߳�
// ���� :	��ʼ�� D3D ��Դ����ȡ��ǰ���ָ����Ϣ����ʼ��ֹͣ�߳�
// ���� :	m_PtrInfo						--- PTR_INFO
//				m_ThreadCount			--- �߳���
//				m_ThreadHandles[]		--- [�߳̾��]����
//				m_ThreadData[]			--- [�߳�Ԫ����]����
// ���� :	Initialize()										--- �ô���Ĳ�����ʼ�� m_ThreadData[]��m_ThreadHandles[]
//				GetPointerInfo()							--- ��ȡ��ǰ���ָ����Ϣ
//				Clean()											--- �ͷ���Դ
//				WaitForThreadTermination()		--- �ȴ��߳̽���
class THREADMANAGER
{
public:
	THREADMANAGER();
	~THREADMANAGER();
	void Clean();
	DUPL_RETURN Initialize(INT SingleOutput, UINT OutputCount, HANDLE UnexpectedErrorEvent, HANDLE ExpectedErrorEvent, HANDLE TerminateThreadsEvent, HANDLE SharedHandle, _In_ RECT* DesktopDim);		// �̹߳�����(�̳߳�)�ĳ�ʼ��
	PTR_INFO* GetPointerInfo();
	void WaitForThreadTermination();

private:
	DUPL_RETURN InitializeDx(_Out_ DX_RESOURCES* Data);
	void CleanDx(_Inout_ DX_RESOURCES* Data);

	// ����
	PTR_INFO m_PtrInfo;
	UINT m_ThreadCount;
	_Field_size_(m_ThreadCount) HANDLE* m_ThreadHandles;			// ��Ա���߳̾�����飬Ԫ��Ϊ���
	_Field_size_(m_ThreadCount) THREAD_DATA* m_ThreadData;	// ��Ա���߳��������飬Ԫ��Ϊ�߳����ݣ��߳�����THREAD_DATA��<CommonTypes.h>����
};

