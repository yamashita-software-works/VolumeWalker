//****************************************************************************
//*                                                                          *
//*  imagehelp.cpp                                                           *
//*                                                                          *
//*  PURPOSE:  Bitmap image helper functions.                                *
//*                                                                          *
//*  AUTHOR:   YAMASHITA Katsuhiro                                           *
//*                                                                          *
//*  HISTORY:  2025-05-06 Created                                            *
//*                                                                          *
//****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include "stdafx.h"

HBITMAP _CreateBitmapARGB(int nWidth, int nHeight)
{
	LPVOID           lpBits;
	BITMAPINFO       bmi;
	BITMAPINFOHEADER bmiHeader;

	ZeroMemory(&bmiHeader, sizeof(BITMAPINFOHEADER));
	bmiHeader.biSize      = sizeof(BITMAPINFOHEADER);
	bmiHeader.biWidth     = nWidth;
	bmiHeader.biHeight    = nHeight;
	bmiHeader.biPlanes    = 1;
	bmiHeader.biBitCount  = 32;

	bmi.bmiHeader = bmiHeader;
	
	return CreateDIBSection(NULL, (LPBITMAPINFO)&bmi, DIB_RGB_COLORS, &lpBits, NULL, 0);
}

HBITMAP _IconToBitmap(HICON hIcon)
{
	UINT    uWidth, uHeight;
	HDC     hdcMem;
	HBITMAP hbmp, hbmpPrev;

	ICONINFO iconinfo = {0};
	GetIconInfo(hIcon,&iconinfo);

	BITMAP bm = {0};
	GetObject(iconinfo.hbmMask,sizeof(bm),&bm);

	uWidth  = bm.bmWidth;
	uHeight = bm.bmHeight;

	DeleteObject(iconinfo.hbmColor);
	DeleteObject(iconinfo.hbmMask);

	hbmp = _CreateBitmapARGB(uWidth, uHeight);
	
	hdcMem = CreateCompatibleDC(NULL);
	hbmpPrev = (HBITMAP)SelectObject(hdcMem, hbmp);
	DrawIconEx(hdcMem, 0, 0, hIcon, uWidth, uHeight, 0, NULL, DI_NORMAL);
	SelectObject(hdcMem, hbmpPrev);
	DeleteDC(hdcMem);

	return hbmp;
}

HBITMAP _IconToBitmap16(HICON hIcon) /* 16 is meaning small icon */
{
	UINT         uWidth, uHeight;
	HDC          hdcMem;
	HBITMAP      hbmp, hbmpPrev;

	uWidth = GetSystemMetrics(SM_CXSMICON);
	uHeight = GetSystemMetrics(SM_CYSMICON);

	hbmp = _CreateBitmapARGB(uWidth, uHeight);
	
	hdcMem = CreateCompatibleDC(NULL);
	hbmpPrev = (HBITMAP)SelectObject(hdcMem, hbmp);
	DrawIconEx(hdcMem, 0, 0, hIcon, uWidth, uHeight, 0, NULL, DI_NORMAL);// DI_IMAGE); BUGFIX 20240216
	SelectObject(hdcMem, hbmpPrev);
	DeleteDC(hdcMem);

	return hbmp;
}

HBITMAP _IconToBitmap32(HICON hIcon)
{
	UINT         uWidth, uHeight;
	HDC          hdcMem;
	HBITMAP      hbmp, hbmpPrev;

	uWidth = GetSystemMetrics(SM_CXICON);
	uHeight = GetSystemMetrics(SM_CYICON);

	hbmp = _CreateBitmapARGB(uWidth, uHeight);
	
	hdcMem = CreateCompatibleDC(NULL);
	hbmpPrev = (HBITMAP)SelectObject(hdcMem, hbmp);
	DrawIconEx(hdcMem, 0, 0, hIcon, uWidth, uHeight, 0, NULL, DI_NORMAL); // DI_IMAGE); BUGFIX 20240216
	SelectObject(hdcMem, hbmpPrev);
	DeleteDC(hdcMem);

	return hbmp;
}

HBITMAP _GetOverlayImageToBitmap(HIMAGELIST himl,int index,int overlay)
{
	UINT         uWidth, uHeight;
	HDC          hdcMem;
	HBITMAP      hbmp, hbmpPrev;

	uWidth = GetSystemMetrics(SM_CXSMICON);
	uHeight = GetSystemMetrics(SM_CYSMICON);
	hbmp = _CreateBitmapARGB(uWidth, uHeight);
	
	hdcMem = CreateCompatibleDC(NULL);
	hbmpPrev = (HBITMAP)SelectObject(hdcMem, hbmp);
	ImageList_Draw(himl,index,hdcMem,0,0,ILD_NORMAL|INDEXTOOVERLAYMASK(overlay));
	SelectObject(hdcMem, hbmpPrev);

	DeleteDC(hdcMem);

	return hbmp;
}

#ifdef _USE_MSIMG32_DLL
VOID _DrawBitmapAlphaBlend(HDC hdc,int x,int y,HBITMAP hbmpARGB)
{
	HDC hdcBmp = NULL;
	hdcBmp = CreateCompatibleDC(hdc);

	HGDIOBJ hbmpOld;
	hbmpOld = (HBITMAP)SelectObject(hdcBmp,hbmpARGB);

	BITMAP bm;
	GetObject(hbmpARGB,sizeof(bm),&bm);

	DIBSECTION dib;
	GetObject(hbmpARGB,sizeof(DIBSECTION),&dib);

	BLENDFUNCTION bf;
	bf.AlphaFormat = AC_SRC_ALPHA;
	bf.BlendFlags = 0;
	bf.BlendOp = AC_SRC_OVER;
	bf.SourceConstantAlpha = 0xff;
	AlphaBlend(hdc,x,y,bm.bmWidth,bm.bmHeight,hdcBmp,0,0,bm.bmWidth,bm.bmHeight,bf);

	SelectObject(hdcBmp,hbmpOld);

	DeleteDC(hdcBmp);
}
#endif
