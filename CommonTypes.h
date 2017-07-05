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

#include "PixelShader.h"		// 像素绘图头文件
#include "VertexShader.h"	// 顶点绘图头文件

#define	MAX_FPS		60

//
// 异常处理
//
enum CA_RETURN
{
	CA_RETURN_SUCCESS = 0,
	CA_RETURN_ERROR_EXPECTED = 1,
	CA_RETURN_ERROR_UNEXPECTED = 2
};

//
// 捕获模式
// 概述 :	主要分为 DDA 和 BBT 两种方法
typedef enum CAPTURE_MODE
{
	CAPTURE_MODE_STOP = 0,
	CAPTURE_MODE_UNKNOWN_FULLSCREEN,
	CAPTURE_MODE_UNKNOWN_WIN,
	CAPTURE_MODE_BBT_FULLSCREEN,
	CAPTURE_MODE_BBT_WINHANDLE,
	CAPTURE_MODE_BBT_WINRECT,
	CAPTURE_MODE_DDA_FULLSCREEN,
	CAPTURE_MODE_DDA_WINHANDLE,
	CAPTURE_MODE_DDA_WINRECT
} _CAPTURE_MODE;

//
// 捕获设置  
// 包括 :	时间戳、FPS、WinHandle、IsFollowCursor、目标矩形、是否显示鼠标
typedef struct CAPTURE_SETTING
{
	UINT64	TImeStamp;
	UINT64	OffsetTimeStamp = 0;
	UINT		FPS = 0;
	HWND		WinHandle;
	RECT		TargetRect;
	POINT		Anchor;
	bool			IsDisplay;
	bool			IsFollowCursor;
} _WINCAPTURE_SETTING;

//
// 帧信息
// 包括 :	位图缓冲区首地址、位图缓冲区大小、BytesPerLine、鼠标位置
typedef struct CAPTURE_FRAMEDATA
{
	BYTE*		pData = nullptr;
	UINT		uSize = 0;
	UINT		BytesPerLine;
	POINT*		CursorPos = nullptr;
} _CAPTURE_FRAMEDATA;



/*=================================================*/
/****************************↓ Used By DDA ↓************************************/
/*********************************************************************************/

#define NUMVERTICES 6
#define BPP 6

#define OCCLUSION_STATUS_MSG WM_USER

//
// 自定义返回值
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
// 鼠标信息
//		*PtrShapeBuffer	--- 鼠标形状缓冲
//		ShapeInfo			--- DXGI_OUTDUPL_POINTER_SHAPE_INFO
//		Position				--- 位置
//		Visible					--- 是否可见
//		BufferSize			--- 缓冲区大小
//		WhoUpdated...
//		LastTimeStamp	--- 上次更新的时间戳
typedef struct PTR_INFO
{
	_Field_size_bytes_(BufferSize) BYTE* PtrShapeBuffer;
	DXGI_OUTDUPL_POINTER_SHAPE_INFO ShapeInfo;	// 鼠标信息
	POINT Position;
	bool Visible;
	UINT BufferSize;
	UINT WhoUpdatedPositionLast;
	LARGE_INTEGER LastTimeStamp;
} _PTR_INFO;

// 
// D3D资源
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
// 线程单位
//		TerminateThreadsEvent		--- 线程终止
//		TexSharedHandle				--- 共享表面的句柄
//		Output									--- 输出数
//		OffsetX								--- X校正值
//		OffsetY									--- Y校正值
//		*PtrInfo								--- 指向{鼠标信息}结构体
//		DxRes									--- {D3D资源}结构体, 包括指向D3D组件的指针
typedef struct THREAD_DATA
{
	// 终止线程的Windows回调函数
	HANDLE TerminateThreadsEvent;

	HANDLE TexSharedHandle;
	UINT Output;
	INT OffsetX;
	INT OffestY;
	PTR_INFO* PtrInfo;
	DX_RESOURCES DxRes;
} _THREAD_DATA;

//
// 帧信息
//		*Frame			--- ID3D11Texture2D
//		FrameInfo	--- DXGI_OUTDUPL_FRAME_INFO
//		*MetaData	--- 脏矩形和移动矩形的缓冲区
//		DirtyCount	--- 脏矩形数
//		MoveCount	--- 移动矩形数
typedef struct FRAME_DATA
{
	ID3D11Texture2D* Frame;
	DXGI_OUTDUPL_FRAME_INFO FrameInfo;
	_Field_size_bytes_((MoveCount * sizeof(DXGI_OUTDUPL_MOVE_RECT)) + (DirtyCount * sizeof(RECT))) BYTE* MetaData; // 存放移动矩形和脏矩形
	UINT DirtyCount;
	UINT MoveCount;
} _FRAME_DATA;

//
// 顶点
//		Pos				--- 3维坐标
//		TexCoord		--- 2维坐标
typedef struct VERTEX
{
	DirectX::XMFLOAT3 Pos;
	DirectX::XMFLOAT2 TexCoord;
} _VERTEX;


/*********************************************************************************/
/****************************↑ Used By DDA ↑************************************/
/*=================================================*/
/****************************↓ Used By BBT  ↓************************************/
/**********************************************************************************/












/*********************************************************************************/
/****************************↑ Used By BBT ↑************************************/
/*=================================================*/
