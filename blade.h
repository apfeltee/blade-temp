
#pragma once

#define _GNU_SOURCE 1
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <math.h>
#include <assert.h>
#include <sys/stat.h>

#if defined(__linux__) || defined(__unix__)
    #include <libgen.h>
    #include <dlfcn.h>
    #include <unistd.h>
    #include <dirent.h>
    #include <termios.h>
    #include <sys/mman.h>
    #include <utime.h>
    #include <sys/utsname.h>
    #include <sys/param.h>
    #include <sys/types.h>
    #include <sys/wait.h>
    #include <sys/time.h>
#endif

#include "xxhash.h"
#include "ktre.h"

#define PCRE2_STATIC
#define PCRE2_CODE_UNIT_WIDTH 8
//#include <pcre2.h>

#define BLADE_EXTENSION ".b"
#define BLADE_VERSION_STRING "0.0.74-rc1"
#define BVM_VERSION "0.0.7"
#define LIBRARY_DIRECTORY "libs"
#define LIBRARY_DIRECTORY_INDEX "index"
#define PACKAGES_DIRECTORY "vendor"
#define LOCAL_PACKAGES_DIRECTORY ".blade"
#define LOCAL_SRC_DIRECTORY "/libs"
// global debug mode flag
#define DEBUG_MODE 0
#define MAX_USING_CASES 256
#define MAX_FUNCTION_PARAMETERS 255
#define FRAMES_MAX 512
#define NUMBER_FORMAT "%.16g"
#define MAX_INTERPOLATION_NESTING 8
#define MAX_EXCEPTION_HANDLERS 16
// Maximum load factor of 12/14
// see: https://engineering.fb.com/2019/04/25/developer-tools/f14/
#define TABLE_MAX_LOAD 0.85714286
#define GC_HEAP_GROWTH_FACTOR 1.25
#define HAVE_TERMIOS_H
#define HAVE_SYS_UTSNAME_H
#define HAVE_UTIME

#ifndef BYTE_ORDER
    #if(BSD >= 199103) || defined(__MACH__) || defined(__APPLE__)
        #include <machine/endian.h>
    #elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
        #include <sys/endian.h>
    #elif defined(linux) || defined(__linux__)
        #include <endian.h>
    #else
        /* least-significant byte first (vax, pc) */
        #define LITTLE_ENDIAN 1234
        /* most-significant byte first (IBM, net) */
        #define BIG_ENDIAN 4321
        /* LSB first in word, MSW first in long (pdp)*/
        #define PDP_ENDIAN 3412
        /* msvc for intel processors */
        /* msvc code on arm executes in little endian mode */
        #if defined(__i386__) || defined(BIT_ZERO_ON_RIGHT) || defined(_M_IX86) || defined(_M_X64) || defined(_M_IA64) || defined(_M_ARM) || defined(_WIN32) || defined(_WIN64)
            #define BYTE_ORDER LITTLE_ENDIAN
        #endif
        #if defined(BIT_ZERO_ON_LEFT)
            #define BYTE_ORDER BIG_ENDIAN
        #endif
    #endif /* BSD */
#endif /* BYTE_ORDER */

#if defined(__BYTE_ORDER) && !defined(BYTE_ORDER)
    #if(__BYTE_ORDER == __LITTLE_ENDIAN)
        #define BYTE_ORDER LITTLE_ENDIAN
    #else
        #define BYTE_ORDER BIG_ENDIAN
    #endif
#endif

#if !defined(BYTE_ORDER) || (BYTE_ORDER != BIG_ENDIAN && BYTE_ORDER != LITTLE_ENDIAN && BYTE_ORDER != PDP_ENDIAN)
    /* you must determine what the correct bit order is for
   * your compiler - the next line is an intentional error
   * which will force your compiles to bomb until you fix
   * the above macros.
   */
    #error "Undefined or invalid BYTE_ORDER"
#endif

#if BYTE_ORDER == LITTLE_ENDIAN
    #define IS_LITTLE_ENDIAN 1
    #define IS_BIG_ENDIAN 0
#elif BYTE_ORDER == BIG_ENDIAN
    #define IS_LITTLE_ENDIAN 0
    #define IS_BIG_ENDIAN 1
#else
    #error "Cannot determine machine endianess."
#endif

#if defined(__x86_64__) || defined(__LP64__) || defined(_LP64) || defined(_WIN64) || defined(__aarch64__)
    #define IS_64_BIT 1
#else
    #define IS_64_BIT 0
#endif

// --> debug mode options starts here...
#if DEBUG_MODE == 1
    #define DEBUG_TRACE_EXECUTION 0
    #define DEBUG_PRINT_CODE 0
    #define DEBUG_TABLE 0
    #define DEBUG_LOG_GC 1
#endif

// --> debug mode options ends here...
#define UINT8_COUNT (UINT8_MAX + 1)
//#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)
#define STACK_MAX (1024)
#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
    #define IS_UNIX
#endif

#define VERSION(x) #x
#define VERSION_STRING(name, major, minor, patch) name " " VERSION(major) "." VERSION(minor) "." VERSION(patch)

#ifdef __clang__
    #define COMPILER VERSION_STRING("Clang", __clang_major__, __clang_minor__, __clang_patchlevel__)
#elif defined(_MSC_VER)
    #define COMPILER VERSION_STRING("MSC", _MSC_VER, 0, 0)
#elif defined(__MINGW32_MAJOR_VERSION)
    #define COMPILER VERSION_STRING("MinGW32", __MINGW32_MAJOR_VERSION, __MINGW32_MINOR_VERSION, 0)
#elif defined(__MINGW64_VERSION_MAJOR)
    #define COMPILER VERSION_STRING("MinGW-64", __MINGW64_VERSION_MAJOR, __MINGW64_VERSION_MAJOR, 0)
#elif defined(__GNUC__)
    #define COMPILER VERSION_STRING("GCC", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__)
#else
    #define COMPILER "Unknown Compiler"
#endif

#if defined(__APPLE__) && defined(__MACH__)// Apple OSX and iOS (Darwin)
    #define LIBRARY_FILE_EXTENSION ".dylib"
#elif defined(_WIN32)
    #define LIBRARY_FILE_EXTENSION ".dll"
#else
    #define LIBRARY_FILE_EXTENSION ".so"
#endif


#if !defined(PATH_MAX)
    #define PATH_MAX 1024
#endif


#define BLADE_PATH_SEPARATOR "/"


#define DEFAULT_GC_START (1024 * 1024)
#define EXIT_COMPILE 10
#define EXIT_RUNTIME 11
#define EXIT_TERMINAL 12
#define BLADE_COPYRIGHT "Copyright (c) 2021 Ore Richard Muyiwa"
// NOTE: METHOD_OBJECT must always be retrieved
// before any call to create an object in a native function.
// failure to do so will lead to the first object created
// within the function to appear as METHOD_OBJECT
#define METHOD_OBJECT args[-1]
/*
#define NORMALIZE_IS_BOOL "bool"
#define NORMALIZE_IS_BYTES "bytes"
#define NORMALIZE_IS_NUMBER "number"
#define NORMALIZE_IS_CHAR "char"
#define NORMALIZE_IS_STRING "string"
#define NORMALIZE_IS_CLOSURE "function"
#define NORMALIZE_IS_INSTANCE "instance"
#define NORMALIZE_IS_CLASS "class"
#define NORMALIZE_IS_LIST "list"
#define NORMALIZE_IS_DICT "dict"
#define NORMALIZE_IS_OBJ "object"
#define NORMALIZE_IS_FILE "file"
#define NORMALIZE_IS_PTR "ptr"
#define NORMALIZE(token) NORMALIZE_##token
*/
#define NORMALIZE(token) #token

#define ENFORCE_ARG_COUNT(name, d) \
    if(argcount != d) \
    { \
        RETURN_ERROR(#name "() expects %d arguments, %d given", d, argcount); \
    }

#define ENFORCE_MIN_ARG(name, d) \
    if(argcount < d) \
    { \
        RETURN_ERROR(#name "() expects minimum of %d arguments, %d given", d, argcount); \
    }

#define ENFORCE_ARG_RANGE(name, low, up) \
    if(argcount < (low) || argcount > (up)) \
    { \
        RETURN_ERROR(#name "() expects between %d and %d arguments, %d given", low, up, argcount); \
    }

#define ENFORCE_ARG_TYPE(name, i, type) \
    if(!type(args[i])) \
    { \
        RETURN_ERROR(#name "() expects argument %d as " NORMALIZE(type) ", %s given", (i) + 1, bl_value_typename(args[i])); \
    }

#define EXCLUDE_ARG_TYPE(methodname, arg_type, index) \
    if(arg_type(args[index])) \
    { \
        RETURN_ERROR("invalid type %s() as argument %d in %s()", bl_value_typename(args[index]), (index) + 1, #methodname); \
    }

#define METHOD_OVERRIDE(override, i) \
    do \
    { \
        if(bl_value_isinstance(args[0])) \
        { \
            ObjInstance* instance = AS_INSTANCE(args[0]); \
            if(bl_vmdo_instanceinvokefromclass(vm, instance->klass, bl_string_copystringlen(vm, "@" #override, (i) + 1), 0)) \
            { \
                args[-1] = TRUE_VAL; \
                return false; \
            } \
        } \
    } while(0);

#define REGEX_COMPILATION_ERROR(re, errornumber, erroroffset) \
    if((re) == NULL) \
    { \
        PCRE2_UCHAR8 buffer[256]; \
        pcre2_get_error_message_8(errornumber, buffer, sizeof(buffer)); \
        RETURN_ERROR("regular expression compilation failed at offset %d: %s", (int)(erroroffset), buffer); \
    }

#define REGEX_ASSERTION_ERROR(re, matchdata, ovector) \
    if((ovector)[0] > (ovector)[1]) \
    { \
        RETURN_ERROR("match aborted: regular expression used \\K in an assertion %.*s to " \
                     "set match start after its end.", \
                     (int)((ovector)[0] - (ovector)[1]), (char*)(subject + (ovector)[1])); \
        pcre2_match_data_free(matchdata); \
        pcre2_code_free(re); \
        RETURN_EMPTY; \
    }

#define REGEX_ERR(message, result) \
    do \
    { \
        PCRE2_UCHAR error[255]; \
        if(pcre2_get_error_message(result, error, 255)) \
        { \
            RETURN_ERROR("RegexError: (%d) %s", result, (char*)error); \
        } \
        RETURN_ERROR("RegexError: %s", message); \
    } while(0)

#define REGEX_RC_ERROR() REGEX_ERR("%d", rc);

#define GET_REGEX_COMPILE_OPTIONS(string, regexshowerror) \
    uint32_t compileoptions = bl_helper_objstringisregex(string); \
    if((regexshowerror) && (int)compileoptions == -1) \
    { \
        RETURN_ERROR("RegexError: Invalid regex"); \
    } \
    else if((regexshowerror) && (int)compileoptions > 1000000) \
    { \
        RETURN_ERROR("RegexError: invalid modifier '%c' ", (char)abs(1000000 - (int)compileoptions)); \
    }

// NOTE:
// any call to bl_mem_gcprotect() within a function/block must accompanied by
// at least one call to bl_mem_gcclearprotect(vm) before exiting the function/block
// otherwise, expected unexpected behavior
// NOTE as well that the call to bl_mem_gcclearprotect(vm) will be automatic for
// native functions.
// NOTE as well that METHOD_OBJECT must be retrieved before any call
// to bl_mem_gcprotect() in a native function.
#define GC_STRING(o) OBJ_VAL(bl_mem_gcprotect(vm, (Object*)bl_string_copystringlen(vm, (const char*)(o), (int)strlen(o))))
#define GC_L_STRING(o, l) OBJ_VAL(bl_mem_gcprotect(vm, (Object*)bl_string_copystringlen(vm, (const char*)(o), (l))))

// promote C values to blade value
#define EMPTY_VAL ((Value){ VAL_EMPTY, { .number = 0 } })
#define NIL_VAL ((Value){ VAL_NIL, { .number = 0 } })
#define TRUE_VAL ((Value){ VAL_BOOL, { .boolean = true } })
#define FALSE_VAL ((Value){ VAL_BOOL, { .boolean = false } })
#define BOOL_VAL(v) ((Value){ VAL_BOOL, { .boolean = (v) } })
#define NUMBER_VAL(v) ((Value){ VAL_NUMBER, { .number = (double)(v) } })
#define INTEGER_VAL(v) ((Value){ VAL_NUMBER, { .number = (double)(v) } })
#define OBJ_VAL(v) ((Value){ VAL_OBJ, { .obj = (Object*)(v) } })

// demote blade values to C value
#define AS_BOOL(v) ((v).as.boolean)
#define AS_NUMBER(v) ((v).as.number)
#define AS_OBJ(v) ((v).as.obj)

// testing blade value types

#define GROW_CAPACITY(capacity) ((capacity) < 4 ? 4 : (capacity)*2)
#define GROW_ARRAY(type, sztype, pointer, oldcount, newcount) (type*)bl_mem_growarray(vm, pointer, sztype, oldcount, newcount)
#define FREE_ARRAY(type, pointer, oldcount) bl_mem_free(vm, pointer, sizeof(type) * (oldcount))
#define FREE(type, pointer) bl_mem_free(vm, pointer, sizeof(type))
#define ALLOCATE(type, count) (type*)bl_mem_realloc(vm, NULL, 0, sizeof(type) * (count))
#define STRING_VAL(val) OBJ_VAL(bl_string_copystringlen(vm, val, (int)strlen(val)))
#define STRING_L_VAL(val, l) OBJ_VAL(bl_string_copystringlen(vm, val, l))
#define STRING_TT_VAL(val) OBJ_VAL(bl_string_takestring(vm, val, (int)strlen(val)))
#define OBJ_TYPE(v) (AS_OBJ(v)->type)

// promote Value to object
#define AS_STRING(v) ((ObjString*)AS_OBJ(v))
#define AS_NATIVE(v) ((ObjNativeFunction*)AS_OBJ(v))
#define AS_FUNCTION(v) ((ObjFunction*)AS_OBJ(v))
#define AS_CLOSURE(v) ((ObjClosure*)AS_OBJ(v))
#define AS_CLASS(v) ((ObjClass*)AS_OBJ(v))
#define AS_INSTANCE(v) ((ObjInstance*)AS_OBJ(v))
#define AS_BOUND(v) ((ObjBoundMethod*)AS_OBJ(v))

// non-user objects
#define AS_SWITCH(v) ((ObjSwitch*)AS_OBJ(v))
#define AS_PTR(v) ((ObjPointer*)AS_OBJ(v))

#define AS_MODULE(v) ((ObjModule*)AS_OBJ(v))

// containers
#define AS_BYTES(v) ((ObjBytes*)AS_OBJ(v))
#define AS_LIST(v) ((ObjArray*)AS_OBJ(v))
#define AS_DICT(v) ((ObjDict*)AS_OBJ(v))
#define AS_FILE(v) ((ObjFile*)AS_OBJ(v))
#define AS_RANGE(v) ((ObjRange*)AS_OBJ(v))

// demote blade value to c string
#define AS_C_STRING(v) (((ObjString*)AS_OBJ(v))->chars)

#define IS_CHAR(v) (bl_value_isstring(v) && (AS_STRING(v)->length == 1 || AS_STRING(v)->length == 0))

#define EXIT_VM() return PTR_RUNTIME_ERR

#define runtime_error(...) \
    if(!bl_vm_throwexception(vm, false, ##__VA_ARGS__)) \
    { \
        EXIT_VM(); \
    }


#define RETURN_EMPTY \
    { \
        args[-1] = NIL_VAL; \
        return false; \
    }

#define RETURN_ERROR(...) \
    { \
        bl_vm_popvaluen(vm, argcount); \
        bl_vm_throwexception(vm, false, ##__VA_ARGS__); \
        args[-1] = FALSE_VAL; \
        return false; \
    }

#define RETURN_BOOL(v) \
    { \
        args[-1] = BOOL_VAL(v); \
        return true; \
    }
#define RETURN_TRUE \
    { \
        args[-1] = TRUE_VAL; \
        return true; \
    }
#define RETURN_FALSE \
    { \
        args[-1] = FALSE_VAL; \
        return true; \
    }
#define RETURN_NUMBER(v) \
    { \
        args[-1] = NUMBER_VAL(v); \
        return true; \
    }
#define RETURN_OBJ(v) \
    { \
        args[-1] = OBJ_VAL(v); \
        return true; \
    }

#define RETURN_L_STRING(v, l) \
    { \
        args[-1] = OBJ_VAL(bl_string_copystringlen(vm, v, l)); \
        return true; \
    }
#define RETURN_T_STRING(v, l) \
    { \
        args[-1] = OBJ_VAL(bl_string_takestring(vm, v, l)); \
        return true; \
    }
#define RETURN_TT_STRING(v) \
    { \
        args[-1] = OBJ_VAL(bl_string_takestring(vm, v, (int)strlen(v))); \
        return true; \
    }
#define RETURN_VALUE(v) \
    { \
        args[-1] = v; \
        return true; \
    }


enum ValType
{
    VAL_NIL,
    VAL_BOOL,
    VAL_NUMBER,
    VAL_OBJ,
    VAL_EMPTY,
};

enum OpCode
{
    OP_DEFINE_GLOBAL,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,
    OP_GET_LOCAL,
    OP_GET_UP_VALUE,
    OP_SET_LOCAL,
    OP_SET_UP_VALUE,
    OP_CLOSE_UP_VALUE,
    OP_GET_PROPERTY,
    OP_GET_SELF_PROPERTY,
    OP_SET_PROPERTY,
    OP_JUMP_IF_FALSE,
    OP_JUMP,
    OP_LOOP,
    OP_EQUAL,
    OP_GREATERTHAN,
    OP_LESSTHAN,
    OP_EMPTY,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_F_DIVIDE,// floor divide
    OP_REMINDER,
    OP_POW,
    OP_NEGATE,
    OP_NOT,
    OP_BIT_NOT,
    OP_BITAND,
    OP_BITOR,
    OP_BITXOR,
    OP_LEFTSHIFT,
    OP_RIGHTSHIFT,
    OP_ONE,
    OP_CONSTANT,// 8-bit constant address (0 - 255)
    OP_ECHO,
    OP_POP,
    OP_DUP,
    OP_POP_N,
    OP_ASSERT,
    OP_DIE,
    OP_CLOSURE,
    OP_CALL,
    OP_INVOKE,
    OP_INVOKE_SELF,
    OP_RETURN,
    OP_CLASS,
    OP_METHOD,
    OP_CLASS_PROPERTY,
    OP_INHERIT,
    OP_GET_SUPER,
    OP_SUPER_INVOKE,
    OP_SUPER_INVOKE_SELF,
    OP_RANGE,
    OP_LIST,
    OP_DICT,
    OP_GET_INDEX,
    OP_GET_RANGED_INDEX,
    OP_SET_INDEX,
    OP_CALL_IMPORT,
    OP_NATIVE_MODULE,
    OP_SELECT_IMPORT,
    OP_SELECT_NATIVE_IMPORT,
    OP_IMPORT_ALL_NATIVE,
    OP_EJECT_IMPORT,
    OP_EJECT_NATIVE_IMPORT,
    OP_IMPORT_ALL,
    OP_TRY,
    OP_POP_TRY,
    OP_PUBLISH_TRY,
    OP_STRINGIFY,
    OP_SWITCH,
    OP_CHOICE,
    // the break placeholder... it never gets to the vm
    // care should be taken to
    OP_BREAK_PL,
};

enum FuncType
{
    TYPE_FUNCTION,
    TYPE_METHOD,
    TYPE_INITIALIZER,
    TYPE_PRIVATE,
    TYPE_STATIC,
    TYPE_SCRIPT,
};

enum ObjType
{
    // containers
    OBJ_STRING,
    OBJ_RANGE,
    OBJ_ARRAY,
    OBJ_DICT,
    OBJ_FILE,
    OBJ_BYTES,
    // base object types
    OBJ_UP_VALUE,
    OBJ_BOUNDFUNCTION,
    OBJ_BOUNDMETHOD = OBJ_BOUNDFUNCTION,
    OBJ_CLOSURE,
    OBJ_SCRIPTFUNCTION,
    OBJ_INSTANCE,
    OBJ_NATIVEFUNCTION,
    OBJ_CLASS,
    // non-user objects
    OBJ_MODULE,
    OBJ_SWITCH,
    OBJ_PTR,// object type that can hold any C pointer
};

enum TokType
{
    // symbols
    TOK_NEWLINE,// \n
    TOK_LPAREN,// (
    TOK_RPAREN,// )
    TOK_LBRACKET,// [
    TOK_RBRACKET,// ]
    TOK_LBRACE,// {
    TOK_RBRACE,// }
    TOK_SEMICOLON,// ;
    TOK_COMMA,// ,
    TOK_BACKSLASH,// '\'
    TOK_BANG,// !
    TOK_BANGEQ,// !=
    TOK_COLON,// :
    TOK_AT,// @
    TOK_DOT,// .
    TOK_RANGE,// ..
    TOK_TRIDOT,// ...
    TOK_PLUS,// +
    TOK_PLUSEQ,// +=
    TOK_INCREMENT,// ++
    TOK_MINUS,// -
    TOK_MINUSEQ,// -=
    TOK_DECREMENT,// --
    TOK_MULTIPLY,// *
    TOK_MULTIPLYEQ,// *=
    TOK_POW,// **
    TOK_POWEQ,// **=
    TOK_DIVIDE,// '/'
    TOK_DIVIDEEQ,// '/='
    TOK_FLOOR,// '//'
    TOK_FLOOREQ,// '//='
    TOK_EQUAL,// =
    TOK_EQUALEQ,// ==
    TOK_LESS,// <
    TOK_LESSEQ,// <=
    TOK_LSHIFT,// <<
    TOK_LSHIFTEQ,// <<=
    TOK_GREATER,// >
    TOK_GREATEREQ,// >=
    TOK_RSHIFT,// >>
    TOK_RSHIFTEQ,// >>=
    TOK_PERCENT,// %
    TOK_PERCENTEQ,// %=
    TOK_AMP,// &
    TOK_AMPEQ,// &=
    TOK_BAR,// |
    TOK_BAREQ,// |=
    TOK_TILDE,// ~
    TOK_TILDEEQ,// ~=
    TOK_XOR,// ^
    TOK_XOREQ,// ^=
    TOK_QUESTION,// ??
    // keywords
    TOK_AND,
    TOK_AS,
    TOK_ASSERT,
    TOK_BREAK,
    TOK_CATCH,
    TOK_CLASS,
    TOK_CONTINUE,
    TOK_DEF,
    TOK_DEFAULT,
    TOK_DIE,
    TOK_DO,
    TOK_ECHO,
    TOK_ELSE,
    TOK_FALSE,
    TOK_FINALLY,
    TOK_FOREACH,
    TOK_IF,
    TOK_IMPORT,
    TOK_IN,
    TOK_FORLOOP,
    TOK_NIL,
    TOK_OR,
    TOK_PARENT,
    TOK_RETURN,
    TOK_SELF,
    TOK_STATIC,
    TOK_TRUE,
    TOK_TRY,
    TOK_USING,
    TOK_VAR,
    TOK_WHEN,
    TOK_WHILE,
    // types token
    TOK_LITERAL,
    TOK_REGNUMBER,// regular numbers (inclusive of doubles)
    TOK_BINNUMBER,// binary numbers
    TOK_OCTNUMBER,// octal numbers
    TOK_HEXNUMBER,// hexadecimal numbers
    TOK_IDENTIFIER,
    TOK_DECORATOR,
    TOK_INTERPOLATION,
    TOK_EOF,
    // error
    TOK_ERROR,
    TOK_EMPTY,
    TOK_UNDEFINED,
};

enum PtrResult
{
    PTR_OK,
    PTR_COMPILE_ERR,
    PTR_RUNTIME_ERR,
};


enum AstPrecedence
{
    PREC_NONE,
    PREC_ASSIGNMENT,// =, &=, |=, *=, +=, -=, /=, **=, %=, >>=, <<=, ^=, //=
    // ~=
    PREC_CONDITIONAL,// ?:
    PREC_OR,// or
    PREC_AND,// and
    PREC_EQUALITY,// ==, !=
    PREC_COMPARISON,// <, >, <=, >=
    PREC_BIT_OR,// |
    PREC_BIT_XOR,// ^
    PREC_BIT_AND,// &
    PREC_SHIFT,// <<, >>
    PREC_RANGE,// ..
    PREC_TERM,// +, -
    PREC_FACTOR,// *, /, %, **, //
    PREC_UNARY,// !, -, ~, (++, -- this two will now be treated as statements)
    PREC_CALL,// ., ()
    PREC_PRIMARY
};


typedef enum OpCode OpCode;
typedef enum FuncType FuncType;
typedef enum ObjType ObjType;
typedef enum PtrResult PtrResult;
typedef enum TokType TokType;
typedef enum AstPrecedence AstPrecedence;
typedef enum ValType ValType;
typedef struct Value Value;
typedef struct AstCompiler AstCompiler;
typedef struct Object Object;
typedef struct ObjString ObjString;
typedef struct VMState VMState;
typedef struct RegModule RegModule;
typedef struct RegFunc RegFunc;
typedef struct RegField RegField;
typedef struct RegClass RegClass;
typedef struct ValArray ValArray;
typedef struct ByteArray ByteArray;
typedef struct BinaryBlob BinaryBlob;
typedef struct HashEntry HashEntry;
typedef struct HashTable HashTable;
typedef struct ObjUpvalue ObjUpvalue;
typedef struct ObjModule ObjModule;
typedef struct ObjFunction ObjFunction;
typedef struct ObjClosure ObjClosure;
typedef struct ObjClass ObjClass;
typedef struct ObjInstance ObjInstance;
typedef struct ObjBoundMethod ObjBoundMethod;
typedef struct ObjNativeFunction ObjNativeFunction;
typedef struct ObjArray ObjArray;
typedef struct ObjRange ObjRange;
typedef struct ObjBytes ObjBytes;
typedef struct ObjDict ObjDict;
typedef struct ObjFile ObjFile;
typedef struct ObjSwitch ObjSwitch;
typedef struct ObjPointer ObjPointer;
typedef struct ExceptionFrame ExceptionFrame;
typedef struct CallFrame CallFrame;
typedef struct AstToken AstToken;
typedef struct AstScanner AstScanner;
typedef struct AstLocal AstLocal;
typedef struct Upvalue Upvalue;
typedef struct AstClassCompiler AstClassCompiler;
typedef struct AstParser AstParser;
typedef struct AstRule AstRule;
typedef struct DynArray DynArray;
typedef struct BProcess BProcess;
typedef struct BProcessShared BProcessShared;
typedef Value (*ClassFieldFunc)(VMState*);
typedef void (*ModLoaderFunc)(VMState*);
typedef RegModule* (*ModInitFunc)(VMState*);
typedef bool (*NativeCallbackFunc)(VMState*, int, Value*);
typedef void (*PtrFreeFunc)(void*);
typedef void (*bparseprefixfn)(AstParser*, bool);
typedef void (*bparseinfixfn)(AstParser*, AstToken, bool);
typedef double(*VMBinaryCallbackFn)(double, double);

struct Value
{
    ValType type;

    union
    {
        bool boolean;
        double number;
        Object* obj;
    } as;
};

struct RegFunc
{
    const char* name;
    bool isstatic;
    NativeCallbackFunc natfn;
};

struct RegField
{
    const char* name;
    bool isstatic;
    ClassFieldFunc fieldfunc;
};

struct RegClass
{
    const char* name;
    RegField* fields;
    RegFunc* functions;
};

struct RegModule
{
    const char* name;
    RegField* fields;
    RegFunc* functions;
    RegClass* classes;
    ModLoaderFunc preloader;
    ModLoaderFunc unloader;
};

struct ValArray
{
    int capacity;
    int count;
    Value* values;
};

struct ByteArray
{
    int count;
    unsigned char* bytes;
};

struct BinaryBlob
{
    int count;
    int capacity;
    uint8_t* code;
    int* lines;
    ValArray constants;
};

struct HashEntry
{
    Value key;
    Value value;
};

struct HashTable
{
    int count;
    int capacity;
    HashEntry* entries;
};

struct Object
{
    ObjType type;
    bool mark;
    bool definitelyreal;
    Object* sibling;
};

struct ObjString
{
    Object obj;
    int length;
    int utf8length;
    bool isascii;
    uint32_t hash;
    char* chars;
};

struct ObjUpvalue
{
    Object obj;
    Value closed;
    Value* location;
    ObjUpvalue* next;
};

struct ObjModule
{
    Object obj;
    bool imported;
    HashTable values;
    char* name;
    char* file;
    void* preloader;
    void* unloader;
    void* handle;
};

struct ObjFunction
{
    Object obj;
    FuncType type;
    int arity;
    int upvaluecount;
    bool isvariadic;
    BinaryBlob blob;
    ObjString* name;
    ObjModule* module;
};

struct ObjClosure
{
    Object obj;
    int upvaluecount;
    ObjFunction* fnptr;
    ObjUpvalue** upvalues;
};

struct ObjClass
{
    Object obj;
    Value initializer;
    HashTable properties;
    HashTable staticproperties;
    HashTable methods;
    ObjString* name;
    ObjClass* superclass;
};

struct ObjInstance
{
    Object obj;
    HashTable properties;
    ObjClass* klass;
};

struct ObjBoundMethod
{
    Object obj;
    Value receiver;
    ObjClosure* method;
};

struct ObjNativeFunction
{
    Object obj;
    FuncType type;
    const char* name;
    NativeCallbackFunc natfn;
};

struct ObjArray
{
    Object obj;
    ValArray items;
};

struct ObjRange
{
    Object obj;
    int lower;
    int upper;
    int range;
};

struct ObjBytes
{
    Object obj;
    ByteArray bytes;
};

struct ObjDict
{
    Object obj;
    ValArray names;
    HashTable items;
};

struct ObjFile
{
    Object obj;
    bool isopen;
    FILE* file;
    ObjString* mode;
    ObjString* path;
};

struct ObjSwitch
{
    Object obj;
    int defaultjump;
    int exitjump;
    HashTable table;
};

struct ObjPointer
{
    Object obj;
    void* pointer;
    const char* name;
    PtrFreeFunc fnptrfree;
};

struct ExceptionFrame
{
    uint16_t address;
    uint16_t finallyaddress;
    ObjClass* klass;
};

struct CallFrame
{
    ObjClosure* closure;
    uint8_t* ip;
    Value* slots;
    int handlerscount;
    ExceptionFrame handlers[MAX_EXCEPTION_HANDLERS];
};

struct VMState
{
    bool allowgc;
    CallFrame frames[FRAMES_MAX];
    int framecount;
    BinaryBlob* blob;
    uint8_t* ip;
    Value stack[STACK_MAX];
    Value* stacktop;
    ObjUpvalue* openupvalues;
    size_t objectcount;
    Object* objectlinks;
    AstCompiler* compiler;
    ObjClass* exceptionclass;
    // gc
    int graycount;
    int graycapacity;
    int gcprotected;
    Object** graystack;
    size_t bytesallocated;
    size_t nextgc;
    // objects tracker
    HashTable modules;
    HashTable strings;
    HashTable globals;
    // object public methods
    ObjClass* classobjobject;
    ObjClass* classobjstring;
    ObjClass* classobjlist;
    ObjClass* classobjdict;
    ObjClass* classobjfile;
    ObjClass* classobjbytes;
    ObjClass* classobjrange;
    char** stdargs;
    int stdargscount;
    // boolean flags
    bool isrepl;
    // for switching through the command line args...
    bool shoulddebugstack;
    bool shouldprintbytecode;
};

struct AstToken
{
    TokType type;
    const char* start;
    int length;
    int line;
};

struct AstScanner
{
    const char* start;
    const char* current;
    int line;
    int interpolatingcount;
    int interpolating[MAX_INTERPOLATION_NESTING];
};

struct AstLocal
{
    AstToken name;
    int depth;
    bool iscaptured;
};

struct Upvalue
{
    uint16_t index;
    bool islocal;
};

struct AstCompiler
{
    AstCompiler* enclosing;
    // current function
    ObjFunction* currfunc;
    FuncType type;
    AstLocal locals[UINT8_COUNT];
    int localcount;
    Upvalue upvalues[UINT8_COUNT];
    int scopedepth;
    int handlercount;
};

struct AstClassCompiler
{
    AstClassCompiler* enclosing;
    AstToken name;
    bool hassuperclass;
};

struct AstParser
{
    AstScanner* scanner;
    VMState* vm;
    AstToken current;
    AstToken previous;
    bool haderror;
    bool panicmode;
    int blockcount;
    bool isreturning;
    bool istrying;
    bool replcanecho;
    AstClassCompiler* currentclass;
    const char* currentfile;
    // used for tracking loops for the continue statement...
    int innermostloopstart;
    int innermostloopscopedepth;
    ObjModule* module;
};

struct AstRule
{
    bparseprefixfn prefix;
    bparseinfixfn infix;
    AstPrecedence precedence;
};

struct DynArray
{
    void* buffer;
    int length;
};

struct BProcess
{
    int pid;
};

struct BProcessShared
{
    char* format;
    char* getformat;
    unsigned char* bytes;
    int formatlength;
    int getformatlength;
    int length;
    bool locked;
};

#include "prot.inc"


