

#include "blade.h"

void bl_state_defineglobal(VMState* vm, ObjString* name, Value val)
{
    bl_vm_pushvalue(vm, OBJ_VAL(name));
    bl_vm_pushvalue(vm, val);
    bl_hashtable_set(vm, &vm->globals, vm->stack[0], vm->stack[1]);
    bl_vm_popvaluen(vm, 2);
}

static void define_userglobal(VMState* vm, const char* name, Value val)
{
    ObjString* objn;
    objn = bl_string_copystringlen(vm, name, (int)strlen(name));
    bl_state_defineglobal(vm, objn, val);
}

static void define_usernative(VMState* vm, const char* name, NativeCallbackFunc function)
{
    define_userglobal(vm, name, OBJ_VAL(bl_object_makenativefunction(vm, function, name)));
}

static ObjString* bin_to_string(VMState* vm, long n)
{
    char str[1024];// assume maximum of 1024 bits
    int count = 0;
    long j = n;
    if(j == 0)
    {
        str[count++] = '0';
    }
    while(j != 0)
    {
        int rem = abs((int)(j % 2));
        j /= 2;
        str[count++] = rem == 1 ? '1' : '0';
    }
    char newstr[1027];// assume maximum of 1024 bits + 0b (indicator) + sign (-).
    int length = 0;
    if(n < 0)
    {
        newstr[length++] = '-';
    }
    newstr[length++] = '0';
    newstr[length++] = 'b';
    for(int i = count - 1; i >= 0; i--)
    {
        newstr[length++] = str[i];
    }
    newstr[length++] = 0;
    return bl_string_copystringlen(vm, newstr, length);
    //  // To store the binary number
    //  long long number = 0;
    //  int cnt = 0;
    //  while (n != 0) {
    //    long long rem = n % 2;
    //    long long c = (long long) pow(10, cnt);
    //    number += rem * c;
    //    n /= 2;
    //
    //    // Count used to store exponent value
    //    cnt++;
    //  }
    //
    //  char str[67]; // assume maximum of 64 bits + 2 binary indicators (0b)
    //  int length = sprintf(str, "0b%lld", number);
    //
    //  return bl_string_copystringlen(vm, str, length);
}


static ObjString* number_to_oct(VMState* vm, long long n, bool numeric)
{
    char str[66];// assume maximum of 64 bits + 2 octal indicators (0c)
    int length = sprintf(str, numeric ? "0c%llo" : "%llo", n);
    return bl_string_copystringlen(vm, str, length);
}

static ObjString* number_to_hex(VMState* vm, long long n, bool numeric)
{
    char str[66];// assume maximum of 64 bits + 2 hex indicators (0x)
    int length = sprintf(str, numeric ? "0x%llx" : "%llx", n);
    return bl_string_copystringlen(vm, str, length);
}


static bool cfn_bytes(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(bytes, 1);
    if(bl_value_isnumber(args[0]))
    {
        RETURN_OBJ(bl_object_makebytes(vm, (int)AS_NUMBER(args[0])));
    }
    else if(bl_value_isarray(args[0]))
    {
        ObjArray* list = AS_LIST(args[0]);
        ObjBytes* bytes = (ObjBytes*)bl_mem_gcprotect(vm, (Object*)bl_object_makebytes(vm, list->items.count));
        for(int i = 0; i < list->items.count; i++)
        {
            if(bl_value_isnumber(list->items.values[i]))
            {
                bytes->bytes.bytes[i] = (unsigned char)AS_NUMBER(list->items.values[i]);
            }
            else
            {
                bytes->bytes.bytes[i] = 0;
            }
        }
        RETURN_OBJ(bytes);
    }
    RETURN_ERROR("expected bytes size or bytes list as argument");
}

/**
 * time()
 *
 * returns the current timestamp in seconds
 */
static bool cfn_time(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(time, 0);
    struct timeval tv;
    gettimeofday(&tv, NULL);
    RETURN_NUMBER((double)tv.tv_sec + ((double)tv.tv_usec / 10000000));
}

/**
 * microtime()
 *
 * returns the current time in microseconds
 */
static bool cfn_microtime(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(microtime, 0);
    struct timeval tv;
    gettimeofday(&tv, NULL);
#ifndef _WIN32
    RETURN_NUMBER(1000000 * tv.tv_sec + tv.tv_usec);
#else
    RETURN_NUMBER((1000000 * (double)tv.tv_sec) + ((double)tv.tv_usec / 10));
#endif// !_WIN32
}

/**
 * id(value: any)
 *
 * returns the unique identifier of value within the system
 */
static bool cfn_id(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(id, 1);
#ifdef _WIN32
    RETURN_NUMBER(PtrToLong(&args[0]));
#else
    RETURN_NUMBER((long)&args[0]);
#endif
}

/**
 * hasprop(object: instance, name: string)
 *
 * returns true if object has the property name or not
 */
static bool cfn_hasprop(VMState* vm, int argcount, Value* args)
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
static bool cfn_getprop(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(getprop, 2);
    ENFORCE_ARG_TYPE(getprop, 0, bl_value_isinstance);
    ENFORCE_ARG_TYPE(getprop, 1, bl_value_isstring);
    ObjInstance* instance = AS_INSTANCE(args[0]);
    Value value;
    if(bl_hashtable_get(&instance->properties, args[1], &value) || bl_hashtable_get(&instance->klass->methods, args[1], &value))
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
static bool cfn_setprop(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(setprop, 3);
    ENFORCE_ARG_TYPE(setprop, 0, bl_value_isinstance);
    ENFORCE_ARG_TYPE(setprop, 1, bl_value_isstring);
    ObjInstance* instance = AS_INSTANCE(args[0]);
    RETURN_BOOL(bl_hashtable_set(vm, &instance->properties, args[1], args[2]));
}

/**
 * delprop(object: instance, name: string)
 *
 * deletes the named property from the object
 * @returns bool
 */
static bool cfn_delprop(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(delprop, 2);
    ENFORCE_ARG_TYPE(delprop, 0, bl_value_isinstance);
    ENFORCE_ARG_TYPE(delprop, 1, bl_value_isstring);
    ObjInstance* instance = AS_INSTANCE(args[0]);
    RETURN_BOOL(bl_hashtable_delete(&instance->properties, args[1]));
}

/**
 * max(number...)
 *
 * returns the greatest of the number arguments
 */
static bool cfn_max(VMState* vm, int argcount, Value* args)
{
    ENFORCE_MIN_ARG(max, 2);
    ENFORCE_ARG_TYPE(max, 0, bl_value_isnumber);
    double max = AS_NUMBER(args[0]);
    for(int i = 1; i < argcount; i++)
    {
        ENFORCE_ARG_TYPE(max, i, bl_value_isnumber);
        double number = AS_NUMBER(args[i]);
        if(number > max)
        {
            max = number;
        }
    }
    RETURN_NUMBER(max);
}

/**
 * min(number...)
 *
 * returns the least of the number arguments
 */
static bool cfn_min(VMState* vm, int argcount, Value* args)
{
    ENFORCE_MIN_ARG(min, 2);
    ENFORCE_ARG_TYPE(min, 0, bl_value_isnumber);
    double min = AS_NUMBER(args[0]);
    for(int i = 1; i < argcount; i++)
    {
        ENFORCE_ARG_TYPE(min, i, bl_value_isnumber);
        double number = AS_NUMBER(args[i]);
        if(number < min)
        {
            min = number;
        }
    }
    RETURN_NUMBER(min);
}

/**
 * sum(number...)
 *
 * returns the summation of all numbers given
 */
static bool cfn_sum(VMState* vm, int argcount, Value* args)
{
    ENFORCE_MIN_ARG(sum, 2);
    double sum = 0;
    for(int i = 0; i < argcount; i++)
    {
        ENFORCE_ARG_TYPE(sum, i, bl_value_isnumber);
        sum += AS_NUMBER(args[i]);
    }
    RETURN_NUMBER(sum);
}

/**
 * abs(x: number)
 *
 * returns the absolute value of a number.
 *
 * if x is not a number but it's class defines a method @to_abs(),
 * returns the result of calling x.to_abs()
 */
static bool cfn_abs(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(abs, 1);
    // handle classes that define a to_abs() method.
    METHOD_OVERRIDE(to_abs, 6);
    ENFORCE_ARG_TYPE(abs, 0, bl_value_isnumber);
    double value = AS_NUMBER(args[0]);
    if(value > -1)
        RETURN_VALUE(args[0]);
    RETURN_NUMBER(-value);
}

/**
 * int(i: number)
 *
 * returns the integer of a number or 0 if no number is given.
 *
 * if i is not a number but it's class defines @to_number(), it
 * returns the result of calling to_number()
 */
static bool cfn_int(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_RANGE(int, 0, 1);
    if(argcount == 0)
    {
        RETURN_NUMBER(0);
    }
    // handle classes that define a to_number() method.
    METHOD_OVERRIDE(to_number, 9);
    ENFORCE_ARG_TYPE(int, 0, bl_value_isnumber);
    RETURN_NUMBER((double)((int)AS_NUMBER(args[0])));
}

/**
 * bin(x: number)
 *
 * converts a number to it's binary string.
 *
 * if i is not a number but it's class defines @to_bin(), it
 * returns the result of calling bin(x.to_bin())
 */
static bool cfn_bin(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(bin, 1);
    // handle classes that define a to_bin() method.
    METHOD_OVERRIDE(to_bin, 6);
    ENFORCE_ARG_TYPE(bin, 0, bl_value_isnumber);
    RETURN_OBJ(bin_to_string(vm, AS_NUMBER(args[0])));
}

/**
 * oct(x: number)
 *
 * converts a number to it's octal string.
 *
 * if i is not a number but it's class defines @to_oct(), it
 * returns the result of calling oct(x.to_oct())
 */
static bool cfn_oct(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(oct, 1);
    // handle classes that define a to_oct() method.
    METHOD_OVERRIDE(to_oct, 6);
    ENFORCE_ARG_TYPE(oct, 0, bl_value_isnumber);
    RETURN_OBJ(number_to_oct(vm, AS_NUMBER(args[0]), false));
}

/**
 * hex(x: number)
 *
 * converts a number to it's hexadecimal string.
 *
 * if i is not a number but it's class defines @to_hex(), it
 * returns the result of calling hex(x.to_hex())
 */
static bool cfn_hex(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(hex, 1);
    // handle classes that define a to_hex() method.
    METHOD_OVERRIDE(to_hex, 6);
    ENFORCE_ARG_TYPE(hex, 0, bl_value_isnumber);
    RETURN_OBJ(number_to_hex(vm, AS_NUMBER(args[0]), false));
}

/**
 * to_bool(value: any)
 *
 * converts a value into a boolean.
 *
 * classes may override the return value by declaring a @to_bool()
 * function.
 */
static bool cfn_tobool(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(to_bool, 1);
    METHOD_OVERRIDE(to_bool, 7);
    RETURN_BOOL(!bl_value_isfalse(args[0]));
}

/**
 * to_string(value: any)
 *
 * convert a value into a string.
 *
 * native classes may override the return value by declaring a @to_string()
 * function.
 */
static bool cfn_tostring(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(to_string, 1);
    METHOD_OVERRIDE(to_string, 9);
    char* result = bl_value_tostring(vm, args[0]);
    RETURN_TT_STRING(result);
}

/**
 * to_number(value: any)
 *
 * convert a value into a number.
 *
 * native classes may override the return value by declaring a @to_number()
 * function.
 */
static bool cfn_tonumber(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(to_number, 1);
    METHOD_OVERRIDE(to_number, 9);
    if(bl_value_isnumber(args[0]))
    {
        RETURN_VALUE(args[0]);
    }
    else if(bl_value_isbool(args[0]))
    {
        RETURN_NUMBER(AS_BOOL(args[0]) ? 1 : 0);
    }
    else if(bl_value_isnil(args[0]))
    {
        RETURN_NUMBER(-1);
    }
    const char* v = (const char*)bl_value_tostring(vm, args[0]);
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
            RETURN_NUMBER(multiplier * strtoll(t, NULL, 2));
        }
        else if(v[end] == 'x')
        {
            RETURN_NUMBER(multiplier * strtol(t, NULL, 16));
        }
        else if(v[end] == 'c')
        {
            RETURN_NUMBER(multiplier * strtol(t, NULL, 8));
        }
    }
    RETURN_NUMBER(strtod(v, NULL));
}

/**
 * to_int(value: any)
 *
 * convert a value into an integer.
 *
 * native classes may override the return value by declaring a @to_int()
 * function.
 */
static bool cfn_toint(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(to_int, 1);
    METHOD_OVERRIDE(to_int, 6);
    ENFORCE_ARG_TYPE(to_int, 0, bl_value_isnumber);
    RETURN_NUMBER((int)AS_NUMBER(args[0]));
}

/**
 * to_list(value: any)
 *
 * convert a value into a list.
 *
 * native classes may override the return value by declaring a @to_list()
 * function.
 */
static bool cfn_tolist(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(to_list, 1);
    METHOD_OVERRIDE(to_list, 0);
    if(bl_value_isarray(args[0]))
    {
        RETURN_VALUE(args[0]);
    }
    ObjArray* list = (ObjArray*)bl_mem_gcprotect(vm, (Object*)bl_object_makelist(vm));
    if(bl_value_isdict(args[0]))
    {
        ObjDict* dict = AS_DICT(args[0]);
        for(int i = 0; i < dict->names.count; i++)
        {
            ObjArray* nlist = (ObjArray*)bl_mem_gcprotect(vm, (Object*)bl_object_makelist(vm));
            bl_valarray_push(vm, &nlist->items, dict->names.values[i]);
            Value value;
            bl_hashtable_get(&dict->items, dict->names.values[i], &value);
            bl_valarray_push(vm, &nlist->items, value);
            bl_valarray_push(vm, &list->items, OBJ_VAL(nlist));
        }
    }
    else if(bl_value_isstring(args[0]))
    {
        ObjString* str = AS_STRING(args[0]);
        for(int i = 0; i < str->utf8length; i++)
        {
            int start = i;
            int end = i + 1;
            bl_util_utf8slice(str->chars, &start, &end);
            bl_array_push(vm, list, STRING_L_VAL(str->chars + start, (int)(end - start)));
        }
    }
    else if(bl_value_isrange(args[0]))
    {
        ObjRange* range = AS_RANGE(args[0]);
        if(range->upper > range->lower)
        {
            for(int i = range->lower; i < range->upper; i++)
            {
                bl_array_push(vm, list, NUMBER_VAL(i));
            }
        }
        else
        {
            for(int i = range->lower; i > range->upper; i--)
            {
                bl_array_push(vm, list, NUMBER_VAL(i));
            }
        }
    }
    else
    {
        bl_valarray_push(vm, &list->items, args[0]);
    }
    RETURN_OBJ(list);
}

/**
 * to_dict(value: any)
 *
 * convert a value into a dictionary.
 *
 * native classes may override the return value by declaring a @to_dict()
 * function.
 */
static bool cfn_todict(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(to_dict, 1);
    METHOD_OVERRIDE(to_dict, 7);
    if(bl_value_isdict(args[0]))
    {
        RETURN_VALUE(args[0]);
    }
    ObjDict* dict = (ObjDict*)bl_mem_gcprotect(vm, (Object*)bl_object_makedict(vm));
    bl_dict_setentry(vm, dict, NUMBER_VAL(0), args[0]);
    RETURN_OBJ(dict);
}

/**
 * chr(i: number)
 *
 * return the string representing a character whose Unicode
 * code point is the number i.
 */
static bool cfn_chr(VMState* vm, int argcount, Value* args)
{
    char* string;
    ENFORCE_ARG_COUNT(chr, 1);
    ENFORCE_ARG_TYPE(chr, 0, bl_value_isnumber);
    string = bl_util_utf8encode((int)AS_NUMBER(args[0]));
    RETURN_T_STRING(string, strlen(string));
}

/**
 * ord(ch: char)
 *
 * return the code point value of a unicode character.
 */
static bool cfn_ord(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(ord, 1);
    ENFORCE_ARG_TYPE(ord, 0, bl_value_isstring);
    ObjString* string = AS_STRING(args[0]);
    int maxlength = string->length > 1 && (int)string->chars[0] < 1 ? 3 : 1;
    if(string->length > maxlength)
    {
        RETURN_ERROR("ord() expects character as argument, string given");
    }
    const uint8_t* bytes = (uint8_t*)string->chars;
    if((bytes[0] & 0xc0) == 0x80)
    {
        RETURN_NUMBER(-1);
    }
    // Decode the UTF-8 sequence.
    RETURN_NUMBER(bl_util_utf8decode((uint8_t*)string->chars, string->length));
}

/**
 * rand([limit: number, [upper: number]])
 *
 * - returns a random number between 0 and 1 if no argument is given
 * - returns a random number between 0 and limit if one argument is given
 * - returns a random number between limit and upper if two arguments is
 * given
 */
static bool cfn_rand(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_RANGE(rand, 0, 2);
    int lowerlimit = 0;
    int upperlimit = 1;
    if(argcount > 0)
    {
        ENFORCE_ARG_TYPE(rand, 0, bl_value_isnumber);
        lowerlimit = AS_NUMBER(args[0]);
    }
    if(argcount == 2)
    {
        ENFORCE_ARG_TYPE(rand, 1, bl_value_isnumber);
        upperlimit = AS_NUMBER(args[1]);
    }
    if(lowerlimit > upperlimit)
    {
        int tmp = upperlimit;
        upperlimit = lowerlimit;
        lowerlimit = tmp;
    }
    int n = upperlimit - lowerlimit + 1;
    int remainder = RAND_MAX % n;
    int x;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    srand((unsigned int)(10000000 * tv.tv_sec + tv.tv_usec + time(NULL)));
    if(lowerlimit == 0 && upperlimit == 1)
    {
        RETURN_NUMBER((double)rand() / RAND_MAX);
    }
    else
    {
        do
        {
            x = rand();
        } while(x >= RAND_MAX - remainder);
        RETURN_NUMBER((double)lowerlimit + x % n);
    }
}

/**
 * type(value: any)
 *
 * returns the name of the type of value
 */
static bool cfn_typeof(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(typeof, 1);
    char* result = (char*)bl_value_typename(args[0]);
    RETURN_L_STRING(result, strlen(result));
}

/**
 * is_callable(value: any)
 *
 * returns true if the value is a callable function or class and false otherwise
 */
static bool cfn_iscallable(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(is_callable, 1);
    RETURN_BOOL(bl_value_isclass(args[0]) || bl_value_isscriptfunction(args[0]) || bl_value_isclosure(args[0]) || bl_value_isboundfunction(args[0])
                || bl_value_isnativefunction(args[0]));
}

/**
 * is_bool(value: any)
 *
 * returns true if the value is a boolean or false otherwise
 */
static bool cfn_isbool(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(is_bool, 1);
    RETURN_BOOL(bl_value_isbool(args[0]));
}

/**
 * is_number(value: any)
 *
 * returns true if the value is a number or false otherwise
 */
static bool cfn_isnumber(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(is_number, 1);
    RETURN_BOOL(bl_value_isnumber(args[0]));
}

/**
 * is_int(value: any)
 *
 * returns true if the value is an integer or false otherwise
 */
static bool cfn_isint(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(is_int, 1);
    RETURN_BOOL(bl_value_isnumber(args[0]) && (((int)AS_NUMBER(args[0])) == AS_NUMBER(args[0])));
}

/**
 * is_string(value: any)
 *
 * returns true if the value is a string or false otherwise
 */
static bool cfn_isstring(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(is_string, 1);
    RETURN_BOOL(bl_value_isstring(args[0]));
}

/**
 * is_bytes(value: any)
 *
 * returns true if the value is a bytes or false otherwise
 */
static bool cfn_isbytes(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(is_bytes, 1);
    RETURN_BOOL(bl_value_isbytes(args[0]));
}

/**
 * is_list(value: any)
 *
 * returns true if the value is a list or false otherwise
 */
static bool cfn_islist(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(is_list, 1);
    RETURN_BOOL(bl_value_isarray(args[0]));
}

/**
 * is_dict(value: any)
 *
 * returns true if the value is a dictionary or false otherwise
 */
static bool cfn_isdict(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(is_dict, 1);
    RETURN_BOOL(bl_value_isdict(args[0]));
}

/**
 * is_object(value: any)
 *
 * returns true if the value is an object or false otherwise
 */
static bool cfn_isobject(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(is_object, 1);
    RETURN_BOOL(bl_value_isobject(args[0]));
}

/**
 * is_function(value: any)
 *
 * returns true if the value is a function or false otherwise
 */
static bool cfn_isfunction(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(is_function, 1);
    RETURN_BOOL(bl_value_isscriptfunction(args[0]) || bl_value_isclosure(args[0]) || bl_value_isboundfunction(args[0]) || bl_value_isnativefunction(args[0]));
}

/**
 * is_iterable(value: any)
 *
 * returns true if the value is an iterable or false otherwise
 */
static bool cfn_isiterable(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(is_iterable, 1);
    bool is_iterable = bl_value_isarray(args[0]) || bl_value_isdict(args[0]) || bl_value_isstring(args[0]) || bl_value_isbytes(args[0]);
    if(!is_iterable && bl_value_isinstance(args[0]))
    {
        ObjClass* klass = AS_INSTANCE(args[0])->klass;
        Value dummy;
        is_iterable = bl_hashtable_get(&klass->methods, STRING_VAL("@iter"), &dummy) && bl_hashtable_get(&klass->methods, STRING_VAL("@itern"), &dummy);
    }
    RETURN_BOOL(is_iterable);
}

/**
 * is_class(value: any)
 *
 * returns true if the value is a class or false otherwise
 */
static bool cfn_isclass(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(is_class, 1);
    RETURN_BOOL(bl_value_isclass(args[0]));
}

/**
 * is_file(value: any)
 *
 * returns true if the value is a file or false otherwise
 */
static bool cfn_isfile(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(is_file, 1);
    RETURN_BOOL(bl_value_isfile(args[0]));
}

/**
 * is_instance(value: any)
 *
 * returns true if the value is an instance of a class
 */
static bool cfn_isinstance(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(is_instance, 1);
    RETURN_BOOL(bl_value_isinstance(args[0]));
}

/**
 * instance_of(value: any, name: class)
 *
 * returns true if the value is an instance the given class, false
 * otherwise
 */
static bool cfn_instanceof(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(instance_of, 2);
    ENFORCE_ARG_TYPE(instance_of, 0, bl_value_isinstance);
    ENFORCE_ARG_TYPE(instance_of, 1, bl_value_isclass);
    RETURN_BOOL(bl_class_isinstanceof(AS_INSTANCE(args[0])->klass, AS_CLASS(args[1])->name->chars));
}

//------------------------------------------------------------------------------
/**
 * print(...)
 *
 * prints values to the standard output
 */
bool bl_util_wrapprintfunc(VMState* vm, int argcount, Value* args, bool doreturn)
{
    for(int i = 0; i < argcount; i++)
    {
        bl_value_printvalue(args[i]);
        if(i != argcount - 1)
        {
            printf(" ");
        }
    }
    if(vm->isrepl)
    {
        printf("\n");
    }
    if(doreturn)
    {
        RETURN_NUMBER(0);
    }
    return true;
}

static bool cfn_print(VMState* vm, int argcount, Value* args)
{
    return bl_util_wrapprintfunc(vm, argcount, args, true);
}

static bool cfn_println(VMState* vm, int argcount, Value* args)
{
    bl_util_wrapprintfunc(vm, argcount, args, false);
    printf("\n");
    RETURN_NUMBER(0);
}

void bl_state_initbuiltinfunctions(VMState* vm)
{
    define_usernative(vm, "abs", cfn_abs);
    define_usernative(vm, "bin", cfn_bin);
    define_usernative(vm, "bytes", cfn_bytes);
    define_usernative(vm, "chr", cfn_chr);
    define_usernative(vm, "delprop", cfn_delprop);
    define_usernative(vm, "file", cfn_file);
    define_usernative(vm, "getprop", cfn_getprop);
    define_usernative(vm, "hasprop", cfn_hasprop);
    define_usernative(vm, "hex", cfn_hex);
    define_usernative(vm, "id", cfn_id);
    define_usernative(vm, "int", cfn_int);
    define_usernative(vm, "is_bool", cfn_isbool);
    define_usernative(vm, "is_callable", cfn_iscallable);
    define_usernative(vm, "is_class", cfn_isclass);
    define_usernative(vm, "is_dict", cfn_isdict);
    define_usernative(vm, "is_function", cfn_isfunction);
    define_usernative(vm, "is_instance", cfn_isinstance);
    define_usernative(vm, "is_int", cfn_isint);
    define_usernative(vm, "is_list", cfn_islist);
    define_usernative(vm, "is_number", cfn_isnumber);
    define_usernative(vm, "is_object", cfn_isobject);
    define_usernative(vm, "is_string", cfn_isstring);
    define_usernative(vm, "is_bytes", cfn_isbytes);
    define_usernative(vm, "is_file", cfn_isfile);
    define_usernative(vm, "is_iterable", cfn_isiterable);
    define_usernative(vm, "instance_of", cfn_instanceof);
    define_usernative(vm, "max", cfn_max);
    define_usernative(vm, "microtime", cfn_microtime);
    define_usernative(vm, "min", cfn_min);
    define_usernative(vm, "oct", cfn_oct);
    define_usernative(vm, "ord", cfn_ord);
    define_usernative(vm, "print", cfn_print);
    define_usernative(vm, "println", cfn_println);
    define_usernative(vm, "rand", cfn_rand);
    define_usernative(vm, "setprop", cfn_setprop);
    define_usernative(vm, "sum", cfn_sum);
    define_usernative(vm, "time", cfn_time);
    define_usernative(vm, "to_bool", cfn_tobool);
    define_usernative(vm, "to_dict", cfn_todict);
    define_usernative(vm, "to_int", cfn_toint);
    define_usernative(vm, "to_list", cfn_tolist);
    define_usernative(vm, "to_number", cfn_tonumber);
    define_usernative(vm, "to_string", cfn_tostring);
    define_usernative(vm, "typeof", cfn_typeof);
}

void bl_state_initbuiltinmethods(VMState* vm)
{
    bl_state_initstringmethods(vm);
    bl_state_initdictmethods(vm);
    bl_state_initarraymethods(vm);
    bl_state_initfilemethods(vm);
    bl_state_initbytesmethods(vm);
    bl_state_initrangemethods(vm);
}


