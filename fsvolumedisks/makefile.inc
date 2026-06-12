
!include ..\runtime_copy.inc

PostBuildSteps:
	mkdir $(_RUNTIME_PATH)
	xcopy /y /d $(_OUTPUT_PATH)\$(TARGETNAME).dll $(_RUNTIME_PATH)
	xcopy /y /d $(_OUTPUT_PATH)\$(TARGETNAME).pdb $(_RUNTIME_PATH)
	xcopy /y /d $(_OUTPUT_PATH)\$(TARGETNAME).lib $(_LIB_STORE_PATH)
