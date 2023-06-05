
#include "blade.h"

static bool objfn_object_gettype(VMState* vm, int argcount, Value* args)
{
    fprintf(stderr, "call to __gettype()\n");
    return bl_value_returnnil(vm, args);
}

void bl_state_initobjectmethods(VMState* vm)
{
    bl_class_defnativemethod(vm, vm->classobjobject, "__gettype", objfn_object_gettype);

}


