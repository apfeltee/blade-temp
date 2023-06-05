
#include "blade.h"

#if defined(__TINYC__)
int __dso_handle;
#endif

bool bl_object_isstdfile(ObjFile* file)
{
    return (file->mode->length == 0);
}

void bl_state_addmodule(VMState* vm, ObjModule* module)
{
    size_t len;
    const char* cs;
    ObjString* copied;
    cs = module->file;
    len = strlen(cs);
    copied = bl_string_copystringlen(vm, cs, (int)len-4);
    bl_hashtable_set(vm, &vm->modules, OBJ_VAL(copied), OBJ_VAL(module));
    if(vm->framecount == 0)
    {
        bl_hashtable_set(vm, &vm->globals, STRING_VAL(module->name), OBJ_VAL(module));
    }
    else
    {
        cs = module->name;
        len = strlen(cs);
        copied = bl_string_copystringlen(vm, cs, (int)len);
        bl_hashtable_set(vm, &vm->frames[vm->framecount - 1].closure->fnptr->module->values, OBJ_VAL(copied), OBJ_VAL(module));
    }
}


static char* number_to_string(VMState* vm, double number)
{
    int length;
    char* numstr;
    length = snprintf(NULL, 0, NUMBER_FORMAT, number);
    numstr = ALLOCATE(char, length + 1);
    if(numstr != NULL)
    {
        sprintf(numstr, NUMBER_FORMAT, number);
    }
    else
    {
        //return "";
    }
    return numstr;

}

// valuetop

static bool bl_value_isobjtype(Value v, ObjType t)
{
    if(bl_value_isobject(v))
    {
        return (AS_OBJ(v)->type == t);
    }
    return false;
}

// object type checks
bool bl_value_isstring(Value v)
{
    return bl_value_isobjtype(v, OBJ_STRING);
}

bool bl_value_isnativefunction(Value v)
{
    return bl_value_isobjtype(v, OBJ_NATIVEFUNCTION);
}

bool bl_value_isscriptfunction(Value v)
{
    return bl_value_isobjtype(v, OBJ_SCRIPTFUNCTION);
}

bool bl_value_isclosure(Value v)
{
    return bl_value_isobjtype(v, OBJ_CLOSURE);
}

bool bl_value_isclass(Value v)
{
    return bl_value_isobjtype(v, OBJ_CLASS);
}

bool bl_value_isinstance(Value v)
{
    return bl_value_isobjtype(v, OBJ_INSTANCE);
}

bool bl_value_isboundfunction(Value v)
{
    return bl_value_isobjtype(v, OBJ_BOUNDFUNCTION);
}

bool bl_value_isnil(Value v)
{
    return ((v).type == VAL_NIL);
}

bool bl_value_isbool(Value v)
{
    return ((v).type == VAL_BOOL);
}

bool bl_value_isnumber(Value v)
{
    return ((v).type == VAL_NUMBER);
}

bool bl_value_isobject(Value v)
{
    return ((v).type == VAL_OBJ);
}

bool bl_value_isempty(Value v)
{
    return ((v).type == VAL_EMPTY);
}

bool bl_value_ismodule(Value v)
{
    return bl_value_isobjtype(v, OBJ_MODULE);
}

bool bl_value_ispointer(Value v)
{
    return bl_value_isobjtype(v, OBJ_PTR);
}

bool bl_value_isbytes(Value v)
{
    return bl_value_isobjtype(v, OBJ_BYTES);
}

bool bl_value_isarray(Value v)
{
    return bl_value_isobjtype(v, OBJ_ARRAY);
}

bool bl_value_isdict(Value v)
{
    return bl_value_isobjtype(v, OBJ_DICT);
}

bool bl_value_isfile(Value v)
{
    return bl_value_isobjtype(v, OBJ_FILE);
}

bool bl_value_isrange(Value v)
{
    return bl_value_isobjtype(v, OBJ_RANGE);
}

static void bl_value_doprintvalue(Value value, bool fixstring)
{
    switch(value.type)
    {
        case VAL_EMPTY:
            break;
        case VAL_NIL:
            printf("nil");
            break;
        case VAL_BOOL:
            printf(AS_BOOL(value) ? "true" : "false");
            break;
        case VAL_NUMBER:
            printf(NUMBER_FORMAT, AS_NUMBER(value));
            break;
        case VAL_OBJ:
            bl_writer_printobject(value, fixstring);
            break;
        default:
            break;
    }
}

void bl_value_printvalue(Value value)
{
    bl_value_doprintvalue(value, false);
}

void bl_value_echovalue(Value value)
{
    bl_value_doprintvalue(value, true);
}

// fixme: should this allocate?
char* bl_value_tostring(VMState* vm, Value value)
{
    switch(value.type)
    {
        case VAL_NIL:
            {
                return (char*)"nil";
            }
            break;
        case VAL_BOOL:
            {
                if(AS_BOOL(value))
                {
                    return (char*)"true";
                }
                else
                {
                    return (char*)"false";
                }
            }
            break;
        case VAL_NUMBER:
            {
                return number_to_string(vm, AS_NUMBER(value));
            }
            break;
        case VAL_OBJ:
            {
                return bl_writer_objecttostring(vm, value);
            }
            break;
        default:
            break;
    }
    return (char*)"";
}

const char* bl_value_typename(Value value)
{
    if(bl_value_isempty(value))
    {
        return "empty";
    }
    if(bl_value_isnil(value))
    {
        return "nil";
    }
    else if(bl_value_isbool(value))
    {
        return "boolean";
    }
    else if(bl_value_isnumber(value))
    {
        return "number";
    }
    else if(bl_value_isobject(value))
    {
        return bl_object_gettype(AS_OBJ(value));
    }
    return "unknown";
}

bool bl_value_valuesequal(Value a, Value b)
{
    if(a.type != b.type)
    {
        return false;
    }
    switch(a.type)
    {
        case VAL_NIL:
        case VAL_EMPTY:
            return true;
        case VAL_BOOL:
            return AS_BOOL(a) == AS_BOOL(b);
        case VAL_NUMBER:
            return AS_NUMBER(a) == AS_NUMBER(b);
        case VAL_OBJ:
            return AS_OBJ(a) == AS_OBJ(b);
        default:
            return false;
    }
}

// Generates a hash code for [object].
static uint32_t bl_object_hashobject(Object* object)
{
    switch(object->type)
    {
        case OBJ_CLASS:
            // Classes just use their name.
            return ((ObjClass*)object)->name->hash;
            // Allow bare (non-closure) functions so that we can use a map to find
            // existing constants in a function's constant table. This is only used
            // internally. Since user code never sees a non-closure function, they
            // cannot use them as map keys.
        case OBJ_SCRIPTFUNCTION:
        {
            ObjFunction* fn = (ObjFunction*)object;
            return bl_util_hashdouble(fn->arity) ^ bl_util_hashdouble(fn->blob.count);
        }
        case OBJ_STRING:
            return ((ObjString*)object)->hash;
        case OBJ_BYTES:
        {
            ObjBytes* bytes = ((ObjBytes*)object);
            return bl_util_hashstring((const char*)bytes->bytes.bytes, bytes->bytes.count);
        }
        default:
            return 0;
    }
}

uint32_t bl_value_hashvalue(Value value)
{
    switch(value.type)
    {
        case VAL_BOOL:
            return AS_BOOL(value) ? 3 : 5;
        case VAL_NIL:
            return 7;
        case VAL_NUMBER:
            return bl_util_hashdouble(AS_NUMBER(value));
        case VAL_OBJ:
            return bl_object_hashobject(AS_OBJ(value));
        default:// VAL_EMPTY
            return 0;
    }
}

/**
 * returns the greater of the two values.
 * this function encapsulates Blade's object hierarchy
 */
static Value bl_value_findmaxvalue(Value a, Value b)
{
    if(bl_value_isnil(a))
    {
        return b;
    }
    else if(bl_value_isbool(a))
    {
        if(bl_value_isnil(b) || (bl_value_isbool(b) && AS_BOOL(b) == false))
        {
            return a;// only nil, false and false are lower than numbers
        }
        else
        {
            return b;
        }
    }
    else if(bl_value_isnumber(a))
    {
        if(bl_value_isnil(b) || bl_value_isbool(b))
        {
            return a;
        }
        else if(bl_value_isnumber(b))
        {
            return AS_NUMBER(a) >= AS_NUMBER(b) ? a : b;
        }
        else
        {
            return b;// every other thing is greater than a number
        }
    }
    else if(bl_value_isobject(a))
    {
        if(bl_value_isstring(a) && bl_value_isstring(b))
        {
            return strcmp(AS_C_STRING(a), AS_C_STRING(b)) >= 0 ? a : b;
        }
        else if(bl_value_isscriptfunction(a) && bl_value_isscriptfunction(b))
        {
            return AS_FUNCTION(a)->arity >= AS_FUNCTION(b)->arity ? a : b;
        }
        else if(bl_value_isclosure(a) && bl_value_isclosure(b))
        {
            return AS_CLOSURE(a)->fnptr->arity >= AS_CLOSURE(b)->fnptr->arity ? a : b;
        }
        else if(bl_value_isrange(a) && bl_value_isrange(b))
        {
            return AS_RANGE(a)->lower >= AS_RANGE(b)->lower ? a : b;
        }
        else if(bl_value_isclass(a) && bl_value_isclass(b))
        {
            return AS_CLASS(a)->methods.count >= AS_CLASS(b)->methods.count ? a : b;
        }
        else if(bl_value_isarray(a) && bl_value_isarray(b))
        {
            return AS_LIST(a)->items.count >= AS_LIST(b)->items.count ? a : b;
        }
        else if(bl_value_isdict(a) && bl_value_isdict(b))
        {
            return AS_DICT(a)->names.count >= AS_DICT(b)->names.count ? a : b;
        }
        else if(bl_value_isbytes(a) && bl_value_isbytes(b))
        {
            return AS_BYTES(a)->bytes.count >= AS_BYTES(b)->bytes.count ? a : b;
        }
        else if(bl_value_isfile(a) && bl_value_isfile(b))
        {
            return strcmp(AS_FILE(a)->path->chars, AS_FILE(b)->path->chars) >= 0 ? a : b;
        }
        else if(bl_value_isobject(b))
        {
            return AS_OBJ(a)->type >= AS_OBJ(b)->type ? a : b;
        }
        else
        {
            return a;
        }
    }
    return a;
}

/**
 * sorts values in an array using the bubble-sort algorithm
 */
void bl_value_sortbubblesort(Value* values, int count)
{
    int i;
    int j;
    Value* temp;
    for(i = 0; i < count; i++)
    {
        for(j = 0; j < count; j++)
        {
            if(bl_value_valuesequal(values[j], bl_value_findmaxvalue(values[i], values[j])))
            {
                temp = &values[i];
                values[i] = values[j];
                values[j] = *temp;
                if(bl_value_isarray(values[i]))
                {
                    bl_value_sortbubblesort(AS_LIST(values[i])->items.values, AS_LIST(values[i])->items.count);
                }
                if(bl_value_isarray(values[j]))
                {
                    bl_value_sortbubblesort(AS_LIST(values[j])->items.values, AS_LIST(values[j])->items.count);
                }
            }
        }
    }
}

bool bl_value_isfalse(Value value)
{
    if(bl_value_isbool(value))
    {
        return bl_value_isbool(value) && !AS_BOOL(value);
    }
    if(bl_value_isnil(value) || bl_value_isempty(value))
    {
        return true;
    }
    // -1 is the number equivalent of false in Blade
    if(bl_value_isnumber(value))
    {
        return AS_NUMBER(value) < 0;
    }
    // Non-empty strings are true, empty strings are false.
    if(bl_value_isstring(value))
    {
        return AS_STRING(value)->length < 1;
    }
    // Non-empty bytes are true, empty strings are false.
    if(bl_value_isbytes(value))
    {
        return AS_BYTES(value)->bytes.count < 1;
    }
    // Non-empty lists are true, empty lists are false.
    if(bl_value_isarray(value))
    {
        return AS_LIST(value)->items.count == 0;
    }
    // Non-empty dicts are true, empty dicts are false.
    if(bl_value_isdict(value))
    {
        return AS_DICT(value)->names.count == 0;
    }
    // All classes are true
    // All closures are true
    // All bound methods are true
    // All functions are in themselves true if you do not account for what they
    // return.
    return false;
}

Value bl_value_copyvalue(VMState* vm, Value value)
{
    if(bl_value_isobject(value))
    {
        switch(AS_OBJ(value)->type)
        {
            case OBJ_STRING:
            {
                ObjString* string = AS_STRING(value);
                return OBJ_VAL(bl_string_copystringlen(vm, string->chars, string->length));
            }
            case OBJ_BYTES:
            {
                ObjBytes* bytes = AS_BYTES(value);
                return OBJ_VAL(bl_bytes_copybytes(vm, bytes->bytes.bytes, bytes->bytes.count));
            }
            case OBJ_ARRAY:
            {
                ObjArray* list = AS_LIST(value);
                ObjArray* nlist = bl_object_makelist(vm);
                bl_vm_pushvalue(vm, OBJ_VAL(nlist));
                for(int i = 0; i < list->items.count; i++)
                {
                    bl_valarray_push(vm, &nlist->items, list->items.values[i]);
                }
                bl_vm_popvalue(vm);
                return OBJ_VAL(nlist);
            }
            /*
            case OBJ_DICT:
                {
                    ObjDict *dict = AS_DICT(value);
                    ObjDict *ndict = bl_object_makedict(vm);
                    // @TODO: Figure out how to handle dictionary values correctly
                    // remember that copying keys is redundant and unnecessary
                }
                break;
            */
            default:
                return value;
        }
    }
    return value;
}

bool bl_value_returnvalue(VMState* vm, Value* args, Value val, bool b)
{
    (void)vm;
    args[-1] = val;
    return b;
}

bool bl_value_returnempty(VMState* vm, Value* args)
{
    return bl_value_returnvalue(vm, args, EMPTY_VAL, true);
}

bool bl_value_returnnil(VMState* vm, Value* args)
{
    return bl_value_returnvalue(vm, args, NIL_VAL, true);
}


void bl_hashtable_reset(HashTable* table)
{
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void bl_hashtable_init(HashTable* table)
{
    bl_hashtable_reset(table);
}

void bl_hashtable_free(VMState* vm, HashTable* table)
{
    FREE_ARRAY(HashEntry, table->entries, table->capacity);
    bl_hashtable_reset(table);
}

void bl_hashtable_cleanfree(VMState* vm, HashTable* table)
{
    int i;
    HashEntry* entry;
    for(i = 0; i < table->capacity; i++)
    {
        entry = &table->entries[i];
        if(entry != NULL)
        {
#if 0
            if(bl_value_isobject(entry->key))
            {
                bl_mem_freeobject(vm, &AS_OBJ(entry->key));
            }
#endif
#if 0
            if(bl_value_isobject(entry->value))
            {
                bl_mem_freeobject(vm, &AS_OBJ(entry->value));
            }
#endif
        }
    }
    FREE_ARRAY(HashEntry, table->entries, table->capacity);
    bl_hashtable_reset(table);
}

static HashEntry* bl_hashtable_findentry(HashEntry* entries, int capacity, Value key)
{
    uint32_t hash;
    uint32_t index;
    HashEntry* entry;
    HashEntry* tombstone;
    hash = bl_value_hashvalue(key);
#if defined(DEBUG_TABLE) && DEBUG_TABLE
    printf("looking for key ");
    bl_value_printvalue(key);
    printf(" with hash %u in table...\n", hash);
#endif
    index = hash & (capacity - 1);
    tombstone = NULL;
    for(;;)
    {
        entry = &entries[index];
        if(bl_value_isempty(entry->key))
        {
            if(bl_value_isnil(entry->value))
            {
                // empty entry
                return tombstone != NULL ? tombstone : entry;
            }
            else
            {
                // we found a tombstone.
                if(tombstone == NULL)
                {
                    tombstone = entry;
                }
            }
        }
        else if(bl_value_valuesequal(key, entry->key))
        {
#if defined(DEBUG_TABLE) && DEBUG_TABLE
            printf("found entry for key ");
            bl_value_printvalue(key);
            printf(" with hash %u in table as ", hash);
            bl_value_printvalue(entry->value);
            printf("...\n");
#endif
            return entry;
        }
        index = (index + 1) & (capacity - 1);
    }
    return NULL;
}

bool bl_hashtable_get(HashTable* table, Value key, Value* value)
{
    HashEntry* entry;
    if(table->count == 0 || table->entries == NULL)
    {
        return false;
    }
#if defined(DEBUG_TABLE) && DEBUG_TABLE
    printf("getting entry with hash %u...\n", bl_value_hashvalue(key));
#endif
    entry = bl_hashtable_findentry(table->entries, table->capacity, key);
    if(bl_value_isempty(entry->key) || bl_value_isnil(entry->key))
    {
        return false;
    }
#if defined(DEBUG_TABLE) && DEBUG_TABLE
    printf("found entry for hash %u == ", bl_value_hashvalue(entry->key));
    bl_value_printvalue(entry->value);
    printf("\n");
#endif
    *value = entry->value;
    return true;
}

static void bl_hashtable_adjustcap(VMState* vm, HashTable* table, int capacity)
{
    int i;
    HashEntry* dest;
    HashEntry* entry;
    HashEntry* entries;
    entries = ALLOCATE(HashEntry, capacity);
    for(i = 0; i < capacity; i++)
    {
        entries[i].key = EMPTY_VAL;
        entries[i].value = NIL_VAL;
    }
    // repopulate buckets
    table->count = 0;
    for(i = 0; i < table->capacity; i++)
    {
        entry = &table->entries[i];
        if(bl_value_isempty(entry->key))
        {
            continue;
        }
        dest = bl_hashtable_findentry(entries, capacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        table->count++;
    }
    // free the old entries...
    FREE_ARRAY(HashEntry, table->entries, table->capacity);
    table->entries = entries;
    table->capacity = capacity;
}

bool bl_hashtable_set(VMState* vm, HashTable* table, Value key, Value value)
{
    bool isnew;
    int capacity;
    HashEntry* entry;
    if(table->count + 1 > table->capacity * TABLE_MAX_LOAD)
    {
        capacity = GROW_CAPACITY(table->capacity);
        bl_hashtable_adjustcap(vm, table, capacity);
    }
    entry = bl_hashtable_findentry(table->entries, table->capacity, key);
    isnew = bl_value_isempty(entry->key);
    if(isnew && bl_value_isnil(entry->value))
    {
        table->count++;
    }
    // overwrites existing entries.
    entry->key = key;
    entry->value = value;
    return isnew;
}

bool bl_hashtable_delete(HashTable* table, Value key)
{
    HashEntry* entry;
    if(table->count == 0)
    {
        return false;
    }
    // find the entry
    entry = bl_hashtable_findentry(table->entries, table->capacity, key);
    if(bl_value_isempty(entry->key))
    {
        return false;
    }
    // place a tombstone in the entry.
    entry->key = EMPTY_VAL;
    entry->value = BOOL_VAL(true);
    return true;
}

void bl_hashtable_addall(VMState* vm, HashTable* from, HashTable* to)
{
    int i;
    HashEntry* entry;
    for(i = 0; i < from->capacity; i++)
    {
        entry = &from->entries[i];
        if(!bl_value_isempty(entry->key))
        {
            bl_hashtable_set(vm, to, entry->key, entry->value);
        }
    }
}

void bl_hashtable_copy(VMState* vm, HashTable* from, HashTable* to)
{
    int i;
    HashEntry* entry;
    for(i = 0; i < from->capacity; i++)
    {
        entry = &from->entries[i];
        if(!bl_value_isempty(entry->key))
        {
            bl_hashtable_set(vm, to, entry->key, bl_value_copyvalue(vm, entry->value));
        }
    }
}

ObjString* bl_hashtable_findstring(HashTable* table, const char* chars, int length, uint32_t hash)
{
    uint32_t index;
    HashEntry* entry;
    ObjString* string;
    if(table->count == 0)
    {
        return NULL;
    }
    index = hash & (table->capacity - 1);
    for(;;)
    {
        entry = &table->entries[index];
        if(bl_value_isempty(entry->key))
        {
            // stop if we find an empty non-tombstone entry
            //if (bl_value_isnil(entry->value))
            return NULL;
        }
        // if (bl_value_isstring(entry->key)) {
        string = AS_STRING(entry->key);
        if(string->length == length && string->hash == hash && memcmp(string->chars, chars, length) == 0)
        {
            // we found it
            return string;
        }
        // }
        index = (index + 1) & (table->capacity - 1);
    }
    return NULL;
}

Value bl_hashtable_findkey(HashTable* table, Value value)
{
    int i;
    HashEntry* entry;
    for(i = 0; i < table->capacity; i++)
    {
        entry = &table->entries[i];
        if(!bl_value_isnil(entry->key) && !bl_value_isempty(entry->key))
        {
            if(bl_value_valuesequal(entry->value, value))
            {
                return entry->key;
            }
        }
    }
    return NIL_VAL;
}

void bl_hashtable_print(HashTable* table)
{
    printf("<HashTable: {");
    for(int i = 0; i < table->capacity; i++)
    {
        HashEntry* entry = &table->entries[i];
        if(!bl_value_isempty(entry->key))
        {
            bl_value_printvalue(entry->key);
            printf(": ");
            bl_value_printvalue(entry->value);
            if(i != table->capacity - 1)
            {
                printf(",");
            }
        }
    }
    printf("}>\n");
}

void bl_hashtable_removewhites(VMState* vm, HashTable* table)
{
    int i;
    HashEntry* entry;
    (void)vm;
    for(i = 0; i < table->capacity; i++)
    {
        entry = &table->entries[i];
        if(bl_value_isobject(entry->key) && !AS_OBJ(entry->key)->mark)
        {
            bl_hashtable_delete(table, entry->key);
        }
    }
}

void bl_blob_init(BinaryBlob* blob)
{
    blob->count = 0;
    blob->capacity = 0;
    blob->code = NULL;
    blob->lines = NULL;
    bl_valarray_init(&blob->constants);
}

void bl_blob_write(VMState* vm, BinaryBlob* blob, uint8_t byte, int line)
{
    if(blob->capacity < blob->count + 1)
    {
        int oldcapacity = blob->capacity;
        blob->capacity = GROW_CAPACITY(oldcapacity);
        blob->code = GROW_ARRAY(uint8_t, sizeof(uint8_t), blob->code, oldcapacity, blob->capacity);
        blob->lines = GROW_ARRAY(int, sizeof(int), blob->lines, oldcapacity, blob->capacity);
    }
    blob->code[blob->count] = byte;
    blob->lines[blob->count] = line;
    blob->count++;
}

void bl_blob_free(VMState* vm, BinaryBlob* blob)
{
    if(blob->code != NULL)
    {
        FREE_ARRAY(uint8_t, blob->code, blob->capacity);
    }
    if(blob->lines != NULL)
    {
        FREE_ARRAY(int, blob->lines, blob->capacity);
    }
    bl_valarray_free(vm, &blob->constants);
    bl_blob_init(blob);
}

int bl_blob_addconst(VMState* vm, BinaryBlob* blob, Value value)
{
    bl_vm_pushvalue(vm, value);// fixing gc corruption
    bl_valarray_push(vm, &blob->constants, value);
    bl_vm_popvalue(vm);// fixing gc corruption
    return blob->constants.count - 1;
}

/**
 * a Blade regex must always start and end with the same delimiter e.g. /
 *
 * e.g.
 * /\d+/
 *
 * it can be followed by one or more matching fine tuning constants
 *
 * e.g.
 *
 * /\d+.+[a-z]+/sim -> '.' matches all, it's case insensitive and multiline
 * (see the function for list of available options)
 *
 * returns:
 * -1 -> false
 * 0 -> true
 * negative value -> invalid delimiter where abs(value) is the character
 * positive value > 0 ? for compiled delimiters
 */
uint32_t bl_helper_objstringisregex(ObjString* string)
{
    int i;
    uint32_t coptions;
    bool matchfound;
    char start;
    start = string->chars[0];
    matchfound = false;
    // pcre2 options
    coptions = 0;
    for(i = 1; i < string->length; i++)
    {
        if(string->chars[i] == start)
        {
            matchfound = i > 0 && string->chars[i - 1] == '\\' ? false : true;
            continue;
        }
        if(matchfound)
        {
            // compile the delimiters
            switch(string->chars[i])
            {
                #if 0
                /* Perl compatible options */
                case 'i':
                    coptions |= PCRE2_CASELESS;
                    break;
                case 'm':
                    coptions |= PCRE2_MULTILINE;
                    break;
                case 's':
                    coptions |= PCRE2_DOTALL;
                    break;
                case 'x':
                    coptions |= PCRE2_EXTENDED;
                    break;
                    /* PCRE specific options */
                case 'A':
                    coptions |= PCRE2_ANCHORED;
                    break;
                case 'D':
                    coptions |= PCRE2_DOLLAR_ENDONLY;
                    break;
                case 'U':
                    coptions |= PCRE2_UNGREEDY;
                    break;
                case 'u':
                    coptions |= PCRE2_UTF;
                    /*
                    * In  PCRE,  by  default, \d, \D, \s, \S, \w, and \W recognize only
                    * ASCII characters, even in UTF-8 mode. However, this can be changed by
                    * setting the PCRE2_UCP option.
                    */
                    #ifdef PCRE2_UCP
                        coptions |= PCRE2_UCP;
                    #endif
                    break;
                case 'J':
                    coptions |= PCRE2_DUPNAMES;
                    break;
                #endif
                case ' ':
                case '\n':
                case '\r':
                    break;
                default:
                    return coptions = (uint32_t)string->chars[i] + 1000000;
            }
        }
    }
    if(!matchfound)
    {
        return -1;
    }
    else
    {
        return coptions;
    }
}

char* bl_helper_objstringremregexdelim(VMState* vm, ObjString* string)
{
    if(string->length == 0)
    {
        return string->chars;
    }
    char start = string->chars[0];
    int i = string->length - 1;
    for(; i > 0; i--)
    {
        if(string->chars[i] == start)
        {
            break;
        }
    }
    char* str = ALLOCATE(char, i);
    memcpy(str, string->chars + 1, (size_t)i - 1);
    str[i - 1] = '\0';
    return str;
}

void bl_valarray_init(ValArray* array)
{
    array->capacity = 0;
    array->count = 0;
    array->values = NULL;
}

void bl_bytearray_init(VMState* vm, ByteArray* array, int length)
{
    array->count = length;
    array->bytes = (unsigned char*)calloc(length, sizeof(unsigned char));
    vm->bytesallocated += sizeof(unsigned char) * length;
}

void bl_valarray_push(VMState* vm, ValArray* array, Value value)
{
    if(array->capacity < array->count + 1)
    {
        int oldcapacity = array->capacity;
        array->capacity = GROW_CAPACITY(oldcapacity);
        array->values = GROW_ARRAY(Value, sizeof(Value), array->values, oldcapacity, array->capacity);
    }
    array->values[array->count] = value;
    array->count++;
}

void bl_valarray_insert(VMState* vm, ValArray* array, Value value, int index)
{
    if(array->capacity <= index)
    {
        array->capacity = GROW_CAPACITY(index);
        array->values = GROW_ARRAY(Value, sizeof(Value), array->values, array->count, array->capacity);
    }
    else if(array->capacity < array->count + 2)
    {
        int capacity = array->capacity;
        array->capacity = GROW_CAPACITY(capacity);
        array->values = GROW_ARRAY(Value, sizeof(Value), array->values, capacity, array->capacity);
    }
    if(index <= array->count)
    {
        for(int i = array->count - 1; i >= index; i--)
        {
            array->values[i + 1] = array->values[i];
        }
    }
    else
    {
        for(int i = array->count; i < index; i++)
        {
            array->values[i] = NIL_VAL;// nil out overflow indices
            array->count++;
        }
    }
    array->values[index] = value;
    array->count++;
}

void bl_valarray_free(VMState* vm, ValArray* array)
{
    FREE_ARRAY(Value, array->values, array->capacity);
    bl_valarray_init(array);
}

void bl_bytearray_free(VMState* vm, ByteArray* array)
{
    if(array && array->count > 0)
    {
        FREE_ARRAY(unsigned char, array->bytes, array->count);
        array->count = 0;
        array->bytes = NULL;
    }
}



Object* bl_object_allocobject(VMState* vm, size_t size, ObjType type)
{
    Object* object;
    object = (Object*)bl_mem_realloc(vm, NULL, 0, size);
    object->type = type;
    object->mark = false;
    object->sibling = vm->objectlinks;
    object->definitelyreal = true;
    vm->objectlinks = object;
    vm->objectcount++;
    //#if defined(DEBUG_LOG_GC) && DEBUG_LOG_GC
    //    fprintf(stderr, "bl_object_allocobject: size %ld type %d\n", size, type);
    //#endif
    return object;
}




ObjSwitch* bl_object_makeswitch(VMState* vm)
{
    ObjSwitch* sw = (ObjSwitch*)bl_object_allocobject(vm, sizeof(ObjSwitch), OBJ_SWITCH);
    bl_hashtable_init(&sw->table);
    sw->defaultjump = -1;
    sw->exitjump = -1;
    return sw;
}

ObjBytes* bl_object_makebytes(VMState* vm, int length)
{
    ObjBytes* bytes = (ObjBytes*)bl_object_allocobject(vm, sizeof(ObjBytes), OBJ_BYTES);
    bl_bytearray_init(vm, &bytes->bytes, length);
    return bytes;
}

ObjRange* bl_object_makerange(VMState* vm, int lower, int upper)
{
    ObjRange* range = (ObjRange*)bl_object_allocobject(vm, sizeof(ObjRange), OBJ_RANGE);
    range->lower = lower;
    range->upper = upper;
    if(upper > lower)
    {
        range->range = upper - lower;
    }
    else
    {
        range->range = lower - upper;
    }
    return range;
}

ObjFile* bl_object_makefile(VMState* vm, ObjString* path, ObjString* mode)
{
    ObjFile* file = (ObjFile*)bl_object_allocobject(vm, sizeof(ObjFile), OBJ_FILE);
    file->isopen = true;
    file->mode = mode;
    file->path = path;
    file->file = NULL;
    return file;
}

ObjBoundMethod* bl_object_makeboundmethod(VMState* vm, Value receiver, ObjClosure* method)
{
    ObjBoundMethod* bound = (ObjBoundMethod*)bl_object_allocobject(vm, sizeof(ObjBoundMethod), OBJ_BOUNDFUNCTION);
    bound->receiver = receiver;
    bound->method = method;
    return bound;
}

ObjClass* bl_object_makeclass(VMState* vm, ObjString* name, ObjClass* superclass)
{
    ObjClass* klass = (ObjClass*)bl_object_allocobject(vm, sizeof(ObjClass), OBJ_CLASS);
    klass->name = name;
    bl_hashtable_init(&klass->properties);
    bl_hashtable_init(&klass->staticproperties);
    bl_hashtable_init(&klass->methods);
    klass->initializer = EMPTY_VAL;
    klass->superclass = superclass;
    return klass;
}

ObjFunction* bl_object_makescriptfunction(VMState* vm, ObjModule* module, FuncType type)
{
    ObjFunction* function = (ObjFunction*)bl_object_allocobject(vm, sizeof(ObjFunction), OBJ_SCRIPTFUNCTION);
    function->arity = 0;
    function->upvaluecount = 0;
    function->isvariadic = false;
    function->name = NULL;
    function->type = type;
    function->module = module;
    bl_blob_init(&function->blob);
    return function;
}

ObjInstance* bl_object_makeinstance(VMState* vm, ObjClass* klass)
{
    ObjInstance* instance = (ObjInstance*)bl_object_allocobject(vm, sizeof(ObjInstance), OBJ_INSTANCE);
    bl_vm_pushvalue(vm, OBJ_VAL(instance));// gc fix
    instance->klass = klass;
    bl_hashtable_init(&instance->properties);
    if(klass->properties.count > 0)
    {
        bl_hashtable_copy(vm, &klass->properties, &instance->properties);
    }
    bl_vm_popvalue(vm);// gc fix
    return instance;
}

ObjNativeFunction* bl_object_makenativefunction(VMState* vm, NativeCallbackFunc function, const char* name)
{
    ObjNativeFunction* native = (ObjNativeFunction*)bl_object_allocobject(vm, sizeof(ObjNativeFunction), OBJ_NATIVEFUNCTION);
    native->natfn = function;
    native->name = name;
    native->type = TYPE_FUNCTION;
    return native;
}

ObjClosure* bl_object_makeclosure(VMState* vm, ObjFunction* function)
{
    ObjUpvalue** upvalues = ALLOCATE(ObjUpvalue*, function->upvaluecount);
    for(int i = 0; i < function->upvaluecount; i++)
    {
        upvalues[i] = NULL;
    }
    ObjClosure* closure = (ObjClosure*)bl_object_allocobject(vm, sizeof(ObjClosure), OBJ_CLOSURE);
    closure->fnptr = function;
    closure->upvalues = upvalues;
    closure->upvaluecount = function->upvaluecount;
    return closure;
}

ObjInstance* bl_object_makeexception(VMState* vm, ObjString* message)
{
    ObjInstance* instance = bl_object_makeinstance(vm, vm->exceptionclass);
    bl_vm_pushvalue(vm, OBJ_VAL(instance));
    bl_hashtable_set(vm, &instance->properties, STRING_L_VAL("message", 7), OBJ_VAL(message));
    bl_vm_popvalue(vm);
    return instance;
}

ObjUpvalue* bl_object_makeupvalue(VMState* vm, Value* slot)
{
    ObjUpvalue* upvalue = (ObjUpvalue*)bl_object_allocobject(vm, sizeof(ObjUpvalue), OBJ_UP_VALUE);
    upvalue->closed = NIL_VAL;
    upvalue->location = slot;
    upvalue->next = NULL;
    return upvalue;
}

static void bl_writer_printfunction(ObjFunction* func)
{
    if(func->name == NULL)
    {
        printf("<script at %p>", (void*)func);
    }
    else
    {
        printf(func->isvariadic ? "<function %s(%d...) at %p>" : "<function %s(%d) at %p>", func->name->chars, func->arity, (void*)func);
    }
}

static void bl_writer_printlist(ObjArray* list)
{
    printf("[");
    for(int i = 0; i < list->items.count; i++)
    {
        bl_value_printvalue(list->items.values[i]);
        if(i != list->items.count - 1)
        {
            printf(", ");
        }
    }
    printf("]");
}

static void bl_writer_printbytes(ObjBytes* bytes)
{
    printf("(");
    for(int i = 0; i < bytes->bytes.count; i++)
    {
        printf("%x", bytes->bytes.bytes[i]);
        if(i > 100)
        {// as bytes can get really heavy
            printf("...");
            break;
        }
        if(i != bytes->bytes.count - 1)
        {
            printf(" ");
        }
    }
    printf(")");
}

static void bl_writer_printdict(ObjDict* dict)
{
    printf("{");
    for(int i = 0; i < dict->names.count; i++)
    {
        bl_value_printvalue(dict->names.values[i]);
        printf(": ");
        Value value;
        if(bl_hashtable_get(&dict->items, dict->names.values[i], &value))
        {
            bl_value_printvalue(value);
        }
        if(i != dict->names.count - 1)
        {
            printf(", ");
        }
    }
    printf("}");
}

static void bl_writer_printfile(ObjFile* file)
{
    printf("<file at %s in mode %s>", file->path->chars, file->mode->chars);
}

void bl_writer_printobject(Value value, bool fixstring)
{
    switch(OBJ_TYPE(value))
    {
        case OBJ_SWITCH:
        {
            break;
        }
        case OBJ_PTR:
        {
            printf("%s", AS_PTR(value)->name);
            break;
        }
        case OBJ_RANGE:
        {
            ObjRange* range = AS_RANGE(value);
            printf("<range %d-%d>", range->lower, range->upper);
            break;
        }
        case OBJ_FILE:
        {
            bl_writer_printfile(AS_FILE(value));
            break;
        }
        case OBJ_DICT:
        {
            bl_writer_printdict(AS_DICT(value));
            break;
        }
        case OBJ_ARRAY:
        {
            bl_writer_printlist(AS_LIST(value));
            break;
        }
        case OBJ_BYTES:
        {
            bl_writer_printbytes(AS_BYTES(value));
            break;
        }
        case OBJ_BOUNDFUNCTION:
        {
            bl_writer_printfunction(AS_BOUND(value)->method->fnptr);
            break;
        }
        case OBJ_MODULE:
        {
            printf("<module %s at %s>", AS_MODULE(value)->name, AS_MODULE(value)->file);
            break;
        }
        case OBJ_CLASS:
        {
            printf("<class %s at %p>", AS_CLASS(value)->name->chars, (void*)AS_CLASS(value));
            break;
        }
        case OBJ_CLOSURE:
        {
            bl_writer_printfunction(AS_CLOSURE(value)->fnptr);
            break;
        }
        case OBJ_SCRIPTFUNCTION:
        {
            bl_writer_printfunction(AS_FUNCTION(value));
            break;
        }
        case OBJ_INSTANCE:
        {
            // @TODO: support the to_string() override
            ObjInstance* instance = AS_INSTANCE(value);
            printf("<class %s instance at %p>", instance->klass->name->chars, (void*)instance);
            break;
        }
        case OBJ_NATIVEFUNCTION:
        {
            ObjNativeFunction* native = AS_NATIVE(value);
            printf("<function %s(native) at %p>", native->name, (void*)native);
            break;
        }
        case OBJ_UP_VALUE:
        {
            printf("up value");
            break;
        }
        case OBJ_STRING:
        {
            ObjString* string = AS_STRING(value);
            if(fixstring)
            {
                printf(strchr(string->chars, '\'') != NULL ? "\"%.*s\"" : "'%.*s'", string->length, string->chars);
            }
            else
            {
                printf("%.*s", string->length, string->chars);
            }
            break;
        }
    }
}

ObjBytes* bl_bytes_copybytes(VMState* vm, unsigned char* b, int length)
{
    ObjBytes* bytes = bl_object_makebytes(vm, length);
    memcpy(bytes->bytes.bytes, b, length);
    return bytes;
}

ObjBytes* bl_bytes_takebytes(VMState* vm, unsigned char* b, int length)
{
    ObjBytes* bytes = (ObjBytes*)bl_object_allocobject(vm, sizeof(ObjBytes), OBJ_BYTES);
    bytes->bytes.count = length;
    bytes->bytes.bytes = b;
    return bytes;
}

static char* bl_writer_functiontostring(ObjFunction* func)
{
    if(func->name == NULL)
    {
        return strdup("<script 0x00>");
    }
    const char* format = func->isvariadic ? "<function %s(%d...)>" : "<function %s(%d)>";
    char* str = (char*)malloc(sizeof(char) * (snprintf(NULL, 0, format, func->name->chars, func->arity)));
    if(str != NULL)
    {
        sprintf(str, format, func->name->chars, func->arity);
        return str;
    }
    return strdup(func->name->chars);
}

static char* bl_writer_listtostring(VMState* vm, ValArray* array)
{
    int i;
    char* str;
    char* val;
    str = strdup("[");
    for(i = 0; i < array->count; i++)
    {
        val = bl_value_tostring(vm, array->values[i]);
        if(val != NULL)
        {
            str = bl_util_appendstring(str, val);
            free(val);
        }
        if(i != array->count - 1)
        {
            str = bl_util_appendstring(str, ", ");
        }
    }
    str = bl_util_appendstring(str, "]");
    return str;
}

static char* bl_writer_bytestostring(VMState* vm, ByteArray* array)
{
    char* str = strdup("(");
    for(int i = 0; i < array->count; i++)
    {
        char* chars = ALLOCATE(char, snprintf(NULL, 0, "0x%x", array->bytes[i]));
        if(chars != NULL)
        {
            sprintf(chars, "0x%x", array->bytes[i]);
            str = bl_util_appendstring(str, chars);
        }
        if(i != array->count - 1)
        {
            str = bl_util_appendstring(str, " ");
        }
    }
    str = bl_util_appendstring(str, ")");
    return str;
}

static char* bl_writer_dicttostring(VMState* vm, ObjDict* dict)
{
    char* str = strdup("{");
    for(int i = 0; i < dict->names.count; i++)
    {
        // bl_value_printvalue(dict->names.values[i]);
        Value key = dict->names.values[i];
        char* _key = bl_value_tostring(vm, key);
        if(_key != NULL)
        {
            str = bl_util_appendstring(str, _key);
        }
        str = bl_util_appendstring(str, ": ");
        Value value;
        bl_hashtable_get(&dict->items, key, &value);
        char* val = bl_value_tostring(vm, value);
        if(val != NULL)
        {
            str = bl_util_appendstring(str, val);
        }
        if(i != dict->names.count - 1)
        {
            str = bl_util_appendstring(str, ", ");
        }
    }
    str = bl_util_appendstring(str, "}");
    return str;
}

char* bl_writer_objecttostring(VMState* vm, Value value)
{
    switch(OBJ_TYPE(value))
    {
        case OBJ_PTR:
        {
            return strdup(AS_PTR(value)->name);
        }
        case OBJ_SWITCH:
        {
            return strdup("<switch>");
        }
        case OBJ_CLASS:
        {
            const char* format = "<class %s>";
            char* data = AS_CLASS(value)->name->chars;
            char* str = ALLOCATE(char, snprintf(NULL, 0, format, data));
            if(str != NULL)
            {
                sprintf(str, format, data);
            }
            return str;
        }
        case OBJ_INSTANCE:
        {
            const char* format = "<instance of %s>";
            char* data = AS_INSTANCE(value)->klass->name->chars;
            char* str = ALLOCATE(char, snprintf(NULL, 0, format, data));
            if(str != NULL)
            {
                sprintf(str, format, data);
            }
            return str;
        }
        case OBJ_CLOSURE:
            return bl_writer_functiontostring(AS_CLOSURE(value)->fnptr);
        case OBJ_BOUNDFUNCTION:
        {
            return bl_writer_functiontostring(AS_BOUND(value)->method->fnptr);
        }
        case OBJ_SCRIPTFUNCTION:
            return bl_writer_functiontostring(AS_FUNCTION(value));
        case OBJ_NATIVEFUNCTION:
        {
            const char* format = "<function %s(native)>";
            const char* data = AS_NATIVE(value)->name;
            char* str = ALLOCATE(char, snprintf(NULL, 0, format, data));
            if(str != NULL)
            {
                sprintf(str, format, data);
            }
            return str;
        }
        case OBJ_RANGE:
        {
            ObjRange* range = AS_RANGE(value);
            const char* format = "<range %d..%d>";
            char* str = ALLOCATE(char, snprintf(NULL, 0, format, range->lower, range->upper));
            if(str != NULL)
            {
                sprintf(str, format, range->lower, range->upper);
            }
            return str;
        }
        case OBJ_MODULE:
        {
            const char* format = "<module %s>";
            const char* data = AS_MODULE(value)->name;
            char* str = ALLOCATE(char, snprintf(NULL, 0, format, data));
            if(str != NULL)
            {
                sprintf(str, format, data);
            }
            return str;
        }
        case OBJ_STRING:
            return strdup(AS_C_STRING(value));
        case OBJ_UP_VALUE:
            return strdup("<up-value>");
        case OBJ_BYTES:
            return bl_writer_bytestostring(vm, &AS_BYTES(value)->bytes);
        case OBJ_ARRAY:
            return bl_writer_listtostring(vm, &AS_LIST(value)->items);
        case OBJ_DICT:
            return bl_writer_dicttostring(vm, AS_DICT(value));
        case OBJ_FILE:
        {
            ObjFile* file = AS_FILE(value);
            const char* format = "<file at %s in mode %s>";
            char* str = ALLOCATE(char, snprintf(NULL, 0, format, file->path->chars, file->mode->chars));
            if(str != NULL)
            {
                sprintf(str, format, file->path->chars, file->mode->chars);
            }
            return str;
        }
    }
    return NULL;
}

const char* bl_object_gettype(Object* object)
{
    switch(object->type)
    {
        case OBJ_MODULE:
            return "Module";
        case OBJ_BYTES:
            return "Bytes";
        case OBJ_RANGE:
            return "Range";
        case OBJ_FILE:
            return "File";
        case OBJ_DICT:
            return "Dictionary";
        case OBJ_ARRAY:
            return "List";
        case OBJ_CLASS:
            return "Class";
        case OBJ_SCRIPTFUNCTION:
        case OBJ_NATIVEFUNCTION:
        case OBJ_CLOSURE:
        case OBJ_BOUNDFUNCTION:
            return "Function";
        case OBJ_INSTANCE:
            return ((ObjInstance*)object)->klass->name->chars;
        case OBJ_STRING:
            return "String";
            //
        case OBJ_PTR:
            return "Pointer";
        case OBJ_SWITCH:
            return "Switch";
        default:
            break;
    }
    return "unknown";
}



#undef FILE_ERROR
#undef RETURN_STATUS
#undef SET_DICT_STRING
#undef DENY_STD
static const ModInitFunc builtinmodules[] = {
    &bl_modload_os,//
    // &bl_modload_io,//
    //&bl_modload_base64,//
    &bl_modload_math,//
    //&bl_modload_date,//
    //&bl_modload_socket,//
    //&bl_modload_hash,//
    &bl_modload_reflect,//
    &bl_modload_array,//
    &bl_modload_process,//
    &bl_modload_struct,//
    NULL,
};

bool load_module(VMState* vm, ModInitFunc init_fn, char* importname, char* source, void* handle)
{
    char* sdup;
    RegModule* module = init_fn(vm);
    if(module != NULL)
    {
        sdup = strdup(module->name);
        ObjModule* themodule = (ObjModule*)bl_mem_gcprotect(vm, (Object*)bl_object_makemodule(vm, sdup, source));
        themodule->preloader = (void*)module->preloader;
        themodule->unloader = (void*)module->unloader;
        if(module->fields != NULL)
        {
            for(int j = 0; module->fields[j].name != NULL; j++)
            {
                RegField field = module->fields[j];
                Value fieldname = GC_STRING(field.name);
                Value v = field.fieldfunc(vm);
                bl_vm_pushvalue(vm, v);
                bl_hashtable_set(vm, &themodule->values, fieldname, v);
                bl_vm_popvalue(vm);
            }
        }
        if(module->functions != NULL)
        {
            for(int j = 0; module->functions[j].name != NULL; j++)
            {
                RegFunc func = module->functions[j];
                Value funcname = GC_STRING(func.name);
                Value funcrealvalue = OBJ_VAL(bl_mem_gcprotect(vm, (Object*)bl_object_makenativefunction(vm, func.natfn, func.name)));
                bl_vm_pushvalue(vm, funcrealvalue);
                bl_hashtable_set(vm, &themodule->values, funcname, funcrealvalue);
                bl_vm_popvalue(vm);
            }
        }
        if(module->classes != NULL)
        {
            for(int j = 0; module->classes[j].name != NULL; j++)
            {
                RegClass klassreg = module->classes[j];
                ObjString* classname = (ObjString*)bl_mem_gcprotect(vm, (Object*)bl_string_copystringlen(vm, klassreg.name, (int)strlen(klassreg.name)));
                ObjClass* klass = (ObjClass*)bl_mem_gcprotect(vm, (Object*)bl_object_makeclass(vm, classname, NULL));
                if(klassreg.functions != NULL)
                {
                    for(int k = 0; klassreg.functions[k].name != NULL; k++)
                    {
                        RegFunc func = klassreg.functions[k];
                        Value funcname = GC_STRING(func.name);
                        ObjNativeFunction* native = (ObjNativeFunction*)bl_mem_gcprotect(vm, (Object*)bl_object_makenativefunction(vm, func.natfn, func.name));
                        if(func.isstatic)
                        {
                            native->type = TYPE_STATIC;
                        }
                        else if(strlen(func.name) > 0 && func.name[0] == '_')
                        {
                            native->type = TYPE_PRIVATE;
                        }
                        bl_hashtable_set(vm, &klass->methods, funcname, OBJ_VAL(native));
                    }
                }
                if(klassreg.fields != NULL)
                {
                    for(int k = 0; klassreg.fields[k].name != NULL; k++)
                    {
                        RegField field = klassreg.fields[k];
                        Value fieldname = GC_STRING(field.name);
                        Value v = field.fieldfunc(vm);
                        bl_vm_pushvalue(vm, v);
                        bl_hashtable_set(vm, field.isstatic ? &klass->staticproperties : &klass->properties, fieldname, v);
                        bl_vm_popvalue(vm);
                    }
                }
                bl_hashtable_set(vm, &themodule->values, OBJ_VAL(classname), OBJ_VAL(klass));
            }
        }
        if(handle != NULL)
        {
            themodule->handle = handle;// set handle for shared library modules
        }
        add_native_module(vm, themodule, themodule->name);
        free(sdup);
        bl_mem_gcclearprotect(vm);
        return true;
    }
    else
    {
        // @TODO: Warn about module loading error...
        printf("Error loading module: _%s\n", importname);
    }
    return false;
}

void add_native_module(VMState* vm, ObjModule* module, const char* as)
{
    if(as != NULL)
    {
        module->name = strdup(as);
    }
    Value name = STRING_VAL(module->name);
    bl_vm_pushvalue(vm, name);
    bl_vm_pushvalue(vm, OBJ_VAL(module));
    bl_hashtable_set(vm, &vm->modules, name, OBJ_VAL(module));
    bl_vm_popvaluen(vm, 2);
}

void bind_user_modules(VMState* vm, char* pkgroot)
{
    size_t dlen;
    int extlength;
    int pathlength;
    int namelength;
    char* dnam;
    char* path;
    char* name;
    char* filename;
    const char* error;
    struct stat sb;
    DIR* dir;
    struct dirent* ent;
    if(pkgroot == NULL)
    {
        return;
    }
    if((dir = opendir(pkgroot)) != NULL)
    {
        while((ent = readdir(dir)) != NULL)
        {
            extlength = (int)strlen(LIBRARY_FILE_EXTENSION);
            // skip . and .. in path
            dlen = strlen(ent->d_name);
            dnam = ent->d_name;
            if(((dlen == 1) && (dnam[0] == '.')) || ((dlen == 2) && ((dnam[0] == '.') && (dnam[1] == '.'))) || dlen < (size_t)(extlength + 1))
            {
                continue;
            }
            path = bl_util_mergepaths(pkgroot, ent->d_name);
            if(!path)
            {
                continue;
            }
            pathlength = (int)strlen(path);
            if(stat(path, &sb) == 0)
            {
                // it's not a directory
                if(S_ISDIR(sb.st_mode) < 1)
                {
                    if(memcmp(path + (pathlength - extlength), LIBRARY_FILE_EXTENSION, extlength) == 0)
                    {// library file
                        filename = bl_util_getrealfilename(path);
                        namelength = (int)strlen(filename) - extlength;
                        name = ALLOCATE(char, namelength + 1);
                        memcpy(name, filename, namelength);
                        name[namelength] = '\0';
                        error = load_user_module(vm, path, name);
                        if(error != NULL)
                        {
                            // @TODO: handle appropriately
                        }
                    }
                }
            }
        }
        closedir(dir);
    }
    bl_mem_gcclearprotect(vm);
}

void bind_native_modules(VMState* vm)
{
    int i;
    for(i = 0; builtinmodules[i] != NULL; i++)
    {
        load_module(vm, builtinmodules[i], NULL, strdup("<__native__>"), NULL);
    }
    //bind_user_modules(vm, bl_util_mergepaths(bl_util_getexedir(), "dist"));
    //bind_user_modules(vm, bl_util_mergepaths(getcwd(NULL, 0), LOCAL_PACKAGES_DIRECTORY LOCAL_EXT_DIRECTORY));
}

// fixme: should 
const char* load_user_module(VMState* vm, const char* path, char* name)
{
    int length;
    char* fnname;
    void* handle;
    ModInitFunc fn;
    length = (int)strlen(name) + 20;// 20 == strlen("blademoduleloader")
    fnname = ALLOCATE(char, length + 1);
    if(fnname == NULL)
    {
        return "failed to load module";
    }
    sprintf(fnname, "blademoduleloader%s", name);
    fnname[length] = '\0';// terminate the raw string
    if((handle = dlopen(path, RTLD_LAZY)) == NULL)
    {
        return dlerror();
    }
    fn = (ModInitFunc)dlsym(handle, fnname);
    if(fn == NULL)
    {
        return dlerror();
    }
    int pathlength = (int)strlen(path);
    char* modulefile = ALLOCATE(char, pathlength + 1);
    memcpy(modulefile, path, pathlength);
    modulefile[pathlength] = '\0';
    if(!load_module(vm, fn, name, modulefile, handle))
    {
        FREE_ARRAY(char, fnname, length + 1);
        FREE_ARRAY(char, modulefile, pathlength + 1);
        dlclose(handle);
        return "failed to call module loader";
    }
    return NULL;
}

void close_dl_module(void* handle)
{
    dlclose(handle);
}


bool bl_class_isinstanceof(ObjClass* klass1, char* klass2name)
{
    while(klass1 != NULL)
    {
        if((int)strlen(klass2name) == klass1->name->length && memcmp(klass1->name->chars, klass2name, klass1->name->length) == 0)
        {
            return true;
        }
        klass1 = klass1->superclass;
    }
    return false;
}

#undef ADD_TIME
#undef ADD_B_TIME
#undef ADD_S_TIME


bool modfn_math_sin(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(sin, 1);
    ENFORCE_ARG_TYPE(sin, 0, bl_value_isnumber);
    RETURN_NUMBER(sin(AS_NUMBER(args[0])));
}

bool modfn_math_cos(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(cos, 1);
    ENFORCE_ARG_TYPE(cos, 0, bl_value_isnumber);
    RETURN_NUMBER(cos(AS_NUMBER(args[0])));
}

bool modfn_math_tan(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(tan, 1);
    ENFORCE_ARG_TYPE(tan, 0, bl_value_isnumber);
    RETURN_NUMBER(tan(AS_NUMBER(args[0])));
}

bool modfn_math_sinh(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(sinh, 1);
    ENFORCE_ARG_TYPE(sinh, 0, bl_value_isnumber);
    RETURN_NUMBER(sinh(AS_NUMBER(args[0])));
}

bool modfn_math_cosh(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(cosh, 1);
    ENFORCE_ARG_TYPE(cosh, 0, bl_value_isnumber);
    RETURN_NUMBER(cosh(AS_NUMBER(args[0])));
}

bool modfn_math_tanh(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(tanh, 1);
    ENFORCE_ARG_TYPE(tanh, 0, bl_value_isnumber);
    RETURN_NUMBER(tanh(AS_NUMBER(args[0])));
}

bool modfn_math_asin(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(asin, 1);
    ENFORCE_ARG_TYPE(asin, 0, bl_value_isnumber);
    RETURN_NUMBER(asin(AS_NUMBER(args[0])));
}

bool modfn_math_acos(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(acos, 1);
    ENFORCE_ARG_TYPE(acos, 0, bl_value_isnumber);
    RETURN_NUMBER(acos(AS_NUMBER(args[0])));
}

bool modfn_math_atan(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(atan, 1);
    ENFORCE_ARG_TYPE(atan, 0, bl_value_isnumber);
    RETURN_NUMBER(atan(AS_NUMBER(args[0])));
}

bool modfn_math_atan2(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(atan2, 2);
    ENFORCE_ARG_TYPE(atan2, 0, bl_value_isnumber);
    ENFORCE_ARG_TYPE(atan2, 1, bl_value_isnumber);
    RETURN_NUMBER(atan2(AS_NUMBER(args[0]), AS_NUMBER(args[1])));
}

bool modfn_math_asinh(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(asinh, 1);
    ENFORCE_ARG_TYPE(asinh, 0, bl_value_isnumber);
    RETURN_NUMBER(asinh(AS_NUMBER(args[0])));
}

bool modfn_math_acosh(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(acosh, 1);
    ENFORCE_ARG_TYPE(acosh, 0, bl_value_isnumber);
    RETURN_NUMBER(acosh(AS_NUMBER(args[0])));
}

bool modfn_math_atanh(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(atanh, 1);
    ENFORCE_ARG_TYPE(atanh, 0, bl_value_isnumber);
    RETURN_NUMBER(atanh(AS_NUMBER(args[0])));
}

bool modfn_math_exp(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(exp, 1);
    ENFORCE_ARG_TYPE(exp, 0, bl_value_isnumber);
    RETURN_NUMBER(exp(AS_NUMBER(args[0])));
}

bool modfn_math_expm1(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(expm1, 1);
    ENFORCE_ARG_TYPE(expm1, 0, bl_value_isnumber);
    RETURN_NUMBER(expm1(AS_NUMBER(args[0])));
}

bool modfn_math_ceil(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(ceil, 1);
    ENFORCE_ARG_TYPE(ceil, 0, bl_value_isnumber);
    RETURN_NUMBER(ceil(AS_NUMBER(args[0])));
}

bool modfn_math_round(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(round, 1);
    ENFORCE_ARG_TYPE(round, 0, bl_value_isnumber);
    RETURN_NUMBER(round(AS_NUMBER(args[0])));
}

bool modfn_math_log(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(log, 1);
    ENFORCE_ARG_TYPE(log, 0, bl_value_isnumber);
    RETURN_NUMBER(log(AS_NUMBER(args[0])));
}

bool modfn_math_log10(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(log10, 1);
    ENFORCE_ARG_TYPE(log10, 0, bl_value_isnumber);
    RETURN_NUMBER(log10(AS_NUMBER(args[0])));
}

bool modfn_math_log2(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(log2, 1);
    ENFORCE_ARG_TYPE(log2, 0, bl_value_isnumber);
    RETURN_NUMBER(log2(AS_NUMBER(args[0])));
}

bool modfn_math_log1p(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(log1p, 1);
    ENFORCE_ARG_TYPE(log1p, 0, bl_value_isnumber);
    RETURN_NUMBER(log1p(AS_NUMBER(args[0])));
}

bool modfn_math_floor(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(floor, 1);
    if(bl_value_isnil(args[0]))
    {
        RETURN_NUMBER(0);
    }
    ENFORCE_ARG_TYPE(floor, 0, bl_value_isnumber);
    RETURN_NUMBER(floor(AS_NUMBER(args[0])));
}

RegModule* bl_modload_math(VMState* vm)
{
    (void)vm;
    static RegFunc modulefunctions[] = {
        { "sin", true, modfn_math_sin },
        { "cos", true, modfn_math_cos },
        { "tan", true, modfn_math_tan },
        { "sinh", true, modfn_math_sinh },
        { "cosh", true, modfn_math_cosh },
        { "tanh", true, modfn_math_tanh },
        { "asin", true, modfn_math_asin },
        { "acos", true, modfn_math_acos },
        { "atan", true, modfn_math_atan },
        { "atan2", true, modfn_math_atan2 },
        { "asinh", true, modfn_math_asinh },
        { "acosh", true, modfn_math_acosh },
        { "atanh", true, modfn_math_atanh },
        { "exp", true, modfn_math_exp },
        { "expm1", true, modfn_math_expm1 },
        { "ceil", true, modfn_math_ceil },
        { "round", true, modfn_math_round },
        { "log", true, modfn_math_log },
        { "log2", true, modfn_math_log2 },
        { "log10", true, modfn_math_log10 },
        { "log1p", true, modfn_math_log1p },
        { "floor", true, modfn_math_floor },
        { NULL, false, NULL },
    };
    static RegModule module = { .name = "_math", .fields = NULL, .functions = modulefunctions, .classes = NULL, .preloader = NULL, .unloader = NULL };
    return &module;
}

bool modfn_os_exec(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(exec, 1);
    ENFORCE_ARG_TYPE(exec, 0, bl_value_isstring);
    ObjString* string = AS_STRING(args[0]);
    if(string->length == 0)
    {
        return bl_value_returnnil(vm, args);
    }
    fflush(stdout);
    FILE* fd = popen(string->chars, "r");
    if(!fd)
    {
        return bl_value_returnnil(vm, args);
    }
    char buffer[256];
    size_t nread;
    size_t outputsize = 256;
    int length = 0;
    char* output = ALLOCATE(char, outputsize);
    if(output != NULL)
    {
        while((nread = fread(buffer, 1, sizeof(buffer), fd)) != 0)
        {
            if(length + nread >= outputsize)
            {
                size_t old = outputsize;
                outputsize *= 2;
                char* temp = GROW_ARRAY(char, sizeof(char), output, old, outputsize);
                if(temp == NULL)
                {
                    RETURN_ERROR("device out of memory");
                }
                else
                {
                    vm->bytesallocated += outputsize / 2;
                    output = temp;
                }
            }
            if((output + length) != NULL)
            {
                strncat(output + length, buffer, nread);
            }
            length += (int)nread;
        }
        if(length == 0)
        {
            pclose(fd);
            return bl_value_returnnil(vm, args);
        }
        output[length - 1] = '\0';
        pclose(fd);
        RETURN_T_STRING(output, length);
    }
    pclose(fd);
    RETURN_L_STRING("", 0);
}

bool modfn_os_info(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(info, 0);
#ifdef HAVE_SYS_UTSNAME_H
    struct utsname os;
    if(uname(&os) != 0)
    {
        RETURN_ERROR("could not access os information");
    }
    ObjDict* dict = (ObjDict*)bl_mem_gcprotect(vm, (Object*)bl_object_makedict(vm));
    bl_dict_addentry(vm, dict, GC_L_STRING("sysname", 7), GC_STRING(os.sysname));
    bl_dict_addentry(vm, dict, GC_L_STRING("nodename", 8), GC_STRING(os.nodename));
    bl_dict_addentry(vm, dict, GC_L_STRING("version", 7), GC_STRING(os.version));
    bl_dict_addentry(vm, dict, GC_L_STRING("release", 7), GC_STRING(os.release));
    bl_dict_addentry(vm, dict, GC_L_STRING("machine", 7), GC_STRING(os.machine));
    RETURN_OBJ(dict);
#else
    RETURN_ERROR("not available: OS does not have uname()")
#endif /* HAVE_SYS_UTSNAME_H */
}

bool modfn_os_sleep(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(sleep, 1);
    ENFORCE_ARG_TYPE(sleep, 0, bl_value_isnumber);
    sleep((int)AS_NUMBER(args[0]));
    return bl_value_returnempty(vm, args);
    ;
}

Value get_os_platform(VMState* vm)
{
#if defined(_WIN32)
    #define PLATFORM_NAME "windows"// Windows
#elif defined(_WIN64)
    #define PLATFORM_NAME "windows"// Windows
#elif defined(__CYGWIN__) && !defined(_WIN32)
    #define PLATFORM_NAME "windows"// Windows (Cygwin POSIX under Microsoft Window)
#elif defined(__ANDROID__)
    #define PLATFORM_NAME "android"// Android (implies Linux, so it must come first)
#elif defined(__linux__)
    #define PLATFORM_NAME "linux"// Debian, Ubuntu, Gentoo, Fedora, openSUSE, RedHat, Centos and other
#elif defined(__unix__) || !defined(__APPLE__) && defined(__MACH__)
    #if defined(BSD)
        #define PLATFORM_NAME "bsd"// FreeBSD, NetBSD, OpenBSD, DragonFly BSD
    #endif
#elif defined(__hpux)
    #define PLATFORM_NAME "hp-ux"// HP-UX
#elif defined(_AIX)
    #define PLATFORM_NAME "aix"// IBM AIX
#elif defined(__APPLE__) && defined(__MACH__)// Apple OSX and iOS (Darwin)
    #if TARGET_IPHONE_SIMULATOR == 1
        #define PLATFORM_NAME "ios"// Apple iOS
    #elif TARGET_OS_IPHONE == 1
        #define PLATFORM_NAME "ios"// Apple iOS
    #elif TARGET_OS_MAC == 1
        #define PLATFORM_NAME "osx"// Apple OSX
    #endif
#elif defined(__sun) && defined(__SVR4)
    #define PLATFORM_NAME "solaris"// Oracle Solaris, Open Indiana
#elif defined(__OS400__)
    #define PLATFORM_NAME "ibm"// IBM OS/400
#elif defined(AMIGA) || defined(__MORPHOS__)
    #define PLATFORM_NAME "amiga"
#else
    #define PLATFORM_NAME "unknown"
#endif
    return OBJ_VAL(bl_string_copystringlen(vm, PLATFORM_NAME, (int)strlen(PLATFORM_NAME)));
#undef PLATFORM_NAME
}

Value get_blade_os_args(VMState* vm)
{
    ObjArray* list = (ObjArray*)bl_mem_gcprotect(vm, (Object*)bl_object_makelist(vm));
    if(vm->stdargs != NULL)
    {
        for(int i = 0; i < vm->stdargscount; i++)
        {
            bl_array_push(vm, list, GC_STRING(vm->stdargs[i]));
        }
    }
    bl_mem_gcclearprotect(vm);
    return OBJ_VAL(list);
}

Value get_blade_os_path_separator(VMState* vm)
{
    return STRING_L_VAL(BLADE_PATH_SEPARATOR, 1);
}

bool modfn_os_getenv(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(getenv, 1);
    ENFORCE_ARG_TYPE(getenv, 0, bl_value_isstring);
    char* env = getenv(AS_C_STRING(args[0]));
    if(env != NULL)
    {
        RETURN_L_STRING(env, strlen(env));
    }
    else
    {
        return bl_value_returnnil(vm, args);
    }
}

bool modfn_os_setenv(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_RANGE(setenv, 2, 3);
    ENFORCE_ARG_TYPE(setenv, 0, bl_value_isstring);
    ENFORCE_ARG_TYPE(setenv, 1, bl_value_isstring);
    int overwrite = 1;
    if(argcount == 3)
    {
        ENFORCE_ARG_TYPE(setenv, 2, bl_value_isbool);
        overwrite = AS_BOOL(args[2]) ? 1 : 0;
    }
#ifdef _WIN32
    #define setenv(e, v, i) _putenv_s(e, v)
#endif
    if(setenv(AS_C_STRING(args[0]), AS_C_STRING(args[1]), overwrite) == 0)
    {
        RETURN_TRUE;
    }
    RETURN_FALSE;
}

bool modfn_os_createdir(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(createdir, 3);
    ENFORCE_ARG_TYPE(createdir, 0, bl_value_isstring);
    ENFORCE_ARG_TYPE(createdir, 1, bl_value_isnumber);
    ENFORCE_ARG_TYPE(createdir, 2, bl_value_isbool);
    ObjString* path = AS_STRING(args[0]);
    int mode = AS_NUMBER(args[1]);
    bool isrecursive = AS_BOOL(args[2]);
    char sep = BLADE_PATH_SEPARATOR[0];
    bool exists = false;
    if(isrecursive)
    {
        for(char* p = strchr(path->chars + 1, sep); p; p = strchr(p + 1, sep))
        {
            *p = '\0';
            if(mkdir(path->chars, mode) == -1)
            {
                if(errno != EEXIST)
                {
                    *p = sep;
                    RETURN_ERROR(strerror(errno));
                }
                else
                {
                    exists = true;
                }
            }
            else
            {
                exists = false;
            }
            //      chmod(path->chars, (mode_t) mode);
            *p = sep;
        }
    }
    else
    {
        if(mkdir(path->chars, mode) == -1)
        {
            if(errno != EEXIST)
            {
                RETURN_ERROR(strerror(errno));
            }
            else
            {
                exists = true;
            }
        }
        //    chmod(path->chars, (mode_t) mode);
    }
    RETURN_BOOL(!exists);
}

bool modfn_os_readdir(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(readdir, 1);
    ENFORCE_ARG_TYPE(readdir, 0, bl_value_isstring);
    ObjString* path = AS_STRING(args[0]);
    DIR* dir;
    if((dir = opendir(path->chars)) != NULL)
    {
        ObjArray* list = (ObjArray*)bl_mem_gcprotect(vm, (Object*)bl_object_makelist(vm));
        struct dirent* ent;
        while((ent = readdir(dir)) != NULL)
        {
            bl_array_push(vm, list, GC_STRING(ent->d_name));
        }
        closedir(dir);
        RETURN_OBJ(list);
    }
    RETURN_ERROR(strerror(errno));
}

static int remove_directory(char* path, int pathlength, bool recursive)
{
    DIR* dir;
    if((dir = opendir(path)) != NULL)
    {
        struct dirent* ent;
        while((ent = readdir(dir)) != NULL)
        {
            // skip . and .. in path
            if(memcmp(ent->d_name, ".", (int)strlen(ent->d_name)) == 0 || memcmp(ent->d_name, "..", (int)strlen(ent->d_name)) == 0)
            {
                continue;
            }
            int pathstringlength = pathlength + (int)strlen(ent->d_name) + 2;
            char* pathstring = bl_util_mergepaths(path, ent->d_name);
            if(pathstring == NULL)
            {
                return -1;
            }
            struct stat sb;
            if(stat(pathstring, &sb) == 0)
            {
                if(S_ISDIR(sb.st_mode) > 0 && recursive)
                {
                    // recurse
                    if(remove_directory(pathstring, pathstringlength, recursive) == -1)
                    {
                        free(pathstring);
                        return -1;
                    }
                }
                else if(unlink(pathstring) == -1)
                {
                    free(pathstring);
                    return -1;
                }
                else
                {
                    free(pathstring);
                }
            }
            else
            {
                free(pathstring);
                return -1;
            }
        }
        closedir(dir);
        return rmdir(path);
    }
    return -1;
}

bool modfn_os_removedir(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(removedir, 2);
    ENFORCE_ARG_TYPE(removedir, 0, bl_value_isstring);
    ENFORCE_ARG_TYPE(removedir, 1, bl_value_isbool);
    ObjString* path = AS_STRING(args[0]);
    bool recursive = AS_BOOL(args[1]);
    if(remove_directory(path->chars, path->length, recursive) >= 0)
    {
        RETURN_TRUE;
    }
    RETURN_ERROR(strerror(errno));
}

bool modfn_os_chmod(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(chmod, 2);
    ENFORCE_ARG_TYPE(chmod, 0, bl_value_isstring);
    ENFORCE_ARG_TYPE(chmod, 1, bl_value_isnumber);
    ObjString* path = AS_STRING(args[0]);
    int mode = AS_NUMBER(args[1]);
    if(chmod(path->chars, mode) != 0)
    {
        RETURN_ERROR(strerror(errno));
    }
    RETURN_TRUE;
}

bool modfn_os_isdir(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(isdir, 1);
    ENFORCE_ARG_TYPE(isdir, 0, bl_value_isstring);
    ObjString* path = AS_STRING(args[0]);
    struct stat sb;
    if(stat(path->chars, &sb) == 0)
    {
        RETURN_BOOL(S_ISDIR(sb.st_mode) > 0);
    }
    RETURN_FALSE;
}

bool modfn_os_exit(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(exit, 1);
    ENFORCE_ARG_TYPE(exit, 0, bl_value_isnumber);
    exit((int)AS_NUMBER(args[0]));
    return bl_value_returnempty(vm, args);
    ;
}

bool modfn_os_cwd(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(cwd, 0);
    char* cwd = getcwd(NULL, 0);
    if(cwd != NULL)
    {
        RETURN_TT_STRING(cwd);
    }
    RETURN_L_STRING("", 1);
}

bool modfn_os_realpath(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(_realpath, 1);
    ENFORCE_ARG_TYPE(_realpath, 0, bl_value_isstring);
    char* path = realpath(AS_C_STRING(args[0]), NULL);
    if(path != NULL)
    {
        RETURN_TT_STRING(path);
    }
    RETURN_VALUE(args[0]);
}

bool modfn_os_chdir(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(chdir, 1);
    ENFORCE_ARG_TYPE(chdir, 0, bl_value_isstring);
    RETURN_BOOL(chdir(AS_STRING(args[0])->chars) == 0);
}

bool modfn_os_exists(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(exists, 1);
    ENFORCE_ARG_TYPE(exists, 0, bl_value_isstring);
    struct stat sb;
    if(stat(AS_STRING(args[0])->chars, &sb) == 0 && sb.st_mode & S_IFDIR)
    {
        RETURN_TRUE;
    }
    RETURN_FALSE;
}

bool modfn_os_dirname(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(dirname, 1);
    ENFORCE_ARG_TYPE(dirname, 0, bl_value_isstring);
    char* dir = dirname(AS_STRING(args[0])->chars);
    if(!dir)
    {
        RETURN_VALUE(args[0]);
    }
    RETURN_L_STRING(dir, strlen(dir));
}

bool modfn_os_basename(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(basename, 1);
    ENFORCE_ARG_TYPE(basename, 0, bl_value_isstring);
    char* dir = basename(AS_STRING(args[0])->chars);
    if(!dir)
    {
        RETURN_VALUE(args[0]);
    }
    RETURN_L_STRING(dir, strlen(dir));
}

/** DIR TYPES BEGIN */
Value modfield_os_dtunknown(VMState* vm)
{
    (void)vm;
    return NUMBER_VAL(DT_UNKNOWN);
}

Value modfield_os_dtreg(VMState* vm)
{
    (void)vm;
    return NUMBER_VAL(DT_REG);
}

Value modfield_os_dtdir(VMState* vm)
{
    (void)vm;
    return NUMBER_VAL(DT_DIR);
}

Value modfield_os_dtfifo(VMState* vm)
{
    (void)vm;
    return NUMBER_VAL(DT_FIFO);
}

Value modfield_os_dtsock(VMState* vm)
{
    (void)vm;
    return NUMBER_VAL(DT_SOCK);
}

Value modfield_os_dtchr(VMState* vm)
{
    (void)vm;
    return NUMBER_VAL(DT_CHR);
}

Value modfield_os_dtblk(VMState* vm)
{
    (void)vm;
    return NUMBER_VAL(DT_BLK);
}

Value modfield_os_dtlnk(VMState* vm)
{
    (void)vm;
    return NUMBER_VAL(DT_LNK);
}

Value modfield_os_dtwht(VMState* vm)
{
    (void)vm;
    return NUMBER_VAL(-1);
}

void __os_module_preloader(VMState* vm)
{
    (void)vm;
#if defined WIN32 || defined _WIN32 || defined WIN64 || defined _WIN64
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
    // References:
    //SetConsoleMode() and ENABLE_VIRTUAL_TERMINAL_PROCESSING?
    //https://stackoverflow.com/questions/38772468/setconsolemode-and-enable-virtual-terminal-processing
    // Windows console with ANSI colors handling
    // https://superuser.com/questions/413073/windows-console-with-ansi-colors-handling
#endif
}

/** DIR TYPES ENDS */
RegModule* bl_modload_os(VMState* vm)
{
    (void)vm;
    static RegFunc osmodulefunctions[] = {
        { "info", true, modfn_os_info },       { "exec", true, modfn_os_exec },         { "sleep", true, modfn_os_sleep },
        { "getenv", true, modfn_os_getenv },   { "setenv", true, modfn_os_setenv },     { "createdir", true, modfn_os_createdir },
        { "readdir", true, modfn_os_readdir }, { "chmod", true, modfn_os_chmod },       { "isdir", true, modfn_os_isdir },
        { "exit", true, modfn_os_exit },       { "cwd", true, modfn_os_cwd },           { "removedir", true, modfn_os_removedir },
        { "chdir", true, modfn_os_chdir },     { "exists", true, modfn_os_exists },     { "realpath", true, modfn_os_realpath },
        { "dirname", true, modfn_os_dirname }, { "basename", true, modfn_os_basename }, { NULL, false, NULL },
    };
    static RegField osmodulefields[] = {
        { "platform", true, get_os_platform },
        { "args", true, get_blade_os_args },
        { "pathseparator", true, get_blade_os_path_separator },
        { "DT_UNKNOWN", true, modfield_os_dtunknown },
        { "DT_BLK", true, modfield_os_dtblk },
        { "DT_CHR", true, modfield_os_dtchr },
        { "DT_DIR", true, modfield_os_dtdir },
        { "DT_FIFO", true, modfield_os_dtfifo },
        { "DT_LNK", true, modfield_os_dtlnk },
        { "DT_REG", true, modfield_os_dtreg },
        { "DT_SOCK", true, modfield_os_dtsock },
        { "DT_WHT", true, modfield_os_dtwht },
        { NULL, false, NULL },
    };
    static RegModule module
    = { .name = "_os", .fields = osmodulefields, .functions = osmodulefunctions, .classes = NULL, .preloader = &__os_module_preloader, .unloader = NULL };
    return &module;
}

Value modfield_process_cpucount(VMState* vm)
{
    (void)vm;
    return NUMBER_VAL(1);
}

void b__free_shared_memory(void* data)
{
    BProcessShared* shared = (BProcessShared*)data;
    munmap(shared->format, shared->formatlength * sizeof(char));
    munmap(shared->getformat, shared->getformatlength * sizeof(char));
    munmap(shared->bytes, shared->length * sizeof(unsigned char));
    munmap(shared, sizeof(BProcessShared));
}

bool modfn_process_process(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_RANGE(Process, 0, 1);
    BProcess* process = ALLOCATE(BProcess, 1);
    ObjPointer* ptr = (ObjPointer*)bl_mem_gcprotect(vm, (Object*)bl_dict_makeptr(vm, process));
    ptr->name = "<*Process::Process>";
    process->pid = -1;
    RETURN_OBJ(ptr);
}

bool modfn_process_create(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(create, 1);
    ENFORCE_ARG_TYPE(create, 0, bl_value_ispointer);
    BProcess* process = (BProcess*)AS_PTR(args[0])->pointer;
    int pid = fork();
    if(pid == -1)
    {
        RETURN_NUMBER(-1);
    }
    else if(!pid)
    {
        process->pid = getpid();
        RETURN_NUMBER(0);
    }
    RETURN_NUMBER(getpid());
}

bool modfn_process_isalive(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(create, 1);
    ENFORCE_ARG_TYPE(create, 0, bl_value_ispointer);
    BProcess* process = (BProcess*)AS_PTR(args[0])->pointer;
    RETURN_BOOL(waitpid(process->pid, NULL, WNOHANG) == 0);
}

bool modfn_process_kill(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(kill, 1);
    ENFORCE_ARG_TYPE(kill, 0, bl_value_ispointer);
    BProcess* process = (BProcess*)AS_PTR(args[0])->pointer;
    RETURN_BOOL(kill(process->pid, SIGKILL) == 0);
}

bool modfn_process_wait(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(create, 1);
    ENFORCE_ARG_TYPE(create, 0, bl_value_ispointer);
    BProcess* process = (BProcess*)AS_PTR(args[0])->pointer;
    int status;
    waitpid(process->pid, &status, 0);
    int p;
    do
    {
        p = waitpid(process->pid, &status, 0);
        if(p == -1)
        {
            if(errno == EINTR)
            {
                continue;
            }
            break;
        }
    } while(p != process->pid);
    if(p == process->pid)
    {
        RETURN_NUMBER(status);
    }
    RETURN_NUMBER(-1);
}

bool modfn_process_id(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(create, 1);
    ENFORCE_ARG_TYPE(create, 0, bl_value_ispointer);
    BProcess* process = (BProcess*)AS_PTR(args[0])->pointer;
    RETURN_NUMBER(process->pid);
}

bool modfn_process_newshared(VMState* vm, int argcount, Value* args)
{
    ObjPointer* ptr;
    BProcessShared* shared;
    ENFORCE_ARG_COUNT(newshared, 0);
    shared = (BProcessShared*)mmap(NULL, sizeof(BProcessShared), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    shared->bytes = (unsigned char*)mmap(NULL, sizeof(unsigned char), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    shared->format = (char*)mmap(NULL, sizeof(char), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    shared->getformat = (char*)mmap(NULL, sizeof(char), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    shared->length = shared->getformatlength = shared->formatlength = 0;
    ptr = (ObjPointer*)bl_mem_gcprotect(vm, (Object*)bl_dict_makeptr(vm, shared));
    ptr->name = "<*Process::SharedValue>";
    ptr->fnptrfree = b__free_shared_memory;
    RETURN_OBJ(ptr);
}

bool modfn_process_sharedwrite(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(sharedwrite, 4);
    ENFORCE_ARG_TYPE(sharedwrite, 0, bl_value_ispointer);
    ENFORCE_ARG_TYPE(sharedwrite, 1, bl_value_isstring);
    ENFORCE_ARG_TYPE(sharedwrite, 2, bl_value_isstring);
    ENFORCE_ARG_TYPE(sharedwrite, 3, bl_value_isbytes);
    BProcessShared* shared = (BProcessShared*)AS_PTR(args[0])->pointer;
    if(!shared->locked)
    {
        ObjString* format = AS_STRING(args[1]);
        ObjString* getformat = AS_STRING(args[2]);
        ByteArray bytes = AS_BYTES(args[3])->bytes;
        memcpy(shared->format, format->chars, format->length);
        shared->formatlength = format->length;
        memcpy(shared->getformat, getformat->chars, getformat->length);
        shared->getformatlength = getformat->length;
        memcpy(shared->bytes, bytes.bytes, bytes.count);
        shared->length = bytes.count;
        // return length written
        RETURN_NUMBER(shared->length);
    }
    RETURN_FALSE;
}

bool modfn_process_sharedread(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(sharedread, 1);
    ENFORCE_ARG_TYPE(sharedread, 0, bl_value_ispointer);
    BProcessShared* shared = (BProcessShared*)AS_PTR(args[0])->pointer;
    if(shared->length > 0 || shared->formatlength > 0)
    {
        ObjBytes* bytes = (ObjBytes*)bl_mem_gcprotect(vm, (Object*)bl_bytes_copybytes(vm, shared->bytes, shared->length));
        // return [format, bytes]
        ObjArray* list = (ObjArray*)bl_mem_gcprotect(vm, (Object*)bl_object_makelist(vm));
        bl_array_push(vm, list, GC_L_STRING(shared->getformat, shared->getformatlength));
        bl_array_push(vm, list, OBJ_VAL(bytes));
        RETURN_OBJ(list);
    }
    return bl_value_returnnil(vm, args);
}

bool modfn_process_sharedlock(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(sharedlock, 1);
    ENFORCE_ARG_TYPE(sharedlock, 0, bl_value_ispointer);
    BProcessShared* shared = (BProcessShared*)AS_PTR(args[0])->pointer;
    shared->locked = true;
    return bl_value_returnempty(vm, args);
    ;
}

bool modfn_process_sharedunlock(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(sharedunlock, 1);
    ENFORCE_ARG_TYPE(sharedunlock, 0, bl_value_ispointer);
    BProcessShared* shared = (BProcessShared*)AS_PTR(args[0])->pointer;
    shared->locked = false;
    return bl_value_returnempty(vm, args);
    ;
}

RegModule* bl_modload_process(VMState* vm)
{
    (void)vm;
    static RegFunc osmodulefunctions[] = {
        { "Process", false, modfn_process_process },
        { "create", false, modfn_process_create },
        { "isalive", false, modfn_process_isalive },
        { "wait", false, modfn_process_wait },
        { "id", false, modfn_process_id },
        { "kill", false, modfn_process_kill },
        { "newshared", false, modfn_process_newshared },
        { "sharedwrite", false, modfn_process_sharedwrite },
        { "sharedread", false, modfn_process_sharedread },
        { "sharedlock", false, modfn_process_sharedlock },
        { "sharedunlock", false, modfn_process_sharedunlock },
        { NULL, false, NULL },
    };
    static RegField osmodulefields[] = {
        { "cpucount", true, modfield_process_cpucount },
        { NULL, false, NULL },
    };
    static RegModule module = { .name = "_process", .fields = osmodulefields, .functions = osmodulefunctions, .classes = NULL, .preloader = NULL, .unloader = NULL };
    return &module;
}

/**
 * hasprop(object: instance, name: string)
 *
 * returns true if object has the property name or false if not
 */
bool modfn_reflect_hasprop(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(hasprop, 2);
    ENFORCE_ARG_TYPE(hasprop, 0, bl_value_isinstance);
    ENFORCE_ARG_TYPE(hasprop, 1, bl_value_isstring);
    ObjInstance* instance = AS_INSTANCE(args[0]);
    Value dummy;
    RETURN_BOOL(bl_hashtable_get(&instance->properties, args[1], &dummy));
}

/**
 * getprop(object: instance, name: string)
 *
 * returns the property of the object matching the given name
 * or nil if the object contains no property with a matching
 * name
 */
bool modfn_reflect_getprop(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(getprop, 2);
    ENFORCE_ARG_TYPE(getprop, 0, bl_value_isinstance);
    ENFORCE_ARG_TYPE(getprop, 1, bl_value_isstring);
    ObjInstance* instance = AS_INSTANCE(args[0]);
    Value value;
    if(bl_hashtable_get(&instance->properties, args[1], &value))
    {
        RETURN_VALUE(value);
    }
    return bl_value_returnnil(vm, args);
}

/**
 * setprop(object: instance, name: string, value: any)
 *
 * sets the named property of the object to value.
 *
 * if the property already exist, it overwrites it
 * @returns bool: true if a new property was set, false if a property was
 * updated
 */
bool modfn_reflect_setprop(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(setprop, 3);
    ENFORCE_ARG_TYPE(setprop, 0, bl_value_isinstance);
    ENFORCE_ARG_TYPE(setprop, 1, bl_value_isstring);
    ENFORCE_ARG_TYPE(setprop, 2, bl_value_isstring);
    ObjInstance* instance = AS_INSTANCE(args[0]);
    RETURN_BOOL(bl_hashtable_set(vm, &instance->properties, args[1], args[2]));
}

/**
 * delprop(object: instance, name: string)
 *
 * deletes the named property from the object
 * @returns bool
 */
bool modfn_reflect_delprop(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(delprop, 2);
    ENFORCE_ARG_TYPE(delprop, 0, bl_value_isinstance);
    ENFORCE_ARG_TYPE(delprop, 1, bl_value_isstring);
    ObjInstance* instance = AS_INSTANCE(args[0]);
    RETURN_BOOL(bl_hashtable_delete(&instance->properties, args[1]));
}

/**
 * hasmethod(object: instance, name: string)
 *
 * returns true if class of the instance has the method name or
 * false if not
 */
bool modfn_reflect_hasmethod(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(hasmethod, 2);
    ENFORCE_ARG_TYPE(hasmethod, 0, bl_value_isinstance);
    ENFORCE_ARG_TYPE(hasmethod, 1, bl_value_isstring);
    ObjInstance* instance = AS_INSTANCE(args[0]);
    Value dummy;
    RETURN_BOOL(bl_hashtable_get(&instance->klass->methods, args[1], &dummy));
}

/**
 * getmethod(object: instance, name: string)
 *
 * returns the method in a class instance matching the given name
 * or nil if the class of the instance contains no method with
 * a matching name
 */
bool modfn_reflect_getmethod(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(getmethod, 2);
    ENFORCE_ARG_TYPE(getmethod, 0, bl_value_isinstance);
    ENFORCE_ARG_TYPE(getmethod, 1, bl_value_isstring);
    ObjInstance* instance = AS_INSTANCE(args[0]);
    Value value;
    if(bl_hashtable_get(&instance->klass->methods, args[1], &value))
    {
        RETURN_VALUE(value);
    }
    return bl_value_returnnil(vm, args);
}

bool modfn_reflect_callmethod(VMState* vm, int argcount, Value* args)
{
    ENFORCE_MIN_ARG(callmethod, 3);
    ENFORCE_ARG_TYPE(callmethod, 0, bl_value_isinstance);
    ENFORCE_ARG_TYPE(callmethod, 1, bl_value_isstring);
    ENFORCE_ARG_TYPE(callmethod, 2, bl_value_isarray);
    Value value;
    if(bl_hashtable_get(&AS_INSTANCE(args[0])->klass->methods, args[1], &value))
    {
        ObjBoundMethod* bound = (ObjBoundMethod*)bl_mem_gcprotect(vm, (Object*)bl_object_makeboundmethod(vm, args[0], AS_CLOSURE(value)));
        ObjArray* list = AS_LIST(args[2]);
        // remove the args list, the string name and the instance
        // then push the bound method
        bl_vm_popvaluen(vm, 3);
        bl_vm_pushvalue(vm, OBJ_VAL(bound));
        // convert the list into function args
        for(int i = 0; i < list->items.count; i++)
        {
            bl_vm_pushvalue(vm, list->items.values[i]);
        }
        return bl_vm_callvalue(vm, OBJ_VAL(bound), list->items.count);
    }
    return bl_value_returnempty(vm, args);
}

bool modfn_reflect_bindmethod(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(delist, 2);
    ENFORCE_ARG_TYPE(delist, 0, bl_value_isinstance);
    ENFORCE_ARG_TYPE(delist, 1, bl_value_isclosure);
    ObjBoundMethod* bound = (ObjBoundMethod*)bl_mem_gcprotect(vm, (Object*)bl_object_makeboundmethod(vm, args[0], AS_CLOSURE(args[1])));
    RETURN_OBJ(bound);
}

bool modfn_reflect_getboundmethod(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(getmethod, 2);
    ENFORCE_ARG_TYPE(getmethod, 0, bl_value_isinstance);
    ENFORCE_ARG_TYPE(getmethod, 1, bl_value_isstring);
    ObjInstance* instance = AS_INSTANCE(args[0]);
    Value value;
    if(bl_hashtable_get(&instance->klass->methods, args[1], &value))
    {
        ObjBoundMethod* bound = (ObjBoundMethod*)bl_mem_gcprotect(vm, (Object*)bl_object_makeboundmethod(vm, args[0], AS_CLOSURE(value)));
        RETURN_OBJ(bound);
    }
    return bl_value_returnnil(vm, args);
}

bool modfn_reflect_gettype(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(gettype, 1);
    ENFORCE_ARG_TYPE(gettype, 0, bl_value_isinstance);
    RETURN_OBJ(AS_INSTANCE(args[0])->klass->name);
}

bool modfn_reflect_isptr(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(isptr, 1);
    RETURN_BOOL(bl_value_ispointer(args[0]));
}

bool modfn_reflect_getfunctionmetadata(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(getfunctionmetadata, 1);
    ENFORCE_ARG_TYPE(getfunctionmetadata, 0, bl_value_isclosure);
    ObjClosure* closure = AS_CLOSURE(args[0]);
    ObjDict* result = (ObjDict*)bl_mem_gcprotect(vm, (Object*)bl_object_makedict(vm));
    bl_dict_setentry(vm, result, GC_STRING("name"), OBJ_VAL(closure->fnptr->name));
    bl_dict_setentry(vm, result, GC_STRING("arity"), NUMBER_VAL(closure->fnptr->arity));
    bl_dict_setentry(vm, result, GC_STRING("isvariadic"), NUMBER_VAL(closure->fnptr->isvariadic));
    bl_dict_setentry(vm, result, GC_STRING("capturedvars"), NUMBER_VAL(closure->upvaluecount));
    bl_dict_setentry(vm, result, GC_STRING("module"), STRING_VAL(closure->fnptr->module->name));
    bl_dict_setentry(vm, result, GC_STRING("file"), STRING_VAL(closure->fnptr->module->file));
    RETURN_OBJ(result);
}

RegModule* bl_modload_reflect(VMState* vm)
{
    (void)vm;
    static RegFunc modulefunctions[] = {
        { "hasprop", true, modfn_reflect_hasprop },
        { "getprop", true, modfn_reflect_getprop },
        { "setprop", true, modfn_reflect_setprop },
        { "delprop", true, modfn_reflect_delprop },
        { "hasmethod", true, modfn_reflect_hasmethod },
        { "getmethod", true, modfn_reflect_getmethod },
        { "getboundmethod", true, modfn_reflect_getboundmethod },
        { "callmethod", true, modfn_reflect_callmethod },
        { "bindmethod", true, modfn_reflect_bindmethod },
        { "gettype", true, modfn_reflect_gettype },
        { "isptr", true, modfn_reflect_isptr },
        { "getfunctionmetadata", true, modfn_reflect_getfunctionmetadata },
        { NULL, false, NULL },
    };
    static RegModule module = { .name = "_reflect", .fields = NULL, .functions = modulefunctions, .classes = NULL, .preloader = NULL, .unloader = NULL };
    return &module;
}

/**
 * The implementation of this module is based on the PHP implementation of
 * pack/unpack which can be found at ext/standard/pack.c in the PHP source code.
 *
 * The original license has been maintained here for reference.
 * @copyright Ore Richard and Blade contributors.
 */
/*
   +----------------------------------------------------------------------+
   | Copyright (c) The PHP Group                                          |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | https://www.php.net/license/301.txt                                 |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Chris Schneider <cschneid@relog.ch>                          |
   +----------------------------------------------------------------------+
 */
#define INC_OUTPUTPOS(a, b) \
    if((a) < 0 || ((INT_MAX - outputpos) / ((int)b)) < (a)) \
    { \
        free(formatcodes); \
        free(formatargs); \
        RETURN_ERROR("Type %c: integer overflow in format string", code); \
    } \
    outputpos += (a) * (b);
#define MAX_LENGTH_OF_LONG 20
//#define LONG_FMT "%" PRId64
#define UNPACK_REAL_NAME() (strspn(realname, "0123456789") == strlen(realname) ? NUMBER_VAL(strtod(realname, NULL)) : (GC_STRING(realname)))
#ifndef MAX
    #define MAX(a, b) (a > b ? a : b)
#endif

static long to_long(VMState* vm, Value value)
{
    if(bl_value_isnumber(value))
    {
        return (long)AS_NUMBER(value);
    }
    else if(bl_value_isbool(value))
    {
        return AS_BOOL(value) ? 1L : 0L;
    }
    else if(bl_value_isnil(value))
    {
        return -1L;
    }
    const char* v = (const char*)bl_value_tostring(vm, value);
    int length = (int)strlen(v);
    int start = 0;
    int end = 1;
    int multiplier = 1;
    if(v[0] == '-')
    {
        start++;
        end++;
        multiplier = -1;
    }
    if(length > (end + 1) && v[start] == '0')
    {
        char* t = ALLOCATE(char, length - 2);
        memcpy(t, v + (end + 1), length - 2);
        if(v[end] == 'b')
        {
            return (long)(multiplier * strtoll(t, NULL, 2));
        }
        else if(v[end] == 'x')
        {
            return multiplier * strtol(t, NULL, 16);
        }
        else if(v[end] == 'c')
        {
            return multiplier * strtol(t, NULL, 8);
        }
    }
    return (long)strtod(v, NULL);
}

static double to_double(VMState* vm, Value value)
{
    if(bl_value_isnumber(value))
    {
        return AS_NUMBER(value);
    }
    else if(bl_value_isbool(value))
    {
        return AS_BOOL(value) ? 1 : 0;
    }
    else if(bl_value_isnil(value))
    {
        return -1;
    }
    const char* v = (const char*)bl_value_tostring(vm, value);
    int length = (int)strlen(v);
    int start = 0;
    int end = 1;
    int multiplier = 1;
    if(v[0] == '-')
    {
        start++;
        end++;
        multiplier = -1;
    }
    if(length > (end + 1) && v[start] == '0')
    {
        char* t = ALLOCATE(char, length - 2);
        memcpy(t, v + (end + 1), length - 2);
        if(v[end] == 'b')
        {
            return (double)(multiplier * strtoll(t, NULL, 2));
        }
        else if(v[end] == 'x')
        {
            return (double)(multiplier * strtol(t, NULL, 16));
        }
        else if(v[end] == 'c')
        {
            return (double)(multiplier * strtol(t, NULL, 8));
        }
    }
    return strtod(v, NULL);
}

static void do_pack(VMState* vm, Value val, size_t size, const int* map, unsigned char* output)
{
    size_t i;
    long aslong = to_long(vm, val);
    char* v = (char*)&aslong;
    for(i = 0; i < size; i++)
    {
        *output++ = v[map[i]];
    }
}

/* Mapping of byte from char (8bit) to long for machine endian */
static int bytemap[1];
/* Mappings of bytes from int (machine dependent) to int for machine endian */
static int intmap[sizeof(int)];
/* Mappings of bytes from shorts (16bit) for all endian environments */
static int machineendianshortmap[2];
static int bigendianshortmap[2];
static int littleendianshortmap[2];
/* Mappings of bytes from longs (32bit) for all endian environments */
static int machineendianlongmap[4];
static int bigendianlongmap[4];
static int littleendianlongmap[4];

#if IS_64_BIT
/* Mappings of bytes from quads (64bit) for all endian environments */
static int machineendianlonglongmap[8];
static int bigendianlonglongmap[8];
static int littleendianlonglongmap[8];
#endif

bool modfn_struct_pack(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(pack, 2);
    ENFORCE_ARG_TYPE(pack, 0, bl_value_isstring);
    ENFORCE_ARG_TYPE(pack, 1, bl_value_isarray);
    ObjString* string = AS_STRING(args[0]);
    ObjArray* params = AS_LIST(args[1]);
    Value* argslist = params->items.values;
    int paramcount = params->items.count;
    size_t i;
    int currentarg;
    char* format = string->chars;
    size_t formatlen = string->length;
    size_t formatcount = 0;
    int outputpos = 0;
    int outputsize = 0;
    /* We have a maximum of <formatlen> format codes to deal with */
    char* formatcodes = ALLOCATE(char, formatlen);
    int* formatargs = ALLOCATE(int, formatlen);
    currentarg = 0;
    for(i = 0; i < formatlen; formatcount++)
    {
        char code = format[i++];
        int arg = 1;
        /* Handle format arguments if any */
        if(i < formatlen)
        {
            char c = format[i];
            if(c == '*')
            {
                arg = -1;
                i++;
            }
            else if(c >= '0' && c <= '9')
            {
                arg = (int)strtol(&format[i], NULL, 10);
                while(format[i] >= '0' && format[i] <= '9' && i < formatlen)
                {
                    i++;
                }
            }
        }
        /* Handle special arg '*' for all codes and check argv overflows */
        switch((int)code)
        {
            /* Never uses any argslist */
            case 'x':
            case 'X':
            case '@':
                if(arg < 0)
                {
                    // TODO: Give warning...
                    //          RETURN_ERROR("Type %c: '*' ignored", code);
                    arg = 1;
                }
                break;
                /* Always uses one arg */
            case 'a':
            case 'A':
            case 'Z':
            case 'h':
            case 'H':
                if(currentarg >= paramcount)
                {
                    free(formatcodes);
                    free(formatargs);
                    RETURN_ERROR("Type %c: not enough arguments", code);
                }
                if(arg < 0)
                {
                    char* asstring = bl_value_tostring(vm, argslist[currentarg]);
                    arg = (int)strlen(asstring);
                    if(code == 'Z')
                    {
                        /* add one because Z is always NUL-terminated:
             * pack("Z*", "aa") === "aa\0"
             * pack("Z2", "aa") === "a\0" */
                        arg++;
                    }
                }
                currentarg++;
                break;
                /* Use as many argslist as specified */
            case 'q':
            case 'Q':
            case 'J':
            case 'P':
#if !IS_64_BIT
                free(formatcodes);
                free(formatargs);
                RETURN_ERROR("64-bit format codes are not available for 32-bit builds of Blade");
#endif
            case 'c':
            case 'C':
            case 's':
            case 'S':
            case 'i':
            case 'I':
            case 'l':
            case 'L':
            case 'n':
            case 'N':
            case 'v':
            case 'V':
            case 'f': /* float */
            case 'g': /* little endian float */
            case 'G': /* big endian float */
            case 'd': /* double */
            case 'e': /* little endian double */
            case 'E': /* big endian double */
                if(arg < 0)
                {
                    arg = paramcount - currentarg;
                }
                if(currentarg > INT_MAX - arg)
                {
                    goto toofewargs;
                }
                currentarg += arg;
                if(currentarg > paramcount)
                {
                toofewargs:
                    free(formatcodes);
                    free(formatargs);
                    RETURN_ERROR("Type %c: too few arguments", code);
                }
                break;
            default:
                free(formatcodes);
                free(formatargs);
                RETURN_ERROR("Type %c: unknown format code", code);
        }
        formatcodes[formatcount] = code;
        formatargs[formatcount] = arg;
    }
    if(currentarg < paramcount)
    {
        // TODO: Give warning...
        //    RETURN_ERROR("%d arguments unused", (paramcount - currentarg));
    }
    /* Calculate output length and upper bound while processing*/
    for(i = 0; i < formatcount; i++)
    {
        int code = (int)formatcodes[i];
        int arg = formatargs[i];
        switch((int)code)
        {
            case 'h':
            case 'H':
                INC_OUTPUTPOS((arg + (arg % 2)) / 2, 1); /* 4 bit per arg */
                break;
            case 'a':
            case 'A':
            case 'Z':
            case 'c':
            case 'C':
            case 'x':
                INC_OUTPUTPOS(arg, 1); /* 8 bit per arg */
                break;
            case 's':
            case 'S':
            case 'n':
            case 'v':
                INC_OUTPUTPOS(arg, 2); /* 16 bit per arg */
                break;
            case 'i':
            case 'I':
                INC_OUTPUTPOS(arg, sizeof(int));
                break;
            case 'l':
            case 'L':
            case 'N':
            case 'V':
                INC_OUTPUTPOS(arg, 4); /* 32 bit per arg */
                break;
#if IS_64_BIT
            case 'q':
            case 'Q':
            case 'J':
            case 'P':
                INC_OUTPUTPOS(arg, 8); /* 32 bit per arg */
                break;
#endif
            case 'f': /* float */
            case 'g': /* little endian float */
            case 'G': /* big endian float */
                INC_OUTPUTPOS(arg, sizeof(float));
                break;
            case 'd': /* double */
            case 'e': /* little endian double */
            case 'E': /* big endian double */
                INC_OUTPUTPOS(arg, sizeof(double));
                break;
            case 'X':
                outputpos -= arg;
                if(outputpos < 0)
                {
                    // TODO: Give warning...
                    //          RETURN_ERROR("Type %c: outside of string", code);
                    outputpos = 0;
                }
                break;
            case '@':
                outputpos = arg;
                break;
        }
        if(outputsize < outputpos)
        {
            outputsize = outputpos;
        }
    }
    unsigned char* output = ALLOCATE(unsigned char, outputsize + 1);
    outputpos = 0;
    currentarg = 0;
    for(i = 0; i < formatcount; i++)
    {
        int code = (int)formatcodes[i];
        int arg = formatargs[i];
        switch((int)code)
        {
            case 'a':
            case 'A':
            case 'Z':
            {
                size_t argcp = (code != 'Z') ? arg : MAX(0, arg - 1);
                char* str = bl_value_tostring(vm, argslist[currentarg++]);
                memset(&output[outputpos], (code == 'a' || code == 'Z') ? '\0' : ' ', arg);
                memcpy(&output[outputpos], str, (strlen(str) < argcp) ? strlen(str) : argcp);
                outputpos += arg;
                break;
            }
            case 'h':
            case 'H':
            {
                int nibbleshift = (code == 'h') ? 0 : 4;
                int first = 1;
                char* str = bl_value_tostring(vm, argslist[currentarg++]);
                outputpos--;
                if((size_t)arg > strlen(str))
                {
                    // TODO: Give warning...
                    //          RETURN_ERROR("Type %c: not enough characters in string", code);
                    arg = (int)strlen(str);
                }
                while(arg-- > 0)
                {
                    char n = *str++;
                    if(n >= '0' && n <= '9')
                    {
                        n -= '0';
                    }
                    else if(n >= 'A' && n <= 'F')
                    {
                        n -= ('A' - 10);
                    }
                    else if(n >= 'a' && n <= 'f')
                    {
                        n -= ('a' - 10);
                    }
                    else
                    {
                        // TODO: Give warning...
                        //            RETURN_ERROR("Type %c: illegal hex digit %c", code, n);
                        n = 0;
                    }
                    if(first--)
                    {
                        output[++outputpos] = 0;
                    }
                    else
                    {
                        first = 1;
                    }
                    output[outputpos] |= (n << nibbleshift);
                    nibbleshift = (nibbleshift + 4) & 7;
                }
                outputpos++;
                break;
            }
            case 'c':
            case 'C':
                while(arg-- > 0)
                {
                    do_pack(vm, argslist[currentarg++], 1, bytemap, &output[outputpos]);
                    outputpos++;
                }
                break;
            case 's':
            case 'S':
            case 'n':
            case 'v':
            {
                int* map = machineendianshortmap;
                if(code == 'n')
                {
                    map = bigendianshortmap;
                }
                else if(code == 'v')
                {
                    map = littleendianshortmap;
                }
                while(arg-- > 0)
                {
                    do_pack(vm, argslist[currentarg++], 2, map, &output[outputpos]);
                    outputpos += 2;
                }
                break;
            }
            case 'i':
            case 'I':
                while(arg-- > 0)
                {
                    do_pack(vm, argslist[currentarg++], sizeof(int), intmap, &output[outputpos]);
                    outputpos += sizeof(int);
                }
                break;
            case 'l':
            case 'L':
            case 'N':
            case 'V':
            {
                int* map = machineendianlongmap;
                if(code == 'N')
                {
                    map = bigendianlongmap;
                }
                else if(code == 'V')
                {
                    map = littleendianlongmap;
                }
                while(arg-- > 0)
                {
                    do_pack(vm, argslist[currentarg++], 4, map, &output[outputpos]);
                    outputpos += 4;
                }
                break;
            }
            case 'q':
            case 'Q':
            case 'J':
            case 'P':
            {
#if IS_64_BIT
                int* map = machineendianlonglongmap;
                if(code == 'J')
                {
                    map = bigendianlonglongmap;
                }
                else if(code == 'P')
                {
                    map = littleendianlonglongmap;
                }
                while(arg-- > 0)
                {
                    do_pack(vm, argslist[currentarg++], 8, map, &output[outputpos]);
                    outputpos += 8;
                }
                break;
#else
                RETURN_ERROR("q, Q, J and P are only supported on 64-bit builds of Blade");
#endif
            }
            case 'f':
            {
                while(arg-- > 0)
                {
                    float v = (float)to_double(vm, argslist[currentarg++]);
                    memcpy(&output[outputpos], &v, sizeof(v));
                    outputpos += sizeof(v);
                }
                break;
            }
            case 'g':
            {
                /* pack little endian float */
                while(arg-- > 0)
                {
                    float v = (float)to_double(vm, argslist[currentarg++]);
                    bl_util_copyfloat(1, &output[outputpos], v);
                    outputpos += sizeof(v);
                }
                break;
            }
            case 'G':
            {
                /* pack big endian float */
                while(arg-- > 0)
                {
                    float v = (float)to_double(vm, argslist[currentarg++]);
                    bl_util_copyfloat(0, &output[outputpos], v);
                    outputpos += sizeof(v);
                }
                break;
            }
            case 'd':
            {
                while(arg-- > 0)
                {
                    double v = to_double(vm, argslist[currentarg++]);
                    memcpy(&output[outputpos], &v, sizeof(v));
                    outputpos += sizeof(v);
                }
                break;
            }
            case 'e':
            {
                /* pack little endian double */
                while(arg-- > 0)
                {
                    double v = to_double(vm, argslist[currentarg++]);
                    bl_util_copydouble(1, &output[outputpos], v);
                    outputpos += sizeof(v);
                }
                break;
            }
            case 'E':
            {
                /* pack big endian double */
                while(arg-- > 0)
                {
                    double v = to_double(vm, argslist[currentarg++]);
                    bl_util_copydouble(0, &output[outputpos], v);
                    outputpos += sizeof(v);
                }
                break;
            }
            case 'x':
                memset(&output[outputpos], '\0', arg);
                outputpos += arg;
                break;
            case 'X':
                outputpos -= arg;
                if(outputpos < 0)
                {
                    outputpos = 0;
                }
                break;
            case '@':
                if(arg > outputpos)
                {
                    memset(&output[outputpos], '\0', arg - outputpos);
                }
                outputpos = arg;
                break;
        }
    }
    free(formatcodes);
    free(formatargs);
    output[outputpos] = '\0';
    ObjBytes* bytes = (ObjBytes*)bl_mem_gcprotect(vm, (Object*)bl_bytes_takebytes(vm, output, outputpos));
    RETURN_OBJ(bytes);
}

bool modfn_struct_unpack(VMState* vm, int argcount, Value* args)
{
    int i;
    int size;
    int argb;
    int offset;
    int namelen;
    int repetitions;
    size_t formatlen;
    size_t inputpos;
    size_t inputlen;
    char type;
    char c;
    char* name;
    char* input;
    char* format;
    ObjDict* returnvalue;
    ObjString* string;
    ObjBytes* data;
    ENFORCE_ARG_COUNT(unpack, 3);
    ENFORCE_ARG_TYPE(unpack, 0, bl_value_isstring);
    ENFORCE_ARG_TYPE(unpack, 1, bl_value_isbytes);
    ENFORCE_ARG_TYPE(unpack, 2, bl_value_isnumber);
    string = AS_STRING(args[0]);
    data = AS_BYTES(args[1]);
    offset = AS_NUMBER(args[2]);
    format = string->chars;
    input = (char*)data->bytes.bytes;
    formatlen = string->length;
    inputpos = 0;
    inputlen = data->bytes.count;
    if((offset < 0) || (offset > (int)inputlen))
    {
        RETURN_ERROR("argument 3 (offset) must be within the range of argument 2 (data)");
    }
    input += offset;
    inputlen -= offset;
    returnvalue = (ObjDict*)bl_mem_gcprotect(vm, (Object*)bl_object_makedict(vm));
    while(formatlen-- > 0)
    {
        type = *(format++);
        repetitions = 1;
        size = 0;
        /* Handle format arguments if any */
        if(formatlen > 0)
        {
            c = *format;
            if(c >= '0' && c <= '9')
            {
                repetitions = (int)strtol(format, NULL, 10);
                while(formatlen > 0 && *format >= '0' && *format <= '9')
                {
                    format++;
                    formatlen--;
                }
            }
            else if(c == '*')
            {
                repetitions = -1;
                format++;
                formatlen--;
            }
        }
        /* Get of new value in array */
        name = format;
        argb = repetitions;
        while(formatlen > 0 && *format != '/')
        {
            formatlen--;
            format++;
        }
        namelen = format - name;
        if(namelen > 200)
        {
            namelen = 200;
        }
        switch((int)type)
        {
            case 'X':
            {
                /* Never use any input */
                size = -1;
                if(repetitions < 0)
                {
                    // TODO: Give warning...
                    //          RETURN_ERROR("Type %c: '*' ignored", type);
                    repetitions = 1;
                }
            }
            break;
            case '@':
            {
                size = 0;
            }
            break;
            case 'a':
            case 'A':
            case 'Z':
            {
                size = repetitions;
                repetitions = 1;
            }
            break;
            case 'h':
            case 'H':
            {
                size = repetitions;
                if(repetitions > 0)
                {
                    size = (repetitions + (repetitions % 2)) / 2;
                }
                repetitions = 1;
            }
            break;
            case 'c':
            case 'C':
            case 'x':
            {
                /* Use 1 byte of input */
                size = 1;
            }
            break;
            case 's':
            case 'S':
            case 'n':
            case 'v':
            {
                /* Use 2 bytes of input */
                size = 2;
            }
            break;
                /* Use sizeof(int) bytes of input */
            case 'i':
            case 'I':
            {
                size = sizeof(int);
            }
            break;
            case 'l':
            case 'L':
            case 'N':
            case 'V':
            {
                /* Use 4 bytes of input */
                size = 4;
            }
            break;
            case 'q':
            case 'Q':
            case 'J':
            case 'P':
            {
                /* Use 8 bytes of input */
#if IS_64_BIT
                size = 8;
                break;
#else
                RETURN_ERROR("64-bit format codes are not available for 32-bit Blade");
#endif
            }
            break;
                /* Use sizeof(float) bytes of input */
            case 'f':
            case 'g':
            case 'G':
            {
                size = sizeof(float);
            }
            break;
                /* Use sizeof(double) bytes of input */
            case 'd':
            case 'e':
            case 'E':
            {
                size = sizeof(double);
            }
            break;
            default:
            {
                RETURN_ERROR("Invalid format type %c", type);
            }
            break;
        }
        if(size != 0 && size != -1 && size < 0)
        {
            // TODO: Give warning...
            //      RETURN_ERROR("Type %c: integer overflow", type);
            RETURN_FALSE;
        }
        /* Do actual unpacking */
        for(i = 0; i != repetitions; i++)
        {
            if(((size != 0) && (size != -1)) && (((INT_MAX - size) + 1) < (int)inputpos))
            {
                // TODO: Give warning...
                //        RETURN_ERROR("Type %c: integer overflow", type);
                RETURN_FALSE;
            }
            if((inputpos + size) <= inputlen)
            {
                char* realname;
                if(repetitions == 1 && namelen > 0)
                {
                    /* Use a part of the formatarg argument directly as the name. */
                    realname = ALLOCATE(char, namelen);
                    memcpy(realname, name, namelen);
                    realname[namelen] = '\0';
                }
                else
                {
                    /* Need to add the 1-based element number to the name */
                    char buf[MAX_LENGTH_OF_LONG + 1];
                    char* res = bl_util_ulongtobuffer(buf + sizeof(buf) - 1, i + 1);
                    size_t digits = buf + sizeof(buf) - 1 - res;
                    realname = ALLOCATE(char, namelen + digits);
                    if(realname == NULL)
                    {
                        RETURN_ERROR("out of memory");
                    }
                    memcpy(realname, name, namelen);
                    memcpy(realname + namelen, res, digits);
                    realname[namelen + digits] = '\0';
                }
                switch((int)type)
                {
                    case 'a':
                    {
                        /* a will not strip any trailing whitespace or null padding */
                        size_t len = inputlen - inputpos; /* Remaining string */
                        /* If size was given take minimum of len and size */
                        if((size >= 0) && (len > (size_t)size))
                        {
                            len = size;
                        }
                        size = (int)len;
                        bl_dict_setentry(vm, returnvalue, UNPACK_REAL_NAME(), GC_L_STRING(&input[inputpos], len));
                    }
                    break;
                    case 'A':
                    {
                        /* A will strip any trailing whitespace */
                        char padn = '\0';
                        char pads = ' ';
                        char padt = '\t';
                        char padc = '\r';
                        char padl = '\n';
                        size_t len = inputlen - inputpos; /* Remaining string */
                        /* If size was given take minimum of len and size */
                        if(((int)size >= 0) && (len > (size_t)size))
                        {
                            len = size;
                        }
                        size = (int)len;
                        /* Remove trailing white space and nulls chars from unpacked data */
                        while((int)--len >= 0)
                        {
                            char inpc;
                            inpc = input[inputpos + len];
                            if(inpc != padn && inpc != pads && inpc != padt && inpc != padc && inpc != padl)
                            {
                                break;
                            }
                        }
                        bl_dict_setentry(vm, returnvalue, UNPACK_REAL_NAME(), GC_L_STRING(&input[inputpos], len + 1));
                        break;
                    }
                        /* New option added for Z to remain in-line with the Perl implementation */
                    case 'Z':
                    {
                        /* Z will strip everything after the first null character */
                        char pad = '\0';
                        size_t s;
                        size_t len = inputlen - inputpos; /* Remaining string */
                        /* If size was given take minimum of len and size */
                        if((size >= 0) && (len > (size_t)size))
                        {
                            len = size;
                        }
                        size = (int)len;
                        /* Remove everything after the first null */
                        for(s = 0; s < len; s++)
                        {
                            if(input[inputpos + s] == pad)
                            {
                                break;
                            }
                        }
                        len = s;
                        bl_dict_setentry(vm, returnvalue, UNPACK_REAL_NAME(), GC_L_STRING(&input[inputpos], len));
                        break;
                    }
                    case 'h':
                    case 'H':
                    {
                        size_t len = (inputlen - inputpos) * 2; /* Remaining */
                        int nibbleshift = (type == 'h') ? 0 : 4;
                        int first = 1;
                        size_t ipos;
                        size_t opos;
                        /* If size was given take minimum of len and size */
                        if((size >= 0) && (len > (size_t)(size * 2)))
                        {
                            len = size * 2;
                        }
                        if((len > 0) && (argb > 0))
                        {
                            len -= argb % 2;
                        }
                        char* buf = ALLOCATE(char, len);
                        for(ipos = opos = 0; opos < len; opos++)
                        {
                            char cc = (input[inputpos + ipos] >> nibbleshift) & 0xf;
                            if(cc < 10)
                            {
                                cc += '0';
                            }
                            else
                            {
                                cc += 'a' - 10;
                            }
                            buf[opos] = cc;
                            nibbleshift = (nibbleshift + 4) & 7;
                            if(first-- == 0)
                            {
                                ipos++;
                                first = 1;
                            }
                        }
                        buf[len] = '\0';
                        bl_dict_setentry(vm, returnvalue, UNPACK_REAL_NAME(), GC_L_STRING(buf, len));
                        break;
                    }
                    case 'c': /* signed */
                    case 'C':
                    { /* unsigned */
                        uint8_t x = input[inputpos];
                        long v = (type == 'c') ? (int8_t)x : x;
                        bl_dict_setentry(vm, returnvalue, UNPACK_REAL_NAME(), NUMBER_VAL(v));
                        break;
                    }
                    case 's': /* signed machine endian   */
                    case 'S': /* unsigned machine endian */
                    case 'n': /* unsigned big endian     */
                    case 'v':
                    { /* unsigned little endian  */
                        long v = 0;
                        uint16_t x = *((uint16_t*)&input[inputpos]);
                        if(type == 's')
                        {
                            v = (int16_t)x;
                        }
                        else if((type == 'n' && IS_LITTLE_ENDIAN) || (type == 'v' && !IS_LITTLE_ENDIAN))
                        {
                            v = bl_util_reverseint16(x);
                        }
                        else
                        {
                            v = x;
                        }
                        bl_dict_setentry(vm, returnvalue, UNPACK_REAL_NAME(), NUMBER_VAL(v));
                    }
                    break;
                    case 'i': /* signed integer, machine size, machine endian */
                    case 'I':
                    { /* unsigned integer, machine size, machine endian */
                        long v;
                        if(type == 'i')
                        {
                            int x = *((int*)&input[inputpos]);
                            v = x;
                        }
                        else
                        {
                            unsigned int x = *((unsigned int*)&input[inputpos]);
                            v = x;
                        }
                        bl_dict_setentry(vm, returnvalue, UNPACK_REAL_NAME(), NUMBER_VAL(v));
                    }
                    break;
                    case 'l': /* signed machine endian   */
                    case 'L': /* unsigned machine endian */
                    case 'N': /* unsigned big endian     */
                    case 'V':
                    { /* unsigned little endian  */
                        long v = 0;
                        uint32_t x = *((uint32_t*)&input[inputpos]);
                        if(type == 'l')
                        {
                            v = (int32_t)x;
                        }
                        else if((type == 'N' && IS_LITTLE_ENDIAN) || (type == 'V' && !IS_LITTLE_ENDIAN))
                        {
                            v = bl_util_reverseint32(x);
                        }
                        else
                        {
                            v = x;
                        }
                        bl_dict_setentry(vm, returnvalue, UNPACK_REAL_NAME(), NUMBER_VAL(v));
                    }
                    break;
                    case 'q': /* signed machine endian   */
                    case 'Q': /* unsigned machine endian */
                    case 'J': /* unsigned big endian     */
                    case 'P':
                    { /* unsigned little endian  */
#if IS_64_BIT
                        long v = 0;
                        uint64_t x = *((uint64_t*)&input[inputpos]);
                        if(type == 'q')
                        {
                            v = (int64_t)x;
                        }
                        else if((type == 'J' && IS_LITTLE_ENDIAN) || (type == 'P' && !IS_LITTLE_ENDIAN))
                        {
                            v = bl_util_reverseint64(x);
                        }
                        else
                        {
                            v = x;
                        }
                        bl_dict_setentry(vm, returnvalue, UNPACK_REAL_NAME(), NUMBER_VAL(v));
#else
                        RETURN_ERROR("q, Q, J and P are only valid on 64 bit build of Blade");
#endif
                    }
                    break;
                    case 'f': /* float */
                    case 'g': /* little endian float*/
                    case 'G': /* big endian float*/
                    {
                        float v;
                        if(type == 'g')
                        {
                            v = bl_util_parsefloat(1, &input[inputpos]);
                        }
                        else if(type == 'G')
                        {
                            v = bl_util_parsefloat(0, &input[inputpos]);
                        }
                        else
                        {
                            memcpy(&v, &input[inputpos], sizeof(float));
                        }
                        bl_dict_setentry(vm, returnvalue, UNPACK_REAL_NAME(), NUMBER_VAL(v));
                    }
                    break;
                    case 'd': /* double */
                    case 'e': /* little endian float */
                    case 'E': /* big endian float */
                    {
                        double v;
                        if(type == 'e')
                        {
                            v = bl_util_parsedouble(1, &input[inputpos]);
                        }
                        else if(type == 'E')
                        {
                            v = bl_util_parsedouble(0, &input[inputpos]);
                        }
                        else
                        {
                            memcpy(&v, &input[inputpos], sizeof(double));
                        }
                        bl_dict_setentry(vm, returnvalue, UNPACK_REAL_NAME(), NUMBER_VAL(v));
                    }
                    break;
                    case 'x':
                    {
                        /* Do nothing with input, just skip it */
                    }
                    break;
                    case 'X':
                    {
                        if((int)inputpos < size)
                        {
                            inputpos = -size;
                            i = repetitions - 1; /* Break out of for loop */
                            if(repetitions >= 0)
                            {
                                // TODO: Give warning...
                                //                RETURN_ERROR("Type %c: outside of string", type);
                            }
                        }
                    }
                    break;
                    case '@':
                        if(repetitions <= (int)inputlen)
                        {
                            inputpos = repetitions;
                        }
                        else
                        {
                            // TODO: Give warning...
                            // RETURN_ERROR("Type %c: outside of string", type);
                        }
                        i = repetitions - 1; /* Done, break out of for loop */
                        break;
                }
                inputpos += size;
                if((int)inputpos < 0)
                {
                    if(size != -1)
                    { /* only print warning if not working with * */
                        // TODO: Give warning...
                        // RETURN_ERROR("Type %c: outside of string", type);
                    }
                    inputpos = 0;
                }
            }
            else if(repetitions < 0)
            {
                /* Reached end of input for '*' repeater */
                break;
            }
            else
            {
                // TODO: Give warning...
                // RETURN_ERROR("Type %c: not enough input, need %d, have " LONG_FMT, type, size, inputlen - inputpos);
                RETURN_FALSE;
            }
        }
        if(formatlen > 0)
        {
            formatlen--; /* Skip '/' separator, does no harm if inputlen == 0 */
            format++;
        }
    }
    RETURN_OBJ(returnvalue);
}

void modfn_struct_modulepreloader(VMState* vm)
{
    int i;
    (void)vm;
#if IS_LITTLE_ENDIAN
    /* Where to get lo to hi bytes from */
    bytemap[0] = 0;
    for(i = 0; i < (int)sizeof(int); i++)
    {
        intmap[i] = i;
    }
    machineendianshortmap[0] = 0;
    machineendianshortmap[1] = 1;
    bigendianshortmap[0] = 1;
    bigendianshortmap[1] = 0;
    littleendianshortmap[0] = 0;
    littleendianshortmap[1] = 1;
    machineendianlongmap[0] = 0;
    machineendianlongmap[1] = 1;
    machineendianlongmap[2] = 2;
    machineendianlongmap[3] = 3;
    bigendianlongmap[0] = 3;
    bigendianlongmap[1] = 2;
    bigendianlongmap[2] = 1;
    bigendianlongmap[3] = 0;
    littleendianlongmap[0] = 0;
    littleendianlongmap[1] = 1;
    littleendianlongmap[2] = 2;
    littleendianlongmap[3] = 3;
    #if IS_64_BIT
    machineendianlonglongmap[0] = 0;
    machineendianlonglongmap[1] = 1;
    machineendianlonglongmap[2] = 2;
    machineendianlonglongmap[3] = 3;
    machineendianlonglongmap[4] = 4;
    machineendianlonglongmap[5] = 5;
    machineendianlonglongmap[6] = 6;
    machineendianlonglongmap[7] = 7;
    bigendianlonglongmap[0] = 7;
    bigendianlonglongmap[1] = 6;
    bigendianlonglongmap[2] = 5;
    bigendianlonglongmap[3] = 4;
    bigendianlonglongmap[4] = 3;
    bigendianlonglongmap[5] = 2;
    bigendianlonglongmap[6] = 1;
    bigendianlonglongmap[7] = 0;
    littleendianlonglongmap[0] = 0;
    littleendianlonglongmap[1] = 1;
    littleendianlonglongmap[2] = 2;
    littleendianlonglongmap[3] = 3;
    littleendianlonglongmap[4] = 4;
    littleendianlonglongmap[5] = 5;
    littleendianlonglongmap[6] = 6;
    littleendianlonglongmap[7] = 7;
    #endif
#else
    int size = sizeof(long);
    /* Where to get hi to lo bytes from */
    bytemap[0] = size - 1;
    for(i = 0; i < (int)sizeof(int); i++)
    {
        intmap[i] = size - (sizeof(int) - i);
    }
    machineendianshortmap[0] = size - 2;
    machineendianshortmap[1] = size - 1;
    bigendianshortmap[0] = size - 2;
    bigendianshortmap[1] = size - 1;
    littleendianshortmap[0] = size - 1;
    littleendianshortmap[1] = size - 2;
    machineendianlongmap[0] = size - 4;
    machineendianlongmap[1] = size - 3;
    machineendianlongmap[2] = size - 2;
    machineendianlongmap[3] = size - 1;
    bigendianlongmap[0] = size - 4;
    bigendianlongmap[1] = size - 3;
    bigendianlongmap[2] = size - 2;
    bigendianlongmap[3] = size - 1;
    littleendianlongmap[0] = size - 1;
    littleendianlongmap[1] = size - 2;
    littleendianlongmap[2] = size - 3;
    littleendianlongmap[3] = size - 4;
    #if ISIS_64_BIT
    machineendianlonglongmap[0] = size - 8;
    machineendianlonglongmap[1] = size - 7;
    machineendianlonglongmap[2] = size - 6;
    machineendianlonglongmap[3] = size - 5;
    machineendianlonglongmap[4] = size - 4;
    machineendianlonglongmap[5] = size - 3;
    machineendianlonglongmap[6] = size - 2;
    machineendianlonglongmap[7] = size - 1;
    bigendianlonglongmap[0] = size - 8;
    bigendianlonglongmap[1] = size - 7;
    bigendianlonglongmap[2] = size - 6;
    bigendianlonglongmap[3] = size - 5;
    bigendianlonglongmap[4] = size - 4;
    bigendianlonglongmap[5] = size - 3;
    bigendianlonglongmap[6] = size - 2;
    bigendianlonglongmap[7] = size - 1;
    littleendianlonglongmap[0] = size - 1;
    littleendianlonglongmap[1] = size - 2;
    littleendianlonglongmap[2] = size - 3;
    littleendianlonglongmap[3] = size - 4;
    littleendianlonglongmap[4] = size - 5;
    littleendianlonglongmap[5] = size - 6;
    littleendianlonglongmap[6] = size - 7;
    littleendianlonglongmap[7] = size - 8;
    #endif
#endif
}

RegModule* bl_modload_struct(VMState* vm)
{
    (void)vm;
    static RegFunc modulefunctions[] = {
        { "pack", true, modfn_struct_pack },
        { "unpack", true, modfn_struct_unpack },
        { NULL, false, NULL },
    };
    static RegModule module
    = { .name = "_struct", .fields = NULL, .functions = modulefunctions, .classes = NULL, .preloader = &modfn_struct_modulepreloader, .unloader = NULL };
    return &module;
}


bool bl_vm_propagateexception(VMState* vm, bool isassert)
{
    ObjInstance* exception = AS_INSTANCE(bl_vm_peekvalue(vm, 0));
    while(vm->framecount > 0)
    {
        CallFrame* frame = &vm->frames[vm->framecount - 1];
        for(int i = frame->handlerscount; i > 0; i--)
        {
            ExceptionFrame handler = frame->handlers[i - 1];
            ObjFunction* function = frame->closure->fnptr;
            if(handler.address != 0 && bl_class_isinstanceof(exception->klass, handler.klass->name->chars))
            {
                frame->ip = &function->blob.code[handler.address];
                return true;
            }
            else if(handler.finallyaddress != 0)
            {
                bl_vm_pushvalue(vm, TRUE_VAL);// continue propagating once the 'finally' block completes
                frame->ip = &function->blob.code[handler.finallyaddress];
                return true;
            }
        }
        vm->framecount--;
    }
    fflush(stdout);// flush out anything on stdout first
    Value message;
    Value trace;
    if(!isassert)
    {
        fprintf(stderr, "Unhandled %s", exception->klass->name->chars);
    }
    else
    {
        fprintf(stderr, "Illegal State");
    }
    if(bl_hashtable_get(&exception->properties, STRING_L_VAL("message", 7), &message))
    {
        char* errormessage = bl_value_tostring(vm, message);
        if(strlen(errormessage) > 0)
        {
            fprintf(stderr, ": %s", errormessage);
        }
        else
        {
            fprintf(stderr, ":");
        }
        fprintf(stderr, "\n");
    }
    else
    {
        fprintf(stderr, "\n");
    }
    if(bl_hashtable_get(&exception->properties, STRING_L_VAL("stacktrace", 10), &trace))
    {
        char* tracestr = bl_value_tostring(vm, trace);
        fprintf(stderr, "  StackTrace:\n%s\n", tracestr);
        free(tracestr);
    }
    return false;
}

bool bl_vm_pushexceptionhandler(VMState* vm, ObjClass* type, int address, int finallyaddress)
{
    CallFrame* frame = &vm->frames[vm->framecount - 1];
    if(frame->handlerscount == MAX_EXCEPTION_HANDLERS)
    {
        bl_vm_runtimeerror(vm, "too many nested exception handlers in one function");
        return false;
    }
    frame->handlers[frame->handlerscount].address = address;
    frame->handlers[frame->handlerscount].finallyaddress = finallyaddress;
    frame->handlers[frame->handlerscount].klass = type;
    frame->handlerscount++;
    return true;
}

bool bl_vm_throwexception(VMState* vm, bool isassert, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    char* message = NULL;
    int length = vasprintf(&message, format, args);
    va_end(args);
    ObjInstance* instance = bl_object_makeexception(vm, bl_string_takestring(vm, message, length));
    bl_vm_pushvalue(vm, OBJ_VAL(instance));
    Value stacktrace = bl_vm_getstacktrace(vm);
    bl_hashtable_set(vm, &instance->properties, STRING_L_VAL("stacktrace", 10), stacktrace);
    return bl_vm_propagateexception(vm, isassert);
}

void bl_vm_initexceptions(VMState* vm, ObjModule* module)
{
    size_t slen;
    const char* sstr;
    ObjString* classname;
    sstr = "Exception";
    slen = strlen(sstr);
    //classname = bl_string_copystringlen(vm, sstr, slen);
    classname = bl_string_fromallocated(vm, strdup(sstr), slen, bl_util_hashstring(sstr, slen));
    bl_vm_pushvalue(vm, OBJ_VAL(classname));
    ObjClass* klass = bl_object_makeclass(vm, classname, NULL);
    bl_vm_popvalue(vm);
    bl_vm_pushvalue(vm, OBJ_VAL(klass));
    ObjFunction* function = bl_object_makescriptfunction(vm, module, TYPE_METHOD);
    bl_vm_popvalue(vm);
    function->arity = 1;
    function->isvariadic = false;
    // gloc 0
    bl_blob_write(vm, &function->blob, OP_GET_LOCAL, 0);
    bl_blob_write(vm, &function->blob, (0 >> 8) & 0xff, 0);
    bl_blob_write(vm, &function->blob, 0 & 0xff, 0);
    // gloc 1
    bl_blob_write(vm, &function->blob, OP_GET_LOCAL, 0);
    bl_blob_write(vm, &function->blob, (1 >> 8) & 0xff, 0);
    bl_blob_write(vm, &function->blob, 1 & 0xff, 0);
    int messageconst = bl_blob_addconst(vm, &function->blob, STRING_L_VAL("message", 7));
    // sprop 1
    bl_blob_write(vm, &function->blob, OP_SET_PROPERTY, 0);
    bl_blob_write(vm, &function->blob, (messageconst >> 8) & 0xff, 0);
    bl_blob_write(vm, &function->blob, messageconst & 0xff, 0);
    // pop
    bl_blob_write(vm, &function->blob, OP_POP, 0);
    // gloc 0
    bl_blob_write(vm, &function->blob, OP_GET_LOCAL, 0);
    bl_blob_write(vm, &function->blob, (0 >> 8) & 0xff, 0);
    bl_blob_write(vm, &function->blob, 0 & 0xff, 0);
    // ret
    bl_blob_write(vm, &function->blob, OP_RETURN, 0);
    bl_vm_pushvalue(vm, OBJ_VAL(function));
    ObjClosure* closure = bl_object_makeclosure(vm, function);
    bl_vm_popvalue(vm);
    // set class constructor
    bl_vm_pushvalue(vm, OBJ_VAL(closure));
    bl_hashtable_set(vm, &klass->methods, OBJ_VAL(classname), OBJ_VAL(closure));
    klass->initializer = OBJ_VAL(closure);
    // set class properties
    bl_hashtable_set(vm, &klass->properties, STRING_L_VAL("message", 7), NIL_VAL);
    bl_hashtable_set(vm, &klass->properties, STRING_L_VAL("stacktrace", 10), NIL_VAL);
    bl_hashtable_set(vm, &vm->globals, OBJ_VAL(classname), OBJ_VAL(klass));
    bl_vm_popvalue(vm);
    bl_vm_popvalue(vm);// assert error name
    vm->exceptionclass = klass;
}




