#pragma once
//****************************************************************************
//*                                                                          *
//*  vdshelp.h                                                               *
//*                                                                          *
//*  VDS Managemant Helper                                                   *
//*                                                                          *
//*  Author:  YAMASHITA Katsuhiro                                            *
//*                                                                          *
//*  History: 2025-09-26                                                     *
//*                                                                          *
//****************************************************************************
//
//  Copyright (C) YAMASHITA Katsuhiro. All rights reserved.
//  Licensed under the MIT License.
//
#include <vds.h>
#include "dparray.h"
#include "dsarray.h"
#include "debugprint.h"
#include "guidtostring.h"

enum
{
	VDSBusTypeVirtual           = 0xe,
//	VDSBusTypeFileBackedVirtual = 0xf,
	VDSBusTypeSpaces            = 0x10,
	VDSBusTypeNVMe              = 0x11,
	VDSBusTypeScm               = 0x12,
	VDSBusTypeUfs               = 0x13,
//	VDSBusTypeMaxReserved       = 0x7f
};

enum
{
	VDS_FST_CSVFS = (VDS_FST_EXFAT + 1),
	VDS_FST_REFS,
};

enum
{
	VDS_DF_REFS_NOT_SUPPORTED = 0x10000,
};

enum
{
	VDS_VF_REFS_NOT_SUPPORTED = 0x800000,
	VDS_VF_BACKS_BOOT_VOLUME = 0x1000000,
};

LPCWSTR VDS_FileSystemTypeText(VDS_FILE_SYSTEM_TYPE FsType);
LPCWSTR VDS_GetVolumeTypeText(VDS_VOLUME_TYPE VolumeType);
LPCWSTR VDS_GetDiskExtentTypeDescription(VDS_DISK_EXTENT_TYPE VolumeType,int descTextType);
LPCWSTR VDS_GetStatusText(VDS_DISK_STATUS status);
LPCWSTR VDS_GetPartitionStyleText(VDS_PARTITION_STYLE style);
LPCWSTR VDS_GetBusTypeText(VDS_STORAGE_BUS_TYPE BusType);
LPCWSTR VDS_GetVolumeStatusText(VDS_VOLUME_STATUS status);

LPCWSTR
VDS_GetDiskFlagText(
	__in VDS_DISK_FLAG flags,
	__in_opt LPWSTR pszFriendlyName,
	__in_opt int cchFriendlyName,
	__in_opt LPWSTR pszFlagName,
	__in_opt int cchFlagName
	);

LPCWSTR
VDS_GetVolumeFlagText(
	__in VDS_VOLUME_FLAG flags,
	__in_opt LPWSTR pszFriendlyName,
	__in_opt int cchFriendlyName,
	__in_opt LPWSTR pszFlagName,
	__in_opt int cchFlagName
	);

struct CVDSItem
{
	CVDSItem()
	{
	}
	~CVDSItem()
	{
	}
};

struct CVDSProviderItem : public CVDSItem
{
	VDS_PROVIDER_PROP Prop;

	CVDSProviderItem(int cItems=32)
	{
		memset(&Prop,0,sizeof(Prop));
	}

	~CVDSProviderItem()
	{
		CoTaskMemFree( Prop.pwszName );
		CoTaskMemFree( Prop.pwszVersion );
	}

	HRESULT SetProp(VDS_PROVIDER_PROP& propProvider)
	{
		Prop = propProvider;
#if 0
		Prop.pwszName    = _CoTaskMemStrDup(propProvider.pwszName);
		Prop.pwszVersion = _CoTaskMemStrDup(propProvider.pwszVersion);
#else
		Prop.pwszName    = propProvider.pwszName;
		Prop.pwszVersion = propProvider.pwszVersion;
#endif
		return S_OK;
	}
};

struct CVDSDiskItem;
struct CVDSVolumeItem;

struct CVDSPackItem : public CVDSItem
{
	VDS_PACK_PROP Prop;
	PtrArray<CVDSDiskItem *>   Disks;
	PtrArray<CVDSVolumeItem *> Volumes;

	CVDSPackItem(int cItems=32)
	{
		memset(&Prop,0,sizeof(Prop));
		Disks.Create(cItems);
		Volumes.Create(cItems);
	}

	~CVDSPackItem()
	{
		CoTaskMemFree(Prop.pwszName);
		Disks.DeleteAll();
		Volumes.DeleteAll();
	}

	HRESULT SetProp(VDS_PACK_PROP& propPack)
	{
		Prop = propPack;
		return S_OK;
	}

	HRESULT AddDisk(CVDSDiskItem *pDisk)
	{
		Disks.Add(pDisk);
		return S_OK;
	}

	HRESULT AddVolume(CVDSVolumeItem *pVolume)
	{
		Volumes.Add(pVolume);
		return S_OK;
	}
};

struct CVDSDiskItem : public CVDSItem
{
	VDS_DISK_PROP Prop;
	VDS_DISK_EXTENT *Extents;
	LONG ExtentsCount;
	CVDSPackItem *PackItem;

	CVDSDiskItem()
	{
		memset(&Prop,0,sizeof(Prop));
		PackItem = nullptr;
		Extents = nullptr;
		ExtentsCount = 0;
	}

	~CVDSDiskItem()
	{
		CoTaskMemFree( Prop.pwszName );
		CoTaskMemFree( Prop.pwszFriendlyName );
		CoTaskMemFree( Prop.pwszDevicePath );
		CoTaskMemFree( Prop.pwszDiskAddress );
		CoTaskMemFree( Prop.pwszAdaptorName );

		CoTaskMemFree( Extents );
	}

	HRESULT SetProp(VDS_DISK_PROP& propPack)
	{
		ASSERT(Prop.pwszDevicePath == nullptr);
		Prop = propPack;
		return S_OK;
	}

	HRESULT SetPackItem(CVDSPackItem *pItem)
	{
		ASSERT(PackItem == nullptr);
		PackItem = pItem;
		return S_OK;
	}

	HRESULT SetExtents(VDS_DISK_EXTENT *pExtentPtrArray,LONG cExtentPtrArray)
	{
		Extents = pExtentPtrArray;
		ExtentsCount = cExtentPtrArray;
		return S_OK;
	}
};

struct CVDSVolumeItem : public CVDSItem
{
	VDS_VOLUME_PROP Prop;
	CVDSPackItem *PackItem;

	typedef struct {
		VDS_VOLUME_PLEX_PROP Prop;
		VDS_DISK_EXTENT *pExtents;
		LONG cExtents;
	} PLEX_EXTENT;

	DSArray<PLEX_EXTENT> PlexArray;

	CVDSVolumeItem()
	{
		memset(&Prop,0,sizeof(Prop));
		PackItem = nullptr;
		PlexArray.Create(16);
	}

	~CVDSVolumeItem()
	{
		CoTaskMemFree( Prop.pwszName );
		FreePlaxAll();
	}

	HRESULT SetProp(VDS_VOLUME_PROP& propPack)
	{
		ASSERT(Prop.pwszName == nullptr);
		Prop = propPack;
		return S_OK;
	}

	HRESULT SetPackItem(CVDSPackItem *pItem)
	{
		ASSERT(PackItem == nullptr);
		PackItem = pItem;
		return S_OK;
	}

	HRESULT AddPlex(VDS_VOLUME_PLEX_PROP Prop,VDS_DISK_EXTENT *pExtents,LONG cExtents)
	{
		PLEX_EXTENT plex = {0};
		plex.Prop = Prop;
		plex.pExtents = pExtents;
		plex.cExtents = cExtents;
		PlexArray.Add( &plex );
		return S_OK;
	}

	HRESULT FreePlaxAll()
	{
		int i,c;
		c = PlexArray.GetCount();
		if( c > 0 )
		{
			for(i = 0; i < c; i++)
			{
				PLEX_EXTENT *plex = PlexArray.GetItemPtr(i);;
				CoTaskMemFree(plex->pExtents);
			}
			PlexArray.DeleteAll();
		}

		ASSERT( PlexArray.GetCount() == 0 );

		return S_OK;
	}
};

class CVDSDataManager
{
    IVdsServiceLoader* m_pLoader;
    IVdsService*       m_pService;

public:
	PtrArray<CVDSPackItem*>     m_pVdsPacks;
	PtrArray<CVDSDiskItem*>     m_pVdsDisks;
	PtrArray<CVDSVolumeItem*>   m_pVdsVolumes;
	PtrArray<CVDSProviderItem*> m_pVdsProviders;
	VDS_DRIVE_LETTER_PROP       m_dosDriveLetters[26];

	CVDSDataManager()
	{
		m_pLoader = nullptr;
		m_pService = nullptr;
	}

	~CVDSDataManager()
	{
		m_pVdsPacks.DestroyCallback(&DeletePackItemCallback,nullptr);
		m_pVdsDisks.DestroyCallback(&DeleteDiskItemCallback,nullptr);
		m_pVdsVolumes.DestroyCallback(&DeleteVolumeItemCallback,nullptr);
		m_pVdsProviders.DestroyCallback(&DeleteProviderItemCallback,nullptr);

		if( m_pService )
			m_pService->Release();
	}

	HRESULT Initialize()
	{
		m_pVdsPacks.Create(8);
		m_pVdsDisks.Create(16);
		m_pVdsVolumes.Create(32);
		m_pVdsProviders.Create(32);

		HRESULT hr;
	    IVdsServiceLoader* pLoader = nullptr;
	    hr = CoCreateInstance(CLSID_VdsLoader, nullptr, CLSCTX_LOCAL_SERVER,
                          IID_IVdsServiceLoader, (void**)&pLoader);
		if( hr == S_OK )
		{
		    hr = pLoader->LoadService(nullptr, &m_pService);

			pLoader->Release();

			if( hr == S_OK )
			{
				hr = m_pService->WaitForServiceReady();

				if( hr == S_OK )
				{
					m_pService->QueryDriveLetters(L'A',_countof(m_dosDriveLetters),m_dosDriveLetters);
				}
			}
		}

		return hr;
	}

	HRESULT EnumVDSItems()
	{
		HRESULT hr;

	    IEnumVdsObject* pEnum = nullptr;
		hr = m_pService->QueryProviders(VDS_QUERY_SOFTWARE_PROVIDERS|VDS_QUERY_VIRTUALDISK_PROVIDERS, &pEnum);

	    if (SUCCEEDED(hr))
		{
			IUnknown* pUnk = nullptr;
			ULONG c;

			while( S_OK == pEnum->Next(1, &pUnk, &c) )
			{
	            IVdsProvider* pProvider = nullptr;
		        hr = pUnk->QueryInterface(IID_IVdsProvider, (void**)&pProvider);
			    pUnk->Release();

				VDS_PROVIDER_PROP propProvider;
				pProvider->GetProperties(&propProvider);

				CVDSProviderItem *pProviderItem = new CVDSProviderItem;
				pProviderItem->SetProp( propProvider );
				m_pVdsProviders.Add( pProviderItem );

				IVdsSwProvider* pSwProvider = nullptr;
	            if (SUCCEEDED(pProvider->QueryInterface(IID_IVdsSwProvider, (void**)&pSwProvider)))
				{
					EnumItems(pSwProvider);

					pSwProvider->Release();
				}

				IVdsVdProvider* pVdProvider = nullptr;
	            if (SUCCEEDED(pProvider->QueryInterface(IID_IVdsVdProvider, (void**)&pVdProvider)))
				{
					EnumVdItems(pProviderItem,pVdProvider);

					pVdProvider->Release();
				}

				pProvider->Release();
			}
			pEnum->Release();
		}

		return hr;
	}

private:
	HRESULT EnumItems(IVdsSwProvider* pSwProvider)
	{
		HRESULT hr;
		ULONG cFetched;

		IEnumVdsObject* pEnumPack = nullptr;
		if( SUCCEEDED( hr = pSwProvider->QueryPacks(&pEnumPack) ) )
		{
			//
			// Enum packs
			//
			IUnknown* pUnkVol = nullptr;
			while( S_OK == pEnumPack->Next(1, &pUnkVol, &cFetched) )
			{
				IVdsPack* pPack = nullptr;
				if( SUCCEEDED(pUnkVol->QueryInterface(IID_IVdsPack, (void**)&pPack)) )
				{
					CVDSPackItem *pvdsPack = new CVDSPackItem;

					VDS_PACK_PROP propPack;
					if( SUCCEEDED(pPack->GetProperties( &propPack )) )
					{
						pvdsPack->SetProp( propPack );
					}

					m_pVdsPacks.Add( pvdsPack );

					//
					// Enum disks in a pack
					//
					IEnumVdsObject* pEnumDisk = nullptr;
					hr = pPack->QueryDisks( &pEnumDisk );
					if( hr == S_OK )
					{
						IUnknown* pUnkVol = nullptr;
						while( S_OK == pEnumDisk->Next(1, &pUnkVol, &cFetched) )
						{
							CVDSDiskItem *pvdsDisk = new CVDSDiskItem;

							IVdsDisk* pDisk = nullptr;
							if( SUCCEEDED(pUnkVol->QueryInterface(IID_IVdsDisk, (void**)&pDisk)) )
							{
								VDS_DISK_PROP prop;

								if( SUCCEEDED(pDisk->GetProperties(&prop)) )
								{
									pvdsDisk->SetProp(prop);
									pvdsDisk->SetPackItem( pvdsPack );
								}

								VDS_DISK_EXTENT *pe;
								LONG ce;
								if( SUCCEEDED(pDisk->QueryExtents(&pe,&ce)) )
								{
									pvdsDisk->SetExtents( pe, ce );
								}

								pDisk->Release();
							}

							pUnkVol->Release();

							pvdsPack->AddDisk( pvdsDisk );

							m_pVdsDisks.Add( pvdsDisk );
						}
					}

					//
					// Enum volumes in a pack
					//
					IEnumVdsObject* pEnumVol = nullptr;
					hr = pPack->QueryVolumes( &pEnumVol );
					if( hr == S_OK )
					{
						IUnknown* pUnkVol = nullptr;
						while( S_OK == pEnumVol->Next(1, &pUnkVol, &cFetched) )
						{
							CVDSVolumeItem *pvdsVolume = new CVDSVolumeItem;

							IVdsVolume* pVolume = nullptr;
							if( SUCCEEDED(pUnkVol->QueryInterface(IID_IVdsVolume, (void**)&pVolume)) )
							{
								VDS_VOLUME_PROP prop;

								if (SUCCEEDED(pVolume->GetProperties(&prop)))
								{
									pvdsVolume->SetProp(prop);
									pvdsVolume->SetPackItem( pvdsPack );
								}
#if 0
								IVdsVolume2* pVolume2 = nullptr;
								if( SUCCEEDED(pUnkVol->QueryInterface(IID_IVdsVolume2, (void**)&pVolume2)) )
								{
									VDS_VOLUME_PROP2 prop2;
									if( SUCCEEDED(pVolume2->GetProperties2(&prop2)) )
									{
										; // todo:
									}
									pVolume2->Release();
								}
#endif
								IVdsVolumePlex* pVolumePlex = nullptr;
								IEnumVdsObject* pEnumPlexes = nullptr;
								if( SUCCEEDED(pVolume->QueryPlexes(&pEnumPlexes)) )
								{
									IUnknown* pUnkVol = nullptr;
									ULONG c;
									while( S_OK == pEnumPlexes->Next(1, &pUnkVol, &c) )
									{
										if( SUCCEEDED(pUnkVol->QueryInterface(IID_IVdsVolumePlex, (void**)&pVolumePlex)) )
										{
											VDS_VOLUME_PLEX_PROP prop;
											pVolumePlex->GetProperties( &prop );
								
											VDS_DISK_EXTENT *pe;
											LONG ce;
											pVolumePlex->QueryExtents( &pe, &ce );
								
											pvdsVolume->AddPlex( prop, pe, ce );
								
											pVolumePlex->Release();
										}
									}
									pEnumPlexes->Release();
								}
								pVolume->Release();
							}

							pUnkVol->Release();

							pvdsPack->AddVolume( pvdsVolume );

							m_pVdsVolumes.Add( pvdsVolume );
						}
					}
					else
					{
						ASSERT(FALSE);
					}

					// Release Pack Object
					pPack->Release();
				}
				pUnkVol->Release();
			}
			pEnumPack->Release();
		}

		return hr;
	}

	HRESULT EnumVdItems(CVDSProviderItem* pProvider,IVdsVdProvider* pVdProvider)
	{
		HRESULT hr;
		ULONG cFetched;

		IEnumVdsObject* pEnumVds = nullptr;
		if( SUCCEEDED( hr = pVdProvider->QueryVDisks(&pEnumVds) ) )
		{
			IUnknown* pUnkVol = nullptr;
			while( S_OK == pEnumVds->Next(1, &pUnkVol, &cFetched) )
			{
				IVdsVDisk * pVDisk = nullptr;
				if( SUCCEEDED(pUnkVol->QueryInterface(IID_IVdsVDisk, (void**)&pVDisk)) )
				{
					IVdsDisk *pDisk = nullptr;
					pVdProvider->GetDiskFromVDisk(pVDisk,&pDisk);
					if( pDisk )
					{
						VDS_DISK_PROP dp;
						pDisk->GetProperties(&dp);
						pDisk->Release();
					}

					VDS_VDISK_PROPERTIES vvp = {0};
					pVDisk->GetProperties( &vvp );
					pVDisk->Release();
				}
			}
			pEnumVds->Release();
			hr = S_OK;
		}

		return hr;
	}

	static int CALLBACK DeletePackItemCallback(__in_opt void *ptr, __in_opt void *pData)
	{
		delete (CVDSPackItem *)ptr;
		return 1;
	}

	static int CALLBACK DeleteDiskItemCallback(__in_opt void *ptr, __in_opt void *pData)
	{
		delete (CVDSDiskItem *)ptr;
		return 1;
	}

	static int CALLBACK DeleteVolumeItemCallback(__in_opt void *ptr, __in_opt void *pData)
	{
		delete (CVDSVolumeItem *)ptr;
		return 1;
	}

	static int CALLBACK DeleteProviderItemCallback(__in_opt void *ptr, __in_opt void *pData)
	{
		delete (CVDSProviderItem *)ptr;
		return 1;
	}
};
