#ifndef _NTPATHCOMPONENT_
#define _NTPATHCOMPONENT_

#define DOS_MAX_COMPONENT_LENGTH  255
#define DOS_MAX_COMPONENT_BYTES   (DOS_MAX_COMPONENT_LENGTH * sizeof(WCHAR))
#define DOS_MAX_PATH_LENGTH       (DOS_MAX_COMPONENT_LENGTH + 5)
#define DOS_MAX_PATH_BYTES        (DOS_MAX_PATH_LENGTH * sizeof(WCHAR))
#define WIN32_MAX_PATH            DOS_MAX_PATH_LENGTH
#define WIN32_MAX_PATH_BYTES      (WIN32_MAX_PATH * sizeof(WCHAR))

#define ANSI_NULL ((CHAR)0)     
#define UNICODE_NULL ((WCHAR)0) 
#ifndef UNICODE_STRING_MAX_BYTES
#define UNICODE_STRING_MAX_BYTES ((USHORT) 65534) 
#endif
#define UNICODE_STRING_MAX_CHARS (32767) 

#define PATH_BUFFER_BYTES   (UNICODE_STRING_MAX_BYTES + sizeof(WCHAR))
#define PATH_BUFFER_LENGTH  (UNICODE_STRING_MAX_CHARS)

#define IS_RELATIVE_DIR_NAME_WITH_UNICODE_SIZE(path,size) \
            ((path[0] == L'.' && size == sizeof(WCHAR)) || \
            (path[0] == L'.' && path[1] == L'.' && (size == (sizeof(WCHAR)*2))))

#define WCHAR_LENGTH(u) ((u) / sizeof(WCHAR))
#define WCHAR_BYTES(w) ((w) * sizeof(WCHAR))
#define WCHAR_CHARS(u) WCHAR_LENGTH(u)

#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE ((HANDLE)-1)
#endif

#define _NT_MAX_PATH_LENGTH                    UNICODE_STRING_MAX_CHARS
#define _NT_MAX_ALTERNATE_STREAM_NAME_LENGTH   (260)    // ex) ":nnn...n"
#define _NT_MAX_ALTERNATE_STREAM_TYPE_LENGTH   (8)      // ex) ":$DATA"
#define _NT_MAX_VOLUME_LENGTH                  (80)     // \\?\Volume{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}

#define _NT_PATH_FULL_LENGTH (_NT_MAX_VOLUME_LENGTH \
						+_NT_MAX_PATH_LENGTH \
						+_NT_MAX_ALTERNATE_STREAM_NAME_LENGTH \
						+_NT_MAX_ALTERNATE_STREAM_TYPE_LENGTH )

#define _NT_PATH_FULL_LENGTH_BYTES (_NT_PATH_FULL_LENGTH * sizeof(WCHAR))

#define VOLUME_GUID_LENGTH (44) // "Volume{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}"
#define VOLUME_GUID_BYTES  (WCHAR_BYTE(VOLUME_GUID_LENGTH))

#endif
