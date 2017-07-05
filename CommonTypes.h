#pragma once

#include <Windows.h>
#include <VersionHelpers.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <sal.h>
#include <warning.h>
#include <DirectXMath.h>

#include <new>
#include <stdio.h>

#include "PixelShader.h"		// ���ػ�ͼͷ�ļ�
#include "VertexShader.h"	// �����ͼͷ�ļ�

#define NUMVERTICES 6
#define BPP 6

#define OCCLUSION_STATUS_MSG WM_USER

/*=================================================*/
/****************************�� Used By DDA ��************************************/
/*********************************************************************************/
//
// �Զ��巵��ֵ
//		0 : SUCCESS
//		1 : ERROR_EXPECTED
//		2 : ERROR_UNEXPECTED
typedef enum DUPL_RETURN
{
	DUPL_RETURN_SUCCESS = 0,
	DUPL_RETURN_ERROR_EXPECTED = 1,
	DUPL_RETURN_ERROR_UNEXPECTED = 2
} _DUPL_RETURN;	


//
// �����Ϣ
//		*PtrShapeBuffer	--- �����״����
//		ShapeInfo			--- DXGI_OUTDUPL_POINTER_SHAPE_INFO
//		Position				--- λ��
//		Visible					--- �Ƿ�ɼ�
//		BufferSize			--- ��������С
//		WhoUpdated...
//		LastTimeStamp	--- �ϴθ��µ�ʱ���
typedef struct PTR_INFO
{
	_Field_size_bytes_(BufferSize) BYTE* PtrShapeBuffer;
	DXGI_OUTDUPL_POINTER_SHAPE_INFO ShapeInfo;	// �����Ϣ
	POINT Position;
	bool Visible;
	UINT BufferSize;
	UINT WhoUpdatedPositionLast;
	LARGE_INTEGER LastTimeStamp;
} _PTR_INFO;

// 
// D3D��Դ
//		*Device					--- ID3D11Device
//		*Context					--- ID3D11DeviceContext
//		*VertexShader			--- ID3D11VertexShader
//		*PixelShader			--- ID3D11PixelShader
//		*InputLayout			--- ID3D11InputLayout
//		*SamplerLinear		--- ID3D11SamplerState
typedef struct DX_RESOURCES
{
	ID3D11Device* Device;
	ID3D11DeviceContext* Context;
	ID3D11VertexShader* VertexShader;
	ID3D11PixelShader* PixelShader;
	ID3D11InputLayout* InputLayout;
	ID3D11SamplerState* SamplerLinear;
} _DX_RESOURCES;

//
// �̵߳�λ
//		TerminateThreadsEvent		--- �߳���ֹ
//		TexSharedHandle				--- �������ľ��
//		Output									--- �����
//		OffsetX								--- XУ��ֵ
//		OffsetY									--- YУ��ֵ
//		*PtrInfo								--- ָ��{�����Ϣ}�ṹ��
//		DxRes									--- {D3D��Դ}�ṹ��, ����ָ��D3D�����ָ��
typedef struct THREAD_DATA
{
	// ��ֹ�̵߳�Windows�ص�����
	HANDLE TerminateThreadsEvent;

	HANDLE TexSharedHandle;
	UINT Output;
	INT OffsetX;
	INT OffestY;
	PTR_INFO* PtrInfo;
	DX_RESOURCES DxRes;
} _THREAD_DATA;

//
// ֡��Ϣ
//		*Frame			--- ID3D11Texture2D
//		FrameInfo	--- DXGI_OUTDUPL_FRAME_INFO
//		*MetaData	--- ����κ��ƶ����εĻ�����
//		DirtyCount	--- �������
//		MoveCount	--- �ƶ�������
typedef struct FRAME_DATA
{
	ID3D11Texture2D* Frame;
	DXGI_OUTDUPL_FRAME_INFO FrameInfo;
	_Field_size_bytes_((MoveCount * sizeof(DXGI_OUTDUPL_MOVE_RECT)) + (DirtyCount * sizeof(RECT))) BYTE* MetaData; // ����ƶ����κ������
	UINT DirtyCount;
	UINT MoveCount;
} _FRAME_DATA;

//
// ����
//		Pos				--- 3ά����
//		TexCoord		--- 2ά����
typedef struct VERTEX
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT2 TexCoord;
} _VERTEX;


/*********************************************************************************/
/****************************�� Used By DDA ��************************************/
/*=================================================*/
/****************************�� Used By BBT  ��************************************/
/**********************************************************************************/












/*********************************************************************************/
/****************************�� Used By BBT ��************************************/
/*=================================================*/
