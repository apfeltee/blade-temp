
#include <foo.h>

extern RegModule* bl_modload_base64(VMState* vm);
extern RegModule* bl_modload_date(VMState* vm);
extern RegModule* bl_modload_io(VMState* vm);
extern RegModule* bl_modload_math(VMState* vm);
extern RegModule* bl_modload_os(VMState* vm);
extern RegModule* bl_modload_socket(VMState* vm);
extern RegModule* bl_modload_hash(VMState* vm);
extern RegModule* bl_modload_json(VMState* vm);
extern RegModule* bl_modload_sqlite(VMState* vm);
extern RegModule* bl_modload_reflect(VMState* vm);
extern RegModule* bl_modload_array(VMState* vm);
extern RegModule* bl_modload_process(VMState* vm);
extern RegModule* bl_modload_struct(VMState* vm);
    #define BLADE_PATH_SEPARATOR /* unix like */ "/"
    #if defined(__linux__) || defined(__CYGWIN__) || defined(__MINGW32_MAJOR_VERSION)
        #define PROC_SELF_EXE "/proc/self/exe"
    #endif
// returns the number of bytes contained in a unicode character
int utf8_number_bytes(int value)
{
    if(value < 0)
    {
        return -1;
    }
    if(value <= 0x7f)
        return 1;
    if(value <= 0x7ff)
        return 2;
    if(value <= 0xffff)
        return 3;
    if(value <= 0x10ffff)
        return 4;
    return 0;
}

