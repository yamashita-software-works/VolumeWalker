#pragma once

//
// meterbox.h
//
// 2017.08.30 Created
//

/////////////////////////////////////////////////////////////////////////////
// MeterBox Custom Control

void InitMeterBox(HMODULE hModule);

#define MTBC_METERBOX_NAME L"MeterBox"

#define MTBS_BORDER          (0x0001)

#define MTBM_SETFULL         (WM_USER+601)
#define MTBM_GETFULL         (WM_USER+602)
#define MTBM_SETPOS          (WM_USER+603)
#define MTBM_GETPOS          (WM_USER+604)
#define MTBM_SETINFO         (WM_USER+605)
#define MTBM_GETINFO         (WM_USER+606)

typedef struct _METERBOX_FULL
{
	LARGE_INTEGER Full;
} METERBOX_FULL;

typedef struct _METERBOX_POS
{
	LARGE_INTEGER Pos;
} METERBOX_POS;

typedef struct _METERBOX_INFO
{
	COLORREF crMeter;
	COLORREF crMeterWarning;
	COLORREF crBackground;
	COLORREF crMeterText;
	COLORREF crBackgroundText;
} METERBOX_INFO;

#define MTBN_CLICKED          0x1
#define MTBN_DBLCLK           0x2
