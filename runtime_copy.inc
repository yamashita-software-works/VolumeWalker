!if $(AMD64)
!if $(FREEBUILD)
_OUTPUT_PATH=.\objfre_win7_amd64\amd64
_RUNTIME_PATH=..\bin\ddk\x64\fre
_LIB_STORE_PATH=..\lib\x64\libfre
!else
_OUTPUT_PATH=.\objchk_win7_amd64\amd64
_RUNTIME_PATH=..\bin\ddk\x64\chk
_LIB_STORE_PATH=..\lib\x64\libchk
!endif
!else
!if $(FREEBUILD)
_OUTPUT_PATH=.\objfre_win7_x86\i386
_RUNTIME_PATH=..\bin\ddk\x86\fre
_LIB_STORE_PATH=..\lib\x86\libfre
!else
_OUTPUT_PATH=.\objchk_win7_x86\i386
_RUNTIME_PATH=..\bin\ddk\x86\chk
_LIB_STORE_PATH=..\lib\x86\libchk
!endif
!endif
