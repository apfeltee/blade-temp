
#include "blade.h"

#define ENFORCE_VALID_DICT_KEY(name, index) \
    EXCLUDE_ARG_TYPE(name, bl_value_isarray, index); \
    EXCLUDE_ARG_TYPE(name, bl_value_isdict, index); \
    EXCLUDE_ARG_TYPE(name, bl_value_isfile, index);



ObjDict* bl_object_makedict(VMState* vm)
{
    ObjDict* dict = (ObjDict*)bl_object_allocobject(vm, sizeof(ObjDict), OBJ_DICT);
    bl_valarray_init(&dict->names);
    bl_hashtable_init(&dict->items);
    return dict;
}


ObjPointer* bl_dict_makeptr(VMState* vm, void* pointer)
{
    ObjPointer* ptr = (ObjPointer*)bl_object_allocobject(vm, sizeof(ObjPointer), OBJ_PTR);
    ptr->pointer = pointer;
    ptr->name = "<void *>";
    ptr->fnptrfree = NULL;
    return ptr;
}

void bl_dict_addentry(VMState* vm, ObjDict* dict, Value key, Value value)
{
    bl_valarray_push(vm, &dict->names, key);
    bl_hashtable_set(vm, &dict->items, key, value);
}

bool bl_dict_getentry(ObjDict* dict, Value key, Value* value)
{
    /* // this will be easier to search than the entire tables
  // if the key doesn't exist.
  if (dict->names.count < (int)sizeof(uint8_t)) {
    int i;
    bool found = false;
    for (i = 0; i < dict->names.count; i++) {
      if (bl_value_valuesequal(dict->names.values[i], key)) {
        found = true;
        break;
      }
    }
    if (!found)
      return false;
  } */
    return bl_hashtable_get(&dict->items, key, value);
}

bool bl_dict_setentry(VMState* vm, ObjDict* dict, Value key, Value value)
{
    Value tempvalue;
    if(!bl_hashtable_get(&dict->items, key, &tempvalue))
    {
        bl_valarray_push(vm, &dict->names, key);// add key if it doesn't exist.
    }
    return bl_hashtable_set(vm, &dict->items, key, value);
}


static bool objfn_dict_length(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(dictionary.length, 0);
    RETURN_NUMBER(AS_DICT(METHOD_OBJECT)->names.count);
}

static bool objfn_dict_add(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(add, 2);
    ENFORCE_VALID_DICT_KEY(add, 0);
    ObjDict* dict = AS_DICT(METHOD_OBJECT);
    Value tempvalue;
    if(bl_hashtable_get(&dict->items, args[0], &tempvalue))
    {
        RETURN_ERROR("duplicate key %s at add()", bl_value_tostring(vm, args[0]));
    }
    bl_dict_addentry(vm, dict, args[0], args[1]);
    return bl_value_returnempty(vm, args);
    ;
}

static bool objfn_dict_set(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(set, 2);
    ENFORCE_VALID_DICT_KEY(set, 0);
    ObjDict* dict = AS_DICT(METHOD_OBJECT);
    Value value;
    if(!bl_hashtable_get(&dict->items, args[0], &value))
    {
        bl_dict_addentry(vm, dict, args[0], args[1]);
    }
    else
    {
        bl_dict_setentry(vm, dict, args[0], args[1]);
    }
    return bl_value_returnempty(vm, args);
    ;
}

static bool objfn_dict_clear(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(dict, 0);
    ObjDict* dict = AS_DICT(METHOD_OBJECT);
    bl_valarray_free(vm, &dict->names);
    bl_hashtable_free(vm, &dict->items);
    return bl_value_returnempty(vm, args);
    ;
}

static bool objfn_dict_clone(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(clone, 0);
    ObjDict* dict = AS_DICT(METHOD_OBJECT);
    ObjDict* ndict = (ObjDict*)bl_mem_gcprotect(vm, (Object*)bl_object_makedict(vm));
    bl_hashtable_addall(vm, &dict->items, &ndict->items);
    for(int i = 0; i < dict->names.count; i++)
    {
        bl_valarray_push(vm, &ndict->names, dict->names.values[i]);
    }
    RETURN_OBJ(ndict);
}

static bool objfn_dict_compact(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(compact, 0);
    ObjDict* dict = AS_DICT(METHOD_OBJECT);
    ObjDict* ndict = (ObjDict*)bl_mem_gcprotect(vm, (Object*)bl_object_makedict(vm));
    for(int i = 0; i < dict->names.count; i++)
    {
        Value tmpvalue;
        bl_hashtable_get(&dict->items, dict->names.values[i], &tmpvalue);
        if(!bl_value_valuesequal(tmpvalue, NIL_VAL))
        {
            bl_dict_addentry(vm, ndict, dict->names.values[i], tmpvalue);
        }
    }
    RETURN_OBJ(ndict);
}

static bool objfn_dict_contains(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(contains, 1);
    ENFORCE_VALID_DICT_KEY(contains, 0);
    ObjDict* dict = AS_DICT(METHOD_OBJECT);
    Value value;
    RETURN_BOOL(bl_hashtable_get(&dict->items, args[0], &value));
}

static bool objfn_dict_extend(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(extend, 1);
    ENFORCE_ARG_TYPE(extend, 0, bl_value_isdict);
    ObjDict* dict = AS_DICT(METHOD_OBJECT);
    ObjDict* dictcpy = AS_DICT(args[0]);
    for(int i = 0; i < dictcpy->names.count; i++)
    {
        bl_valarray_push(vm, &dict->names, dictcpy->names.values[i]);
    }
    bl_hashtable_addall(vm, &dictcpy->items, &dict->items);
    return bl_value_returnempty(vm, args);
    ;
}

static bool objfn_dict_get(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_RANGE(get, 1, 2);
    ENFORCE_VALID_DICT_KEY(get, 0);
    ObjDict* dict = AS_DICT(METHOD_OBJECT);
    Value value;
    if(!bl_dict_getentry(dict, args[0], &value))
    {
        if(argcount == 1)
        {
            return bl_value_returnnil(vm, args);
        }
        else
        {
            RETURN_VALUE(args[1]);// return default
        }
    }
    RETURN_VALUE(value);
}

static bool objfn_dict_keys(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(keys, 0);
    ObjDict* dict = AS_DICT(METHOD_OBJECT);
    ObjArray* list = (ObjArray*)bl_mem_gcprotect(vm, (Object*)bl_object_makelist(vm));
    for(int i = 0; i < dict->names.count; i++)
    {
        bl_array_push(vm, list, dict->names.values[i]);
    }
    RETURN_OBJ(list);
}

static bool objfn_dict_values(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(values, 0);
    ObjDict* dict = AS_DICT(METHOD_OBJECT);
    ObjArray* list = (ObjArray*)bl_mem_gcprotect(vm, (Object*)bl_object_makelist(vm));
    for(int i = 0; i < dict->names.count; i++)
    {
        Value tmpvalue;
        bl_dict_getentry(dict, dict->names.values[i], &tmpvalue);
        bl_array_push(vm, list, tmpvalue);
    }
    RETURN_OBJ(list);
}

static bool objfn_dict_remove(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(remove, 1);
    ENFORCE_VALID_DICT_KEY(remove, 0);
    ObjDict* dict = AS_DICT(METHOD_OBJECT);
    Value value;
    if(bl_hashtable_get(&dict->items, args[0], &value))
    {
        bl_hashtable_delete(&dict->items, args[0]);
        int index = -1;
        for(int i = 0; i < dict->names.count; i++)
        {
            if(bl_value_valuesequal(dict->names.values[i], args[0]))
            {
                index = i;
                break;
            }
        }
        for(int i = index; i < dict->names.count; i++)
        {
            dict->names.values[i] = dict->names.values[i + 1];
        }
        dict->names.count--;
        RETURN_VALUE(value);
    }
    return bl_value_returnnil(vm, args);
}

static bool objfn_dict_isempty(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(isempty, 0);
    RETURN_BOOL(AS_DICT(METHOD_OBJECT)->names.count == 0);
}

static bool objfn_dict_findkey(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(findkey, 1);
    RETURN_VALUE(bl_hashtable_findkey(&AS_DICT(METHOD_OBJECT)->items, args[0]));
}

static bool objfn_dict_tolist(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(to_list, 0);
    ObjDict* dict = AS_DICT(METHOD_OBJECT);
    ObjArray* namelist = (ObjArray*)bl_mem_gcprotect(vm, (Object*)bl_object_makelist(vm));
    ObjArray* valuelist = (ObjArray*)bl_mem_gcprotect(vm, (Object*)bl_object_makelist(vm));
    for(int i = 0; i < dict->names.count; i++)
    {
        bl_array_push(vm, namelist, dict->names.values[i]);
        Value value;
        if(bl_hashtable_get(&dict->items, dict->names.values[i], &value))
        {
            bl_array_push(vm, valuelist, value);
        }
        else
        {// theoretically impossible
            bl_array_push(vm, valuelist, NIL_VAL);
        }
    }
    ObjArray* list = (ObjArray*)bl_mem_gcprotect(vm, (Object*)bl_object_makelist(vm));
    bl_array_push(vm, list, OBJ_VAL(namelist));
    bl_array_push(vm, list, OBJ_VAL(valuelist));
    RETURN_OBJ(list);
}

static bool objfn_dict_iter(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(__iter__, 1);
    ObjDict* dict = AS_DICT(METHOD_OBJECT);
    Value result;
    if(bl_hashtable_get(&dict->items, args[0], &result))
    {
        RETURN_VALUE(result);
    }
    return bl_value_returnnil(vm, args);
}

static bool objfn_dict_itern(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(__itern__, 1);
    ObjDict* dict = AS_DICT(METHOD_OBJECT);
    if(bl_value_isnil(args[0]))
    {
        if(dict->names.count == 0)
            RETURN_FALSE;
        RETURN_VALUE(dict->names.values[0]);
    }
    for(int i = 0; i < dict->names.count; i++)
    {
        if(bl_value_valuesequal(args[0], dict->names.values[i]) && (i + 1) < dict->names.count)
        {
            RETURN_VALUE(dict->names.values[i + 1]);
        }
    }
    return bl_value_returnnil(vm, args);
}

void bl_state_initdictmethods(VMState* vm)
{
    // dictionary methods
    bl_object_defnativemethod(vm, &vm->classobjdict->methods, "length", objfn_dict_length);
    bl_object_defnativemethod(vm, &vm->classobjdict->methods, "add", objfn_dict_add);
    bl_object_defnativemethod(vm, &vm->classobjdict->methods, "set", objfn_dict_set);
    bl_object_defnativemethod(vm, &vm->classobjdict->methods, "clear", objfn_dict_clear);
    bl_object_defnativemethod(vm, &vm->classobjdict->methods, "clone", objfn_dict_clone);
    bl_object_defnativemethod(vm, &vm->classobjdict->methods, "compact", objfn_dict_compact);
    bl_object_defnativemethod(vm, &vm->classobjdict->methods, "contains", objfn_dict_contains);
    bl_object_defnativemethod(vm, &vm->classobjdict->methods, "extend", objfn_dict_extend);
    bl_object_defnativemethod(vm, &vm->classobjdict->methods, "get", objfn_dict_get);
    bl_object_defnativemethod(vm, &vm->classobjdict->methods, "keys", objfn_dict_keys);
    bl_object_defnativemethod(vm, &vm->classobjdict->methods, "values", objfn_dict_values);
    bl_object_defnativemethod(vm, &vm->classobjdict->methods, "remove", objfn_dict_remove);
    bl_object_defnativemethod(vm, &vm->classobjdict->methods, "isempty", objfn_dict_isempty);
    bl_object_defnativemethod(vm, &vm->classobjdict->methods, "findkey", objfn_dict_findkey);
    bl_object_defnativemethod(vm, &vm->classobjdict->methods, "to_list", objfn_dict_tolist);
    bl_object_defnativemethod(vm, &vm->classobjdict->methods, "@iter", objfn_dict_iter);
    bl_object_defnativemethod(vm, &vm->classobjdict->methods, "@itern", objfn_dict_itern);
}

