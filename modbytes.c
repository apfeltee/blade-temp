
#include "blade.h"


ObjBytes* bl_bytes_addbytes(VMState* vm, ObjBytes* a, ObjBytes* b)
{
    ObjBytes* bytes;
    bytes = bl_object_makebytes(vm, a->bytes.count + b->bytes.count);
    memcpy(bytes->bytes.bytes, a->bytes.bytes, a->bytes.count);
    memcpy(bytes->bytes.bytes + a->bytes.count, b->bytes.bytes, b->bytes.count);
    return bytes;
}


static bool objfn_bytes_length(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(length, 0);
    RETURN_NUMBER(AS_BYTES(METHOD_OBJECT)->bytes.count);
}

static bool objfn_bytes_append(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(append, 1);
    if(bl_value_isnumber(args[0]))
    {
        int byte = (int)AS_NUMBER(args[0]);
        if(byte < 0 || byte > 255)
        {
            RETURN_ERROR("invalid byte. bytes range from 0 to 255");
        }
        // append here...
        ObjBytes* bytes = AS_BYTES(METHOD_OBJECT);
        int oldcount = bytes->bytes.count;
        bytes->bytes.count++;
        bytes->bytes.bytes = GROW_ARRAY(unsigned char, sizeof(unsigned char), bytes->bytes.bytes, oldcount, bytes->bytes.count);
        bytes->bytes.bytes[bytes->bytes.count - 1] = (unsigned char)byte;
        return bl_value_returnempty(vm, args);
        ;
    }
    else if(bl_value_isarray(args[0]))
    {
        ObjArray* list = AS_LIST(args[0]);
        if(list->items.count > 0)
        {
            // append here...
            ObjBytes* bytes = AS_BYTES(METHOD_OBJECT);
            bytes->bytes.bytes
            = GROW_ARRAY(unsigned char, sizeof(unsigned char), bytes->bytes.bytes, bytes->bytes.count, (size_t)bytes->bytes.count + (size_t)list->items.count);
            if(bytes->bytes.bytes == NULL)
            {
                RETURN_ERROR("out of memory");
            }
            for(int i = 0; i < list->items.count; i++)
            {
                if(!bl_value_isnumber(list->items.values[i]))
                {
                    RETURN_ERROR("bytes lists can only contain numbers");
                }
                int byte = (int)AS_NUMBER(list->items.values[i]);
                if(byte < 0 || byte > 255)
                {
                    RETURN_ERROR("invalid byte. bytes range from 0 to 255");
                }
                bytes->bytes.bytes[bytes->bytes.count + i] = (unsigned char)byte;
            }
            bytes->bytes.count += list->items.count;
        }
        return bl_value_returnempty(vm, args);
        ;
    }
    RETURN_ERROR("bytes can only append a byte or a list of bytes");
}

static bool objfn_bytes_clone(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(clone, 0);
    ObjBytes* bytes = AS_BYTES(METHOD_OBJECT);
    ObjBytes* nbytes = (ObjBytes*)bl_mem_gcprotect(vm, (Object*)bl_object_makebytes(vm, bytes->bytes.count));
    memcpy(nbytes->bytes.bytes, bytes->bytes.bytes, bytes->bytes.count);
    RETURN_OBJ(nbytes);
}

static bool objfn_bytes_extend(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(extend, 1);
    ENFORCE_ARG_TYPE(extend, 0, bl_value_isbytes);
    ObjBytes* bytes = AS_BYTES(METHOD_OBJECT);
    ObjBytes* nbytes = AS_BYTES(args[0]);
    bytes->bytes.bytes = GROW_ARRAY(unsigned char, sizeof(unsigned char), bytes->bytes.bytes, bytes->bytes.count, bytes->bytes.count + nbytes->bytes.count);
    if(bytes->bytes.bytes == NULL)
    {
        RETURN_ERROR("out of memory");
    }
    memcpy(bytes->bytes.bytes + bytes->bytes.count, nbytes->bytes.bytes, nbytes->bytes.count);
    bytes->bytes.count += nbytes->bytes.count;
    RETURN_OBJ(bytes);
}

static bool objfn_bytes_pop(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(pop, 0);
    ObjBytes* bytes = AS_BYTES(METHOD_OBJECT);
    unsigned char c = bytes->bytes.bytes[bytes->bytes.count - 1];
    bytes->bytes.count--;
    RETURN_NUMBER((double)((int)c));
}

static bool objfn_bytes_remove(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(remove, 1);
    ENFORCE_ARG_TYPE(remove, 0, bl_value_isnumber);
    ObjBytes* bytes = AS_BYTES(METHOD_OBJECT);
    int index = AS_NUMBER(args[0]);
    if(index < 0 || index >= bytes->bytes.count)
    {
        RETURN_ERROR("bytes index %d out of range", index);
    }
    unsigned char val = bytes->bytes.bytes[index];
    for(int i = index; i < bytes->bytes.count; i++)
    {
        bytes->bytes.bytes[i] = bytes->bytes.bytes[i + 1];
    }
    bytes->bytes.count--;
    RETURN_NUMBER((double)((int)val));
}

static bool objfn_bytes_reverse(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(reverse, 0);
    ObjBytes* bytes = AS_BYTES(METHOD_OBJECT);
    ObjBytes* nbytes = (ObjBytes*)bl_mem_gcprotect(vm, (Object*)bl_object_makebytes(vm, bytes->bytes.count));
    for(int i = 0; i < bytes->bytes.count; i++)
    {
        nbytes->bytes.bytes[i] = bytes->bytes.bytes[bytes->bytes.count - i - 1];
    }
    RETURN_OBJ(nbytes);
}

static bool objfn_bytes_split(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(split, 1);
    ENFORCE_ARG_TYPE(split, 0, bl_value_isbytes);
    ByteArray object = AS_BYTES(METHOD_OBJECT)->bytes;
    ByteArray delimeter = AS_BYTES(args[0])->bytes;
    if(object.count == 0 || delimeter.count > object.count)
        RETURN_OBJ(bl_object_makelist(vm));
    ObjArray* list = (ObjArray*)bl_mem_gcprotect(vm, (Object*)bl_object_makelist(vm));
    // main work here...
    if(delimeter.count > 0)
    {
        int start = 0;
        for(int i = 0; i <= object.count; i++)
        {
            // match found.
            if(memcmp(object.bytes + i, delimeter.bytes, delimeter.count) == 0 || i == object.count)
            {
                ObjBytes* bytes = (ObjBytes*)bl_mem_gcprotect(vm, (Object*)bl_object_makebytes(vm, i - start));
                memcpy(bytes->bytes.bytes, object.bytes + start, i - start);
                bl_array_push(vm, list, OBJ_VAL(bytes));
                i += delimeter.count - 1;
                start = i + 1;
            }
        }
    }
    else
    {
        int length = object.count;
        for(int i = 0; i < length; i++)
        {
            ObjBytes* bytes = (ObjBytes*)bl_mem_gcprotect(vm, (Object*)bl_object_makebytes(vm, 1));
            memcpy(bytes->bytes.bytes, object.bytes + i, 1);
            bl_array_push(vm, list, OBJ_VAL(bytes));
        }
    }
    RETURN_OBJ(list);
}

static bool objfn_bytes_first(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(first, 0);
    RETURN_NUMBER((double)((int)AS_BYTES(METHOD_OBJECT)->bytes.bytes[0]));
}

static bool objfn_bytes_last(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(first, 0);
    ObjBytes* bytes = AS_BYTES(METHOD_OBJECT);
    RETURN_NUMBER((double)((int)bytes->bytes.bytes[bytes->bytes.count - 1]));
}

static bool objfn_bytes_get(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(get, 1);
    ENFORCE_ARG_TYPE(get, 0, bl_value_isnumber);
    ObjBytes* bytes = AS_BYTES(METHOD_OBJECT);
    int index = AS_NUMBER(args[0]);
    if(index < 0 || index >= bytes->bytes.count)
    {
        RETURN_ERROR("bytes index %d out of range", index);
    }
    RETURN_NUMBER((double)((int)bytes->bytes.bytes[index]));
}

static bool objfn_bytes_isalpha(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(bl_scanutil_isalpha, 0);
    ObjBytes* bytes = AS_BYTES(METHOD_OBJECT);
    for(int i = 0; i < bytes->bytes.count; i++)
    {
        if(!isalpha(bytes->bytes.bytes[i]))
        {
            RETURN_FALSE;
        }
    }
    RETURN_TRUE;
}

static bool objfn_bytes_isalnum(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(isalnum, 0);
    ObjBytes* bytes = AS_BYTES(METHOD_OBJECT);
    for(int i = 0; i < bytes->bytes.count; i++)
    {
        if(!isalnum(bytes->bytes.bytes[i]))
        {
            RETURN_FALSE;
        }
    }
    RETURN_TRUE;
}

static bool objfn_bytes_isnumber(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(is_number, 0);
    ObjBytes* bytes = AS_BYTES(METHOD_OBJECT);
    for(int i = 0; i < bytes->bytes.count; i++)
    {
        if(!isdigit(bytes->bytes.bytes[i]))
        {
            RETURN_FALSE;
        }
    }
    RETURN_TRUE;
}

static bool objfn_bytes_islower(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(islower, 0);
    ObjBytes* bytes = AS_BYTES(METHOD_OBJECT);
    for(int i = 0; i < bytes->bytes.count; i++)
    {
        if(!islower(bytes->bytes.bytes[i]))
        {
            RETURN_FALSE;
        }
    }
    RETURN_TRUE;
}

static bool objfn_bytes_isupper(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(isupper, 0);
    ObjBytes* bytes = AS_BYTES(METHOD_OBJECT);
    for(int i = 0; i < bytes->bytes.count; i++)
    {
        if(!isupper(bytes->bytes.bytes[i]))
        {
            RETURN_FALSE;
        }
    }
    RETURN_TRUE;
}

static bool objfn_bytes_isspace(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(isspace, 0);
    ObjBytes* bytes = AS_BYTES(METHOD_OBJECT);
    for(int i = 0; i < bytes->bytes.count; i++)
    {
        if(!isspace(bytes->bytes.bytes[i]))
        {
            RETURN_FALSE;
        }
    }
    RETURN_TRUE;
}

static bool objfn_bytes_dispose(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(dispose, 0);
    ObjBytes* bytes = AS_BYTES(METHOD_OBJECT);
    bl_bytearray_free(vm, &bytes->bytes);
    return bl_value_returnempty(vm, args);
    ;
}

static bool objfn_bytes_tolist(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(to_list, 0);
    ObjBytes* bytes = AS_BYTES(METHOD_OBJECT);
    ObjArray* list = (ObjArray*)bl_mem_gcprotect(vm, (Object*)bl_object_makelist(vm));
    for(int i = 0; i < bytes->bytes.count; i++)
    {
        bl_array_push(vm, list, NUMBER_VAL((double)((int)bytes->bytes.bytes[i])));
    }
    RETURN_OBJ(list);
}

static bool objfn_bytes_tostring(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(to_string, 0);
    ObjBytes* bytes = AS_BYTES(METHOD_OBJECT);
    char* string = (char*)bytes->bytes.bytes;
    RETURN_L_STRING(string, bytes->bytes.count);
}

static bool objfn_bytes_iter(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(__iter__, 1);
    ENFORCE_ARG_TYPE(__iter__, 0, bl_value_isnumber);
    ObjBytes* bytes = AS_BYTES(METHOD_OBJECT);
    int index = AS_NUMBER(args[0]);
    if(index > -1 && index < bytes->bytes.count)
    {
        RETURN_NUMBER((int)bytes->bytes.bytes[index]);
    }
    return bl_value_returnnil(vm, args);
}

static bool objfn_bytes_itern(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(__itern__, 1);
    ObjBytes* bytes = AS_BYTES(METHOD_OBJECT);
    if(bl_value_isnil(args[0]))
    {
        if(bytes->bytes.count == 0)
            RETURN_FALSE;
        RETURN_NUMBER(0);
    }
    if(!bl_value_isnumber(args[0]))
    {
        RETURN_ERROR("bytes are numerically indexed");
    }
    int index = AS_NUMBER(args[0]);
    if(index < bytes->bytes.count - 1)
    {
        RETURN_NUMBER((double)index + 1);
    }
    return bl_value_returnnil(vm, args);
}

void bl_state_initbytesmethods(VMState* vm)
{
    // bytes
    bl_class_defnativemethod(vm, vm->classobjbytes, "length", objfn_bytes_length);
    bl_class_defnativemethod(vm, vm->classobjbytes, "append", objfn_bytes_append);
    bl_class_defnativemethod(vm, vm->classobjbytes, "clone", objfn_bytes_clone);
    bl_class_defnativemethod(vm, vm->classobjbytes, "extend", objfn_bytes_extend);
    bl_class_defnativemethod(vm, vm->classobjbytes, "pop", objfn_bytes_pop);
    bl_class_defnativemethod(vm, vm->classobjbytes, "remove", objfn_bytes_remove);
    bl_class_defnativemethod(vm, vm->classobjbytes, "reverse", objfn_bytes_reverse);
    bl_class_defnativemethod(vm, vm->classobjbytes, "first", objfn_bytes_first);
    bl_class_defnativemethod(vm, vm->classobjbytes, "last", objfn_bytes_last);
    bl_class_defnativemethod(vm, vm->classobjbytes, "get", objfn_bytes_get);
    bl_class_defnativemethod(vm, vm->classobjbytes, "split", objfn_bytes_split);
    bl_class_defnativemethod(vm, vm->classobjbytes, "dispose", objfn_bytes_dispose);
    bl_class_defnativemethod(vm, vm->classobjbytes, "bl_scanutil_isalpha", objfn_bytes_isalpha);
    bl_class_defnativemethod(vm, vm->classobjbytes, "isalnum", objfn_bytes_isalnum);
    bl_class_defnativemethod(vm, vm->classobjbytes, "is_number", objfn_bytes_isnumber);
    bl_class_defnativemethod(vm, vm->classobjbytes, "islower", objfn_bytes_islower);
    bl_class_defnativemethod(vm, vm->classobjbytes, "isupper", objfn_bytes_isupper);
    bl_class_defnativemethod(vm, vm->classobjbytes, "isspace", objfn_bytes_isspace);
    bl_class_defnativemethod(vm, vm->classobjbytes, "to_list", objfn_bytes_tolist);
    bl_class_defnativemethod(vm, vm->classobjbytes, "to_string", objfn_bytes_tostring);
    bl_class_defnativemethod(vm, vm->classobjbytes, "@iter", objfn_bytes_iter);
    bl_class_defnativemethod(vm, vm->classobjbytes, "@itern", objfn_bytes_itern);
}


