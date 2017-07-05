#pragma once

#include "CommonTypes.h"

// ��ʾ������
// ����: D3D��Դ��ʼ����֡�Ĵ���
// ��Ա:	*m_Device										--- ID3D11Device
//			*m_DeviceContext							--- ID3D11DeviceContext
//			*m_MoveSurf									--- ID3D11Texture2D
//			*m_VertexShader								--- ID3D11VertexShader
//			*m_PixelShader								--- ID3D11PixelShader
//			*m_InputLayout								--- ID3D11InputLayout
//			*m_RTV											--- ID3D11RenderTargetView
//			*m_SamplerLinear							--- ID3D11SamplerState
//			*m_DirtyVertexBufferAlloc				--- BYTE, �ඥ�㻺�����׵�ַ
//			*m_DirtyVertexBufferAllocSize		--- �ඥ�㻺�����Ĵ�С
// ����:	InitD3D()					--- ���ݴ���� DX_RESOURCES ��ʼ��D3D��Դ
//			ProcessFrame()		--- ����֡(������㷨), ���� ID3D11Texture2D ����
//			GetDevice()				--- ��ȡ D3D �豸
//			CleanRefs()				--- �������
class DISPLAYMANAGER
{
public:
	DISPLAYMANAGER();
	~DISPLAYMANAGER();
	void InitD3D(DX_RESOURCES* Data);		// ����Data��ʼ��D3D��Դ
	ID3D11Device* GetDevice();					// ��ȡD3D�豸
	DUPL_RETURN ProcessFrame(_In_ FRAME_DATA* Data, _Inout_ ID3D11Texture2D* SharedSurf, INT OffsetX, INT OffsetY, _In_ DXGI_OUTPUT_DESC* DeskDesc);	// ����֡
	void CleanRefs();										// ������ü���
private:
	// ����
	DUPL_RETURN CopyDirty(_In_ ID3D11Texture2D* SrcSurface, _Inout_ ID3D11Texture2D* SharedSurf, _In_reads_(DirtyCount) RECT* DirtyBuffer, UINT DirtyCount, INT OffsetX, INT OffsetY, _In_ DXGI_OUTPUT_DESC* DeskDesc);
	DUPL_RETURN CopyMove(_Inout_ ID3D11Texture2D* SharedSurf, _In_reads_(MoveCount) DXGI_OUTDUPL_MOVE_RECT* MoveBuffer, UINT MoveCount, INT OffsetX, INT OffsetY, _In_ DXGI_OUTPUT_DESC* DeskDesc, INT TexWidth, INT TexHeight);	// �����ƶ�����
	void SetDirtyVert(_Out_writes_(NUMVERTICES) VERTEX* Vertices, _In_ RECT* Dirty, INT OffsetX, INT OffsetY, _In_ DXGI_OUTPUT_DESC* DeskDesc, _In_ D3D11_TEXTURE2D_DESC* FullDesc, _In_ D3D11_TEXTURE2D_DESC* ThisDesc);								// ��������ζ���
	void SetMoveRect(_Out_ RECT* SrcRect, _Out_ RECT* DestRect, _In_ DXGI_OUTPUT_DESC* DeskDesc, _In_ DXGI_OUTDUPL_MOVE_RECT* MoveRect, INT TexWidth, INT TexHeight);																													// �����ƶ�����

	// ����
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

