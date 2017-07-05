#pragma once

#include "CommonTypes.h"

//
// 复制项管理工具
// 功能:		利用 DXGI 的 DDA 接口, 从桌面屏幕获取一帧 ID3D11Texture2D, 并保存到 FRAME_DATA 中
//				还可以获取鼠标的信息, 并保存到 PTR_INFO 中
// 成员:		*m_DeskDupl							--- IDXGIOutputDuplication
//				*m_AcquiredDesktopImage	--- ID3D11Texture2D
//				*m_MetaDataBuffer					--- 元数据缓冲区(保存脏矩形和移动矩形)
//				m_MetaDataSize						--- 元数据缓冲区的大小
//				m_OutputNumber					--- 输出数
//				m_OutputDesc						--- DXGI_OUPUT_DESC, 包括设备名、桌面尺寸、朝向
//				*m_Device								--- ID3D11Device
// 方法:		InitDupl()					--- 通过 Device 初始化 DXGI 相关组件 
//				GetFrame()				--- 获取桌面帧, 写入FRAME_DATA
//				GetMouse()				--- 获取鼠标帧, 写入 PTR_INFO
//				DoneWithFrame()	--- 释放帧
//				GetOutputDesc()		--- 获取输出描述 DXGI_OUTPUT_DESC
//				
class DUPLICATIONMANAGER
{
public:
	DUPLICATIONMANAGER();
	~DUPLICATIONMANAGER();
	DUPL_RETURN GetFrame(_Out_ FRAME_DATA* Data, _Out_ bool* Timeout);	// 获取帧
	DUPL_RETURN DoneWithFrame();	// 释放帧
	DUPL_RETURN InitDupl(_In_ ID3D11Device* Device, UINT Output);	// 初始化
	DUPL_RETURN GetMouse(_Inout_ PTR_INFO* PtrInfo, _In_ DXGI_OUTDUPL_FRAME_INFO* FrameInfo, INT OffsetX, INT OffsetY);	// 获取鼠标
	void GetOutputDesc(_Out_ DXGI_OUTPUT_DESC* DescPtr);		// 获取输出描述
private:
	// 成员
	IDXGIOutputDuplication* m_DeskDupl;											// 复制项
	ID3D11Texture2D* m_AcquiredDesktopImage;								// 请求的桌面图片
	_Field_size_bytes_(m_MetaDataSize) BYTE* m_MetaDataBuffer;	// 元数据缓冲区(保存脏矩形和移动矩形)
	UINT m_MetaDataSize;																	// 元数据大小
	UINT m_OutputNumber;																	// 输出数
	DXGI_OUTPUT_DESC m_OutputDesc;												// 输出描述
	ID3D11Device* m_Device;																// 设备
};

