#pragma once

#include "CommonTypes.h"

//
// �����������
// ����:		���� DXGI �� DDA �ӿ�, ��������Ļ��ȡһ֡ ID3D11Texture2D, �����浽 FRAME_DATA ��
//				�����Ի�ȡ������Ϣ, �����浽 PTR_INFO ��
// ��Ա:		*m_DeskDupl							--- IDXGIOutputDuplication
//				*m_AcquiredDesktopImage	--- ID3D11Texture2D
//				*m_MetaDataBuffer					--- Ԫ���ݻ�����(��������κ��ƶ�����)
//				m_MetaDataSize						--- Ԫ���ݻ������Ĵ�С
//				m_OutputNumber					--- �����
//				m_OutputDesc						--- DXGI_OUPUT_DESC, �����豸��������ߴ硢����
//				*m_Device								--- ID3D11Device
// ����:		InitDupl()					--- ͨ�� Device ��ʼ�� DXGI ������ 
//				GetFrame()				--- ��ȡ����֡, д��FRAME_DATA
//				GetMouse()				--- ��ȡ���֡, д�� PTR_INFO
//				DoneWithFrame()	--- �ͷ�֡
//				GetOutputDesc()		--- ��ȡ������� DXGI_OUTPUT_DESC
//				
class DUPLICATIONMANAGER
{
public:
	DUPLICATIONMANAGER();
	~DUPLICATIONMANAGER();
	DUPL_RETURN GetFrame(_Out_ FRAME_DATA* Data, _Out_ bool* Timeout);	// ��ȡ֡
	DUPL_RETURN DoneWithFrame();	// �ͷ�֡
	DUPL_RETURN InitDupl(_In_ ID3D11Device* Device, UINT Output);	// ��ʼ��
	DUPL_RETURN GetMouse(_Inout_ PTR_INFO* PtrInfo, _In_ DXGI_OUTDUPL_FRAME_INFO* FrameInfo, INT OffsetX, INT OffsetY);	// ��ȡ���
	void GetOutputDesc(_Out_ DXGI_OUTPUT_DESC* DescPtr);		// ��ȡ�������
private:
	// ��Ա
	IDXGIOutputDuplication* m_DeskDupl;											// ������
	ID3D11Texture2D* m_AcquiredDesktopImage;								// ���������ͼƬ
	_Field_size_bytes_(m_MetaDataSize) BYTE* m_MetaDataBuffer;	// Ԫ���ݻ�����(��������κ��ƶ�����)
	UINT m_MetaDataSize;																	// Ԫ���ݴ�С
	UINT m_OutputNumber;																	// �����
	DXGI_OUTPUT_DESC m_OutputDesc;												// �������
	ID3D11Device* m_Device;																// �豸
};

