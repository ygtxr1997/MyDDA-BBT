#pragma once

#include "CommonTypes.h"

// 显示管理工具
// 功能: D3D资源初始化、帧的处理
// 成员:	*m_Device										--- ID3D11Device
//			*m_DeviceContext							--- ID3D11DeviceContext
//			*m_MoveSurf									--- ID3D11Texture2D
//			*m_VertexShader								--- ID3D11VertexShader
//			*m_PixelShader								--- ID3D11PixelShader
//			*m_InputLayout								--- ID3D11InputLayout
//			*m_RTV											--- ID3D11RenderTargetView
//			*m_SamplerLinear							--- ID3D11SamplerState
//			*m_DirtyVertexBufferAlloc				--- BYTE, 脏顶点缓冲区首地址
//			*m_DirtyVertexBufferAllocSize		--- 脏顶点缓冲区的大小
// 方法:	InitD3D()					--- 根据传入的 DX_RESOURCES 初始化D3D资源
//			ProcessFrame()		--- 处理帧(脏矩形算法), 更新 ID3D11Texture2D 参数
//			GetDevice()				--- 获取 D3D 设备
//			CleanRefs()				--- 清空引用
class DISPLAYMANAGER
{
public:
	DISPLAYMANAGER();
	~DISPLAYMANAGER();
	void InitD3D(DX_RESOURCES* Data);		// 根据Data初始化D3D资源
	ID3D11Device* GetDevice();					// 获取D3D设备
	DUPL_RETURN ProcessFrame(_In_ FRAME_DATA* Data, _Inout_ ID3D11Texture2D* SharedSurf, INT OffsetX, INT OffsetY, _In_ DXGI_OUTPUT_DESC* DeskDesc);	// 处理帧
	void CleanRefs();										// 清空引用计数
private:
	// 方法
	DUPL_RETURN CopyDirty(_In_ ID3D11Texture2D* SrcSurface, _Inout_ ID3D11Texture2D* SharedSurf, _In_reads_(DirtyCount) RECT* DirtyBuffer, UINT DirtyCount, INT OffsetX, INT OffsetY, _In_ DXGI_OUTPUT_DESC* DeskDesc);
	DUPL_RETURN CopyMove(_Inout_ ID3D11Texture2D* SharedSurf, _In_reads_(MoveCount) DXGI_OUTDUPL_MOVE_RECT* MoveBuffer, UINT MoveCount, INT OffsetX, INT OffsetY, _In_ DXGI_OUTPUT_DESC* DeskDesc, INT TexWidth, INT TexHeight);	// 复制移动矩形
	void SetDirtyVert(_Out_writes_(NUMVERTICES) VERTEX* Vertices, _In_ RECT* Dirty, INT OffsetX, INT OffsetY, _In_ DXGI_OUTPUT_DESC* DeskDesc, _In_ D3D11_TEXTURE2D_DESC* FullDesc, _In_ D3D11_TEXTURE2D_DESC* ThisDesc);								// 设置脏矩形顶点
	void SetMoveRect(_Out_ RECT* SrcRect, _Out_ RECT* DestRect, _In_ DXGI_OUTPUT_DESC* DeskDesc, _In_ DXGI_OUTDUPL_MOVE_RECT* MoveRect, INT TexWidth, INT TexHeight);																													// 设置移动矩形

	// 变量
	ID3D11Device* m_Device;
	ID3D11DeviceContext* m_DeviceContext;
	ID3D11Texture2D* m_MoveSurf;
	ID3D11VertexShader* m_VertexShader;
	ID3D11PixelShader* m_PixelShader;
	ID3D11InputLayout* m_InputLayout;
	ID3D11RenderTargetView* m_RTV;
	ID3D11SamplerState* m_SamplerLinear;
	BYTE* m_DirtyVertexBufferAlloc;
	UINT m_DirtyVertexBufferAllocSize;
};

