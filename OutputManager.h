#pragma once

#include "CommonTypes.h"

//
// 绘制帧数据到窗口中, 包含绘制鼠标的方法
// 功能 :	初始化 D3D 资源和不断更新窗口
// 变量 :	*m_SwapChain					--- IDXGISwapChain1
//				*m_Device							--- ID3D11Device
//				*m_Factory							--- IDXGIFactory2
//				*m_DeviceContext				--- ID3D11DeviceContext
//				*m_RTV								--- ID3D11RenderTargetView
//				*m_SamplerLinear				--- ID3D11SamplerState
//				*m_BlendState						--- ID3D11BlendState
//				*m_VertexShader					--- ID3D11VertexShader
//				*m_PixelShader					--- ID3D11PixelShader
//				*m_InputLayout					--- ID3D11InputLayout
//				*m_SharedSurf						--- ID3D11Texture2D
//				*m_KeyMutex						--- IDXGIKeyedMutex
//				m_WindowHandle				--- 目标窗口句柄
//				m_NeedsResize					--- 是否需要改变尺寸
//				m_OcclusionCookie				--- DWORD
// 方法 :	InitOutput()									--- 初始化 D3D 等资源
//				UpdateApplicationWindow()		--- 不断更新窗口
//				CleanRefs()									--- 清空引用, 释放资源
//				GetSharedHandle()						--- 获取共享资源的句柄
//				WindowResize()							--- 窗口尺寸发生变化
class OUTPUTMANAGER
{
public:
	OUTPUTMANAGER();
	~OUTPUTMANAGER();
	DUPL_RETURN InitOutput(HWND Window, INT SingleOutput, _Out_ UINT* OutCount, _Out_ RECT* DeskBounds);
	DUPL_RETURN UpdateApplicationWindow(_In_ PTR_INFO* PointerInfo, _Inout_ bool* Occluded);
	void CleanRefs();
	HANDLE GetSharedHandle();
	void WindowResize();
private:
	// 方法
	DUPL_RETURN ProcessMonoMask(bool IsMono, _Inout_ PTR_INFO* PtrInfo, _Out_ INT* PtrWidth, _Out_ INT* PtrHeight, _Out_ INT* PtrLeft, _Out_ INT* PtrTop, _Outptr_result_bytebuffer_(*PtrHeight * *PtrWidth * BPP) BYTE** InitBuffer, _Out_ D3D11_BOX* Box);
	DUPL_RETURN MakeRTV();
	void SetViewPort(UINT Width, UINT Height);
	DUPL_RETURN InitShaders();
	DUPL_RETURN InitGeometry();
	DUPL_RETURN CreateSharedSurf(INT SingleOutput, _Out_ UINT* OutCount, _Out_ RECT* DeskBounds);
	DUPL_RETURN DrawFrame();
	DUPL_RETURN DrawMouse(_In_ PTR_INFO* PtrInfo);
	DUPL_RETURN ResizeSwapChain();

	// 变量
	IDXGISwapChain1* m_SwapChain;
	ID3D11Device* m_Device;
	IDXGIFactory2* m_Factory;
	ID3D11DeviceContext* m_DeviceContext;
	ID3D11RenderTargetView* m_RTV;
	ID3D11SamplerState* m_SamplerLinear;
	ID3D11BlendState* m_BlendState;
	ID3D11VertexShader* m_VertexShader;
	ID3D11PixelShader* m_PixelShader;
	ID3D11InputLayout* m_InputLayout;
	ID3D11Texture2D* m_SharedSurf;
	IDXGIKeyedMutex* m_KeyMutex;
	HWND m_WindowHandle;
	bool m_NeedsResize;
	DWORD m_OcclusionCookie;
};

