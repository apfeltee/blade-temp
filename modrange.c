
#include "blade.h"

static bool objfn_range_lower(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(lower, 0);
    RETURN_NUMBER(AS_RANGE(METHOD_OBJECT)->lower);
}

static bool objfn_range_upper(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(upper, 0);
    RETURN_NUMBER(AS_RANGE(METHOD_OBJECT)->upper);
}

static bool objfn_range_iter(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(__iter__, 1);
    ENFORCE_ARG_TYPE(__iter__, 0, bl_value_isnumber);
    ObjRange* range = AS_RANGE(METHOD_OBJECT);
    int index = AS_NUMBER(args[0]);
    if(index >= 0 && index < range->range)
    {
        if(index == 0)
            RETURN_NUMBER(range->lower);
        RETURN_NUMBER(range->lower > range->upper ? --range->lower : ++range->lower);
    }
    return bl_value_returnnil(vm, args);
}

static bool objfn_range_itern(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(__itern__, 1);
    ObjRange* range = AS_RANGE(METHOD_OBJECT);
    if(bl_value_isnil(args[0]))
    {
        if(range->range == 0)
        {
            return bl_value_returnnil(vm, args);
        }
        RETURN_NUMBER(0);
    }
    if(!bl_value_isnumber(args[0]))
    {
        RETURN_ERROR("ranges are numerically indexed");
    }
    int index = (int)AS_NUMBER(args[0]) + 1;
    if(index < range->range)
    {
        RETURN_NUMBER(index);
    }
    return bl_value_returnnil(vm, args);
}

void bl_state_initrangemethods(VMState* vm)
{
    // range
    bl_class_defnativemethod(vm, vm->classobjrange, "lower", objfn_range_lower);
    bl_class_defnativemethod(vm, vm->classobjrange, "upper", objfn_range_upper);
    bl_class_defnativemethod(vm, vm->classobjrange, "@iter", objfn_range_iter);
    bl_class_defnativemethod(vm, vm->classobjrange, "@itern", objfn_range_itern);
}
