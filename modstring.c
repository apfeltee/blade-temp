
#include "blade.h"

ObjString* bl_string_fromallocated(VMState* vm, char* chars, int length, uint32_t hash)
{
    //fprintf(stderr, "call to bl_string_fromallocated! chars=\"%.*s\"\n", length, chars);
    ObjString* string = (ObjString*)bl_object_allocobject(vm, sizeof(ObjString), OBJ_STRING);
    string->chars = chars;
    string->length = length;
    string->utf8length = bl_util_utf8length(chars);
    string->isascii = false;
    string->hash = hash;
    bl_vm_pushvalue(vm, OBJ_VAL(string));// fixing gc corruption
    bl_hashtable_set(vm, &vm->strings, OBJ_VAL(string), NIL_VAL);
    bl_vm_popvalue(vm);// fixing gc corruption
    return string;
}

ObjString* bl_string_takestring(VMState* vm, char* chars, int length)
{
    uint32_t hash = bl_util_hashstring(chars, length);
    ObjString* interned = bl_hashtable_findstring(&vm->strings, chars, length, hash);
    if(interned != NULL)
    {
        FREE_ARRAY(char, chars, (size_t)length + 1);
        return interned;
    }
    return bl_string_fromallocated(vm, chars, length, hash);
}

ObjString* bl_string_copystringlen(VMState* vm, const char* chars, int length)
{
    uint32_t hash = bl_util_hashstring(chars, length);
    ObjString* interned = bl_hashtable_findstring(&vm->strings, chars, length, hash);
    if(interned != NULL)
    {
        return interned;
    }
    char* heapchars = ALLOCATE(char, (size_t)length + 1);
    memcpy(heapchars, chars, length);
    heapchars[length] = '\0';
    return bl_string_fromallocated(vm, heapchars, length, hash);
}

ObjString* bl_string_copystring(VMState* vm, const char* chars)
{
    return bl_string_copystringlen(vm, chars, strlen(chars));
}


static bool objfn_string_length(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(length, 0);
    ObjString* string = AS_STRING(METHOD_OBJECT);
    RETURN_NUMBER(string->isascii ? string->length : string->utf8length);
}

static bool objfn_string_upper(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(upper, 0);
    char* string = (char*)strdup(AS_C_STRING(METHOD_OBJECT));
    for(char* p = string; *p; p++)
    {
        *p = toupper(*p);
    }
    RETURN_L_STRING(string, AS_STRING(METHOD_OBJECT)->length);
}

static bool objfn_string_lower(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(lower, 0);
    char* string = (char*)strdup(AS_C_STRING(METHOD_OBJECT));
    for(char* p = string; *p; p++)
    {
        *p = tolower(*p);
    }
    RETURN_L_STRING(string, AS_STRING(METHOD_OBJECT)->length);
}

static bool objfn_string_isalpha(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(bl_scanutil_isalpha, 0);
    ObjString* string = AS_STRING(METHOD_OBJECT);
    for(int i = 0; i < string->length; i++)
    {
        if(!isalpha((unsigned char)string->chars[i]))
        {
            RETURN_FALSE;
        }
    }
    RETURN_BOOL(string->length != 0);
}

static bool objfn_string_isalnum(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(isalnum, 0);
    ObjString* string = AS_STRING(METHOD_OBJECT);
    for(int i = 0; i < string->length; i++)
    {
        if(!isalnum((unsigned char)string->chars[i]))
        {
            RETURN_FALSE;
        }
    }
    RETURN_BOOL(string->length != 0);
}

static bool objfn_string_isnumber(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(is_number, 0);
    ObjString* string = AS_STRING(METHOD_OBJECT);
    for(int i = 0; i < string->length; i++)
    {
        if(!isdigit((unsigned char)string->chars[i]))
        {
            RETURN_FALSE;
        }
    }
    RETURN_BOOL(string->length != 0);
}

static bool objfn_string_islower(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(islower, 0);
    ObjString* string = AS_STRING(METHOD_OBJECT);
    bool hasalpha;
    for(int i = 0; i < string->length; i++)
    {
        bool isal = isalpha((unsigned char)string->chars[i]);
        if(!hasalpha)
        {
            hasalpha = isal;
        }
        if(isal && !islower((unsigned char)string->chars[i]))
        {
            RETURN_FALSE;
        }
    }
    RETURN_BOOL(string->length != 0 && hasalpha);
}

static bool objfn_string_isupper(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(isupper, 0);
    ObjString* string = AS_STRING(METHOD_OBJECT);
    bool hasalpha;
    for(int i = 0; i < string->length; i++)
    {
        bool isal = isalpha((unsigned char)string->chars[i]);
        if(!hasalpha)
        {
            hasalpha = isal;
        }
        if(isal && !isupper((unsigned char)string->chars[i]))
        {
            RETURN_FALSE;
        }
    }
    RETURN_BOOL(string->length != 0 && hasalpha);
}

static bool objfn_string_isspace(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(isspace, 0);
    ObjString* string = AS_STRING(METHOD_OBJECT);
    for(int i = 0; i < string->length; i++)
    {
        if(!isspace((unsigned char)string->chars[i]))
        {
            RETURN_FALSE;
        }
    }
    RETURN_BOOL(string->length != 0);
}

static bool objfn_string_trim(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_RANGE(trim, 0, 1);
    char trimmer = '\0';
    if(argcount == 1)
    {
        ENFORCE_ARG_TYPE(trim, 0, IS_CHAR);
        trimmer = (char)AS_STRING(args[0])->chars[0];
    }
    char* string = AS_C_STRING(METHOD_OBJECT);
    char* end = NULL;
    // Trim leading space
    if(trimmer == '\0')
    {
        while(isspace((unsigned char)*string))
        {
            string++;
        }
    }
    else
    {
        while(trimmer == *string)
        {
            string++;
        }
    }
    if(*string == 0)
    {// All spaces?
        RETURN_OBJ(bl_string_copystringlen(vm, "", 0));
    }
    // Trim trailing space
    end = string + strlen(string) - 1;
    if(trimmer == '\0')
    {
        while(end > string && isspace((unsigned char)*end))
        {
            end--;
        }
    }
    else
    {
        while(end > string && trimmer == *end)
        {
            end--;
        }
    }
    // Write new null terminator character
    end[1] = '\0';
    RETURN_L_STRING(string, strlen(string));
}

static bool objfn_string_ltrim(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_RANGE(ltrim, 0, 1);
    char trimmer = '\0';
    if(argcount == 1)
    {
        ENFORCE_ARG_TYPE(ltrim, 0, IS_CHAR);
        trimmer = (char)AS_STRING(args[0])->chars[0];
    }
    char* string = AS_C_STRING(METHOD_OBJECT);
    char* end = NULL;
    // Trim leading space
    if(trimmer == '\0')
    {
        while(isspace((unsigned char)*string))
        {
            string++;
        }
    }
    else
    {
        while(trimmer == *string)
        {
            string++;
        }
    }
    if(*string == 0)
    {// All spaces?
        RETURN_OBJ(bl_string_copystringlen(vm, "", 0));
    }
    end = string + strlen(string) - 1;
    // Write new null terminator character
    end[1] = '\0';
    RETURN_L_STRING(string, strlen(string));
}

static bool objfn_string_rtrim(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_RANGE(rtrim, 0, 1);
    char trimmer = '\0';
    if(argcount == 1)
    {
        ENFORCE_ARG_TYPE(rtrim, 0, IS_CHAR);
        trimmer = (char)AS_STRING(args[0])->chars[0];
    }
    char* string = AS_C_STRING(METHOD_OBJECT);
    char* end = NULL;
    if(*string == 0)
    {// All spaces?
        RETURN_OBJ(bl_string_copystringlen(vm, "", 0));
    }
    end = string + strlen(string) - 1;
    if(trimmer == '\0')
    {
        while(end > string && isspace((unsigned char)*end))
        {
            end--;
        }
    }
    else
    {
        while(end > string && trimmer == *end)
        {
            end--;
        }
    }
    // Write new null terminator character
    end[1] = '\0';
    RETURN_L_STRING(string, strlen(string));
}

static bool objfn_string_join(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(join, 1);
    ENFORCE_ARG_TYPE(join, 0, bl_value_isobject);
    ObjString* methodobj = AS_STRING(METHOD_OBJECT);
    Value argument = args[0];
    if(bl_value_isstring(argument))
    {
        // empty argument
        if(methodobj->length == 0)
        {
            RETURN_VALUE(argument);
        }
        else if(AS_STRING(argument)->length == 0)
        {
            RETURN_VALUE(argument);
        }
        ObjString* string = AS_STRING(argument);
        char* result = ALLOCATE(char, 2);
        result[0] = string->chars[0];
        result[1] = '\0';
        for(int i = 1; i < string->length; i++)
        {
            if(methodobj->length > 0)
            {
                result = bl_util_appendstring(result, methodobj->chars);
            }
            char* chr = (char*)calloc(2, sizeof(char));
            chr[0] = string->chars[i];
            chr[1] = '\0';
            result = bl_util_appendstring(result, chr);
            free(chr);
        }
        RETURN_TT_STRING(result);
    }
    else if(bl_value_isarray(argument) || bl_value_isdict(argument))
    {
        Value* list;
        int count = 0;
        if(bl_value_isdict(argument))
        {
            list = AS_DICT(argument)->names.values;
            count = AS_DICT(argument)->names.count;
        }
        else
        {
            list = AS_LIST(argument)->items.values;
            count = AS_LIST(argument)->items.count;
        }
        if(count == 0)
        {
            RETURN_L_STRING("", 0);
        }
        char* result = bl_value_tostring(vm, list[0]);
        for(int i = 1; i < count; i++)
        {
            if(methodobj->length > 0)
            {
                result = bl_util_appendstring(result, methodobj->chars);
            }
            char* str = bl_value_tostring(vm, list[i]);
            result = bl_util_appendstring(result, str);
            free(str);
        }
        RETURN_TT_STRING(result);
    }
    RETURN_ERROR("join() does not support object of type %s", bl_value_typename(argument));
}

static bool objfn_string_split(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(split, 1);
    ENFORCE_ARG_TYPE(split, 0, bl_value_isstring);
    ObjString* object = AS_STRING(METHOD_OBJECT);
    ObjString* delimeter = AS_STRING(args[0]);
    if(object->length == 0 || delimeter->length > object->length)
        RETURN_OBJ(bl_object_makelist(vm));
    ObjArray* list = (ObjArray*)bl_mem_gcprotect(vm, (Object*)bl_object_makelist(vm));
    // main work here...
    if(delimeter->length > 0)
    {
        int start = 0;
        for(int i = 0; i <= object->length; i++)
        {
            // match found.
            if(memcmp(object->chars + i, delimeter->chars, delimeter->length) == 0 || i == object->length)
            {
                bl_array_push(vm, list, GC_L_STRING(object->chars + start, i - start));
                i += delimeter->length - 1;
                start = i + 1;
            }
        }
    }
    else
    {
        int length = object->isascii ? object->length : object->utf8length;
        for(int i = 0; i < length; i++)
        {
            int start = i;
            int end = i + 1;
            if(!object->isascii)
            {
                bl_util_utf8slice(object->chars, &start, &end);
            }
            bl_array_push(vm, list, GC_L_STRING(object->chars + start, (int)(end - start)));
        }
    }
    RETURN_OBJ(list);
}

static bool objfn_string_indexof(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(indexof, 1);
    ENFORCE_ARG_TYPE(indexof, 0, bl_value_isstring);
    char* str = AS_C_STRING(METHOD_OBJECT);
    char* result = strstr(str, AS_C_STRING(args[0]));
    if(result != NULL)
        RETURN_NUMBER((int)(result - str));
    RETURN_NUMBER(-1);
}

static bool objfn_string_startswith(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(startswith, 1);
    ENFORCE_ARG_TYPE(startswith, 0, bl_value_isstring);
    ObjString* string = AS_STRING(METHOD_OBJECT);
    ObjString* substr = AS_STRING(args[0]);
    if(string->length == 0 || substr->length == 0 || substr->length > string->length)
        RETURN_FALSE;
    RETURN_BOOL(memcmp(substr->chars, string->chars, substr->length) == 0);
}

static bool objfn_string_endswith(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(endswith, 1);
    ENFORCE_ARG_TYPE(endswith, 0, bl_value_isstring);
    ObjString* string = AS_STRING(METHOD_OBJECT);
    ObjString* substr = AS_STRING(args[0]);
    if(string->length == 0 || substr->length == 0 || substr->length > string->length)
        RETURN_FALSE;
    int difference = string->length - substr->length;
    RETURN_BOOL(memcmp(substr->chars, string->chars + difference, substr->length) == 0);
}

static bool objfn_string_count(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(count, 1);
    ENFORCE_ARG_TYPE(count, 0, bl_value_isstring);
    ObjString* string = AS_STRING(METHOD_OBJECT);
    ObjString* substr = AS_STRING(args[0]);
    if(substr->length == 0 || string->length == 0)
        RETURN_NUMBER(0);
    int count = 0;
    const char* tmp = string->chars;
    while((tmp = strstr(tmp, substr->chars)))
    {
        count++;
        tmp++;
    }
    RETURN_NUMBER(count);
}

static bool objfn_string_tonumber(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(to_number, 0);
    RETURN_NUMBER(strtod(AS_C_STRING(METHOD_OBJECT), NULL));
}

static bool objfn_string_ascii(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(ascii, 0);
    ObjString* string = AS_STRING(METHOD_OBJECT);
    string->isascii = true;
    RETURN_OBJ(string);
}

static bool objfn_string_tolist(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(to_list, 0);
    ObjString* string = AS_STRING(METHOD_OBJECT);
    ObjArray* list = (ObjArray*)bl_mem_gcprotect(vm, (Object*)bl_object_makelist(vm));
    int length = string->isascii ? string->length : string->utf8length;
    if(length > 0)
    {
        for(int i = 0; i < length; i++)
        {
            int start = i;
            int end = i + 1;
            if(!string->isascii)
            {
                bl_util_utf8slice(string->chars, &start, &end);
            }
            bl_array_push(vm, list, GC_L_STRING(string->chars + start, (int)(end - start)));
        }
    }
    RETURN_OBJ(list);
}

static bool objfn_string_lpad(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_RANGE(lpad, 1, 2);
    ENFORCE_ARG_TYPE(lpad, 0, bl_value_isnumber);
    ObjString* string = AS_STRING(METHOD_OBJECT);
    int width = AS_NUMBER(args[0]);
    char fillchar = ' ';
    if(argcount == 2)
    {
        ENFORCE_ARG_TYPE(lpad, 1, IS_CHAR);
        fillchar = AS_C_STRING(args[1])[0];
    }
    if(width <= string->utf8length)
        RETURN_VALUE(METHOD_OBJECT);
    int fillsize = width - string->utf8length;
    char* fill = ALLOCATE(char, (size_t)fillsize + 1);
    int finalsize = string->length + fillsize;
    int finalutf8size = string->utf8length + fillsize;
    for(int i = 0; i < fillsize; i++)
    {
        fill[i] = fillchar;
    }
    char* str = ALLOCATE(char, (size_t)string->length + (size_t)fillsize + 1);
    memcpy(str, fill, fillsize);
    memcpy(str + fillsize, string->chars, string->length);
    str[finalsize] = '\0';
    ObjString* result = bl_string_takestring(vm, str, finalsize);
    result->utf8length = finalutf8size;
    result->length = finalsize;
    RETURN_OBJ(result);
}

static bool objfn_string_rpad(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_RANGE(rpad, 1, 2);
    ENFORCE_ARG_TYPE(rpad, 0, bl_value_isnumber);
    ObjString* string = AS_STRING(METHOD_OBJECT);
    int width = AS_NUMBER(args[0]);
    char fillchar = ' ';
    if(argcount == 2)
    {
        ENFORCE_ARG_TYPE(rpad, 1, IS_CHAR);
        fillchar = AS_C_STRING(args[1])[0];
    }
    if(width <= string->utf8length)
        RETURN_VALUE(METHOD_OBJECT);
    int fillsize = width - string->utf8length;
    char* fill = ALLOCATE(char, (size_t)fillsize + 1);
    int finalsize = string->length + fillsize;
    int finalutf8size = string->utf8length + fillsize;
    for(int i = 0; i < fillsize; i++)
    {
        fill[i] = fillchar;
    }
    char* str = ALLOCATE(char, (size_t)string->length + (size_t)fillsize + 1);
    memcpy(str, string->chars, string->length);
    memcpy(str + string->length, fill, fillsize);
    str[finalsize] = '\0';
    ObjString* result = bl_string_takestring(vm, str, finalsize);
    result->utf8length = finalutf8size;
    result->length = finalsize;
    RETURN_OBJ(result);
}

static bool objfn_string_match(VMState* vm, int argcount, Value* args)
{
    #if 0
    ENFORCE_ARG_COUNT(match, 1);
    ENFORCE_ARG_TYPE(match, 0, bl_value_isstring);
    ObjString* string = AS_STRING(METHOD_OBJECT);
    ObjString* substr = AS_STRING(args[0]);
    if(string->length == 0 && substr->length == 0)
    {
        RETURN_TRUE;
    }
    else if(string->length == 0 || substr->length == 0)
    {
        RETURN_FALSE;
    }
    GET_REGEX_COMPILE_OPTIONS(substr, false);
    if((int)compileoptions < 0)
    {
        RETURN_BOOL(strstr(string->chars, substr->chars) - string->chars > -1);
    }
    char* realregex = bl_helper_objstringremregexdelim(vm, substr);
    int errornumber;
    PCRE2_SIZE erroroffset;
    PCRE2_SPTR pattern = (PCRE2_SPTR)realregex;
    PCRE2_SPTR subject = (PCRE2_SPTR)string->chars;
    PCRE2_SIZE subjectlength = (PCRE2_SIZE)string->length;
    pcre2_code* re = pcre2_compile(pattern, PCRE2_ZERO_TERMINATED, compileoptions, &errornumber, &erroroffset, NULL);
    free(realregex);
    if(!re)
    {
        REGEX_COMPILATION_ERROR(re, errornumber, erroroffset);
    }
    pcre2_match_data* matchdata = pcre2_match_data_create_from_pattern(re, NULL);
    int rc = pcre2_match(re, subject, subjectlength, 0, 0, matchdata, NULL);
    if(rc < 0)
    {
        if(rc == PCRE2_ERROR_NOMATCH)
        {
            RETURN_FALSE;
        }
        else
        {
            REGEX_RC_ERROR();
        }
    }
    PCRE2_SIZE* ovector = pcre2_get_ovector_pointer(matchdata);
    uint32_t namecount;
    ObjDict* result = (ObjDict*)bl_mem_gcprotect(vm, (Object*)bl_object_makedict(vm));
    (void)pcre2_pattern_info(re, PCRE2_INFO_NAMECOUNT, &namecount);
    for(int i = 0; i < rc; i++)
    {
        PCRE2_SIZE substringlength = ovector[2 * i + 1] - ovector[2 * i];
        PCRE2_SPTR substringstart = subject + ovector[2 * i];
        bl_dict_setentry(vm, result, NUMBER_VAL(i), GC_L_STRING((char*)substringstart, (int)substringlength));
    }
    if(namecount > 0)
    {
        uint32_t nameentrysize;
        PCRE2_SPTR nametable;
        PCRE2_SPTR tabptr;
        (void)pcre2_pattern_info(re, PCRE2_INFO_NAMETABLE, &nametable);
        (void)pcre2_pattern_info(re, PCRE2_INFO_NAMEENTRYSIZE, &nameentrysize);
        tabptr = nametable;
        for(int i = 0; i < (int)namecount; i++)
        {
            int n = (tabptr[0] << 8) | tabptr[1];
            int valuelength = (int)(ovector[2 * n + 1] - ovector[2 * n]);
            int keylength = (int)nameentrysize - 3;
            char* _key = ALLOCATE(char, keylength + 1);
            char* _val = ALLOCATE(char, valuelength + 1);
            sprintf(_key, "%*s", keylength, tabptr + 2);
            sprintf(_val, "%*s", valuelength, subject + ovector[2 * n]);
            while(isspace((unsigned char)*_key))
            {
                _key++;
            }
            bl_dict_setentry(vm, result, OBJ_VAL(bl_mem_gcprotect(vm, (Object*)bl_string_takestring(vm, _key, keylength))),
                             OBJ_VAL(bl_mem_gcprotect(vm, (Object*)bl_string_takestring(vm, _val, valuelength))));
            tabptr += nameentrysize;
        }
    }
    pcre2_match_data_free(matchdata);
    pcre2_code_free(re);
    RETURN_OBJ(result);
    #endif
    RETURN_NUMBER(0);
}

static bool objfn_string_matches(VMState* vm, int argcount, Value* args)
{
    #if 0
    ENFORCE_ARG_COUNT(matches, 1);
    ENFORCE_ARG_TYPE(matches, 0, bl_value_isstring);
    ObjString* string = AS_STRING(METHOD_OBJECT);
    ObjString* substr = AS_STRING(args[0]);
    if(string->length == 0 && substr->length == 0)
    {
        RETURN_OBJ(bl_object_makelist(vm));// empty string matches empty string to empty list
    }
    else if(string->length == 0 || substr->length == 0)
    {
        RETURN_FALSE;// if either string or str is empty, return false
    }
    GET_REGEX_COMPILE_OPTIONS(substr, true);
    char* realregex = bl_helper_objstringremregexdelim(vm, substr);
    int errornumber;
    PCRE2_SIZE erroroffset;
    uint32_t optionbits;
    uint32_t newline;
    uint32_t namecount;
    uint32_t groupcount;
    uint32_t nameentrysize;
    PCRE2_SPTR nametable;
    PCRE2_SPTR pattern = (PCRE2_SPTR)realregex;
    PCRE2_SPTR subject = (PCRE2_SPTR)string->chars;
    PCRE2_SIZE subjectlength = (PCRE2_SIZE)string->length;
    pcre2_code* re = pcre2_compile(pattern, PCRE2_ZERO_TERMINATED, compileoptions, &errornumber, &erroroffset, NULL);
    free(realregex);
    if(!re)
    {
        REGEX_COMPILATION_ERROR(re, errornumber, erroroffset);
    }
    pcre2_match_data* matchdata = pcre2_match_data_create_from_pattern(re, NULL);
    int rc = pcre2_match(re, subject, subjectlength, 0, 0, matchdata, NULL);
    if(rc < 0)
    {
        if(rc == PCRE2_ERROR_NOMATCH)
        {
            RETURN_FALSE;
        }
        else
        {
            REGEX_RC_ERROR();
        }
    }
    PCRE2_SIZE* ovector = pcre2_get_ovector_pointer(matchdata);
    //   REGEX_VECTOR_SIZE_WARNING();
    // handle edge cases such as /(?=.\K)/
    REGEX_ASSERTION_ERROR(re, matchdata, ovector);
    (void)pcre2_pattern_info(re, PCRE2_INFO_NAMECOUNT, &namecount);
    (void)pcre2_pattern_info(re, PCRE2_INFO_CAPTURECOUNT, &groupcount);
    ObjDict* result = (ObjDict*)bl_mem_gcprotect(vm, (Object*)bl_object_makedict(vm));
    for(int i = 0; i < rc; i++)
    {
        bl_dict_setentry(vm, result, NUMBER_VAL(0), NIL_VAL);
    }
    // add first set of matches to response
    for(int i = 0; i < rc; i++)
    {
        ObjArray* list = (ObjArray*)bl_mem_gcprotect(vm, (Object*)bl_object_makelist(vm));
        PCRE2_SIZE substringlength = ovector[2 * i + 1] - ovector[2 * i];
        PCRE2_SPTR substringstart = subject + ovector[2 * i];
        bl_array_push(vm, list, GC_L_STRING((char*)substringstart, (int)substringlength));
        bl_dict_setentry(vm, result, NUMBER_VAL(i), OBJ_VAL(list));
    }
    if(namecount > 0)
    {
        PCRE2_SPTR tabptr;
        (void)pcre2_pattern_info(re, PCRE2_INFO_NAMETABLE, &nametable);
        (void)pcre2_pattern_info(re, PCRE2_INFO_NAMEENTRYSIZE, &nameentrysize);
        tabptr = nametable;
        for(int i = 0; i < (int)namecount; i++)
        {
            int n = (tabptr[0] << 8) | tabptr[1];
            int valuelength = (int)(ovector[2 * n + 1] - ovector[2 * n]);
            int keylength = (int)nameentrysize - 3;
            char* _key = ALLOCATE(char, keylength + 1);
            char* _val = ALLOCATE(char, valuelength + 1);
            sprintf(_key, "%*s", keylength, tabptr + 2);
            sprintf(_val, "%*s", valuelength, subject + ovector[2 * n]);
            while(isspace((unsigned char)*_key))
            {
                _key++;
            }
            ObjArray* list = (ObjArray*)bl_mem_gcprotect(vm, (Object*)bl_object_makelist(vm));
            bl_array_push(vm, list, OBJ_VAL(bl_mem_gcprotect(vm, (Object*)bl_string_takestring(vm, _val, valuelength))));
            bl_dict_addentry(vm, result, OBJ_VAL(bl_mem_gcprotect(vm, (Object*)bl_string_takestring(vm, _key, keylength))), OBJ_VAL(list));
            tabptr += nameentrysize;
        }
    }
    (void)pcre2_pattern_info(re, PCRE2_INFO_ALLOPTIONS, &optionbits);
    int utf8 = (optionbits & PCRE2_UTF) != 0;
    (void)pcre2_pattern_info(re, PCRE2_INFO_NEWLINE, &newline);
    int crlfisnewline = newline == PCRE2_NEWLINE_ANY || newline == PCRE2_NEWLINE_CRLF || newline == PCRE2_NEWLINE_ANYCRLF;
    // find the other matches
    for(;;)
    {
        uint32_t options = 0;
        PCRE2_SIZE startoffset = ovector[1];
        // if the previous match was for an empty string
        if(ovector[0] == ovector[1])
        {
            if(ovector[0] == subjectlength)
            {
                break;
            }
            options = PCRE2_NOTEMPTY_ATSTART | PCRE2_ANCHORED;
        }
        else
        {
            PCRE2_SIZE startchar = pcre2_get_startchar(matchdata);
            if(startoffset > subjectlength - 1)
            {
                break;
            }
            if(startoffset <= startchar)
            {
                if(startchar >= subjectlength - 1)
                {
                    break;
                }
                startoffset = startchar + 1;
                if(utf8)
                {
                    for(; startoffset < subjectlength; startoffset++)
                    {
                        if((subject[startoffset] & 0xc0) != 0x80)
                        {
                            break;
                        }
                    }
                }
            }
        }
        rc = pcre2_match(re, subject, subjectlength, startoffset, options, matchdata, NULL);
        if(rc == PCRE2_ERROR_NOMATCH)
        {
            if(options == 0)
            {
                break;
            }
            ovector[1] = startoffset + 1;
            if(crlfisnewline && startoffset < subjectlength - 1 && subject[startoffset] == '\r' && subject[startoffset + 1] == '\n')
            {
                ovector[1] += 1;
            }
            else if(utf8)
            {
                while(ovector[1] < subjectlength)
                {
                    if((subject[ovector[1]] & 0xc0) != 0x80)
                    {
                        break;
                    }
                    ovector[1] += 1;
                }
            }
            continue;
        }
        if(rc < 0 && rc != PCRE2_ERROR_PARTIAL)
        {
            pcre2_match_data_free(matchdata);
            pcre2_code_free(re);
            REGEX_ERR("regular expression error %d", rc);
        }
        // REGEX_VECTOR_SIZE_WARNING();
        REGEX_ASSERTION_ERROR(re, matchdata, ovector);
        for(int i = 0; i < rc; i++)
        {
            PCRE2_SIZE substringlength = ovector[2 * i + 1] - ovector[2 * i];
            PCRE2_SPTR substringstart = subject + ovector[2 * i];
            Value vlist;
            if(bl_dict_getentry(result, NUMBER_VAL(i), &vlist))
            {
                bl_array_push(vm, AS_LIST(vlist), GC_L_STRING((char*)substringstart, (int)substringlength));
            }
            else
            {
                ObjArray* list = (ObjArray*)bl_mem_gcprotect(vm, (Object*)bl_object_makelist(vm));
                bl_array_push(vm, list, GC_L_STRING((char*)substringstart, (int)substringlength));
                bl_dict_setentry(vm, result, NUMBER_VAL(i), OBJ_VAL(list));
            }
        }
        if(namecount > 0)
        {
            PCRE2_SPTR tabptr;
            (void)pcre2_pattern_info(re, PCRE2_INFO_NAMETABLE, &nametable);
            (void)pcre2_pattern_info(re, PCRE2_INFO_NAMEENTRYSIZE, &nameentrysize);
            tabptr = nametable;
            for(int i = 0; i < (int)namecount; i++)
            {
                int n = (tabptr[0] << 8) | tabptr[1];
                int valuelength = (int)(ovector[2 * n + 1] - ovector[2 * n]);
                int keylength = (int)nameentrysize - 3;
                char* _key = ALLOCATE(char, keylength + 1);
                char* _val = ALLOCATE(char, valuelength + 1);
                sprintf(_key, "%*s", keylength, tabptr + 2);
                sprintf(_val, "%*s", valuelength, subject + ovector[2 * n]);
                while(isspace((unsigned char)*_key))
                {
                    _key++;
                }
                ObjString* name = (ObjString*)bl_mem_gcprotect(vm, (Object*)bl_string_takestring(vm, _key, keylength));
                ObjString* value = (ObjString*)bl_mem_gcprotect(vm, (Object*)bl_string_takestring(vm, _val, valuelength));
                Value nlist;
                if(bl_dict_getentry(result, OBJ_VAL(name), &nlist))
                {
                    bl_array_push(vm, AS_LIST(nlist), OBJ_VAL(value));
                }
                else
                {
                    ObjArray* list = (ObjArray*)bl_mem_gcprotect(vm, (Object*)bl_object_makelist(vm));
                    bl_array_push(vm, list, OBJ_VAL(value));
                    bl_dict_setentry(vm, result, OBJ_VAL(name), OBJ_VAL(list));
                }
                tabptr += nameentrysize;
            }
        }
    }
    pcre2_match_data_free(matchdata);
    pcre2_code_free(re);
    RETURN_OBJ(result);
    #endif
    RETURN_NUMBER(0);
}

static bool objfn_string_replace(VMState* vm, int argcount, Value* args)
{
    #if 0
    ENFORCE_ARG_COUNT(replace, 2);
    ENFORCE_ARG_TYPE(replace, 0, bl_value_isstring);
    ENFORCE_ARG_TYPE(replace, 1, bl_value_isstring);
    ObjString* string = AS_STRING(METHOD_OBJECT);
    ObjString* substr = AS_STRING(args[0]);
    ObjString* repsubstr = AS_STRING(args[1]);
    if(string->length == 0 && substr->length == 0)
    {
        RETURN_TRUE;
    }
    else if(string->length == 0 || substr->length == 0)
    {
        RETURN_FALSE;
    }
    GET_REGEX_COMPILE_OPTIONS(substr, false);
    char* realregex = bl_helper_objstringremregexdelim(vm, substr);
    PCRE2_SPTR input = (PCRE2_SPTR)string->chars;
    PCRE2_SPTR pattern = (PCRE2_SPTR)realregex;
    PCRE2_SPTR replacement = (PCRE2_SPTR)repsubstr->chars;
    int result;
    int errornumber;
    PCRE2_SIZE erroroffset;
    pcre2_code* re = pcre2_compile(pattern, PCRE2_ZERO_TERMINATED, compileoptions & PCRE2_MULTILINE, &errornumber, &erroroffset, 0);
    free(realregex);
    if(!re)
    {
        REGEX_COMPILATION_ERROR(re, errornumber, erroroffset);
    }
    pcre2_match_context* matchcontext = pcre2_match_context_create(0);
    PCRE2_SIZE outputlength = 0;
    result = pcre2_substitute(re, input, PCRE2_ZERO_TERMINATED, 0, PCRE2_SUBSTITUTE_GLOBAL | PCRE2_SUBSTITUTE_OVERFLOW_LENGTH, 0, matchcontext, replacement,
                              PCRE2_ZERO_TERMINATED, 0, &outputlength);
    if(result < 0 && result != PCRE2_ERROR_NOMEMORY)
    {
        REGEX_ERR("regular expression post-compilation failed for replacement", result);
    }
    PCRE2_UCHAR* outputbuffer = ALLOCATE(PCRE2_UCHAR, outputlength);
    result = pcre2_substitute(re, input, PCRE2_ZERO_TERMINATED, 0, PCRE2_SUBSTITUTE_GLOBAL | PCRE2_SUBSTITUTE_UNSET_EMPTY, 0, matchcontext, replacement,
                              PCRE2_ZERO_TERMINATED, outputbuffer, &outputlength);
    if(result < 0 && result != PCRE2_ERROR_NOMEMORY)
    {
        REGEX_ERR("regular expression error at replacement time", result);
    }
    ObjString* response = bl_string_takestring(vm, (char*)outputbuffer, (int)outputlength);
    pcre2_match_context_free(matchcontext);
    pcre2_code_free(re);
    RETURN_OBJ(response);
    #endif
    RETURN_NUMBER(0);
}

static bool objfn_string_tobytes(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(tobytes, 0);
    ObjString* string = AS_STRING(METHOD_OBJECT);
    RETURN_OBJ(bl_bytes_copybytes(vm, (unsigned char*)string->chars, string->length));
}

static bool objfn_string_iter(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(__iter__, 1);
    ENFORCE_ARG_TYPE(__iter__, 0, bl_value_isnumber);
    ObjString* string = AS_STRING(METHOD_OBJECT);
    int length = string->isascii ? string->length : string->utf8length;
    int index = AS_NUMBER(args[0]);
    if(index > -1 && index < length)
    {
        int start = index;
        int end = index + 1;
        if(!string->isascii)
        {
            bl_util_utf8slice(string->chars, &start, &end);
        }
        RETURN_L_STRING(string->chars + start, (int)(end - start));
    }
    return bl_value_returnnil(vm, args);
}

static bool objfn_string_itern(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(__itern__, 1);
    ObjString* string = AS_STRING(METHOD_OBJECT);
    int length = string->isascii ? string->length : string->utf8length;
    if(bl_value_isnil(args[0]))
    {
        if(length == 0)
        {
            RETURN_FALSE;
        }
        RETURN_NUMBER(0);
    }
    if(!bl_value_isnumber(args[0]))
    {
        RETURN_ERROR("bytes are numerically indexed");
    }
    int index = AS_NUMBER(args[0]);
    if(index < length - 1)
    {
        RETURN_NUMBER((double)index + 1);
    }
    return bl_value_returnnil(vm, args);
}

void bl_state_initstringmethods(VMState* vm)
{
    bl_state_defineglobal(vm, vm->classobjstring->name, OBJ_VAL(vm->classobjstring));
    // string methods
    bl_class_defnativefield(vm, vm->classobjstring, "length", objfn_string_length);
    bl_class_defnativemethod(vm, vm->classobjstring, "upper", objfn_string_upper);
    bl_class_defnativemethod(vm, vm->classobjstring, "lower", objfn_string_lower);
    bl_class_defnativemethod(vm, vm->classobjstring, "isalpha", objfn_string_isalpha);
    bl_class_defnativemethod(vm, vm->classobjstring, "isalnum", objfn_string_isalnum);
    bl_class_defnativemethod(vm, vm->classobjstring, "is_number", objfn_string_isnumber);
    bl_class_defnativemethod(vm, vm->classobjstring, "islower", objfn_string_islower);
    bl_class_defnativemethod(vm, vm->classobjstring, "isupper", objfn_string_isupper);
    bl_class_defnativemethod(vm, vm->classobjstring, "isspace", objfn_string_isspace);
    bl_class_defnativemethod(vm, vm->classobjstring, "trim", objfn_string_trim);
    bl_class_defnativemethod(vm, vm->classobjstring, "ltrim", objfn_string_ltrim);
    bl_class_defnativemethod(vm, vm->classobjstring, "rtrim", objfn_string_rtrim);
    bl_class_defnativemethod(vm, vm->classobjstring, "join", objfn_string_join);
    bl_class_defnativemethod(vm, vm->classobjstring, "split", objfn_string_split);
    bl_class_defnativemethod(vm, vm->classobjstring, "indexof", objfn_string_indexof);
    bl_class_defnativemethod(vm, vm->classobjstring, "startswith", objfn_string_startswith);
    bl_class_defnativemethod(vm, vm->classobjstring, "endswith", objfn_string_endswith);
    bl_class_defnativemethod(vm, vm->classobjstring, "count", objfn_string_count);
    bl_class_defnativemethod(vm, vm->classobjstring, "to_number", objfn_string_tonumber);
    bl_class_defnativemethod(vm, vm->classobjstring, "to_list", objfn_string_tolist);
    bl_class_defnativemethod(vm, vm->classobjstring, "tobytes", objfn_string_tobytes);
    bl_class_defnativemethod(vm, vm->classobjstring, "lpad", objfn_string_lpad);
    bl_class_defnativemethod(vm, vm->classobjstring, "rpad", objfn_string_rpad);
    bl_class_defnativemethod(vm, vm->classobjstring, "match", objfn_string_match);
    bl_class_defnativemethod(vm, vm->classobjstring, "matches", objfn_string_matches);
    bl_class_defnativemethod(vm, vm->classobjstring, "replace", objfn_string_replace);
    bl_class_defnativemethod(vm, vm->classobjstring, "ascii", objfn_string_ascii);
    bl_class_defnativemethod(vm, vm->classobjstring, "@iter", objfn_string_iter);
    bl_class_defnativemethod(vm, vm->classobjstring, "@itern", objfn_string_itern);
}
