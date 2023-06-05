
#include "blade.h"


Object* bl_mem_gcprotect(VMState* vm, Object* object)
{
    bl_vm_pushvalue(vm, OBJ_VAL(object));
    vm->gcprotected++;
    return object;
}

void bl_mem_gcclearprotect(VMState* vm)
{
    if(vm->gcprotected > 0)
    {
        vm->stacktop -= vm->gcprotected;
    }
    vm->gcprotected = 0;
}


void bl_mem_free(VMState* vm, void* pointer, size_t sz)
{
    vm->bytesallocated -= sz;
    free(pointer);
}

void* bl_mem_realloc(VMState* vm, void* pointer, size_t oldsize, size_t newsize)
{
    void* result;
    vm->bytesallocated += newsize - oldsize;
    if(newsize > oldsize && vm->bytesallocated > vm->nextgc)
    {
        bl_mem_collectgarbage(vm);
    }
    result = (void*)realloc(pointer, newsize);
    // just in case reallocation fails... computers ain't infinite!
    if(result == NULL)
    {
        fflush(stdout);// flush out anything on stdout first
        fprintf(stderr, "Exit: device out of memory\n");
        exit(EXIT_TERMINAL);
    }
    return result;
}

void* bl_mem_growarray(VMState* vm, void* ptr, size_t tsz, size_t oldcount, size_t newcount)
{
    return bl_mem_realloc(vm, ptr, tsz * (oldcount), tsz * (newcount));
}

void bl_mem_markobject(VMState* vm, Object* object)
{
    if(object == NULL)
    {
        return;
    }
    if(object->mark)
    {
        return;
    }
    //#if defined(DEBUG_LOG_GC) && DEBUG_LOG_GC
    //  printf("%p mark ", (void *)object);
    //  bl_writer_printobject(OBJ_VAL(object), false);
    //  printf("\n");
    //#endif
    object->mark = true;
    if(vm->graycapacity < vm->graycount + 1)
    {
        vm->graycapacity = GROW_CAPACITY(vm->graycapacity);
        vm->graystack = (Object**)realloc(vm->graystack, sizeof(Object*) * vm->graycapacity);
        if(vm->graystack == NULL)
        {
            fflush(stdout);// flush out anything on stdout first
            fprintf(stderr, "GC encountered an error");
            exit(1);
        }
    }
    vm->graystack[vm->graycount++] = object;
}

void bl_mem_markvalue(VMState* vm, Value value)
{
    if(bl_value_isobject(value))
    {
        bl_mem_markobject(vm, AS_OBJ(value));
    }
}

void bl_mem_markarray(VMState* vm, ValArray* array)
{
    for(int i = 0; i < array->count; i++)
    {
        bl_mem_markvalue(vm, array->values[i]);
    }
}

void bl_mem_marktable(VMState* vm, HashTable* table)
{
    for(int i = 0; i < table->capacity; i++)
    {
        HashEntry* entry = &table->entries[i];
        if(entry != NULL)
        {
            bl_mem_markvalue(vm, entry->key);
            bl_mem_markvalue(vm, entry->value);
        }
    }
}

void bl_mem_blackenobject(VMState* vm, Object* object)
{
    //#if defined(DEBUG_LOG_GC) && DEBUG_LOG_GC
    //  printf("%p blacken ", (void *)object);
    //  bl_writer_printobject(OBJ_VAL(object), false);
    //  printf("\n");
    //#endif
    switch(object->type)
    {
        case OBJ_MODULE:
        {
            ObjModule* module = (ObjModule*)object;
            bl_mem_marktable(vm, &module->values);
            break;
        }
        case OBJ_SWITCH:
        {
            ObjSwitch* sw = (ObjSwitch*)object;
            bl_mem_marktable(vm, &sw->table);
            break;
        }
        case OBJ_FILE:
        {
            ObjFile* file = (ObjFile*)object;
            bl_mem_markobject(vm, (Object*)file->mode);
            bl_mem_markobject(vm, (Object*)file->path);
            break;
        }
        case OBJ_DICT:
        {
            ObjDict* dict = (ObjDict*)object;
            bl_mem_markarray(vm, &dict->names);
            bl_mem_marktable(vm, &dict->items);
            break;
        }
        case OBJ_ARRAY:
        {
            ObjArray* list = (ObjArray*)object;
            bl_mem_markarray(vm, &list->items);
            break;
        }
        case OBJ_BOUNDFUNCTION:
        {
            ObjBoundMethod* bound = (ObjBoundMethod*)object;
            bl_mem_markvalue(vm, bound->receiver);
            bl_mem_markobject(vm, (Object*)bound->method);
        }
        break;
        case OBJ_CLASS:
        {
            ObjClass* klass = (ObjClass*)object;
            bl_mem_markobject(vm, (Object*)klass->name);
            bl_mem_marktable(vm, &klass->methods);
            bl_mem_marktable(vm, &klass->properties);
            bl_mem_marktable(vm, &klass->staticproperties);
            bl_mem_markvalue(vm, klass->initializer);
            if(klass->superclass != NULL)
            {
                bl_mem_markobject(vm, (Object*)klass->superclass);
            }
        }
        break;
        case OBJ_CLOSURE:
        {
            ObjClosure* closure = (ObjClosure*)object;
            bl_mem_markobject(vm, (Object*)closure->fnptr);
            for(int i = 0; i < closure->upvaluecount; i++)
            {
                bl_mem_markobject(vm, (Object*)closure->upvalues[i]);
            }
        }
        break;
        case OBJ_SCRIPTFUNCTION:
        {
            ObjFunction* function = (ObjFunction*)object;
            bl_mem_markobject(vm, (Object*)function->name);
            bl_mem_markobject(vm, (Object*)function->module);
            bl_mem_markarray(vm, &function->blob.constants);
        }
        break;
        case OBJ_INSTANCE:
        {
            ObjInstance* instance = (ObjInstance*)object;
            bl_mem_markobject(vm, (Object*)instance->klass);
            bl_mem_marktable(vm, &instance->properties);
        }
        break;
        case OBJ_UP_VALUE:
        {
            bl_mem_markvalue(vm, ((ObjUpvalue*)object)->closed);
        }
        break;
        case OBJ_BYTES:
        case OBJ_RANGE:
        case OBJ_NATIVEFUNCTION:
        case OBJ_PTR:
        {
            bl_mem_markobject(vm, object);
        }
        break;
        case OBJ_STRING:
        {
        }
        break;
    }
}

void bl_mem_freeobject(VMState* vm, Object** pobject)
{
    //#if defined(DEBUG_LOG_GC) && DEBUG_LOG_GC
    //  printf("%p free type %d\n", (void *)object, object->type);
    //#endif
    Object* object;
    if(pobject == NULL)
    {
        return;
    }
    object = (*pobject);
    if(object == NULL)
    {
        return;
    }
    switch(object->type)
    {
        case OBJ_MODULE:
        {
            ObjModule* module = (ObjModule*)object;
            bl_hashtable_free(vm, &module->values);
            free(module->name);
            free(module->file);
            if(module->unloader != NULL && module->imported)
            {
                ((ModLoaderFunc)module->unloader)(vm);
            }
            if(module->handle != NULL)
            {
                close_dl_module(module->handle);// free the shared library...
            }
            FREE(ObjModule, object);
            break;
        }
        case OBJ_BYTES:
        {
            ObjBytes* bytes = (ObjBytes*)object;
            bl_bytearray_free(vm, &bytes->bytes);
            FREE(ObjBytes, object);
            break;
        }
        case OBJ_FILE:
        {
            ObjFile* file = (ObjFile*)object;
            if(file->mode->length != 0 && !bl_object_isstdfile(file) && file->file != NULL)
            {
                fclose(file->file);
            }
            FREE(ObjFile, object);
            break;
        }
        case OBJ_DICT:
        {
            ObjDict* dict = (ObjDict*)object;
            bl_valarray_free(vm, &dict->names);
            bl_hashtable_free(vm, &dict->items);
            FREE(ObjDict, object);
            break;
        }
        case OBJ_ARRAY:
        {
            ObjArray* list = (ObjArray*)object;
            bl_valarray_free(vm, &list->items);
            FREE(ObjArray, object);
            break;
        }
        case OBJ_BOUNDFUNCTION:
        {
            // a closure may be bound to multiple instances
            // for this reason, we do not free closures when freeing bound methods
            FREE(ObjBoundMethod, object);
            break;
        }
        case OBJ_CLASS:
        {
            ObjClass* klass = (ObjClass*)object;
            //bl_mem_freeobject(vm, (Object**)&klass->name);
            bl_hashtable_free(vm, &klass->methods);
            bl_hashtable_free(vm, &klass->properties);
            bl_hashtable_free(vm, &klass->staticproperties);
            if(!bl_value_isempty(klass->initializer))
            {
                // FIXME: uninitialized
                //bl_mem_freeobject(vm, &AS_OBJ(klass->initializer));
            }
            FREE(ObjClass, object);
            break;
        }
        case OBJ_CLOSURE:
        {
            ObjClosure* closure = (ObjClosure*)object;
            FREE_ARRAY(ObjUpvalue*, closure->upvalues, closure->upvaluecount);
            // there may be multiple closures that all reference the same function
            // for this reason, we do not free functions when freeing closures
            FREE(ObjClosure, object);
            break;
        }
        case OBJ_SCRIPTFUNCTION:
        {
            ObjFunction* function = (ObjFunction*)object;
            bl_blob_free(vm, &function->blob);
            if(function->name != NULL)
            {
                //bl_mem_freeobject(vm, (Object**)&function->name);
            }
            FREE(ObjFunction, object);
            break;
        }
        case OBJ_INSTANCE:
        {
            ObjInstance* instance = (ObjInstance*)object;
            bl_hashtable_free(vm, &instance->properties);
            FREE(ObjInstance, object);
            break;
        }
        case OBJ_NATIVEFUNCTION:
        {
            FREE(ObjNativeFunction, object);
            break;
        }
        case OBJ_UP_VALUE:
        {
            FREE(ObjUpvalue, object);
            break;
        }
        case OBJ_RANGE:
        {
            FREE(ObjRange, object);
            break;
        }
        case OBJ_STRING:
        {
            ObjString* string = (ObjString*)object;
            //if(string->length > 0)
            {
                FREE_ARRAY(char, string->chars, (size_t)string->length + 1);
            }
            FREE(ObjString, object);
            break;
        }
        case OBJ_SWITCH:
        {
            ObjSwitch* sw = (ObjSwitch*)object;
            bl_hashtable_free(vm, &sw->table);
            FREE(ObjSwitch, object);
            break;
        }
        case OBJ_PTR:
        {
            ObjPointer* ptr = (ObjPointer*)object;
            if(ptr->fnptrfree)
            {
                ptr->fnptrfree(ptr->pointer);
            }
            FREE(ObjPointer, object);
            break;
        }
        default:
            break;
    }
    *pobject = NULL;
}

static void bl_mem_markroots(VMState* vm)
{
    int i;
    int j;
    ExceptionFrame* handler;
    ObjUpvalue* upvalue;
    Value* slot;
    for(slot = vm->stack; slot < vm->stacktop; slot++)
    {
        bl_mem_markvalue(vm, *slot);
    }
    for(i = 0; i < vm->framecount; i++)
    {
        bl_mem_markobject(vm, (Object*)vm->frames[i].closure);
        for(j = 0; j < vm->frames[i].handlerscount; j++)
        {
            handler = &vm->frames[i].handlers[j];
            bl_mem_markobject(vm, (Object*)handler->klass);
        }
    }
    for(upvalue = vm->openupvalues; upvalue != NULL; upvalue = upvalue->next)
    {
        bl_mem_markobject(vm, (Object*)upvalue);
    }
    bl_mem_marktable(vm, &vm->globals);
    bl_mem_marktable(vm, &vm->modules);
    /*
    bl_mem_marktable(vm, &vm->classobjstring);
    bl_mem_marktable(vm, &vm->classobjbytes);
    bl_mem_marktable(vm, &vm->classobjfile);
    bl_mem_marktable(vm, &vm->classobjlist);
    bl_mem_marktable(vm, &vm->classobjdict);
    bl_mem_marktable(vm, &vm->classobjrange);
    */
    bl_mem_markobject(vm, (Object*)vm->exceptionclass);
    bl_parser_markcompilerroots(vm);
}

static void bl_mem_tracerefs(VMState* vm)
{
    Object* object;
    while(vm->graycount > 0)
    {
        object = vm->graystack[--vm->graycount];
        bl_mem_blackenobject(vm, object);
    }
}

static void bl_mem_gcsweep(VMState* vm)
{
    Object* previous;
    Object* object;
    Object* unreached;
    previous = NULL;
    object = vm->objectlinks;
    while(object != NULL)
    {
        if(object->mark)
        {
            object->mark = false;
            previous = object;
            object = object->sibling;
        }
        else
        {
            unreached = object;
            object = object->sibling;
            if(previous != NULL)
            {
                previous->sibling = object;
            }
            else
            {
                vm->objectlinks = object;
            }
            bl_mem_freeobject(vm, &unreached);
        }
    }
}

void bl_mem_freegcobjects(VMState* vm)
{
    size_t i;
    Object* next;
    Object* object;
    i = 0;
    object = vm->objectlinks;
    while(object != NULL)
    {
        i++;
        //fprintf(stderr, "bl_mem_freegcobjects: index %d of %d\n", i, vm->objectcount);
        next = object->sibling;
        bl_mem_freeobject(vm, &object);
        object = next;
    }
    free(vm->graystack);
    vm->graystack = NULL;
}

void bl_mem_collectgarbage(VMState* vm)
{
    if(!vm->allowgc)
    {
        //return;
    }
#if defined(DEBUG_LOG_GC) && DEBUG_LOG_GC
    printf("-- gc begins\n");
    size_t before = vm->bytesallocated;
#endif
    vm->allowgc = false;
    bl_mem_markroots(vm);
    bl_mem_tracerefs(vm);
    bl_hashtable_removewhites(vm, &vm->strings);
    bl_hashtable_removewhites(vm, &vm->modules);
    bl_mem_gcsweep(vm);
    vm->nextgc = vm->bytesallocated * GC_HEAP_GROWTH_FACTOR;
    vm->allowgc = true;
#if defined(DEBUG_LOG_GC) && DEBUG_LOG_GC
    printf("-- gc ends\n");
    printf("   collected %zu bytes (from %zu to %zu), next at %zu\n", before - vm->bytesallocated, before, vm->bytesallocated, vm->nextgc);
#endif
}



