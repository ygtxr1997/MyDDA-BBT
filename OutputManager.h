#pragma once

#include "CommonTypes.h"

//
// ����֡���ݵ�������, �����������ķ���
// ���� :	��ʼ�� D3D ��Դ�Ͳ��ϸ��´���
// ���� :	*m_SwapChain					--- IDXGISwapChain1
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
//				m_WindowHandle				--- Ŀ�괰�ھ��
//				m_NeedsResize					--- �Ƿ���Ҫ�ı�ߴ�
//				m_OcclusionCookie				--- DWORD
// ���� :	InitOutput()									--- ��ʼ�� D3D ����Դ
//				UpdateApplicationWindow()		--- ���ϸ��´���
//				CleanRefs()									--- �������, �ͷ���Դ
//				GetSharedHandle()						--- ��ȡ������Դ�ľ��
//				WindowResize()							--- ���ڳߴ緢���仯
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
	// ����
	DUPL_RETURN ProcessMonoMask(bool IsMono, _Inout_ PTR_INFO* PtrInfo, _Out_ INT* PtrWidth, _Out_ INT* PtrHeight, _Out_ INT* PtrLeft, _Out_ INT* PtrTop, _Outptr_result_bytebuffer_(*PtrHeight * *PtrWidth * BPP) BYTE** InitBuffer, _Out_ D3D11_BOX* Box);
	DUPL_RETURN MakeRTV();
	void SetViewPort(UINT Width, UINT Height);
	DUPL_RETURN InitShaders();
	DUPL_RETURN InitGeometry();
	DUPL_RETURN CreateSharedSurf(INT SingleOutput, _Out_ UINT* OutCount, _Out_ RECT* DeskBounds);
	DUPL_RETURN DrawFrame();
	DUPL_RETURN DrawMouse(_In_ PTR_INFO* PtrInfo);
	DUPL_RETURN ResizeSwapChain();

	// ����
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

