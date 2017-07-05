#pragma once

#include "CommonTypes.h"

//
// 线程管理工具，类似线程池
// 功能 :	初始化 D3D 资源、获取当前鼠标指针信息、开始和停止线程
// 变量 :	m_PtrInfo						--- PTR_INFO
//				m_ThreadCount			--- 线程数
//				m_ThreadHandles[]		--- [线程句柄]数组
//				m_ThreadData[]			--- [线程元数据]数组
// 方法 :	Initialize()										--- 用传入的参数初始化 m_ThreadData[]、m_ThreadHandles[]
//				GetPointerInfo()							--- 获取当前鼠标指针信息
//				Clean()											--- 释放资源
//				WaitForThreadTermination()		--- 等待线程结束
class THREADMANAGER
{
public:
	THREADMANAGER();
	~THREADMANAGER();
	void Clean();
	DUPL_RETURN Initialize(INT SingleOutput, UINT OutputCount, HANDLE UnexpectedErrorEvent, HANDLE ExpectedErrorEvent, HANDLE TerminateThreadsEvent, HANDLE SharedHandle, _In_ RECT* DesktopDim);		// 线程管理工具(线程池)的初始化
	PTR_INFO* GetPointerInfo();
	void WaitForThreadTermination();

private:
	DUPL_RETURN InitializeDx(_Out_ DX_RESOURCES* Data);
	void CleanDx(_Inout_ DX_RESOURCES* Data);

	// 变量
	PTR_INFO m_PtrInfo;
	UINT m_ThreadCount;
	_Field_size_(m_ThreadCount) HANDLE* m_ThreadHandles;			// 成员，线程句柄数组，元素为句柄
	_Field_size_(m_ThreadCount) THREAD_DATA* m_ThreadData;	// 成员，线程数据数组，元素为线程数据，线程数据THREAD_DATA在<CommonTypes.h>定义
};

