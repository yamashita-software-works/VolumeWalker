
!if $(AMD64)
_ARCH_=x64
!else
_ARCH_=x86
!endif

_OUTPUTLIBPATH=..\lib\$(_ARCH_)\lib$(_BUILDTYPE)

PostBuildSteps: $(TARGET)
	md $(_OUTPUTLIBPATH)
	copy obj$(BUILD_ALT_DIR)\$(TARGET_DIRECTORY)\$(TARGETNAME).lib $(_OUTPUTLIBPATH)