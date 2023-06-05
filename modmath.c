
#include "blade.h"

static bool objfn_math_round(VMState* vm, int argcount, Value* args)
{
    return bl_value_returnnil(vm, args);
}

static bool objfn_math_floor(VMState* vm, int argcount, Value* args)
{
    return bl_value_returnnil(vm, args);
}

void bl_state_initmathmethods(VMState* vm)
{
    bl_class_defstaticnativemethod(vm, vm->classobjmath, "round", objfn_math_round);
    bl_class_defstaticnativemethod(vm, vm->classobjmath, "floor", objfn_math_floor);

}



