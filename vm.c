
#include "blade.h"

void bl_vm_pushvalue(VMState* vm, Value value)
{
    *vm->stacktop = value;
    vm->stacktop++;
}

Value bl_vm_popvalue(VMState* vm)
{
    vm->stacktop--;
    return *vm->stacktop;
}

Value bl_vm_popvaluen(VMState* vm, int n)
{
    vm->stacktop -= n;
    return *vm->stacktop;
}

Value bl_vm_peekvalue(VMState* vm, int distance)
{
    return vm->stacktop[-1 - distance];
}


void bl_vm_resetstack(VMState* vm)
{
    vm->stacktop = vm->stack;
    vm->framecount = 0;
    vm->openupvalues = NULL;
}

void init_vm(VMState* vm)
{
    fprintf(stderr, "call to init_vm()\n");
    bl_vm_resetstack(vm);
    vm->compiler = NULL;
    vm->objectlinks = NULL;
    vm->objectcount = 0;
    vm->exceptionclass = NULL;
    vm->bytesallocated = 0;
    vm->gcprotected = 0;
    vm->nextgc = DEFAULT_GC_START;// default is 1mb. Can be modified via the -g flag.
    vm->isrepl = false;
    vm->shoulddebugstack = false;
    vm->shouldprintbytecode = false;
    vm->graycount = 0;
    vm->graycapacity = 0;
    vm->framecount = 0;
    vm->graystack = NULL;
    vm->stdargs = NULL;
    vm->stdargscount = 0;
    vm->allowgc = false;
    bl_hashtable_init(&vm->modules);
    bl_hashtable_init(&vm->strings);
    bl_hashtable_init(&vm->globals);
    // object methods tables
    vm->classobjobject = bl_object_makeclass(vm, bl_string_copystring(vm, "Object"), NULL);
    vm->classobjstring = bl_object_makeclass(vm, bl_string_copystring(vm, "String"), vm->classobjobject);
    vm->classobjlist = bl_object_makeclass(vm, bl_string_copystring(vm, "array"), vm->classobjobject);
    vm->classobjdict = bl_object_makeclass(vm, bl_string_copystring(vm, "dict"), vm->classobjobject);
    vm->classobjfile = bl_object_makeclass(vm, bl_string_copystring(vm, "file"), vm->classobjobject);
    vm->classobjbytes = bl_object_makeclass(vm, bl_string_copystring(vm, "bytes"), vm->classobjobject);
    vm->classobjrange = bl_object_makeclass(vm, bl_string_copystring(vm, "range"), vm->classobjobject);
    bl_state_initbuiltinfunctions(vm);
    bl_state_initbuiltinmethods(vm);
    //vm->allowgc = true;
}

void bl_vm_freevm(VMState* vm)
{
    fprintf(stderr, "call to bl_vm_freevm()\n");
    //@TODO: Fix segfault from enabling this...
    bl_mem_freegcobjects(vm);
    bl_hashtable_free(vm, &vm->strings);
    bl_hashtable_free(vm, &vm->globals);
    // since object in module can exist in globals
    // it must come after
    bl_hashtable_cleanfree(vm, &vm->modules);
    /*
    bl_hashtable_free(vm, &vm->classobjstring);
    bl_hashtable_free(vm, &vm->classobjlist);
    bl_hashtable_free(vm, &vm->classobjdict);
    bl_hashtable_free(vm, &vm->classobjfile);
    bl_hashtable_free(vm, &vm->classobjbytes);
    bl_hashtable_free(vm, &vm->classobjrange);
    */
}

PtrResult bl_vm_interpsource(VMState* vm, ObjModule* module, const char* source)
{
    ObjClosure* closure;
    ObjFunction* function;
    BinaryBlob blob;
    bl_blob_init(&blob);
    vm->allowgc = false;
    bl_vm_initexceptions(vm, module);
    vm->allowgc = true;
    function = bl_compiler_compilesource(vm, module, source, &blob);
    if(vm->shouldprintbytecode)
    {
        return PTR_OK;
    }
    if(function == NULL)
    {
        bl_blob_free(vm, &blob);
        return PTR_COMPILE_ERR;
    }
    bl_vm_pushvalue(vm, OBJ_VAL(function));
    closure = bl_object_makeclosure(vm, function);
    bl_vm_popvalue(vm);
    bl_vm_pushvalue(vm, OBJ_VAL(closure));
    bl_vmdo_docall(vm, closure, 0);
    PtrResult result = bl_vm_run(vm);
    return result;
}


void bl_vm_runtimeerror(VMState* vm, const char* format, ...)
{
    int i;
    int line;
    size_t instruction;
    va_list args;
    ObjFunction* function;
    CallFrame* frame;
    // flush out anything on stdout first
    fflush(stdout);
    frame = &vm->frames[vm->framecount - 1];
    function = frame->closure->fnptr;
    instruction = frame->ip - function->blob.code - 1;
    line = function->blob.lines[instruction];
    fprintf(stderr, "RuntimeError: ");
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, " -> %s:%d ", function->module->file, line);
    fputs("\n", stderr);
    if(vm->framecount > 1)
    {
        fprintf(stderr, "StackTrace:\n");
        for(i = vm->framecount - 1; i >= 0; i--)
        {
            frame = &vm->frames[i];
            function = frame->closure->fnptr;
            // -1 because the IP is sitting on the next instruction to be executed
            instruction = frame->ip - function->blob.code - 1;
            fprintf(stderr, "    %s:%d -> ", function->module->file, function->blob.lines[instruction]);
            if(function->name == NULL)
            {
                fprintf(stderr, "@.script()");
            }
            else
            {
                fprintf(stderr, "%s()", function->name->chars);
            }
            fprintf(stderr, "\n");
        }
    }
    bl_vm_resetstack(vm);
}


FuncType bl_vmutil_getmethodtype(Value method)
{
    switch(OBJ_TYPE(method))
    {
        case OBJ_NATIVEFUNCTION:
            {
                return AS_NATIVE(method)->type;
            }
            break;
        case OBJ_CLOSURE:
            {
                return AS_CLOSURE(method)->fnptr->type;
            }
            break;
        default:
            break;
    }
    return TYPE_FUNCTION;
}

Value bl_vm_getstacktrace(VMState* vm)
{
    int i;
    int line;
    size_t instruction;
    size_t tracelinelength;
    char* trace;
    char* fnname;
    char* traceline;
    const char* traceformat;
    CallFrame* frame;
    ObjFunction* function;
    trace = (char*)calloc(1, sizeof(char));
    if(trace != NULL)
    {
        for(i = 0; i < vm->framecount; i++)
        {
            frame = &vm->frames[i];
            function = frame->closure->fnptr;
            // -1 because the IP is sitting on the next instruction to be executed
            instruction = frame->ip - function->blob.code - 1;
            line = function->blob.lines[instruction];
            traceformat = i != vm->framecount - 1 ? "    %s:%d -> %s()\n" : "    %s:%d -> %s()";
            fnname = function->name == NULL ? (char*)"@.script" : function->name->chars;
            tracelinelength = snprintf(NULL, 0, traceformat, function->module->file, line, fnname);
            traceline = ALLOCATE(char, tracelinelength + 1);
            if(traceline != NULL)
            {
                sprintf(traceline, traceformat, function->module->file, line, fnname);
                traceline[(int)tracelinelength] = '\0';
            }
            trace = bl_util_appendstring(trace, traceline);
            free(traceline);
        }
        return STRING_TT_VAL(trace);
    }
    return STRING_L_VAL("", 0);
}


bool bl_vmdo_instanceinvokefromclass(VMState* vm, ObjClass* klass, ObjString* name, int argcount)
{
    Value method;
    if(bl_hashtable_get(&klass->methods, OBJ_VAL(name), &method))
    {
        if(bl_vmutil_getmethodtype(method) == TYPE_PRIVATE)
        {
            return bl_vm_throwexception(vm, false, "cannot call private method '%s' from instance of %s", name->chars, klass->name->chars);
        }
        return bl_vmdo_callvalue(vm, method, argcount);
    }
    return bl_vm_throwexception(vm, false, "undefined method '%s' in %s", name->chars, klass->name->chars);
}


bool bl_vmdo_classbindmethod(VMState* vm, ObjClass* klass, ObjString* name)
{
    Value method;
    if(bl_hashtable_get(&klass->methods, OBJ_VAL(name), &method))
    {
        if(bl_vmutil_getmethodtype(method) == TYPE_PRIVATE)
        {
            return bl_vm_throwexception(vm, false, "cannot get private property '%s' from instance", name->chars);
        }
        ObjBoundMethod* bound = bl_object_makeboundmethod(vm, bl_vm_peekvalue(vm, 0), AS_CLOSURE(method));
        bl_vm_popvalue(vm);
        bl_vm_pushvalue(vm, OBJ_VAL(bound));
        return true;
    }
    return bl_vm_throwexception(vm, false, "undefined property '%s'", name->chars);
}


bool bl_vmdo_dictgetindex(VMState* vm, ObjDict* dict, bool willassign)
{
    Value index;
    Value result;
    index = bl_vm_peekvalue(vm, 0);
    if(bl_dict_getentry(dict, index, &result))
    {
        if(!willassign)
        {
            bl_vm_popvaluen(vm, 2);// we can safely get rid of the index from the stack
        }
        bl_vm_pushvalue(vm, result);
        return true;
    }
    bl_vm_popvaluen(vm, 1);
    return bl_vm_throwexception(vm, false, "invalid index %s", bl_value_tostring(vm, index));
}

void bl_vmdo_dictsetindex(VMState* vm, ObjDict* dict, Value index, Value value)
{
    bl_dict_setentry(vm, dict, index, value);
    bl_vm_popvaluen(vm, 3);// pop the value, index and dict out
    // leave the value on the stack for consumption
    // e.g. variable = dict[index] = 10
    bl_vm_pushvalue(vm, value);
}


static bool bl_vmdo_callnativemethod(VMState* vm, ObjNativeFunction* native, int argcount)
{
    if(native->natfn(vm, argcount, vm->stacktop - argcount))
    {
        bl_mem_gcclearprotect(vm);
        vm->stacktop -= argcount;
        return true;
    }
    /*
    else
    {
        bl_mem_gcclearprotect(vm);
        bool overridden = AS_BOOL(vm->stacktop[-argcount - 1]);
        *//*if (!overridden) {
        vm->stacktop -= argcount + 1;
        }*//*
        return overridden;
    }*/
    return true;
}

bool bl_vmdo_docall(VMState* vm, ObjClosure* closure, int argcount)
{
    int i;
    int vaargsstart;
    CallFrame* frame;
    ObjArray* argslist;
    // fill empty parameters if not variadic
    for(; !closure->fnptr->isvariadic && argcount < closure->fnptr->arity; argcount++)
    {
        bl_vm_pushvalue(vm, NIL_VAL);
    }
    // handle variadic arguments...
    if(closure->fnptr->isvariadic && argcount >= closure->fnptr->arity - 1)
    {
        vaargsstart = argcount - closure->fnptr->arity;
        argslist = bl_object_makelist(vm);
        bl_vm_pushvalue(vm, OBJ_VAL(argslist));
        for(i = vaargsstart; i >= 0; i--)
        {
            bl_valarray_push(vm, &argslist->items, bl_vm_peekvalue(vm, i + 1));
        }
        argcount -= vaargsstart;
        bl_vm_popvaluen(vm, vaargsstart + 2);// +1 for the gc protection push above
        bl_vm_pushvalue(vm, OBJ_VAL(argslist));
    }
    if(argcount != closure->fnptr->arity)
    {
        bl_vm_popvaluen(vm, argcount);
        if(closure->fnptr->isvariadic)
        {
            return bl_vm_throwexception(vm, false, "expected at least %d arguments but got %d", closure->fnptr->arity - 1, argcount);
        }
        else
        {
            return bl_vm_throwexception(vm, false, "expected %d arguments but got %d", closure->fnptr->arity, argcount);
        }
    }
    if(vm->framecount == FRAMES_MAX)
    {
        bl_vm_popvaluen(vm, argcount);
        return bl_vm_throwexception(vm, false, "stack overflow");
    }
    frame = &vm->frames[vm->framecount++];
    frame->closure = closure;
    frame->ip = closure->fnptr->blob.code;
    frame->slots = vm->stacktop - argcount - 1;
    return true;
}


bool bl_vmdo_callvalue(VMState* vm, Value callee, int argcount)
{
    if(bl_value_isobject(callee))
    {
        switch(OBJ_TYPE(callee))
        {
            case OBJ_BOUNDFUNCTION:
                {
                    ObjBoundMethod* bound = AS_BOUND(callee);
                    vm->stacktop[-argcount - 1] = bound->receiver;
                    return bl_vmdo_docall(vm, bound->method, argcount);
                }
                break;
            case OBJ_CLASS:
                {
                    ObjClass* klass = AS_CLASS(callee);
                    vm->stacktop[-argcount - 1] = OBJ_VAL(bl_object_makeinstance(vm, klass));
                    if(!bl_value_isempty(klass->initializer))
                    {
                        return bl_vmdo_docall(vm, AS_CLOSURE(klass->initializer), argcount);
                    }
                    else if(klass->superclass != NULL && !bl_value_isempty(klass->superclass->initializer))
                    {
                        return bl_vmdo_docall(vm, AS_CLOSURE(klass->superclass->initializer), argcount);
                    }
                    else if(argcount != 0)
                    {
                        return bl_vm_throwexception(vm, false, "%s constructor expects 0 arguments, %d given", klass->name->chars, argcount);
                    }
                    return true;
                }
                break;
            case OBJ_MODULE:
                {
                    ObjModule* module = AS_MODULE(callee);
                    Value callable;
                    if(bl_hashtable_get(&module->values, STRING_VAL(module->name), &callable))
                    {
                        return bl_vmdo_callvalue(vm, callable, argcount);
                    }
                }
                break;
            case OBJ_CLOSURE:
                {
                    return bl_vmdo_docall(vm, AS_CLOSURE(callee), argcount);
                }
                break;
            case OBJ_NATIVEFUNCTION:
                {
                    return bl_vmdo_callnativemethod(vm, AS_NATIVE(callee), argcount);
                }
                break;
            default:// non callable
                break;
        }
    }
    return bl_vm_throwexception(vm, false, "object of type %s is not callable", bl_value_typename(callee));
}

static bool bl_instance_invokefromself(VMState* vm, ObjString* name, int argcount)
{
    Value value;
    Value receiver;
    ObjInstance* instance;
    receiver = bl_vm_peekvalue(vm, argcount);
    if(bl_value_isinstance(receiver))
    {
        instance = AS_INSTANCE(receiver);
        if(bl_hashtable_get(&instance->klass->methods, OBJ_VAL(name), &value))
        {
            return bl_vmdo_callvalue(vm, value, argcount);
        }
        if(bl_hashtable_get(&instance->properties, OBJ_VAL(name), &value))
        {
            vm->stacktop[-argcount - 1] = value;
            return bl_vmdo_callvalue(vm, value, argcount);
        }
    }
    else if(bl_value_isclass(receiver))
    {
        if(bl_hashtable_get(&AS_CLASS(receiver)->methods, OBJ_VAL(name), &value))
        {
            if(bl_vmutil_getmethodtype(value) == TYPE_STATIC)
            {
                return bl_vmdo_callvalue(vm, value, argcount);
            }
            return bl_vm_throwexception(vm, false, "cannot call non-static method %s() on non instance", name->chars);
        }
    }
    return bl_vm_throwexception(vm, false, "cannot call method %s on object of type %s", name->chars, bl_value_typename(receiver));
}

static bool blade_vm_invokemethod(VMState* vm, ObjString* name, int argcount)
{
    Value value;
    Value receiver;
    receiver = bl_vm_peekvalue(vm, argcount);
    if(!bl_value_isobject(receiver))
    {
        // @TODO: have methods for non objects as well.
        return bl_vm_throwexception(vm, false, "non-object %s has no method", bl_value_typename(receiver));
    }
    else
    {
        switch(AS_OBJ(receiver)->type)
        {
            case OBJ_MODULE:
                {
                    ObjModule* module = AS_MODULE(receiver);
                    if(bl_hashtable_get(&module->values, OBJ_VAL(name), &value))
                    {
                        if(name->length > 0 && name->chars[0] == '_')
                        {
                            return bl_vm_throwexception(vm, false, "cannot call private module method '%s'", name->chars);
                        }
                        return bl_vmdo_callvalue(vm, value, argcount);
                    }
                    return bl_vm_throwexception(vm, false, "module %s does not define class or method %s()", module->name, name->chars);
                }
                break;
            case OBJ_CLASS:
                {
                    if(bl_hashtable_get(&AS_CLASS(receiver)->methods, OBJ_VAL(name), &value))
                    {
                        if(bl_vmutil_getmethodtype(value) == TYPE_PRIVATE)
                        {
                            return bl_vm_throwexception(vm, false, "cannot call private method %s() on %s", name->chars, AS_CLASS(receiver)->name->chars);
                        }
                        return bl_vmdo_callvalue(vm, value, argcount);
                    }
                    else if(bl_hashtable_get(&AS_CLASS(receiver)->staticproperties, OBJ_VAL(name), &value))
                    {
                        return bl_vmdo_callvalue(vm, value, argcount);
                    }
                    return bl_vm_throwexception(vm, false, "unknown method %s() in class %s", name->chars, AS_CLASS(receiver)->name->chars);
                }
                break;
            case OBJ_INSTANCE:
                {
                    ObjInstance* instance = AS_INSTANCE(receiver);
                    if(bl_hashtable_get(&instance->properties, OBJ_VAL(name), &value))
                    {
                        vm->stacktop[-argcount - 1] = value;
                        return bl_vmdo_callvalue(vm, value, argcount);
                    }
                    return bl_vmdo_instanceinvokefromclass(vm, instance->klass, name, argcount);
                }
                break;
            case OBJ_STRING:
                {
                    if(bl_hashtable_get(&vm->classobjstring->methods, OBJ_VAL(name), &value))
                    {
                        return bl_vmdo_callnativemethod(vm, AS_NATIVE(value), argcount);
                    }
                    return bl_vm_throwexception(vm, false, "String has no method %s()", name->chars);
                }
                break;
            case OBJ_ARRAY:
                {
                    if(bl_hashtable_get(&vm->classobjlist->methods, OBJ_VAL(name), &value))
                    {
                        return bl_vmdo_callnativemethod(vm, AS_NATIVE(value), argcount);
                    }
                    return bl_vm_throwexception(vm, false, "List has no method %s()", name->chars);
                }
                break;
            case OBJ_RANGE:
                {
                    if(bl_hashtable_get(&vm->classobjrange->methods, OBJ_VAL(name), &value))
                    {
                        return bl_vmdo_callnativemethod(vm, AS_NATIVE(value), argcount);
                    }
                    return bl_vm_throwexception(vm, false, "Range has no method %s()", name->chars);
                }
                break;
            case OBJ_DICT:
                {
                    if(bl_hashtable_get(&vm->classobjdict->methods, OBJ_VAL(name), &value))
                    {
                        return bl_vmdo_callnativemethod(vm, AS_NATIVE(value), argcount);
                    }
                    return bl_vm_throwexception(vm, false, "Dict has no method %s()", name->chars);
                }
                break;
            case OBJ_FILE:
                {
                    if(bl_hashtable_get(&vm->classobjfile->methods, OBJ_VAL(name), &value))
                    {
                        return bl_vmdo_callnativemethod(vm, AS_NATIVE(value), argcount);
                    }
                    return bl_vm_throwexception(vm, false, "File has no method %s()", name->chars);
                }
                break;
            case OBJ_BYTES:
                {
                    if(bl_hashtable_get(&vm->classobjbytes->methods, OBJ_VAL(name), &value))
                    {
                        return bl_vmdo_callnativemethod(vm, AS_NATIVE(value), argcount);
                    }
                    return bl_vm_throwexception(vm, false, "Bytes has no method %s()", name->chars);
                }
                break;
            default:
                {
                }
                break;
        }
    }
    return bl_vm_throwexception(vm, false, "cannot call method %s on object of type %s", name->chars, bl_value_typename(receiver));
}

static ObjUpvalue* bl_vm_captureupvalue(VMState* vm, Value* local)
{
    ObjUpvalue* upvalue;
    ObjUpvalue* prevupvalue;
    ObjUpvalue* createdupvalue;
    prevupvalue = NULL;
    upvalue = vm->openupvalues;
    while(upvalue != NULL && upvalue->location > local)
    {
        prevupvalue = upvalue;
        upvalue = upvalue->next;
    }
    if(upvalue != NULL && upvalue->location == local)
    {
        return upvalue;
    }
    createdupvalue = bl_object_makeupvalue(vm, local);
    createdupvalue->next = upvalue;
    if(prevupvalue == NULL)
    {
        vm->openupvalues = createdupvalue;
    }
    else
    {
        prevupvalue->next = createdupvalue;
    }
    return createdupvalue;
}

static void bl_vm_closeupvalues(VMState* vm, const Value* last)
{
    ObjUpvalue* upvalue;
    while(vm->openupvalues != NULL && vm->openupvalues->location >= last)
    {
        upvalue = vm->openupvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        vm->openupvalues = upvalue->next;
    }
}

static void bl_vm_classdefmethod(VMState* vm, ObjString* name)
{
    Value method;
    ObjClass* klass;
    method = bl_vm_peekvalue(vm, 0);
    klass = AS_CLASS(bl_vm_peekvalue(vm, 1));
    bl_hashtable_set(vm, &klass->methods, OBJ_VAL(name), method);
    if(bl_vmutil_getmethodtype(method) == TYPE_INITIALIZER)
    {
        klass->initializer = method;
    }
    bl_vm_popvalue(vm);
}

static void bl_vm_classdefproperty(VMState* vm, ObjString* name, bool isstatic)
{
    Value property;
    ObjClass* klass;
    property = bl_vm_peekvalue(vm, 0);
    klass = AS_CLASS(bl_vm_peekvalue(vm, 1));
    if(!isstatic)
    {
        bl_hashtable_set(vm, &klass->properties, OBJ_VAL(name), property);
    }
    else
    {
        bl_hashtable_set(vm, &klass->staticproperties, OBJ_VAL(name), property);
    }
    bl_vm_popvalue(vm);
}


static inline ObjString* bl_vmdo_stringmultiply(VMState* vm, ObjString* str, double number)
{
    int i;
    int times;
    int totallength;
    char* result;
    times = (int)number;
    if(times <= 0)
    {
        // 'str' * 0 == '', 'str' * -1 == ''
        return bl_string_copystringlen(vm, "", 0);
    }
    else if(times == 1)
    {
        // 'str' * 1 == 'str'
        return str;
    }
    totallength = str->length * times;
    result = ALLOCATE(char, (size_t)totallength + 1);
    for(i = 0; i < times; i++)
    {
        memcpy(result + (str->length * i), str->chars, str->length);
    }
    result[totallength] = '\0';
    return bl_string_takestring(vm, result, totallength);
}

static inline ObjArray* bl_array_addarray(VMState* vm, ObjArray* a, ObjArray* b)
{
    int i;
    ObjArray* list;
    list = bl_object_makelist(vm);
    bl_vm_pushvalue(vm, OBJ_VAL(list));
    for(i = 0; i < a->items.count; i++)
    {
        bl_valarray_push(vm, &list->items, a->items.values[i]);
    }
    for(i = 0; i < b->items.count; i++)
    {
        bl_valarray_push(vm, &list->items, b->items.values[i]);
    }
    bl_vm_popvalue(vm);
    return list;
}

static inline void bl_vmdo_listmultiply(VMState* vm, ObjArray* a, ObjArray* newlist, int times)
{
    int i;
    int j;
    for(i = 0; i < times; i++)
    {
        for(j = 0; j < a->items.count; j++)
        {
            bl_valarray_push(vm, &newlist->items, a->items.values[j]);
        }
    }
}

static inline bool bl_vmdo_modulegetindex(VMState* vm, ObjModule* module, bool willassign)
{
    Value index;
    Value result;
    index = bl_vm_peekvalue(vm, 0);
    if(bl_hashtable_get(&module->values, index, &result))
    {
        if(!willassign)
        {
            bl_vm_popvaluen(vm, 2);// we can safely get rid of the index from the stack
        }
        bl_vm_pushvalue(vm, result);
        return true;
    }
    bl_vm_popvaluen(vm, 1);
    return bl_vm_throwexception(vm, false, "%s is undefined in module %s", bl_value_tostring(vm, index), module->name);
}

static inline bool bl_vmdo_stringgetindex(VMState* vm, ObjString* string, bool willassign)
{
    int end;
    int start;
    int index;
    int length;
    Value lower;
    lower = bl_vm_peekvalue(vm, 0);
    if(!bl_value_isnumber(lower))
    {
        bl_vm_popvaluen(vm, 1);
        return bl_vm_throwexception(vm, false, "strings are numerically indexed");
    }
    index = AS_NUMBER(lower);
    length = string->isascii ? string->length : string->utf8length;
    int realindex = index;
    if(index < 0)
    {
        index = length + index;
    }
    if(index < length && index >= 0)
    {
        start = index;
        end = index + 1;
        if(!string->isascii)
        {
            bl_util_utf8slice(string->chars, &start, &end);
        }
        if(!willassign)
        {
            // we can safely get rid of the index from the stack
            bl_vm_popvaluen(vm, 2);// +1 for the string itself
        }
        bl_vm_pushvalue(vm, STRING_L_VAL(string->chars + start, end - start));
        return true;
    }
    bl_vm_popvaluen(vm, 1);
    return bl_vm_throwexception(vm, false, "string index %d out of range", realindex);
}

static inline bool bl_vmdo_stringgetrangedindex(VMState* vm, ObjString* string, bool willassign)
{
    int end;
    int start;
    int length;
    int lowerindex;
    int upperindex;
    Value upper;
    Value lower;
    upper = bl_vm_peekvalue(vm, 0);
    lower = bl_vm_peekvalue(vm, 1);
    if(!(bl_value_isnil(lower) || bl_value_isnumber(lower)) || !(bl_value_isnumber(upper) || bl_value_isnil(upper)))
    {
        bl_vm_popvaluen(vm, 2);
        return bl_vm_throwexception(vm, false, "string are numerically indexed");
    }
    length = string->isascii ? string->length : string->utf8length;
    lowerindex = bl_value_isnumber(lower) ? AS_NUMBER(lower) : 0;
    upperindex = bl_value_isnil(upper) ? length : AS_NUMBER(upper);
    if(lowerindex < 0 || (upperindex < 0 && ((length + upperindex) < 0)))
    {
        // always return an empty string...
        if(!willassign)
        {
            bl_vm_popvaluen(vm, 3);// +1 for the string itself
        }
        bl_vm_pushvalue(vm, STRING_L_VAL("", 0));
        return true;
    }
    if(upperindex < 0)
    {
        upperindex = length + upperindex;
    }
    if(upperindex > length)
    {
        upperindex = length;
    }
    start = lowerindex;
    end = upperindex;
    if(!string->isascii)
    {
        bl_util_utf8slice(string->chars, &start, &end);
    }
    if(!willassign)
    {
        bl_vm_popvaluen(vm, 3);// +1 for the string itself
    }
    bl_vm_pushvalue(vm, STRING_L_VAL(string->chars + start, end - start));
    return true;
}

static inline bool bl_vmdo_bytesgetindex(VMState* vm, ObjBytes* bytes, bool willassign)
{
    int index;
    Value lower;
    lower = bl_vm_peekvalue(vm, 0);
    if(!bl_value_isnumber(lower))
    {
        bl_vm_popvaluen(vm, 1);
        return bl_vm_throwexception(vm, false, "bytes are numerically indexed");
    }
    index = AS_NUMBER(lower);
    int realindex = index;
    if(index < 0)
    {
        index = bytes->bytes.count + index;
    }
    if(index < bytes->bytes.count && index >= 0)
    {
        if(!willassign)
        {
            // we can safely get rid of the index from the stack
            bl_vm_popvaluen(vm, 2);// +1 for the bytes itself
        }
        bl_vm_pushvalue(vm, NUMBER_VAL((int)bytes->bytes.bytes[index]));
        return true;
    }
    else
    {
        bl_vm_popvaluen(vm, 1);
        return bl_vm_throwexception(vm, false, "bytes index %d out of range", realindex);
    }
}

static inline bool bl_vmdo_bytesgetrangedindex(VMState* vm, ObjBytes* bytes, bool willassign)
{
    Value upper = bl_vm_peekvalue(vm, 0);
    Value lower = bl_vm_peekvalue(vm, 1);
    if(!(bl_value_isnil(lower) || bl_value_isnumber(lower)) || !(bl_value_isnumber(upper) || bl_value_isnil(upper)))
    {
        bl_vm_popvaluen(vm, 2);
        return bl_vm_throwexception(vm, false, "bytes are numerically indexed");
    }
    int lowerindex = bl_value_isnumber(lower) ? AS_NUMBER(lower) : 0;
    int upperindex = bl_value_isnil(upper) ? bytes->bytes.count : AS_NUMBER(upper);
    if(lowerindex < 0 || (upperindex < 0 && ((bytes->bytes.count + upperindex) < 0)))
    {
        // always return an empty bytes...
        if(!willassign)
        {
            bl_vm_popvaluen(vm, 3);// +1 for the bytes itself
        }
        bl_vm_pushvalue(vm, OBJ_VAL(bl_object_makebytes(vm, 0)));
        return true;
    }
    if(upperindex < 0)
    {
        upperindex = bytes->bytes.count + upperindex;
    }
    if(upperindex > bytes->bytes.count)
    {
        upperindex = bytes->bytes.count;
    }
    if(!willassign)
    {
        bl_vm_popvaluen(vm, 3);// +1 for the list itself
    }
    bl_vm_pushvalue(vm, OBJ_VAL(bl_bytes_copybytes(vm, bytes->bytes.bytes + lowerindex, upperindex - lowerindex)));
    return true;
}

static inline bool bl_vmdo_listgetindex(VMState* vm, ObjArray* list, bool willassign)
{
    Value lower = bl_vm_peekvalue(vm, 0);
    if(!bl_value_isnumber(lower))
    {
        bl_vm_popvaluen(vm, 1);
        return bl_vm_throwexception(vm, false, "list are numerically indexed");
    }
    int index = AS_NUMBER(lower);
    int realindex = index;
    if(index < 0)
    {
        index = list->items.count + index;
    }
    if(index < list->items.count && index >= 0)
    {
        if(!willassign)
        {
            // we can safely get rid of the index from the stack
            bl_vm_popvaluen(vm, 2);// +1 for the list itself
        }
        bl_vm_pushvalue(vm, list->items.values[index]);
        return true;
    }
    else
    {
        bl_vm_popvaluen(vm, 1);
        return bl_vm_throwexception(vm, false, "list index %d out of range", realindex);
    }
}

static inline bool bl_vmdo_listgetrangedindex(VMState* vm, ObjArray* list, bool willassign)
{
    Value upper = bl_vm_peekvalue(vm, 0);
    Value lower = bl_vm_peekvalue(vm, 1);
    if(!(bl_value_isnil(lower) || bl_value_isnumber(lower)) || !(bl_value_isnumber(upper) || bl_value_isnil(upper)))
    {
        bl_vm_popvaluen(vm, 2);
        return bl_vm_throwexception(vm, false, "list are numerically indexed");
    }
    int lowerindex = bl_value_isnumber(lower) ? AS_NUMBER(lower) : 0;
    int upperindex = bl_value_isnil(upper) ? list->items.count : AS_NUMBER(upper);
    if(lowerindex < 0 || (upperindex < 0 && ((list->items.count + upperindex) < 0)))
    {
        // always return an empty list...
        if(!willassign)
        {
            bl_vm_popvaluen(vm, 3);// +1 for the list itself
        }
        bl_vm_pushvalue(vm, OBJ_VAL(bl_object_makelist(vm)));
        return true;
    }
    if(upperindex < 0)
    {
        upperindex = list->items.count + upperindex;
    }
    if(upperindex > list->items.count)
    {
        upperindex = list->items.count;
    }
    ObjArray* nlist = bl_object_makelist(vm);
    bl_vm_pushvalue(vm, OBJ_VAL(nlist));// gc protect
    for(int i = lowerindex; i < upperindex; i++)
    {
        bl_valarray_push(vm, &nlist->items, list->items.values[i]);
    }
    bl_vm_popvalue(vm);// clear gc protect
    if(!willassign)
    {
        bl_vm_popvaluen(vm, 3);// +1 for the list itself
    }
    bl_vm_pushvalue(vm, OBJ_VAL(nlist));
    return true;
}

static inline void bl_vmdo_modulesetindex(VMState* vm, ObjModule* module, Value index, Value value)
{
    bl_hashtable_set(vm, &module->values, index, value);
    bl_vm_popvaluen(vm, 3);// pop the value, index and dict out
    // leave the value on the stack for consumption
    // e.g. variable = dict[index] = 10
    bl_vm_pushvalue(vm, value);
}

static inline bool bl_vmdo_listsetindex(VMState* vm, ObjArray* list, Value index, Value value)
{
    if(!bl_value_isnumber(index))
    {
        bl_vm_popvaluen(vm, 3);// pop the value, index and list out
        return bl_vm_throwexception(vm, false, "list are numerically indexed");
    }
    int _position = AS_NUMBER(index);
    int position = _position < 0 ? list->items.count + _position : _position;
    if(position < list->items.count && position > -(list->items.count))
    {
        list->items.values[position] = value;
        bl_vm_popvaluen(vm, 3);// pop the value, index and list out
        // leave the value on the stack for consumption
        // e.g. variable = list[index] = 10
        bl_vm_pushvalue(vm, value);
        return true;
    }
    bl_vm_popvaluen(vm, 3);// pop the value, index and list out
    return bl_vm_throwexception(vm, false, "lists index %d out of range", _position);
}


static inline bool bl_vmdo_bytessetindex(VMState* vm, ObjBytes* bytes, Value index, Value value)
{
    if(!bl_value_isnumber(index))
    {
        bl_vm_popvaluen(vm, 3);// pop the value, index and bytes out
        return bl_vm_throwexception(vm, false, "bytes are numerically indexed");
    }
    else if(!bl_value_isnumber(value) || AS_NUMBER(value) < 0 || AS_NUMBER(value) > 255)
    {
        bl_vm_popvaluen(vm, 3);// pop the value, index and bytes out
        return bl_vm_throwexception(vm, false, "invalid byte. bytes are numbers between 0 and 255.");
    }
    int _position = AS_NUMBER(index);
    int byte = AS_NUMBER(value);
    int position = _position < 0 ? bytes->bytes.count + _position : _position;
    if(position < bytes->bytes.count && position > -(bytes->bytes.count))
    {
        bytes->bytes.bytes[position] = (unsigned char)byte;
        bl_vm_popvaluen(vm, 3);// pop the value, index and bytes out
        // leave the value on the stack for consumption
        // e.g. variable = bytes[index] = 10
        bl_vm_pushvalue(vm, value);
        return true;
    }
    bl_vm_popvaluen(vm, 3);// pop the value, index and bytes out
    return bl_vm_throwexception(vm, false, "bytes index %d out of range", _position);
}

static inline bool bl_vmdo_concat(VMState* vm)
{
    Value _b = bl_vm_peekvalue(vm, 0);
    Value _a = bl_vm_peekvalue(vm, 1);
    if(bl_value_isnil(_a))
    {
        bl_vm_popvaluen(vm, 2);
        bl_vm_pushvalue(vm, _b);
    }
    else if(bl_value_isnil(_b))
    {
        bl_vm_popvalue(vm);
    }
    else if(bl_value_isnumber(_a))
    {
        double a = AS_NUMBER(_a);
        char numstr[27];// + 1 for null terminator
        int numlength = sprintf(numstr, NUMBER_FORMAT, a);
        ObjString* b = AS_STRING(_b);
        int length = numlength + b->length;
        char* chars = ALLOCATE(char, (size_t)length + 1);
        memcpy(chars, numstr, numlength);
        memcpy(chars + numlength, b->chars, b->length);
        chars[length] = '\0';
        ObjString* result = bl_string_takestring(vm, chars, length);
        result->utf8length = numlength + b->utf8length;
        bl_vm_popvaluen(vm, 2);
        bl_vm_pushvalue(vm, OBJ_VAL(result));
    }
    else if(bl_value_isnumber(_b))
    {
        ObjString* a = AS_STRING(_a);
        double b = AS_NUMBER(_b);
        char numstr[27];// + 1 for null terminator
        int numlength = sprintf(numstr, NUMBER_FORMAT, b);
        int length = numlength + a->length;
        char* chars = ALLOCATE(char, (size_t)length + 1);
        memcpy(chars, a->chars, a->length);
        memcpy(chars + a->length, numstr, numlength);
        chars[length] = '\0';
        ObjString* result = bl_string_takestring(vm, chars, length);
        result->utf8length = numlength + a->utf8length;
        bl_vm_popvaluen(vm, 2);
        bl_vm_pushvalue(vm, OBJ_VAL(result));
    }
    else if(bl_value_isstring(_a) && bl_value_isstring(_b))
    {
        ObjString* b = AS_STRING(_b);
        ObjString* a = AS_STRING(_a);
        int length = a->length + b->length;
        char* chars = ALLOCATE(char, length + 1);
        memcpy(chars, a->chars, a->length);
        memcpy(chars + a->length, b->chars, b->length);
        chars[length] = '\0';
        ObjString* result = bl_string_takestring(vm, chars, length);
        result->utf8length = a->utf8length + b->utf8length;
        bl_vm_popvaluen(vm, 2);
        bl_vm_pushvalue(vm, OBJ_VAL(result));
    }
    else
    {
        return false;
    }
    return true;
}

static int bl_util_floordiv(double a, double b)
{
    int d = (int)a / (int)b;
    return d - ((d * b == a) & ((a < 0) ^ (b < 0)));
}

static double bl_util_modulo(double a, double b)
{
    double r = fmod(a, b);
    if(r != 0 && ((r < 0) != (b < 0)))
    {
        r += b;
    }
    return r;
}

static inline uint8_t READ_BYTE(CallFrame* frame)
{
    return *frame->ip++;
}

static inline uint16_t READ_SHORT(CallFrame* frame)
{
    frame->ip += 2;
    return (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]);
}

#define READ_CONSTANT(frame) (frame->closure->fnptr->blob.constants.values[READ_SHORT(frame)])

#define READ_STRING(frame) (AS_STRING(READ_CONSTANT(frame)))


static inline int vmutil_numtoint32(Value val)
{
    if(bl_value_isbool(val))
    {
        return (AS_BOOL(val) ? 1 : 0);
    }
    return bl_util_numbertoint32(AS_NUMBER(val));
}

static inline unsigned int vmutil_numtouint32(Value val)
{
    if(bl_value_isbool(val))
    {
        return (AS_BOOL(val) ? 1 : 0);
    }
    return bl_util_numbertouint32(AS_NUMBER(val));
}

static inline double bl_vmutil_tonum(Value val)
{
    if(bl_value_isbool(val))
    {
        return (AS_BOOL(val) ? 1 : 0);
    }
    return AS_NUMBER(val);
}

static inline long bl_vmutil_toint(Value val)
{
    if(bl_value_isbool(val))
    {
        return (AS_BOOL(val) ? 1 : 0);
    }
    return AS_NUMBER(val);
}

static inline bool bl_vmdo_binaryop(VMState* vm, CallFrame* frame, bool asbool, int op, VMBinaryCallbackFn fn)
{
    long intleft;
    long intright;
    double nval;
    double innleft;
    double innright;
    
    bool inbleft;
    bool inbright;
    Value val;
    Value invleft;
    Value invright;
    Value peekleft;
    Value peekright;
    peekleft = bl_vm_peekvalue(vm, 0);
    peekright = bl_vm_peekvalue(vm, 1);
    if((!bl_value_isnumber(peekleft) && !bl_value_isbool(peekleft)) || (!bl_value_isnumber(peekright) && !bl_value_isbool(peekright)))
    {
        runtime_error("unsupported operand %d for %s and %s", op, bl_value_typename(bl_vm_peekvalue(vm, 0)), bl_value_typename(bl_vm_peekvalue(vm, 1)));
        return false;
    }
    invright = bl_vm_popvalue(vm);
    invleft = bl_vm_popvalue(vm);
    if(fn != NULL)
    {
        innleft = bl_vmutil_tonum(invleft);
        innright = bl_vmutil_tonum(invright);
        nval = fn(innleft, innright);
    }
    else
    {
        switch(op)
        {
            case OP_ADD:
                {
                    innleft = bl_vmutil_tonum(invleft);
                    innright = bl_vmutil_tonum(invright);
                    nval = (innleft + innright);
                }
                break;
            case OP_SUBTRACT:
                {
                    innleft = bl_vmutil_tonum(invleft);
                    innright = bl_vmutil_tonum(invright);
                    nval = (innleft - innright);
                }
                break;
            case OP_MULTIPLY:
                {
                    innleft = bl_vmutil_tonum(invleft);
                    innright = bl_vmutil_tonum(invright);
                    nval = (innleft * innright);
                }
                break;
            case OP_DIVIDE:
                {
                    innleft = bl_vmutil_tonum(invleft);
                    innright = bl_vmutil_tonum(invright);
                    nval = (innleft / innright);
                }
                break;
            case OP_RIGHTSHIFT:
                {
                    int uleft;
                    unsigned int uright;
                    uleft = vmutil_numtoint32(invleft);
                    uright = vmutil_numtouint32(invright);
                    nval = uleft >> (uright & 0x1F);
                }
                break;
            case OP_LEFTSHIFT:
                {
                    int uleft;
                    unsigned int uright;
                    uleft = vmutil_numtoint32(invleft);
                    uright = vmutil_numtouint32(invright);
                    nval = uleft << (uright & 0x1F);
                }
                break;
            case OP_BITXOR:
                {
                    intleft = bl_vmutil_toint(invleft);
                    intright = bl_vmutil_toint(invright);
                    nval = (intleft ^ intright);
                }
                break;
            case OP_BITOR:
                {
                    intleft = bl_vmutil_toint(invleft);
                    intright = bl_vmutil_toint(invright);
                    nval = (intleft | intright);
                }
                break;
            case OP_BITAND:
                {
                    intleft = bl_vmutil_toint(invleft);
                    intright = bl_vmutil_toint(invright);
                    nval = (intleft & intright);
                }
                break;
            case OP_GREATERTHAN:
                {
                    innleft = bl_vmutil_tonum(invleft);
                    innright = bl_vmutil_tonum(invright);
                    nval = (innleft > innright);
                }
                break;
            case OP_LESSTHAN:
                {
                    innleft = bl_vmutil_tonum(invleft);
                    innright = bl_vmutil_tonum(invright);
                    nval = (innleft < innright);
                }
                break;
            default:
                {
                    fprintf(stderr, "missed an opcode here?\n");
                    assert(false);
                }
                break;
        }
    }
    if(asbool)
    {
        val = BOOL_VAL(nval);
    }
    else
    {
        val = NUMBER_VAL(nval);
    }
    bl_vm_pushvalue(vm, val);
    return true;
}


PtrResult bl_vm_run(VMState* vm)
{
    uint8_t instruction;
    CallFrame* frame;
    frame = &vm->frames[vm->framecount - 1];
    for(;;)
    {
        // try...finally... (i.e. try without a catch but a finally
        // but whose try body raises an exception)
        // can cause us to go into an invalid mode where frame count == 0
        // to fix this, we need to exit with an appropriate mode here.
        if(vm->framecount == 0)
        {
            return PTR_RUNTIME_ERR;
        }
        if(vm->shoulddebugstack)
        {
            printf("          ");
            for(Value* slot = vm->stack; slot < vm->stacktop; slot++)
            {
                printf("[ ");
                bl_value_printvalue(*slot);
                printf(" ]");
            }
            printf("\n");
            bl_blob_disassembleinst(&frame->closure->fnptr->blob, (int)(frame->ip - frame->closure->fnptr->blob.code));
        }
        switch(instruction = READ_BYTE(frame))
        {
            case OP_CONSTANT:
                {
                    Value constant = READ_CONSTANT(frame);
                    bl_vm_pushvalue(vm, constant);
                }
                break;
            case OP_ADD:
                {
                    if(bl_value_isstring(bl_vm_peekvalue(vm, 0)) || bl_value_isstring(bl_vm_peekvalue(vm, 1)))
                    {
                        if(!bl_vmdo_concat(vm))
                        {
                            runtime_error("unsupported operand + for %s and %s", bl_value_typename(bl_vm_peekvalue(vm, 0)), bl_value_typename(bl_vm_peekvalue(vm, 1)));
                            break;
                        }
                    }
                    else if(bl_value_isarray(bl_vm_peekvalue(vm, 0)) && bl_value_isarray(bl_vm_peekvalue(vm, 1)))
                    {
                        Value result = OBJ_VAL(bl_array_addarray(vm, AS_LIST(bl_vm_peekvalue(vm, 1)), AS_LIST(bl_vm_peekvalue(vm, 0))));
                        bl_vm_popvaluen(vm, 2);
                        bl_vm_pushvalue(vm, result);
                    }
                    else if(bl_value_isbytes(bl_vm_peekvalue(vm, 0)) && bl_value_isbytes(bl_vm_peekvalue(vm, 1)))
                    {
                        Value result = OBJ_VAL(bl_bytes_addbytes(vm, AS_BYTES(bl_vm_peekvalue(vm, 1)), AS_BYTES(bl_vm_peekvalue(vm, 0))));
                        bl_vm_popvaluen(vm, 2);
                        bl_vm_pushvalue(vm, result);
                    }
                    else
                    {
                        bl_vmdo_binaryop(vm, frame, false, OP_ADD, NULL);
                        break;
                    }
                }
                break;
            case OP_SUBTRACT:
                {
                    bl_vmdo_binaryop(vm, frame, false, OP_SUBTRACT, NULL);
                }
                break;
            case OP_MULTIPLY:
            {
                if(bl_value_isstring(bl_vm_peekvalue(vm, 1)) && bl_value_isnumber(bl_vm_peekvalue(vm, 0)))
                {
                    double number = AS_NUMBER(bl_vm_peekvalue(vm, 0));
                    ObjString* string = AS_STRING(bl_vm_peekvalue(vm, 1));
                    Value result = OBJ_VAL(bl_vmdo_stringmultiply(vm, string, number));
                    bl_vm_popvaluen(vm, 2);
                    bl_vm_pushvalue(vm, result);
                    break;
                }
                else if(bl_value_isarray(bl_vm_peekvalue(vm, 1)) && bl_value_isnumber(bl_vm_peekvalue(vm, 0)))
                {
                    int number = (int)AS_NUMBER(bl_vm_popvalue(vm));
                    ObjArray* list = AS_LIST(bl_vm_peekvalue(vm, 0));
                    ObjArray* nlist = bl_object_makelist(vm);
                    bl_vm_pushvalue(vm, OBJ_VAL(nlist));
                    bl_vmdo_listmultiply(vm, list, nlist, number);
                    bl_vm_popvaluen(vm, 2);
                    bl_vm_pushvalue(vm, OBJ_VAL(nlist));
                    break;
                }
                bl_vmdo_binaryop(vm, frame, false, OP_MULTIPLY, NULL);
                break;
            }
            case OP_DIVIDE:
            {
                bl_vmdo_binaryop(vm, frame, false, OP_DIVIDE, NULL);
                break;
            }
            case OP_REMINDER:
            {
                bl_vmdo_binaryop(vm, frame, false, OP_REMINDER, bl_util_modulo);
                break;
            }
            case OP_POW:
            {
                bl_vmdo_binaryop(vm, frame, false, OP_POW, pow);
                break;
            }
            case OP_F_DIVIDE:
            {
                bl_vmdo_binaryop(vm, frame, false, OP_F_DIVIDE, bl_util_floordiv);
                break;
            }
            case OP_NEGATE:
            {
                if(!bl_value_isnumber(bl_vm_peekvalue(vm, 0)))
                {
                    runtime_error("operator - not defined for object of type %s", bl_value_typename(bl_vm_peekvalue(vm, 0)));
                    break;
                }
                bl_vm_pushvalue(vm, NUMBER_VAL(-AS_NUMBER(bl_vm_popvalue(vm))));
                break;
            }
            case OP_BIT_NOT:
            {
                if(!bl_value_isnumber(bl_vm_peekvalue(vm, 0)))
                {
                    runtime_error("operator ~ not defined for object of type %s", bl_value_typename(bl_vm_peekvalue(vm, 0)));
                    break;
                }
                bl_vm_pushvalue(vm, INTEGER_VAL(~((int)AS_NUMBER(bl_vm_popvalue(vm)))));
                break;
            }
            case OP_BITAND:
            {
                bl_vmdo_binaryop(vm, frame, false, OP_BITAND, NULL);
                break;
            }
            case OP_BITOR:
            {
                bl_vmdo_binaryop(vm, frame, false, OP_BITOR, NULL);
                break;
            }
            case OP_BITXOR:
            {
                bl_vmdo_binaryop(vm, frame, false, OP_BITXOR, NULL);
                break;
            }
            case OP_LEFTSHIFT:
            {
                bl_vmdo_binaryop(vm, frame, false, OP_LEFTSHIFT, NULL);
                break;
            }
            case OP_RIGHTSHIFT:
            {
                bl_vmdo_binaryop(vm, frame, false, OP_RIGHTSHIFT, NULL);
                break;
            }
            case OP_ONE:
            {
                bl_vm_pushvalue(vm, NUMBER_VAL(1));
                break;
            }
                // comparisons
            case OP_EQUAL:
            {
                Value b = bl_vm_popvalue(vm);
                Value a = bl_vm_popvalue(vm);
                bl_vm_pushvalue(vm, BOOL_VAL(bl_value_valuesequal(a, b)));
                break;
            }
            case OP_GREATERTHAN:
            {
                bl_vmdo_binaryop(vm, frame, true, OP_GREATERTHAN, NULL);
                break;
            }
            case OP_LESSTHAN:
            {
                bl_vmdo_binaryop(vm, frame, true, OP_LESSTHAN, NULL);
                break;
            }
            case OP_NOT:
                bl_vm_pushvalue(vm, BOOL_VAL(bl_value_isfalse(bl_vm_popvalue(vm))));
                break;
            case OP_NIL:
                bl_vm_pushvalue(vm, NIL_VAL);
                break;
            case OP_EMPTY:
                bl_vm_pushvalue(vm, EMPTY_VAL);
                break;
            case OP_TRUE:
                bl_vm_pushvalue(vm, BOOL_VAL(true));
                break;
            case OP_FALSE:
                bl_vm_pushvalue(vm, BOOL_VAL(false));
                break;
            case OP_JUMP:
            {
                uint16_t offset = READ_SHORT(frame);
                frame->ip += offset;
                break;
            }
            case OP_JUMP_IF_FALSE:
            {
                uint16_t offset = READ_SHORT(frame);
                if(bl_value_isfalse(bl_vm_peekvalue(vm, 0)))
                {
                    frame->ip += offset;
                }
                break;
            }
            case OP_LOOP:
            {
                uint16_t offset = READ_SHORT(frame);
                frame->ip -= offset;
                break;
            }
            case OP_ECHO:
            {
                Value val = bl_vm_peekvalue(vm, 0);
                if(vm->isrepl)
                {
                    bl_value_echovalue(val);
                }
                else
                {
                    bl_value_printvalue(val);
                }
                if(!bl_value_isempty(val))
                {
                    printf("\n");
                }
                bl_vm_popvalue(vm);
                break;
            }
            case OP_STRINGIFY:
            {
                if(!bl_value_isstring(bl_vm_peekvalue(vm, 0)) && !bl_value_isnil(bl_vm_peekvalue(vm, 0)))
                {
                    char* value = bl_value_tostring(vm, bl_vm_popvalue(vm));
                    if((int)strlen(value) != 0)
                    {
                        bl_vm_pushvalue(vm, STRING_TT_VAL(value));
                    }
                    else
                    {
                        bl_vm_pushvalue(vm, NIL_VAL);
                    }
                }
                break;
            }
            case OP_DUP:
            {
                bl_vm_pushvalue(vm, bl_vm_peekvalue(vm, 0));
                break;
            }
            case OP_POP:
            {
                bl_vm_popvalue(vm);
                break;
            }
            case OP_POP_N:
            {
                bl_vm_popvaluen(vm, READ_SHORT(frame));
                break;
            }
            case OP_CLOSE_UP_VALUE:
            {
                bl_vm_closeupvalues(vm, vm->stacktop - 1);
                bl_vm_popvalue(vm);
                break;
            }
            case OP_DEFINE_GLOBAL:
            {
                ObjString* name = READ_STRING(frame);
                if(bl_value_isempty(bl_vm_peekvalue(vm, 0)))
                {
                    runtime_error("empty cannot be assigned");
                    break;
                }
                bl_hashtable_set(vm, &frame->closure->fnptr->module->values, OBJ_VAL(name), bl_vm_peekvalue(vm, 0));
                bl_vm_popvalue(vm);
#if defined(DEBUG_TABLE) && DEBUG_TABLE
                bl_hashtable_print(&vm->globals);
#endif
                break;
            }
            case OP_GET_GLOBAL:
            {
                ObjString* name = READ_STRING(frame);
                Value value;
                if(!bl_hashtable_get(&frame->closure->fnptr->module->values, OBJ_VAL(name), &value))
                {
                    if(!bl_hashtable_get(&vm->globals, OBJ_VAL(name), &value))
                    {
                        runtime_error("'%s' is undefined in this scope", name->chars);
                        break;
                    }
                }
                bl_vm_pushvalue(vm, value);
                break;
            }
            case OP_SET_GLOBAL:
            {
                if(bl_value_isempty(bl_vm_peekvalue(vm, 0)))
                {
                    runtime_error("empty cannot be assigned");
                    break;
                }
                ObjString* name = READ_STRING(frame);
                HashTable* table = &frame->closure->fnptr->module->values;
                if(bl_hashtable_set(vm, table, OBJ_VAL(name), bl_vm_peekvalue(vm, 0)))
                {
                    bl_hashtable_delete(table, OBJ_VAL(name));
                    runtime_error("%s is undefined in this scope", name->chars);
                    break;
                }
                break;
            }
            case OP_GET_LOCAL:
            {
                uint16_t slot = READ_SHORT(frame);
                bl_vm_pushvalue(vm, frame->slots[slot]);
                break;
            }
            case OP_SET_LOCAL:
            {
                uint16_t slot = READ_SHORT(frame);
                if(bl_value_isempty(bl_vm_peekvalue(vm, 0)))
                {
                    runtime_error("empty cannot be assigned");
                    break;
                }
                frame->slots[slot] = bl_vm_peekvalue(vm, 0);
                break;
            }
            case OP_GET_PROPERTY:
            {
                ObjString* name = READ_STRING(frame);
                if(bl_value_isobject(bl_vm_peekvalue(vm, 0)))
                {
                    Value value;
                    switch(AS_OBJ(bl_vm_peekvalue(vm, 0))->type)
                    {
                        case OBJ_MODULE:
                        {
                            ObjModule* module = AS_MODULE(bl_vm_peekvalue(vm, 0));
                            if(bl_hashtable_get(&module->values, OBJ_VAL(name), &value))
                            {
                                if(name->length > 0 && name->chars[0] == '_')
                                {
                                    runtime_error("cannot get private module property '%s'", name->chars);
                                    break;
                                }
                                bl_vm_popvalue(vm);// pop the list...
                                bl_vm_pushvalue(vm, value);
                                break;
                            }
                            runtime_error("%s module does not define '%s'", module->name, name->chars);
                            break;
                        }
                        case OBJ_CLASS:
                        {
                            if(bl_hashtable_get(&AS_CLASS(bl_vm_peekvalue(vm, 0))->methods, OBJ_VAL(name), &value))
                            {
                                if(bl_vmutil_getmethodtype(value) == TYPE_STATIC)
                                {
                                    if(name->length > 0 && name->chars[0] == '_')
                                    {
                                        runtime_error("cannot call private property '%s' of class %s", name->chars, AS_CLASS(bl_vm_peekvalue(vm, 0))->name->chars);
                                        break;
                                    }
                                    bl_vm_popvalue(vm);// pop the class...
                                    bl_vm_pushvalue(vm, value);
                                    break;
                                }
                            }
                            else if(bl_hashtable_get(&AS_CLASS(bl_vm_peekvalue(vm, 0))->staticproperties, OBJ_VAL(name), &value))
                            {
                                if(name->length > 0 && name->chars[0] == '_')
                                {
                                    runtime_error("cannot call private property '%s' of class %s", name->chars, AS_CLASS(bl_vm_peekvalue(vm, 0))->name->chars);
                                    break;
                                }
                                bl_vm_popvalue(vm);// pop the class...
                                bl_vm_pushvalue(vm, value);
                                break;
                            }
                            runtime_error("class %s does not have a static property or method named '%s'", AS_CLASS(bl_vm_peekvalue(vm, 0))->name->chars, name->chars);
                            break;
                        }
                        case OBJ_INSTANCE:
                        {
                            ObjInstance* instance = AS_INSTANCE(bl_vm_peekvalue(vm, 0));
                            if(bl_hashtable_get(&instance->properties, OBJ_VAL(name), &value))
                            {
                                if(name->length > 0 && name->chars[0] == '_')
                                {
                                    runtime_error("cannot call private property '%s' from instance of %s", name->chars, instance->klass->name->chars);
                                    break;
                                }
                                bl_vm_popvalue(vm);// pop the instance...
                                bl_vm_pushvalue(vm, value);
                                break;
                            }
                            if(name->length > 0 && name->chars[0] == '_')
                            {
                                runtime_error("cannot bind private property '%s' to instance of %s", name->chars, instance->klass->name->chars);
                                break;
                            }
                            if(bl_vmdo_classbindmethod(vm, instance->klass, name))
                            {
                                break;
                            }
                            runtime_error("instance of class %s does not have a property or method named '%s'",
                                          AS_INSTANCE(bl_vm_peekvalue(vm, 0))->klass->name->chars, name->chars);
                            break;
                        }
                        case OBJ_STRING:
                        {
                            if(bl_hashtable_get(&vm->classobjstring->methods, OBJ_VAL(name), &value))
                            {
                                bl_vm_popvalue(vm);// pop the list...
                                bl_vm_pushvalue(vm, value);
                                break;
                            }
                            runtime_error("class String has no named property '%s'", name->chars);
                            break;
                        }
                        case OBJ_ARRAY:
                        {
                            if(bl_hashtable_get(&vm->classobjlist->methods, OBJ_VAL(name), &value))
                            {
                                bl_vm_popvalue(vm);// pop the list...
                                bl_vm_pushvalue(vm, value);
                                break;
                            }
                            runtime_error("class List has no named property '%s'", name->chars);
                            break;
                        }
                        case OBJ_RANGE:
                        {
                            if(bl_hashtable_get(&vm->classobjrange->methods, OBJ_VAL(name), &value))
                            {
                                bl_vm_popvalue(vm);// pop the list...
                                bl_vm_pushvalue(vm, value);
                                break;
                            }
                            runtime_error("class Range has no named property '%s'", name->chars);
                            break;
                        }
                        case OBJ_DICT:
                        {
                            if(bl_hashtable_get(&AS_DICT(bl_vm_peekvalue(vm, 0))->items, OBJ_VAL(name), &value) || bl_hashtable_get(&vm->classobjdict->methods, OBJ_VAL(name), &value))
                            {
                                bl_vm_popvalue(vm);// pop the dictionary...
                                bl_vm_pushvalue(vm, value);
                                break;
                            }
                            runtime_error("unknown key or class Dict property '%s'", name->chars);
                            break;
                        }
                        case OBJ_BYTES:
                        {
                            if(bl_hashtable_get(&vm->classobjbytes->methods, OBJ_VAL(name), &value))
                            {
                                bl_vm_popvalue(vm);// pop the list...
                                bl_vm_pushvalue(vm, value);
                                break;
                            }
                            runtime_error("class Bytes has no named property '%s'", name->chars);
                            break;
                        }
                        case OBJ_FILE:
                        {
                            if(bl_hashtable_get(&vm->classobjfile->methods, OBJ_VAL(name), &value))
                            {
                                bl_vm_popvalue(vm);// pop the list...
                                bl_vm_pushvalue(vm, value);
                                break;
                            }
                            runtime_error("class File has no named property '%s'", name->chars);
                            break;
                        }
                        default:
                        {
                            runtime_error("object of type %s does not carry properties", bl_value_typename(bl_vm_peekvalue(vm, 0)));
                            break;
                        }
                    }
                }
                else
                {
                    runtime_error("'%s' of type %s does not have properties", bl_value_tostring(vm, bl_vm_peekvalue(vm, 0)), bl_value_typename(bl_vm_peekvalue(vm, 0)));
                    break;
                }
                break;
            }
            case OP_GET_SELF_PROPERTY:
            {
                ObjString* name = READ_STRING(frame);
                Value value;
                if(bl_value_isinstance(bl_vm_peekvalue(vm, 0)))
                {
                    ObjInstance* instance = AS_INSTANCE(bl_vm_peekvalue(vm, 0));
                    if(bl_hashtable_get(&instance->properties, OBJ_VAL(name), &value))
                    {
                        bl_vm_popvalue(vm);// pop the instance...
                        bl_vm_pushvalue(vm, value);
                        break;
                    }
                    if(bl_vmdo_classbindmethod(vm, instance->klass, name))
                    {
                        break;
                    }
                    runtime_error("instance of class %s does not have a property or method named '%s'", AS_INSTANCE(bl_vm_peekvalue(vm, 0))->klass->name->chars,
                                  name->chars);
                    break;
                }
                else if(bl_value_isclass(bl_vm_peekvalue(vm, 0)))
                {
                    ObjClass* klass = AS_CLASS(bl_vm_peekvalue(vm, 0));
                    if(bl_hashtable_get(&klass->methods, OBJ_VAL(name), &value))
                    {
                        if(bl_vmutil_getmethodtype(value) == TYPE_STATIC)
                        {
                            bl_vm_popvalue(vm);// pop the class...
                            bl_vm_pushvalue(vm, value);
                            break;
                        }
                    }
                    else if(bl_hashtable_get(&klass->staticproperties, OBJ_VAL(name), &value))
                    {
                        bl_vm_popvalue(vm);// pop the class...
                        bl_vm_pushvalue(vm, value);
                        break;
                    }
                    runtime_error("class %s does not have a static property or method named '%s'", klass->name->chars, name->chars);
                    break;
                }
                else if(bl_value_ismodule(bl_vm_peekvalue(vm, 0)))
                {
                    ObjModule* module = AS_MODULE(bl_vm_peekvalue(vm, 0));
                    if(bl_hashtable_get(&module->values, OBJ_VAL(name), &value))
                    {
                        bl_vm_popvalue(vm);// pop the class...
                        bl_vm_pushvalue(vm, value);
                        break;
                    }
                    runtime_error("module %s does not define '%s'", module->name, name->chars);
                    break;
                }
                runtime_error("'%s' of type %s does not have properties", bl_value_tostring(vm, bl_vm_peekvalue(vm, 0)), bl_value_typename(bl_vm_peekvalue(vm, 0)));
                break;
            }
            case OP_SET_PROPERTY:
            {
                if(!bl_value_isinstance(bl_vm_peekvalue(vm, 1)) && !bl_value_isdict(bl_vm_peekvalue(vm, 1)))
                {
                    runtime_error("object of type %s can not carry properties", bl_value_typename(bl_vm_peekvalue(vm, 1)));
                    break;
                }
                else if(bl_value_isempty(bl_vm_peekvalue(vm, 0)))
                {
                    runtime_error("empty cannot be assigned");
                    break;
                }
                ObjString* name = READ_STRING(frame);
                if(bl_value_isinstance(bl_vm_peekvalue(vm, 1)))
                {
                    ObjInstance* instance = AS_INSTANCE(bl_vm_peekvalue(vm, 1));
                    bl_hashtable_set(vm, &instance->properties, OBJ_VAL(name), bl_vm_peekvalue(vm, 0));
                    Value value = bl_vm_popvalue(vm);
                    bl_vm_popvalue(vm);// removing the instance object
                    bl_vm_pushvalue(vm, value);
                }
                else
                {
                    ObjDict* dict = AS_DICT(bl_vm_peekvalue(vm, 1));
                    bl_dict_setentry(vm, dict, OBJ_VAL(name), bl_vm_peekvalue(vm, 0));
                    Value value = bl_vm_popvalue(vm);
                    bl_vm_popvalue(vm);// removing the dictionary object
                    bl_vm_pushvalue(vm, value);
                }
                break;
            }
            case OP_CLOSURE:
            {
                ObjFunction* function = AS_FUNCTION(READ_CONSTANT(frame));
                ObjClosure* closure = bl_object_makeclosure(vm, function);
                bl_vm_pushvalue(vm, OBJ_VAL(closure));
                for(int i = 0; i < closure->upvaluecount; i++)
                {
                    uint8_t islocal = READ_BYTE(frame);
                    int index = READ_SHORT(frame);
                    if(islocal)
                    {
                        closure->upvalues[i] = bl_vm_captureupvalue(vm, frame->slots + index);
                    }
                    else
                    {
                        closure->upvalues[i] = ((ObjClosure*)frame->closure)->upvalues[index];
                    }
                }
                break;
            }
            case OP_GET_UP_VALUE:
            {
                int index = READ_SHORT(frame);
                bl_vm_pushvalue(vm, *((ObjClosure*)frame->closure)->upvalues[index]->location);
                break;
            }
            case OP_SET_UP_VALUE:
            {
                int index = READ_SHORT(frame);
                if(bl_value_isempty(bl_vm_peekvalue(vm, 0)))
                {
                    runtime_error("empty cannot be assigned");
                    break;
                }
                *((ObjClosure*)frame->closure)->upvalues[index]->location = bl_vm_peekvalue(vm, 0);
                break;
            }
            case OP_CALL:
            {
                int argcount = READ_BYTE(frame);
                if(!bl_vmdo_callvalue(vm, bl_vm_peekvalue(vm, argcount), argcount))
                {
                    EXIT_VM();
                }
                frame = &vm->frames[vm->framecount - 1];
                break;
            }
            case OP_INVOKE:
            {
                ObjString* method = READ_STRING(frame);
                int argcount = READ_BYTE(frame);
                if(!blade_vm_invokemethod(vm, method, argcount))
                {
                    EXIT_VM();
                }
                frame = &vm->frames[vm->framecount - 1];
                break;
            }
            case OP_INVOKE_SELF:
            {
                ObjString* method = READ_STRING(frame);
                int argcount = READ_BYTE(frame);
                if(!bl_instance_invokefromself(vm, method, argcount))
                {
                    EXIT_VM();
                }
                frame = &vm->frames[vm->framecount - 1];
                break;
            }
            case OP_CLASS:
            {
                ObjString* name = READ_STRING(frame);
                bl_vm_pushvalue(vm, OBJ_VAL(bl_object_makeclass(vm, name, NULL)));
                break;
            }
            case OP_METHOD:
            {
                ObjString* name = READ_STRING(frame);
                bl_vm_classdefmethod(vm, name);
                break;
            }
            case OP_CLASS_PROPERTY:
            {
                ObjString* name = READ_STRING(frame);
                int isstatic = READ_BYTE(frame);
                bl_vm_classdefproperty(vm, name, isstatic == 1);
                break;
            }
            case OP_INHERIT:
            {
                if(!bl_value_isclass(bl_vm_peekvalue(vm, 1)))
                {
                    runtime_error("cannot inherit from non-class object");
                    break;
                }
                ObjClass* superclass = AS_CLASS(bl_vm_peekvalue(vm, 1));
                ObjClass* subclass = AS_CLASS(bl_vm_peekvalue(vm, 0));
                bl_hashtable_addall(vm, &superclass->properties, &subclass->properties);
                bl_hashtable_addall(vm, &superclass->methods, &subclass->methods);
                subclass->superclass = superclass;
                bl_vm_popvalue(vm);// pop the subclass
                break;
            }
            case OP_GET_SUPER:
            {
                ObjString* name = READ_STRING(frame);
                ObjClass* klass = AS_CLASS(bl_vm_peekvalue(vm, 0));
                if(!bl_vmdo_classbindmethod(vm, klass->superclass, name))
                {
                    runtime_error("class %s does not define a function %s", klass->name->chars, name->chars);
                }
                break;
            }
            case OP_SUPER_INVOKE:
            {
                ObjString* method = READ_STRING(frame);
                int argcount = READ_BYTE(frame);
                ObjClass* klass = AS_CLASS(bl_vm_popvalue(vm));
                if(!bl_vmdo_instanceinvokefromclass(vm, klass, method, argcount))
                {
                    EXIT_VM();
                }
                frame = &vm->frames[vm->framecount - 1];
                break;
            }
            case OP_SUPER_INVOKE_SELF:
            {
                int argcount = READ_BYTE(frame);
                ObjClass* klass = AS_CLASS(bl_vm_popvalue(vm));
                if(!bl_vmdo_instanceinvokefromclass(vm, klass, klass->name, argcount))
                {
                    EXIT_VM();
                }
                frame = &vm->frames[vm->framecount - 1];
                break;
            }
            case OP_LIST:
            {
                int count = READ_SHORT(frame);
                ObjArray* list = bl_object_makelist(vm);
                vm->stacktop[-count - 1] = OBJ_VAL(list);
                for(int i = count - 1; i >= 0; i--)
                {
                    bl_array_push(vm, list, bl_vm_peekvalue(vm, i));
                }
                bl_vm_popvaluen(vm, count);
                break;
            }
            case OP_RANGE:
            {
                Value _upper = bl_vm_peekvalue(vm, 0);
                Value _lower = bl_vm_peekvalue(vm, 1);
                if(!bl_value_isnumber(_upper) || !bl_value_isnumber(_lower))
                {
                    runtime_error("invalid range boundaries");
                    break;
                }
                double lower = AS_NUMBER(_lower);
                double upper = AS_NUMBER(_upper);
                bl_vm_popvaluen(vm, 2);
                bl_vm_pushvalue(vm, OBJ_VAL(bl_object_makerange(vm, lower, upper)));
                break;
            }
            case OP_DICT:
            {
                int count = READ_SHORT(frame) * 2;// 1 for key, 1 for value
                ObjDict* dict = bl_object_makedict(vm);
                vm->stacktop[-count - 1] = OBJ_VAL(dict);
                for(int i = 0; i < count; i += 2)
                {
                    Value name = vm->stacktop[-count + i];
                    if(!bl_value_isstring(name) && !bl_value_isnumber(name) && !bl_value_isbool(name))
                    {
                        runtime_error("dictionary key must be one of string, number or boolean");
                    }
                    Value value = vm->stacktop[-count + i + 1];
                    bl_dict_addentry(vm, dict, name, value);
                }
                bl_vm_popvaluen(vm, count);
                break;
            }
            case OP_GET_RANGED_INDEX:
            {
                uint8_t willassign = READ_BYTE(frame);
                bool isgotten = true;
                if(bl_value_isobject(bl_vm_peekvalue(vm, 2)))
                {
                    switch(AS_OBJ(bl_vm_peekvalue(vm, 2))->type)
                    {
                        case OBJ_STRING:
                        {
                            if(!bl_vmdo_stringgetrangedindex(vm, AS_STRING(bl_vm_peekvalue(vm, 2)), willassign == (uint8_t)1))
                            {
                                EXIT_VM();
                            }
                            break;
                        }
                        case OBJ_ARRAY:
                        {
                            if(!bl_vmdo_listgetrangedindex(vm, AS_LIST(bl_vm_peekvalue(vm, 2)), willassign == (uint8_t)1))
                            {
                                EXIT_VM();
                            }
                            break;
                        }
                        case OBJ_BYTES:
                        {
                            if(!bl_vmdo_bytesgetrangedindex(vm, AS_BYTES(bl_vm_peekvalue(vm, 2)), willassign == (uint8_t)1))
                            {
                                EXIT_VM();
                            }
                            break;
                        }
                        default:
                        {
                            isgotten = false;
                            break;
                        }
                    }
                }
                else
                {
                    isgotten = false;
                }
                if(!isgotten)
                {
                    runtime_error("cannot range index object of type %s", bl_value_typename(bl_vm_peekvalue(vm, 2)));
                }
                break;
            }
            case OP_GET_INDEX:
            {
                uint8_t willassign = READ_BYTE(frame);
                bool isgotten = true;
                if(bl_value_isobject(bl_vm_peekvalue(vm, 1)))
                {
                    switch(AS_OBJ(bl_vm_peekvalue(vm, 1))->type)
                    {
                        case OBJ_STRING:
                        {
                            if(!bl_vmdo_stringgetindex(vm, AS_STRING(bl_vm_peekvalue(vm, 1)), willassign == (uint8_t)1))
                            {
                                EXIT_VM();
                            }
                            break;
                        }
                        case OBJ_ARRAY:
                        {
                            if(!bl_vmdo_listgetindex(vm, AS_LIST(bl_vm_peekvalue(vm, 1)), willassign == (uint8_t)1))
                            {
                                EXIT_VM();
                            }
                            break;
                        }
                        case OBJ_DICT:
                        {
                            if(!bl_vmdo_dictgetindex(vm, AS_DICT(bl_vm_peekvalue(vm, 1)), willassign == (uint8_t)1))
                            {
                                EXIT_VM();
                            }
                            break;
                        }
                        case OBJ_MODULE:
                        {
                            if(!bl_vmdo_modulegetindex(vm, AS_MODULE(bl_vm_peekvalue(vm, 1)), willassign == (uint8_t)1))
                            {
                                EXIT_VM();
                            }
                            break;
                        }
                        case OBJ_BYTES:
                        {
                            if(!bl_vmdo_bytesgetindex(vm, AS_BYTES(bl_vm_peekvalue(vm, 1)), willassign == (uint8_t)1))
                            {
                                EXIT_VM();
                            }
                            break;
                        }
                        default:
                        {
                            isgotten = false;
                            break;
                        }
                    }
                }
                else
                {
                    isgotten = false;
                }
                if(!isgotten)
                {
                    runtime_error("cannot index object of type %s", bl_value_typename(bl_vm_peekvalue(vm, 1)));
                }
                break;
            }
            case OP_SET_INDEX:
            {
                bool isset = true;
                if(bl_value_isobject(bl_vm_peekvalue(vm, 2)))
                {
                    Value value = bl_vm_peekvalue(vm, 0);
                    Value index = bl_vm_peekvalue(vm, 1);
                    if(bl_value_isempty(value))
                    {
                        runtime_error("empty cannot be assigned");
                        break;
                    }
                    switch(AS_OBJ(bl_vm_peekvalue(vm, 2))->type)
                    {
                        case OBJ_ARRAY:
                        {
                            if(!bl_vmdo_listsetindex(vm, AS_LIST(bl_vm_peekvalue(vm, 2)), index, value))
                            {
                                EXIT_VM();
                            }
                            break;
                        }
                        case OBJ_STRING:
                        {
                            runtime_error("strings do not support object assignment");
                            break;
                        }
                        case OBJ_DICT:
                        {
                            bl_vmdo_dictsetindex(vm, AS_DICT(bl_vm_peekvalue(vm, 2)), index, value);
                            break;
                        }
                        case OBJ_MODULE:
                        {
                            bl_vmdo_modulesetindex(vm, AS_MODULE(bl_vm_peekvalue(vm, 2)), index, value);
                            break;
                        }
                        case OBJ_BYTES:
                        {
                            if(!bl_vmdo_bytessetindex(vm, AS_BYTES(bl_vm_peekvalue(vm, 2)), index, value))
                            {
                                EXIT_VM();
                            }
                            break;
                        }
                        default:
                        {
                            isset = false;
                            break;
                        }
                    }
                }
                else
                {
                    isset = false;
                }
                if(!isset)
                {
                    runtime_error("type of %s is not a valid iterable", bl_value_typename(bl_vm_peekvalue(vm, 3)));
                }
                break;
            }
            case OP_RETURN:
            {
                Value result = bl_vm_popvalue(vm);
                bl_vm_closeupvalues(vm, frame->slots);
                vm->framecount--;
                if(vm->framecount == 0)
                {
                    bl_vm_popvalue(vm);
                    return PTR_OK;
                }
                vm->stacktop = frame->slots;
                bl_vm_pushvalue(vm, result);
                frame = &vm->frames[vm->framecount - 1];
                break;
            }
            case OP_CALL_IMPORT:
            {
                ObjClosure* closure = AS_CLOSURE(READ_CONSTANT(frame));
                bl_state_addmodule(vm, closure->fnptr->module);
                bl_vmdo_docall(vm, closure, 0);
                frame = &vm->frames[vm->framecount - 1];
                break;
            }
            case OP_NATIVE_MODULE:
            {
                ObjString* modulename = READ_STRING(frame);
                Value value;
                if(bl_hashtable_get(&vm->modules, OBJ_VAL(modulename), &value))
                {
                    ObjModule* module = AS_MODULE(value);
                    if(module->preloader != NULL)
                    {
                        ((ModLoaderFunc)module->preloader)(vm);
                    }
                    module->imported = true;
                    bl_hashtable_set(vm, &frame->closure->fnptr->module->values, OBJ_VAL(modulename), value);
                    break;
                }
                runtime_error("module '%s' not found", modulename->chars);
                break;
            }
            case OP_SELECT_IMPORT:
            {
                ObjString* entryname = READ_STRING(frame);
                ObjFunction* function = AS_CLOSURE(bl_vm_peekvalue(vm, 0))->fnptr;
                Value value;
                if(bl_hashtable_get(&function->module->values, OBJ_VAL(entryname), &value))
                {
                    bl_hashtable_set(vm, &frame->closure->fnptr->module->values, OBJ_VAL(entryname), value);
                }
                else
                {
                    runtime_error("module %s does not define '%s'", function->module->name, entryname->chars);
                }
                break;
            }
            case OP_SELECT_NATIVE_IMPORT:
            {
                ObjString* modulename = AS_STRING(bl_vm_peekvalue(vm, 0));
                ObjString* valuename = READ_STRING(frame);
                Value mod;
                if(bl_hashtable_get(&vm->modules, OBJ_VAL(modulename), &mod))
                {
                    ObjModule* module = AS_MODULE(mod);
                    Value value;
                    if(bl_hashtable_get(&module->values, OBJ_VAL(valuename), &value))
                    {
                        bl_hashtable_set(vm, &frame->closure->fnptr->module->values, OBJ_VAL(valuename), value);
                    }
                    else
                    {
                        runtime_error("module %s does not define '%s'", module->name, valuename->chars);
                    }
                }
                else
                {
                    runtime_error("module '%s' not found", modulename->chars);
                }
                break;
            }
            case OP_IMPORT_ALL:
            {
                bl_hashtable_addall(vm, &AS_CLOSURE(bl_vm_peekvalue(vm, 0))->fnptr->module->values, &frame->closure->fnptr->module->values);
                break;
            }
            case OP_IMPORT_ALL_NATIVE:
            {
                ObjString* name = AS_STRING(bl_vm_peekvalue(vm, 0));
                Value mod;
                if(bl_hashtable_get(&vm->modules, OBJ_VAL(name), &mod))
                {
                    bl_hashtable_addall(vm, &AS_MODULE(mod)->values, &frame->closure->fnptr->module->values);
                }
                break;
            }
            case OP_EJECT_IMPORT:
            {
                ObjFunction* function = AS_CLOSURE(READ_CONSTANT(frame))->fnptr;
                bl_hashtable_delete(&frame->closure->fnptr->module->values, STRING_VAL(function->module->name));
                break;
            }
            case OP_EJECT_NATIVE_IMPORT:
            {
                Value mod;
                ObjString* name = READ_STRING(frame);
                if(bl_hashtable_get(&vm->modules, OBJ_VAL(name), &mod))
                {
                    bl_hashtable_addall(vm, &AS_MODULE(mod)->values, &frame->closure->fnptr->module->values);
                    bl_hashtable_delete(&frame->closure->fnptr->module->values, OBJ_VAL(name));
                }
                break;
            }
            case OP_ASSERT:
            {
                Value message = bl_vm_popvalue(vm);
                Value expression = bl_vm_popvalue(vm);
                if(bl_value_isfalse(expression))
                {
                    if(!bl_value_isnil(message))
                    {
                        bl_vm_throwexception(vm, true, bl_value_tostring(vm, message));
                    }
                    else
                    {
                        bl_vm_throwexception(vm, true, "");
                    }
                }
                break;
            }
            case OP_DIE:
            {
                if(!bl_value_isinstance(bl_vm_peekvalue(vm, 0)) || !bl_class_isinstanceof(AS_INSTANCE(bl_vm_peekvalue(vm, 0))->klass, vm->exceptionclass->name->chars))
                {
                    runtime_error("instance of Exception expected");
                    break;
                }
                Value stacktrace = bl_vm_getstacktrace(vm);
                ObjInstance* instance = AS_INSTANCE(bl_vm_peekvalue(vm, 0));
                bl_hashtable_set(vm, &instance->properties, STRING_L_VAL("stacktrace", 10), stacktrace);
                if(bl_vm_propagateexception(vm, false))
                {
                    frame = &vm->frames[vm->framecount - 1];
                    break;
                }
                EXIT_VM();
            }
            case OP_TRY:
            {
                ObjString* type = READ_STRING(frame);
                uint16_t address = READ_SHORT(frame);
                uint16_t finallyaddress = READ_SHORT(frame);
                if(address != 0)
                {
                    Value value;
                    if(!bl_hashtable_get(&vm->globals, OBJ_VAL(type), &value) || !bl_value_isclass(value))
                    {
                        runtime_error("object of type '%s' is not an exception", type->chars);
                        break;
                    }
                    bl_vm_pushexceptionhandler(vm, AS_CLASS(value), address, finallyaddress);
                }
                else
                {
                    bl_vm_pushexceptionhandler(vm, NULL, address, finallyaddress);
                }
                break;
            }
            case OP_POP_TRY:
            {
                frame->handlerscount--;
                break;
            }
            case OP_PUBLISH_TRY:
            {
                frame->handlerscount--;
                if(bl_vm_propagateexception(vm, false))
                {
                    frame = &vm->frames[vm->framecount - 1];
                    break;
                }
                EXIT_VM();
            }
            case OP_SWITCH:
            {
                ObjSwitch* sw = AS_SWITCH(READ_CONSTANT(frame));
                Value expr = bl_vm_peekvalue(vm, 0);
                Value value;
                if(bl_hashtable_get(&sw->table, expr, &value))
                {
                    frame->ip += (int)AS_NUMBER(value);
                }
                else if(sw->defaultjump != -1)
                {
                    frame->ip += sw->defaultjump;
                }
                else
                {
                    frame->ip += sw->exitjump;
                }
                bl_vm_popvalue(vm);
                break;
            }
            case OP_CHOICE:
            {
                Value _else = bl_vm_peekvalue(vm, 0);
                Value _then = bl_vm_peekvalue(vm, 1);
                Value _condition = bl_vm_peekvalue(vm, 2);
                bl_vm_popvaluen(vm, 3);
                if(!bl_value_isfalse(_condition))
                {
                    bl_vm_pushvalue(vm, _then);
                }
                else
                {
                    bl_vm_pushvalue(vm, _else);
                }
                break;
            }
            default:
                break;
        }
    }
}


