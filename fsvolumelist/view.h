#pragma once

#include "pagewbdbase.h"
#include "page_volumehome.h"
#include "page_volumebasicinfo.h"
#include "page_physicaldriveinfo.h"
#include "page_disklayout.h"
#include "page_storagedevice.h"
#include "page_mounteddevice.h"
#include "page_volumelist.h"
#include "page_physicaldrivelist.h"
#include "page_shadowcopy.h"
#include "page_dosdrive.h"
#include "page_statistics.h"
#include "page_simplehexdump.h"
#include "page_filterdriver.h"

HRESULT ViewBase_CreateObject(HINSTANCE hInstance,IViewBaseWindow **pObject);
