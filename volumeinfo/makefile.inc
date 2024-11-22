
!include ..\runtime_copy.inc

PostBuildSteps:
	mkdir $(_RUNTIME_PATH)
	xcopy /y /d $(_OUTPUT_PATH)\$(TARGETNAME).exe $(_RUNTIME_PATH)
	xcopy /y /d $(_OUTPUT_PATH)\$(TARGETNAME).pdb $(_RUNTIME_PATH)
