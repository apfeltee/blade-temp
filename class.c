
#include "blade.h"

void bl_class_defhashtabfield(VMState* vm, ObjClass* klass, HashTable* tbl, ObjString* name, Object* objval)
{
    Value vkey;
    Value vval;
    (void)klass;
    bl_vm_pushvalue(vm, OBJ_VAL(name));
    bl_vm_pushvalue(vm, OBJ_VAL(objval));
    vkey = vm->stack[0];
    vval = vm->stack[1];
    bl_hashtable_set(vm, tbl, vkey, vval);
    bl_vm_popvaluen(vm, 2);
}

void bl_class_defuserhashtabfield(VMState* vm, ObjClass* klass, HashTable* tbl, const char* name, Object* objval)
{
    ObjString* oname;
    oname = bl_string_copystringlen(vm, name, strlen(name));
    bl_class_defhashtabfield(vm, klass, tbl, oname, objval);
}


void bl_class_defnativemethod(VMState* vm, ObjClass* klass, const char* name, NativeCallbackFunc function)
{
    Object* oval;
    oval = (Object*)bl_object_makenativefunction(vm, function, name);
    bl_class_defuserhashtabfield(vm, klass, &klass->methods, name, oval);
}

void bl_class_defnativefield(VMState* vm, ObjClass* klass, const char* name, NativeCallbackFunc function)
{
    Object* oval;
    oval = (Object*)bl_object_makenativefunction(vm, function, name);
    bl_class_defuserhashtabfield(vm, klass, &klass->properties, name, oval);
}


void bl_class_defstaticnativemethod(VMState* vm, ObjClass* klass, const char* name, NativeCallbackFunc function)
{
    Object* oval;
    oval = (Object*)bl_object_makenativefunction(vm, function, name);
    bl_class_defuserhashtabfield(vm, klass, &klass->staticproperties, name, oval);
}







