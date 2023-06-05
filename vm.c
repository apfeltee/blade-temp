
#include "blade.h"

#define EXIT_VM() return PTR_RUNTIME_ERR

#define runtime_error(...) \
    { \
        if(!bl_vm_throwexception(vm, false, ##__VA_ARGS__)) \
        { \
            EXIT_VM(); \
        } \
    }



static inline bool bl_vmdo_docall(VMState* vm, ObjClosure* closure, int argcount);

static inline void bl_vmdo_pushvalue(VMState* vm, Value value)
{
    *vm->stacktop = value;
    vm->stacktop++;
}

void bl_vm_pushvalue(VMState* vm, Value value)
{
    return bl_vmdo_pushvalue(vm, value);
}

static inline Value bl_vmdo_popvalue(VMState* vm)
{
    vm->stacktop--;
    return *vm->stacktop;
}

Value bl_vm_popvalue(VMState* vm)
{
    return bl_vmdo_popvalue(vm);
}

static inline Value bl_vmdo_popvaluen(VMState* vm, int n)
{
    vm->stacktop -= n;
    return *vm->stacktop;
}

Value bl_vm_popvaluen(VMState* vm, int n)
{
    return bl_vmdo_popvaluen(vm, n);
}

static inline Value bl_vmdo_peekvalue(VMState* vm, int distance)
{
    return vm->stacktop[-1 - distance];
}

Value bl_vm_peekvalue(VMState* vm, int distance)
{
    return bl_vmdo_peekvalue(vm, distance);
}

void bl_vm_resetstack(VMState* vm)
{
    vm->stacktop = vm->stack;
    vm->framecount = 0;
    vm->openupvalues = NULL;
}

static inline ObjClass* bl_vmutil_makeclass(VMState* vm, const char* name, ObjClass* parent)
{
    ObjString* objstr;
    ObjClass* oclass;
    objstr = bl_string_copystring(vm, name);
    oclass = bl_object_makeclass(vm, objstr, parent);
    bl_state_defineglobal(vm, oclass->name, OBJ_VAL(oclass));
    return oclass;
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
    vm->classobjobject = bl_vmutil_makeclass(vm, "Object", NULL);
    vm->classobjstring = bl_vmutil_makeclass(vm, "String", vm->classobjobject);
    vm->classobjlist = bl_vmutil_makeclass(vm, "Array", vm->classobjobject);
    vm->classobjdict = bl_vmutil_makeclass(vm, "Dict", vm->classobjobject);
    vm->classobjfile = bl_vmutil_makeclass(vm, "File", vm->classobjobject);
    vm->classobjbytes = bl_vmutil_makeclass(vm, "Bytes", vm->classobjobject);
    vm->classobjrange = bl_vmutil_makeclass(vm, "Range", vm->classobjobject);
    vm->classobjmath = bl_vmutil_makeclass(vm, "Math", vm->classobjobject);
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
    bl_vmdo_pushvalue(vm, OBJ_VAL(function));
    closure = bl_object_makeclosure(vm, function);
    bl_vmdo_popvalue(vm);
    bl_vmdo_pushvalue(vm, OBJ_VAL(closure));
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

bool bl_vm_instanceinvokefromclass(VMState* vm, ObjClass* klass, ObjString* name, int argcount)
{
    Value method;
    if(bl_hashtable_get(&klass->methods, OBJ_VAL(name), &method))
    {
        if(bl_vmutil_getmethodtype(method) == TYPE_PRIVATE)
        {
            return bl_vm_throwexception(vm, false, "cannot call private method '%s' from instance of %s", name->chars, klass->name->chars);
        }
        return bl_vm_callvalue(vm, method, argcount);
    }
    return bl_vm_throwexception(vm, false, "undefined method '%s' in %s", name->chars, klass->name->chars);
}

static inline bool bl_vmdo_classbindmethod(VMState* vm, ObjClass* klass, ObjString* name)
{
    Value method;
    if(bl_hashtable_get(&klass->methods, OBJ_VAL(name), &method))
    {
        if(bl_vmutil_getmethodtype(method) == TYPE_PRIVATE)
        {
            return bl_vm_throwexception(vm, false, "cannot get private property '%s' from instance", name->chars);
        }
        ObjBoundMethod* bound = bl_object_makeboundmethod(vm, bl_vmdo_peekvalue(vm, 0), AS_CLOSURE(method));
        bl_vmdo_popvalue(vm);
        bl_vmdo_pushvalue(vm, OBJ_VAL(bound));
        return true;
    }
    return bl_vm_throwexception(vm, false, "undefined property '%s'", name->chars);
}


static inline bool bl_vmdo_dictgetindex(VMState* vm, ObjDict* dict, bool willassign)
{
    Value index;
    Value result;
    index = bl_vmdo_peekvalue(vm, 0);
    if(bl_dict_getentry(dict, index, &result))
    {
        if(!willassign)
        {
            bl_vmdo_popvaluen(vm, 2);// we can safely get rid of the index from the stack
        }
        bl_vmdo_pushvalue(vm, result);
        return true;
    }
    bl_vmdo_popvaluen(vm, 1);
    return bl_vm_throwexception(vm, false, "invalid index %s", bl_value_tostring(vm, index));
}

static inline void bl_vmdo_dictsetindex(VMState* vm, ObjDict* dict, Value index, Value value)
{
    bl_dict_setentry(vm, dict, index, value);
    // pop the value, index and dict out
    bl_vmdo_popvaluen(vm, 3);
    // leave the value on the stack for consumption
    // e.g. variable = dict[index] = 10
    bl_vmdo_pushvalue(vm, value);
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

static inline bool bl_vmdo_docall(VMState* vm, ObjClosure* closure, int argcount)
{
    int i;
    int vaargsstart;
    CallFrame* frame;
    ObjArray* argslist;
    // fill empty parameters if not variadic
    for(; !closure->fnptr->isvariadic && argcount < closure->fnptr->arity; argcount++)
    {
        bl_vmdo_pushvalue(vm, NIL_VAL);
    }
    // handle variadic arguments...
    if(closure->fnptr->isvariadic && argcount >= closure->fnptr->arity - 1)
    {
        vaargsstart = argcount - closure->fnptr->arity;
        argslist = bl_object_makelist(vm);
        bl_vmdo_pushvalue(vm, OBJ_VAL(argslist));
        for(i = vaargsstart; i >= 0; i--)
        {
            bl_valarray_push(vm, &argslist->items, bl_vmdo_peekvalue(vm, i + 1));
        }
        argcount -= vaargsstart;
        bl_vmdo_popvaluen(vm, vaargsstart + 2);// +1 for the gc protection push above
        bl_vmdo_pushvalue(vm, OBJ_VAL(argslist));
    }
    
    if(argcount != closure->fnptr->arity)
    {
        bl_vmdo_popvaluen(vm, argcount);
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
        bl_vmdo_popvaluen(vm, argcount);
        return bl_vm_throwexception(vm, false, "stack overflow");
    }
    frame = &vm->frames[vm->framecount++];
    frame->closure = closure;
    frame->ip = closure->fnptr->blob.code;
    frame->slots = vm->stacktop - argcount - 1;
    return true;
}

bool bl_vm_callvalue(VMState* vm, Value callee, int argcount)
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
                        return bl_vm_callvalue(vm, callable, argcount);
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
    receiver = bl_vmdo_peekvalue(vm, argcount);
    if(bl_value_isinstance(receiver))
    {
        instance = AS_INSTANCE(receiver);
        if(bl_hashtable_get(&instance->klass->methods, OBJ_VAL(name), &value))
        {
            return bl_vm_callvalue(vm, value, argcount);
        }
        if(bl_hashtable_get(&instance->properties, OBJ_VAL(name), &value))
        {
            vm->stacktop[-argcount - 1] = value;
            return bl_vm_callvalue(vm, value, argcount);
        }
    }
    else if(bl_value_isclass(receiver))
    {
        if(bl_hashtable_get(&AS_CLASS(receiver)->methods, OBJ_VAL(name), &value))
        {
            if(bl_vmutil_getmethodtype(value) == TYPE_STATIC)
            {
                return bl_vm_callvalue(vm, value, argcount);
            }
            return bl_vm_throwexception(vm, false, "cannot call non-static method %s() on non instance", name->chars);
        }
    }
    return bl_vm_throwexception(vm, false, "cannot call method %s on object of type %s", name->chars, bl_value_typename(receiver));
}

static inline bool bl_vm_invokemodulemethod(VMState* vm, ObjString* name, int argcount, Value receiver)
{
    Value value;
    ObjModule* module;
    module = AS_MODULE(receiver);
    if(bl_hashtable_get(&module->values, OBJ_VAL(name), &value))
    {
        if(name->length > 0 && name->chars[0] == '_')
        {
            return bl_vm_throwexception(vm, false, "cannot call private module method '%s'", name->chars);
        }
        return bl_vm_callvalue(vm, value, argcount);
    }
    return bl_vm_throwexception(vm, false, "module %s does not define class or method %s()", module->name, name->chars);
}

static inline bool bl_vm_invokeclassmethod(VMState* vm, ObjString* name, int argcount, Value receiver)
{
    Value value;
    if(bl_hashtable_get(&AS_CLASS(receiver)->methods, OBJ_VAL(name), &value))
    {
        if(bl_vmutil_getmethodtype(value) == TYPE_PRIVATE)
        {
            return bl_vm_throwexception(vm, false, "cannot call private method %s() on %s", name->chars, AS_CLASS(receiver)->name->chars);
        }
        return bl_vm_callvalue(vm, value, argcount);
    }
    else if(bl_hashtable_get(&AS_CLASS(receiver)->staticproperties, OBJ_VAL(name), &value))
    {
        return bl_vm_callvalue(vm, value, argcount);
    }
    return bl_vm_throwexception(vm, false, "unknown method %s() in class %s", name->chars, AS_CLASS(receiver)->name->chars);
}

static inline bool bl_vm_invokeinstancemethod(VMState* vm, ObjString* name, int argcount, Value receiver)
{
    Value value;
    ObjInstance* instance;
    instance = AS_INSTANCE(receiver);
    if(bl_hashtable_get(&instance->properties, OBJ_VAL(name), &value))
    {
        vm->stacktop[-argcount - 1] = value;
        return bl_vm_callvalue(vm, value, argcount);
    }
    return bl_vm_instanceinvokefromclass(vm, instance->klass, name, argcount);
}

static inline bool bl_vm_invokemethod(VMState* vm, ObjString* name, int argcount)
{
    Value value;
    Value receiver;
    Object* recobj;
    ObjClass* klass;
    klass = NULL;
    receiver = bl_vmdo_peekvalue(vm, argcount);
    if(!bl_value_isobject(receiver))
    {
        // @TODO: have methods for non objects as well.
        return bl_vm_throwexception(vm, false, "non-object %s has no methods", bl_value_typename(receiver));
    }
    else
    {
        recobj = AS_OBJ(receiver);
        if(recobj->type == OBJ_MODULE)
        {
            return bl_vm_invokemodulemethod(vm, name, argcount, receiver);
        }
        else if(recobj->type == OBJ_CLASS)
        {
            return bl_vm_invokeclassmethod(vm, name, argcount, receiver);
        }
        else if(recobj->type == OBJ_INSTANCE)
        {
            return bl_vm_invokeinstancemethod(vm, name, argcount, receiver);    
        }
        switch(recobj->type)
        {
            case OBJ_STRING:
                {
                    klass = vm->classobjstring;
                }
                break;
            case OBJ_ARRAY:
                {
                    klass = vm->classobjlist;
                }
                break;
            case OBJ_RANGE:
                {
                    klass = vm->classobjrange;
                }
                break;
            case OBJ_DICT:
                {
                    klass = vm->classobjdict;
                }
                break;
            case OBJ_FILE:
                {
                    klass = vm->classobjfile;
                }
                break;
            case OBJ_BYTES:
                {
                    klass = vm->classobjbytes;
                }
                break;
            default:
                {
                }
                break;
        }
        if(klass != NULL)
        {
            //if(bl_hashtable_get(&klass->methods, OBJ_VAL(name), &value))
            if(bl_class_getmethod(vm, klass, name, &value))
            {
                return bl_vmdo_callnativemethod(vm, AS_NATIVE(value), argcount);
            }
            return bl_vm_throwexception(vm, false, "class %s has no method %s()", klass->name->chars, name->chars);
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
    method = bl_vmdo_peekvalue(vm, 0);
    klass = AS_CLASS(bl_vmdo_peekvalue(vm, 1));
    bl_hashtable_set(vm, &klass->methods, OBJ_VAL(name), method);
    if(bl_vmutil_getmethodtype(method) == TYPE_INITIALIZER)
    {
        klass->initializer = method;
    }
    bl_vmdo_popvalue(vm);
}

static void bl_vm_classdefproperty(VMState* vm, ObjString* name, bool isstatic)
{
    Value property;
    ObjClass* klass;
    property = bl_vmdo_peekvalue(vm, 0);
    klass = AS_CLASS(bl_vmdo_peekvalue(vm, 1));
    if(!isstatic)
    {
        bl_hashtable_set(vm, &klass->properties, OBJ_VAL(name), property);
    }
    else
    {
        bl_hashtable_set(vm, &klass->staticproperties, OBJ_VAL(name), property);
    }
    bl_vmdo_popvalue(vm);
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
    bl_vmdo_pushvalue(vm, OBJ_VAL(list));
    for(i = 0; i < a->items.count; i++)
    {
        bl_valarray_push(vm, &list->items, a->items.values[i]);
    }
    for(i = 0; i < b->items.count; i++)
    {
        bl_valarray_push(vm, &list->items, b->items.values[i]);
    }
    bl_vmdo_popvalue(vm);
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
    index = bl_vmdo_peekvalue(vm, 0);
    if(bl_hashtable_get(&module->values, index, &result))
    {
        if(!willassign)
        {
            bl_vmdo_popvaluen(vm, 2);// we can safely get rid of the index from the stack
        }
        bl_vmdo_pushvalue(vm, result);
        return true;
    }
    bl_vmdo_popvaluen(vm, 1);
    return bl_vm_throwexception(vm, false, "%s is undefined in module %s", bl_value_tostring(vm, index), module->name);
}

static inline bool bl_vmdo_stringgetindex(VMState* vm, ObjString* string, bool willassign)
{
    int end;
    int start;
    int index;
    int length;
    int realindex;
    Value lower;
    lower = bl_vmdo_peekvalue(vm, 0);
    if(!bl_value_isnumber(lower))
    {
        bl_vmdo_popvaluen(vm, 1);
        return bl_vm_throwexception(vm, false, "strings are numerically indexed");
    }
    index = AS_NUMBER(lower);
    length = string->isascii ? string->length : string->utf8length;
    realindex = index;
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
            bl_vmdo_popvaluen(vm, 2);// +1 for the string itself
        }
        bl_vmdo_pushvalue(vm, STRING_L_VAL(string->chars + start, end - start));
        return true;
    }
    bl_vmdo_popvaluen(vm, 1);
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
    upper = bl_vmdo_peekvalue(vm, 0);
    lower = bl_vmdo_peekvalue(vm, 1);
    if(!(bl_value_isnil(lower) || bl_value_isnumber(lower)) || !(bl_value_isnumber(upper) || bl_value_isnil(upper)))
    {
        bl_vmdo_popvaluen(vm, 2);
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
            bl_vmdo_popvaluen(vm, 3);// +1 for the string itself
        }
        bl_vmdo_pushvalue(vm, STRING_L_VAL("", 0));
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
        bl_vmdo_popvaluen(vm, 3);// +1 for the string itself
    }
    bl_vmdo_pushvalue(vm, STRING_L_VAL(string->chars + start, end - start));
    return true;
}

static inline bool bl_vmdo_bytesgetindex(VMState* vm, ObjBytes* bytes, bool willassign)
{
    int index;
    int realindex;
    Value lower;
    lower = bl_vmdo_peekvalue(vm, 0);
    if(!bl_value_isnumber(lower))
    {
        bl_vmdo_popvaluen(vm, 1);
        return bl_vm_throwexception(vm, false, "bytes are numerically indexed");
    }
    index = AS_NUMBER(lower);
    realindex = index;
    if(index < 0)
    {
        index = bytes->bytes.count + index;
    }
    if(index < bytes->bytes.count && index >= 0)
    {
        if(!willassign)
        {
            // we can safely get rid of the index from the stack
            bl_vmdo_popvaluen(vm, 2);// +1 for the bytes itself
        }
        bl_vmdo_pushvalue(vm, NUMBER_VAL((int)bytes->bytes.bytes[index]));
        return true;
    }
    bl_vmdo_popvaluen(vm, 1);
    return bl_vm_throwexception(vm, false, "bytes index %d out of range", realindex);
}

static inline bool bl_vmdo_bytesgetrangedindex(VMState* vm, ObjBytes* bytes, bool willassign)
{
    int upperindex;
    int lowerindex;
    Value upper;
    Value lower;
    upper = bl_vmdo_peekvalue(vm, 0);
    lower = bl_vmdo_peekvalue(vm, 1);
    if(!(bl_value_isnil(lower) || bl_value_isnumber(lower)) || !(bl_value_isnumber(upper) || bl_value_isnil(upper)))
    {
        bl_vmdo_popvaluen(vm, 2);
        return bl_vm_throwexception(vm, false, "bytes are numerically indexed");
    }
    lowerindex = bl_value_isnumber(lower) ? AS_NUMBER(lower) : 0;
    upperindex = bl_value_isnil(upper) ? bytes->bytes.count : AS_NUMBER(upper);
    if(lowerindex < 0 || (upperindex < 0 && ((bytes->bytes.count + upperindex) < 0)))
    {
        // always return an empty bytes...
        if(!willassign)
        {
            bl_vmdo_popvaluen(vm, 3);// +1 for the bytes itself
        }
        bl_vmdo_pushvalue(vm, OBJ_VAL(bl_object_makebytes(vm, 0)));
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
        bl_vmdo_popvaluen(vm, 3);// +1 for the list itself
    }
    bl_vmdo_pushvalue(vm, OBJ_VAL(bl_bytes_copybytes(vm, bytes->bytes.bytes + lowerindex, upperindex - lowerindex)));
    return true;
}

static inline bool bl_vmdo_listgetindex(VMState* vm, ObjArray* list, bool willassign)
{
    int index;
    int realindex;
    Value lower;
    lower = bl_vmdo_peekvalue(vm, 0);
    if(!bl_value_isnumber(lower))
    {
        bl_vmdo_popvaluen(vm, 1);
        return bl_vm_throwexception(vm, false, "list are numerically indexed");
    }
    index = AS_NUMBER(lower);
    realindex = index;
    if(index < 0)
    {
        index = list->items.count + index;
    }
    if(index < list->items.count && index >= 0)
    {
        if(!willassign)
        {
            // we can safely get rid of the index from the stack
            bl_vmdo_popvaluen(vm, 2);// +1 for the list itself
        }
        bl_vmdo_pushvalue(vm, list->items.values[index]);
        return true;
    }    
    bl_vmdo_popvaluen(vm, 1);
    return bl_vm_throwexception(vm, false, "list index %d out of range", realindex);
}

static inline bool bl_vmdo_listgetrangedindex(VMState* vm, ObjArray* list, bool willassign)
{
    int i;
    int upperindex;
    int lowerindex;
    Value upper;
    Value lower;
    ObjArray* nlist;
    upper = bl_vmdo_peekvalue(vm, 0);
    lower = bl_vmdo_peekvalue(vm, 1);
    if(!(bl_value_isnil(lower) || bl_value_isnumber(lower)) || !(bl_value_isnumber(upper) || bl_value_isnil(upper)))
    {
        bl_vmdo_popvaluen(vm, 2);
        return bl_vm_throwexception(vm, false, "list are numerically indexed");
    }
    lowerindex = bl_value_isnumber(lower) ? AS_NUMBER(lower) : 0;
    upperindex = bl_value_isnil(upper) ? list->items.count : AS_NUMBER(upper);
    if(lowerindex < 0 || (upperindex < 0 && ((list->items.count + upperindex) < 0)))
    {
        // always return an empty list...
        if(!willassign)
        {
            bl_vmdo_popvaluen(vm, 3);// +1 for the list itself
        }
        bl_vmdo_pushvalue(vm, OBJ_VAL(bl_object_makelist(vm)));
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
    nlist = bl_object_makelist(vm);
    bl_vmdo_pushvalue(vm, OBJ_VAL(nlist));// gc protect
    for(i = lowerindex; i < upperindex; i++)
    {
        bl_valarray_push(vm, &nlist->items, list->items.values[i]);
    }
    bl_vmdo_popvalue(vm);// clear gc protect
    if(!willassign)
    {
        bl_vmdo_popvaluen(vm, 3);// +1 for the list itself
    }
    bl_vmdo_pushvalue(vm, OBJ_VAL(nlist));
    return true;
}

static inline void bl_vmdo_modulesetindex(VMState* vm, ObjModule* module, Value index, Value value)
{
    bl_hashtable_set(vm, &module->values, index, value);
    // pop the value, index and dict out
    bl_vmdo_popvaluen(vm, 3);
    // leave the value on the stack for consumption
    // e.g. variable = dict[index] = 10
    bl_vmdo_pushvalue(vm, value);
}

static inline bool bl_vmdo_listsetindex(VMState* vm, ObjArray* list, Value index, Value value)
{
    int rawpos;
    int position;
    if(!bl_value_isnumber(index))
    {
        // pop the value, index and list out
        bl_vmdo_popvaluen(vm, 3);
        return bl_vm_throwexception(vm, false, "list are numerically indexed");
    }
    rawpos = AS_NUMBER(index);
    position = rawpos < 0 ? list->items.count + rawpos : rawpos;
    if(position < list->items.count && position > -(list->items.count))
    {
        list->items.values[position] = value;
        // pop the value, index and list out
        bl_vmdo_popvaluen(vm, 3);
        // leave the value on the stack for consumption
        // e.g. variable = list[index] = 10
        bl_vmdo_pushvalue(vm, value);
        return true;
    }
    // pop the value, index and list out
    bl_vmdo_popvaluen(vm, 3);
    return bl_vm_throwexception(vm, false, "lists index %d out of range", rawpos);
}


static inline bool bl_vmdo_bytessetindex(VMState* vm, ObjBytes* bytes, Value index, Value value)
{
    int byte;
    int rawpos;
    int position;
    if(!bl_value_isnumber(index))
    {
        // pop the value, index and bytes out
        bl_vmdo_popvaluen(vm, 3);
        return bl_vm_throwexception(vm, false, "bytes are numerically indexed");
    }
    else if(!bl_value_isnumber(value) || AS_NUMBER(value) < 0 || AS_NUMBER(value) > 255)
    {
        // pop the value, index and bytes out
        bl_vmdo_popvaluen(vm, 3);
        return bl_vm_throwexception(vm, false, "invalid byte. bytes are numbers between 0 and 255.");
    }
    rawpos = AS_NUMBER(index);
    byte = AS_NUMBER(value);
    position = rawpos < 0 ? bytes->bytes.count + rawpos : rawpos;
    if(position < bytes->bytes.count && position > -(bytes->bytes.count))
    {
        bytes->bytes.bytes[position] = (unsigned char)byte;
        // pop the value, index and bytes out
        bl_vmdo_popvaluen(vm, 3);
        // leave the value on the stack for consumption
        // e.g. variable = bytes[index] = 10
        bl_vmdo_pushvalue(vm, value);
        return true;
    }
    // pop the value, index and bytes out
    bl_vmdo_popvaluen(vm, 3);
    return bl_vm_throwexception(vm, false, "bytes index %d out of range", rawpos);
}

static inline bool bl_vmdo_concatvalues(VMState* vm)
{
    int length;
    int numlength;
    double numa;
    double numb;
    char* chars;
    Value vala;
    Value valb;
    ObjString* stra;
    ObjString* strb;
    ObjString* result;
    char numstr[27];// + 1 for null terminator
    valb = bl_vmdo_peekvalue(vm, 0);
    vala = bl_vmdo_peekvalue(vm, 1);
    if(bl_value_isnil(vala))
    {
        bl_vmdo_popvaluen(vm, 2);
        bl_vmdo_pushvalue(vm, valb);
    }
    else if(bl_value_isnil(valb))
    {
        bl_vmdo_popvalue(vm);
    }
    else if(bl_value_isnumber(vala))
    {
        numa = AS_NUMBER(vala);
        numlength = sprintf(numstr, NUMBER_FORMAT, numa);
        strb = AS_STRING(valb);
        length = numlength + strb->length;
        chars = ALLOCATE(char, (size_t)length + 1);
        memcpy(chars, numstr, numlength);
        memcpy(chars + numlength, strb->chars, strb->length);
        chars[length] = '\0';
        result = bl_string_takestring(vm, chars, length);
        result->utf8length = numlength + strb->utf8length;
        bl_vmdo_popvaluen(vm, 2);
        bl_vmdo_pushvalue(vm, OBJ_VAL(result));
    }
    else if(bl_value_isnumber(valb))
    {
        stra = AS_STRING(vala);
        numb = AS_NUMBER(valb);
        numlength = sprintf(numstr, NUMBER_FORMAT, numb);
        length = numlength + stra->length;
        chars = ALLOCATE(char, (size_t)length + 1);
        memcpy(chars, stra->chars, stra->length);
        memcpy(chars + stra->length, numstr, numlength);
        chars[length] = '\0';
        result = bl_string_takestring(vm, chars, length);
        result->utf8length = numlength + stra->utf8length;
        bl_vmdo_popvaluen(vm, 2);
        bl_vmdo_pushvalue(vm, OBJ_VAL(result));
    }
    else if(bl_value_isstring(vala) && bl_value_isstring(valb))
    {
        strb = AS_STRING(valb);
        stra = AS_STRING(vala);
        length = stra->length + strb->length;
        chars = ALLOCATE(char, length + 1);
        memcpy(chars, stra->chars, stra->length);
        memcpy(chars + stra->length, strb->chars, strb->length);
        chars[length] = '\0';
        result = bl_string_takestring(vm, chars, length);
        result->utf8length = stra->utf8length + strb->utf8length;
        bl_vmdo_popvaluen(vm, 2);
        bl_vmdo_pushvalue(vm, OBJ_VAL(result));
    }
    else
    {
        return false;
    }
    return true;
}

static double bl_util_floordiv(double a, double b)
{
    int d;
    d = (int)a / (int)b;
    return d - ((d * b == a) & ((a < 0) ^ (b < 0)));
}

static double bl_util_modulo(double a, double b)
{
    double r;
    r = fmod(a, b);
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

static inline Value READ_CONSTANT(CallFrame* frame)
{
    return frame->closure->fnptr->blob.constants.values[READ_SHORT(frame)];
}

static inline ObjString* READ_STRING(CallFrame* frame)
{
    return AS_STRING(READ_CONSTANT(frame));
}

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

static inline PtrResult bl_vmdo_binaryop(VMState* vm, CallFrame* frame, bool asbool, int op, VMBinaryCallbackFn fn)
{
    long leftint;
    long rightint;
    double numres;
    double leftflt;
    double rightflt;
    int leftsigned;
    unsigned int rightusigned;
    Value resval;
    Value leftinval;
    Value rightinval;
    (void)frame;
    rightinval = bl_vmdo_popvalue(vm);
    leftinval = bl_vmdo_popvalue(vm);
    if((!bl_value_isnumber(leftinval) && !bl_value_isbool(leftinval)) || (!bl_value_isnumber(rightinval) && !bl_value_isbool(rightinval)))
    {
        runtime_error("unsupported operand %d for %s and %s", op, bl_value_typename(leftinval), bl_value_typename(rightinval));
    }
    if(fn != NULL)
    {
        leftflt = bl_vmutil_tonum(leftinval);
        rightflt = bl_vmutil_tonum(rightinval);
        numres = fn(leftflt, rightflt);
    }
    else
    {
        switch(op)
        {
            case OP_ADD:
                {
                    leftflt = bl_vmutil_tonum(leftinval);
                    rightflt = bl_vmutil_tonum(rightinval);
                    numres = (leftflt + rightflt);
                }
                break;
            case OP_SUBTRACT:
                {
                    leftflt = bl_vmutil_tonum(leftinval);
                    rightflt = bl_vmutil_tonum(rightinval);
                    numres = (leftflt - rightflt);
                }
                break;
            case OP_MULTIPLY:
                {
                    leftflt = bl_vmutil_tonum(leftinval);
                    rightflt = bl_vmutil_tonum(rightinval);
                    numres = (leftflt * rightflt);
                }
                break;
            case OP_DIVIDE:
                {
                    leftflt = bl_vmutil_tonum(leftinval);
                    rightflt = bl_vmutil_tonum(rightinval);
                    numres = (leftflt / rightflt);
                }
                break;
            case OP_RIGHTSHIFT:
                {
                    leftsigned = vmutil_numtoint32(leftinval);
                    rightusigned = vmutil_numtouint32(rightinval);
                    numres = leftsigned >> (rightusigned & 0x1F);
                }
                break;
            case OP_LEFTSHIFT:
                {
                    leftsigned = vmutil_numtoint32(leftinval);
                    rightusigned = vmutil_numtouint32(rightinval);
                    numres = leftsigned << (rightusigned & 0x1F);
                }
                break;
            case OP_BITXOR:
                {
                    leftint = bl_vmutil_toint(leftinval);
                    rightint = bl_vmutil_toint(rightinval);
                    numres = (leftint ^ rightint);
                }
                break;
            case OP_BITOR:
                {
                    leftint = bl_vmutil_toint(leftinval);
                    rightint = bl_vmutil_toint(rightinval);
                    numres = (leftint | rightint);
                }
                break;
            case OP_BITAND:
                {
                    leftint = bl_vmutil_toint(leftinval);
                    rightint = bl_vmutil_toint(rightinval);
                    numres = (leftint & rightint);
                }
                break;
            case OP_GREATERTHAN:
                {
                    leftflt = bl_vmutil_tonum(leftinval);
                    rightflt = bl_vmutil_tonum(rightinval);
                    numres = (leftflt > rightflt);
                }
                break;
            case OP_LESSTHAN:
                {
                    leftflt = bl_vmutil_tonum(leftinval);
                    rightflt = bl_vmutil_tonum(rightinval);
                    numres = (leftflt < rightflt);
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
        resval = BOOL_VAL(numres);
    }
    else
    {
        resval = NUMBER_VAL(numres);
    }
    bl_vmdo_pushvalue(vm, resval);
    return PTR_OK;
}

PtrResult bl_vmdo_rungetproperty(VMState* vm, CallFrame* frame)
{
    Value value;
    Value peeked;
    Object* peekobj;
    ObjClass* klass;
    ObjString* name;
    klass = NULL;
    name = READ_STRING(frame);
    peeked = bl_vmdo_peekvalue(vm, 0);
    if(bl_value_isobject(peeked))
    {
        peekobj = AS_OBJ(peeked);
        if(peekobj->type == OBJ_MODULE)
        {
            ObjModule* module = AS_MODULE(peeked);
            if(bl_hashtable_get(&module->values, OBJ_VAL(name), &value))
            {
                if(name->length > 0 && name->chars[0] == '_')
                {
                    runtime_error("cannot get private module property '%s'", name->chars);
                    //return PTR_RUNTIME_ERR;
                }
                bl_vmdo_popvalue(vm);
                bl_vmdo_pushvalue(vm, value);
                return PTR_OK;
            }
            runtime_error("%s module does not define '%s'", module->name, name->chars);
            //return PTR_RUNTIME_ERR;
        }
        else if(peekobj->type == OBJ_CLASS)
        {
            if(bl_hashtable_get(&AS_CLASS(peeked)->methods, OBJ_VAL(name), &value))
            {
                if(bl_vmutil_getmethodtype(value) == TYPE_STATIC)
                {
                    if(name->length > 0 && name->chars[0] == '_')
                    {
                        runtime_error("cannot call private property '%s' of class %s", name->chars, AS_CLASS(peeked)->name->chars);
                        //return PTR_RUNTIME_ERR;
                    }
                    // pop the class...
                    bl_vmdo_popvalue(vm);
                    bl_vmdo_pushvalue(vm, value);
                    return PTR_OK;
                }
            }
            else if(bl_hashtable_get(&AS_CLASS(peeked)->staticproperties, OBJ_VAL(name), &value))
            {
                if(name->length > 0 && name->chars[0] == '_')
                {
                    runtime_error("cannot call private property '%s' of class %s", name->chars, AS_CLASS(peeked)->name->chars);
                    //return PTR_RUNTIME_ERR;
                }
                // pop the class...
                bl_vmdo_popvalue(vm);
                bl_vmdo_pushvalue(vm, value);
                return PTR_OK;
            }
            runtime_error("class %s does not have a static property or method named '%s'", AS_CLASS(peeked)->name->chars, name->chars);
            return PTR_OK;
        }
        else if(peekobj->type == OBJ_INSTANCE)
        {
            ObjInstance* instance = AS_INSTANCE(peeked);
            if(bl_hashtable_get(&instance->properties, OBJ_VAL(name), &value))
            {
                if(name->length > 0 && name->chars[0] == '_')
                {
                    runtime_error("cannot call private property '%s' from instance of %s", name->chars, instance->klass->name->chars);
                    //return PTR_RUNTIME_ERR;
                }
                // pop the instance...
                bl_vmdo_popvalue(vm);
                bl_vmdo_pushvalue(vm, value);
                return PTR_OK;
            }
            if(name->length > 0 && name->chars[0] == '_')
            {
                runtime_error("cannot bind private property '%s' to instance of %s", name->chars, instance->klass->name->chars);
                //return PTR_RUNTIME_ERR;
            }
            if(bl_vmdo_classbindmethod(vm, instance->klass, name))
            {
                return PTR_OK;
            }
            runtime_error("instance of class %s does not have a property or method named '%s'", AS_INSTANCE(peeked)->klass->name->chars, name->chars);
            //return PTR_RUNTIME_ERR;
        }
        else if(peekobj->type == OBJ_DICT)
        {
            if(bl_hashtable_get(&AS_DICT(peeked)->items, OBJ_VAL(name), &value) || bl_hashtable_get(&vm->classobjdict->methods, OBJ_VAL(name), &value))
            {
                bl_vmdo_popvalue(vm);
                bl_vmdo_pushvalue(vm, value);
                return PTR_OK;
            }
            runtime_error("unknown key or class Dict property '%s'", name->chars);
            //return PTR_RUNTIME_ERR;
        }
        switch(peekobj->type)
        {
            case OBJ_STRING:
                {
                    klass = vm->classobjstring;
                }
                break;
            case OBJ_ARRAY:
                {
                    klass = vm->classobjlist;
                }
                break;
            case OBJ_RANGE:
                {
                    klass = vm->classobjrange;
                }
                break;
            case OBJ_BYTES:
                {
                    klass = vm->classobjbytes;
                }
                break;
            case OBJ_FILE:
                {
                    klass = vm->classobjfile;
                }
                break;
            default:
                {
                    runtime_error("object of type %s does not carry properties", bl_value_typename(peeked));
                    //return PTR_RUNTIME_ERR;
                }
                break;
        }

        if(klass != NULL)
        {
            /*
            * TODO:
            * this is where actual property fields become important:
            *  `bl_class_getproperty(vm, klass, name, &value)`
            * would be ambiguous as $value could be a function, which could then be called
            * with incorrect argument count...
            */
            if(bl_class_getmethod(vm, klass, name, &value))
            {
                bl_vmdo_popvalue(vm);
                bl_vmdo_pushvalue(vm, value);
                return PTR_OK;
            }
            else if(bl_class_getproperty(vm, klass, name, &value))
            {
                bl_vmdo_popvalue(vm);
                if(bl_value_isobject(value) && bl_value_isnativefunction(value))
                {
                    //bool bl_vm_callvalue(VMState *vm, Value callee, int argcount);
                    bl_vmdo_pushvalue(vm, peeked);
                    bl_vm_callvalue(vm, value, 0);
                }
                else
                {
                    bl_vmdo_pushvalue(vm, value);
                }
                return PTR_OK;
            }
        }
        runtime_error("class %s has no named property '%s'", klass->name->chars, name->chars);
        //return PTR_RUNTIME_ERR;
    }
    else
    {
        runtime_error("'%s' of type %s does not have properties", bl_value_tostring(vm, peeked), bl_value_typename(peeked));
        //return PTR_RUNTIME_ERR;
    }
    return PTR_OK;
}

PtrResult bl_vmdo_rungetselfproperty(VMState* vm, CallFrame* frame)
{
    Value value;
    Value peeked;
    ObjString* name;
    ObjClass* klass;
    ObjModule* module;
    ObjInstance* instance;
    name = READ_STRING(frame);
    peeked = bl_vmdo_peekvalue(vm, 0);
    if(bl_value_isinstance(peeked))
    {
        instance = AS_INSTANCE(peeked);
        if(bl_hashtable_get(&instance->properties, OBJ_VAL(name), &value))
        {
            bl_vmdo_popvalue(vm);
            bl_vmdo_pushvalue(vm, value);
            return PTR_OK;
        }
        if(bl_vmdo_classbindmethod(vm, instance->klass, name))
        {
            return PTR_OK;
        }
        runtime_error("instance of class %s does not have a property or method named '%s'", AS_INSTANCE(peeked)->klass->name->chars,
                      name->chars);
    }
    else if(bl_value_isclass(peeked))
    {
        klass = AS_CLASS(peeked);
        if(bl_hashtable_get(&klass->methods, OBJ_VAL(name), &value))
        {
            if(bl_vmutil_getmethodtype(value) == TYPE_STATIC)
            {
                bl_vmdo_popvalue(vm);
                bl_vmdo_pushvalue(vm, value);
                return PTR_OK;
            }
        }
        else if(bl_hashtable_get(&klass->staticproperties, OBJ_VAL(name), &value))
        {
            bl_vmdo_popvalue(vm);
            bl_vmdo_pushvalue(vm, value);
            return PTR_OK;
        }
        runtime_error("class %s does not have a static property or method named '%s'", klass->name->chars, name->chars);
    }
    else if(bl_value_ismodule(peeked))
    {
        module = AS_MODULE(peeked);
        if(bl_hashtable_get(&module->values, OBJ_VAL(name), &value))
        {
            bl_vmdo_popvalue(vm);
            bl_vmdo_pushvalue(vm, value);
            return PTR_OK;
        }
        runtime_error("module %s does not define '%s'", module->name, name->chars);
    }
    runtime_error("'%s' of type %s does not have properties", bl_value_tostring(vm, peeked), bl_value_typename(peeked));
    return PTR_RUNTIME_ERR;
}

PtrResult bl_vmdo_runsetproperty(VMState* vm, CallFrame* frame)
{
    Value objto;
    Value key;
    Value val;
    objto = bl_vmdo_peekvalue(vm, 1);
    key = bl_vmdo_peekvalue(vm, 0);
    if(!bl_value_isinstance(objto) && !bl_value_isdict(objto))
    {
        runtime_error("object of type %s can not carry properties", bl_value_typename(objto));
    }
    else if(bl_value_isempty(key))
    {
        runtime_error("empty cannot be assigned");
    }
    ObjString* name = READ_STRING(frame);
    if(bl_value_isinstance(objto))
    {
        ObjInstance* instance = AS_INSTANCE(objto);
        val = bl_vmdo_peekvalue(vm, 0);
        bl_hashtable_set(vm, &instance->properties, OBJ_VAL(name), val);
        Value value = bl_vmdo_popvalue(vm);
        bl_vmdo_popvalue(vm);// removing the instance object
        bl_vmdo_pushvalue(vm, value);
    }
    else
    {
        ObjDict* dict = AS_DICT(objto);
        val = bl_vmdo_peekvalue(vm, 0);
        bl_dict_setentry(vm, dict, OBJ_VAL(name), val);
        Value value = bl_vmdo_popvalue(vm);
        bl_vmdo_popvalue(vm);// removing the dictionary object
        bl_vmdo_pushvalue(vm, value);
    }
    return PTR_OK;
}

#define vm_mac_execfunc(name) \
    { \
        doresult = name(vm, frame); \
        if(doresult != PTR_OK) \
        { \
            return doresult; \
        } \
    }


#define vm_mac_execfuncargs(name, ...) \
    { \
        doresult = name(vm, frame, __VA_ARGS__); \
        if(doresult != PTR_OK) \
        { \
            return doresult; \
        } \
    }

PtrResult bl_vm_run(VMState* vm)
{
    uint8_t instruction;
    PtrResult doresult;
    CallFrame* frame;
    Value* stackslot;
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
            for(stackslot = vm->stack; stackslot < vm->stacktop; stackslot++)
            {
                printf("[ ");
                bl_value_printvalue(*stackslot);
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
                    bl_vmdo_pushvalue(vm, constant);
                }
                break;
            case OP_ADD:
                {
                    if(bl_value_isstring(bl_vmdo_peekvalue(vm, 0)) || bl_value_isstring(bl_vmdo_peekvalue(vm, 1)))
                    {
                        if(!bl_vmdo_concatvalues(vm))
                        {
                            runtime_error("unsupported operand + for %s and %s", bl_value_typename(bl_vmdo_peekvalue(vm, 0)), bl_value_typename(bl_vmdo_peekvalue(vm, 1)));
                            break;
                        }
                    }
                    else if(bl_value_isarray(bl_vmdo_peekvalue(vm, 0)) && bl_value_isarray(bl_vmdo_peekvalue(vm, 1)))
                    {
                        Value result = OBJ_VAL(bl_array_addarray(vm, AS_LIST(bl_vmdo_peekvalue(vm, 1)), AS_LIST(bl_vmdo_peekvalue(vm, 0))));
                        bl_vmdo_popvaluen(vm, 2);
                        bl_vmdo_pushvalue(vm, result);
                    }
                    else if(bl_value_isbytes(bl_vmdo_peekvalue(vm, 0)) && bl_value_isbytes(bl_vmdo_peekvalue(vm, 1)))
                    {
                        Value result = OBJ_VAL(bl_bytes_addbytes(vm, AS_BYTES(bl_vmdo_peekvalue(vm, 1)), AS_BYTES(bl_vmdo_peekvalue(vm, 0))));
                        bl_vmdo_popvaluen(vm, 2);
                        bl_vmdo_pushvalue(vm, result);
                    }
                    else
                    {
                        vm_mac_execfuncargs(bl_vmdo_binaryop, false, OP_ADD, NULL);
                        break;
                    }
                }
                break;
            case OP_SUBTRACT:
                {
                    vm_mac_execfuncargs(bl_vmdo_binaryop, false, OP_SUBTRACT, NULL);
                }
                break;
            case OP_MULTIPLY:
                {
                    if(bl_value_isstring(bl_vmdo_peekvalue(vm, 1)) && bl_value_isnumber(bl_vmdo_peekvalue(vm, 0)))
                    {
                        double number = AS_NUMBER(bl_vmdo_peekvalue(vm, 0));
                        ObjString* string = AS_STRING(bl_vmdo_peekvalue(vm, 1));
                        Value result = OBJ_VAL(bl_vmdo_stringmultiply(vm, string, number));
                        bl_vmdo_popvaluen(vm, 2);
                        bl_vmdo_pushvalue(vm, result);
                        break;
                    }
                    else if(bl_value_isarray(bl_vmdo_peekvalue(vm, 1)) && bl_value_isnumber(bl_vmdo_peekvalue(vm, 0)))
                    {
                        int number = (int)AS_NUMBER(bl_vmdo_popvalue(vm));
                        ObjArray* list = AS_LIST(bl_vmdo_peekvalue(vm, 0));
                        ObjArray* nlist = bl_object_makelist(vm);
                        bl_vmdo_pushvalue(vm, OBJ_VAL(nlist));
                        bl_vmdo_listmultiply(vm, list, nlist, number);
                        bl_vmdo_popvaluen(vm, 2);
                        bl_vmdo_pushvalue(vm, OBJ_VAL(nlist));
                        break;
                    }
                    vm_mac_execfuncargs(bl_vmdo_binaryop, false, OP_MULTIPLY, NULL);
                }
                break;
            case OP_DIVIDE:
                {
                    vm_mac_execfuncargs(bl_vmdo_binaryop, false, OP_DIVIDE, NULL);
                }
                break;
            case OP_REMINDER:
                {
                    vm_mac_execfuncargs(bl_vmdo_binaryop, false, OP_REMINDER, (VMBinaryCallbackFn)bl_util_modulo);
                }
                break;
            case OP_POW:
                {
                    vm_mac_execfuncargs(bl_vmdo_binaryop, false, OP_POW, (VMBinaryCallbackFn)pow);
                }
                break;
            case OP_F_DIVIDE:
                {
                    vm_mac_execfuncargs(bl_vmdo_binaryop, false, OP_F_DIVIDE, (VMBinaryCallbackFn)bl_util_floordiv);
                }
                break;
            case OP_NEGATE:
                {
                    if(!bl_value_isnumber(bl_vmdo_peekvalue(vm, 0)))
                    {
                        runtime_error("operator - not defined for object of type %s", bl_value_typename(bl_vmdo_peekvalue(vm, 0)));
                        break;
                    }
                    bl_vmdo_pushvalue(vm, NUMBER_VAL(-AS_NUMBER(bl_vmdo_popvalue(vm))));
                }
                break;
            case OP_BIT_NOT:
                {
                    if(!bl_value_isnumber(bl_vmdo_peekvalue(vm, 0)))
                    {
                        runtime_error("operator ~ not defined for object of type %s", bl_value_typename(bl_vmdo_peekvalue(vm, 0)));
                        break;
                    }
                    bl_vmdo_pushvalue(vm, INTEGER_VAL(~((int)AS_NUMBER(bl_vmdo_popvalue(vm)))));
                }
                break;
            case OP_BITAND:
                {
                    vm_mac_execfuncargs(bl_vmdo_binaryop, false, OP_BITAND, NULL);
                }
                break;
            case OP_BITOR:
                {
                    vm_mac_execfuncargs(bl_vmdo_binaryop, false, OP_BITOR, NULL);
                }
                break;
            case OP_BITXOR:
                {
                    vm_mac_execfuncargs(bl_vmdo_binaryop, false, OP_BITXOR, NULL);
                }
                break;
            case OP_LEFTSHIFT:
                {
                    vm_mac_execfuncargs(bl_vmdo_binaryop, false, OP_LEFTSHIFT, NULL);
                }
                break;
            case OP_RIGHTSHIFT:
                {
                    vm_mac_execfuncargs(bl_vmdo_binaryop, false, OP_RIGHTSHIFT, NULL);
                }
                break;
            case OP_ONE:
                {
                    bl_vmdo_pushvalue(vm, NUMBER_VAL(1));
                }
                break;
                // comparisons
            case OP_EQUAL:
                {
                    Value b = bl_vmdo_popvalue(vm);
                    Value a = bl_vmdo_popvalue(vm);
                    bl_vmdo_pushvalue(vm, BOOL_VAL(bl_value_valuesequal(a, b)));
                }
                break;
            case OP_GREATERTHAN:
                {
                    vm_mac_execfuncargs(bl_vmdo_binaryop, true, OP_GREATERTHAN, NULL);
                }
                break;
            case OP_LESSTHAN:
                {
                    vm_mac_execfuncargs(bl_vmdo_binaryop, true, OP_LESSTHAN, NULL);
                }
                break;
            case OP_NOT:
                {
                    bl_vmdo_pushvalue(vm, BOOL_VAL(bl_value_isfalse(bl_vmdo_popvalue(vm))));
                }
                break;
            case OP_NIL:
                {
                    bl_vmdo_pushvalue(vm, NIL_VAL);
                }
                break;
            case OP_EMPTY:
                {
                    bl_vmdo_pushvalue(vm, EMPTY_VAL);
                }
                break;
            case OP_TRUE:
                {
                    bl_vmdo_pushvalue(vm, BOOL_VAL(true));
                }
                break;
            case OP_FALSE:
                {
                    bl_vmdo_pushvalue(vm, BOOL_VAL(false));
                }
                break;
            case OP_JUMP:
                {
                    uint16_t offset = READ_SHORT(frame);
                    frame->ip += offset;
                }
                break;
            case OP_JUMP_IF_FALSE:
                {
                    uint16_t offset = READ_SHORT(frame);
                    if(bl_value_isfalse(bl_vmdo_peekvalue(vm, 0)))
                    {
                        frame->ip += offset;
                    }
                }
                break;
            case OP_LOOP:
                {
                    uint16_t offset = READ_SHORT(frame);
                    frame->ip -= offset;
                }
                break;
            case OP_ECHO:
                {
                    Value val = bl_vmdo_peekvalue(vm, 0);
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
                    bl_vmdo_popvalue(vm);
                }
                break;
            case OP_STRINGIFY:
                {
                    if(!bl_value_isstring(bl_vmdo_peekvalue(vm, 0)) && !bl_value_isnil(bl_vmdo_peekvalue(vm, 0)))
                    {
                        char* value = bl_value_tostring(vm, bl_vmdo_popvalue(vm));
                        if((int)strlen(value) != 0)
                        {
                            bl_vmdo_pushvalue(vm, STRING_TT_VAL(value));
                        }
                        else
                        {
                            bl_vmdo_pushvalue(vm, NIL_VAL);
                        }
                    }
                }
                break;
            case OP_DUP:
                {
                    bl_vmdo_pushvalue(vm, bl_vmdo_peekvalue(vm, 0));
                }
                break;
            case OP_POP:
                {
                    bl_vmdo_popvalue(vm);
                }
                break;
            case OP_POP_N:
                {
                    bl_vmdo_popvaluen(vm, READ_SHORT(frame));
                }
                break;
            case OP_CLOSE_UP_VALUE:
                {
                    bl_vm_closeupvalues(vm, vm->stacktop - 1);
                    bl_vmdo_popvalue(vm);
                }
                break;
            case OP_DEFINE_GLOBAL:
                {
                    ObjString* name = READ_STRING(frame);
                    if(bl_value_isempty(bl_vmdo_peekvalue(vm, 0)))
                    {
                        runtime_error("empty cannot be assigned");
                        break;
                    }
                    bl_hashtable_set(vm, &frame->closure->fnptr->module->values, OBJ_VAL(name), bl_vmdo_peekvalue(vm, 0));
                    bl_vmdo_popvalue(vm);
                    #if defined(DEBUG_TABLE) && DEBUG_TABLE
                        bl_hashtable_print(&vm->globals);
                    #endif
                }
                break;
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
                    bl_vmdo_pushvalue(vm, value);
                }
                break;
            case OP_SET_GLOBAL:
                {
                    if(bl_value_isempty(bl_vmdo_peekvalue(vm, 0)))
                    {
                        runtime_error("empty cannot be assigned");
                        break;
                    }
                    ObjString* name = READ_STRING(frame);
                    HashTable* table = &frame->closure->fnptr->module->values;
                    if(bl_hashtable_set(vm, table, OBJ_VAL(name), bl_vmdo_peekvalue(vm, 0)))
                    {
                        bl_hashtable_delete(table, OBJ_VAL(name));
                        runtime_error("%s is undefined in this scope", name->chars);
                        break;
                    }
                }
                break;
            case OP_GET_LOCAL:
                {
                    uint16_t slot = READ_SHORT(frame);
                    bl_vmdo_pushvalue(vm, frame->slots[slot]);
                }
                break;
            case OP_SET_LOCAL:
                {
                    uint16_t slot = READ_SHORT(frame);
                    if(bl_value_isempty(bl_vmdo_peekvalue(vm, 0)))
                    {
                        runtime_error("empty cannot be assigned");
                        break;
                    }
                    frame->slots[slot] = bl_vmdo_peekvalue(vm, 0);
                }
                break;
            case OP_GET_PROPERTY:
                {
                    vm_mac_execfunc(bl_vmdo_rungetproperty);
                }
                break;
            case OP_GET_SELF_PROPERTY:
                {
                    vm_mac_execfunc(bl_vmdo_rungetselfproperty);
                }
                break;
            case OP_SET_PROPERTY:
                {
                    vm_mac_execfunc(bl_vmdo_runsetproperty);
                }
                break;
            case OP_CLOSURE:
                {
                    ObjFunction* function = AS_FUNCTION(READ_CONSTANT(frame));
                    ObjClosure* closure = bl_object_makeclosure(vm, function);
                    bl_vmdo_pushvalue(vm, OBJ_VAL(closure));
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
                }
                break;

            case OP_GET_UP_VALUE:
                {
                    int index = READ_SHORT(frame);
                    bl_vmdo_pushvalue(vm, *((ObjClosure*)frame->closure)->upvalues[index]->location);
                }
                break;
            case OP_SET_UP_VALUE:
                {
                    int index = READ_SHORT(frame);
                    if(bl_value_isempty(bl_vmdo_peekvalue(vm, 0)))
                    {
                        runtime_error("empty cannot be assigned");
                        break;
                    }
                    *((ObjClosure*)frame->closure)->upvalues[index]->location = bl_vmdo_peekvalue(vm, 0);
                }
                break;
            case OP_CALL:
                {
                    int argcount = READ_BYTE(frame);
                    if(!bl_vm_callvalue(vm, bl_vmdo_peekvalue(vm, argcount), argcount))
                    {
                        EXIT_VM();
                    }
                    frame = &vm->frames[vm->framecount - 1];
                }
                break;
            case OP_INVOKE:
                {
                    ObjString* method = READ_STRING(frame);
                    int argcount = READ_BYTE(frame);
                    if(!bl_vm_invokemethod(vm, method, argcount))
                    {
                        EXIT_VM();
                    }
                    frame = &vm->frames[vm->framecount - 1];
                }
                break;
            case OP_INVOKE_SELF:
                {
                    ObjString* method = READ_STRING(frame);
                    int argcount = READ_BYTE(frame);
                    if(!bl_instance_invokefromself(vm, method, argcount))
                    {
                        EXIT_VM();
                    }
                    frame = &vm->frames[vm->framecount - 1];
                }
                break;
            case OP_CLASS:
                {
                    ObjString* name = READ_STRING(frame);
                    bl_vmdo_pushvalue(vm, OBJ_VAL(bl_object_makeclass(vm, name, NULL)));
                }
                break;
            case OP_METHOD:
                {
                    ObjString* name = READ_STRING(frame);
                    bl_vm_classdefmethod(vm, name);
                }
                break;
            case OP_CLASS_PROPERTY:
                {
                    ObjString* name = READ_STRING(frame);
                    int isstatic = READ_BYTE(frame);
                    bl_vm_classdefproperty(vm, name, isstatic == 1);
                }
                break;
            case OP_INHERIT:
                {
                    if(!bl_value_isclass(bl_vmdo_peekvalue(vm, 1)))
                    {
                        runtime_error("cannot inherit from non-class object");
                        break;
                    }
                    ObjClass* superclass = AS_CLASS(bl_vmdo_peekvalue(vm, 1));
                    ObjClass* subclass = AS_CLASS(bl_vmdo_peekvalue(vm, 0));
                    bl_hashtable_addall(vm, &superclass->properties, &subclass->properties);
                    bl_hashtable_addall(vm, &superclass->methods, &subclass->methods);
                    subclass->superclass = superclass;
                    bl_vmdo_popvalue(vm);
                }
                break;
            case OP_GET_SUPER:
                {
                    ObjString* name = READ_STRING(frame);
                    ObjClass* klass = AS_CLASS(bl_vmdo_peekvalue(vm, 0));
                    if(!bl_vmdo_classbindmethod(vm, klass->superclass, name))
                    {
                        runtime_error("class %s does not define a function %s", klass->name->chars, name->chars);
                    }
                }
                break;
            case OP_SUPER_INVOKE:
                {
                    ObjString* method = READ_STRING(frame);
                    int argcount = READ_BYTE(frame);
                    ObjClass* klass = AS_CLASS(bl_vmdo_popvalue(vm));
                    if(!bl_vm_instanceinvokefromclass(vm, klass, method, argcount))
                    {
                        EXIT_VM();
                    }
                    frame = &vm->frames[vm->framecount - 1];
                }
                break;
            case OP_SUPER_INVOKE_SELF:
                {
                    int argcount = READ_BYTE(frame);
                    ObjClass* klass = AS_CLASS(bl_vmdo_popvalue(vm));
                    if(!bl_vm_instanceinvokefromclass(vm, klass, klass->name, argcount))
                    {
                        EXIT_VM();
                    }
                    frame = &vm->frames[vm->framecount - 1];
                }
                break;

            case OP_LIST:
                {
                    int count = READ_SHORT(frame);
                    ObjArray* list = bl_object_makelist(vm);
                    vm->stacktop[-count - 1] = OBJ_VAL(list);
                    for(int i = count - 1; i >= 0; i--)
                    {
                        bl_array_push(vm, list, bl_vmdo_peekvalue(vm, i));
                    }
                    bl_vmdo_popvaluen(vm, count);
                }
                break;
            case OP_RANGE:
                {
                    Value _upper = bl_vmdo_peekvalue(vm, 0);
                    Value _lower = bl_vmdo_peekvalue(vm, 1);
                    if(!bl_value_isnumber(_upper) || !bl_value_isnumber(_lower))
                    {
                        runtime_error("invalid range boundaries");
                        break;
                    }
                    double lower = AS_NUMBER(_lower);
                    double upper = AS_NUMBER(_upper);
                    bl_vmdo_popvaluen(vm, 2);
                    bl_vmdo_pushvalue(vm, OBJ_VAL(bl_object_makerange(vm, lower, upper)));
                }
                break;
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
                    bl_vmdo_popvaluen(vm, count);
                }
                break;
            case OP_GET_RANGED_INDEX:
                {
                    uint8_t willassign = READ_BYTE(frame);
                    bool isgotten = true;
                    if(bl_value_isobject(bl_vmdo_peekvalue(vm, 2)))
                    {
                        switch(AS_OBJ(bl_vmdo_peekvalue(vm, 2))->type)
                        {
                            case OBJ_STRING:
                            {
                                if(!bl_vmdo_stringgetrangedindex(vm, AS_STRING(bl_vmdo_peekvalue(vm, 2)), willassign == (uint8_t)1))
                                {
                                    EXIT_VM();
                                }
                                break;
                            }
                            case OBJ_ARRAY:
                            {
                                if(!bl_vmdo_listgetrangedindex(vm, AS_LIST(bl_vmdo_peekvalue(vm, 2)), willassign == (uint8_t)1))
                                {
                                    EXIT_VM();
                                }
                                break;
                            }
                            case OBJ_BYTES:
                            {
                                if(!bl_vmdo_bytesgetrangedindex(vm, AS_BYTES(bl_vmdo_peekvalue(vm, 2)), willassign == (uint8_t)1))
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
                        runtime_error("cannot range index object of type %s", bl_value_typename(bl_vmdo_peekvalue(vm, 2)));
                    }
                }
                break;

            case OP_GET_INDEX:
            {
                uint8_t willassign = READ_BYTE(frame);
                bool isgotten = true;
                if(bl_value_isobject(bl_vmdo_peekvalue(vm, 1)))
                {
                    switch(AS_OBJ(bl_vmdo_peekvalue(vm, 1))->type)
                    {
                        case OBJ_STRING:
                        {
                            if(!bl_vmdo_stringgetindex(vm, AS_STRING(bl_vmdo_peekvalue(vm, 1)), willassign == (uint8_t)1))
                            {
                                EXIT_VM();
                            }
                            break;
                        }
                        case OBJ_ARRAY:
                        {
                            if(!bl_vmdo_listgetindex(vm, AS_LIST(bl_vmdo_peekvalue(vm, 1)), willassign == (uint8_t)1))
                            {
                                EXIT_VM();
                            }
                            break;
                        }
                        case OBJ_DICT:
                        {
                            if(!bl_vmdo_dictgetindex(vm, AS_DICT(bl_vmdo_peekvalue(vm, 1)), willassign == (uint8_t)1))
                            {
                                EXIT_VM();
                            }
                            break;
                        }
                        case OBJ_MODULE:
                        {
                            if(!bl_vmdo_modulegetindex(vm, AS_MODULE(bl_vmdo_peekvalue(vm, 1)), willassign == (uint8_t)1))
                            {
                                EXIT_VM();
                            }
                            break;
                        }
                        case OBJ_BYTES:
                        {
                            if(!bl_vmdo_bytesgetindex(vm, AS_BYTES(bl_vmdo_peekvalue(vm, 1)), willassign == (uint8_t)1))
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
                    runtime_error("cannot index object of type %s", bl_value_typename(bl_vmdo_peekvalue(vm, 1)));
                }
                break;
            }
            case OP_SET_INDEX:
            {
                bool isset = true;
                if(bl_value_isobject(bl_vmdo_peekvalue(vm, 2)))
                {
                    Value value = bl_vmdo_peekvalue(vm, 0);
                    Value index = bl_vmdo_peekvalue(vm, 1);
                    if(bl_value_isempty(value))
                    {
                        runtime_error("empty cannot be assigned");
                        break;
                    }
                    switch(AS_OBJ(bl_vmdo_peekvalue(vm, 2))->type)
                    {
                        case OBJ_ARRAY:
                        {
                            if(!bl_vmdo_listsetindex(vm, AS_LIST(bl_vmdo_peekvalue(vm, 2)), index, value))
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
                            bl_vmdo_dictsetindex(vm, AS_DICT(bl_vmdo_peekvalue(vm, 2)), index, value);
                            break;
                        }
                        case OBJ_MODULE:
                        {
                            bl_vmdo_modulesetindex(vm, AS_MODULE(bl_vmdo_peekvalue(vm, 2)), index, value);
                            break;
                        }
                        case OBJ_BYTES:
                        {
                            if(!bl_vmdo_bytessetindex(vm, AS_BYTES(bl_vmdo_peekvalue(vm, 2)), index, value))
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
                    runtime_error("type of %s is not a valid iterable", bl_value_typename(bl_vmdo_peekvalue(vm, 3)));
                }
                break;
            }
            case OP_RETURN:
            {
                Value result = bl_vmdo_popvalue(vm);
                bl_vm_closeupvalues(vm, frame->slots);
                vm->framecount--;
                if(vm->framecount == 0)
                {
                    bl_vmdo_popvalue(vm);
                    return PTR_OK;
                }
                vm->stacktop = frame->slots;
                bl_vmdo_pushvalue(vm, result);
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
                ObjFunction* function = AS_CLOSURE(bl_vmdo_peekvalue(vm, 0))->fnptr;
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
                ObjString* modulename = AS_STRING(bl_vmdo_peekvalue(vm, 0));
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
                bl_hashtable_addall(vm, &AS_CLOSURE(bl_vmdo_peekvalue(vm, 0))->fnptr->module->values, &frame->closure->fnptr->module->values);
                break;
            }
            case OP_IMPORT_ALL_NATIVE:
            {
                ObjString* name = AS_STRING(bl_vmdo_peekvalue(vm, 0));
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
                Value message = bl_vmdo_popvalue(vm);
                Value expression = bl_vmdo_popvalue(vm);
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
                if(!bl_value_isinstance(bl_vmdo_peekvalue(vm, 0)) || !bl_class_isinstanceof(AS_INSTANCE(bl_vmdo_peekvalue(vm, 0))->klass, vm->exceptionclass->name->chars))
                {
                    runtime_error("instance of Exception expected");
                    break;
                }
                Value stacktrace = bl_vm_getstacktrace(vm);
                ObjInstance* instance = AS_INSTANCE(bl_vmdo_peekvalue(vm, 0));
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
                Value expr = bl_vmdo_peekvalue(vm, 0);
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
                bl_vmdo_popvalue(vm);
                break;
            }
            case OP_CHOICE:
            {
                Value _else = bl_vmdo_peekvalue(vm, 0);
                Value _then = bl_vmdo_peekvalue(vm, 1);
                Value _condition = bl_vmdo_peekvalue(vm, 2);
                bl_vmdo_popvaluen(vm, 3);
                if(!bl_value_isfalse(_condition))
                {
                    bl_vmdo_pushvalue(vm, _then);
                }
                else
                {
                    bl_vmdo_pushvalue(vm, _else);
                }
                break;
            }
            default:
                break;
        }
    }
}


