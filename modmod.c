
#include "blade.h"

ObjModule* bl_object_makemodule(VMState* vm, char* name, char* file)
{
    ObjModule* module = (ObjModule*)bl_object_allocobject(vm, sizeof(ObjModule), OBJ_MODULE);
    bl_hashtable_init(&module->values);
    module->name = name;
    module->file = file;
    module->unloader = NULL;
    module->preloader = NULL;
    module->handle = NULL;
    module->imported = false;
    return module;
}


