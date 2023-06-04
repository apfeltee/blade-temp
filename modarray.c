
#include "blade.h"


ObjArray* bl_object_makelist(VMState* vm)
{
    ObjArray* list = (ObjArray*)bl_object_allocobject(vm, sizeof(ObjArray), OBJ_ARRAY);
    bl_valarray_init(&list->items);
    return list;
}


void bl_array_push(VMState* vm, ObjArray* list, Value value)
{
    bl_valarray_push(vm, &list->items, value);
}

void array_free(void* data)
{
    if(data)
    {
        free(data);
    }
}

ObjPointer* new_array(VMState* vm, DynArray* array)
{
    ObjPointer* ptr = (ObjPointer*)bl_mem_gcprotect(vm, (Object*)bl_dict_makeptr(vm, array));
    ptr->fnptrfree = &array_free;
    return ptr;
}

ObjArray* bl_array_copy(VMState* vm, ObjArray* list, int start, int length)
{
    int i;
    ObjArray* nl;
    (void)start;
    (void)length;
    nl = (ObjArray*)bl_mem_gcprotect(vm, (Object*)bl_object_makelist(vm));
    for(i = 0; i < list->items.count; i++)
    {
        bl_array_push(vm, nl, list->items.values[i]);
    }
    return nl;
}


//--------- COMMON STARTS -------------------------
bool modfn_array_length(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(length, 1);
    ENFORCE_ARG_TYPE(length, 0, bl_value_ispointer);
    ObjPointer* ptr = AS_PTR(args[0]);
    RETURN_NUMBER(((DynArray*)ptr->pointer)->length);
}

bool modfn_array_first(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(first, 1);
    ENFORCE_ARG_TYPE(first, 0, bl_value_ispointer);
    RETURN_NUMBER(((double*)((DynArray*)AS_PTR(args[0])->pointer)->buffer)[0]);
}

bool modfn_array_last(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(first, 1);
    ENFORCE_ARG_TYPE(first, 0, bl_value_ispointer);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    RETURN_NUMBER(((double*)array->buffer)[array->length - 1]);
}

bool modfn_array_extend(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(extend, 2);
    ENFORCE_ARG_TYPE(extend, 0, bl_value_ispointer);
    ENFORCE_ARG_TYPE(extend, 1, bl_value_ispointer);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    DynArray* array2 = (DynArray*)AS_PTR(args[1])->pointer;
    array->buffer = GROW_ARRAY(void, sizeof(void*), array->buffer, array->length, array->length + array2->length);
    memcpy(((unsigned char*)array->buffer) + array->length, array2->buffer, array2->length);
    array->length += array2->length;
    return bl_value_returnempty(vm, args);
    ;
}

bool modfn_array_tostring(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(to_string, 1);
    ENFORCE_ARG_TYPE(to_string, 0, bl_value_ispointer);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    RETURN_L_STRING((const char*)array->buffer, array->length);
}

bool modfn_array_itern_(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(@itern, 2);
    ENFORCE_ARG_TYPE(@itern, 0, bl_value_ispointer);
    ENFORCE_ARG_TYPE(@itern, 1, bl_value_isnumber);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    if(bl_value_isnil(args[1]))
    {
        if(array->length == 0)
            RETURN_FALSE;
        RETURN_NUMBER(0);
    }
    if(!bl_value_isnumber(args[1]))
    {
        RETURN_ERROR("Arrays are numerically indexed");
    }
    int index = AS_NUMBER(args[0]);
    if(index < array->length - 1)
    {
        RETURN_NUMBER((double)index + 1);
    }
    return bl_value_returnnil(vm, args);
}


static bool objfn_list_length(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(length, 0);
    RETURN_NUMBER(AS_LIST(METHOD_OBJECT)->items.count);
}

static bool objfn_list_append(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(append, 1);
    bl_array_push(vm, AS_LIST(METHOD_OBJECT), args[0]);
    return bl_value_returnempty(vm, args);
}

static bool objfn_list_clear(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(clear, 0);
    bl_valarray_free(vm, &AS_LIST(METHOD_OBJECT)->items);
    return bl_value_returnempty(vm, args);
}

static bool objfn_list_clone(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(clone, 0);
    ObjArray* list = AS_LIST(METHOD_OBJECT);
    RETURN_OBJ(bl_array_copy(vm, list, 0, list->items.count));
}

static bool objfn_list_count(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(count, 1);
    ObjArray* list = AS_LIST(METHOD_OBJECT);
    int count = 0;
    for(int i = 0; i < list->items.count; i++)
    {
        if(bl_value_valuesequal(list->items.values[i], args[0]))
        {
            count++;
        }
    }
    RETURN_NUMBER(count);
}

static bool objfn_list_extend(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(extend, 1);
    ENFORCE_ARG_TYPE(extend, 0, bl_value_isarray);
    ObjArray* list = AS_LIST(METHOD_OBJECT);
    ObjArray* list2 = AS_LIST(args[0]);
    for(int i = 0; i < list2->items.count; i++)
    {
        bl_array_push(vm, list, list2->items.values[i]);
    }
    return bl_value_returnempty(vm, args);
    ;
}

static bool objfn_list_indexof(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(indexof, 1);
    ObjArray* list = AS_LIST(METHOD_OBJECT);
    for(int i = 0; i < list->items.count; i++)
    {
        if(bl_value_valuesequal(list->items.values[i], args[0]))
        {
            RETURN_NUMBER(i);
        }
    }
    RETURN_NUMBER(-1);
}

static bool objfn_list_insert(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(insert, 2);
    ENFORCE_ARG_TYPE(insert, 1, bl_value_isnumber);
    ObjArray* list = AS_LIST(METHOD_OBJECT);
    int index = (int)AS_NUMBER(args[1]);
    bl_valarray_insert(vm, &list->items, args[0], index);
    return bl_value_returnempty(vm, args);
    ;
}

static bool objfn_list_pop(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(pop, 0);
    ObjArray* list = AS_LIST(METHOD_OBJECT);
    if(list->items.count > 0)
    {
        Value value = list->items.values[list->items.count - 1];// value to pop
        list->items.count--;
        RETURN_VALUE(value);
    }
    return bl_value_returnnil(vm, args);
}

static bool objfn_list_shift(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_RANGE(shift, 0, 1);
    int count = 1;
    if(argcount == 1)
    {
        ENFORCE_ARG_TYPE(shift, 0, bl_value_isnumber);
        count = AS_NUMBER(args[0]);
    }
    ObjArray* list = AS_LIST(METHOD_OBJECT);
    if(count >= list->items.count || list->items.count == 1)
    {
        list->items.count = 0;
        return bl_value_returnnil(vm, args);
    }
    else if(count > 0)
    {
        ObjArray* nlist = (ObjArray*)bl_mem_gcprotect(vm, (Object*)bl_object_makelist(vm));
        for(int i = 0; i < count; i++)
        {
            bl_array_push(vm, nlist, list->items.values[0]);
            for(int j = 0; j < list->items.count; j++)
            {
                list->items.values[j] = list->items.values[j + 1];
            }
            list->items.count -= 1;
        }
        if(count == 1)
        {
            RETURN_VALUE(nlist->items.values[0]);
        }
        else
        {
            RETURN_OBJ(nlist);
        }
    }
    return bl_value_returnnil(vm, args);
}

static bool objfn_list_removeat(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(remove_at, 1);
    ENFORCE_ARG_TYPE(remove_at, 0, bl_value_isnumber);
    ObjArray* list = AS_LIST(METHOD_OBJECT);
    int index = AS_NUMBER(args[0]);
    if(index < 0 || index >= list->items.count)
    {
        RETURN_ERROR("list index %d out of range at remove_at()", index);
    }
    Value value = list->items.values[index];
    for(int i = index; i < list->items.count; i++)
    {
        list->items.values[i] = list->items.values[i + 1];
    }
    list->items.count--;
    RETURN_VALUE(value);
}

static bool objfn_list_remove(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(remove, 1);
    ObjArray* list = AS_LIST(METHOD_OBJECT);
    int index = -1;
    for(int i = 0; i < list->items.count; i++)
    {
        if(bl_value_valuesequal(list->items.values[i], args[0]))
        {
            index = i;
            break;
        }
    }
    if(index != -1)
    {
        for(int i = index; i < list->items.count; i++)
        {
            list->items.values[i] = list->items.values[i + 1];
        }
        list->items.count--;
    }
    return bl_value_returnempty(vm, args);
    ;
}

static bool objfn_list_reverse(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(reverse, 0);
    ObjArray* list = AS_LIST(METHOD_OBJECT);
    ObjArray* nlist = (ObjArray*)bl_mem_gcprotect(vm, (Object*)bl_object_makelist(vm));
    /*// in-place reversal
  int start = 0, end = list->items.count - 1;
  while (start < end) {
    Value temp = list->items.values[start];
    list->items.values[start] = list->items.values[end];
    list->items.values[end] = temp;
    start++;
    end--;
  }*/
    for(int i = list->items.count - 1; i >= 0; i--)
    {
        bl_array_push(vm, nlist, list->items.values[i]);
    }
    RETURN_OBJ(nlist);
}

static bool objfn_list_sort(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(sort, 0);
    ObjArray* list = AS_LIST(METHOD_OBJECT);
    bl_value_sortbubblesort(list->items.values, list->items.count);
    return bl_value_returnempty(vm, args);
    ;
}

static bool objfn_list_contains(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(contains, 1);
    ObjArray* list = AS_LIST(METHOD_OBJECT);
    for(int i = 0; i < list->items.count; i++)
    {
        if(bl_value_valuesequal(args[0], list->items.values[i]))
        {
            RETURN_TRUE;
        }
    }
    RETURN_FALSE;
}

static bool objfn_list_delete(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_RANGE(delete, 1, 2);
    ENFORCE_ARG_TYPE(delete, 0, bl_value_isnumber);
    int lowerindex = AS_NUMBER(args[0]);
    int upperindex = lowerindex;
    if(argcount == 2)
    {
        ENFORCE_ARG_TYPE(delete, 1, bl_value_isnumber);
        upperindex = AS_NUMBER(args[1]);
    }
    ObjArray* list = AS_LIST(METHOD_OBJECT);
    if(lowerindex < 0 || lowerindex >= list->items.count)
    {
        RETURN_ERROR("list index %d out of range at delete()", lowerindex);
    }
    else if(upperindex < lowerindex || upperindex >= list->items.count)
    {
        RETURN_ERROR("invalid upper limit %d at delete()", upperindex);
    }
    for(int i = 0; i < list->items.count - upperindex; i++)
    {
        list->items.values[lowerindex + i] = list->items.values[i + upperindex + 1];
    }
    list->items.count -= upperindex - lowerindex + 1;
    RETURN_NUMBER((double)upperindex - (double)lowerindex + 1);
}

static bool objfn_list_first(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(first, 0);
    ObjArray* list = AS_LIST(METHOD_OBJECT);
    if(list->items.count > 0)
    {
        RETURN_VALUE(list->items.values[0]);
    }
    return bl_value_returnnil(vm, args);
}

static bool objfn_list_last(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(last, 0);
    ObjArray* list = AS_LIST(METHOD_OBJECT);
    if(list->items.count > 0)
    {
        RETURN_VALUE(list->items.values[list->items.count - 1]);
    }
    return bl_value_returnnil(vm, args);
}

static bool objfn_list_isempty(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(isempty, 0);
    RETURN_BOOL(AS_LIST(METHOD_OBJECT)->items.count == 0);
}

static bool objfn_list_take(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(take, 1);
    ENFORCE_ARG_TYPE(take, 0, bl_value_isnumber);
    ObjArray* list = AS_LIST(METHOD_OBJECT);
    int count = AS_NUMBER(args[0]);
    if(count < 0)
    {
        count = list->items.count + count;
    }
    if(list->items.count < count)
    {
        RETURN_OBJ(bl_array_copy(vm, list, 0, list->items.count));
    }
    RETURN_OBJ(bl_array_copy(vm, list, 0, count));
}

static bool objfn_list_get(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(get, 1);
    ENFORCE_ARG_TYPE(get, 0, bl_value_isnumber);
    ObjArray* list = AS_LIST(METHOD_OBJECT);
    int index = AS_NUMBER(args[0]);
    if(index < 0 || index >= list->items.count)
    {
        RETURN_ERROR("list index %d out of range at get()", index);
    }
    RETURN_VALUE(list->items.values[index]);
}

static bool objfn_list_compact(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(compact, 0);
    ObjArray* list = AS_LIST(METHOD_OBJECT);
    ObjArray* nlist = (ObjArray*)bl_mem_gcprotect(vm, (Object*)bl_object_makelist(vm));
    for(int i = 0; i < list->items.count; i++)
    {
        if(!bl_value_valuesequal(list->items.values[i], NIL_VAL))
        {
            bl_array_push(vm, nlist, list->items.values[i]);
        }
    }
    RETURN_OBJ(nlist);
}

static bool objfn_list_unique(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(unique, 0);
    ObjArray* list = AS_LIST(METHOD_OBJECT);
    ObjArray* nlist = (ObjArray*)bl_mem_gcprotect(vm, (Object*)bl_object_makelist(vm));
    for(int i = 0; i < list->items.count; i++)
    {
        bool found = false;
        for(int j = 0; j < nlist->items.count; j++)
        {
            if(bl_value_valuesequal(nlist->items.values[j], list->items.values[i]))
            {
                found = true;
                continue;
            }
        }
        if(!found)
        {
            bl_array_push(vm, nlist, list->items.values[i]);
        }
    }
    RETURN_OBJ(nlist);
}

static bool objfn_list_zip(VMState* vm, int argcount, Value* args)
{
    ObjArray* list = AS_LIST(METHOD_OBJECT);
    ObjArray* nlist = (ObjArray*)bl_mem_gcprotect(vm, (Object*)bl_object_makelist(vm));
    ObjArray** arglist = ALLOCATE(ObjArray*, argcount);
    for(int i = 0; i < argcount; i++)
    {
        ENFORCE_ARG_TYPE(zip, i, bl_value_isarray);
        arglist[i] = AS_LIST(args[i]);
    }
    for(int i = 0; i < list->items.count; i++)
    {
        ObjArray* alist = (ObjArray*)bl_mem_gcprotect(vm, (Object*)bl_object_makelist(vm));
        bl_array_push(vm, alist, list->items.values[i]);// item of main list
        for(int j = 0; j < argcount; j++)
        {// item of argument lists
            if(i < arglist[j]->items.count)
            {
                bl_array_push(vm, alist, arglist[j]->items.values[i]);
            }
            else
            {
                bl_array_push(vm, alist, NIL_VAL);
            }
        }
        bl_array_push(vm, nlist, OBJ_VAL(alist));
    }
    RETURN_OBJ(nlist);
}

static bool objfn_list_todict(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(to_dict, 0);
    ObjDict* dict = (ObjDict*)bl_mem_gcprotect(vm, (Object*)bl_object_makedict(vm));
    ObjArray* list = AS_LIST(METHOD_OBJECT);
    for(int i = 0; i < list->items.count; i++)
    {
        bl_dict_setentry(vm, dict, NUMBER_VAL(i), list->items.values[i]);
    }
    RETURN_OBJ(dict);
}

static bool objfn_list_iter(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(__iter__, 1);
    ENFORCE_ARG_TYPE(__iter__, 0, bl_value_isnumber);
    ObjArray* list = AS_LIST(METHOD_OBJECT);
    int index = AS_NUMBER(args[0]);
    if(index > -1 && index < list->items.count)
    {
        RETURN_VALUE(list->items.values[index]);
    }
    return bl_value_returnnil(vm, args);
}

static bool objfn_list_itern(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(__itern__, 1);
    ObjArray* list = AS_LIST(METHOD_OBJECT);
    if(bl_value_isnil(args[0]))
    {
        if(list->items.count == 0)
        {
            RETURN_FALSE;
        }
        RETURN_NUMBER(0);
    }
    if(!bl_value_isnumber(args[0]))
    {
        RETURN_ERROR("lists are numerically indexed");
    }
    int index = AS_NUMBER(args[0]);
    if(index < list->items.count - 1)
    {
        RETURN_NUMBER((double)index + 1);
    }
    return bl_value_returnnil(vm, args);
}


RegModule* bl_modload_array(VMState* vm)
{
    (void)vm;
    static RegFunc modulefunctions[] = {

        // common
        { "length", false, modfn_array_length },
        { "first", false, modfn_array_first },
        { "last", false, modfn_array_last },
        { "extend", false, modfn_array_extend },
        { "to_string", false, modfn_array_tostring },
        { "itern", false, modfn_array_itern_ },
        { NULL, false, NULL },
    };
    static RegModule module = { .name = "_array", .fields = NULL, .functions = modulefunctions, .classes = NULL, .preloader = NULL, .unloader = NULL };
    return &module;
}

void bl_state_initarraymethods(VMState* vm)
{
    // list methods
    bl_class_defnativefield(vm, vm->classobjlist, "length", objfn_list_length);
    bl_class_defnativemethod(vm, vm->classobjlist, "append", objfn_list_append);
    bl_class_defnativemethod(vm, vm->classobjlist, "push", objfn_list_append);
    bl_class_defnativemethod(vm, vm->classobjlist, "clear", objfn_list_clear);
    bl_class_defnativemethod(vm, vm->classobjlist, "clone", objfn_list_clone);
    bl_class_defnativemethod(vm, vm->classobjlist, "count", objfn_list_count);
    bl_class_defnativemethod(vm, vm->classobjlist, "extend", objfn_list_extend);
    bl_class_defnativemethod(vm, vm->classobjlist, "indexof", objfn_list_indexof);
    bl_class_defnativemethod(vm, vm->classobjlist, "insert", objfn_list_insert);
    bl_class_defnativemethod(vm, vm->classobjlist, "pop", objfn_list_pop);
    bl_class_defnativemethod(vm, vm->classobjlist, "shift", objfn_list_shift);
    bl_class_defnativemethod(vm, vm->classobjlist, "remove_at", objfn_list_removeat);
    bl_class_defnativemethod(vm, vm->classobjlist, "remove", objfn_list_remove);
    bl_class_defnativemethod(vm, vm->classobjlist, "reverse", objfn_list_reverse);
    bl_class_defnativemethod(vm, vm->classobjlist, "sort", objfn_list_sort);
    bl_class_defnativemethod(vm, vm->classobjlist, "contains", objfn_list_contains);
    bl_class_defnativemethod(vm, vm->classobjlist, "delete", objfn_list_delete);
    bl_class_defnativemethod(vm, vm->classobjlist, "first", objfn_list_first);
    bl_class_defnativemethod(vm, vm->classobjlist, "last", objfn_list_last);
    bl_class_defnativemethod(vm, vm->classobjlist, "isempty", objfn_list_isempty);
    bl_class_defnativemethod(vm, vm->classobjlist, "take", objfn_list_take);
    bl_class_defnativemethod(vm, vm->classobjlist, "get", objfn_list_get);
    bl_class_defnativemethod(vm, vm->classobjlist, "compact", objfn_list_compact);
    bl_class_defnativemethod(vm, vm->classobjlist, "unique", objfn_list_unique);
    bl_class_defnativemethod(vm, vm->classobjlist, "zip", objfn_list_zip);
    bl_class_defnativemethod(vm, vm->classobjlist, "to_dict", objfn_list_todict);
    bl_class_defnativemethod(vm, vm->classobjlist, "@iter", objfn_list_iter);
    bl_class_defnativemethod(vm, vm->classobjlist, "@itern", objfn_list_itern);
}

