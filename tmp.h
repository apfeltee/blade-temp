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

#include <sys/stat.h>
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
#include "xxhash.h"
#include "ktre.h"

#define PCRE2_STATIC
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

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
        #if defined(__i386__) defined(BIT_ZERO_ON_RIGHT) || defined(_M_IX86) || defined(_M_X64) || defined(_M_IA64) || defined(_M_ARM) || defined(_WIN32)
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

#define ENFORCE_ARG_COUNT(name, d)                                            \
    if(argcount != d)                                                         \
    {                                                                         \
        RETURN_ERROR(#name "() expects %d arguments, %d given", d, argcount); \
    }

#define ENFORCE_MIN_ARG(name, d)                                                         \
    if(argcount < d)                                                                     \
    {                                                                                    \
        RETURN_ERROR(#name "() expects minimum of %d arguments, %d given", d, argcount); \
    }

#define ENFORCE_ARG_RANGE(name, low, up)                                                           \
    if(argcount < (low) || argcount > (up))                                                        \
    {                                                                                              \
        RETURN_ERROR(#name "() expects between %d and %d arguments, %d given", low, up, argcount); \
    }

#define ENFORCE_ARG_TYPE(name, i, type)                                                                                     \
    if(!type(args[i]))                                                                                                      \
    {                                                                                                                       \
        RETURN_ERROR(#name "() expects argument %d as " NORMALIZE(type) ", %s given", (i) + 1, bl_value_typename(args[i])); \
    }

#define EXCLUDE_ARG_TYPE(methodname, arg_type, index)                                                                       \
    if(arg_type(args[index]))                                                                                               \
    {                                                                                                                       \
        RETURN_ERROR("invalid type %s() as argument %d in %s()", bl_value_typename(args[index]), (index) + 1, #methodname); \
    }

#define METHOD_OVERRIDE(override, i)                                                                                  \
    do                                                                                                                \
    {                                                                                                                 \
        if(bl_value_isinstance(args[0]))                                                                              \
        {                                                                                                             \
            ObjInstance* instance = AS_INSTANCE(args[0]);                                                             \
            if(bl_instance_invokefromclass(vm, instance->klass, bl_string_copystring(vm, "@" #override, (i) + 1), 0)) \
            {                                                                                                         \
                args[-1] = TRUE_VAL;                                                                                  \
                return false;                                                                                         \
            }                                                                                                         \
        }                                                                                                             \
    } while(0);

#define REGEX_COMPILATION_ERROR(re, errornumber, erroroffset)                                               \
    if((re) == NULL)                                                                                        \
    {                                                                                                       \
        PCRE2_UCHAR8 buffer[256];                                                                           \
        pcre2_get_error_message_8(errornumber, buffer, sizeof(buffer));                                     \
        RETURN_ERROR("regular expression compilation failed at offset %d: %s", (int)(erroroffset), buffer); \
    }

#define REGEX_ASSERTION_ERROR(re, matchdata, ovector)                                      \
    if((ovector)[0] > (ovector)[1])                                                        \
    {                                                                                      \
        RETURN_ERROR("match aborted: regular expression used \\K in an assertion %.*s to " \
                     "set match start after its end.",                                     \
                     (int)((ovector)[0] - (ovector)[1]), (char*)(subject + (ovector)[1])); \
        pcre2_match_data_free(matchdata);                                                  \
        pcre2_code_free(re);                                                               \
        RETURN_EMPTY;                                                                      \
    }

#define REGEX_ERR(message, result)                                     \
    do                                                                 \
    {                                                                  \
        PCRE2_UCHAR error[255];                                        \
        if(pcre2_get_error_message(result, error, 255))                \
        {                                                              \
            RETURN_ERROR("RegexError: (%d) %s", result, (char*)error); \
        }                                                              \
        RETURN_ERROR("RegexError: %s", message);                       \
    } while(0)

#define REGEX_RC_ERROR() REGEX_ERR("%d", rc);

#define GET_REGEX_COMPILE_OPTIONS(string, regexshowerror)                                             \
    uint32_t compileoptions = bl_helper_objstringisregex(string);                                     \
    if((regexshowerror) && (int)compileoptions == -1)                                                 \
    {                                                                                                 \
        RETURN_ERROR("RegexError: Invalid regex");                                                    \
    }                                                                                                 \
    else if((regexshowerror) && (int)compileoptions > 1000000)                                        \
    {                                                                                                 \
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
#define GC_STRING(o) OBJ_VAL(bl_mem_gcprotect(vm, (Object*)bl_string_copystring(vm, (const char*)(o), (int)strlen(o))))
#define GC_L_STRING(o, l) OBJ_VAL(bl_mem_gcprotect(vm, (Object*)bl_string_copystring(vm, (const char*)(o), (l))))

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
#define STRING_VAL(val) OBJ_VAL(bl_string_copystring(vm, val, (int)strlen(val)))
#define STRING_L_VAL(val, l) OBJ_VAL(bl_string_copystring(vm, val, l))
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

#define runtime_error(...)                              \
    if(!bl_vm_throwexception(vm, false, ##__VA_ARGS__)) \
    {                                                   \
        EXIT_VM();                                      \
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
    OP_GREATER,
    OP_LESS,
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
    OP_AND,
    OP_OR,
    OP_XOR,
    OP_LSHIFT,
    OP_RSHIFT,
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
    HashTable methodsstring;
    HashTable methodslist;
    HashTable methodsdict;
    HashTable methodsfile;
    HashTable methodsbytes;
    HashTable methodsrange;
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
    pid_t pid;
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

#if defined(__TINYC__)
int __dso_handle;
#endif

static bool bl_object_isstdfile(ObjFile* file)
{
    return (file->mode->length == 0);
}

static void bl_state_addmodule(VMState* vm, ObjModule* module)
{
    size_t len;
    const char* cs;
    cs = module->file;
    len = strlen(cs);
    bl_hashtable_set(vm, &vm->modules, OBJ_VAL(bl_string_copystring(vm, cs, (int)len)), OBJ_VAL(module));
    if(vm->framecount == 0)
    {
        bl_hashtable_set(vm, &vm->globals, STRING_VAL(module->name), OBJ_VAL(module));
    }
    else
    {
        cs = module->name;
        bl_hashtable_set(vm, &vm->frames[vm->framecount - 1].closure->fnptr->module->values, OBJ_VAL(bl_string_copystring(vm, cs, (int)len)), OBJ_VAL(module));
    }
}

static Object* bl_mem_gcprotect(VMState* vm, Object* object)
{
    bl_vm_pushvalue(vm, OBJ_VAL(object));
    vm->gcprotected++;
    return object;
}

static void bl_mem_gcclearprotect(VMState* vm)
{
    if(vm->gcprotected > 0)
    {
        vm->stacktop -= vm->gcprotected;
    }
    vm->gcprotected = 0;
}

#define BLADE_PATH_SEPARATOR "/"
#if defined(__linux__) || defined(__CYGWIN__) || defined(__MINGW32_MAJOR_VERSION)
    #define PROC_SELF_EXE "/proc/self/exe"
#endif

// returns the number of bytes contained in a unicode character
int bl_util_utf8numbytes(int value)
{
    if(value < 0)
    {
        return -1;
    }
    if(value <= 0x7f)
    {
        return 1;
    }
    if(value <= 0x7ff)
    {
        return 2;
    }
    if(value <= 0xffff)
    {
        return 3;
    }
    if(value <= 0x10ffff)
    {
        return 4;
    }
    return 0;
}

char* bl_util_utf8encode(unsigned int code)
{
    int count;
    char* chars;
    count = bl_util_utf8numbytes((int)code);
    if(count > 0)
    {
        chars = (char*)calloc((size_t)count + 1, sizeof(char));
        if(chars != NULL)
        {
            if(code <= 0x7F)
            {
                chars[0] = (char)(code & 0x7F);
                chars[1] = '\0';
            }
            else if(code <= 0x7FF)
            {
                // one continuation byte
                chars[1] = (char)(0x80 | (code & 0x3F));
                code = (code >> 6);
                chars[0] = (char)(0xC0 | (code & 0x1F));
            }
            else if(code <= 0xFFFF)
            {
                // two continuation bytes
                chars[2] = (char)(0x80 | (code & 0x3F));
                code = (code >> 6);
                chars[1] = (char)(0x80 | (code & 0x3F));
                code = (code >> 6);
                chars[0] = (char)(0xE0 | (code & 0xF));
            }
            else if(code <= 0x10FFFF)
            {
                // three continuation bytes
                chars[3] = (char)(0x80 | (code & 0x3F));
                code = (code >> 6);
                chars[2] = (char)(0x80 | (code & 0x3F));
                code = (code >> 6);
                chars[1] = (char)(0x80 | (code & 0x3F));
                code = (code >> 6);
                chars[0] = (char)(0xF0 | (code & 0x7));
            }
            else
            {
                // unicode replacement character
                chars[2] = (char)0xEF;
                chars[1] = (char)0xBF;
                chars[0] = (char)0xBD;
            }
            return chars;
        }
    }
    return NULL;
}

int bl_util_utf8decodenumbytes(uint8_t byte)
{
    // If the byte starts with 10xxxxx, it's the middle of a UTF-8 sequence, so
    // don't count it at all.
    if((byte & 0xc0) == 0x80)
    {
        return 0;
    }
    // The first byte's high bits tell us how many bytes are in the UTF-8
    // sequence.
    if((byte & 0xf8) == 0xf0)
    {
        return 4;
    }
    if((byte & 0xf0) == 0xe0)
    {
        return 3;
    }
    if((byte & 0xe0) == 0xc0)
    {
        return 2;
    }
    return 1;
}

int bl_util_utf8decode(const uint8_t* bytes, uint32_t length)
{
    // Single byte (i.e. fits in ASCII).
    if(*bytes <= 0x7f)
    {
        return *bytes;
    }
    int value;
    uint32_t remainingbytes;
    if((*bytes & 0xe0) == 0xc0)
    {
        // Two byte sequence: 110xxxxx 10xxxxxx.
        value = *bytes & 0x1f;
        remainingbytes = 1;
    }
    else if((*bytes & 0xf0) == 0xe0)
    {
        // Three byte sequence: 1110xxxx	 10xxxxxx 10xxxxxx.
        value = *bytes & 0x0f;
        remainingbytes = 2;
    }
    else if((*bytes & 0xf8) == 0xf0)
    {
        // Four byte sequence: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx.
        value = *bytes & 0x07;
        remainingbytes = 3;
    }
    else
    {
        // Invalid UTF-8 sequence.
        return -1;
    }
    // Don't read past the end of the buffer on truncated UTF-8.
    if(remainingbytes > length - 1)
    {
        return -1;
    }
    while(remainingbytes > 0)
    {
        bytes++;
        remainingbytes--;
        // Remaining bytes must be of form 10xxxxxx.
        if((*bytes & 0xc0) != 0x80)
        {
            return -1;
        }
        value = value << 6 | (*bytes & 0x3f);
    }
    return value;
}

char* bl_util_appendstring(char* old, const char* newstr)
{
    char* out;
    size_t oldlen;
    size_t newlen;
    size_t outlen;
    // quick exit...
    if(newstr == NULL)
    {
        return old;
    }
    // find the size of the string to allocate
    oldlen = strlen(old);
    newlen = strlen(newstr);
    outlen = oldlen + newlen;
    // allocate a pointer to the new string
    out = (char*)realloc((void*)old, outlen + 1);
    // concat both strings and return
    if(out != NULL)
    {
        memcpy(out + oldlen, newstr, newlen);
        out[outlen] = '\0';
        return out;
    }
    return old;
}

int bl_util_utf8length(char* s)
{
    int len = 0;
    for(; *s; ++s)
    {
        if((*s & 0xC0) != 0x80)
        {
            ++len;
        }
    }
    return len;
}

// returns a pointer to the beginning of the pos'th utf8 codepoint
// in the buffer at s
char* bl_util_utf8index(char* s, int pos)
{
    ++pos;
    for(; *s; ++s)
    {
        if((*s & 0xC0) != 0x80)
        {
            --pos;
        }
        if(pos == 0)
        {
            return s;
        }
    }
    return NULL;
}

// converts codepoint indexes start and end to byte offsets in the buffer at s
void bl_util_utf8slice(char* s, int* start, int* end)
{
    char* p;
    p = bl_util_utf8index(s, *start);
    *start = p != NULL ? (int)(p - s) : -1;
    p = bl_util_utf8index(s, *end);
    *end = p != NULL ? (int)(p - s) : (int)strlen(s);
}

char* bl_util_readfile(const char* path)
{
    FILE* fp = fopen(path, "rb");
    // file not readable (maybe due to permission)
    if(fp == NULL)
    {
        return NULL;
    }
    fseek(fp, 0L, SEEK_END);
    size_t filesize = ftell(fp);
    rewind(fp);
    char* buffer = (char*)malloc(filesize + 1);
    // the system might not have enough memory to read the file.
    if(buffer == NULL)
    {
        fclose(fp);
        return NULL;
    }
    size_t bytesread = fread(buffer, sizeof(char), filesize, fp);
    // if we couldn't read the entire file
    if(bytesread < filesize)
    {
        fclose(fp);
        free(buffer);
        return NULL;
    }
    buffer[bytesread] = '\0';
    fclose(fp);
    return buffer;
}

char* bl_util_getexepath()
{
    char rawpath[PATH_MAX];
    ssize_t readlength;
    if((readlength = readlink(PROC_SELF_EXE, rawpath, sizeof(rawpath))) > -1 && readlength < PATH_MAX)
    {
        return strdup(rawpath);
    }
    return strdup("");
}

char* bl_util_getexedir()
{
    return dirname(bl_util_getexepath());
}

char* bl_util_mergepaths(const char* a, const char* b)
{
    int lenb;
    int lena;
    char* finalpath;
    finalpath = (char*)calloc(1, sizeof(char));
    memset(finalpath, 0, 1);
    // by checking b first, we guarantee that b is neither NULL nor
    // empty by the time we are checking a so that we can return a
    // duplicate of b
    lenb = (int)strlen(b);
    if(b == NULL || lenb == 0)
    {
        free(finalpath);
        return strdup(a);// just in case a is const char*
    }
    lena = strlen(a);
    if(a == NULL || lena == 0)
    {
        free(finalpath);
        return strdup(b);// just in case b is const char*
    }
    finalpath = bl_util_appendstring(finalpath, a);
    if(!(lenb == 2 && b[0] == '.' && b[1] == 'b'))
    {
        finalpath = bl_util_appendstring(finalpath, BLADE_PATH_SEPARATOR);
    }
    finalpath = bl_util_appendstring(finalpath, b);
    return finalpath;
}

bool bl_util_fileexists(char* filepath)
{
    return access(filepath, F_OK) == 0;
}

char* bl_util_getbladefilename(const char* filename)
{
    return bl_util_mergepaths(filename, BLADE_EXTENSION);
}

char* bl_util_resolveimportpath(char* modulename, const char* currentfile, bool isrelative)
{
    char* bladefilename = bl_util_getbladefilename(modulename);
    // check relative to the current file...
    char* filedirectory = dirname((char*)strdup(currentfile));
    // fixing last path / if exists (looking at windows)...
    int filedirectorylength = (int)strlen(filedirectory);
    if(filedirectory[filedirectorylength - 1] == '\\')
    {
        filedirectory[filedirectorylength - 1] = '\0';
    }
    // search system library if we are not looking for a relative module.
    if(!isrelative)
    {
        // firstly, search the local vendor directory for a matching module
        char* rootdir = getcwd(NULL, 0);
        // fixing last path / if exists (looking at windows)...
        int rootdirlength = (int)strlen(rootdir);
        if(rootdir[rootdirlength - 1] == '\\')
        {
            rootdir[rootdirlength - 1] = '\0';
        }
        char* vendorfile = bl_util_mergepaths(bl_util_mergepaths(rootdir, LOCAL_PACKAGES_DIRECTORY LOCAL_SRC_DIRECTORY), bladefilename);
        if(bl_util_fileexists(vendorfile))
        {
            // stop a core library from importing itself
            char* path1 = realpath(vendorfile, NULL);
            char* path2 = realpath(currentfile, NULL);
            if(path1 != NULL)
            {
                if(path2 == NULL || memcmp(path1, path2, (int)strlen(path2)) != 0)
                {
                    return path1;
                }
            }
        }
        // or a matching package
        char* vendorindexfile = bl_util_mergepaths(bl_util_mergepaths(bl_util_mergepaths(rootdir, LOCAL_PACKAGES_DIRECTORY LOCAL_SRC_DIRECTORY), modulename),
                                                   LIBRARY_DIRECTORY_INDEX BLADE_EXTENSION);
        if(bl_util_fileexists(vendorindexfile))
        {
            // stop a core library from importing itself
            char* path1 = realpath(vendorindexfile, NULL);
            char* path2 = realpath(currentfile, NULL);
            if(path1 != NULL)
            {
                if(path2 == NULL || memcmp(path1, path2, (int)strlen(path2)) != 0)
                {
                    return path1;
                }
            }
        }
        // then, check in blade's default locations
        char* exedir = bl_util_getexedir();
        char* bladedirectory = bl_util_mergepaths(exedir, LIBRARY_DIRECTORY);
        // check blade libs directory for a matching module...
        char* libraryfile = bl_util_mergepaths(bladedirectory, bladefilename);
        if(bl_util_fileexists(libraryfile))
        {
            // stop a core library from importing itself
            char* path1 = realpath(libraryfile, NULL);
            char* path2 = realpath(currentfile, NULL);
            if(path1 != NULL)
            {
                if(path2 == NULL || memcmp(path1, path2, (int)strlen(path2)) != 0)
                {
                    return path1;
                }
            }
        }
        // check blade libs directory for a matching package...
        char* libraryindexfile = bl_util_mergepaths(bl_util_mergepaths(bladedirectory, modulename), bl_util_getbladefilename(LIBRARY_DIRECTORY_INDEX));
        if(bl_util_fileexists(libraryindexfile))
        {
            char* path1 = realpath(libraryindexfile, NULL);
            char* path2 = realpath(currentfile, NULL);
            if(path1 != NULL)
            {
                if(path2 == NULL || memcmp(path1, path2, (int)strlen(path2)) != 0)
                {
                    return path1;
                }
            }
        }
        // check blade vendor directory installed module...
        char* bladepackagedirectory = bl_util_mergepaths(exedir, PACKAGES_DIRECTORY);
        char* packagefile = bl_util_mergepaths(bladepackagedirectory, bladefilename);
        if(bl_util_fileexists(packagefile))
        {
            char* path1 = realpath(packagefile, NULL);
            char* path2 = realpath(currentfile, NULL);
            if(path1 != NULL)
            {
                if(path2 == NULL || memcmp(path1, path2, (int)strlen(path2)) != 0)
                {
                    return path1;
                }
            }
        }
        // check blade vendor directory installed package...
        char* packageindexfile = bl_util_mergepaths(bl_util_mergepaths(bladepackagedirectory, modulename), LIBRARY_DIRECTORY_INDEX BLADE_EXTENSION);
        if(bl_util_fileexists(packageindexfile))
        {
            char* path1 = realpath(packageindexfile, NULL);
            char* path2 = realpath(currentfile, NULL);
            if(path1 != NULL)
            {
                if(path2 == NULL || memcmp(path1, path2, (int)strlen(path2)) != 0)
                {
                    return path1;
                }
            }
        }
    }
    else
    {
        // otherwise, search the relative path for a matching module
        char* relativefile = bl_util_mergepaths(filedirectory, bladefilename);
        if(bl_util_fileexists(relativefile))
        {
            // stop a user module from importing itself
            char* path1 = realpath(relativefile, NULL);
            char* path2 = realpath(currentfile, NULL);
            if(path1 != NULL)
            {
                if(path2 == NULL || memcmp(path1, path2, (int)strlen(path2)) != 0)
                {
                    return path1;
                }
            }
        }
        // or a matching package
        char* relativeindexfile = bl_util_mergepaths(bl_util_mergepaths(filedirectory, modulename), bl_util_getbladefilename(LIBRARY_DIRECTORY_INDEX));
        if(bl_util_fileexists(relativeindexfile))
        {
            char* path1 = realpath(relativeindexfile, NULL);
            char* path2 = realpath(currentfile, NULL);
            if(path1 != NULL)
            {
                if(path2 == NULL || memcmp(path1, path2, (int)strlen(path2)) != 0)
                {
                    return path1;
                }
            }
        }
    }
    return NULL;
}

char* bl_util_getrealfilename(char* path)
{
    return basename(path);
}

static uint32_t bl_util_hashbits(uint64_t hash)
{
    // From v8's ComputeLongHash() which in turn cites:
    // Thomas Wang, Integer Hash Functions.
    // http://www.concentric.net/~Ttwang/tech/inthash.htm
    hash = ~hash + (hash << 18);// hash = (hash << 18) - hash - 1;
    hash = hash ^ (hash >> 31);
    hash = hash * 21;// hash = (hash + (hash << 2)) + (hash << 4);
    hash = hash ^ (hash >> 11);
    hash = hash + (hash << 6);
    hash = hash ^ (hash >> 22);
    return (uint32_t)(hash & 0x3fffffff);
}

uint32_t bl_util_hashdouble(double value)
{
    union bdoubleunion
    {
        uint64_t bits;
        double num;
    };
    union bdoubleunion bits;
    bits.num = value;
    return bl_util_hashbits(bits.bits);
}

uint32_t bl_util_hashstring(const char* key, int length)
{
    /*
    uint32_t hash = 2166136261u;
    const char* be = key + length;
    while(key < be)
    {
        hash = (hash ^ *key++) * 16777619;
    }
    return hash;
    // return siphash24(127, 255, key, length);
    */
    return XXH3_64bits(key, length);
}

static uint16_t bl_util_reverseint16(uint16_t arg)
{
    return ((arg & 0xFF) << 8) | ((arg >> 8) & 0xFF);
}

static uint32_t bl_util_reverseint32(uint32_t arg)
{
    uint32_t result;
    result = ((arg & 0xFF) << 24) | ((arg & 0xFF00) << 8) | ((arg >> 8) & 0xFF00) | ((arg >> 24) & 0xFF);
    return result;
}

static uint64_t bl_util_reverseint64(uint64_t arg)
{
    union swaptag
    {
        uint64_t i;
        uint32_t ul[2];
    } tmp, result;

    tmp.i = arg;
    result.ul[0] = bl_util_reverseint32(tmp.ul[1]);
    result.ul[1] = bl_util_reverseint32(tmp.ul[0]);
    return result.i;
}

static void bl_util_copyfloat(int islittleendian, void* dst, float f)
{
    union floattag
    {
        float f;
        uint32_t i;
    } m;

    m.f = f;
#if IS_BIG_ENDIAN
    if(islittleendian)
    {
#else
    if(!islittleendian)
    {
#endif
        m.i = bl_util_reverseint32(m.i);
    }

    memcpy(dst, &m.f, sizeof(float));
}

static void bl_util_copydouble(int islittleendian, void* dst, double d)
{
    union doubletag
    {
        double d;
        uint64_t i;
    } m;

    m.d = d;
#if IS_BIG_ENDIAN
    if(islittleendian)
    {
#else
    if(!islittleendian)
    {
#endif
        m.i = bl_util_reverseint64(m.i);
    }

    memcpy(dst, &m.d, sizeof(double));
}

static char* bl_util_ulongtobuffer(char* buf, long num)
{
    *buf = '\0';
    do
    {
        *--buf = (char)((char)(num % 10) + '0');
        num /= 10;
    } while(num > 0);
    return buf;
}

static float bl_util_parsefloat(int islittleendian, void* src)
{
    union floattag
    {
        float f;
        uint32_t i;
    } m;

    memcpy(&m.i, src, sizeof(float));
#if IS_BIG_ENDIAN
    if(islittleendian)
#else
    if(!islittleendian)
#endif
    {
        m.i = bl_util_reverseint32(m.i);
    }

    return m.f;
}

static double bl_util_parsedouble(int islittleendian, void* src)
{
    union doubletag
    {
        double d;
        uint64_t i;
    } m;

    memcpy(&m.i, src, sizeof(double));
#if IS_BIG_ENDIAN
    if(islittleendian)
    {
#else
    if(!islittleendian)
    {
#endif
        m.i = bl_util_reverseint64(m.i);
    }

    return m.d;
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
    bl_mem_marktable(vm, &vm->methodsstring);
    bl_mem_marktable(vm, &vm->methodsbytes);
    bl_mem_marktable(vm, &vm->methodsfile);
    bl_mem_marktable(vm, &vm->methodslist);
    bl_mem_marktable(vm, &vm->methodsdict);
    bl_mem_marktable(vm, &vm->methodsrange);
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
    /*
        tin_gcmem_vmmarkroots(vm);
    tin_gcmem_vmtracerefs(vm);
    tin_strreg_remwhite(vm->state);
    tin_gcmem_vmsweep(vm);
    vm->state->gcnext = vm->state->gcbytescount * TIN_GC_HEAP_GROW_FACTOR;
    */
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
                return OBJ_VAL(bl_string_copystring(vm, string->chars, string->length));
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

#define RETURN_EMPTY        \
    {                       \
        args[-1] = NIL_VAL; \
        return false;       \
    }

#define RETURN_ERROR(...)                               \
    {                                                   \
        bl_vm_popvaluen(vm, argcount);                  \
        bl_vm_throwexception(vm, false, ##__VA_ARGS__); \
        args[-1] = FALSE_VAL;                           \
        return false;                                   \
    }

#define RETURN_BOOL(v)          \
    {                           \
        args[-1] = BOOL_VAL(v); \
        return true;            \
    }
#define RETURN_TRUE          \
    {                        \
        args[-1] = TRUE_VAL; \
        return true;         \
    }
#define RETURN_FALSE          \
    {                         \
        args[-1] = FALSE_VAL; \
        return true;          \
    }
#define RETURN_NUMBER(v)          \
    {                             \
        args[-1] = NUMBER_VAL(v); \
        return true;              \
    }
#define RETURN_OBJ(v)          \
    {                          \
        args[-1] = OBJ_VAL(v); \
        return true;           \
    }

#define RETURN_L_STRING(v, l)                               \
    {                                                       \
        args[-1] = OBJ_VAL(bl_string_copystring(vm, v, l)); \
        return true;                                        \
    }
#define RETURN_T_STRING(v, l)                               \
    {                                                       \
        args[-1] = OBJ_VAL(bl_string_takestring(vm, v, l)); \
        return true;                                        \
    }
#define RETURN_TT_STRING(v)                                              \
    {                                                                    \
        args[-1] = OBJ_VAL(bl_string_takestring(vm, v, (int)strlen(v))); \
        return true;                                                     \
    }
#define RETURN_VALUE(v) \
    {                   \
        args[-1] = v;   \
        return true;    \
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
    char start = string->chars[0];
    bool matchfound = false;
    uint32_t coptions = 0;// pcre2 options
    for(int i = 1; i < string->length; i++)
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
                    /* In  PCRE,  by  default, \d, \D, \s, \S, \w, and \W recognize only
         ASCII characters, even in UTF-8 mode. However, this can be changed by
         setting the PCRE2_UCP option. */
#ifdef PCRE2_UCP
                    coptions |= PCRE2_UCP;
#endif
                    break;
                case 'J':
                    coptions |= PCRE2_DUPNAMES;
                    break;
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

void bl_array_push(VMState* vm, ObjArray* list, Value value)
{
    bl_valarray_push(vm, &list->items, value);
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

#define ENFORCE_VALID_DICT_KEY(name, index)          \
    EXCLUDE_ARG_TYPE(name, bl_value_isarray, index); \
    EXCLUDE_ARG_TYPE(name, bl_value_isdict, index);  \
    EXCLUDE_ARG_TYPE(name, bl_value_isfile, index);

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

ObjPointer* bl_dict_makeptr(VMState* vm, void* pointer)
{
    ObjPointer* ptr = (ObjPointer*)bl_object_allocobject(vm, sizeof(ObjPointer), OBJ_PTR);
    ptr->pointer = pointer;
    ptr->name = "<void *>";
    ptr->fnptrfree = NULL;
    return ptr;
}

ObjDict* bl_object_makedict(VMState* vm)
{
    ObjDict* dict = (ObjDict*)bl_object_allocobject(vm, sizeof(ObjDict), OBJ_DICT);
    bl_valarray_init(&dict->names);
    bl_hashtable_init(&dict->items);
    return dict;
}

ObjArray* bl_object_makelist(VMState* vm)
{
    ObjArray* list = (ObjArray*)bl_object_allocobject(vm, sizeof(ObjArray), OBJ_ARRAY);
    bl_valarray_init(&list->items);
    return list;
}

ObjModule* bl_object_makemodule(VMState* vm, char* name, char* file)
{
    ObjModule* module = (ObjModule*)bl_object_allocobject(vm, sizeof(ObjModule), OBJ_MODULE);
    bl_hashtable_init(&module->values);
    module->name = name;
    module->file = file;
    module->unloader = NULL;
    module->preloader = NULL;
    module->handle = NULL;
    module->imported = false;
    return module;
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

ObjClass* bl_object_makeclass(VMState* vm, ObjString* name)
{
    ObjClass* klass = (ObjClass*)bl_object_allocobject(vm, sizeof(ObjClass), OBJ_CLASS);
    klass->name = name;
    bl_hashtable_init(&klass->properties);
    bl_hashtable_init(&klass->staticproperties);
    bl_hashtable_init(&klass->methods);
    klass->initializer = EMPTY_VAL;
    klass->superclass = NULL;
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

ObjString* bl_string_copystring(VMState* vm, const char* chars, int length)
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
            return "module";
        case OBJ_BYTES:
            return "bytes";
        case OBJ_RANGE:
            return "range";
        case OBJ_FILE:
            return "file";
        case OBJ_DICT:
            return "dictionary";
        case OBJ_ARRAY:
            return "list";
        case OBJ_CLASS:
            return "class";
        case OBJ_SCRIPTFUNCTION:
        case OBJ_NATIVEFUNCTION:
        case OBJ_CLOSURE:
        case OBJ_BOUNDFUNCTION:
            return "function";
        case OBJ_INSTANCE:
            return ((ObjInstance*)object)->klass->name->chars;
        case OBJ_STRING:
            return "string";
            //
        case OBJ_PTR:
            return "pointer";
        case OBJ_SWITCH:
            return "switch";
        default:
            return "unknown";
    }
}

bool bl_vmdo_dictgetindex(VMState* vm, ObjDict* dict, bool willassign)
{
    Value index = bl_vm_peekvalue(vm, 0);
    Value result;
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

bool objfn_list_length(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(length, 0);
    RETURN_NUMBER(AS_LIST(METHOD_OBJECT)->items.count);
}

bool objfn_list_append(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(append, 1);
    bl_array_push(vm, AS_LIST(METHOD_OBJECT), args[0]);
    return bl_value_returnempty(vm, args);
}

bool objfn_list_clear(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(clear, 0);
    bl_valarray_free(vm, &AS_LIST(METHOD_OBJECT)->items);
    return bl_value_returnempty(vm, args);
}

bool objfn_list_clone(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(clone, 0);
    ObjArray* list = AS_LIST(METHOD_OBJECT);
    RETURN_OBJ(bl_array_copy(vm, list, 0, list->items.count));
}

bool objfn_list_count(VMState* vm, int argcount, Value* args)
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

bool objfn_list_extend(VMState* vm, int argcount, Value* args)
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

bool objfn_list_indexof(VMState* vm, int argcount, Value* args)
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

bool objfn_list_insert(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(insert, 2);
    ENFORCE_ARG_TYPE(insert, 1, bl_value_isnumber);
    ObjArray* list = AS_LIST(METHOD_OBJECT);
    int index = (int)AS_NUMBER(args[1]);
    bl_valarray_insert(vm, &list->items, args[0], index);
    return bl_value_returnempty(vm, args);
    ;
}

bool objfn_list_pop(VMState* vm, int argcount, Value* args)
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

bool objfn_list_shift(VMState* vm, int argcount, Value* args)
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

bool objfn_list_removeat(VMState* vm, int argcount, Value* args)
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

bool objfn_list_remove(VMState* vm, int argcount, Value* args)
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

bool objfn_list_reverse(VMState* vm, int argcount, Value* args)
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

bool objfn_list_sort(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(sort, 0);
    ObjArray* list = AS_LIST(METHOD_OBJECT);
    bl_value_sortbubblesort(list->items.values, list->items.count);
    return bl_value_returnempty(vm, args);
    ;
}

bool objfn_list_contains(VMState* vm, int argcount, Value* args)
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

bool objfn_list_delete(VMState* vm, int argcount, Value* args)
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

bool objfn_list_first(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(first, 0);
    ObjArray* list = AS_LIST(METHOD_OBJECT);
    if(list->items.count > 0)
    {
        RETURN_VALUE(list->items.values[0]);
    }
    return bl_value_returnnil(vm, args);
}

bool objfn_list_last(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(last, 0);
    ObjArray* list = AS_LIST(METHOD_OBJECT);
    if(list->items.count > 0)
    {
        RETURN_VALUE(list->items.values[list->items.count - 1]);
    }
    return bl_value_returnnil(vm, args);
}

bool objfn_list_isempty(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(isempty, 0);
    RETURN_BOOL(AS_LIST(METHOD_OBJECT)->items.count == 0);
}

bool objfn_list_take(VMState* vm, int argcount, Value* args)
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

bool objfn_list_get(VMState* vm, int argcount, Value* args)
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

bool objfn_list_compact(VMState* vm, int argcount, Value* args)
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

bool objfn_list_unique(VMState* vm, int argcount, Value* args)
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

bool objfn_list_zip(VMState* vm, int argcount, Value* args)
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

bool objfn_list_todict(VMState* vm, int argcount, Value* args)
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

bool objfn_list_iter(VMState* vm, int argcount, Value* args)
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

bool objfn_list_itern(VMState* vm, int argcount, Value* args)
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

bool objfn_string_length(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(length, 0);
    ObjString* string = AS_STRING(METHOD_OBJECT);
    RETURN_NUMBER(string->isascii ? string->length : string->utf8length);
}

bool objfn_string_upper(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(upper, 0);
    char* string = (char*)strdup(AS_C_STRING(METHOD_OBJECT));
    for(char* p = string; *p; p++)
    {
        *p = toupper(*p);
    }
    RETURN_L_STRING(string, AS_STRING(METHOD_OBJECT)->length);
}

bool objfn_string_lower(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(lower, 0);
    char* string = (char*)strdup(AS_C_STRING(METHOD_OBJECT));
    for(char* p = string; *p; p++)
    {
        *p = tolower(*p);
    }
    RETURN_L_STRING(string, AS_STRING(METHOD_OBJECT)->length);
}

bool objfn_string_isalpha(VMState* vm, int argcount, Value* args)
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

bool objfn_string_isalnum(VMState* vm, int argcount, Value* args)
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

bool objfn_string_isnumber(VMState* vm, int argcount, Value* args)
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

bool objfn_string_islower(VMState* vm, int argcount, Value* args)
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

bool objfn_string_isupper(VMState* vm, int argcount, Value* args)
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

bool objfn_string_isspace(VMState* vm, int argcount, Value* args)
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

bool objfn_string_trim(VMState* vm, int argcount, Value* args)
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
        RETURN_OBJ(bl_string_copystring(vm, "", 0));
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

bool objfn_string_ltrim(VMState* vm, int argcount, Value* args)
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
        RETURN_OBJ(bl_string_copystring(vm, "", 0));
    }
    end = string + strlen(string) - 1;
    // Write new null terminator character
    end[1] = '\0';
    RETURN_L_STRING(string, strlen(string));
}

bool objfn_string_rtrim(VMState* vm, int argcount, Value* args)
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
        RETURN_OBJ(bl_string_copystring(vm, "", 0));
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

bool objfn_string_join(VMState* vm, int argcount, Value* args)
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

bool objfn_string_split(VMState* vm, int argcount, Value* args)
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

bool objfn_string_indexof(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(indexof, 1);
    ENFORCE_ARG_TYPE(indexof, 0, bl_value_isstring);
    char* str = AS_C_STRING(METHOD_OBJECT);
    char* result = strstr(str, AS_C_STRING(args[0]));
    if(result != NULL)
        RETURN_NUMBER((int)(result - str));
    RETURN_NUMBER(-1);
}

bool objfn_string_startswith(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(startswith, 1);
    ENFORCE_ARG_TYPE(startswith, 0, bl_value_isstring);
    ObjString* string = AS_STRING(METHOD_OBJECT);
    ObjString* substr = AS_STRING(args[0]);
    if(string->length == 0 || substr->length == 0 || substr->length > string->length)
        RETURN_FALSE;
    RETURN_BOOL(memcmp(substr->chars, string->chars, substr->length) == 0);
}

bool objfn_string_endswith(VMState* vm, int argcount, Value* args)
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

bool objfn_string_count(VMState* vm, int argcount, Value* args)
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

bool objfn_string_tonumber(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(to_number, 0);
    RETURN_NUMBER(strtod(AS_C_STRING(METHOD_OBJECT), NULL));
}

bool objfn_string_ascii(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(ascii, 0);
    ObjString* string = AS_STRING(METHOD_OBJECT);
    string->isascii = true;
    RETURN_OBJ(string);
}

bool objfn_string_tolist(VMState* vm, int argcount, Value* args)
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

bool objfn_string_lpad(VMState* vm, int argcount, Value* args)
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

bool objfn_string_rpad(VMState* vm, int argcount, Value* args)
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

bool objfn_string_match(VMState* vm, int argcount, Value* args)
{
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
}

bool objfn_string_matches(VMState* vm, int argcount, Value* args)
{
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
}

bool objfn_string_replace(VMState* vm, int argcount, Value* args)
{
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
}

bool objfn_string_tobytes(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(tobytes, 0);
    ObjString* string = AS_STRING(METHOD_OBJECT);
    RETURN_OBJ(bl_bytes_copybytes(vm, (unsigned char*)string->chars, string->length));
}

bool objfn_string_iter(VMState* vm, int argcount, Value* args)
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

bool objfn_string_itern(VMState* vm, int argcount, Value* args)
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

bool cfn_bytes(VMState* vm, int argcount, Value* args)
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

bool objfn_bytes_length(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(length, 0);
    RETURN_NUMBER(AS_BYTES(METHOD_OBJECT)->bytes.count);
}

bool objfn_bytes_append(VMState* vm, int argcount, Value* args)
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

bool objfn_bytes_clone(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(clone, 0);
    ObjBytes* bytes = AS_BYTES(METHOD_OBJECT);
    ObjBytes* nbytes = (ObjBytes*)bl_mem_gcprotect(vm, (Object*)bl_object_makebytes(vm, bytes->bytes.count));
    memcpy(nbytes->bytes.bytes, bytes->bytes.bytes, bytes->bytes.count);
    RETURN_OBJ(nbytes);
}

bool objfn_bytes_extend(VMState* vm, int argcount, Value* args)
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

bool objfn_bytes_pop(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(pop, 0);
    ObjBytes* bytes = AS_BYTES(METHOD_OBJECT);
    unsigned char c = bytes->bytes.bytes[bytes->bytes.count - 1];
    bytes->bytes.count--;
    RETURN_NUMBER((double)((int)c));
}

bool objfn_bytes_remove(VMState* vm, int argcount, Value* args)
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

bool objfn_bytes_reverse(VMState* vm, int argcount, Value* args)
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

bool objfn_bytes_split(VMState* vm, int argcount, Value* args)
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

bool objfn_bytes_first(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(first, 0);
    RETURN_NUMBER((double)((int)AS_BYTES(METHOD_OBJECT)->bytes.bytes[0]));
}

bool objfn_bytes_last(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(first, 0);
    ObjBytes* bytes = AS_BYTES(METHOD_OBJECT);
    RETURN_NUMBER((double)((int)bytes->bytes.bytes[bytes->bytes.count - 1]));
}

bool objfn_bytes_get(VMState* vm, int argcount, Value* args)
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

bool objfn_bytes_isalpha(VMState* vm, int argcount, Value* args)
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

bool objfn_bytes_isalnum(VMState* vm, int argcount, Value* args)
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

bool objfn_bytes_isnumber(VMState* vm, int argcount, Value* args)
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

bool objfn_bytes_islower(VMState* vm, int argcount, Value* args)
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

bool objfn_bytes_isupper(VMState* vm, int argcount, Value* args)
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

bool objfn_bytes_isspace(VMState* vm, int argcount, Value* args)
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

bool objfn_bytes_dispose(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(dispose, 0);
    ObjBytes* bytes = AS_BYTES(METHOD_OBJECT);
    bl_bytearray_free(vm, &bytes->bytes);
    return bl_value_returnempty(vm, args);
    ;
}

bool objfn_bytes_tolist(VMState* vm, int argcount, Value* args)
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

bool objfn_bytes_tostring(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(to_string, 0);
    ObjBytes* bytes = AS_BYTES(METHOD_OBJECT);
    char* string = (char*)bytes->bytes.bytes;
    RETURN_L_STRING(string, bytes->bytes.count);
}

bool objfn_bytes_iter(VMState* vm, int argcount, Value* args)
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

bool objfn_bytes_itern(VMState* vm, int argcount, Value* args)
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

bool objfn_dict_length(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(dictionary.length, 0);
    RETURN_NUMBER(AS_DICT(METHOD_OBJECT)->names.count);
}

bool objfn_dict_add(VMState* vm, int argcount, Value* args)
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

bool objfn_dict_set(VMState* vm, int argcount, Value* args)
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

bool objfn_dict_clear(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(dict, 0);
    ObjDict* dict = AS_DICT(METHOD_OBJECT);
    bl_valarray_free(vm, &dict->names);
    bl_hashtable_free(vm, &dict->items);
    return bl_value_returnempty(vm, args);
    ;
}

bool objfn_dict_clone(VMState* vm, int argcount, Value* args)
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

bool objfn_dict_compact(VMState* vm, int argcount, Value* args)
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

bool objfn_dict_contains(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(contains, 1);
    ENFORCE_VALID_DICT_KEY(contains, 0);
    ObjDict* dict = AS_DICT(METHOD_OBJECT);
    Value value;
    RETURN_BOOL(bl_hashtable_get(&dict->items, args[0], &value));
}

bool objfn_dict_extend(VMState* vm, int argcount, Value* args)
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

bool objfn_dict_get(VMState* vm, int argcount, Value* args)
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

bool objfn_dict_keys(VMState* vm, int argcount, Value* args)
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

bool objfn_dict_values(VMState* vm, int argcount, Value* args)
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

bool objfn_dict_remove(VMState* vm, int argcount, Value* args)
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

bool objfn_dict_isempty(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(isempty, 0);
    RETURN_BOOL(AS_DICT(METHOD_OBJECT)->names.count == 0);
}

bool objfn_dict_findkey(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(findkey, 1);
    RETURN_VALUE(bl_hashtable_findkey(&AS_DICT(METHOD_OBJECT)->items, args[0]));
}

bool objfn_dict_tolist(VMState* vm, int argcount, Value* args)
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

bool objfn_dict_iter(VMState* vm, int argcount, Value* args)
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

bool objfn_dict_itern(VMState* vm, int argcount, Value* args)
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

#undef ENFORCE_VALID_DICT_KEY

void bl_scanner_init(AstScanner* s, const char* source)
{
    s->current = source;
    s->start = source;
    s->line = 1;
    s->interpolatingcount = -1;
}

bool bl_scanner_isatend(AstScanner* s)
{
    return *s->current == '\0';
}

static AstToken bl_scanner_maketoken(AstScanner* s, TokType type)
{
    AstToken t;
    t.type = type;
    t.start = s->start;
    t.length = (int)(s->current - s->start);
    t.line = s->line;
    return t;
}

static AstToken bl_scanner_makeerrortoken(AstScanner* s, const char* message, ...)
{
    va_list args;
    va_start(args, message);
    char* err = NULL;
    int length = vasprintf(&err, message, args);
    va_end(args);
    AstToken t;
    t.type = TOK_ERROR;
    t.start = err;
    if(err != NULL)
    {
        t.length = length;
    }
    else
    {
        t.length = 0;
    }
    t.line = s->line;
    return t;
}

static bool bl_scanutil_isdigit(char c)
{
    return c >= '0' && c <= '9';
}

static bool bl_scanutil_isbinary(char c)
{
    return c == '0' || c == '1';
}

static bool bl_scanutil_isalpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool bl_scanutil_isoctal(char c)
{
    return c >= '0' && c <= '7';
}

static bool bl_scanutil_ishexadecimal(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static char bl_scanner_advance(AstScanner* s)
{
    s->current++;
    if(s->current[-1] == '\n')
    {
        s->line++;
    }
    return s->current[-1];
}

static bool bl_scanner_match(AstScanner* s, char expected)
{
    if(bl_scanner_isatend(s))
    {
        return false;
    }
    if(*s->current != expected)
    {
        return false;
    }
    s->current++;
    if(s->current[-1] == '\n')
    {
        s->line++;
    }
    return true;
}

static char bl_scanner_current(AstScanner* s)
{
    return *s->current;
}

static char bl_scanner_previous(AstScanner* s)
{
    return s->current[-1];
}

static char bl_scanner_next(AstScanner* s)
{
    if(bl_scanner_isatend(s))
    {
        return '\0';
    }
    return s->current[1];
}

AstToken bl_scanner_skipblockcomments(AstScanner* s)
{
    int nesting = 1;
    while(nesting > 0)
    {
        if(bl_scanner_isatend(s))
        {
            return bl_scanner_makeerrortoken(s, "unclosed block comment");
        }
        // internal comment open
        if(bl_scanner_current(s) == '/' && bl_scanner_next(s) == '*')
        {
            bl_scanner_advance(s);
            bl_scanner_advance(s);
            nesting++;
            continue;
        }
        // comment close
        if(bl_scanner_current(s) == '*' && bl_scanner_next(s) == '/')
        {
            bl_scanner_advance(s);
            bl_scanner_advance(s);
            nesting--;
            continue;
        }
        // regular comment body
        bl_scanner_advance(s);
    }
    return bl_scanner_maketoken(s, TOK_UNDEFINED);
}

AstToken bl_scanner_skipwhitespace(AstScanner* s)
{
    for(;;)
    {
        char c = bl_scanner_current(s);
        switch(c)
        {
            case ' ':
            case '\r':
            case '\t':
                bl_scanner_advance(s);
                break;
                /* case '\n': {
          s->line++;
          bl_scanner_advance(s);
          break;
        } */
            case '#':
            {// single line comment
                while(bl_scanner_current(s) != '\n' && !bl_scanner_isatend(s))
                {
                    bl_scanner_advance(s);
                }
                break;
            }
            case '/':
                if(bl_scanner_next(s) == '*')
                {
                    bl_scanner_advance(s);
                    bl_scanner_advance(s);
                    AstToken result = bl_scanner_skipblockcomments(s);
                    if(result.type != TOK_UNDEFINED)
                    {
                        return result;
                    }
                    break;
                }
                else
                {
                    return bl_scanner_maketoken(s, TOK_UNDEFINED);
                }
                // exit as soon as we see a non-whitespace...
            default:
                return bl_scanner_maketoken(s, TOK_UNDEFINED);
        }
    }
}

static AstToken bl_scanner_scanstring(AstScanner* s, char quote)
{
    while(bl_scanner_current(s) != quote && !bl_scanner_isatend(s))
    {
        if(bl_scanner_current(s) == '$' && bl_scanner_next(s) == '{' && bl_scanner_previous(s) != '\\')
        {// interpolation started
            if(s->interpolatingcount - 1 < MAX_INTERPOLATION_NESTING)
            {
                s->interpolatingcount++;
                s->interpolating[s->interpolatingcount] = (int)quote;
                s->current++;
                AstToken tkn = bl_scanner_maketoken(s, TOK_INTERPOLATION);
                s->current++;
                return tkn;
            }
            return bl_scanner_makeerrortoken(s, "maximum interpolation nesting of %d exceeded by %d", MAX_INTERPOLATION_NESTING,
                                             MAX_INTERPOLATION_NESTING - s->interpolatingcount + 1);
        }
        if(bl_scanner_current(s) == '\\' && (bl_scanner_next(s) == quote || bl_scanner_next(s) == '\\'))
        {
            bl_scanner_advance(s);
        }
        bl_scanner_advance(s);
    }
    if(bl_scanner_isatend(s))
    {
        return bl_scanner_makeerrortoken(s, "unterminated string (opening quote not matched)");
    }
    bl_scanner_match(s, quote);// the closing quote
    return bl_scanner_maketoken(s, TOK_LITERAL);
}

static AstToken bl_scanner_scannumber(AstScanner* s)
{
    // handle binary, octal and hexadecimals
    if(bl_scanner_previous(s) == '0')
    {
        if(bl_scanner_match(s, 'b'))
        {// binary number
            while(bl_scanutil_isbinary(bl_scanner_current(s)))
            {
                bl_scanner_advance(s);
            }
            return bl_scanner_maketoken(s, TOK_BINNUMBER);
        }
        else if(bl_scanner_match(s, 'c'))
        {
            while(bl_scanutil_isoctal(bl_scanner_current(s)))
            {
                bl_scanner_advance(s);
            }
            return bl_scanner_maketoken(s, TOK_OCTNUMBER);
        }
        else if(bl_scanner_match(s, 'x'))
        {
            while(bl_scanutil_ishexadecimal(bl_scanner_current(s)))
            {
                bl_scanner_advance(s);
            }
            return bl_scanner_maketoken(s, TOK_HEXNUMBER);
        }
    }
    while(bl_scanutil_isdigit(bl_scanner_current(s)))
    {
        bl_scanner_advance(s);
    }
    // dots(.) are only valid here when followed by a digit
    if(bl_scanner_current(s) == '.' && bl_scanutil_isdigit(bl_scanner_next(s)))
    {
        bl_scanner_advance(s);
        while(bl_scanutil_isdigit(bl_scanner_current(s)))
        {
            bl_scanner_advance(s);
        }
        // E or e are only valid here when followed by a digit and occurring after a
        // dot
        if((bl_scanner_current(s) == 'e' || bl_scanner_current(s) == 'E') && (bl_scanner_next(s) == '+' || bl_scanner_next(s) == '-'))
        {
            bl_scanner_advance(s);
            bl_scanner_advance(s);
            while(bl_scanutil_isdigit(bl_scanner_current(s)))
            {
                bl_scanner_advance(s);
            }
        }
    }
    return bl_scanner_maketoken(s, TOK_REGNUMBER);
}

static TokType bl_scanner_scanidenttype(AstScanner* s)
{
    static const struct
    {
        int tokid;
        const char* str;
    } keywords[] = { { TOK_AND, "and" },
                     { TOK_ASSERT, "assert" },
                     { TOK_AS, "as" },
                     { TOK_BREAK, "break" },
                     { TOK_CATCH, "catch" },
                     { TOK_CLASS, "class" },
                     { TOK_CONTINUE, "continue" },
                     { TOK_DEFAULT, "default" },
                     { TOK_DEF, "def" },
                     { TOK_DIE, "die" },
                     { TOK_DO, "do" },
                     { TOK_ECHO, "echo" },
                     { TOK_ELSE, "else" },
                     { TOK_EMPTY, "empty" },
                     { TOK_FALSE, "false" },
                     { TOK_FINALLY, "finally" },
                     { TOK_FOREACH, "foreach" },
                     { TOK_IF, "if" },
                     { TOK_IMPORT, "import" },
                     { TOK_IN, "in" },
                     { TOK_FORLOOP, "for" },
                     { TOK_NIL, "nil" },
                     { TOK_OR, "or" },
                     { TOK_PARENT, "parent" },
                     { TOK_RETURN, "return" },
                     { TOK_SELF, "self" },
                     { TOK_STATIC, "static" },
                     { TOK_TRUE, "true" },
                     { TOK_TRY, "try" },
                     { TOK_USING, "using" },
                     { TOK_VAR, "var" },
                     { TOK_WHILE, "while" },
                     { TOK_WHEN, "when" },
                     { 0, NULL } };

    size_t i;
    size_t kwlen;
    size_t ofs;
    const char* kwtext;
    for(i = 0; keywords[i].str != NULL; i++)
    {
        kwtext = keywords[i].str;
        kwlen = strlen(kwtext);
        ofs = (s->current - s->start);
        if((ofs == (0 + kwlen)) && (memcmp(s->start + 0, kwtext, kwlen) == 0))
        {
            return (TokType)keywords[i].tokid;
        }
    }
    return TOK_IDENTIFIER;
}

static AstToken bl_scanner_scanidentifier(AstScanner* s)
{
    while(bl_scanutil_isalpha(bl_scanner_current(s)) || bl_scanutil_isdigit(bl_scanner_current(s)))
    {
        bl_scanner_advance(s);
    }
    return bl_scanner_maketoken(s, bl_scanner_scanidenttype(s));
}

static AstToken bl_scanner_decorator(AstScanner* s)
{
    while(bl_scanutil_isalpha(bl_scanner_current(s)) || bl_scanutil_isdigit(bl_scanner_current(s)))
    {
        bl_scanner_advance(s);
    }
    return bl_scanner_maketoken(s, TOK_DECORATOR);
}

AstToken bl_scanner_scantoken(AstScanner* s)
{
    AstToken tk = bl_scanner_skipwhitespace(s);
    if(tk.type != TOK_UNDEFINED)
    {
        return tk;
    }
    s->start = s->current;
    if(bl_scanner_isatend(s))
    {
        return bl_scanner_maketoken(s, TOK_EOF);
    }
    char c = bl_scanner_advance(s);
    if(bl_scanutil_isdigit(c))
    {
        return bl_scanner_scannumber(s);
    }
    else if(bl_scanutil_isalpha(c))
    {
        return bl_scanner_scanidentifier(s);
    }
    switch(c)
    {
        case '(':
            return bl_scanner_maketoken(s, TOK_LPAREN);
        case ')':
            return bl_scanner_maketoken(s, TOK_RPAREN);
        case '[':
            return bl_scanner_maketoken(s, TOK_LBRACKET);
        case ']':
            return bl_scanner_maketoken(s, TOK_RBRACKET);
        case '{':
            return bl_scanner_maketoken(s, TOK_LBRACE);
        case '}':
            if(s->interpolatingcount > -1)
            {
                AstToken token = bl_scanner_scanstring(s, (char)s->interpolating[s->interpolatingcount]);
                s->interpolatingcount--;
                return token;
            }
            return bl_scanner_maketoken(s, TOK_RBRACE);
        case ';':
            return bl_scanner_maketoken(s, TOK_SEMICOLON);
        case '\\':
            return bl_scanner_maketoken(s, TOK_BACKSLASH);
        case ':':
            return bl_scanner_maketoken(s, TOK_COLON);
        case ',':
            return bl_scanner_maketoken(s, TOK_COMMA);
        case '@':
            return bl_scanner_decorator(s);
        case '!':
            return bl_scanner_maketoken(s, bl_scanner_match(s, '=') ? TOK_BANGEQ : TOK_BANG);
        case '.':
            if(bl_scanner_match(s, '.'))
            {
                return bl_scanner_maketoken(s, bl_scanner_match(s, '.') ? TOK_TRIDOT : TOK_RANGE);
            }
            return bl_scanner_maketoken(s, TOK_DOT);
        case '+':
        {
            if(bl_scanner_match(s, '+'))
            {
                return bl_scanner_maketoken(s, TOK_INCREMENT);
            }
            if(bl_scanner_match(s, '='))
            {
                return bl_scanner_maketoken(s, TOK_PLUSEQ);
            }
            else
            {
                return bl_scanner_maketoken(s, TOK_PLUS);
            }
        }
        case '-':
        {
            if(bl_scanner_match(s, '-'))
            {
                return bl_scanner_maketoken(s, TOK_DECREMENT);
            }
            if(bl_scanner_match(s, '='))
            {
                return bl_scanner_maketoken(s, TOK_MINUSEQ);
            }
            else
            {
                return bl_scanner_maketoken(s, TOK_MINUS);
            }
        }
        case '*':
            if(bl_scanner_match(s, '*'))
            {
                return bl_scanner_maketoken(s, bl_scanner_match(s, '=') ? TOK_POWEQ : TOK_POW);
            }
            else
            {
                return bl_scanner_maketoken(s, bl_scanner_match(s, '=') ? TOK_MULTIPLYEQ : TOK_MULTIPLY);
            }
        case '/':
            if(bl_scanner_match(s, '/'))
            {
                return bl_scanner_maketoken(s, bl_scanner_match(s, '=') ? TOK_FLOOREQ : TOK_FLOOR);
            }
            else
            {
                return bl_scanner_maketoken(s, bl_scanner_match(s, '=') ? TOK_DIVIDEEQ : TOK_DIVIDE);
            }
        case '=':
            return bl_scanner_maketoken(s, bl_scanner_match(s, '=') ? TOK_EQUALEQ : TOK_EQUAL);
        case '<':
            if(bl_scanner_match(s, '<'))
            {
                return bl_scanner_maketoken(s, bl_scanner_match(s, '=') ? TOK_LSHIFTEQ : TOK_LSHIFT);
            }
            else
            {
                return bl_scanner_maketoken(s, bl_scanner_match(s, '=') ? TOK_LESSEQ : TOK_LESS);
            }
        case '>':
            if(bl_scanner_match(s, '>'))
            {
                return bl_scanner_maketoken(s, bl_scanner_match(s, '=') ? TOK_RSHIFTEQ : TOK_RSHIFT);
            }
            else
            {
                return bl_scanner_maketoken(s, bl_scanner_match(s, '=') ? TOK_GREATEREQ : TOK_GREATER);
            }
        case '%':
            return bl_scanner_maketoken(s, bl_scanner_match(s, '=') ? TOK_PERCENTEQ : TOK_PERCENT);
        case '&':
            return bl_scanner_maketoken(s, bl_scanner_match(s, '=') ? TOK_AMPEQ : TOK_AMP);
        case '|':
            return bl_scanner_maketoken(s, bl_scanner_match(s, '=') ? TOK_BAREQ : TOK_BAR);
        case '~':
            return bl_scanner_maketoken(s, bl_scanner_match(s, '=') ? TOK_TILDEEQ : TOK_TILDE);
        case '^':
            return bl_scanner_maketoken(s, bl_scanner_match(s, '=') ? TOK_XOREQ : TOK_XOR);
            // newline
        case '\n':
            return bl_scanner_maketoken(s, TOK_NEWLINE);
        case '"':
            return bl_scanner_scanstring(s, '"');
        case '\'':
            return bl_scanner_scanstring(s, '\'');
        case '?':
            return bl_scanner_maketoken(s, TOK_QUESTION);
            // --- DO NOT MOVE ABOVE OR BELOW THE DEFAULT CASE ---
            // fall-through tokens goes here... this tokens are only valid
            // when the carry another token with them...
            // be careful not to add break after them so that they may use the default
            // case.
        default:
            break;
    }
    return bl_scanner_makeerrortoken(s, "unexpected character %c", c);
}

static BinaryBlob* bl_parser_currentblob(AstParser* p);
static void bl_parser_errorat(AstParser* p, AstToken* t, const char* message, va_list args);
static void bl_parser_raiseerror(AstParser* p, const char* message, ...);
static void bl_parser_raiseerroratcurrent(AstParser* p, const char* message, ...);
static void bl_parser_advance(AstParser* p);
static void bl_parser_consume(AstParser* p, TokType t, const char* message);
static void bl_parser_consumeor(AstParser* p, const char* message, const TokType ts[], int count);
static bool bl_parser_checknumber(AstParser* p);
static bool bl_parser_check(AstParser* p, TokType t);
static bool bl_parser_match(AstParser* p, TokType t);
static void bl_parser_consumestmtend(AstParser* p);
static void bl_parser_ignorespace(AstParser* p);
static int bl_parser_getcodeargscount(const uint8_t* bytecode, const Value* constants, int ip);
static void bl_parser_emitbyte(AstParser* p, uint8_t byte);
static void bl_parser_emitshort(AstParser* p, uint16_t byte);
static void bl_parser_emitbytes(AstParser* p, uint8_t byte, uint8_t byte2);
static void bl_parser_emitbyte_and_short(AstParser* p, uint8_t byte, uint16_t byte2);
static void bl_parser_emitloop(AstParser* p, int loopstart);
static void bl_parser_emitreturn(AstParser* p);
static int bl_parser_makeconstant(AstParser* p, Value value);
static void bl_parser_emitconstant(AstParser* p, Value value);
static int bl_parser_emitjump(AstParser* p, uint8_t instruction);
static int bl_parser_emitswitch(AstParser* p);
static int bl_parser_emittry(AstParser* p);
static void bl_parser_patchswitch(AstParser* p, int offset, int constant);
static void bl_parser_patchtry(AstParser* p, int offset, int type, int address, int finally);
static void bl_parser_patchjump(AstParser* p, int offset);
static void bl_compiler_init(AstParser* p, AstCompiler* compiler, FuncType type);
static int bl_parser_identconst(AstParser* p, AstToken* name);
static bool bl_parser_identsequal(AstToken* a, AstToken* b);
static int bl_compiler_resolvelocal(AstParser* p, AstCompiler* compiler, AstToken* name);
static int bl_compiler_addupvalue(AstParser* p, AstCompiler* compiler, uint16_t index, bool islocal);
static int bl_compiler_resolveupvalue(AstParser* p, AstCompiler* compiler, AstToken* name);
static int bl_parser_addlocal(AstParser* p, AstToken name);
static void bl_parser_declvar(AstParser* p);
static int bl_parser_parsevar(AstParser* p, const char* message);
static void bl_parser_markinit(AstParser* p);
static void bl_parser_defvar(AstParser* p, int global);
static AstToken bl_parser_synthtoken(const char* name);
static ObjFunction* bl_compiler_end(AstParser* p);
static void bl_parser_beginscope(AstParser* p);
static void bl_parser_endscope(AstParser* p);
static void bl_parser_discardlocal(AstParser* p, int depth);
static void bl_parser_endloop(AstParser* p);
static uint8_t bl_parser_parsearglist(AstParser* p);
static void bl_parser_parseassign(AstParser* p, uint8_t realop, uint8_t getop, uint8_t setop, int arg);
static void bl_parser_doassign(AstParser* p, uint8_t getop, uint8_t setop, int arg, bool canassign);
static void bl_parser_namedvar(AstParser* p, AstToken name, bool canassign);
static Value bl_parser_compilenumber(AstParser* p);
static int bl_parser_readhexdigit(char c);
static int bl_parser_readhexescape(AstParser* p, char* str, int index, int count);
static int bl_parser_readunicodeescape(AstParser* p, char* string, char* realstring, int numberbytes, int realindex, int index);
static char* bl_parser_compilestring(AstParser* p, int* length);
static void bl_parser_doparseprecedence(AstParser* p, AstPrecedence precedence);
static void bl_parser_parseprecedence(AstParser* p, AstPrecedence precedence);
static void bl_parser_parseprecnoadvance(AstParser* p, AstPrecedence precedence);
static AstRule* bl_parser_getrule(TokType type);
static void bl_parser_parseexpr(AstParser* p);
static void bl_parser_parseblock(AstParser* p);
static void bl_parser_parsefunctionargs(AstParser* p);
static void bl_parser_parsefunctionbody(AstParser* p, AstCompiler* compiler);
static void bl_parser_parsefunction(AstParser* p, FuncType type);
static void bl_parser_parsemethod(AstParser* p, AstToken classname, bool isstatic);
static void bl_parser_parsefield(AstParser* p, bool isstatic);
static void bl_parser_parsefunctiondecl(AstParser* p);
static void bl_parser_parseclassdecl(AstParser* p);
static void bl_parser_compilevardecl(AstParser* p, bool isinitializer);
static void bl_parser_parsevardecl(AstParser* p);
static void bl_parser_parseexprstmt(AstParser* p, bool isinitializer, bool semi);
static void bl_parser_parseforloopstmt(AstParser* p);
static void bl_parser_parseforeachstmt(AstParser* p);
static void bl_parser_parseusingstmt(AstParser* p);
static void bl_parser_parseifstmt(AstParser* p);
static void bl_parser_parseechostmt(AstParser* p);
static void bl_parser_parsediestmt(AstParser* p);
static void bl_parser_parsespecificimport(AstParser* p, char* modulename, int importconstant, bool wasrenamed, bool isnative);
static void bl_parser_parseimportstmt(AstParser* p);
static void bl_parser_parseassertstmt(AstParser* p);
static void bl_parser_parsetrystmt(AstParser* p);
static void bl_parser_parsereturnstmt(AstParser* p);
static void bl_parser_parsewhilestmt(AstParser* p);
static void bl_parser_parsedowhilestmt(AstParser* p);
static void bl_parser_parsecontinuestmt(AstParser* p);
static void bl_parser_parsebreakstmt(AstParser* p);
static void bl_parser_synchronize(AstParser* p);
static void bl_parser_parsedeclaration(AstParser* p);
static void bl_parser_parsestmt(AstParser* p);

static BinaryBlob* bl_parser_currentblob(AstParser* p)
{
    return &p->vm->compiler->currfunc->blob;
}

static void bl_parser_errorat(AstParser* p, AstToken* t, const char* message, va_list args)
{
    fflush(stdout);// flush out anything on stdout first
    // do not cascade error
    // suppress error if already in panic mode
    if(p->panicmode)
    {
        return;
    }
    p->panicmode = true;
    fprintf(stderr, "SyntaxError");
    if(t->type == TOK_EOF)
    {
        fprintf(stderr, " at end");
    }
    else if(t->type == TOK_ERROR)
    {
        // do nothing
    }
    else
    {
        if(t->length == 1 && *t->start == '\n')
        {
            fprintf(stderr, " at newline");
        }
        else
        {
            fprintf(stderr, " at '%.*s'", t->length, t->start);
        }
    }
    fprintf(stderr, ": ");
    vfprintf(stderr, message, args);
    fputs("\n", stderr);
    fprintf(stderr, "  %s:%d\n", p->module->file, t->line);
    p->haderror = true;
}

static void bl_parser_raiseerror(AstParser* p, const char* message, ...)
{
    va_list args;
    va_start(args, message);
    bl_parser_errorat(p, &p->previous, message, args);
    va_end(args);
}

static void bl_parser_raiseerroratcurrent(AstParser* p, const char* message, ...)
{
    va_list args;
    va_start(args, message);
    bl_parser_errorat(p, &p->current, message, args);
    va_end(args);
}

static void bl_parser_advance(AstParser* p)
{
    p->previous = p->current;
    for(;;)
    {
        p->current = bl_scanner_scantoken(p->scanner);
        if(p->current.type != TOK_ERROR)
        {
            break;
        }
        bl_parser_raiseerroratcurrent(p, p->current.start);
    }
}

static void bl_parser_consume(AstParser* p, TokType t, const char* message)
{
    if(p->current.type == t)
    {
        bl_parser_advance(p);
        return;
    }
    bl_parser_raiseerroratcurrent(p, message);
}

static void bl_parser_consumeor(AstParser* p, const char* message, const TokType ts[], int count)
{
    for(int i = 0; i < count; i++)
    {
        if(p->current.type == ts[i])
        {
            bl_parser_advance(p);
            return;
        }
    }
    bl_parser_raiseerroratcurrent(p, message);
}

static bool bl_parser_checknumber(AstParser* p)
{
    if(p->previous.type == TOK_REGNUMBER || p->previous.type == TOK_OCTNUMBER || p->previous.type == TOK_BINNUMBER || p->previous.type == TOK_HEXNUMBER)
    {
        return true;
    }
    return false;
}

static bool bl_parser_check(AstParser* p, TokType t)
{
    return p->current.type == t;
}

static bool bl_parser_match(AstParser* p, TokType t)
{
    if(!bl_parser_check(p, t))
    {
        return false;
    }
    bl_parser_advance(p);
    return true;
}

static void bl_parser_consumestmtend(AstParser* p)
{
    // allow block last statement to omit statement end
    if(p->blockcount > 0 && bl_parser_check(p, TOK_RBRACE))
    {
        return;
    }
    if(bl_parser_match(p, TOK_SEMICOLON))
    {
        while(bl_parser_match(p, TOK_SEMICOLON) || bl_parser_match(p, TOK_NEWLINE))
        {
            ;
        }
        return;
    }
    if(bl_parser_match(p, TOK_EOF) || p->previous.type == TOK_EOF)
    {
        return;
    }
    bl_parser_consume(p, TOK_NEWLINE, "end of statement expected");
    while(bl_parser_match(p, TOK_SEMICOLON) || bl_parser_match(p, TOK_NEWLINE))
    {
        ;
    }
}

static void bl_parser_ignorespace(AstParser* p)
{
    while(bl_parser_match(p, TOK_NEWLINE))
    {
        ;
    }
}

static int bl_parser_getcodeargscount(const uint8_t* bytecode, const Value* constants, int ip)
{
    OpCode code = (OpCode)bytecode[ip];
    switch(code)
    {
        case OP_EQUAL:
        case OP_GREATER:
        case OP_LESS:
        case OP_NIL:
        case OP_TRUE:
        case OP_FALSE:
        case OP_ADD:
        case OP_SUBTRACT:
        case OP_MULTIPLY:
        case OP_DIVIDE:
        case OP_F_DIVIDE:
        case OP_REMINDER:
        case OP_POW:
        case OP_NEGATE:
        case OP_NOT:
        case OP_ECHO:
        case OP_POP:
        case OP_CLOSE_UP_VALUE:
        case OP_DUP:
        case OP_RETURN:
        case OP_INHERIT:
        case OP_GET_SUPER:
        case OP_AND:
        case OP_OR:
        case OP_XOR:
        case OP_LSHIFT:
        case OP_RSHIFT:
        case OP_BIT_NOT:
        case OP_ONE:
        case OP_SET_INDEX:
        case OP_ASSERT:
        case OP_DIE:
        case OP_POP_TRY:
        case OP_RANGE:
        case OP_STRINGIFY:
        case OP_CHOICE:
        case OP_EMPTY:
        case OP_IMPORT_ALL_NATIVE:
        case OP_IMPORT_ALL:
        case OP_PUBLISH_TRY:
            return 0;
        case OP_CALL:
        case OP_SUPER_INVOKE_SELF:
        case OP_GET_INDEX:
        case OP_GET_RANGED_INDEX:
            return 1;
        case OP_DEFINE_GLOBAL:
        case OP_GET_GLOBAL:
        case OP_SET_GLOBAL:
        case OP_GET_LOCAL:
        case OP_SET_LOCAL:
        case OP_GET_UP_VALUE:
        case OP_SET_UP_VALUE:
        case OP_JUMP_IF_FALSE:
        case OP_JUMP:
        case OP_BREAK_PL:
        case OP_LOOP:
        case OP_CONSTANT:
        case OP_POP_N:
        case OP_CLASS:
        case OP_GET_PROPERTY:
        case OP_GET_SELF_PROPERTY:
        case OP_SET_PROPERTY:
        case OP_LIST:
        case OP_DICT:
        case OP_CALL_IMPORT:
        case OP_NATIVE_MODULE:
        case OP_SELECT_NATIVE_IMPORT:
        case OP_SWITCH:
        case OP_METHOD:
        case OP_EJECT_IMPORT:
        case OP_EJECT_NATIVE_IMPORT:
        case OP_SELECT_IMPORT:
            return 2;
        case OP_INVOKE:
        case OP_INVOKE_SELF:
        case OP_SUPER_INVOKE:
        case OP_CLASS_PROPERTY:
            return 3;
        case OP_TRY:
            return 6;
        case OP_CLOSURE:
        {
            int constant = (bytecode[ip + 1] << 8) | bytecode[ip + 2];
            ObjFunction* fn = AS_FUNCTION(constants[constant]);
            // There is two byte for the constant, then three for each up value.
            return 2 + (fn->upvaluecount * 3);
        }
            //    default:
            //      return 0;
    }
    return 0;
}

static void bl_parser_emitbyte(AstParser* p, uint8_t byte)
{
    bl_blob_write(p->vm, bl_parser_currentblob(p), byte, p->previous.line);
}

static void bl_parser_emitshort(AstParser* p, uint16_t byte)
{
    bl_blob_write(p->vm, bl_parser_currentblob(p), (byte >> 8) & 0xff, p->previous.line);
    bl_blob_write(p->vm, bl_parser_currentblob(p), byte & 0xff, p->previous.line);
}

static void bl_parser_emitbytes(AstParser* p, uint8_t byte, uint8_t byte2)
{
    bl_blob_write(p->vm, bl_parser_currentblob(p), byte, p->previous.line);
    bl_blob_write(p->vm, bl_parser_currentblob(p), byte2, p->previous.line);
}

static void bl_parser_emitbyte_and_short(AstParser* p, uint8_t byte, uint16_t byte2)
{
    bl_blob_write(p->vm, bl_parser_currentblob(p), byte, p->previous.line);
    bl_blob_write(p->vm, bl_parser_currentblob(p), (byte2 >> 8) & 0xff, p->previous.line);
    bl_blob_write(p->vm, bl_parser_currentblob(p), byte2 & 0xff, p->previous.line);
}

/* static void bl_parser_emitbyte_and_long(AstParser *p, uint8_t byte, uint16_t byte2) {
  bl_blob_write(p->vm, bl_parser_currentblob(p), byte, p->previous.line);
  bl_blob_write(p->vm, bl_parser_currentblob(p), (byte2 >> 16) & 0xff, p->previous.line);
  bl_blob_write(p->vm, bl_parser_currentblob(p), (byte2 >> 8) & 0xff, p->previous.line);
  bl_blob_write(p->vm, bl_parser_currentblob(p), byte2 & 0xff, p->previous.line);
} */
static void bl_parser_emitloop(AstParser* p, int loopstart)
{
    bl_parser_emitbyte(p, OP_LOOP);
    int offset = bl_parser_currentblob(p)->count - loopstart + 2;
    if(offset > UINT16_MAX)
    {
        bl_parser_raiseerror(p, "loop body too large");
    }
    bl_parser_emitbyte(p, (offset >> 8) & 0xff);
    bl_parser_emitbyte(p, offset & 0xff);
}

static void bl_parser_emitreturn(AstParser* p)
{
    if(p->istrying)
    {
        bl_parser_emitbyte(p, OP_POP_TRY);
    }
    if(p->vm->compiler->type == TYPE_INITIALIZER)
    {
        bl_parser_emitbyte_and_short(p, OP_GET_LOCAL, 0);
    }
    else
    {
        bl_parser_emitbyte(p, OP_EMPTY);
    }
    bl_parser_emitbyte(p, OP_RETURN);
}

static int bl_parser_makeconstant(AstParser* p, Value value)
{
    int constant = bl_blob_addconst(p->vm, bl_parser_currentblob(p), value);
    if(constant >= UINT16_MAX)
    {
        bl_parser_raiseerror(p, "too many constants in current scope");
        return 0;
    }
    return constant;
}

static void bl_parser_emitconstant(AstParser* p, Value value)
{
    int constant = bl_parser_makeconstant(p, value);
    bl_parser_emitbyte_and_short(p, OP_CONSTANT, (uint16_t)constant);
}

static int bl_parser_emitjump(AstParser* p, uint8_t instruction)
{
    bl_parser_emitbyte(p, instruction);
    // placeholders
    bl_parser_emitbyte(p, 0xff);
    bl_parser_emitbyte(p, 0xff);
    return bl_parser_currentblob(p)->count - 2;
}

static int bl_parser_emitswitch(AstParser* p)
{
    bl_parser_emitbyte(p, OP_SWITCH);
    // placeholders
    bl_parser_emitbyte(p, 0xff);
    bl_parser_emitbyte(p, 0xff);
    return bl_parser_currentblob(p)->count - 2;
}

static int bl_parser_emittry(AstParser* p)
{
    bl_parser_emitbyte(p, OP_TRY);
    // type placeholders
    bl_parser_emitbyte(p, 0xff);
    bl_parser_emitbyte(p, 0xff);
    // handler placeholders
    bl_parser_emitbyte(p, 0xff);
    bl_parser_emitbyte(p, 0xff);
    // finally placeholders
    bl_parser_emitbyte(p, 0xff);
    bl_parser_emitbyte(p, 0xff);
    return bl_parser_currentblob(p)->count - 6;
}

static void bl_parser_patchswitch(AstParser* p, int offset, int constant)
{
    bl_parser_currentblob(p)->code[offset] = (constant >> 8) & 0xff;
    bl_parser_currentblob(p)->code[offset + 1] = constant & 0xff;
}

static void bl_parser_patchtry(AstParser* p, int offset, int type, int address, int finally)
{
    // patch type
    bl_parser_currentblob(p)->code[offset] = (type >> 8) & 0xff;
    bl_parser_currentblob(p)->code[offset + 1] = type & 0xff;
    // patch address
    bl_parser_currentblob(p)->code[offset + 2] = (address >> 8) & 0xff;
    bl_parser_currentblob(p)->code[offset + 3] = address & 0xff;
    // patch finally
    bl_parser_currentblob(p)->code[offset + 4] = (finally >> 8) & 0xff;
    bl_parser_currentblob(p)->code[offset + 5] = finally & 0xff;
}

static void bl_parser_patchjump(AstParser* p, int offset)
{
    // -2 to adjust the bytecode for the offset itself
    int jump = bl_parser_currentblob(p)->count - offset - 2;
    if(jump > UINT16_MAX)
    {
        bl_parser_raiseerror(p, "body of conditional block too large");
    }
    bl_parser_currentblob(p)->code[offset] = (jump >> 8) & 0xff;
    bl_parser_currentblob(p)->code[offset + 1] = jump & 0xff;
}

static void bl_compiler_init(AstParser* p, AstCompiler* compiler, FuncType type)
{
    compiler->enclosing = p->vm->compiler;
    compiler->currfunc = NULL;
    compiler->type = type;
    compiler->localcount = 0;
    compiler->scopedepth = 0;
    compiler->handlercount = 0;
    compiler->currfunc = bl_object_makescriptfunction(p->vm, p->module, type);
    p->vm->compiler = compiler;
    if(type != TYPE_SCRIPT)
    {
        bl_vm_pushvalue(p->vm, OBJ_VAL(compiler->currfunc));
        p->vm->compiler->currfunc->name = bl_string_copystring(p->vm, p->previous.start, p->previous.length);
        bl_vm_popvalue(p->vm);
    }
    // claiming slot zero for use in class methods
    AstLocal* local = &p->vm->compiler->locals[p->vm->compiler->localcount++];
    local->depth = 0;
    local->iscaptured = false;
    if(type != TYPE_FUNCTION)
    {
        local->name.start = "self";
        local->name.length = 4;
    }
    else
    {
        local->name.start = "";
        local->name.length = 0;
    }
}

static int bl_parser_identconst(AstParser* p, AstToken* name)
{
    return bl_parser_makeconstant(p, OBJ_VAL(bl_string_copystring(p->vm, name->start, name->length)));
}

static bool bl_parser_identsequal(AstToken* a, AstToken* b)
{
    return a->length == b->length && memcmp(a->start, b->start, a->length) == 0;
}

static int bl_compiler_resolvelocal(AstParser* p, AstCompiler* compiler, AstToken* name)
{
    for(int i = compiler->localcount - 1; i >= 0; i--)
    {
        AstLocal* local = &compiler->locals[i];
        if(bl_parser_identsequal(&local->name, name))
        {
            if(local->depth == -1)
            {
                bl_parser_raiseerror(p, "cannot read local variable in it's own initializer");
            }
            return i;
        }
    }
    return -1;
}

static int bl_compiler_addupvalue(AstParser* p, AstCompiler* compiler, uint16_t index, bool islocal)
{
    int upvaluecount = compiler->currfunc->upvaluecount;
    for(int i = 0; i < upvaluecount; i++)
    {
        Upvalue* upvalue = &compiler->upvalues[i];
        if(upvalue->index == index && upvalue->islocal == islocal)
        {
            return i;
        }
    }
    if(upvaluecount == UINT8_COUNT)
    {
        bl_parser_raiseerror(p, "too many closure variables in function");
        return 0;
    }
    compiler->upvalues[upvaluecount].islocal = islocal;
    compiler->upvalues[upvaluecount].index = index;
    return compiler->currfunc->upvaluecount++;
}

static int bl_compiler_resolveupvalue(AstParser* p, AstCompiler* compiler, AstToken* name)
{
    if(compiler->enclosing == NULL)
    {
        return -1;
    }
    int local = bl_compiler_resolvelocal(p, compiler->enclosing, name);
    if(local != -1)
    {
        compiler->enclosing->locals[local].iscaptured = true;
        return bl_compiler_addupvalue(p, compiler, (uint16_t)local, true);
    }
    int upvalue = bl_compiler_resolveupvalue(p, compiler->enclosing, name);
    if(upvalue != -1)
    {
        return bl_compiler_addupvalue(p, compiler, (uint16_t)upvalue, false);
    }
    return -1;
}

static int bl_parser_addlocal(AstParser* p, AstToken name)
{
    if(p->vm->compiler->localcount == UINT8_COUNT)
    {
        // we've reached maximum local variables per scope
        bl_parser_raiseerror(p, "too many local variables in scope");
        return -1;
    }
    AstLocal* local = &p->vm->compiler->locals[p->vm->compiler->localcount++];
    local->name = name;
    local->depth = -1;
    local->iscaptured = false;
    return p->vm->compiler->localcount;
}

static void bl_parser_declvar(AstParser* p)
{
    // global variables are implicitly declared...
    if(p->vm->compiler->scopedepth == 0)
    {
        return;
    }
    AstToken* name = &p->previous;
    for(int i = p->vm->compiler->localcount - 1; i >= 0; i--)
    {
        AstLocal* local = &p->vm->compiler->locals[i];
        if(local->depth != -1 && local->depth < p->vm->compiler->scopedepth)
        {
            break;
        }
        if(bl_parser_identsequal(name, &local->name))
        {
            bl_parser_raiseerror(p, "%.*s already declared in current scope", name->length, name->start);
        }
    }
    bl_parser_addlocal(p, *name);
}

static int bl_parser_parsevar(AstParser* p, const char* message)
{
    bl_parser_consume(p, TOK_IDENTIFIER, message);
    bl_parser_declvar(p);
    if(p->vm->compiler->scopedepth > 0)
    {// we are in a local scope...
        return 0;
    }
    return bl_parser_identconst(p, &p->previous);
}

static void bl_parser_markinit(AstParser* p)
{
    if(p->vm->compiler->scopedepth == 0)
    {
        return;
    }
    p->vm->compiler->locals[p->vm->compiler->localcount - 1].depth = p->vm->compiler->scopedepth;
}

static void bl_parser_defvar(AstParser* p, int global)
{
    if(p->vm->compiler->scopedepth > 0)
    {// we are in a local scope...
        bl_parser_markinit(p);
        return;
    }
    bl_parser_emitbyte_and_short(p, OP_DEFINE_GLOBAL, global);
}

static AstToken bl_parser_synthtoken(const char* name)
{
    AstToken token;
    token.start = name;
    token.length = (int)strlen(name);
    return token;
}

static ObjFunction* bl_compiler_end(AstParser* p)
{
    bl_parser_emitreturn(p);
    ObjFunction* function = p->vm->compiler->currfunc;
    if(!p->haderror && p->vm->shouldprintbytecode)
    {
        bl_blob_disassembleitem(bl_parser_currentblob(p), function->name == NULL ? p->module->file : function->name->chars);
    }
    p->vm->compiler = p->vm->compiler->enclosing;
    return function;
}

static void bl_parser_beginscope(AstParser* p)
{
    p->vm->compiler->scopedepth++;
}

static void bl_parser_endscope(AstParser* p)
{
    p->vm->compiler->scopedepth--;
    // remove all variables declared in scope while exiting...
    while(p->vm->compiler->localcount > 0 && p->vm->compiler->locals[p->vm->compiler->localcount - 1].depth > p->vm->compiler->scopedepth)
    {
        if(p->vm->compiler->locals[p->vm->compiler->localcount - 1].iscaptured)
        {
            bl_parser_emitbyte(p, OP_CLOSE_UP_VALUE);
        }
        else
        {
            bl_parser_emitbyte(p, OP_POP);
        }
        p->vm->compiler->localcount--;
    }
}

static void bl_parser_discardlocal(AstParser* p, int depth)
{
    if(p->vm->compiler->scopedepth == -1)
    {
        bl_parser_raiseerror(p, "cannot exit top-level scope");
    }
    for(int i = p->vm->compiler->localcount - 1; i >= 0 && p->vm->compiler->locals[i].depth > depth; i--)
    {
        if(p->vm->compiler->locals[i].iscaptured)
        {
            bl_parser_emitbyte(p, OP_CLOSE_UP_VALUE);
        }
        else
        {
            bl_parser_emitbyte(p, OP_POP);
        }
    }
}

static void bl_parser_endloop(AstParser* p)
{
    // find all OP_BREAK_PL placeholder and replace with the appropriate jump...
    int i = p->innermostloopstart;
    while(i < p->vm->compiler->currfunc->blob.count)
    {
        if(p->vm->compiler->currfunc->blob.code[i] == OP_BREAK_PL)
        {
            p->vm->compiler->currfunc->blob.code[i] = OP_JUMP;
            bl_parser_patchjump(p, i + 1);
        }
        else
        {
            i += 1 + bl_parser_getcodeargscount(p->vm->compiler->currfunc->blob.code, p->vm->compiler->currfunc->blob.constants.values, i);
        }
    }
}

static void bl_parser_rulebinary(AstParser* p, AstToken previous, bool canassign)
{
    TokType op;
    AstRule* rule;
    (void)previous;
    (void)canassign;
    op = p->previous.type;
    // compile the right operand
    rule = bl_parser_getrule(op);
    bl_parser_parseprecedence(p, (AstPrecedence)(rule->precedence + 1));
    // emit the operator instruction
    switch(op)
    {
        case TOK_PLUS:
            bl_parser_emitbyte(p, OP_ADD);
            break;
        case TOK_MINUS:
            bl_parser_emitbyte(p, OP_SUBTRACT);
            break;
        case TOK_MULTIPLY:
            bl_parser_emitbyte(p, OP_MULTIPLY);
            break;
        case TOK_DIVIDE:
            bl_parser_emitbyte(p, OP_DIVIDE);
            break;
        case TOK_PERCENT:
            bl_parser_emitbyte(p, OP_REMINDER);
            break;
        case TOK_POW:
            bl_parser_emitbyte(p, OP_POW);
            break;
        case TOK_FLOOR:
            bl_parser_emitbyte(p, OP_F_DIVIDE);
            break;
            // equality
        case TOK_EQUALEQ:
            bl_parser_emitbyte(p, OP_EQUAL);
            break;
        case TOK_BANGEQ:
            bl_parser_emitbytes(p, OP_EQUAL, OP_NOT);
            break;
        case TOK_GREATER:
            bl_parser_emitbyte(p, OP_GREATER);
            break;
        case TOK_GREATEREQ:
            bl_parser_emitbytes(p, OP_LESS, OP_NOT);
            break;
        case TOK_LESS:
            bl_parser_emitbyte(p, OP_LESS);
            break;
        case TOK_LESSEQ:
            bl_parser_emitbytes(p, OP_GREATER, OP_NOT);
            break;
            // bitwise
        case TOK_AMP:
            bl_parser_emitbyte(p, OP_AND);
            break;
        case TOK_BAR:
            bl_parser_emitbyte(p, OP_OR);
            break;
        case TOK_XOR:
            bl_parser_emitbyte(p, OP_XOR);
            break;
        case TOK_LSHIFT:
            bl_parser_emitbyte(p, OP_LSHIFT);
            break;
        case TOK_RSHIFT:
            bl_parser_emitbyte(p, OP_RSHIFT);
            break;
            // range
        case TOK_RANGE:
            bl_parser_emitbyte(p, OP_RANGE);
            break;
        default:
            break;
    }
}

static uint8_t bl_parser_parsearglist(AstParser* p)
{
    uint8_t argcount = 0;
    if(!bl_parser_check(p, TOK_RPAREN))
    {
        do
        {
            bl_parser_ignorespace(p);
            bl_parser_parseexpr(p);
            if(argcount == MAX_FUNCTION_PARAMETERS)
            {
                bl_parser_raiseerror(p, "cannot have more than %d arguments to a function", MAX_FUNCTION_PARAMETERS);
            }
            argcount++;
        } while(bl_parser_match(p, TOK_COMMA));
    }
    bl_parser_ignorespace(p);
    bl_parser_consume(p, TOK_RPAREN, "expected ')' after argument list");
    return argcount;
}

static void bl_parser_rulecall(AstParser* p, AstToken previous, bool canassign)
{
    uint8_t argcount;
    (void)previous;
    (void)canassign;
    argcount = bl_parser_parsearglist(p);
    bl_parser_emitbytes(p, OP_CALL, argcount);
}

static void bl_parser_ruleliteral(AstParser* p, bool canassign)
{
    (void)canassign;
    switch(p->previous.type)
    {
        case TOK_NIL:
            bl_parser_emitbyte(p, OP_NIL);
            break;
        case TOK_TRUE:
            bl_parser_emitbyte(p, OP_TRUE);
            break;
        case TOK_FALSE:
            bl_parser_emitbyte(p, OP_FALSE);
            break;
        default:
            return;
    }
}

static void bl_parser_parseassign(AstParser* p, uint8_t realop, uint8_t getop, uint8_t setop, int arg)
{
    p->replcanecho = false;
    if(getop == OP_GET_PROPERTY || getop == OP_GET_SELF_PROPERTY)
    {
        bl_parser_emitbyte(p, OP_DUP);
    }
    if(arg != -1)
    {
        bl_parser_emitbyte_and_short(p, getop, arg);
    }
    else
    {
        bl_parser_emitbytes(p, getop, 1);
    }
    bl_parser_parseexpr(p);
    bl_parser_emitbyte(p, realop);
    if(arg != -1)
    {
        bl_parser_emitbyte_and_short(p, setop, (uint16_t)arg);
    }
    else
    {
        bl_parser_emitbyte(p, setop);
    }
}

static void bl_parser_doassign(AstParser* p, uint8_t getop, uint8_t setop, int arg, bool canassign)
{
    if(canassign && bl_parser_match(p, TOK_EQUAL))
    {
        p->replcanecho = false;
        bl_parser_parseexpr(p);
        if(arg != -1)
        {
            bl_parser_emitbyte_and_short(p, setop, (uint16_t)arg);
        }
        else
        {
            bl_parser_emitbyte(p, setop);
        }
    }
    else if(canassign && bl_parser_match(p, TOK_PLUSEQ))
    {
        bl_parser_parseassign(p, OP_ADD, getop, setop, arg);
    }
    else if(canassign && bl_parser_match(p, TOK_MINUSEQ))
    {
        bl_parser_parseassign(p, OP_SUBTRACT, getop, setop, arg);
    }
    else if(canassign && bl_parser_match(p, TOK_MULTIPLYEQ))
    {
        bl_parser_parseassign(p, OP_MULTIPLY, getop, setop, arg);
    }
    else if(canassign && bl_parser_match(p, TOK_DIVIDEEQ))
    {
        bl_parser_parseassign(p, OP_DIVIDE, getop, setop, arg);
    }
    else if(canassign && bl_parser_match(p, TOK_POWEQ))
    {
        bl_parser_parseassign(p, OP_POW, getop, setop, arg);
    }
    else if(canassign && bl_parser_match(p, TOK_PERCENTEQ))
    {
        bl_parser_parseassign(p, OP_REMINDER, getop, setop, arg);
    }
    else if(canassign && bl_parser_match(p, TOK_FLOOREQ))
    {
        bl_parser_parseassign(p, OP_F_DIVIDE, getop, setop, arg);
    }
    else if(canassign && bl_parser_match(p, TOK_AMPEQ))
    {
        bl_parser_parseassign(p, OP_AND, getop, setop, arg);
    }
    else if(canassign && bl_parser_match(p, TOK_BAREQ))
    {
        bl_parser_parseassign(p, OP_OR, getop, setop, arg);
    }
    else if(canassign && bl_parser_match(p, TOK_TILDEEQ))
    {
        bl_parser_parseassign(p, OP_BIT_NOT, getop, setop, arg);
    }
    else if(canassign && bl_parser_match(p, TOK_XOREQ))
    {
        bl_parser_parseassign(p, OP_XOR, getop, setop, arg);
    }
    else if(canassign && bl_parser_match(p, TOK_LSHIFTEQ))
    {
        bl_parser_parseassign(p, OP_LSHIFT, getop, setop, arg);
    }
    else if(canassign && bl_parser_match(p, TOK_RSHIFTEQ))
    {
        bl_parser_parseassign(p, OP_RSHIFT, getop, setop, arg);
    }
    else if(canassign && bl_parser_match(p, TOK_INCREMENT))
    {
        p->replcanecho = false;
        if(getop == OP_GET_PROPERTY || getop == OP_GET_SELF_PROPERTY)
        {
            bl_parser_emitbyte(p, OP_DUP);
        }
        if(arg != -1)
        {
            bl_parser_emitbyte_and_short(p, getop, arg);
        }
        else
        {
            bl_parser_emitbytes(p, getop, 1);
        }
        bl_parser_emitbytes(p, OP_ONE, OP_ADD);
        bl_parser_emitbyte_and_short(p, setop, (uint16_t)arg);
    }
    else if(canassign && bl_parser_match(p, TOK_DECREMENT))
    {
        p->replcanecho = false;
        if(getop == OP_GET_PROPERTY || getop == OP_GET_SELF_PROPERTY)
        {
            bl_parser_emitbyte(p, OP_DUP);
        }
        if(arg != -1)
        {
            bl_parser_emitbyte_and_short(p, getop, arg);
        }
        else
        {
            bl_parser_emitbytes(p, getop, 1);
        }
        bl_parser_emitbytes(p, OP_ONE, OP_SUBTRACT);
        bl_parser_emitbyte_and_short(p, setop, (uint16_t)arg);
    }
    else
    {
        if(arg != -1)
        {
            if(getop == OP_GET_INDEX || getop == OP_GET_RANGED_INDEX)
            {
                bl_parser_emitbytes(p, getop, (uint8_t)0);
            }
            else
            {
                bl_parser_emitbyte_and_short(p, getop, (uint16_t)arg);
            }
        }
        else
        {
            bl_parser_emitbytes(p, getop, (uint8_t)0);
        }
    }
}

static void bl_parser_ruledot(AstParser* p, AstToken previous, bool canassign)
{
    bl_parser_ignorespace(p);
    bl_parser_consume(p, TOK_IDENTIFIER, "expected property name after '.'");
    int name = bl_parser_identconst(p, &p->previous);
    if(bl_parser_match(p, TOK_LPAREN))
    {
        uint8_t argcount = bl_parser_parsearglist(p);
        if(p->currentclass != NULL && (previous.type == TOK_SELF || bl_parser_identsequal(&p->previous, &p->currentclass->name)))
        {
            bl_parser_emitbyte_and_short(p, OP_INVOKE_SELF, name);
        }
        else
        {
            bl_parser_emitbyte_and_short(p, OP_INVOKE, name);
        }
        bl_parser_emitbyte(p, argcount);
    }
    else
    {
        OpCode getop = OP_GET_PROPERTY;
        OpCode setop = OP_SET_PROPERTY;
        if(p->currentclass != NULL && (previous.type == TOK_SELF || bl_parser_identsequal(&p->previous, &p->currentclass->name)))
        {
            getop = OP_GET_SELF_PROPERTY;
        }
        bl_parser_doassign(p, getop, setop, name, canassign);
    }
}

static void bl_parser_namedvar(AstParser* p, AstToken name, bool canassign)
{
    uint8_t getop;
    uint8_t setop;
    int arg = bl_compiler_resolvelocal(p, p->vm->compiler, &name);
    if(arg != -1)
    {
        getop = OP_GET_LOCAL;
        setop = OP_SET_LOCAL;
    }
    else if((arg = bl_compiler_resolveupvalue(p, p->vm->compiler, &name)) != -1)
    {
        getop = OP_GET_UP_VALUE;
        setop = OP_SET_UP_VALUE;
    }
    else
    {
        arg = bl_parser_identconst(p, &name);
        getop = OP_GET_GLOBAL;
        setop = OP_SET_GLOBAL;
    }
    bl_parser_doassign(p, getop, setop, arg, canassign);
}

static void bl_parser_rulelist(AstParser* p, bool canassign)
{
    int count;
    (void)canassign;
    bl_parser_emitbyte(p, OP_NIL);// placeholder for the list
    count = 0;
    if(!bl_parser_check(p, TOK_RBRACKET))
    {
        do
        {
            bl_parser_ignorespace(p);
            bl_parser_parseexpr(p);
            bl_parser_ignorespace(p);
            count++;
        } while(bl_parser_match(p, TOK_COMMA));
    }
    bl_parser_ignorespace(p);
    bl_parser_consume(p, TOK_RBRACKET, "expected ']' at end of list");
    bl_parser_emitbyte_and_short(p, OP_LIST, count);
}

static void bl_parser_ruledict(AstParser* p, bool canassign)
{
    int itemcount;
    (void)canassign;
    bl_parser_emitbyte(p, OP_NIL);// placeholder for the dictionary
    itemcount = 0;
    if(!bl_parser_check(p, TOK_RBRACE))
    {
        do
        {
            bl_parser_ignorespace(p);
            if(!bl_parser_check(p, TOK_RBRACE))
            {// allow last pair to end with a comma
                if(bl_parser_check(p, TOK_IDENTIFIER))
                {
                    bl_parser_consume(p, TOK_IDENTIFIER, "");
                    bl_parser_emitconstant(p, OBJ_VAL(bl_string_copystring(p->vm, p->previous.start, p->previous.length)));
                }
                else
                {
                    bl_parser_parseexpr(p);
                }
                bl_parser_ignorespace(p);
                bl_parser_consume(p, TOK_COLON, "expected ':' after dictionary key");
                bl_parser_ignorespace(p);
                bl_parser_parseexpr(p);
                itemcount++;
            }
        } while(bl_parser_match(p, TOK_COMMA));
    }
    bl_parser_ignorespace(p);
    bl_parser_consume(p, TOK_RBRACE, "expected '}' after dictionary");
    bl_parser_emitbyte_and_short(p, OP_DICT, itemcount);
}

static void bl_parser_ruleindexing(AstParser* p, AstToken previous, bool canassign)
{
    uint8_t getop;
    bool assignable;
    bool commamatch;
    (void)previous;
    (void)canassign;
    assignable = true;
    commamatch = false;
    getop = OP_GET_INDEX;
    if(bl_parser_match(p, TOK_COMMA))
    {
        bl_parser_emitbyte(p, OP_NIL);
        commamatch = true;
        getop = OP_GET_RANGED_INDEX;
    }
    else
    {
        bl_parser_parseexpr(p);
    }
    if(!bl_parser_match(p, TOK_RBRACKET))
    {
        getop = OP_GET_RANGED_INDEX;
        if(!commamatch)
        {
            bl_parser_consume(p, TOK_COMMA, "expecting ',' or ']'");
        }
        if(bl_parser_match(p, TOK_RBRACKET))
        {
            bl_parser_emitbyte(p, OP_NIL);
        }
        else
        {
            bl_parser_parseexpr(p);
            bl_parser_consume(p, TOK_RBRACKET, "expected ']' after indexing");
        }
        assignable = false;
    }
    else
    {
        if(commamatch)
        {
            bl_parser_emitbyte(p, OP_NIL);
        }
    }
    bl_parser_doassign(p, getop, OP_SET_INDEX, -1, assignable);
}

static void bl_parser_rulevariable(AstParser* p, bool canassign)
{
    bl_parser_namedvar(p, p->previous, canassign);
}

static void bl_parser_ruleself(AstParser* p, bool canassign)
{
    (void)canassign;
    if(p->currentclass == NULL)
    {
        bl_parser_raiseerror(p, "cannot use keyword 'self' outside of a class");
        return;
    }
    bl_parser_rulevariable(p, false);
}

static void bl_parser_ruleparent(AstParser* p, bool canassign)
{
    int name;
    bool invself;
    (void)canassign;
    if(p->currentclass == NULL)
    {
        bl_parser_raiseerror(p, "cannot use keyword 'parent' outside of a class");
    }
    else if(!p->currentclass->hassuperclass)
    {
        bl_parser_raiseerror(p, "cannot use keyword 'parent' in a class without a parent");
    }
    name = -1;
    invself = false;
    if(!bl_parser_check(p, TOK_LPAREN))
    {
        bl_parser_consume(p, TOK_DOT, "expected '.' or '(' after parent");
        bl_parser_consume(p, TOK_IDENTIFIER, "expected parent class method name after .");
        name = bl_parser_identconst(p, &p->previous);
    }
    else
    {
        invself = true;
    }
    bl_parser_namedvar(p, bl_parser_synthtoken("self"), false);
    if(bl_parser_match(p, TOK_LPAREN))
    {
        uint8_t argcount = bl_parser_parsearglist(p);
        bl_parser_namedvar(p, bl_parser_synthtoken("parent"), false);
        if(!invself)
        {
            bl_parser_emitbyte_and_short(p, OP_SUPER_INVOKE, name);
            bl_parser_emitbyte(p, argcount);
        }
        else
        {
            bl_parser_emitbytes(p, OP_SUPER_INVOKE_SELF, argcount);
        }
    }
    else
    {
        bl_parser_namedvar(p, bl_parser_synthtoken("parent"), false);
        bl_parser_emitbyte_and_short(p, OP_GET_SUPER, name);
    }
}

static void bl_parser_rulegrouping(AstParser* p, bool canassign)
{
    (void)canassign;
    bl_parser_ignorespace(p);
    bl_parser_parseexpr(p);
    bl_parser_ignorespace(p);
    bl_parser_consume(p, TOK_RPAREN, "expected ')' after grouped expression");
}

static Value bl_parser_compilenumber(AstParser* p)
{
    if(p->previous.type == TOK_BINNUMBER)
    {
        long long value = strtoll(p->previous.start + 2, NULL, 2);
        return NUMBER_VAL(value);
    }
    else if(p->previous.type == TOK_OCTNUMBER)
    {
        long value = strtol(p->previous.start + 2, NULL, 8);
        return NUMBER_VAL(value);
    }
    else if(p->previous.type == TOK_HEXNUMBER)
    {
        long value = strtol(p->previous.start, NULL, 16);
        return NUMBER_VAL(value);
    }
    else
    {
        double value = strtod(p->previous.start, NULL);
        return NUMBER_VAL(value);
    }
}

static void bl_parser_rulenumber(AstParser* p, bool canassign)
{
    (void)canassign;
    bl_parser_emitconstant(p, bl_parser_compilenumber(p));
}

// Reads the next character, which should be a hex digit (0-9, a-f, or A-F) and
// returns its numeric value. If the character isn't a hex digit, returns -1.
static int bl_parser_readhexdigit(char c)
{
    if(c >= '0' && c <= '9')
    {
        return c - '0';
    }
    if(c >= 'a' && c <= 'f')
    {
        return c - 'a' + 10;
    }
    if(c >= 'A' && c <= 'F')
    {
        return c - 'A' + 10;
    }
    return -1;
}

// Reads [digits] hex digits in a string literal and returns their number value.
static int bl_parser_readhexescape(AstParser* p, char* str, int index, int count)
{
    int value = 0;
    int i = 0;
    int digit = 0;
    for(; i < count; i++)
    {
        digit = bl_parser_readhexdigit(str[index + i + 2]);
        if(digit == -1)
        {
            bl_parser_raiseerror(p, "invalid escape sequence");
        }
        value = (value * 16) | digit;
    }
    if(count == 4 && (digit = bl_parser_readhexdigit(str[index + i + 2])) != -1)
    {
        value = (value * 16) | digit;
    }
    return value;
}

static int bl_parser_readunicodeescape(AstParser* p, char* string, char* realstring, int numberbytes, int realindex, int index)
{
    int value = bl_parser_readhexescape(p, realstring, realindex, numberbytes);
    int count = bl_util_utf8numbytes(value);
    if(count == -1)
    {
        bl_parser_raiseerror(p, "cannot encode a negative unicode value");
    }
    if(value > 65535)
    {// check for greater that \uffff
        count++;
    }
    if(count != 0)
    {
        memcpy(string + index, bl_util_utf8encode(value), (size_t)count + 1);
    }
    /* if (value > 65535) // but greater than \uffff doesn't occupy any extra byte
    count--; */
    return count;
}

static char* bl_parser_compilestring(AstParser* p, int* length)
{
    int k;
    int i;
    int count;
    int reallength;
    char c;
    char* str;
    char* real;
    str = (char*)malloc((((size_t)p->previous.length - 2) + 1) * sizeof(char));
    real = (char*)p->previous.start + 1;
    reallength = p->previous.length - 2;
    k = 0;
    for(i = 0; i < reallength; i++, k++)
    {
        c = real[i];
        if(c == '\\' && i < reallength - 1)
        {
            switch(real[i + 1])
            {
                case '0':
                    {
                        c = '\0';
                    }
                    break;
                case '$':
                    {
                        c = '$';
                    }
                    break;
                case '\'':
                    {
                        c = '\'';
                    }
                    break;
                case '"':
                    {
                        c = '"';
                    }
                    break;
                case 'a':
                    {
                        c = '\a';
                    }
                    break;
                case 'b':
                    {
                        c = '\b';
                    }
                    break;
                case 'f':
                    {
                        c = '\f';
                    }
                    break;
                case 'n':
                    {
                        c = '\n';
                    }
                    break;
                case 'r':
                    {
                        c = '\r';
                    }
                    break;
                case 't':
                    {
                        c = '\t';
                    }
                    break;
                case '\\':
                    {
                        c = '\\';
                    }
                    break;
                case 'v':
                    {
                        c = '\v';
                    }
                    break;
                case 'x':
                    {
                        k += bl_parser_readunicodeescape(p, str, real, 2, i, k) - 1;
                        i += 3;
                    }
                    continue;
                case 'u':
                    {
                        count = bl_parser_readunicodeescape(p, str, real, 4, i, k);
                        k += count > 4 ? count - 2 : count - 1;
                        i += count > 4 ? 6 : 5;
                    }
                    continue;
                case 'U':
                    {
                        count = bl_parser_readunicodeescape(p, str, real, 8, i, k);
                        k += count > 4 ? count - 2 : count - 1;
                        i += 9;
                    }
                    continue;
                default:
                    {
                        i--;
                    }
                    break;
            }
            i++;
        }
        memcpy(str + k, &c, 1);
    }
    *length = k;
    str[k] = '\0';
    return str;
}

static void bl_parser_rulestring(AstParser* p, bool canassign)
{
    int length;
    char* str;
    (void)canassign;
    str = bl_parser_compilestring(p, &length);
    bl_parser_emitconstant(p, OBJ_VAL(bl_string_takestring(p->vm, str, length)));
}

static void bl_parser_rulestrinterpol(AstParser* p, bool canassign)
{
    int count = 0;
    do
    {
        bool doadd = false;
        bool stringmatched = false;
        if(p->previous.length - 2 > 0)
        {
            bl_parser_rulestring(p, canassign);
            doadd = true;
            stringmatched = true;
            if(count > 0)
            {
                bl_parser_emitbyte(p, OP_ADD);
            }
        }
        bl_parser_parseexpr(p);
        bl_parser_emitbyte(p, OP_STRINGIFY);
        if(doadd || (count >= 1 && stringmatched == false))
        {
            bl_parser_emitbyte(p, OP_ADD);
        }
        count++;
    } while(bl_parser_match(p, TOK_INTERPOLATION));
    bl_parser_consume(p, TOK_LITERAL, "unterminated string interpolation");
    if(p->previous.length - 2 > 0)
    {
        bl_parser_rulestring(p, canassign);
        bl_parser_emitbyte(p, OP_ADD);
    }
}

static void bl_parser_ruleunary(AstParser* p, bool canassign)
{
    TokType op;
    (void)canassign;
    op = p->previous.type;
    // compile the expression
    bl_parser_parseprecedence(p, PREC_UNARY);
    // emit instruction
    switch(op)
    {
        case TOK_MINUS:
            bl_parser_emitbyte(p, OP_NEGATE);
            break;
        case TOK_BANG:
            bl_parser_emitbyte(p, OP_NOT);
            break;
        case TOK_TILDE:
            bl_parser_emitbyte(p, OP_BIT_NOT);
            break;
        default:
            break;
    }
}

static void bl_parser_ruleand(AstParser* p, AstToken previous, bool canassign)
{
    int endjump;
    (void)previous;
    (void)canassign;
    endjump = bl_parser_emitjump(p, OP_JUMP_IF_FALSE);
    bl_parser_emitbyte(p, OP_POP);
    bl_parser_parseprecedence(p, PREC_AND);
    bl_parser_patchjump(p, endjump);
}

static void bl_parser_ruleor(AstParser* p, AstToken previous, bool canassign)
{
    int elsejump;
    int endjump;
    (void)previous;
    (void)canassign;
    elsejump = bl_parser_emitjump(p, OP_JUMP_IF_FALSE);
    endjump = bl_parser_emitjump(p, OP_JUMP);
    bl_parser_patchjump(p, elsejump);
    bl_parser_emitbyte(p, OP_POP);
    bl_parser_parseprecedence(p, PREC_OR);
    bl_parser_patchjump(p, endjump);
}

static void bl_parser_ruleconditional(AstParser* p, AstToken previous, bool canassign)
{
    (void)previous;
    (void)canassign;
    int thenjump = bl_parser_emitjump(p, OP_JUMP_IF_FALSE);
    bl_parser_emitbyte(p, OP_POP);
    bl_parser_ignorespace(p);
    // compile the then expression
    bl_parser_parseprecedence(p, PREC_CONDITIONAL);
    bl_parser_ignorespace(p);
    int elsejump = bl_parser_emitjump(p, OP_JUMP);
    bl_parser_patchjump(p, thenjump);
    bl_parser_emitbyte(p, OP_POP);
    bl_parser_consume(p, TOK_COLON, "expected matching ':' after '?' conditional");
    bl_parser_ignorespace(p);
    // compile the else expression
    // here we parse at PREC_ASSIGNMENT precedence as
    // linear conditionals can be nested.
    bl_parser_parseprecedence(p, PREC_ASSIGNMENT);
    bl_parser_patchjump(p, elsejump);
}

static void bl_parser_ruleanon(AstParser* p, bool canassign)
{
    AstCompiler compiler;
    (void)canassign;
    bl_compiler_init(p, &compiler, TYPE_FUNCTION);
    bl_parser_beginscope(p);
    // compile parameter list
    if(!bl_parser_check(p, TOK_BAR))
    {
        bl_parser_parsefunctionargs(p);
    }
    bl_parser_consume(p, TOK_BAR, "expected '|' after anonymous function parameters");
    bl_parser_parsefunctionbody(p, &compiler);
}



static void bl_parser_doparseprecedence(AstParser* p, AstPrecedence precedence)
{
    AstToken previous;
    bparseprefixfn prefix_rule = bl_parser_getrule(p->previous.type)->prefix;
    if(prefix_rule == NULL)
    {
        bl_parser_raiseerror(p, "expected expression");
        return;
    }
    bool canassign = precedence <= PREC_ASSIGNMENT;
    prefix_rule(p, canassign);
    while(precedence <= bl_parser_getrule(p->current.type)->precedence)
    {
        previous = p->previous;
        bl_parser_ignorespace(p);
        bl_parser_advance(p);
        bparseinfixfn infix_rule = bl_parser_getrule(p->previous.type)->infix;
        infix_rule(p, previous, canassign);
    }
    if(canassign && bl_parser_match(p, TOK_EQUAL))
    {
        bl_parser_raiseerror(p, "invalid assignment target");
    }
}

static void bl_parser_parseprecedence(AstParser* p, AstPrecedence precedence)
{
    if(bl_scanner_isatend(p->scanner) && p->vm->isrepl)
    {
        return;
    }
    bl_parser_ignorespace(p);
    if(bl_scanner_isatend(p->scanner) && p->vm->isrepl)
    {
        return;
    }
    bl_parser_advance(p);
    bl_parser_doparseprecedence(p, precedence);
}

static void bl_parser_parseprecnoadvance(AstParser* p, AstPrecedence precedence)
{
    if(bl_scanner_isatend(p->scanner) && p->vm->isrepl)
    {
        return;
    }
    bl_parser_ignorespace(p);
    if(bl_scanner_isatend(p->scanner) && p->vm->isrepl)
    {
        return;
    }
    bl_parser_doparseprecedence(p, precedence);
}

static inline AstRule* bl_parser_makerule(AstRule* dest, bparseprefixfn prefix, bparseinfixfn infix, AstPrecedence precedence)
{
    dest->prefix = prefix;
    dest->infix = infix;
    dest->precedence = precedence;
    return dest;
}

static AstRule* bl_parser_getrule(TokType type)
{
    #if 0
    // clang-format off
    static AstRule parserules[] = {
        // symbols
        [TOK_NEWLINE] = { NULL, NULL, PREC_NONE },// (
        [TOK_LPAREN] = { bl_parser_rulegrouping, bl_parser_rulecall, PREC_CALL },// (
        [TOK_RPAREN] = { NULL, NULL, PREC_NONE },// )
        [TOK_LBRACKET] = { bl_parser_rulelist, bl_parser_ruleindexing, PREC_CALL },// [
        [TOK_RBRACKET] = { NULL, NULL, PREC_NONE },// ]
        [TOK_LBRACE] = { bl_parser_ruledict, NULL, PREC_NONE },// {
        [TOK_RBRACE] = { NULL, NULL, PREC_NONE },// }
        [TOK_SEMICOLON] = { NULL, NULL, PREC_NONE },// ;
        [TOK_COMMA] = { NULL, NULL, PREC_NONE },// ,
        [TOK_BACKSLASH] = { NULL, NULL, PREC_NONE },// '\'
        [TOK_BANG] = { bl_parser_ruleunary, NULL, PREC_NONE },// !
        [TOK_BANGEQ] = { NULL, bl_parser_rulebinary, PREC_EQUALITY },// !=
        [TOK_COLON] = { NULL, NULL, PREC_NONE },// :
        [TOK_AT] = { NULL, NULL, PREC_NONE },// @
        [TOK_DOT] = { NULL, bl_parser_ruledot, PREC_CALL },// .
        [TOK_RANGE] = { NULL, bl_parser_rulebinary, PREC_RANGE },// ..
        [TOK_TRIDOT] = { NULL, NULL, PREC_NONE },// ...
        [TOK_PLUS] = { bl_parser_ruleunary, bl_parser_rulebinary, PREC_TERM },// +
        [TOK_PLUSEQ] = { NULL, NULL, PREC_NONE },// +=
        [TOK_INCREMENT] = { NULL, NULL, PREC_NONE },// ++
        [TOK_MINUS] = { bl_parser_ruleunary, bl_parser_rulebinary, PREC_TERM },// -
        [TOK_MINUSEQ] = { NULL, NULL, PREC_NONE },// -=
        [TOK_DECREMENT] = { NULL, NULL, PREC_NONE },// --
        [TOK_MULTIPLY] = { NULL, bl_parser_rulebinary, PREC_FACTOR },// *
        [TOK_MULTIPLYEQ] = { NULL, NULL, PREC_NONE },// *=
        [TOK_POW] = { NULL, bl_parser_rulebinary, PREC_FACTOR },// **
        [TOK_POWEQ] = { NULL, NULL, PREC_NONE },// **=
        [TOK_DIVIDE] = { NULL, bl_parser_rulebinary, PREC_FACTOR },// '/'
        [TOK_DIVIDEEQ] = { NULL, NULL, PREC_NONE },// '/='
        [TOK_FLOOR] = { NULL, bl_parser_rulebinary, PREC_FACTOR },// '//'
        [TOK_FLOOREQ] = { NULL, NULL, PREC_NONE },// '//='
        [TOK_EQUAL] = { NULL, NULL, PREC_NONE },// =
        [TOK_EQUALEQ] = { NULL, bl_parser_rulebinary, PREC_EQUALITY },// ==
        [TOK_LESS] = { NULL, bl_parser_rulebinary, PREC_COMPARISON },// <
        [TOK_LESSEQ] = { NULL, bl_parser_rulebinary, PREC_COMPARISON },// <=
        [TOK_LSHIFT] = { NULL, bl_parser_rulebinary, PREC_SHIFT },// <<
        [TOK_LSHIFTEQ] = { NULL, NULL, PREC_NONE },// <<=
        [TOK_GREATER] = { NULL, bl_parser_rulebinary, PREC_COMPARISON },// >
        [TOK_GREATEREQ] = { NULL, bl_parser_rulebinary, PREC_COMPARISON },// >=
        [TOK_RSHIFT] = { NULL, bl_parser_rulebinary, PREC_SHIFT },// >>
        [TOK_RSHIFTEQ] = { NULL, NULL, PREC_NONE },// >>=
        [TOK_PERCENT] = { NULL, bl_parser_rulebinary, PREC_FACTOR },// %
        [TOK_PERCENTEQ] = { NULL, NULL, PREC_NONE },// %=
        [TOK_AMP] = { NULL, bl_parser_rulebinary, PREC_BIT_AND },// &
        [TOK_AMPEQ] = { NULL, NULL, PREC_NONE },// &=
        [TOK_BAR] = { bl_parser_ruleanon, bl_parser_rulebinary, PREC_BIT_OR },// |
        [TOK_BAREQ] = { NULL, NULL, PREC_NONE },// |=
        [TOK_TILDE] = { bl_parser_ruleunary, NULL, PREC_UNARY },// ~
        [TOK_TILDEEQ] = { NULL, NULL, PREC_NONE },// ~=
        [TOK_XOR] = { NULL, bl_parser_rulebinary, PREC_BIT_XOR },// ^
        [TOK_XOREQ] = { NULL, NULL, PREC_NONE },// ^=
        [TOK_QUESTION] = { NULL, bl_parser_ruleconditional, PREC_CONDITIONAL },// ??
        // keywords
        [TOK_AND] = { NULL, bl_parser_ruleand, PREC_AND },
        [TOK_AS] = { NULL, NULL, PREC_NONE },
        [TOK_ASSERT] = { NULL, NULL, PREC_NONE },
        [TOK_BREAK] = { NULL, NULL, PREC_NONE },
        [TOK_CLASS] = { NULL, NULL, PREC_NONE },
        [TOK_CONTINUE] = { NULL, NULL, PREC_NONE },
        [TOK_DEF] = { NULL, NULL, PREC_NONE },
        [TOK_DEFAULT] = { NULL, NULL, PREC_NONE },
        [TOK_DIE] = { NULL, NULL, PREC_NONE },
        [TOK_DO] = { NULL, NULL, PREC_NONE },
        [TOK_ECHO] = { NULL, NULL, PREC_NONE },
        [TOK_ELSE] = { NULL, NULL, PREC_NONE },
        [TOK_FALSE] = { bl_parser_ruleliteral, NULL, PREC_NONE },
        [TOK_FOREACH] = { NULL, NULL, PREC_NONE },
        [TOK_IF] = { NULL, NULL, PREC_NONE },
        [TOK_IMPORT] = { NULL, NULL, PREC_NONE },
        [TOK_IN] = { NULL, NULL, PREC_NONE },
        [TOK_FORLOOP] = { NULL, NULL, PREC_NONE },
        [TOK_VAR] = { NULL, NULL, PREC_NONE },
        [TOK_NIL] = { bl_parser_ruleliteral, NULL, PREC_NONE },
        [TOK_OR] = { NULL, bl_parser_ruleor, PREC_OR },
        [TOK_PARENT] = { bl_parser_ruleparent, NULL, PREC_NONE },
        [TOK_RETURN] = { NULL, NULL, PREC_NONE },
        [TOK_SELF] = { bl_parser_ruleself, NULL, PREC_NONE },
        [TOK_STATIC] = { NULL, NULL, PREC_NONE },
        [TOK_TRUE] = { bl_parser_ruleliteral, NULL, PREC_NONE },
        [TOK_USING] = { NULL, NULL, PREC_NONE },
        [TOK_WHEN] = { NULL, NULL, PREC_NONE },
        [TOK_WHILE] = { NULL, NULL, PREC_NONE },
        [TOK_TRY] = { NULL, NULL, PREC_NONE },
        [TOK_CATCH] = { NULL, NULL, PREC_NONE },
        [TOK_FINALLY] = { NULL, NULL, PREC_NONE },
        // types token
        [TOK_LITERAL] = { bl_parser_rulestring, NULL, PREC_NONE },
        [TOK_REGNUMBER] = { bl_parser_rulenumber, NULL, PREC_NONE },// regular numbers
        [TOK_BINNUMBER] = { bl_parser_rulenumber, NULL, PREC_NONE },// binary numbers
        [TOK_OCTNUMBER] = { bl_parser_rulenumber, NULL, PREC_NONE },// octal numbers
        [TOK_HEXNUMBER] = { bl_parser_rulenumber, NULL, PREC_NONE },// hexadecimal numbers
        [TOK_IDENTIFIER] = { bl_parser_rulevariable, NULL, PREC_NONE },
        [TOK_INTERPOLATION] = { bl_parser_rulestrinterpol, NULL, PREC_NONE },
        [TOK_EOF] = { NULL, NULL, PREC_NONE },
        // error
        [TOK_ERROR] = { NULL, NULL, PREC_NONE },
        [TOK_EMPTY] = { bl_parser_ruleliteral, NULL, PREC_NONE },
        [TOK_UNDEFINED] = { NULL, NULL, PREC_NONE },
    };
    // clang-format on
    return &parserules[type];
    #else
    static AstRule rule;
    switch(type)
    {
        // symbols
        // (
        case TOK_NEWLINE:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        // (
        case TOK_LPAREN:
            return bl_parser_makerule(&rule, bl_parser_rulegrouping, bl_parser_rulecall, PREC_CALL);
            break;
        // )
        case TOK_RPAREN:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        // [
        case TOK_LBRACKET:
            return bl_parser_makerule(&rule, bl_parser_rulelist, bl_parser_ruleindexing, PREC_CALL);
            break;
        // ]
        case TOK_RBRACKET:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        // {
        case TOK_LBRACE:
            return bl_parser_makerule(&rule, bl_parser_ruledict, NULL, PREC_NONE);
            break;
        // }
        case TOK_RBRACE:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        // ;
        case TOK_SEMICOLON:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        // ,
        case TOK_COMMA:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        // ''
        case TOK_BACKSLASH:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        // !
        case TOK_BANG:
            return bl_parser_makerule(&rule, bl_parser_ruleunary, NULL, PREC_NONE);
            break;
        // !=
        case TOK_BANGEQ:
            return bl_parser_makerule(&rule, NULL, bl_parser_rulebinary, PREC_EQUALITY);
            break;
        // :
        case TOK_COLON:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        // @
        case TOK_AT:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        // .
        case TOK_DOT:
            return bl_parser_makerule(&rule, NULL, bl_parser_ruledot, PREC_CALL);
            break;
        // ..
        case TOK_RANGE:
            return bl_parser_makerule(&rule, NULL, bl_parser_rulebinary, PREC_RANGE);
            break;
        // ...
        case TOK_TRIDOT:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        // +
        case TOK_PLUS:
            return bl_parser_makerule(&rule, bl_parser_ruleunary, bl_parser_rulebinary, PREC_TERM);
            break;
        // +=
        case TOK_PLUSEQ:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        // ++
        case TOK_INCREMENT:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        // -
        case TOK_MINUS:
            return bl_parser_makerule(&rule, bl_parser_ruleunary, bl_parser_rulebinary, PREC_TERM);
            break;
        // -=
        case TOK_MINUSEQ:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        // --
        case TOK_DECREMENT:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        // *
        case TOK_MULTIPLY:
            return bl_parser_makerule(&rule, NULL, bl_parser_rulebinary, PREC_FACTOR);
            break;
        // *=
        case TOK_MULTIPLYEQ:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        // **
        case TOK_POW:
            return bl_parser_makerule(&rule, NULL, bl_parser_rulebinary, PREC_FACTOR);
            break;
        // **=
        case TOK_POWEQ:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        // '/'
        case TOK_DIVIDE:
            return bl_parser_makerule(&rule, NULL, bl_parser_rulebinary, PREC_FACTOR);
            break;
        // '/='
        case TOK_DIVIDEEQ:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        // '//'
        case TOK_FLOOR:
            return bl_parser_makerule(&rule, NULL, bl_parser_rulebinary, PREC_FACTOR);
            break;
        // '//='
        case TOK_FLOOREQ:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        // =
        case TOK_EQUAL:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        // ==
        case TOK_EQUALEQ:
            return bl_parser_makerule(&rule, NULL, bl_parser_rulebinary, PREC_EQUALITY);
            break;
        // <
        case TOK_LESS:
            return bl_parser_makerule(&rule, NULL, bl_parser_rulebinary, PREC_COMPARISON);
            break;
        // <=
        case TOK_LESSEQ:
            return bl_parser_makerule(&rule, NULL, bl_parser_rulebinary, PREC_COMPARISON);
            break;
        // <<
        case TOK_LSHIFT:
            return bl_parser_makerule(&rule, NULL, bl_parser_rulebinary, PREC_SHIFT);
            break;
        // <<=
        case TOK_LSHIFTEQ:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        // >
        case TOK_GREATER:
            return bl_parser_makerule(&rule, NULL, bl_parser_rulebinary, PREC_COMPARISON);
            break;
        // >=
        case TOK_GREATEREQ:
            return bl_parser_makerule(&rule, NULL, bl_parser_rulebinary, PREC_COMPARISON);
            break;
        // >>
        case TOK_RSHIFT:
            return bl_parser_makerule(&rule, NULL, bl_parser_rulebinary, PREC_SHIFT);
            break;
        // >>=
        case TOK_RSHIFTEQ:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        // %
        case TOK_PERCENT:
            return bl_parser_makerule(&rule, NULL, bl_parser_rulebinary, PREC_FACTOR);
            break;
        // %=
        case TOK_PERCENTEQ:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        // &
        case TOK_AMP:
            return bl_parser_makerule(&rule, NULL, bl_parser_rulebinary, PREC_BIT_AND);
            break;
        // &=
        case TOK_AMPEQ:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        // |
        case TOK_BAR:
            return bl_parser_makerule(&rule, bl_parser_ruleanon, bl_parser_rulebinary, PREC_BIT_OR);
            break;
        // |=
        case TOK_BAREQ:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        // ~
        case TOK_TILDE:
            return bl_parser_makerule(&rule, bl_parser_ruleunary, NULL, PREC_UNARY);
            break;
        // ~=
        case TOK_TILDEEQ:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        // ^
        case TOK_XOR:
            return bl_parser_makerule(&rule, NULL, bl_parser_rulebinary, PREC_BIT_XOR);
            break;
        // ^=
        case TOK_XOREQ:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        // ??
        case TOK_QUESTION:
            return bl_parser_makerule(&rule, NULL, bl_parser_ruleconditional, PREC_CONDITIONAL);
            break;
        // keywords
        case TOK_AND:
            return bl_parser_makerule(&rule, NULL, bl_parser_ruleand, PREC_AND);
            break;
        case TOK_AS:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        case TOK_ASSERT:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        case TOK_BREAK:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        case TOK_CLASS:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        case TOK_CONTINUE:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        case TOK_DEF:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        case TOK_DEFAULT:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        case TOK_DIE:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        case TOK_DO:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        case TOK_ECHO:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        case TOK_ELSE:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        case TOK_FALSE:
            return bl_parser_makerule(&rule, bl_parser_ruleliteral, NULL, PREC_NONE);
            break;
        case TOK_FOREACH:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        case TOK_IF:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        case TOK_IMPORT:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        case TOK_IN:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        case TOK_FORLOOP:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        case TOK_VAR:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        case TOK_NIL:
            return bl_parser_makerule(&rule, bl_parser_ruleliteral, NULL, PREC_NONE);
            break;
        case TOK_OR:
            return bl_parser_makerule(&rule, NULL, bl_parser_ruleor, PREC_OR);
            break;
        case TOK_PARENT:
            return bl_parser_makerule(&rule, bl_parser_ruleparent, NULL, PREC_NONE);
            break;
        case TOK_RETURN:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        case TOK_SELF:
            return bl_parser_makerule(&rule, bl_parser_ruleself, NULL, PREC_NONE);
            break;
        case TOK_STATIC:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        case TOK_TRUE:
            return bl_parser_makerule(&rule, bl_parser_ruleliteral, NULL, PREC_NONE);
            break;
        case TOK_USING:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        case TOK_WHEN:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        case TOK_WHILE:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        case TOK_TRY:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        case TOK_CATCH:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        case TOK_FINALLY:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        // types token
        case TOK_LITERAL:
            return bl_parser_makerule(&rule, bl_parser_rulestring, NULL, PREC_NONE);
            break;
        // regular numbers
        case TOK_REGNUMBER:
            return bl_parser_makerule(&rule, bl_parser_rulenumber, NULL, PREC_NONE);
            break;
        // binary numbers
        case TOK_BINNUMBER:
            return bl_parser_makerule(&rule, bl_parser_rulenumber, NULL, PREC_NONE);
            break;
        // octal numbers
        case TOK_OCTNUMBER:
            return bl_parser_makerule(&rule, bl_parser_rulenumber, NULL, PREC_NONE);
            break;
        // hexadecimal numbers
        case TOK_HEXNUMBER:
            return bl_parser_makerule(&rule, bl_parser_rulenumber, NULL, PREC_NONE);
            break;
        case TOK_IDENTIFIER:
            return bl_parser_makerule(&rule, bl_parser_rulevariable, NULL, PREC_NONE);
            break;
        case TOK_INTERPOLATION:
            return bl_parser_makerule(&rule, bl_parser_rulestrinterpol, NULL, PREC_NONE);
            break;
        case TOK_EOF:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        // error
        case TOK_ERROR:
            return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
            break;
        case TOK_EMPTY:
            return bl_parser_makerule(&rule, bl_parser_ruleliteral, NULL, PREC_NONE);
            break;
        default:
            break;
    }
    return bl_parser_makerule(&rule, NULL, NULL, PREC_NONE);
    #endif
}

static void bl_parser_parseexpr(AstParser* p)
{
    bl_parser_parseprecedence(p, PREC_ASSIGNMENT);
}

static void bl_parser_parseblock(AstParser* p)
{
    p->blockcount++;
    bl_parser_ignorespace(p);
    while(!bl_parser_check(p, TOK_RBRACE) && !bl_parser_check(p, TOK_EOF))
    {
        bl_parser_parsedeclaration(p);
    }
    p->blockcount--;
    bl_parser_consume(p, TOK_RBRACE, "expected '}' after block");
}

static void bl_parser_parsefunctionargs(AstParser* p)
{
    // compile argument list...
    do
    {
        bl_parser_ignorespace(p);
        p->vm->compiler->currfunc->arity++;
        if(p->vm->compiler->currfunc->arity > MAX_FUNCTION_PARAMETERS)
        {
            bl_parser_raiseerroratcurrent(p, "cannot have more than %d function parameters", MAX_FUNCTION_PARAMETERS);
        }
        if(bl_parser_match(p, TOK_TRIDOT))
        {
            p->vm->compiler->currfunc->isvariadic = true;
            bl_parser_addlocal(p, bl_parser_synthtoken("__args__"));
            bl_parser_defvar(p, 0);
            break;
        }
        int paramconstant = bl_parser_parsevar(p, "expected parameter name");
        bl_parser_defvar(p, paramconstant);
        bl_parser_ignorespace(p);
    } while(bl_parser_match(p, TOK_COMMA));
}

static void bl_parser_parsefunctionbody(AstParser* p, AstCompiler* compiler)
{
    // compile the body
    bl_parser_ignorespace(p);
    bl_parser_consume(p, TOK_LBRACE, "expected '{' before function body");
    bl_parser_parseblock(p);
    // create the function object
    ObjFunction* function = bl_compiler_end(p);
    bl_parser_emitbyte_and_short(p, OP_CLOSURE, bl_parser_makeconstant(p, OBJ_VAL(function)));
    for(int i = 0; i < function->upvaluecount; i++)
    {
        bl_parser_emitbyte(p, compiler->upvalues[i].islocal ? 1 : 0);
        bl_parser_emitshort(p, compiler->upvalues[i].index);
    }
}

static void bl_parser_parsefunction(AstParser* p, FuncType type)
{
    AstCompiler compiler;
    bl_compiler_init(p, &compiler, type);
    bl_parser_beginscope(p);
    // compile parameter list
    bl_parser_consume(p, TOK_LPAREN, "expected '(' after function name");
    if(!bl_parser_check(p, TOK_RPAREN))
    {
        bl_parser_parsefunctionargs(p);
    }
    bl_parser_consume(p, TOK_RPAREN, "expected ')' after function parameters");
    bl_parser_parsefunctionbody(p, &compiler);
}

static void bl_parser_parsemethod(AstParser* p, AstToken classname, bool isstatic)
{
    TokType tkns[] = { TOK_IDENTIFIER, TOK_DECORATOR };
    bl_parser_consumeor(p, "method name expected", tkns, 2);
    int constant = bl_parser_identconst(p, &p->previous);
    FuncType type = isstatic ? TYPE_STATIC : TYPE_METHOD;
    if(p->previous.length == classname.length && memcmp(p->previous.start, classname.start, classname.length) == 0)
    {
        type = TYPE_INITIALIZER;
    }
    else if(p->previous.length > 0 && p->previous.start[0] == '_')
    {
        type = TYPE_PRIVATE;
    }
    bl_parser_parsefunction(p, type);
    bl_parser_emitbyte_and_short(p, OP_METHOD, constant);
}

static void bl_parser_parsefield(AstParser* p, bool isstatic)
{
    bl_parser_consume(p, TOK_IDENTIFIER, "class property name expected");
    int fieldconstant = bl_parser_identconst(p, &p->previous);
    if(bl_parser_match(p, TOK_EQUAL))
    {
        bl_parser_parseexpr(p);
    }
    else
    {
        bl_parser_emitbyte(p, OP_NIL);
    }
    bl_parser_emitbyte_and_short(p, OP_CLASS_PROPERTY, fieldconstant);
    bl_parser_emitbyte(p, isstatic ? 1 : 0);
    bl_parser_consumestmtend(p);
    bl_parser_ignorespace(p);
}

static void bl_parser_parsefunctiondecl(AstParser* p)
{
    int global = bl_parser_parsevar(p, "function name expected");
    bl_parser_markinit(p);
    bl_parser_parsefunction(p, TYPE_FUNCTION);
    bl_parser_defvar(p, global);
}

static void bl_parser_parseclassdecl(AstParser* p)
{
    bl_parser_consume(p, TOK_IDENTIFIER, "class name expected");
    int nameconstant = bl_parser_identconst(p, &p->previous);
    AstToken classname = p->previous;
    bl_parser_declvar(p);
    bl_parser_emitbyte_and_short(p, OP_CLASS, nameconstant);
    bl_parser_defvar(p, nameconstant);
    AstClassCompiler classcompiler;
    classcompiler.name = p->previous;
    classcompiler.hassuperclass = false;
    classcompiler.enclosing = p->currentclass;
    p->currentclass = &classcompiler;
    if(bl_parser_match(p, TOK_LESS))
    {
        bl_parser_consume(p, TOK_IDENTIFIER, "name of superclass expected");
        bl_parser_rulevariable(p, false);
        if(bl_parser_identsequal(&classname, &p->previous))
        {
            bl_parser_raiseerror(p, "class %.*s cannot inherit from itself", classname.length, classname.start);
        }
        bl_parser_beginscope(p);
        bl_parser_addlocal(p, bl_parser_synthtoken("parent"));
        bl_parser_defvar(p, 0);
        bl_parser_namedvar(p, classname, false);
        bl_parser_emitbyte(p, OP_INHERIT);
        classcompiler.hassuperclass = true;
    }
    bl_parser_namedvar(p, classname, false);
    bl_parser_ignorespace(p);
    bl_parser_consume(p, TOK_LBRACE, "expected '{' before class body");
    bl_parser_ignorespace(p);
    while(!bl_parser_check(p, TOK_RBRACE) && !bl_parser_check(p, TOK_EOF))
    {
        bool isstatic = false;
        if(bl_parser_match(p, TOK_STATIC))
        {
            isstatic = true;
        }
        if(bl_parser_match(p, TOK_VAR))
        {
            bl_parser_parsefield(p, isstatic);
        }
        else
        {
            bl_parser_parsemethod(p, classname, isstatic);
            bl_parser_ignorespace(p);
        }
    }
    bl_parser_consume(p, TOK_RBRACE, "expected '}' after class body");
    bl_parser_emitbyte(p, OP_POP);
    if(classcompiler.hassuperclass)
    {
        bl_parser_endscope(p);
    }
    p->currentclass = p->currentclass->enclosing;
}

static void bl_parser_compilevardecl(AstParser* p, bool isinitializer)
{
    int totalparsed = 0;
    do
    {
        if(totalparsed > 0)
        {
            bl_parser_ignorespace(p);
        }
        int global = bl_parser_parsevar(p, "variable name expected");
        if(bl_parser_match(p, TOK_EQUAL))
        {
            bl_parser_parseexpr(p);
        }
        else
        {
            bl_parser_emitbyte(p, OP_NIL);
        }
        bl_parser_defvar(p, global);
        totalparsed++;
    } while(bl_parser_match(p, TOK_COMMA));
    if(!isinitializer)
    {
        bl_parser_consumestmtend(p);
    }
    else
    {
        bl_parser_consume(p, TOK_SEMICOLON, "expected ';' after initializer");
        bl_parser_ignorespace(p);
    }
}

static void bl_parser_parsevardecl(AstParser* p)
{
    bl_parser_compilevardecl(p, false);
}

static void bl_parser_parseexprstmt(AstParser* p, bool isinitializer, bool semi)
{
    if(p->vm->isrepl && p->vm->compiler->scopedepth == 0)
    {
        p->replcanecho = true;
    }
    if(!semi)
    {
        bl_parser_parseexpr(p);
    }
    else
    {
        bl_parser_parseprecnoadvance(p, PREC_ASSIGNMENT);
    }
    if(!isinitializer)
    {
        if(p->replcanecho && p->vm->isrepl)
        {
            bl_parser_emitbyte(p, OP_ECHO);
            p->replcanecho = false;
        }
        else
        {
            bl_parser_emitbyte(p, OP_POP);
        }
        bl_parser_consumestmtend(p);
    }
    else
    {
        bl_parser_consume(p, TOK_SEMICOLON, "expected ';' after initializer");
        bl_parser_ignorespace(p);
        bl_parser_emitbyte(p, OP_POP);
    }
}

/**
 * iter statements are like for loops in c...
 * they are desugared into a while loop
 *
 * i.e.
 *
 * iter i = 0; i < 10; i++ {
 *    ...
 * }
 *
 * desugars into:
 *
 * var i = 0
 * while i < 10 {
 *    ...
 *    i = i + 1
 * }
 */
static void bl_parser_parseforloopstmt(AstParser* p)
{
    bl_parser_beginscope(p);
    // parse initializer...
    if(bl_parser_match(p, TOK_SEMICOLON))
    {
        // no initializer
    }
    else if(bl_parser_match(p, TOK_VAR))
    {
        bl_parser_compilevardecl(p, true);
    }
    else
    {
        bl_parser_parseexprstmt(p, true, false);
    }
    // keep a copy of the surrounding loop's start and depth
    int surroundingloopstart = p->innermostloopstart;
    int surroundingscopedepth = p->innermostloopscopedepth;
    // update the parser's loop start and depth to the current
    p->innermostloopstart = bl_parser_currentblob(p)->count;
    p->innermostloopscopedepth = p->vm->compiler->scopedepth;
    int exitjump = -1;
    if(!bl_parser_match(p, TOK_SEMICOLON))
    {// the condition is optional
        bl_parser_parseexpr(p);
        bl_parser_consume(p, TOK_SEMICOLON, "expected ';' after condition");
        bl_parser_ignorespace(p);
        // jump out of the loop if the condition is false...
        exitjump = bl_parser_emitjump(p, OP_JUMP_IF_FALSE);
        bl_parser_emitbyte(p, OP_POP);// pop the condition
    }
    // the iterator...
    if(!bl_parser_check(p, TOK_LBRACE))
    {
        int bodyjump = bl_parser_emitjump(p, OP_JUMP);
        int incrementstart = bl_parser_currentblob(p)->count;
        bl_parser_parseexpr(p);
        bl_parser_ignorespace(p);
        bl_parser_emitbyte(p, OP_POP);
        bl_parser_emitloop(p, p->innermostloopstart);
        p->innermostloopstart = incrementstart;
        bl_parser_patchjump(p, bodyjump);
    }
    bl_parser_parsestmt(p);
    bl_parser_emitloop(p, p->innermostloopstart);
    if(exitjump != -1)
    {
        bl_parser_patchjump(p, exitjump);
        bl_parser_emitbyte(p, OP_POP);
    }
    bl_parser_endloop(p);
    // reset the loop start and scope depth to the surrounding value
    p->innermostloopstart = surroundingloopstart;
    p->innermostloopscopedepth = surroundingscopedepth;
    bl_parser_endscope(p);
}

/**
 * for x in iterable {
 *    ...
 * }
 *
 * ==
 *
 * {
 *    var iterable = expression()
 *    var _
 *
 *    while _ = iterable.@itern() {
 *      var x = iterable.@iter()
 *      ...
 *    }
 * }
 *
 * ---------------------------------
 *
 * for x, y in iterable {
 *    ...
 * }
 *
 * ==
 *
 * {
 *    var iterable = expression()
 *    var x
 *
 *    while x = iterable.@itern() {
 *      var y = iterable.@iter()
 *      ...
 *    }
 * }
 *
 * Every blade iterable must implement the @iter(x) and the @itern(x)
 * function.
 *
 * to make instances of a user created class iterable,
 * the class must implement the @iter(x) and the @itern(x) function.
 * the @itern(x) must return the current iterating index of the object and
 * the
 * @iter(x) function must return the value at that index.
 * _NOTE_: the @iter(x) function will no longer be called after the
 * @itern(x) function returns a false value. so the @iter(x) never needs
 * to return a false value
 */
static void bl_parser_parseforeachstmt(AstParser* p)
{
    bl_parser_beginscope(p);
    // define @iter and @itern constant
    int iter__ = bl_parser_makeconstant(p, OBJ_VAL(bl_string_copystring(p->vm, "@iter", 5)));
    int iter_n__ = bl_parser_makeconstant(p, OBJ_VAL(bl_string_copystring(p->vm, "@itern", 6)));
    /*
    if(bl_parser_match(p, TOK_LPAREN))
    {
        bl_parser_advance(p);
    }
    */
    bl_parser_consume(p, TOK_IDENTIFIER, "expected variable name after 'for'");
    AstToken keytoken;
    AstToken valuetoken;
    if(!bl_parser_check(p, TOK_COMMA))
    {
        keytoken = bl_parser_synthtoken(" _ ");
        valuetoken = p->previous;
    }
    else
    {
        keytoken = p->previous;
        bl_parser_consume(p, TOK_COMMA, "");
        bl_parser_consume(p, TOK_IDENTIFIER, "expected variable name after ','");
        valuetoken = p->previous;
    }
    bl_parser_consume(p, TOK_IN, "expected 'in' after for loop variable(s)");
    bl_parser_ignorespace(p);
    // The space in the variable name ensures it won't collide with a user-defined
    // variable.
    AstToken iteratortoken = bl_parser_synthtoken(" iterator ");
    // Evaluate the sequence expression and store it in a hidden local variable.
    bl_parser_parseexpr(p);
    if(p->vm->compiler->localcount + 3 > UINT8_COUNT)
    {
        bl_parser_raiseerror(p, "cannot declare more than %d variables in one scope", UINT8_COUNT);
        return;
    }
    /*
    if(bl_parser_match(p, TOK_RPAREN))
    {
        bl_parser_advance(p);
    }
    */
    // add the iterator to the local scope
    int iteratorslot = bl_parser_addlocal(p, iteratortoken) - 1;
    bl_parser_defvar(p, 0);
    // Create the key local variable.
    bl_parser_emitbyte(p, OP_NIL);
    int keyslot = bl_parser_addlocal(p, keytoken) - 1;
    bl_parser_defvar(p, keyslot);
    // create the local value slot
    bl_parser_emitbyte(p, OP_NIL);
    int valueslot = bl_parser_addlocal(p, valuetoken) - 1;
    bl_parser_defvar(p, 0);
    int surroundingloopstart = p->innermostloopstart;
    int surroundingscopedepth = p->innermostloopscopedepth;
    // we'll be jumping back to right before the
    // expression after the loop body
    p->innermostloopstart = bl_parser_currentblob(p)->count;
    p->innermostloopscopedepth = p->vm->compiler->scopedepth;
    // key = iterable.iter_n__(key)
    bl_parser_emitbyte_and_short(p, OP_GET_LOCAL, iteratorslot);
    bl_parser_emitbyte_and_short(p, OP_GET_LOCAL, keyslot);
    bl_parser_emitbyte_and_short(p, OP_INVOKE, iter_n__);
    bl_parser_emitbyte(p, 1);
    bl_parser_emitbyte_and_short(p, OP_SET_LOCAL, keyslot);
    int falsejump = bl_parser_emitjump(p, OP_JUMP_IF_FALSE);
    bl_parser_emitbyte(p, OP_POP);
    // value = iterable.iter__(key)
    bl_parser_emitbyte_and_short(p, OP_GET_LOCAL, iteratorslot);
    bl_parser_emitbyte_and_short(p, OP_GET_LOCAL, keyslot);
    bl_parser_emitbyte_and_short(p, OP_INVOKE, iter__);
    bl_parser_emitbyte(p, 1);
    // Bind the loop value in its own scope. This ensures we get a fresh
    // variable each iteration so that closures for it don't all see the same one.
    bl_parser_beginscope(p);
    // update the value
    bl_parser_emitbyte_and_short(p, OP_SET_LOCAL, valueslot);
    bl_parser_emitbyte(p, OP_POP);
    bl_parser_parsestmt(p);
    bl_parser_endscope(p);
    bl_parser_emitloop(p, p->innermostloopstart);
    bl_parser_patchjump(p, falsejump);
    bl_parser_emitbyte(p, OP_POP);
    bl_parser_endloop(p);
    p->innermostloopstart = surroundingloopstart;
    p->innermostloopscopedepth = surroundingscopedepth;
    bl_parser_endscope(p);
}

/**
 * using expression {
 *    when expression {
 *      ...
 *    }
 *    when expression {
 *      ...
 *    }
 *    ...
 * }
 */
static void bl_parser_parseusingstmt(AstParser* p)
{
    bl_parser_parseexpr(p);// the expression
    bl_parser_consume(p, TOK_LBRACE, "expected '{' after using expression");
    bl_parser_ignorespace(p);
    int state = 0;// 0: before all cases, 1: before default, 2: after default
    int caseends[MAX_USING_CASES];
    int casecount = 0;
    ObjSwitch* sw = bl_object_makeswitch(p->vm);
    bl_vm_pushvalue(p->vm, OBJ_VAL(sw));
    int switchcode = bl_parser_emitswitch(p);
    // bl_parser_emitbyte_and_short(p, OP_SWITCH, bl_parser_makeconstant(p, OBJ_VAL(sw)));
    int startoffset = bl_parser_currentblob(p)->count;
    while(!bl_parser_match(p, TOK_RBRACE) && !bl_parser_check(p, TOK_EOF))
    {
        if(bl_parser_match(p, TOK_WHEN) || bl_parser_match(p, TOK_DEFAULT))
        {
            TokType casetype = p->previous.type;
            if(state == 2)
            {
                bl_parser_raiseerror(p, "cannot have another case after a default case");
            }
            if(state == 1)
            {
                // at the end of the previous case, jump over the others...
                caseends[casecount++] = bl_parser_emitjump(p, OP_JUMP);
            }
            if(casetype == TOK_WHEN)
            {
                state = 1;
                do
                {
                    bl_parser_advance(p);
                    Value jump = NUMBER_VAL((double)bl_parser_currentblob(p)->count - (double)startoffset);
                    if(p->previous.type == TOK_TRUE)
                    {
                        bl_hashtable_set(p->vm, &sw->table, TRUE_VAL, jump);
                    }
                    else if(p->previous.type == TOK_FALSE)
                    {
                        bl_hashtable_set(p->vm, &sw->table, FALSE_VAL, jump);
                    }
                    else if(p->previous.type == TOK_LITERAL)
                    {
                        int length;
                        char* str = bl_parser_compilestring(p, &length);
                        ObjString* string = bl_string_copystring(p->vm, str, length);
                        bl_vm_pushvalue(p->vm, OBJ_VAL(string));// gc fix
                        bl_hashtable_set(p->vm, &sw->table, OBJ_VAL(string), jump);
                        bl_vm_popvalue(p->vm);// gc fix
                    }
                    else if(bl_parser_checknumber(p))
                    {
                        bl_hashtable_set(p->vm, &sw->table, bl_parser_compilenumber(p), jump);
                    }
                    else
                    {
                        bl_vm_popvalue(p->vm);// pop the switch
                        bl_parser_raiseerror(p, "only constants can be used in when expressions");
                        return;
                    }
                } while(bl_parser_match(p, TOK_COMMA));
            }
            else
            {
                state = 2;
                sw->defaultjump = bl_parser_currentblob(p)->count - startoffset;
            }
        }
        else
        {
            // otherwise, it's a statement inside the current case
            if(state == 0)
            {
                bl_parser_raiseerror(p, "cannot have statements before any case");
            }
            bl_parser_parsestmt(p);
        }
    }
    // if we ended without a default case, patch its condition jump
    if(state == 1)
    {
        caseends[casecount++] = bl_parser_emitjump(p, OP_JUMP);
    }
    // patch all the case jumps to the end
    for(int i = 0; i < casecount; i++)
    {
        bl_parser_patchjump(p, caseends[i]);
    }
    sw->exitjump = bl_parser_currentblob(p)->count - startoffset;
    bl_parser_patchswitch(p, switchcode, bl_parser_makeconstant(p, OBJ_VAL(sw)));
    bl_vm_popvalue(p->vm);// pop the switch
}

static void bl_parser_parseifstmt(AstParser* p)
{
    bl_parser_parseexpr(p);
    int thenjump = bl_parser_emitjump(p, OP_JUMP_IF_FALSE);
    bl_parser_emitbyte(p, OP_POP);
    bl_parser_parsestmt(p);
    int elsejump = bl_parser_emitjump(p, OP_JUMP);
    bl_parser_patchjump(p, thenjump);
    bl_parser_emitbyte(p, OP_POP);
    if(bl_parser_match(p, TOK_ELSE))
    {
        bl_parser_parsestmt(p);
    }
    bl_parser_patchjump(p, elsejump);
}

static void bl_parser_parseechostmt(AstParser* p)
{
    bl_parser_parseexpr(p);
    bl_parser_emitbyte(p, OP_ECHO);
    bl_parser_consumestmtend(p);
}

static void bl_parser_parsediestmt(AstParser* p)
{
    bl_parser_parseexpr(p);
    bl_parser_emitbyte(p, OP_DIE);
    bl_parser_consumestmtend(p);
}

static void bl_parser_parsespecificimport(AstParser* p, char* modulename, int importconstant, bool wasrenamed, bool isnative)
{
    if(bl_parser_match(p, TOK_LBRACE))
    {
        if(wasrenamed)
        {
            bl_parser_raiseerror(p, "selective import on renamed module");
            return;
        }
        bl_parser_emitbyte_and_short(p, OP_CONSTANT, importconstant);
        bool samenameselectiveexist = false;
        do
        {
            bl_parser_ignorespace(p);
            // terminate on all (*)
            if(bl_parser_match(p, TOK_MULTIPLY))
            {
                bl_parser_emitbyte(p, isnative ? OP_IMPORT_ALL_NATIVE : OP_IMPORT_ALL);
                break;
            }
            int name = bl_parser_parsevar(p, "module object name expected");
            if(modulename != NULL && p->previous.length == (int)strlen(modulename) && memcmp(modulename, p->previous.start, p->previous.length) == 0)
            {
                samenameselectiveexist = true;
            }
            bl_parser_emitbyte_and_short(p, isnative ? OP_SELECT_NATIVE_IMPORT : OP_SELECT_IMPORT, name);
        } while(bl_parser_match(p, TOK_COMMA));
        bl_parser_ignorespace(p);
        bl_parser_consume(p, TOK_RBRACE, "expected '}' at end of selective import");
        if(!samenameselectiveexist)
        {
            bl_parser_emitbyte_and_short(p, isnative ? OP_EJECT_NATIVE_IMPORT : OP_EJECT_IMPORT, importconstant);
        }
        bl_parser_emitbyte(p, OP_POP);// pop the module constant from stack
        bl_parser_consumestmtend(p);
    }
}

static void bl_parser_parseimportstmt(AstParser* p)
{
    //  bl_parser_consume(p, TOK_LITERAL, "expected module name");
    //  int modulenamelength;
    //  char *modulename = bl_parser_compilestring(p, &modulenamelength);
    char* modulename = NULL;
    char* modulefile = NULL;
    int partcount = 0;
    bool isrelative = bl_parser_match(p, TOK_DOT);
    // allow for import starting with ..
    if(!isrelative)
    {
        if(bl_parser_match(p, TOK_RANGE))
        {
        }
    }
    else
    {
        if(bl_parser_match(p, TOK_RANGE))
        {
            bl_parser_raiseerror(p, "conflicting module path. Parent or current directory?");
            return;
        }
    }
    do
    {
        if(p->previous.type == TOK_RANGE)
        {
            isrelative = true;
            if(modulefile == NULL)
            {
                modulefile = strdup("/../");
            }
            else
            {
                modulefile = bl_util_appendstring(modulefile, "/../");
            }
        }
        if(modulename != NULL)
        {
            free(modulename);
        }
        bl_parser_consume(p, TOK_IDENTIFIER, "module name expected");
        modulename = (char*)calloc(p->previous.length + 1, sizeof(char));
        memcpy(modulename, p->previous.start, p->previous.length);
        modulename[p->previous.length] = '\0';
        // handle native modules
        if(partcount == 0 && modulename[0] == '_' && !isrelative)
        {
            int module = bl_parser_makeconstant(p, OBJ_VAL(bl_string_copystring(p->vm, modulename, (int)strlen(modulename))));
            bl_parser_emitbyte_and_short(p, OP_NATIVE_MODULE, module);
            bl_parser_parsespecificimport(p, modulename, module, false, true);
            return;
        }
        if(modulefile == NULL)
        {
            modulefile = strdup(modulename);
        }
        else
        {
            if(modulefile[strlen(modulefile) - 1] != BLADE_PATH_SEPARATOR[0])
            {
                modulefile = bl_util_appendstring(modulefile, BLADE_PATH_SEPARATOR);
            }
            modulefile = bl_util_appendstring(modulefile, modulename);
        }
        partcount++;
    } while(bl_parser_match(p, TOK_DOT) || bl_parser_match(p, TOK_RANGE));
    bool wasrenamed = false;
    if(bl_parser_match(p, TOK_AS))
    {
        bl_parser_consume(p, TOK_IDENTIFIER, "module name expected");
        free(modulename);
        modulename = (char*)calloc(p->previous.length + 1, sizeof(char));
        if(modulename == NULL)
        {
            bl_parser_raiseerror(p, "could not allocate memory for module name");
            return;
        }
        memcpy(modulename, p->previous.start, p->previous.length);
        modulename[p->previous.length] = '\0';
        wasrenamed = true;
    }
    char* modulepath = bl_util_resolveimportpath(modulefile, p->module->file, isrelative);
    if(modulepath == NULL)
    {
        // check if there is one in the vm's registry
        // handle native modules
        Value md;
        ObjString* finalmodulename = bl_string_copystring(p->vm, modulename, (int)strlen(modulename));
        if(bl_hashtable_get(&p->vm->modules, OBJ_VAL(finalmodulename), &md))
        {
            int module = bl_parser_makeconstant(p, OBJ_VAL(finalmodulename));
            bl_parser_emitbyte_and_short(p, OP_NATIVE_MODULE, module);
            bl_parser_parsespecificimport(p, modulename, module, false, true);
            return;
        }
        free(modulepath);
        bl_parser_raiseerror(p, "module not found");
        return;
    }
    if(!bl_parser_check(p, TOK_LBRACE))
    {
        bl_parser_consumestmtend(p);
    }
    // do the import here...
    char* source = bl_util_readfile(modulepath);
    if(source == NULL)
    {
        bl_parser_raiseerror(p, "could not read import file %s", modulepath);
        return;
    }
    BinaryBlob blob;
    bl_blob_init(&blob);
    ObjModule* module = bl_object_makemodule(p->vm, modulename, modulepath);
    bl_vm_pushvalue(p->vm, OBJ_VAL(module));
    ObjFunction* function = bl_compiler_compilesource(p->vm, module, source, &blob);
    bl_vm_popvalue(p->vm);
    free(source);
    if(function == NULL)
    {
        bl_parser_raiseerror(p, "failed to import %s", modulename);
        return;
    }
    function->name = NULL;
    bl_vm_pushvalue(p->vm, OBJ_VAL(function));
    ObjClosure* closure = bl_object_makeclosure(p->vm, function);
    bl_vm_popvalue(p->vm);
    int importconstant = bl_parser_makeconstant(p, OBJ_VAL(closure));
    bl_parser_emitbyte_and_short(p, OP_CALL_IMPORT, importconstant);
    bl_parser_parsespecificimport(p, modulename, importconstant, wasrenamed, false);
}

static void bl_parser_parseassertstmt(AstParser* p)
{
    bl_parser_parseexpr(p);
    if(bl_parser_match(p, TOK_COMMA))
    {
        bl_parser_ignorespace(p);
        bl_parser_parseexpr(p);
    }
    else
    {
        bl_parser_emitbyte(p, OP_NIL);
    }
    bl_parser_emitbyte(p, OP_ASSERT);
    bl_parser_consumestmtend(p);
}

static void bl_parser_parsetrystmt(AstParser* p)
{
    if(p->vm->compiler->handlercount == MAX_EXCEPTION_HANDLERS)
    {
        bl_parser_raiseerror(p, "maximum exception handler in scope exceeded");
    }
    p->vm->compiler->handlercount++;
    p->istrying = true;
    bl_parser_ignorespace(p);
    int trybegins = bl_parser_emittry(p);
    bl_parser_parsestmt(p);// compile the try body
    bl_parser_emitbyte(p, OP_POP_TRY);
    int exitjump = bl_parser_emitjump(p, OP_JUMP);
    p->istrying = false;
    // we can safely use 0 because a program cannot start with a
    // catch or finally block
    int address = 0;
    int type = -1;
    int finally = 0;
    bool catchexists = false, finalexists = false;
    // catch body must maintain its own scope
    if(bl_parser_match(p, TOK_CATCH))
    {
        catchexists = true;
        bl_parser_beginscope(p);
        bl_parser_consume(p, TOK_IDENTIFIER, "missing exception class name");
        type = bl_parser_identconst(p, &p->previous);
        address = bl_parser_currentblob(p)->count;
        if(bl_parser_match(p, TOK_IDENTIFIER))
        {
            int var = bl_parser_addlocal(p, p->previous) - 1;
            bl_parser_markinit(p);
            bl_parser_emitbyte_and_short(p, OP_SET_LOCAL, var);
            bl_parser_emitbyte(p, OP_POP);
        }
        bl_parser_emitbyte(p, OP_POP_TRY);
        bl_parser_ignorespace(p);
        bl_parser_parsestmt(p);
        bl_parser_endscope(p);
    }
    else
    {
        type = bl_parser_makeconstant(p, OBJ_VAL(bl_string_copystring(p->vm, "Exception", 9)));
    }
    bl_parser_patchjump(p, exitjump);
    if(bl_parser_match(p, TOK_FINALLY))
    {
        finalexists = true;
        // if we arrived here from either the try or handler block,
        // we don't want to continue propagating the exception
        bl_parser_emitbyte(p, OP_FALSE);
        finally = bl_parser_currentblob(p)->count;
        bl_parser_ignorespace(p);
        bl_parser_parsestmt(p);
        int continueexecutionaddress = bl_parser_emitjump(p, OP_JUMP_IF_FALSE);
        bl_parser_emitbyte(p, OP_POP);// pop the bool off the stack
        bl_parser_emitbyte(p, OP_PUBLISH_TRY);
        bl_parser_patchjump(p, continueexecutionaddress);
        bl_parser_emitbyte(p, OP_POP);
    }
    if(!finalexists && !catchexists)
    {
        bl_parser_raiseerror(p, "try block must contain at least one of catch or finally");
    }
    bl_parser_patchtry(p, trybegins, type, address, finally);
}

static void bl_parser_parsereturnstmt(AstParser* p)
{
    p->isreturning = true;
    if(p->vm->compiler->type == TYPE_SCRIPT)
    {
        bl_parser_raiseerror(p, "cannot return from top-level code");
    }
    if(bl_parser_match(p, TOK_SEMICOLON) || bl_parser_match(p, TOK_NEWLINE))
    {
        bl_parser_emitreturn(p);
    }
    else
    {
        if(p->vm->compiler->type == TYPE_INITIALIZER)
        {
            bl_parser_raiseerror(p, "cannot return value from constructor");
        }
        if(p->istrying)
        {
            bl_parser_emitbyte(p, OP_POP_TRY);
        }
        bl_parser_parseexpr(p);
        bl_parser_emitbyte(p, OP_RETURN);
        bl_parser_consumestmtend(p);
    }
    p->isreturning = false;
}

static void bl_parser_parsewhilestmt(AstParser* p)
{
    int surroundingloopstart = p->innermostloopstart;
    int surroundingscopedepth = p->innermostloopscopedepth;
    // we'll be jumping back to right before the
    // expression after the loop body
    p->innermostloopstart = bl_parser_currentblob(p)->count;
    bl_parser_parseexpr(p);
    int exitjump = bl_parser_emitjump(p, OP_JUMP_IF_FALSE);
    bl_parser_emitbyte(p, OP_POP);
    bl_parser_parsestmt(p);
    bl_parser_emitloop(p, p->innermostloopstart);
    bl_parser_patchjump(p, exitjump);
    bl_parser_emitbyte(p, OP_POP);
    bl_parser_endloop(p);
    p->innermostloopstart = surroundingloopstart;
    p->innermostloopscopedepth = surroundingscopedepth;
}

static void bl_parser_parsedowhilestmt(AstParser* p)
{
    int surroundingloopstart = p->innermostloopstart;
    int surroundingscopedepth = p->innermostloopscopedepth;
    // we'll be jumping back to right before the
    // statements after the loop body
    p->innermostloopstart = bl_parser_currentblob(p)->count;
    bl_parser_parsestmt(p);
    bl_parser_consume(p, TOK_WHILE, "expecting 'while' statement");
    bl_parser_parseexpr(p);
    int exitjump = bl_parser_emitjump(p, OP_JUMP_IF_FALSE);
    bl_parser_emitbyte(p, OP_POP);
    bl_parser_emitloop(p, p->innermostloopstart);
    bl_parser_patchjump(p, exitjump);
    bl_parser_emitbyte(p, OP_POP);
    bl_parser_endloop(p);
    p->innermostloopstart = surroundingloopstart;
    p->innermostloopscopedepth = surroundingscopedepth;
}

static void bl_parser_parsecontinuestmt(AstParser* p)
{
    if(p->innermostloopstart == -1)
    {
        bl_parser_raiseerror(p, "'continue' can only be used in a loop");
    }
    // discard local variables created in the loop
    bl_parser_discardlocal(p, p->innermostloopscopedepth);
    // go back to the top of the loop
    bl_parser_emitloop(p, p->innermostloopstart);
    bl_parser_consumestmtend(p);
}

static void bl_parser_parsebreakstmt(AstParser* p)
{
    if(p->innermostloopstart == -1)
    {
        bl_parser_raiseerror(p, "'break' can only be used in a loop");
    }
    // discard local variables created in the loop
    //  bl_parser_discardlocal(p, p->innermostloopscopedepth);
    bl_parser_emitjump(p, OP_BREAK_PL);
    bl_parser_consumestmtend(p);
}

static void bl_parser_synchronize(AstParser* p)
{
    p->panicmode = false;
    while(p->current.type != TOK_EOF)
    {
        if(p->current.type == TOK_NEWLINE || p->current.type == TOK_SEMICOLON)
        {
            return;
        }
        switch(p->current.type)
        {
            case TOK_CLASS:
            case TOK_DEF:
            case TOK_VAR:
            case TOK_FOREACH:
            case TOK_IF:
            case TOK_USING:
            case TOK_WHEN:
            case TOK_FORLOOP:
            case TOK_DO:
            case TOK_WHILE:
            case TOK_ECHO:
            case TOK_ASSERT:
            case TOK_TRY:
            case TOK_CATCH:
            case TOK_DIE:
            case TOK_RETURN:
            case TOK_STATIC:
            case TOK_SELF:
            case TOK_PARENT:
            case TOK_FINALLY:
            case TOK_IN:
            case TOK_IMPORT:
            case TOK_AS:
                return;
            default:;// do nothing
        }
        bl_parser_advance(p);
    }
}

static void bl_parser_parsedeclaration(AstParser* p)
{
    bl_parser_ignorespace(p);
    if(bl_parser_match(p, TOK_CLASS))
    {
        bl_parser_parseclassdecl(p);
    }
    else if(bl_parser_match(p, TOK_DEF))
    {
        bl_parser_parsefunctiondecl(p);
    }
    else if(bl_parser_match(p, TOK_VAR))
    {
        bl_parser_parsevardecl(p);
    }
    else if(bl_parser_match(p, TOK_LBRACE))
    {
        if(!bl_parser_check(p, TOK_NEWLINE) && p->vm->compiler->scopedepth == 0)
        {
            bl_parser_parseexprstmt(p, false, true);
        }
        else
        {
            bl_parser_beginscope(p);
            bl_parser_parseblock(p);
            bl_parser_endscope(p);
        }
    }
    else
    {
        bl_parser_parsestmt(p);
    }
    bl_parser_ignorespace(p);
    if(p->panicmode)
    {
        bl_parser_synchronize(p);
    }
    bl_parser_ignorespace(p);
}

static void bl_parser_parsestmt(AstParser* p)
{
    p->replcanecho = false;
    bl_parser_ignorespace(p);
    if(bl_parser_match(p, TOK_ECHO))
    {
        bl_parser_parseechostmt(p);
    }
    else if(bl_parser_match(p, TOK_IF))
    {
        bl_parser_parseifstmt(p);
    }
    else if(bl_parser_match(p, TOK_DO))
    {
        bl_parser_parsedowhilestmt(p);
    }
    else if(bl_parser_match(p, TOK_WHILE))
    {
        bl_parser_parsewhilestmt(p);
    }
    else if(bl_parser_match(p, TOK_FORLOOP))
    {
        bl_parser_parseforloopstmt(p);
    }
    else if(bl_parser_match(p, TOK_FOREACH))
    {
        bl_parser_parseforeachstmt(p);
    }
    else if(bl_parser_match(p, TOK_USING))
    {
        bl_parser_parseusingstmt(p);
    }
    else if(bl_parser_match(p, TOK_CONTINUE))
    {
        bl_parser_parsecontinuestmt(p);
    }
    else if(bl_parser_match(p, TOK_BREAK))
    {
        bl_parser_parsebreakstmt(p);
    }
    else if(bl_parser_match(p, TOK_RETURN))
    {
        bl_parser_parsereturnstmt(p);
    }
    else if(bl_parser_match(p, TOK_ASSERT))
    {
        bl_parser_parseassertstmt(p);
    }
    else if(bl_parser_match(p, TOK_DIE))
    {
        bl_parser_parsediestmt(p);
    }
    else if(bl_parser_match(p, TOK_LBRACE))
    {
        bl_parser_beginscope(p);
        bl_parser_parseblock(p);
        bl_parser_endscope(p);
    }
    else if(bl_parser_match(p, TOK_IMPORT))
    {
        bl_parser_parseimportstmt(p);
    }
    else if(bl_parser_match(p, TOK_TRY))
    {
        bl_parser_parsetrystmt(p);
    }
    else
    {
        bl_parser_parseexprstmt(p, false, false);
    }
    bl_parser_ignorespace(p);
}

ObjFunction* bl_compiler_compilesource(VMState* vm, ObjModule* module, const char* source, BinaryBlob* blob)
{
    AstScanner scanner;
    AstParser parser;
    AstCompiler compiler;
    (void)blob;
    bl_scanner_init(&scanner, source);
    parser.vm = vm;
    parser.scanner = &scanner;
    parser.haderror = false;
    parser.panicmode = false;
    parser.blockcount = 0;
    parser.replcanecho = false;
    parser.isreturning = false;
    parser.istrying = false;
    parser.innermostloopstart = -1;
    parser.innermostloopscopedepth = 0;
    parser.currentclass = NULL;
    parser.module = module;
    bl_compiler_init(&parser, &compiler, TYPE_SCRIPT);
    bl_parser_advance(&parser);
    bl_parser_ignorespace(&parser);
    while(!bl_parser_match(&parser, TOK_EOF))
    {
        bl_parser_parsedeclaration(&parser);
    }
    ObjFunction* function = bl_compiler_end(&parser);
    return parser.haderror ? NULL : function;
}

void bl_parser_markcompilerroots(VMState* vm)
{
    AstCompiler* compiler = vm->compiler;
    while(compiler != NULL)
    {
        bl_mem_markobject(vm, (Object*)compiler->currfunc);
        compiler = compiler->enclosing;
    }
}

void bl_blob_disassembleitem(BinaryBlob* blob, const char* name)
{
    printf("== %s ==\n", name);
    for(int offset = 0; offset < blob->count;)
    {
        offset = bl_blob_disassembleinst(blob, offset);
    }
}

int bl_blob_disaspriminst(const char* name, int offset)
{
    printf("%s\n", name);
    return offset + 1;
}

int bl_blob_disasconstinst(const char* name, BinaryBlob* blob, int offset)
{
    uint16_t constant = (blob->code[offset + 1] << 8) | blob->code[offset + 2];
    printf("%-16s %8d '", name, constant);
    bl_value_printvalue(blob->constants.values[constant]);
    printf("'\n");
    return offset + 3;
}

int bl_blob_disasshortinst(const char* name, BinaryBlob* blob, int offset)
{
    uint16_t slot = (blob->code[offset + 1] << 8) | blob->code[offset + 2];
    printf("%-16s %8d\n", name, slot);
    return offset + 3;
}

static int bl_blob_disasbyteinst(const char* name, BinaryBlob* blob, int offset)
{
    uint8_t slot = blob->code[offset + 1];
    printf("%-16s %8d\n", name, slot);
    return offset + 2;
}

static int bl_blob_disasjumpinst(const char* name, int sign, BinaryBlob* blob, int offset)
{
    uint16_t jump = (uint16_t)(blob->code[offset + 1] << 8);
    jump |= blob->code[offset + 2];
    printf("%-16s %8d -> %d\n", name, offset, offset + 3 + sign * jump);
    return offset + 3;
}

static int bl_blob_disastryinst(const char* name, BinaryBlob* blob, int offset)
{
    uint16_t type = (uint16_t)(blob->code[offset + 1] << 8);
    type |= blob->code[offset + 2];
    uint16_t address = (uint16_t)(blob->code[offset + 3] << 8);
    address |= blob->code[offset + 4];
    uint16_t finally = (uint16_t)(blob->code[offset + 5] << 8);
    finally |= blob->code[offset + 6];
    printf("%-16s %8d -> %d, %d\n", name, type, address, finally);
    return offset + 7;
}

static int bl_blob_disasinvokeinst(const char* name, BinaryBlob* blob, int offset)
{
    uint16_t constant = (uint16_t)(blob->code[offset + 1] << 8);
    constant |= blob->code[offset + 2];
    uint8_t argcount = blob->code[offset + 3];
    printf("%-16s (%d args) %8d '", name, argcount, constant);
    bl_value_printvalue(blob->constants.values[constant]);
    printf("'\n");
    return offset + 4;
}

int bl_blob_disassembleinst(BinaryBlob* blob, int offset)
{
    printf("%08d ", offset);
    if(offset > 0 && blob->lines[offset] == blob->lines[offset - 1])
    {
        printf("       | ");
    }
    else
    {
        printf("%8d ", blob->lines[offset]);
    }
    uint8_t instruction = blob->code[offset];
    switch(instruction)
    {
        case OP_JUMP_IF_FALSE:
            return bl_blob_disasjumpinst("fjump", 1, blob, offset);
        case OP_JUMP:
            return bl_blob_disasjumpinst("jump", 1, blob, offset);
        case OP_TRY:
            return bl_blob_disastryinst("itry", blob, offset);
        case OP_LOOP:
            return bl_blob_disasjumpinst("loop", -1, blob, offset);
        case OP_DEFINE_GLOBAL:
            return bl_blob_disasconstinst("dglob", blob, offset);
        case OP_GET_GLOBAL:
            return bl_blob_disasconstinst("gglob", blob, offset);
        case OP_SET_GLOBAL:
            return bl_blob_disasconstinst("sglob", blob, offset);
        case OP_GET_LOCAL:
            return bl_blob_disasshortinst("gloc", blob, offset);
        case OP_SET_LOCAL:
            return bl_blob_disasshortinst("sloc", blob, offset);
        case OP_GET_PROPERTY:
            return bl_blob_disasconstinst("gprop", blob, offset);
        case OP_GET_SELF_PROPERTY:
            return bl_blob_disasconstinst("gprops", blob, offset);
        case OP_SET_PROPERTY:
            return bl_blob_disasconstinst("sprop", blob, offset);
        case OP_GET_UP_VALUE:
            return bl_blob_disasshortinst("gupv", blob, offset);
        case OP_SET_UP_VALUE:
            return bl_blob_disasshortinst("supv", blob, offset);
        case OP_POP_TRY:
            return bl_blob_disaspriminst("ptry", offset);
        case OP_PUBLISH_TRY:
            return bl_blob_disaspriminst("pubtry", offset);
        case OP_CONSTANT:
            return bl_blob_disasconstinst("load", blob, offset);
        case OP_EQUAL:
            return bl_blob_disaspriminst("eq", offset);
        case OP_GREATER:
            return bl_blob_disaspriminst("gt", offset);
        case OP_LESS:
            return bl_blob_disaspriminst("less", offset);
        case OP_EMPTY:
            return bl_blob_disaspriminst("em", offset);
        case OP_NIL:
            return bl_blob_disaspriminst("nil", offset);
        case OP_TRUE:
            return bl_blob_disaspriminst("true", offset);
        case OP_FALSE:
            return bl_blob_disaspriminst("false", offset);
        case OP_ADD:
            return bl_blob_disaspriminst("add", offset);
        case OP_SUBTRACT:
            return bl_blob_disaspriminst("sub", offset);
        case OP_MULTIPLY:
            return bl_blob_disaspriminst("mul", offset);
        case OP_DIVIDE:
            return bl_blob_disaspriminst("div", offset);
        case OP_F_DIVIDE:
            return bl_blob_disaspriminst("fdiv", offset);
        case OP_REMINDER:
            return bl_blob_disaspriminst("rmod", offset);
        case OP_POW:
            return bl_blob_disaspriminst("pow", offset);
        case OP_NEGATE:
            return bl_blob_disaspriminst("neg", offset);
        case OP_NOT:
            return bl_blob_disaspriminst("not", offset);
        case OP_BIT_NOT:
            return bl_blob_disaspriminst("bnot", offset);
        case OP_AND:
            return bl_blob_disaspriminst("band", offset);
        case OP_OR:
            return bl_blob_disaspriminst("bor", offset);
        case OP_XOR:
            return bl_blob_disaspriminst("bxor", offset);
        case OP_LSHIFT:
            return bl_blob_disaspriminst("lshift", offset);
        case OP_RSHIFT:
            return bl_blob_disaspriminst("rshift", offset);
        case OP_ONE:
            return bl_blob_disaspriminst("one", offset);
        case OP_CALL_IMPORT:
            return bl_blob_disasshortinst("cimport", blob, offset);
        case OP_NATIVE_MODULE:
            return bl_blob_disasshortinst("fimport", blob, offset);
        case OP_SELECT_IMPORT:
            return bl_blob_disasshortinst("simport", blob, offset);
        case OP_SELECT_NATIVE_IMPORT:
            return bl_blob_disasshortinst("snimport", blob, offset);
        case OP_EJECT_IMPORT:
            return bl_blob_disasshortinst("eimport", blob, offset);
        case OP_EJECT_NATIVE_IMPORT:
            return bl_blob_disasshortinst("enimport", blob, offset);
        case OP_IMPORT_ALL:
            return bl_blob_disaspriminst("aimport", offset);
        case OP_IMPORT_ALL_NATIVE:
            return bl_blob_disaspriminst("animport", offset);
        case OP_ECHO:
            return bl_blob_disaspriminst("echo", offset);
        case OP_STRINGIFY:
            return bl_blob_disaspriminst("str", offset);
        case OP_CHOICE:
            return bl_blob_disaspriminst("cho", offset);
        case OP_DIE:
            return bl_blob_disaspriminst("die", offset);
        case OP_POP:
            return bl_blob_disaspriminst("pop", offset);
        case OP_CLOSE_UP_VALUE:
            return bl_blob_disaspriminst("clupv", offset);
        case OP_DUP:
            return bl_blob_disaspriminst("dup", offset);
        case OP_ASSERT:
            return bl_blob_disaspriminst("assrt", offset);
        case OP_POP_N:
            return bl_blob_disasshortinst("pop_n", blob, offset);
            // non-user objects...
        case OP_SWITCH:
            return bl_blob_disasshortinst("sw", blob, offset);
            // data container manipulators
        case OP_RANGE:
            return bl_blob_disasshortinst("rng", blob, offset);
        case OP_LIST:
            return bl_blob_disasshortinst("list", blob, offset);
        case OP_DICT:
            return bl_blob_disasshortinst("dict", blob, offset);
        case OP_GET_INDEX:
            return bl_blob_disasbyteinst("gind", blob, offset);
        case OP_GET_RANGED_INDEX:
            return bl_blob_disasbyteinst("grind", blob, offset);
        case OP_SET_INDEX:
            return bl_blob_disaspriminst("sind", offset);
        case OP_CLOSURE:
        {
            offset++;
            uint16_t constant = blob->code[offset++] << 8;
            constant |= blob->code[offset++];
            printf("%-16s %8d ", "clsur", constant);
            bl_value_printvalue(blob->constants.values[constant]);
            printf("\n");
            ObjFunction* function = AS_FUNCTION(blob->constants.values[constant]);
            for(int j = 0; j < function->upvaluecount; j++)
            {
                int islocal = blob->code[offset++];
                uint16_t index = blob->code[offset++] << 8;
                index |= blob->code[offset++];
                printf("%04d      |                     %s %d\n", offset - 3, islocal ? "local" : "up-value", (int)index);
            }
            return offset;
        }
        case OP_CALL:
            return bl_blob_disasbyteinst("call", blob, offset);
        case OP_INVOKE:
            return bl_blob_disasinvokeinst("invk", blob, offset);
        case OP_INVOKE_SELF:
            return bl_blob_disasinvokeinst("invks", blob, offset);
        case OP_RETURN:
            return bl_blob_disaspriminst("ret", offset);
        case OP_CLASS:
            return bl_blob_disasconstinst("class", blob, offset);
        case OP_METHOD:
            return bl_blob_disasconstinst("meth", blob, offset);
        case OP_CLASS_PROPERTY:
            return bl_blob_disasconstinst("clprop", blob, offset);
        case OP_GET_SUPER:
            return bl_blob_disasconstinst("gsup", blob, offset);
        case OP_INHERIT:
            return bl_blob_disaspriminst("inher", offset);
        case OP_SUPER_INVOKE:
            return bl_blob_disasinvokeinst("sinvk", blob, offset);
        case OP_SUPER_INVOKE_SELF:
            return bl_blob_disasbyteinst("sinvks", blob, offset);
        default:
            printf("unknown opcode %d\n", instruction);
            return offset + 1;
    }
}

#define FILE_ERROR(type, message) \
    file_close(file);             \
    RETURN_ERROR(#type " -> %s", message, file->path->chars);
#define RETURN_STATUS(status)              \
    if((status) == 0)                      \
    {                                      \
        RETURN_TRUE;                       \
    }                                      \
    else                                   \
    {                                      \
        FILE_ERROR(File, strerror(errno)); \
    }
#define DENY_STD()              \
    if(file->mode->length == 0) \
        RETURN_ERROR("method not supported for std files");
#define SET_DICT_STRING(d, n, l, v) bl_dict_addentry(vm, d, GC_L_STRING(n, l), v)

static int file_close(ObjFile* file)
{
    if(file->file != NULL && !bl_object_isstdfile(file))
    {
        fflush(file->file);
        int result = fclose(file->file);
        file->file = NULL;
        file->isopen = false;
        return result;
    }
    return -1;
}

// fixme: mode is cast to char*
static void file_open(ObjFile* file)
{
    if((file->file == NULL || !file->isopen) && !bl_object_isstdfile(file))
    {
        char* mode = file->mode->chars;
        if(strstr(file->mode->chars, "w") != NULL && strstr(file->mode->chars, "+") != NULL)
        {
            mode = (char*)"a+";
        }
        file->file = fopen(file->path->chars, mode);
        file->isopen = true;
    }
}

bool cfn_file(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_RANGE(file, 1, 2);
    ENFORCE_ARG_TYPE(file, 0, bl_value_isstring);
    ObjString* path = AS_STRING(args[0]);
    if(path->length == 0)
    {
        RETURN_ERROR("file path cannot be empty");
    }
    ObjString* mode = NULL;
    if(argcount == 2)
    {
        ENFORCE_ARG_TYPE(file, 1, bl_value_isstring);
        mode = AS_STRING(args[1]);
    }
    else
    {
        mode = (ObjString*)bl_mem_gcprotect(vm, (Object*)bl_string_copystring(vm, "r", 1));
    }
    ObjFile* file = (ObjFile*)bl_mem_gcprotect(vm, (Object*)bl_object_makefile(vm, path, mode));
    file_open(file);
    RETURN_OBJ(file);
}

bool objfn_file_exists(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(exists, 0);
    ObjFile* file = AS_FILE(METHOD_OBJECT);
    DENY_STD();
    RETURN_BOOL(bl_util_fileexists(file->path->chars));
}

bool objfn_file_close(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(close, 0);
    file_close(AS_FILE(METHOD_OBJECT));
    return bl_value_returnempty(vm, args);
    ;
}

bool objfn_file_open(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(open, 0);
    file_open(AS_FILE(METHOD_OBJECT));
    return bl_value_returnempty(vm, args);
    ;
}

bool objfn_file_isopen(VMState* vm, int argcount, Value* args)
{
    ObjFile* file;
    (void)vm;
    (void)argcount;
    file = AS_FILE(METHOD_OBJECT);
    RETURN_BOOL(bl_object_isstdfile(file) || (file->isopen && file->file != NULL));
}

bool objfn_file_isclosed(VMState* vm, int argcount, Value* args)
{
    ObjFile* file;
    (void)argcount;
    (void)vm;
    file = AS_FILE(METHOD_OBJECT);
    RETURN_BOOL(!bl_object_isstdfile(file) && !file->isopen && file->file == NULL);
}

bool objfn_file_read(VMState* vm, int argcount, Value* args)
{
    size_t filesize;
    size_t filesizereal;
    bool inbinarymode;
    char* buffer;
    size_t bytesread;
    struct stat stats;
    ObjFile* file;
    ENFORCE_ARG_RANGE(read, 0, 1);
    filesize = -1;
    filesizereal = -1;
    if(argcount == 1)
    {
        ENFORCE_ARG_TYPE(read, 0, bl_value_isnumber);
        filesize = (size_t)AS_NUMBER(args[0]);
    }
    file = AS_FILE(METHOD_OBJECT);
    inbinarymode = strstr(file->mode->chars, "b") != NULL;
    if(!bl_object_isstdfile(file))
    {
        // file is in read mode and file does not exist
        if(strstr(file->mode->chars, "r") != NULL && !bl_util_fileexists(file->path->chars))
        {
            FILE_ERROR(NotFound, "no such file or directory");
        }
        // file is in write only mode
        else if(strstr(file->mode->chars, "w") != NULL && strstr(file->mode->chars, "+") == NULL)
        {
            FILE_ERROR(Unsupported, "cannot read file in write mode");
        }
        if(!file->isopen)
        {// open the file if it isn't open
            file_open(file);
        }
        if(file->file == NULL)
        {
            FILE_ERROR(Read, "could not read file");
        }
        // Get file size
        if(lstat(file->path->chars, &stats) == 0)
        {
            filesizereal = (size_t)stats.st_size;
        }
        else
        {
            // fallback
            fseek(file->file, 0L, SEEK_END);
            filesizereal = ftell(file->file);
            rewind(file->file);
        }
        if(filesize == (size_t)-1 || filesize > filesizereal)
        {
            filesize = filesizereal;
        }
    }
    else
    {
        // stdout should not read
        if(fileno(stdout) == fileno(file->file) || fileno(stderr) == fileno(file->file))
        {
            FILE_ERROR(Unsupported, "cannot read from output file");
        }
        // for non-file objects such as stdin
        // minimum read bytes should be 1
        if(filesize == (size_t)-1)
        {
            filesize = 1;
        }
    }
    buffer = (char*)ALLOCATE(char, filesize + 1);// +1 for terminator '\0'
    if(buffer == NULL && filesize != 0)
    {
        FILE_ERROR(Buffer, "not enough memory to read file");
    }
    bytesread = fread(buffer, sizeof(char), filesize, file->file);
    if(bytesread == 0 && filesize != 0 && filesize == filesizereal)
    {
        FILE_ERROR(Read, "could not read file contents");
    }
    // we made use of +1 so we can terminate the string.
    if(buffer != NULL)
    {
        buffer[bytesread] = '\0';
    }
    // close file
    /*if (bytesread == filesize) {
    file_close(file);
  }*/
    file_close(file);
    if(!inbinarymode)
    {
        RETURN_T_STRING(buffer, bytesread);
    }
    RETURN_OBJ(bl_bytes_takebytes(vm, (unsigned char*)buffer, bytesread));
}

bool objfn_file_gets(VMState* vm, int argcount, Value* args)
{
    bool inbinarymode;
    long length;
    long end;
    long currentpos;
    size_t bytesread;
    char* buffer;
    ObjFile* file;
    ENFORCE_ARG_RANGE(gets, 0, 1);
    length = -1;
    if(argcount == 1)
    {
        ENFORCE_ARG_TYPE(read, 0, bl_value_isnumber);
        length = (size_t)AS_NUMBER(args[0]);
    }
    file = AS_FILE(METHOD_OBJECT);
    inbinarymode = strstr(file->mode->chars, "b") != NULL;
    if(!bl_object_isstdfile(file))
    {
        // file is in read mode and file does not exist
        if(strstr(file->mode->chars, "r") != NULL && !bl_util_fileexists(file->path->chars))
        {
            FILE_ERROR(NotFound, "no such file or directory");
        }
        // file is in write only mode
        else if(strstr(file->mode->chars, "w") != NULL && strstr(file->mode->chars, "+") == NULL)
        {
            FILE_ERROR(Unsupported, "cannot read file in write mode");
        }
        if(!file->isopen)
        {// open the file if it isn't open
            FILE_ERROR(Read, "file not open");
        }
        if(file->file == NULL)
        {
            FILE_ERROR(Read, "could not read file");
        }
        if(length == -1)
        {
            currentpos = ftell(file->file);
            fseek(file->file, 0L, SEEK_END);
            end = ftell(file->file);
            // go back to where we were before.
            fseek(file->file, currentpos, SEEK_SET);
            length = end - currentpos;
        }
    }
    else
    {
        // stdout should not read
        if(fileno(stdout) == fileno(file->file) || fileno(stderr) == fileno(file->file))
        {
            FILE_ERROR(Unsupported, "cannot read from output file");
        }
        // for non-file objects such as stdin
        // minimum read bytes should be 1
        if(length == -1)
        {
            length = 1;
        }
    }
    buffer = (char*)ALLOCATE(char, length + 1);// +1 for terminator '\0'
    if(buffer == NULL && length != 0)
    {
        FILE_ERROR(Buffer, "not enough memory to read file");
    }
    bytesread = fread(buffer, sizeof(char), length, file->file);
    if(bytesread == 0 && length != 0)
    {
        FILE_ERROR(Read, "could not read file contents");
    }
    // we made use of +1, so we can terminate the string.
    if(buffer != NULL)
    {
        buffer[bytesread] = '\0';
    }
    if(!inbinarymode)
    {
        RETURN_T_STRING(buffer, bytesread);
    }
    RETURN_OBJ(bl_bytes_takebytes(vm, (unsigned char*)buffer, bytesread));
}

bool objfn_file_write(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(write, 1);
    ObjFile* file = AS_FILE(METHOD_OBJECT);
    ObjString* string = NULL;
    ObjBytes* bytes = NULL;
    bool inbinarymode = strstr(file->mode->chars, "b") != NULL;
    unsigned char* data;
    int length;
    if(!inbinarymode)
    {
        ENFORCE_ARG_TYPE(write, 0, bl_value_isstring);
        string = AS_STRING(args[0]);
        data = (unsigned char*)string->chars;
        length = string->length;
    }
    else
    {
        ENFORCE_ARG_TYPE(write, 0, bl_value_isbytes);
        bytes = AS_BYTES(args[0]);
        data = bytes->bytes.bytes;
        length = bytes->bytes.count;
    }
    // file is in read only mode
    if(!bl_object_isstdfile(file))
    {
        if(strstr(file->mode->chars, "r") != NULL && strstr(file->mode->chars, "+") == NULL)
        {
            FILE_ERROR(Unsupported, "cannot write into non-writable file");
        }
        if(length == 0)
        {
            FILE_ERROR(Write, "cannot write empty buffer to file");
        }
        if(file->file == NULL || !file->isopen)
        {// open the file if it isn't open
            file_open(file);
        }
        if(file->file == NULL)
        {
            FILE_ERROR(Write, "could not write to file");
        }
    }
    else
    {
        // stdin should not write
        if(fileno(stdin) == fileno(file->file))
        {
            FILE_ERROR(Unsupported, "cannot write to input file");
        }
    }
    size_t count = fwrite(data, sizeof(unsigned char), length, file->file);
    // close file
    file_close(file);
    if(count > (size_t)0)
    {
        RETURN_TRUE;
    }
    RETURN_FALSE;
}

bool objfn_file_puts(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(puts, 1);
    ObjFile* file = AS_FILE(METHOD_OBJECT);
    ObjString* string = NULL;
    ObjBytes* bytes = NULL;
    bool inbinarymode = strstr(file->mode->chars, "b") != NULL;
    unsigned char* data;
    int length;
    if(!inbinarymode)
    {
        ENFORCE_ARG_TYPE(write, 0, bl_value_isstring);
        string = AS_STRING(args[0]);
        data = (unsigned char*)string->chars;
        length = string->length;
    }
    else
    {
        ENFORCE_ARG_TYPE(write, 0, bl_value_isbytes);
        bytes = AS_BYTES(args[0]);
        data = bytes->bytes.bytes;
        length = bytes->bytes.count;
    }
    // file is in read only mode
    if(!bl_object_isstdfile(file))
    {
        if(strstr(file->mode->chars, "r") != NULL && strstr(file->mode->chars, "+") == NULL)
        {
            FILE_ERROR(Unsupported, "cannot write into non-writable file");
        }
        if(length == 0)
        {
            FILE_ERROR(Write, "cannot write empty buffer to file");
        }
        if(!file->isopen)
        {// open the file if it isn't open
            FILE_ERROR(Write, "file not open");
        }
        if(file->file == NULL)
        {
            FILE_ERROR(Write, "could not write to file");
        }
    }
    else
    {
        // stdin should not write
        if(fileno(stdin) == fileno(file->file))
        {
            FILE_ERROR(Unsupported, "cannot write to input file");
        }
    }
    size_t count = fwrite(data, sizeof(unsigned char), length, file->file);
    if(count > (size_t)0 || length == 0)
    {
        RETURN_TRUE;
    }
    RETURN_FALSE;
}

bool objfn_file_number(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(number, 0);
    ObjFile* file = AS_FILE(METHOD_OBJECT);
    if(file->file == NULL)
    {
        RETURN_NUMBER(-1);
    }
    else
    {
        RETURN_NUMBER(fileno(file->file));
    }
}

bool objfn_file_istty(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(istty, 0);
    ObjFile* file = AS_FILE(METHOD_OBJECT);
    if(bl_object_isstdfile(file))
    {
        RETURN_BOOL(isatty(fileno(file->file)) && fileno(file->file) == fileno(stdout));
    }
    RETURN_FALSE;
}

bool objfn_file_flush(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(flush, 0);
    ObjFile* file = AS_FILE(METHOD_OBJECT);
    if(!file->isopen)
    {
        FILE_ERROR(Unsupported, "i/o operation on closed file");
    }
#if defined(IS_UNIX)
    // using fflush on stdin have undesired effect on unix environments
    if(fileno(stdin) == fileno(file->file))
    {
        while((getchar()) != '\n')
        {
        }
    }
    else
    {
        fflush(file->file);
    }
#else
    fflush(file->file);
#endif
    return bl_value_returnempty(vm, args);
    ;
}

bool objfn_file_stats(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(stats, 0);
    ObjFile* file = AS_FILE(METHOD_OBJECT);
    ObjDict* dict = (ObjDict*)bl_mem_gcprotect(vm, (Object*)bl_object_makedict(vm));
    if(!bl_object_isstdfile(file))
    {
        if(bl_util_fileexists(file->path->chars))
        {
            struct stat stats;
            if(lstat(file->path->chars, &stats) == 0)
            {
                // read mode
                SET_DICT_STRING(dict, "isreadable", 11, BOOL_VAL(((stats.st_mode & S_IRUSR) != 0)));
                // write mode
                SET_DICT_STRING(dict, "iswritable", 11, BOOL_VAL(((stats.st_mode & S_IWUSR) != 0)));
                // execute mode
                SET_DICT_STRING(dict, "isexecutable", 13, BOOL_VAL(((stats.st_mode & S_IXUSR) != 0)));
                // is symbolic link
                SET_DICT_STRING(dict, "issymbolic", 11, BOOL_VAL((S_ISLNK(stats.st_mode) != 0)));
                // file details
                SET_DICT_STRING(dict, "size", 4, NUMBER_VAL(stats.st_size));
                SET_DICT_STRING(dict, "mode", 4, NUMBER_VAL(stats.st_mode));
                SET_DICT_STRING(dict, "dev", 3, NUMBER_VAL(stats.st_dev));
                SET_DICT_STRING(dict, "ino", 3, NUMBER_VAL(stats.st_ino));
                SET_DICT_STRING(dict, "nlink", 5, NUMBER_VAL(stats.st_nlink));
                SET_DICT_STRING(dict, "uid", 3, NUMBER_VAL(stats.st_uid));
                SET_DICT_STRING(dict, "gid", 3, NUMBER_VAL(stats.st_gid));
                // last modified time in milliseconds
                SET_DICT_STRING(dict, "mtime", 5, NUMBER_VAL(stats.st_mtime));
                // last accessed time in milliseconds
                SET_DICT_STRING(dict, "mtime", 5, NUMBER_VAL(stats.st_mtime));
                // last c time in milliseconds
                SET_DICT_STRING(dict, "ctime", 5, NUMBER_VAL(stats.st_ctime));
            }
        }
        else
        {
            RETURN_ERROR("cannot get stats for non-existing file");
        }
    }
    else
    {
        // we are dealing with an std
        if(fileno(stdin) == fileno(file->file))
        {
            SET_DICT_STRING(dict, "isreadable", 11, TRUE_VAL);
            SET_DICT_STRING(dict, "iswritable", 11, FALSE_VAL);
        }
        else if(fileno(stdout) == fileno(file->file) || fileno(stderr) == fileno(file->file))
        {
            SET_DICT_STRING(dict, "isreadable", 11, FALSE_VAL);
            SET_DICT_STRING(dict, "iswritable", 11, TRUE_VAL);
        }
        SET_DICT_STRING(dict, "isexecutable", 13, FALSE_VAL);
        SET_DICT_STRING(dict, "size", 4, NUMBER_VAL(1));
    }
    RETURN_OBJ(dict);
}

bool objfn_file_symlink(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(symlink, 1);
    ENFORCE_ARG_TYPE(symlink, 0, bl_value_isstring);
    ObjFile* file = AS_FILE(METHOD_OBJECT);
    DENY_STD();
    if(bl_util_fileexists(file->path->chars))
    {
        ObjString* path = AS_STRING(args[0]);
        RETURN_BOOL(symlink(file->path->chars, path->chars) == 0);
    }
    else
    {
        RETURN_ERROR("symlink to file not found");
    }
}

bool objfn_file_delete(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(delete, 0);
    ObjFile* file = AS_FILE(METHOD_OBJECT);
    DENY_STD();
    if(file_close(file) != 0)
    {
        RETURN_ERROR("error closing file.");
    }
    RETURN_STATUS(unlink(file->path->chars));
}

bool objfn_file_rename(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(rename, 1);
    ENFORCE_ARG_TYPE(rename, 0, bl_value_isstring);
    ObjFile* file = AS_FILE(METHOD_OBJECT);
    DENY_STD();
    if(bl_util_fileexists(file->path->chars))
    {
        ObjString* newname = AS_STRING(args[0]);
        if(newname->length == 0)
        {
            FILE_ERROR(Operation, "file name cannot be empty");
        }
        file_close(file);
        RETURN_STATUS(rename(file->path->chars, newname->chars));
    }
    else
    {
        RETURN_ERROR("file not found");
    }
}

bool objfn_file_path(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(path, 0);
    ObjFile* file = AS_FILE(METHOD_OBJECT);
    DENY_STD();
    RETURN_OBJ(file->path);
}

bool objfn_file_mode(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(mode, 0);
    ObjFile* file = AS_FILE(METHOD_OBJECT);
    DENY_STD();
    RETURN_OBJ(file->mode);
}

bool objfn_file_name(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(name, 0);
    ObjFile* file = AS_FILE(METHOD_OBJECT);
    DENY_STD();
    char* name = bl_util_getrealfilename(file->path->chars);
    RETURN_L_STRING(name, strlen(name));
}

bool objfn_file_abspath(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(abspath, 0);
    ObjFile* file = AS_FILE(METHOD_OBJECT);
    DENY_STD();
    char* abspath = realpath(file->path->chars, NULL);
    if(abspath != NULL)
        RETURN_L_STRING(abspath, strlen(abspath));
    RETURN_L_STRING("", 0);
}

bool objfn_file_copy(VMState* vm, int argcount, Value* args)
{
    size_t nread;
    size_t nwrite;
    const char* mode;
    unsigned char buffer[8192];
    FILE* fp;
    ObjFile* file;    
    ENFORCE_ARG_COUNT(copy, 1);
    ENFORCE_ARG_TYPE(copy, 0, bl_value_isstring);
    file = AS_FILE(METHOD_OBJECT);
    DENY_STD();
    if(bl_util_fileexists(file->path->chars))
    {
        ObjString* name = AS_STRING(args[0]);
        if(strstr(file->mode->chars, "r") == NULL)
        {
            FILE_ERROR(Unsupported, "file not open for reading");
        }
        mode = "w";
        // if we are dealing with a binary file
        if(strstr(file->mode->chars, "b") != NULL)
        {
            mode = "wb";
        }
        fp = fopen(name->chars, mode);
        if(fp == NULL)
        {
            FILE_ERROR(Permission, "unable to create new file");
        }
        do
        {
            nread = fread(buffer, 1, sizeof(buffer), file->file);
            if(nread > 0)
            {
                nwrite = fwrite(buffer, 1, nread, fp);
            }
            else
            {
                nwrite = 0;
            }
        } while((nread > 0) && (nread == nwrite));
        if(nwrite > 0)
        {
            FILE_ERROR(Operation, "error copying file");
        }
        fflush(fp);
        fclose(fp);
        file_close(file);
        RETURN_BOOL(nread == nwrite);
    }
    RETURN_ERROR("file not found");
}

bool objfn_file_truncate(VMState* vm, int argcount, Value* args)
{
    off_t finalsize;
    ObjFile* file;
    ENFORCE_ARG_RANGE(truncate, 0, 1);
    finalsize = 0;
    if(argcount == 1)
    {
        ENFORCE_ARG_TYPE(truncate, 0, bl_value_isnumber);
        finalsize = (off_t)AS_NUMBER(args[0]);
    }
    file = AS_FILE(METHOD_OBJECT);
    DENY_STD();
    RETURN_STATUS(truncate(file->path->chars, finalsize));
}

bool objfn_file_chmod(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(chmod, 1);
    ENFORCE_ARG_TYPE(chmod, 0, bl_value_isnumber);
    ObjFile* file = AS_FILE(METHOD_OBJECT);
    DENY_STD();
    if(bl_util_fileexists(file->path->chars))
    {
        int mode = AS_NUMBER(args[0]);
        RETURN_STATUS(chmod(file->path->chars, (mode_t)mode));
    }
    else
    {
        RETURN_ERROR("file not found");
    }
}

bool objfn_file_settimes(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(settimes, 2);
    ENFORCE_ARG_TYPE(settimes, 0, bl_value_isnumber);
    ENFORCE_ARG_TYPE(settimes, 1, bl_value_isnumber);
#ifdef HAVE_UTIME
    ObjFile* file = AS_FILE(METHOD_OBJECT);
    DENY_STD();
    if(bl_util_fileexists(file->path->chars))
    {
        time_t atime = (time_t)AS_NUMBER(args[0]);
        time_t mtime = (time_t)AS_NUMBER(args[1]);
        struct stat stats;
        int status = lstat(file->path->chars, &stats);
        if(status == 0)
        {
            struct utimbuf newtimes;
    #if !defined(_WIN32) && (!defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE))
            if(atime == (time_t)-1)
                newtimes.actime = stats.st_atimespec.tv_sec;
            else
                newtimes.actime = atime;
            if(mtime == (time_t)-1)
                newtimes.modtime = stats.st_mtimespec.tv_sec;
            else
                newtimes.modtime = mtime;
    #else
            if(atime == (time_t)-1)
            {
                newtimes.actime = stats.st_atime;
            }
            else
            {
                newtimes.actime = atime;
            }
            if(mtime == (time_t)-1)
            {
                newtimes.modtime = stats.st_mtime;
            }
            else
            {
                newtimes.modtime = mtime;
            }
    #endif
            RETURN_STATUS(utime(file->path->chars, &newtimes));
        }
        else
        {
            RETURN_STATUS(status);
        }
    }
    else
    {
        FILE_ERROR(Access, "file not found");
    }
#else
    RETURN_ERROR("not available: OS does not support utime");
#endif /* ifdef HAVE_UTIME */
}

bool objfn_file_seek(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(seek, 2);
    ENFORCE_ARG_TYPE(seek, 0, bl_value_isnumber);
    ENFORCE_ARG_TYPE(seek, 1, bl_value_isnumber);
    ObjFile* file = AS_FILE(METHOD_OBJECT);
    DENY_STD();
    long position = (long)AS_NUMBER(args[0]);
    int seektype = AS_NUMBER(args[1]);
    RETURN_STATUS(fseek(file->file, position, seektype));
}

bool objfn_file_tell(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(tell, 0);
    ObjFile* file = AS_FILE(METHOD_OBJECT);
    DENY_STD();
    RETURN_NUMBER(ftell(file->file));
}

#undef FILE_ERROR
#undef RETURN_STATUS
#undef SET_DICT_STRING
#undef DENY_STD
static const ModInitFunc builtinmodules[] = {
    &bl_modload_os,//
    &bl_modload_io,//
    //&bl_modload_base64,//
    &bl_modload_math,//
    &bl_modload_date,//
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
                ObjString* classname = (ObjString*)bl_mem_gcprotect(vm, (Object*)bl_string_copystring(vm, klassreg.name, (int)strlen(klassreg.name)));
                ObjClass* klass = (ObjClass*)bl_mem_gcprotect(vm, (Object*)bl_object_makeclass(vm, classname));
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
    return bl_string_copystring(vm, newstr, length);
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
    //  return bl_string_copystring(vm, str, length);
}

static ObjString* number_to_oct(VMState* vm, long long n, bool numeric)
{
    char str[66];// assume maximum of 64 bits + 2 octal indicators (0c)
    int length = sprintf(str, numeric ? "0c%llo" : "%llo", n);
    return bl_string_copystring(vm, str, length);
}

static ObjString* number_to_hex(VMState* vm, long long n, bool numeric)
{
    char str[66];// assume maximum of 64 bits + 2 hex indicators (0x)
    int length = sprintf(str, numeric ? "0x%llx" : "%llx", n);
    return bl_string_copystring(vm, str, length);
}

/**
 * time()
 *
 * returns the current timestamp in seconds
 */
bool cfn_time(VMState* vm, int argcount, Value* args)
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
bool cfn_microtime(VMState* vm, int argcount, Value* args)
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
bool cfn_id(VMState* vm, int argcount, Value* args)
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
bool cfn_hasprop(VMState* vm, int argcount, Value* args)
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
bool cfn_getprop(VMState* vm, int argcount, Value* args)
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
bool cfn_setprop(VMState* vm, int argcount, Value* args)
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
bool cfn_delprop(VMState* vm, int argcount, Value* args)
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
bool cfn_max(VMState* vm, int argcount, Value* args)
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
bool cfn_min(VMState* vm, int argcount, Value* args)
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
bool cfn_sum(VMState* vm, int argcount, Value* args)
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
bool cfn_abs(VMState* vm, int argcount, Value* args)
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
bool cfn_int(VMState* vm, int argcount, Value* args)
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
bool cfn_bin(VMState* vm, int argcount, Value* args)
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
bool cfn_oct(VMState* vm, int argcount, Value* args)
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
bool cfn_hex(VMState* vm, int argcount, Value* args)
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
bool cfn_tobool(VMState* vm, int argcount, Value* args)
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
bool cfn_tostring(VMState* vm, int argcount, Value* args)
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
bool cfn_tonumber(VMState* vm, int argcount, Value* args)
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
bool cfn_toint(VMState* vm, int argcount, Value* args)
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
bool cfn_tolist(VMState* vm, int argcount, Value* args)
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
bool cfn_todict(VMState* vm, int argcount, Value* args)
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
bool cfn_chr(VMState* vm, int argcount, Value* args)
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
bool cfn_ord(VMState* vm, int argcount, Value* args)
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
bool cfn_rand(VMState* vm, int argcount, Value* args)
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
bool cfn_typeof(VMState* vm, int argcount, Value* args)
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
bool cfn_iscallable(VMState* vm, int argcount, Value* args)
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
bool cfn_isbool(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(is_bool, 1);
    RETURN_BOOL(bl_value_isbool(args[0]));
}

/**
 * is_number(value: any)
 *
 * returns true if the value is a number or false otherwise
 */
bool cfn_isnumber(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(is_number, 1);
    RETURN_BOOL(bl_value_isnumber(args[0]));
}

/**
 * is_int(value: any)
 *
 * returns true if the value is an integer or false otherwise
 */
bool cfn_isint(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(is_int, 1);
    RETURN_BOOL(bl_value_isnumber(args[0]) && (((int)AS_NUMBER(args[0])) == AS_NUMBER(args[0])));
}

/**
 * is_string(value: any)
 *
 * returns true if the value is a string or false otherwise
 */
bool cfn_isstring(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(is_string, 1);
    RETURN_BOOL(bl_value_isstring(args[0]));
}

/**
 * is_bytes(value: any)
 *
 * returns true if the value is a bytes or false otherwise
 */
bool cfn_isbytes(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(is_bytes, 1);
    RETURN_BOOL(bl_value_isbytes(args[0]));
}

/**
 * is_list(value: any)
 *
 * returns true if the value is a list or false otherwise
 */
bool cfn_islist(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(is_list, 1);
    RETURN_BOOL(bl_value_isarray(args[0]));
}

/**
 * is_dict(value: any)
 *
 * returns true if the value is a dictionary or false otherwise
 */
bool cfn_isdict(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(is_dict, 1);
    RETURN_BOOL(bl_value_isdict(args[0]));
}

/**
 * is_object(value: any)
 *
 * returns true if the value is an object or false otherwise
 */
bool cfn_isobject(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(is_object, 1);
    RETURN_BOOL(bl_value_isobject(args[0]));
}

/**
 * is_function(value: any)
 *
 * returns true if the value is a function or false otherwise
 */
bool cfn_isfunction(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(is_function, 1);
    RETURN_BOOL(bl_value_isscriptfunction(args[0]) || bl_value_isclosure(args[0]) || bl_value_isboundfunction(args[0]) || bl_value_isnativefunction(args[0]));
}

/**
 * is_iterable(value: any)
 *
 * returns true if the value is an iterable or false otherwise
 */
bool cfn_isiterable(VMState* vm, int argcount, Value* args)
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
bool cfn_isclass(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(is_class, 1);
    RETURN_BOOL(bl_value_isclass(args[0]));
}

/**
 * is_file(value: any)
 *
 * returns true if the value is a file or false otherwise
 */
bool cfn_isfile(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(is_file, 1);
    RETURN_BOOL(bl_value_isfile(args[0]));
}

/**
 * is_instance(value: any)
 *
 * returns true if the value is an instance of a class
 */
bool cfn_isinstance(VMState* vm, int argcount, Value* args)
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
bool cfn_instanceof(VMState* vm, int argcount, Value* args)
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
bool cfn_print(VMState* vm, int argcount, Value* args)
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
    RETURN_NUMBER(0);
}

bool objfn_range_lower(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(lower, 0);
    RETURN_NUMBER(AS_RANGE(METHOD_OBJECT)->lower);
}

bool objfn_range_upper(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(upper, 0);
    RETURN_NUMBER(AS_RANGE(METHOD_OBJECT)->upper);
}

bool objfn_range_iter(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(__iter__, 1);
    ENFORCE_ARG_TYPE(__iter__, 0, bl_value_isnumber);
    ObjRange* range = AS_RANGE(METHOD_OBJECT);
    int index = AS_NUMBER(args[0]);
    if(index >= 0 && index < range->range)
    {
        if(index == 0)
            RETURN_NUMBER(range->lower);
        RETURN_NUMBER(range->lower > range->upper ? --range->lower : ++range->lower);
    }
    return bl_value_returnnil(vm, args);
}

bool objfn_range_itern(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(__itern__, 1);
    ObjRange* range = AS_RANGE(METHOD_OBJECT);
    if(bl_value_isnil(args[0]))
    {
        if(range->range == 0)
        {
            return bl_value_returnnil(vm, args);
        }
        RETURN_NUMBER(0);
    }
    if(!bl_value_isnumber(args[0]))
    {
        RETURN_ERROR("ranges are numerically indexed");
    }
    int index = (int)AS_NUMBER(args[0]) + 1;
    if(index < range->range)
    {
        RETURN_NUMBER(index);
    }
    return bl_value_returnnil(vm, args);
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

//--------- INT 16 STARTS -------------------------
DynArray* bl_object_makedynarrayint16(VMState* vm, int length)
{
    DynArray* array = (DynArray*)bl_object_allocobject(vm, sizeof(DynArray), OBJ_BYTES);
    array->length = length;
    array->buffer = ALLOCATE(int16_t, length);
    return array;
}

bool modfn_array_int16array(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(int16array, 1);
    if(bl_value_isnumber(args[0]))
    {
        RETURN_OBJ(new_array(vm, bl_object_makedynarrayint16(vm, (int)AS_NUMBER(args[0]))));
    }
    else if(bl_value_isarray(args[0]))
    {
        ObjArray* list = AS_LIST(args[0]);
        DynArray* array = bl_object_makedynarrayint16(vm, list->items.count);
        int16_t* values = (int16_t*)array->buffer;
        for(int i = 0; i < list->items.count; i++)
        {
            if(!bl_value_isnumber(list->items.values[i]))
            {
                RETURN_ERROR("Int16Array() expects a list of valid int16");
            }
            values[i] = (int16_t)AS_NUMBER(list->items.values[i]);
        }
        RETURN_OBJ(new_array(vm, array));
    }
    RETURN_ERROR("expected array size or int16 list as argument");
}

bool modfn_array_int16append(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(append, 2);
    ENFORCE_ARG_TYPE(append, 0, bl_value_ispointer);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    if(bl_value_isnumber(args[1]))
    {
        array->length++;
        array->buffer = GROW_ARRAY(int16_t, sizeof(int16_t), array->buffer, array->length - 1, array->length);
        int16_t* values = (int16_t*)array->buffer;
        values[array->length - 1] = (int16_t)AS_NUMBER(args[1]);
    }
    else if(bl_value_isarray(args[1]))
    {
        ObjArray* list = AS_LIST(args[1]);
        if(list->items.count > 0)
        {
            array->buffer = GROW_ARRAY(int16_t, sizeof(int16_t), array->buffer, array->length, array->length + list->items.count);
            int16_t* values = (int16_t*)array->buffer;
            for(int i = 0; i < list->items.count; i++)
            {
                if(!bl_value_isnumber(list->items.values[i]))
                {
                    RETURN_ERROR("Int16Array lists can only contain numbers");
                }
                values[array->length + i] = (int16_t)AS_NUMBER(list->items.values[i]);
            }
            array->length += list->items.count;
        }
    }
    else
    {
        RETURN_ERROR("Int16Array can only append an int16 or a list of int16");
    }
    return bl_value_returnempty(vm, args);
    ;
}

bool modfn_array_int16get(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(get, 2);
    ENFORCE_ARG_TYPE(get, 0, bl_value_ispointer);
    ENFORCE_ARG_TYPE(get, 1, bl_value_isnumber);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    int16_t* data = (int16_t*)array->buffer;
    int index = AS_NUMBER(args[1]);
    if(index < 0 || index >= array->length)
    {
        RETURN_ERROR("Int16Array index %d out of range", index);
    }
    RETURN_NUMBER((double)data[index]);
}

bool modfn_array_int16reverse(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(reverse, 1);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    int16_t* data = (int16_t*)array->buffer;
    DynArray* narray = bl_object_makedynarrayint16(vm, array->length);
    int16_t* ndata = (int16_t*)narray->buffer;
    for(int i = array->length - 1; i >= 0; i--)
    {
        ndata[i] = data[i];
    }
    RETURN_OBJ(new_array(vm, narray));
}

bool modfn_array_int16clone(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(clone, 1);
    ENFORCE_ARG_TYPE(clone, 0, bl_value_ispointer);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    DynArray* narray = bl_object_makedynarrayint16(vm, array->length);
    memcpy(narray->buffer, array->buffer, array->length);
    RETURN_OBJ(new_array(vm, narray));
}

bool modfn_array_int16pop(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(pop, 1);
    ENFORCE_ARG_TYPE(pop, 0, bl_value_ispointer);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    int16_t last = ((int16_t*)array->buffer)[array->length - 1];
    array->length--;
    RETURN_NUMBER(last);
}

bool modfn_array_int16tolist(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(to_list, 1);
    ENFORCE_ARG_TYPE(to_list, 0, bl_value_ispointer);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    int16_t* values = (int16_t*)array->buffer;
    ObjArray* list = (ObjArray*)bl_mem_gcprotect(vm, (Object*)bl_object_makelist(vm));
    for(int i = 0; i < array->length; i++)
    {
        bl_array_push(vm, list, NUMBER_VAL((double)values[i]));
    }
    RETURN_OBJ(list);
}

bool modfn_array_int16tobytes(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(to_list, 1);
    ENFORCE_ARG_TYPE(to_list, 0, bl_value_ispointer);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    ObjBytes* bytes = (ObjBytes*)bl_mem_gcprotect(vm, (Object*)bl_object_makebytes(vm, array->length * 2));
    memcpy(bytes->bytes.bytes, array->buffer, array->length * 2);
    RETURN_OBJ(bytes);
}

bool modfn_array_int16iter_(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(@iter, 2);
    ENFORCE_ARG_TYPE(@iter, 0, bl_value_ispointer);
    ENFORCE_ARG_TYPE(@iter, 1, bl_value_isnumber);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    int16_t* values = (int16_t*)array->buffer;
    int index = AS_NUMBER(args[1]);
    if(index > -1 && index < array->length)
    {
        RETURN_NUMBER(values[index]);
    }
    return bl_value_returnnil(vm, args);
}

//--------- INT 32 STARTS -------------------------
DynArray* bl_object_makedynarrayint32(VMState* vm, int length)
{
    DynArray* array = (DynArray*)bl_object_allocobject(vm, sizeof(DynArray), OBJ_BYTES);
    array->length = length;
    array->buffer = ALLOCATE(int32_t, length);
    return array;
}

bool modfn_array_int32array(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(int32array, 1);
    if(bl_value_isnumber(args[0]))
    {
        RETURN_OBJ(new_array(vm, bl_object_makedynarrayint32(vm, (int)AS_NUMBER(args[0]))));
    }
    else if(bl_value_isarray(args[0]))
    {
        ObjArray* list = AS_LIST(args[0]);
        DynArray* array = bl_object_makedynarrayint32(vm, list->items.count);
        int32_t* values = (int32_t*)array->buffer;
        for(int i = 0; i < list->items.count; i++)
        {
            if(!bl_value_isnumber(list->items.values[i]))
            {
                RETURN_ERROR("Int32Array() expects a list of valid int32");
            }
            values[i] = (int32_t)AS_NUMBER(list->items.values[i]);
        }
        RETURN_OBJ(new_array(vm, array));
    }
    RETURN_ERROR("expected array size or int32 list as argument");
}

bool modfn_array_int32append(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(append, 2);
    ENFORCE_ARG_TYPE(append, 0, bl_value_ispointer);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    if(bl_value_isnumber(args[1]))
    {
        array->length++;
        array->buffer = GROW_ARRAY(int32_t, sizeof(int32_t), array->buffer, array->length - 1, array->length);
        int32_t* values = (int32_t*)array->buffer;
        values[array->length - 1] = (int32_t)AS_NUMBER(args[1]);
    }
    else if(bl_value_isarray(args[1]))
    {
        ObjArray* list = AS_LIST(args[1]);
        if(list->items.count > 0)
        {
            array->buffer = GROW_ARRAY(int32_t, sizeof(int32_t), array->buffer, array->length, array->length + list->items.count);
            int32_t* values = (int32_t*)array->buffer;
            for(int i = 0; i < list->items.count; i++)
            {
                if(!bl_value_isnumber(list->items.values[i]))
                {
                    RETURN_ERROR("Int32Array lists can only contain numbers");
                }
                values[array->length + i] = (int32_t)AS_NUMBER(list->items.values[i]);
            }
            array->length += list->items.count;
        }
    }
    else
    {
        RETURN_ERROR("Int32Array can only append an int32 or a list of int32");
    }
    return bl_value_returnempty(vm, args);
    ;
}

bool modfn_array_int32get(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(get, 2);
    ENFORCE_ARG_TYPE(get, 0, bl_value_ispointer);
    ENFORCE_ARG_TYPE(get, 1, bl_value_isnumber);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    int32_t* data = (int32_t*)array->buffer;
    int index = AS_NUMBER(args[1]);
    if(index < 0 || index >= array->length)
    {
        RETURN_ERROR("Int32Array index %d out of range", index);
    }
    RETURN_NUMBER((double)data[index]);
}

bool modfn_array_int32reverse(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(reverse, 1);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    int32_t* data = (int32_t*)array->buffer;
    DynArray* narray = bl_object_makedynarrayint32(vm, array->length);
    int32_t* ndata = (int32_t*)narray->buffer;
    for(int i = array->length - 1; i >= 0; i--)
    {
        ndata[i] = data[i];
    }
    RETURN_OBJ(new_array(vm, narray));
}

bool modfn_array_int32clone(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(clone, 1);
    ENFORCE_ARG_TYPE(clone, 0, bl_value_ispointer);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    DynArray* narray = bl_object_makedynarrayint32(vm, array->length);
    memcpy(narray->buffer, array->buffer, array->length);
    RETURN_OBJ(new_array(vm, narray));
}

bool modfn_array_int32pop(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(pop, 1);
    ENFORCE_ARG_TYPE(pop, 0, bl_value_ispointer);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    int32_t last = ((int32_t*)array->buffer)[array->length - 1];
    array->length--;
    RETURN_NUMBER(last);
}

bool modfn_array_int32tolist(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(to_list, 1);
    ENFORCE_ARG_TYPE(to_list, 0, bl_value_ispointer);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    int32_t* values = (int32_t*)array->buffer;
    ObjArray* list = (ObjArray*)bl_mem_gcprotect(vm, (Object*)bl_object_makelist(vm));
    for(int i = 0; i < array->length; i++)
    {
        bl_array_push(vm, list, NUMBER_VAL((double)values[i]));
    }
    RETURN_OBJ(list);
}

bool modfn_array_int32tobytes(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(to_list, 1);
    ENFORCE_ARG_TYPE(to_list, 0, bl_value_ispointer);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    ObjBytes* bytes = (ObjBytes*)bl_mem_gcprotect(vm, (Object*)bl_object_makebytes(vm, array->length * 4));
    memcpy(bytes->bytes.bytes, array->buffer, array->length * 4);
    RETURN_OBJ(bytes);
}

bool modfn_array_int32iter_(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(@iter, 2);
    ENFORCE_ARG_TYPE(@iter, 0, bl_value_ispointer);
    ENFORCE_ARG_TYPE(@iter, 1, bl_value_isnumber);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    int32_t* values = (int32_t*)array->buffer;
    int index = AS_NUMBER(args[1]);
    if(index > -1 && index < array->length)
    {
        RETURN_NUMBER(values[index]);
    }
    return bl_value_returnnil(vm, args);
}

//--------- INT 64 STARTS -------------------------
DynArray* bl_object_makedynarrayint64(VMState* vm, int length)
{
    DynArray* array = (DynArray*)bl_object_allocobject(vm, sizeof(DynArray), OBJ_BYTES);
    array->length = length;
    array->buffer = ALLOCATE(int64_t, length);
    return array;
}

bool modfn_array_int64array(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(int64array, 1);
    if(bl_value_isnumber(args[0]))
    {
        RETURN_OBJ(new_array(vm, bl_object_makedynarrayint64(vm, (int)AS_NUMBER(args[0]))));
    }
    else if(bl_value_isarray(args[0]))
    {
        ObjArray* list = AS_LIST(args[0]);
        DynArray* array = bl_object_makedynarrayint64(vm, list->items.count);
        int64_t* values = (int64_t*)array->buffer;
        for(int i = 0; i < list->items.count; i++)
        {
            if(!bl_value_isnumber(list->items.values[i]))
            {
                RETURN_ERROR("Int64Array() expects a list of valid int64");
            }
            values[i] = (int64_t)AS_NUMBER(list->items.values[i]);
        }
        RETURN_OBJ(new_array(vm, array));
    }
    RETURN_ERROR("expected array size or int64 list as argument");
}

bool modfn_array_int64append(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(append, 2);
    ENFORCE_ARG_TYPE(append, 0, bl_value_ispointer);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    if(bl_value_isnumber(args[1]))
    {
        array->length++;
        array->buffer = GROW_ARRAY(int64_t, sizeof(int64_t), array->buffer, array->length - 1, array->length);
        int64_t* values = (int64_t*)array->buffer;
        values[array->length - 1] = (int64_t)AS_NUMBER(args[1]);
    }
    else if(bl_value_isarray(args[1]))
    {
        ObjArray* list = AS_LIST(args[1]);
        if(list->items.count > 0)
        {
            array->buffer = GROW_ARRAY(int64_t, sizeof(int64_t), array->buffer, array->length, array->length + list->items.count);
            int64_t* values = (int64_t*)array->buffer;
            for(int i = 0; i < list->items.count; i++)
            {
                if(!bl_value_isnumber(list->items.values[i]))
                {
                    RETURN_ERROR("Int64Array lists can only contain numbers");
                }
                values[array->length + i] = (int64_t)AS_NUMBER(list->items.values[i]);
            }
            array->length += list->items.count;
        }
    }
    else
    {
        RETURN_ERROR("Int64Array can only append an int64 or a list of int64");
    }
    return bl_value_returnempty(vm, args);
    ;
}

bool modfn_array_int64get(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(get, 2);
    ENFORCE_ARG_TYPE(get, 0, bl_value_ispointer);
    ENFORCE_ARG_TYPE(get, 1, bl_value_isnumber);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    int64_t* data = (int64_t*)array->buffer;
    int index = AS_NUMBER(args[1]);
    if(index < 0 || index >= array->length)
    {
        RETURN_ERROR("Int64Array index %d out of range", index);
    }
    RETURN_NUMBER((double)data[index]);
}

bool modfn_array_int64reverse(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(reverse, 1);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    int64_t* data = (int64_t*)array->buffer;
    DynArray* narray = bl_object_makedynarrayint64(vm, array->length);
    int64_t* ndata = (int64_t*)narray->buffer;
    for(int i = array->length - 1; i >= 0; i--)
    {
        ndata[i] = data[i];
    }
    RETURN_OBJ(new_array(vm, narray));
}

bool modfn_array_int64clone(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(clone, 1);
    ENFORCE_ARG_TYPE(clone, 0, bl_value_ispointer);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    DynArray* narray = bl_object_makedynarrayint64(vm, array->length);
    memcpy(narray->buffer, array->buffer, array->length);
    RETURN_OBJ(new_array(vm, narray));
}

bool modfn_array_int64pop(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(pop, 1);
    ENFORCE_ARG_TYPE(pop, 0, bl_value_ispointer);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    int64_t last = ((int64_t*)array->buffer)[array->length - 1];
    array->length--;
    RETURN_NUMBER(last);
}

bool modfn_array_int64tolist(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(to_list, 1);
    ENFORCE_ARG_TYPE(to_list, 0, bl_value_ispointer);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    int64_t* values = (int64_t*)array->buffer;
    ObjArray* list = (ObjArray*)bl_mem_gcprotect(vm, (Object*)bl_object_makelist(vm));
    for(int i = 0; i < array->length; i++)
    {
        bl_array_push(vm, list, NUMBER_VAL((double)values[i]));
    }
    RETURN_OBJ(list);
}

bool modfn_array_int64tobytes(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(to_list, 1);
    ENFORCE_ARG_TYPE(to_list, 0, bl_value_ispointer);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    ObjBytes* bytes = (ObjBytes*)bl_mem_gcprotect(vm, (Object*)bl_object_makebytes(vm, array->length * 8));
    memcpy(bytes->bytes.bytes, array->buffer, array->length * 8);
    RETURN_OBJ(bytes);
}

bool modfn_array_int64iter_(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(@iter, 2);
    ENFORCE_ARG_TYPE(@iter, 0, bl_value_ispointer);
    ENFORCE_ARG_TYPE(@iter, 1, bl_value_isnumber);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    int64_t* values = (int64_t*)array->buffer;
    int index = AS_NUMBER(args[1]);
    if(index > -1 && index < array->length)
    {
        RETURN_NUMBER(values[index]);
    }
    return bl_value_returnnil(vm, args);
}

//--------- Unsigned INT 16 STARTS ----------------
DynArray* bl_object_makedynarrayuint16(VMState* vm, int length)
{
    DynArray* array = (DynArray*)bl_object_allocobject(vm, sizeof(DynArray), OBJ_BYTES);
    array->length = length;
    array->buffer = ALLOCATE(uint16_t, length);
    return array;
}

bool modfn_array_uint16array(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(uint16array, 1);
    if(bl_value_isnumber(args[0]))
    {
        RETURN_OBJ(new_array(vm, bl_object_makedynarrayuint16(vm, (int)AS_NUMBER(args[0]))));
    }
    else if(bl_value_isarray(args[0]))
    {
        ObjArray* list = AS_LIST(args[0]);
        DynArray* array = bl_object_makedynarrayuint16(vm, list->items.count);
        uint16_t* values = (uint16_t*)array->buffer;
        for(int i = 0; i < list->items.count; i++)
        {
            if(!bl_value_isnumber(list->items.values[i]))
            {
                RETURN_ERROR("UInt16Array() expects a list of valid uint16");
            }
            values[i] = (uint16_t)AS_NUMBER(list->items.values[i]);
        }
        RETURN_OBJ(new_array(vm, array));
    }
    RETURN_ERROR("expected array size or uint16 list as argument");
}

bool modfn_array_uint16append(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(append, 2);
    ENFORCE_ARG_TYPE(append, 0, bl_value_ispointer);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    if(bl_value_isnumber(args[1]))
    {
        array->length++;
        array->buffer = GROW_ARRAY(uint16_t, sizeof(uint16_t), array->buffer, array->length - 1, array->length);
        uint16_t* values = (uint16_t*)array->buffer;
        values[array->length - 1] = (uint16_t)AS_NUMBER(args[1]);
    }
    else if(bl_value_isarray(args[1]))
    {
        ObjArray* list = AS_LIST(args[1]);
        if(list->items.count > 0)
        {
            array->buffer = GROW_ARRAY(uint16_t, sizeof(uint16_t), array->buffer, array->length, array->length + list->items.count);
            uint16_t* values = (uint16_t*)array->buffer;
            for(int i = 0; i < list->items.count; i++)
            {
                if(!bl_value_isnumber(list->items.values[i]))
                {
                    RETURN_ERROR("UInt16Array lists can only contain numbers");
                }
                values[array->length + i] = (uint16_t)AS_NUMBER(list->items.values[i]);
            }
            array->length += list->items.count;
        }
    }
    else
    {
        RETURN_ERROR("UInt16Array can only append an uint16 or a list of uint16");
    }
    return bl_value_returnempty(vm, args);
    ;
}

bool modfn_array_uint16get(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(get, 2);
    ENFORCE_ARG_TYPE(get, 0, bl_value_ispointer);
    ENFORCE_ARG_TYPE(get, 1, bl_value_isnumber);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    uint16_t* data = (uint16_t*)array->buffer;
    int index = AS_NUMBER(args[1]);
    if(index < 0 || index >= array->length)
    {
        RETURN_ERROR("UInt16Array index %d out of range", index);
    }
    RETURN_NUMBER((double)data[index]);
}

bool modfn_array_uint16reverse(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(reverse, 1);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    uint16_t* data = (uint16_t*)array->buffer;
    DynArray* narray = bl_object_makedynarrayuint16(vm, array->length);
    uint16_t* ndata = (uint16_t*)narray->buffer;
    for(int i = array->length - 1; i >= 0; i--)
    {
        ndata[i] = data[i];
    }
    RETURN_OBJ(new_array(vm, narray));
}

bool modfn_array_uint16clone(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(clone, 1);
    ENFORCE_ARG_TYPE(clone, 0, bl_value_ispointer);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    DynArray* narray = bl_object_makedynarrayuint16(vm, array->length);
    memcpy(narray->buffer, array->buffer, array->length);
    RETURN_OBJ(new_array(vm, narray));
}

bool modfn_array_uint16pop(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(pop, 1);
    ENFORCE_ARG_TYPE(pop, 0, bl_value_ispointer);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    uint16_t last = ((uint16_t*)array->buffer)[array->length - 1];
    array->length--;
    RETURN_NUMBER(last);
}

bool modfn_array_uint16tolist(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(to_list, 1);
    ENFORCE_ARG_TYPE(to_list, 0, bl_value_ispointer);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    uint16_t* values = (uint16_t*)array->buffer;
    ObjArray* list = (ObjArray*)bl_mem_gcprotect(vm, (Object*)bl_object_makelist(vm));
    for(int i = 0; i < array->length; i++)
    {
        bl_array_push(vm, list, NUMBER_VAL((double)values[i]));
    }
    RETURN_OBJ(list);
}

bool modfn_array_uint16tobytes(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(to_list, 1);
    ENFORCE_ARG_TYPE(to_list, 0, bl_value_ispointer);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    ObjBytes* bytes = (ObjBytes*)bl_mem_gcprotect(vm, (Object*)bl_object_makebytes(vm, array->length * 2));
    memcpy(bytes->bytes.bytes, array->buffer, array->length * 2);
    RETURN_OBJ(bytes);
}

bool modfn_array_uint16iter_(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(@iter, 2);
    ENFORCE_ARG_TYPE(@iter, 0, bl_value_ispointer);
    ENFORCE_ARG_TYPE(@iter, 1, bl_value_isnumber);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    uint16_t* values = (uint16_t*)array->buffer;
    int index = AS_NUMBER(args[1]);
    if(index > -1 && index < array->length)
    {
        RETURN_NUMBER(values[index]);
    }
    return bl_value_returnnil(vm, args);
}

//--------- Unsigned INT 32 STARTS ----------------
DynArray* bl_object_makedynarrayuint32(VMState* vm, int length)
{
    DynArray* array = (DynArray*)bl_object_allocobject(vm, sizeof(DynArray), OBJ_BYTES);
    array->length = length;
    array->buffer = ALLOCATE(uint32_t, length);
    return array;
}

bool modfn_array_uint32array(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(uint32array, 1);
    if(bl_value_isnumber(args[0]))
    {
        RETURN_OBJ(new_array(vm, bl_object_makedynarrayuint32(vm, (int)AS_NUMBER(args[0]))));
    }
    else if(bl_value_isarray(args[0]))
    {
        ObjArray* list = AS_LIST(args[0]);
        DynArray* array = bl_object_makedynarrayuint32(vm, list->items.count);
        uint32_t* values = (uint32_t*)array->buffer;
        for(int i = 0; i < list->items.count; i++)
        {
            if(!bl_value_isnumber(list->items.values[i]))
            {
                RETURN_ERROR("UInt32Array() expects a list of valid uint32");
            }
            values[i] = (uint32_t)AS_NUMBER(list->items.values[i]);
        }
        RETURN_OBJ(new_array(vm, array));
    }
    RETURN_ERROR("expected array size or uint32 list as argument");
}

bool modfn_array_uint32append(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(append, 2);
    ENFORCE_ARG_TYPE(append, 0, bl_value_ispointer);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    if(bl_value_isnumber(args[1]))
    {
        array->length++;
        array->buffer = GROW_ARRAY(uint32_t, sizeof(uint32_t), array->buffer, array->length - 1, array->length);
        uint32_t* values = (uint32_t*)array->buffer;
        values[array->length - 1] = (uint32_t)AS_NUMBER(args[1]);
    }
    else if(bl_value_isarray(args[1]))
    {
        ObjArray* list = AS_LIST(args[1]);
        if(list->items.count > 0)
        {
            array->buffer = GROW_ARRAY(uint32_t, sizeof(uint32_t), array->buffer, array->length, array->length + list->items.count);
            uint32_t* values = (uint32_t*)array->buffer;
            for(int i = 0; i < list->items.count; i++)
            {
                if(!bl_value_isnumber(list->items.values[i]))
                {
                    RETURN_ERROR("UInt32Array lists can only contain numbers");
                }
                values[array->length + i] = (uint32_t)AS_NUMBER(list->items.values[i]);
            }
            array->length += list->items.count;
        }
    }
    else
    {
        RETURN_ERROR("UInt32Array can only append an uint32 or a list of uint32");
    }
    return bl_value_returnempty(vm, args);
    ;
}

bool modfn_array_uint32get(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(get, 2);
    ENFORCE_ARG_TYPE(get, 0, bl_value_ispointer);
    ENFORCE_ARG_TYPE(get, 1, bl_value_isnumber);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    uint32_t* data = (uint32_t*)array->buffer;
    int index = AS_NUMBER(args[1]);
    if(index < 0 || index >= array->length)
    {
        RETURN_ERROR("UInt32Array index %d out of range", index);
    }
    RETURN_NUMBER((double)data[index]);
}

bool modfn_array_uint32reverse(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(reverse, 1);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    uint32_t* data = (uint32_t*)array->buffer;
    DynArray* narray = bl_object_makedynarrayuint32(vm, array->length);
    uint32_t* ndata = (uint32_t*)narray->buffer;
    for(int i = array->length - 1; i >= 0; i--)
    {
        ndata[i] = data[i];
    }
    RETURN_OBJ(new_array(vm, narray));
}

bool modfn_array_uint32clone(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(clone, 1);
    ENFORCE_ARG_TYPE(clone, 0, bl_value_ispointer);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    DynArray* narray = bl_object_makedynarrayuint32(vm, array->length);
    memcpy(narray->buffer, array->buffer, array->length);
    RETURN_OBJ(new_array(vm, narray));
}

bool modfn_array_uint32pop(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(pop, 1);
    ENFORCE_ARG_TYPE(pop, 0, bl_value_ispointer);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    uint32_t last = ((uint32_t*)array->buffer)[array->length - 1];
    array->length--;
    RETURN_NUMBER(last);
}

bool modfn_array_uint32tolist(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(to_list, 1);
    ENFORCE_ARG_TYPE(to_list, 0, bl_value_ispointer);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    uint32_t* values = (uint32_t*)array->buffer;
    ObjArray* list = (ObjArray*)bl_mem_gcprotect(vm, (Object*)bl_object_makelist(vm));
    for(int i = 0; i < array->length; i++)
    {
        bl_array_push(vm, list, NUMBER_VAL((double)values[i]));
    }
    RETURN_OBJ(list);
}

bool modfn_array_uint32tobytes(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(to_list, 1);
    ENFORCE_ARG_TYPE(to_list, 0, bl_value_ispointer);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    ObjBytes* bytes = (ObjBytes*)bl_mem_gcprotect(vm, (Object*)bl_object_makebytes(vm, array->length * 4));
    memcpy(bytes->bytes.bytes, array->buffer, array->length * 4);
    RETURN_OBJ(bytes);
}

bool modfn_array_uint32iter_(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(@iter, 2);
    ENFORCE_ARG_TYPE(@iter, 0, bl_value_ispointer);
    ENFORCE_ARG_TYPE(@iter, 1, bl_value_isnumber);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    uint32_t* values = (uint32_t*)array->buffer;
    int index = AS_NUMBER(args[1]);
    if(index > -1 && index < array->length)
    {
        RETURN_NUMBER(values[index]);
    }
    return bl_value_returnnil(vm, args);
}

//--------- Unsigned INT 64 STARTS ----------------
DynArray* bl_object_makedynarrayuint64(VMState* vm, int length)
{
    DynArray* array = (DynArray*)bl_object_allocobject(vm, sizeof(DynArray), OBJ_BYTES);
    array->length = length;
    array->buffer = ALLOCATE(int64_t, length);
    return array;
}

bool modfn_array_uint64array(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(uint32array, 1);
    if(bl_value_isnumber(args[0]))
    {
        RETURN_OBJ(new_array(vm, bl_object_makedynarrayuint64(vm, (int)AS_NUMBER(args[0]))));
    }
    else if(bl_value_isarray(args[0]))
    {
        ObjArray* list = AS_LIST(args[0]);
        DynArray* array = bl_object_makedynarrayuint64(vm, list->items.count);
        uint64_t* values = (uint64_t*)array->buffer;
        for(int i = 0; i < list->items.count; i++)
        {
            if(!bl_value_isnumber(list->items.values[i]))
            {
                RETURN_ERROR("UInt32Array() expects a list of valid uint64");
            }
            values[i] = (uint64_t)AS_NUMBER(list->items.values[i]);
        }
        RETURN_OBJ(new_array(vm, array));
    }
    RETURN_ERROR("expected array size or uint64 list as argument");
}

bool modfn_array_uint64append(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(append, 2);
    ENFORCE_ARG_TYPE(append, 0, bl_value_ispointer);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    if(bl_value_isnumber(args[1]))
    {
        array->length++;
        array->buffer = GROW_ARRAY(uint64_t, sizeof(uint64_t), array->buffer, array->length - 1, array->length);
        uint64_t* values = (uint64_t*)array->buffer;
        values[array->length - 1] = (uint64_t)AS_NUMBER(args[1]);
    }
    else if(bl_value_isarray(args[1]))
    {
        ObjArray* list = AS_LIST(args[1]);
        if(list->items.count > 0)
        {
            array->buffer = GROW_ARRAY(uint64_t, sizeof(uint64_t), array->buffer, array->length, array->length + list->items.count);
            uint64_t* values = (uint64_t*)array->buffer;
            for(int i = 0; i < list->items.count; i++)
            {
                if(!bl_value_isnumber(list->items.values[i]))
                {
                    RETURN_ERROR("UInt64Array lists can only contain numbers");
                }
                values[array->length + i] = (uint64_t)AS_NUMBER(list->items.values[i]);
            }
            array->length += list->items.count;
        }
    }
    else
    {
        RETURN_ERROR("UInt64Array can only append an uint64 or a list of uint64");
    }
    return bl_value_returnempty(vm, args);
    ;
}

bool modfn_array_uint64get(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(get, 2);
    ENFORCE_ARG_TYPE(get, 0, bl_value_ispointer);
    ENFORCE_ARG_TYPE(get, 1, bl_value_isnumber);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    uint64_t* data = (uint64_t*)array->buffer;
    int index = AS_NUMBER(args[1]);
    if(index < 0 || index >= array->length)
    {
        RETURN_ERROR("UInt64Array index %d out of range", index);
    }
    RETURN_NUMBER((double)data[index]);
}

bool modfn_array_uint64reverse(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(reverse, 1);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    uint64_t* data = (uint64_t*)array->buffer;
    DynArray* narray = bl_object_makedynarrayuint64(vm, array->length);
    uint64_t* ndata = (uint64_t*)narray->buffer;
    for(int i = array->length - 1; i >= 0; i--)
    {
        ndata[i] = data[i];
    }
    RETURN_OBJ(new_array(vm, narray));
}

bool modfn_array_uint64clone(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(clone, 1);
    ENFORCE_ARG_TYPE(clone, 0, bl_value_ispointer);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    DynArray* narray = bl_object_makedynarrayuint64(vm, array->length);
    memcpy(narray->buffer, array->buffer, array->length);
    RETURN_OBJ(new_array(vm, narray));
}

bool modfn_array_uint64pop(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(pop, 1);
    ENFORCE_ARG_TYPE(pop, 0, bl_value_ispointer);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    uint64_t last = ((uint64_t*)array->buffer)[array->length - 1];
    array->length--;
    RETURN_NUMBER(last);
}

bool modfn_array_uint64tolist(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(to_list, 1);
    ENFORCE_ARG_TYPE(to_list, 0, bl_value_ispointer);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    uint64_t* values = (uint64_t*)array->buffer;
    ObjArray* list = (ObjArray*)bl_mem_gcprotect(vm, (Object*)bl_object_makelist(vm));
    for(int i = 0; i < array->length; i++)
    {
        bl_array_push(vm, list, NUMBER_VAL((double)values[i]));
    }
    RETURN_OBJ(list);
}

bool modfn_array_uint64tobytes(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(to_list, 1);
    ENFORCE_ARG_TYPE(to_list, 0, bl_value_ispointer);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    ObjBytes* bytes = (ObjBytes*)bl_mem_gcprotect(vm, (Object*)bl_object_makebytes(vm, array->length * 8));
    memcpy(bytes->bytes.bytes, array->buffer, array->length * 8);
    RETURN_OBJ(bytes);
}

bool modfn_array_uint64iter_(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(@iter, 2);
    ENFORCE_ARG_TYPE(@iter, 0, bl_value_ispointer);
    ENFORCE_ARG_TYPE(@iter, 1, bl_value_isnumber);
    DynArray* array = (DynArray*)AS_PTR(args[0])->pointer;
    uint64_t* values = (uint64_t*)array->buffer;
    int index = AS_NUMBER(args[1]);
    if(index > -1 && index < array->length)
    {
        RETURN_NUMBER(values[index]);
    }
    return bl_value_returnnil(vm, args);
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

RegModule* bl_modload_array(VMState* vm)
{
    (void)vm;
    static RegFunc modulefunctions[] = {
        // int16
        { "Int16Array", false, modfn_array_int16array },
        { "int16append", false, modfn_array_int16append },
        { "int16get", false, modfn_array_int16get },
        { "int16reverse", false, modfn_array_int16reverse },
        { "int16clone", false, modfn_array_int16clone },
        { "int16pop", false, modfn_array_int16pop },
        { "int16tolist", false, modfn_array_int16tolist },
        { "int16tobytes", false, modfn_array_int16tobytes },
        { "int16iter", false, modfn_array_int16iter_ },
        // int32
        { "Int32Array", false, modfn_array_int32array },
        { "int32append", false, modfn_array_int32append },
        { "int32get", false, modfn_array_int32get },
        { "int32reverse", false, modfn_array_int32reverse },
        { "int32clone", false, modfn_array_int32clone },
        { "int32pop", false, modfn_array_int32pop },
        { "int32tolist", false, modfn_array_int32tolist },
        { "int32tobytes", false, modfn_array_int32tobytes },
        { "int32iter", false, modfn_array_int32iter_ },
        // int64
        { "Int64Array", false, modfn_array_int64array },
        { "int64append", false, modfn_array_int64append },
        { "int64get", false, modfn_array_int64get },
        { "int64reverse", false, modfn_array_int64reverse },
        { "int64clone", false, modfn_array_int64clone },
        { "int64pop", false, modfn_array_int64pop },
        { "int64tolist", false, modfn_array_int64tolist },
        { "int64tobytes", false, modfn_array_int64tobytes },
        { "int64iter", false, modfn_array_int64iter_ },
        // uint16
        { "UInt16Array", false, modfn_array_uint16array },
        { "uint16append", false, modfn_array_uint16append },
        { "uint16get", false, modfn_array_uint16get },
        { "uint16reverse", false, modfn_array_uint16reverse },
        { "uint16clone", false, modfn_array_uint16clone },
        { "uint16pop", false, modfn_array_uint16pop },
        { "uint16tolist", false, modfn_array_uint16tolist },
        { "uint16tobytes", false, modfn_array_uint16tobytes },
        { "uint16iter", false, modfn_array_uint16iter_ },
        // uint32
        { "UInt32Array", false, modfn_array_uint32array },
        { "uint32append", false, modfn_array_uint32append },
        { "uint32get", false, modfn_array_uint32get },
        { "uint32reverse", false, modfn_array_uint32reverse },
        { "uint32clone", false, modfn_array_uint32clone },
        { "uint32pop", false, modfn_array_uint32pop },
        { "uint32tolist", false, modfn_array_uint32tolist },
        { "uint32tobytes", false, modfn_array_uint32tobytes },
        { "uint32iter", false, modfn_array_uint32iter_ },
        // uint64
        { "UInt64Array", false, modfn_array_uint64array },
        { "uint64append", false, modfn_array_uint64append },
        { "uint64get", false, modfn_array_uint64get },
        { "uint64reverse", false, modfn_array_uint64reverse },
        { "uint64clone", false, modfn_array_uint64clone },
        { "uint64pop", false, modfn_array_uint64pop },
        { "uint64tolist", false, modfn_array_uint64tolist },
        { "uint64tobytes", false, modfn_array_uint64tobytes },
        { "uint64iter", false, modfn_array_uint64iter_ },
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

#define ADD_TIME(n, l, v) bl_dict_addentry(vm, dict, STRING_L_VAL(n, l), NUMBER_VAL(v))
#define ADD_B_TIME(n, l, v) bl_dict_addentry(vm, dict, STRING_L_VAL(n, l), BOOL_VAL(v))
#define ADD_G_TIME(n, l, v) bl_dict_addentry(vm, dict, STRING_L_VAL(n, l), STRING_L_VAL(v, (int)strlen(v)))
#if defined(_WIN32)
    #define ADD_S_TIME(n, l, v, g) bl_dict_addentry(vm, dict, STRING_L_VAL(n, l), STRING_L_VAL(v, g))
#endif
bool modfn_io_mktime(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_RANGE(mktime, 1, 8);
    if(argcount < 7)
    {
        for(int i = 0; i < argcount; i++)
        {
            ENFORCE_ARG_TYPE(mktime, i, bl_value_isnumber);
        }
    }
    else
    {
        for(int i = 0; i < 6; i++)
        {
            ENFORCE_ARG_TYPE(mktime, i, bl_value_isnumber);
        }
        ENFORCE_ARG_TYPE(mktime, 6, bl_value_isbool);
    }
    int year = -1900;
    int month = 1;
    int day = 1;
    int hour = 0;
    int minute = 0;
    int seconds = 0;
    int isdst = 0;
    year += AS_NUMBER(args[0]);
    if(argcount > 1)
    {
        month = AS_NUMBER(args[1]);
    }
    if(argcount > 2)
    {
        day = AS_NUMBER(args[2]);
    }
    if(argcount > 3)
    {
        hour = AS_NUMBER(args[3]);
    }
    if(argcount > 4)
    {
        minute = AS_NUMBER(args[4]);
    }
    if(argcount > 5)
    {
        seconds = AS_NUMBER(args[5]);
    }
    if(argcount > 6)
    {
        isdst = AS_BOOL(args[5]) ? 1 : 0;
    }
    struct tm t;
    t.tm_year = year;
    t.tm_mon = month - 1;
    t.tm_mday = day;
    t.tm_hour = hour;
    t.tm_min = minute;
    t.tm_sec = seconds;
    t.tm_isdst = isdst;
    RETURN_NUMBER((long)mktime(&t));
}

bool modfn_io_localtime(VMState* vm, int argcount, Value* args)
{
    struct timeval rawtime;
    struct tm now;
    ObjDict* dict;
    (void)argcount;
    gettimeofday(&rawtime, NULL);
    localtime_r(&rawtime.tv_sec, &now);
    dict = (ObjDict*)bl_mem_gcprotect(vm, (Object*)bl_object_makedict(vm));
    ADD_TIME("year", 4, (double)now.tm_year + 1900);
    ADD_TIME("month", 5, (double)now.tm_mon + 1);
    ADD_TIME("day", 3, now.tm_mday);
    ADD_TIME("weekday", 8, now.tm_wday);
    ADD_TIME("yearday", 8, now.tm_yday);
    ADD_TIME("hour", 4, now.tm_hour);
    ADD_TIME("minute", 6, now.tm_min);
    if(now.tm_sec <= 59)
    {
        ADD_TIME("seconds", 7, now.tm_sec);
    }
    else
    {
        ADD_TIME("seconds", 7, 59);
    }
    ADD_TIME("microseconds", 12, (double)rawtime.tv_usec);
    ADD_B_TIME("isdst", 6, now.tm_isdst == 1 ? true : false);
#ifndef _WIN32
    // set time zone
    ADD_G_TIME("zone", 4, now.tm_zone);
    // setting gmt offset
    ADD_TIME("gmtoffset", 10, now.tm_gmtoff);
#else
    // set time zone
    ADD_S_TIME("zone", 4, "", 0);
    // setting gmt offset
    ADD_TIME("gmtoffset", 10, 0);
#endif
    RETURN_OBJ(dict);
}

bool modfn_io_gmtime(VMState* vm, int argcount, Value* args)
{
    struct timeval rawtime;
    struct tm now;
    ObjDict* dict;
    (void)argcount;
    gettimeofday(&rawtime, NULL);
    gmtime_r(&rawtime.tv_sec, &now);
    dict = (ObjDict*)bl_mem_gcprotect(vm, (Object*)bl_object_makedict(vm));
    ADD_TIME("year", 4, (double)now.tm_year + 1900);
    ADD_TIME("month", 5, (double)now.tm_mon + 1);
    ADD_TIME("day", 3, now.tm_mday);
    ADD_TIME("weekday", 8, now.tm_wday);
    ADD_TIME("yearday", 8, now.tm_yday);
    ADD_TIME("hour", 4, now.tm_hour);
    ADD_TIME("minute", 6, now.tm_min);
    if(now.tm_sec <= 59)
    {
        ADD_TIME("seconds", 7, now.tm_sec);
    }
    else
    {
        ADD_TIME("seconds", 7, 59);
    }
    ADD_TIME("microseconds", 12, (double)rawtime.tv_usec);
    ADD_B_TIME("isdst", 6, now.tm_isdst == 1 ? true : false);
#ifndef _WIN32
    // set time zone
    ADD_G_TIME("zone", 4, now.tm_zone);
    // setting gmt offset
    ADD_TIME("gmtoffset", 10, now.tm_gmtoff);
#else
    // set time zone
    ADD_S_TIME("zone", 4, "", 0);
    // setting gmt offset
    ADD_TIME("gmtoffset", 10, 0);
#endif
    RETURN_OBJ(dict);
}

RegModule* bl_modload_date(VMState* vm)
{
    (void)vm;
    static RegFunc modulefunctions[] = {
        { "localtime", true, modfn_io_localtime },
        { "gmtime", true, modfn_io_gmtime },
        { "mktime", false, modfn_io_mktime },
        { NULL, false, NULL },
    };
    static RegModule module = { .name = "_date", .fields = NULL, .functions = modulefunctions, .classes = NULL, .preloader = NULL, .unloader = NULL };
    return &module;
}

#undef ADD_TIME
#undef ADD_B_TIME
#undef ADD_S_TIME
static struct termios origtermios;
static bool setattrwascalled = false;

void disable_raw_mode(void)
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &origtermios);
}
static struct termios nterm;
static struct termios oterm;

static int bl_util_cbreak(int fd)
{
    if((tcgetattr(fd, &oterm)) == -1)
    {
        return -1;
    }
    nterm = oterm;
    nterm.c_lflag = nterm.c_lflag & ~(ECHO | ICANON);
    nterm.c_cc[VMIN] = 1;
    nterm.c_cc[VTIME] = 0;
    if((tcsetattr(fd, TCSAFLUSH, &nterm)) == -1)
    {
        return -1;
    }
    return 1;
}

int bl_util_getch()
{
    int cinput;
    if(bl_util_cbreak(STDIN_FILENO) == -1)
    {
        fprintf(stderr, "cbreak failure, exiting \n");
        exit(EXIT_FAILURE);
    }
    cinput = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oterm);
    return cinput;
}

static int read_line(char line[], int max)
{
    int nch = 0;
    int c;
    max = max - 1;// leave room for '\0'
    while((c = getchar()) != EOF && c != '\0' && c != '\n')
    {
        if(nch < max)
        {
            line[nch] = *bl_util_utf8encode(c);
            nch = nch + 1;
        }
        else
        {
            break;
        }
    }
    if(c == EOF && nch == 0)
    {
        return EOF;
    }
    line[nch] = '\0';
    return nch;
}

/**
 * tty._tcgetattr()
 *
 * returns the configuration of the current tty input
 */
bool modfn_io_ttytcgetattr(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(_tcgetattr, 1);
    ENFORCE_ARG_TYPE(_tcsetattr, 0, bl_value_isfile);
#ifdef HAVE_TERMIOS_H
    ObjFile* file = AS_FILE(args[0]);
    if(!bl_object_isstdfile(file))
    {
        RETURN_ERROR("can only use tty on std objects");
    }
    struct termios rawattr;
    if(tcgetattr(fileno(file->file), &rawattr) != 0)
    {
        RETURN_ERROR(strerror(errno));
    }
    // we have our attributes already
    ObjDict* dict = (ObjDict*)bl_mem_gcprotect(vm, (Object*)bl_object_makedict(vm));
    bl_dict_addentry(vm, dict, NUMBER_VAL(0), NUMBER_VAL(rawattr.c_iflag));
    bl_dict_addentry(vm, dict, NUMBER_VAL(1), NUMBER_VAL(rawattr.c_oflag));
    bl_dict_addentry(vm, dict, NUMBER_VAL(2), NUMBER_VAL(rawattr.c_cflag));
    bl_dict_addentry(vm, dict, NUMBER_VAL(3), NUMBER_VAL(rawattr.c_lflag));
    bl_dict_addentry(vm, dict, NUMBER_VAL(4), NUMBER_VAL(rawattr.c_ispeed));
    bl_dict_addentry(vm, dict, NUMBER_VAL(5), NUMBER_VAL(rawattr.c_ospeed));
    RETURN_OBJ(dict);
#else
    RETURN_ERROR("tcgetattr() is not supported on this platform");
#endif /* HAVE_TERMIOS_H */
}

/**
 * tty._tcsetattr(attrs: dict)
 *
 * sets the attributes of a tty
 * @return true if succeed or false otherwise
 * TODO: support the c_cc flag
 */
bool modfn_io_ttytcsetattr(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(_tcsetattr, 3);
    ENFORCE_ARG_TYPE(_tcsetattr, 0, bl_value_isfile);
    ENFORCE_ARG_TYPE(_tcsetattr, 1, bl_value_isnumber);
    ENFORCE_ARG_TYPE(_tcsetattr, 2, bl_value_isdict);
#ifdef HAVE_TERMIOS_H
    ObjFile* file = AS_FILE(args[0]);
    int type = AS_NUMBER(args[1]);
    ObjDict* dict = AS_DICT(args[2]);
    if(!bl_object_isstdfile(file))
    {
        RETURN_ERROR("can only use tty on std objects");
    }
    if(type < 0)
    {
        RETURN_ERROR("tty options should be one of TTY's TCSA");
    }
    // make sure we have good values so that we don't freeze the tty
    for(int i = 0; i < dict->names.count; i++)
    {
        if(!bl_value_isnumber(dict->names.values[i]) || AS_NUMBER(dict->names.values[i]) < 0 ||// c_iflag
           AS_NUMBER(dict->names.values[i]) > 5)
        {// ospeed
            RETURN_ERROR("attributes must be one of io TTY flags");
        }
        Value dummyvalue;
        if(bl_dict_getentry(dict, dict->names.values[i], &dummyvalue))
        {
            if(!bl_value_isnumber(dummyvalue))
            {
                RETURN_ERROR("TTY attribute cannot be %s", bl_value_typename(dummyvalue));
            }
        }
    }
    Value iflag = NIL_VAL;
    Value oflag = NIL_VAL;
    Value cflag = NIL_VAL;
    Value lflag = NIL_VAL;
    Value ispeed = NIL_VAL;
    Value ospeed = NIL_VAL;
    tcgetattr(STDIN_FILENO, &origtermios);
    atexit(disable_raw_mode);
    struct termios raw = origtermios;
    if(bl_dict_getentry(dict, NUMBER_VAL(0), &iflag))
    {
        raw.c_iflag = (long)AS_NUMBER(iflag);
    }
    if(bl_dict_getentry(dict, NUMBER_VAL(1), &iflag))
    {
        raw.c_oflag = (long)AS_NUMBER(oflag);
    }
    if(bl_dict_getentry(dict, NUMBER_VAL(2), &iflag))
    {
        raw.c_cflag = (long)AS_NUMBER(cflag);
    }
    if(bl_dict_getentry(dict, NUMBER_VAL(3), &iflag))
    {
        raw.c_lflag = (long)AS_NUMBER(lflag);
    }
    if(bl_dict_getentry(dict, NUMBER_VAL(4), &iflag))
    {
        raw.c_ispeed = (long)AS_NUMBER(ispeed);
    }
    if(bl_dict_getentry(dict, NUMBER_VAL(5), &iflag))
    {
        raw.c_ospeed = (long)AS_NUMBER(ospeed);
    }
    setattrwascalled = true;
    int result = tcsetattr(fileno(file->file), type, &raw);
    RETURN_BOOL(result != -1);
#else
    RETURN_ERROR("tcsetattr() is not supported on this platform");
#endif /* HAVE_TERMIOS_H */
}

/**
 * TTY.exit_raw()
 * exits raw mode
 * @return nil
 */
bool modfn_io_ttyexitraw(VMState* vm, int argcount, Value* args)
{
#ifdef HAVE_TERMIOS_H
    ENFORCE_ARG_COUNT(TTY.exit_raw, 0);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &origtermios);
    return bl_value_returnempty(vm, args);
    ;
#else
    RETURN_ERROR("exit_raw() is not supported on this platform");
#endif /* HAVE_TERMIOS_H */
}

/**
 * TTY.flush()
 * flushes the standard output and standard error interface
 * @return nil
 */
bool modfn_io_ttyflush(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(TTY.flush, 0);
    fflush(stdout);
    fflush(stderr);
    return bl_value_returnempty(vm, args);
    ;
}

/**
 * flush()
 * flushes the given file handle
 * @return nil
 */
bool modfn_io_flush(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(flush, 1);
    ENFORCE_ARG_TYPE(flush, 0, bl_value_isfile);
    ObjFile* file = AS_FILE(args[0]);
    if(file->isopen)
    {
        fflush(file->file);
    }
    return bl_value_returnempty(vm, args);
    ;
}

/**
 * getc()
 *
 * reads character(s) from standard input
 *
 * when length is given, gets `length` number of characters
 * else, gets a single character
 * @returns char
 */
bool modfn_io_getc(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_RANGE(getc, 0, 1);
    int length = 1;
    if(argcount == 1)
    {
        ENFORCE_ARG_TYPE(getc, 0, bl_value_isnumber);
        length = AS_NUMBER(args[0]);
    }
    char* result = ALLOCATE(char, (size_t)length + 2);
    read_line(result, length + 1);
    RETURN_L_STRING(result, length);
}

/**
 * bl_util_getch()
 *
 * reads character(s) from standard input without printing to standard output
 *
 * when length is given, gets `length` number of characters
 * else, gets a single character
 * @returns char
 */
bool modfn_io_getch(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(bl_util_getch, 0);
    char* result = ALLOCATE(char, 2);
    result[0] = (char)bl_util_getch();
    result[1] = '\0';
    RETURN_L_STRING(result, 1);
}

/**
 * putc(c: char)
 * writes character c to the screen
 * @return nil
 */
bool modfn_io_putc(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(putc, 1);
    ENFORCE_ARG_TYPE(putc, 0, bl_value_isstring);
    ObjString* string = AS_STRING(args[0]);
    int count = string->length;
#ifdef _WIN32
    if(count > 32767 && isatty(STDIN_FILENO))
    {
        /* Issue #11395: the Windows console returns an error (12: not
       enough space error) on writing into stdout if stdout mode is
       binary and the length is greater than 66,000 bytes (or less,
       depending on heap usage). */
        count = 32767;
    }
#endif
    if(write(STDOUT_FILENO, string->chars, count) != -1)
    {
        fflush(stdout);
    }
    return bl_value_returnempty(vm, args);
    ;
}

/**
 * stdin()
 *
 * returns the standard input
 */
Value io_module_stdin(VMState* vm)
{
    ObjFile* file;
    ObjString* mode;
    ObjString* name;
    name = bl_string_copystring(vm, "<stdin>", 7);
    mode = bl_string_copystring(vm, "rb", 2);
    file = bl_object_makefile(vm, name, mode);
    file->file = stdin;
    file->isopen = true;
    //file->mode = mode;
    return OBJ_VAL(file);
}

/**
 * stdout()
 *
 * returns the standard output interface
 */
Value io_module_stdout(VMState* vm)
{
    ObjFile* file;
    ObjString* mode;
    ObjString* name;
    name = bl_string_copystring(vm, "<stdout>", 8);
    mode = bl_string_copystring(vm, "wb", 2);
    file = bl_object_makefile(vm, name, mode);
    file->file = stdout;
    file->isopen = true;
    return OBJ_VAL(file);
}

/**
 * stderr()
 *
 * returns the standard error interface
 */
Value io_module_stderr(VMState* vm)
{
    ObjFile* file;
    ObjString* mode;
    ObjString* name;
    name = bl_string_copystring(vm, "<stderr>", 8);
    mode = bl_string_copystring(vm, "wb", 2);
    file = bl_object_makefile(vm, name, mode);
    file->file = stderr;
    file->isopen = true;
    return OBJ_VAL(file);
}

void modfn_io_unload(VMState* vm)
{
    (void)vm;
#ifdef HAVE_TERMIOS_H
    if(setattrwascalled)
    {
        disable_raw_mode();
    }
#endif /* ifdef HAVE_TERMIOS_H */
}

RegModule* bl_modload_io(VMState* vm)
{
    (void)vm;
    static RegField iomodulefields[] = {
        { "stdin", false, io_module_stdin },
        { "stdout", false, io_module_stdout },
        { "stderr", false, io_module_stderr },
        { NULL, false, NULL },
    };
    static RegFunc iofunctions[] = {
        { "getc", false, modfn_io_getc },
        { "getch", false, modfn_io_getch },
        { "putc", false, modfn_io_putc },
        { "flush", false, modfn_io_flush },
        { NULL, false, NULL },
    };
    static RegFunc ttyclassfunctions[] = {
        { "tcgetattr", false, modfn_io_ttytcgetattr },
        { "tcsetattr", false, modfn_io_ttytcsetattr },
        { "flush", false, modfn_io_ttyflush },
        { "exit_raw", false, modfn_io_ttyexitraw },
        { NULL, false, NULL },
    };
    static RegClass classes[] = {
        { "TTY", NULL, ttyclassfunctions },
        { NULL, NULL, NULL },
    };
    static RegModule module = { .name = "_io", .fields = iomodulefields, .functions = iofunctions, .classes = classes, .preloader = NULL, .unloader = &modfn_io_unload };
    return &module;
}

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
    return OBJ_VAL(bl_string_copystring(vm, PLATFORM_NAME, (int)strlen(PLATFORM_NAME)));
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
    pid_t pid = fork();
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
    pid_t p;
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
        return bl_vmdo_callvalue(vm, OBJ_VAL(bound), list->items.count);
    }
    return bl_value_returnempty(vm, args);
    ;
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
#define INC_OUTPUTPOS(a, b)                                               \
    if((a) < 0 || ((INT_MAX - outputpos) / ((int)b)) < (a))               \
    {                                                                     \
        free(formatcodes);                                                \
        free(formatargs);                                                 \
        RETURN_ERROR("Type %c: integer overflow in format string", code); \
    }                                                                     \
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

#define ERR_CANT_ASSIGN_EMPTY "empty cannot be assigned."

static void bl_vm_resetstack(VMState* vm)
{
    vm->stacktop = vm->stack;
    vm->framecount = 0;
    vm->openupvalues = NULL;
}

static Value bl_vm_getstacktrace(VMState* vm)
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

static void bl_vm_initexceptions(VMState* vm, ObjModule* module)
{
    size_t slen;
    const char* sstr;
    ObjString* classname;
    sstr = "Exception";
    slen = strlen(sstr);
    //classname = bl_string_copystring(vm, sstr, slen);
    classname = bl_string_fromallocated(vm, strdup(sstr), slen, bl_util_hashstring(sstr, slen));
    bl_vm_pushvalue(vm, OBJ_VAL(classname));
    ObjClass* klass = bl_object_makeclass(vm, classname);
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

void bl_vm_runtimeerror(VMState* vm, const char* format, ...)
{
    fflush(stdout);// flush out anything on stdout first
    CallFrame* frame = &vm->frames[vm->framecount - 1];
    ObjFunction* function = frame->closure->fnptr;
    size_t instruction = frame->ip - function->blob.code - 1;
    int line = function->blob.lines[instruction];
    fprintf(stderr, "RuntimeError: ");
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, " -> %s:%d ", function->module->file, line);
    fputs("\n", stderr);
    if(vm->framecount > 1)
    {
        fprintf(stderr, "StackTrace:\n");
        for(int i = vm->framecount - 1; i >= 0; i--)
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

static void define_native(VMState* vm, const char* name, NativeCallbackFunc function)
{
    bl_vm_pushvalue(vm, STRING_VAL(name));
    bl_vm_pushvalue(vm, OBJ_VAL(bl_object_makenativefunction(vm, function, name)));
    bl_hashtable_set(vm, &vm->globals, vm->stack[0], vm->stack[1]);
    bl_vm_popvaluen(vm, 2);
}

void bl_object_defnativemethod(VMState* vm, HashTable* table, const char* name, NativeCallbackFunc function)
{
    bl_vm_pushvalue(vm, STRING_VAL(name));
    bl_vm_pushvalue(vm, OBJ_VAL(bl_object_makenativefunction(vm, function, name)));
    bl_hashtable_set(vm, table, vm->stack[0], vm->stack[1]);
    bl_vm_popvaluen(vm, 2);
}

static void init_builtin_functions(VMState* vm)
{
    define_native(vm, "abs", cfn_abs);
    define_native(vm, "bin", cfn_bin);
    define_native(vm, "bytes", cfn_bytes);
    define_native(vm, "chr", cfn_chr);
    define_native(vm, "delprop", cfn_delprop);
    define_native(vm, "file", cfn_file);
    define_native(vm, "getprop", cfn_getprop);
    define_native(vm, "hasprop", cfn_hasprop);
    define_native(vm, "hex", cfn_hex);
    define_native(vm, "id", cfn_id);
    define_native(vm, "int", cfn_int);
    define_native(vm, "is_bool", cfn_isbool);
    define_native(vm, "is_callable", cfn_iscallable);
    define_native(vm, "is_class", cfn_isclass);
    define_native(vm, "is_dict", cfn_isdict);
    define_native(vm, "is_function", cfn_isfunction);
    define_native(vm, "is_instance", cfn_isinstance);
    define_native(vm, "is_int", cfn_isint);
    define_native(vm, "is_list", cfn_islist);
    define_native(vm, "is_number", cfn_isnumber);
    define_native(vm, "is_object", cfn_isobject);
    define_native(vm, "is_string", cfn_isstring);
    define_native(vm, "is_bytes", cfn_isbytes);
    define_native(vm, "is_file", cfn_isfile);
    define_native(vm, "is_iterable", cfn_isiterable);
    define_native(vm, "instance_of", cfn_instanceof);
    define_native(vm, "max", cfn_max);
    define_native(vm, "microtime", cfn_microtime);
    define_native(vm, "min", cfn_min);
    define_native(vm, "oct", cfn_oct);
    define_native(vm, "ord", cfn_ord);
    define_native(vm, "print", cfn_print);
    define_native(vm, "rand", cfn_rand);
    define_native(vm, "setprop", cfn_setprop);
    define_native(vm, "sum", cfn_sum);
    define_native(vm, "time", cfn_time);
    define_native(vm, "to_bool", cfn_tobool);
    define_native(vm, "to_dict", cfn_todict);
    define_native(vm, "to_int", cfn_toint);
    define_native(vm, "to_list", cfn_tolist);
    define_native(vm, "to_number", cfn_tonumber);
    define_native(vm, "to_string", cfn_tostring);
    define_native(vm, "typeof", cfn_typeof);
}

static void init_builtin_methods(VMState* vm)
{
    {
        // string methods
        bl_object_defnativemethod(vm, &vm->methodsstring, "length", objfn_string_length);
        bl_object_defnativemethod(vm, &vm->methodsstring, "upper", objfn_string_upper);
        bl_object_defnativemethod(vm, &vm->methodsstring, "lower", objfn_string_lower);
        bl_object_defnativemethod(vm, &vm->methodsstring, "bl_scanutil_isalpha", objfn_string_isalpha);
        bl_object_defnativemethod(vm, &vm->methodsstring, "isalnum", objfn_string_isalnum);
        bl_object_defnativemethod(vm, &vm->methodsstring, "is_number", objfn_string_isnumber);
        bl_object_defnativemethod(vm, &vm->methodsstring, "islower", objfn_string_islower);
        bl_object_defnativemethod(vm, &vm->methodsstring, "isupper", objfn_string_isupper);
        bl_object_defnativemethod(vm, &vm->methodsstring, "isspace", objfn_string_isspace);
        bl_object_defnativemethod(vm, &vm->methodsstring, "trim", objfn_string_trim);
        bl_object_defnativemethod(vm, &vm->methodsstring, "ltrim", objfn_string_ltrim);
        bl_object_defnativemethod(vm, &vm->methodsstring, "rtrim", objfn_string_rtrim);
        bl_object_defnativemethod(vm, &vm->methodsstring, "join", objfn_string_join);
        bl_object_defnativemethod(vm, &vm->methodsstring, "split", objfn_string_split);
        bl_object_defnativemethod(vm, &vm->methodsstring, "indexof", objfn_string_indexof);
        bl_object_defnativemethod(vm, &vm->methodsstring, "startswith", objfn_string_startswith);
        bl_object_defnativemethod(vm, &vm->methodsstring, "endswith", objfn_string_endswith);
        bl_object_defnativemethod(vm, &vm->methodsstring, "count", objfn_string_count);
        bl_object_defnativemethod(vm, &vm->methodsstring, "to_number", objfn_string_tonumber);
        bl_object_defnativemethod(vm, &vm->methodsstring, "to_list", objfn_string_tolist);
        bl_object_defnativemethod(vm, &vm->methodsstring, "tobytes", objfn_string_tobytes);
        bl_object_defnativemethod(vm, &vm->methodsstring, "lpad", objfn_string_lpad);
        bl_object_defnativemethod(vm, &vm->methodsstring, "rpad", objfn_string_rpad);
        bl_object_defnativemethod(vm, &vm->methodsstring, "match", objfn_string_match);
        bl_object_defnativemethod(vm, &vm->methodsstring, "matches", objfn_string_matches);
        bl_object_defnativemethod(vm, &vm->methodsstring, "replace", objfn_string_replace);
        bl_object_defnativemethod(vm, &vm->methodsstring, "ascii", objfn_string_ascii);
        bl_object_defnativemethod(vm, &vm->methodsstring, "@iter", objfn_string_iter);
        bl_object_defnativemethod(vm, &vm->methodsstring, "@itern", objfn_string_itern);
    }
    {
        // list methods
        bl_object_defnativemethod(vm, &vm->methodslist, "length", objfn_list_length);
        bl_object_defnativemethod(vm, &vm->methodslist, "append", objfn_list_append);
        bl_object_defnativemethod(vm, &vm->methodslist, "push", objfn_list_append);
        bl_object_defnativemethod(vm, &vm->methodslist, "clear", objfn_list_clear);
        bl_object_defnativemethod(vm, &vm->methodslist, "clone", objfn_list_clone);
        bl_object_defnativemethod(vm, &vm->methodslist, "count", objfn_list_count);
        bl_object_defnativemethod(vm, &vm->methodslist, "extend", objfn_list_extend);
        bl_object_defnativemethod(vm, &vm->methodslist, "indexof", objfn_list_indexof);
        bl_object_defnativemethod(vm, &vm->methodslist, "insert", objfn_list_insert);
        bl_object_defnativemethod(vm, &vm->methodslist, "pop", objfn_list_pop);
        bl_object_defnativemethod(vm, &vm->methodslist, "shift", objfn_list_shift);
        bl_object_defnativemethod(vm, &vm->methodslist, "remove_at", objfn_list_removeat);
        bl_object_defnativemethod(vm, &vm->methodslist, "remove", objfn_list_remove);
        bl_object_defnativemethod(vm, &vm->methodslist, "reverse", objfn_list_reverse);
        bl_object_defnativemethod(vm, &vm->methodslist, "sort", objfn_list_sort);
        bl_object_defnativemethod(vm, &vm->methodslist, "contains", objfn_list_contains);
        bl_object_defnativemethod(vm, &vm->methodslist, "delete", objfn_list_delete);
        bl_object_defnativemethod(vm, &vm->methodslist, "first", objfn_list_first);
        bl_object_defnativemethod(vm, &vm->methodslist, "last", objfn_list_last);
        bl_object_defnativemethod(vm, &vm->methodslist, "isempty", objfn_list_isempty);
        bl_object_defnativemethod(vm, &vm->methodslist, "take", objfn_list_take);
        bl_object_defnativemethod(vm, &vm->methodslist, "get", objfn_list_get);
        bl_object_defnativemethod(vm, &vm->methodslist, "compact", objfn_list_compact);
        bl_object_defnativemethod(vm, &vm->methodslist, "unique", objfn_list_unique);
        bl_object_defnativemethod(vm, &vm->methodslist, "zip", objfn_list_zip);
        bl_object_defnativemethod(vm, &vm->methodslist, "to_dict", objfn_list_todict);
        bl_object_defnativemethod(vm, &vm->methodslist, "@iter", objfn_list_iter);
        bl_object_defnativemethod(vm, &vm->methodslist, "@itern", objfn_list_itern);
    }
    {
        // dictionary methods
        bl_object_defnativemethod(vm, &vm->methodsdict, "length", objfn_dict_length);
        bl_object_defnativemethod(vm, &vm->methodsdict, "add", objfn_dict_add);
        bl_object_defnativemethod(vm, &vm->methodsdict, "set", objfn_dict_set);
        bl_object_defnativemethod(vm, &vm->methodsdict, "clear", objfn_dict_clear);
        bl_object_defnativemethod(vm, &vm->methodsdict, "clone", objfn_dict_clone);
        bl_object_defnativemethod(vm, &vm->methodsdict, "compact", objfn_dict_compact);
        bl_object_defnativemethod(vm, &vm->methodsdict, "contains", objfn_dict_contains);
        bl_object_defnativemethod(vm, &vm->methodsdict, "extend", objfn_dict_extend);
        bl_object_defnativemethod(vm, &vm->methodsdict, "get", objfn_dict_get);
        bl_object_defnativemethod(vm, &vm->methodsdict, "keys", objfn_dict_keys);
        bl_object_defnativemethod(vm, &vm->methodsdict, "values", objfn_dict_values);
        bl_object_defnativemethod(vm, &vm->methodsdict, "remove", objfn_dict_remove);
        bl_object_defnativemethod(vm, &vm->methodsdict, "isempty", objfn_dict_isempty);
        bl_object_defnativemethod(vm, &vm->methodsdict, "findkey", objfn_dict_findkey);
        bl_object_defnativemethod(vm, &vm->methodsdict, "to_list", objfn_dict_tolist);
        bl_object_defnativemethod(vm, &vm->methodsdict, "@iter", objfn_dict_iter);
        bl_object_defnativemethod(vm, &vm->methodsdict, "@itern", objfn_dict_itern);
    }
    {
        // file methods
        bl_object_defnativemethod(vm, &vm->methodsfile, "exists", objfn_file_exists);
        bl_object_defnativemethod(vm, &vm->methodsfile, "close", objfn_file_close);
        bl_object_defnativemethod(vm, &vm->methodsfile, "open", objfn_file_open);
        bl_object_defnativemethod(vm, &vm->methodsfile, "read", objfn_file_read);
        bl_object_defnativemethod(vm, &vm->methodsfile, "gets", objfn_file_gets);
        bl_object_defnativemethod(vm, &vm->methodsfile, "write", objfn_file_write);
        bl_object_defnativemethod(vm, &vm->methodsfile, "puts", objfn_file_puts);
        bl_object_defnativemethod(vm, &vm->methodsfile, "number", objfn_file_number);
        bl_object_defnativemethod(vm, &vm->methodsfile, "istty", objfn_file_istty);
        bl_object_defnativemethod(vm, &vm->methodsfile, "isopen", objfn_file_isopen);
        bl_object_defnativemethod(vm, &vm->methodsfile, "isclosed", objfn_file_isclosed);
        bl_object_defnativemethod(vm, &vm->methodsfile, "flush", objfn_file_flush);
        bl_object_defnativemethod(vm, &vm->methodsfile, "stats", objfn_file_stats);
        bl_object_defnativemethod(vm, &vm->methodsfile, "symlink", objfn_file_symlink);
        bl_object_defnativemethod(vm, &vm->methodsfile, "delete", objfn_file_delete);
        bl_object_defnativemethod(vm, &vm->methodsfile, "rename", objfn_file_rename);
        bl_object_defnativemethod(vm, &vm->methodsfile, "path", objfn_file_path);
        bl_object_defnativemethod(vm, &vm->methodsfile, "abspath", objfn_file_abspath);
        bl_object_defnativemethod(vm, &vm->methodsfile, "copy", objfn_file_copy);
        bl_object_defnativemethod(vm, &vm->methodsfile, "truncate", objfn_file_truncate);
        bl_object_defnativemethod(vm, &vm->methodsfile, "chmod", objfn_file_chmod);
        bl_object_defnativemethod(vm, &vm->methodsfile, "settimes", objfn_file_settimes);
        bl_object_defnativemethod(vm, &vm->methodsfile, "seek", objfn_file_seek);
        bl_object_defnativemethod(vm, &vm->methodsfile, "tell", objfn_file_tell);
        bl_object_defnativemethod(vm, &vm->methodsfile, "mode", objfn_file_mode);
        bl_object_defnativemethod(vm, &vm->methodsfile, "name", objfn_file_name);
    }
    {
        // bytes
        bl_object_defnativemethod(vm, &vm->methodsbytes, "length", objfn_bytes_length);
        bl_object_defnativemethod(vm, &vm->methodsbytes, "append", objfn_bytes_append);
        bl_object_defnativemethod(vm, &vm->methodsbytes, "clone", objfn_bytes_clone);
        bl_object_defnativemethod(vm, &vm->methodsbytes, "extend", objfn_bytes_extend);
        bl_object_defnativemethod(vm, &vm->methodsbytes, "pop", objfn_bytes_pop);
        bl_object_defnativemethod(vm, &vm->methodsbytes, "remove", objfn_bytes_remove);
        bl_object_defnativemethod(vm, &vm->methodsbytes, "reverse", objfn_bytes_reverse);
        bl_object_defnativemethod(vm, &vm->methodsbytes, "first", objfn_bytes_first);
        bl_object_defnativemethod(vm, &vm->methodsbytes, "last", objfn_bytes_last);
        bl_object_defnativemethod(vm, &vm->methodsbytes, "get", objfn_bytes_get);
        bl_object_defnativemethod(vm, &vm->methodsbytes, "split", objfn_bytes_split);
        bl_object_defnativemethod(vm, &vm->methodsbytes, "dispose", objfn_bytes_dispose);
        bl_object_defnativemethod(vm, &vm->methodsbytes, "bl_scanutil_isalpha", objfn_bytes_isalpha);
        bl_object_defnativemethod(vm, &vm->methodsbytes, "isalnum", objfn_bytes_isalnum);
        bl_object_defnativemethod(vm, &vm->methodsbytes, "is_number", objfn_bytes_isnumber);
        bl_object_defnativemethod(vm, &vm->methodsbytes, "islower", objfn_bytes_islower);
        bl_object_defnativemethod(vm, &vm->methodsbytes, "isupper", objfn_bytes_isupper);
        bl_object_defnativemethod(vm, &vm->methodsbytes, "isspace", objfn_bytes_isspace);
        bl_object_defnativemethod(vm, &vm->methodsbytes, "to_list", objfn_bytes_tolist);
        bl_object_defnativemethod(vm, &vm->methodsbytes, "to_string", objfn_bytes_tostring);
        bl_object_defnativemethod(vm, &vm->methodsbytes, "@iter", objfn_bytes_iter);
        bl_object_defnativemethod(vm, &vm->methodsbytes, "@itern", objfn_bytes_itern);
    }
    {
        // range
        bl_object_defnativemethod(vm, &vm->methodsrange, "lower", objfn_range_lower);
        bl_object_defnativemethod(vm, &vm->methodsrange, "upper", objfn_range_upper);
        bl_object_defnativemethod(vm, &vm->methodsrange, "@iter", objfn_range_iter);
        bl_object_defnativemethod(vm, &vm->methodsrange, "@itern", objfn_range_itern);
    }
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
    bl_hashtable_init(&vm->methodsstring);
    bl_hashtable_init(&vm->methodslist);
    bl_hashtable_init(&vm->methodsdict);
    bl_hashtable_init(&vm->methodsfile);
    bl_hashtable_init(&vm->methodsbytes);
    bl_hashtable_init(&vm->methodsrange);
    init_builtin_functions(vm);
    init_builtin_methods(vm);
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
    bl_hashtable_free(vm, &vm->methodsstring);
    bl_hashtable_free(vm, &vm->methodslist);
    bl_hashtable_free(vm, &vm->methodsdict);
    bl_hashtable_free(vm, &vm->methodsfile);
    bl_hashtable_free(vm, &vm->methodsbytes);
    bl_hashtable_free(vm, &vm->methodsrange);
}

static bool bl_vmdo_docall(VMState* vm, ObjClosure* closure, int argcount)
{
    // fill empty parameters if not variadic
    for(; !closure->fnptr->isvariadic && argcount < closure->fnptr->arity; argcount++)
    {
        bl_vm_pushvalue(vm, NIL_VAL);
    }
    // handle variadic arguments...
    if(closure->fnptr->isvariadic && argcount >= closure->fnptr->arity - 1)
    {
        int vaargsstart = argcount - closure->fnptr->arity;
        ObjArray* argslist = bl_object_makelist(vm);
        bl_vm_pushvalue(vm, OBJ_VAL(argslist));
        for(int i = vaargsstart; i >= 0; i--)
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
    CallFrame* frame = &vm->frames[vm->framecount++];
    frame->closure = closure;
    frame->ip = closure->fnptr->blob.code;
    frame->slots = vm->stacktop - argcount - 1;
    return true;
}

static bool bl_vmdo_callnativemethod(VMState* vm, ObjNativeFunction* native, int argcount)
{
    if(native->natfn(vm, argcount, vm->stacktop - argcount))
    {
        bl_mem_gcclearprotect(vm);
        vm->stacktop -= argcount;
        return true;
    }/* else {
    bl_mem_gcclearprotect(vm);
    bool overridden = AS_BOOL(vm->stacktop[-argcount - 1]);
    *//*if (!overridden) {
      vm->stacktop -= argcount + 1;
    }*//*
    return overridden;
  }*/
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

static FuncType bl_value_getmethodtype(Value method)
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

bool bl_instance_invokefromclass(VMState* vm, ObjClass* klass, ObjString* name, int argcount)
{
    Value method;
    if(bl_hashtable_get(&klass->methods, OBJ_VAL(name), &method))
    {
        if(bl_value_getmethodtype(method) == TYPE_PRIVATE)
        {
            return bl_vm_throwexception(vm, false, "cannot call private method '%s' from instance of %s", name->chars, klass->name->chars);
        }
        return bl_vmdo_callvalue(vm, method, argcount);
    }
    return bl_vm_throwexception(vm, false, "undefined method '%s' in %s", name->chars, klass->name->chars);
}

static bool bl_instance_invokefromself(VMState* vm, ObjString* name, int argcount)
{
    Value receiver = bl_vm_peekvalue(vm, argcount);
    Value value;
    if(bl_value_isinstance(receiver))
    {
        ObjInstance* instance = AS_INSTANCE(receiver);
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
            if(bl_value_getmethodtype(value) == TYPE_STATIC)
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
    Value receiver = bl_vm_peekvalue(vm, argcount);
    Value value;
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
                break;
            }
            case OBJ_CLASS:
            {
                if(bl_hashtable_get(&AS_CLASS(receiver)->methods, OBJ_VAL(name), &value))
                {
                    if(bl_value_getmethodtype(value) == TYPE_PRIVATE)
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
            case OBJ_INSTANCE:
            {
                ObjInstance* instance = AS_INSTANCE(receiver);
                if(bl_hashtable_get(&instance->properties, OBJ_VAL(name), &value))
                {
                    vm->stacktop[-argcount - 1] = value;
                    return bl_vmdo_callvalue(vm, value, argcount);
                }
                return bl_instance_invokefromclass(vm, instance->klass, name, argcount);
            }
            case OBJ_STRING:
            {
                if(bl_hashtable_get(&vm->methodsstring, OBJ_VAL(name), &value))
                {
                    return bl_vmdo_callnativemethod(vm, AS_NATIVE(value), argcount);
                }
                return bl_vm_throwexception(vm, false, "String has no method %s()", name->chars);
            }
            case OBJ_ARRAY:
            {
                if(bl_hashtable_get(&vm->methodslist, OBJ_VAL(name), &value))
                {
                    return bl_vmdo_callnativemethod(vm, AS_NATIVE(value), argcount);
                }
                return bl_vm_throwexception(vm, false, "List has no method %s()", name->chars);
            }
            case OBJ_RANGE:
            {
                if(bl_hashtable_get(&vm->methodsrange, OBJ_VAL(name), &value))
                {
                    return bl_vmdo_callnativemethod(vm, AS_NATIVE(value), argcount);
                }
                return bl_vm_throwexception(vm, false, "Range has no method %s()", name->chars);
            }
            case OBJ_DICT:
            {
                if(bl_hashtable_get(&vm->methodsdict, OBJ_VAL(name), &value))
                {
                    return bl_vmdo_callnativemethod(vm, AS_NATIVE(value), argcount);
                }
                return bl_vm_throwexception(vm, false, "Dict has no method %s()", name->chars);
            }
            case OBJ_FILE:
            {
                if(bl_hashtable_get(&vm->methodsfile, OBJ_VAL(name), &value))
                {
                    return bl_vmdo_callnativemethod(vm, AS_NATIVE(value), argcount);
                }
                return bl_vm_throwexception(vm, false, "File has no method %s()", name->chars);
            }
            case OBJ_BYTES:
            {
                if(bl_hashtable_get(&vm->methodsbytes, OBJ_VAL(name), &value))
                {
                    return bl_vmdo_callnativemethod(vm, AS_NATIVE(value), argcount);
                }
                return bl_vm_throwexception(vm, false, "Bytes has no method %s()", name->chars);
            }
            default:
            {
                return bl_vm_throwexception(vm, false, "cannot call method %s on object of type %s", name->chars, bl_value_typename(receiver));
            }
        }
    }
}

static bool bl_class_bindmethod(VMState* vm, ObjClass* klass, ObjString* name)
{
    Value method;
    if(bl_hashtable_get(&klass->methods, OBJ_VAL(name), &method))
    {
        if(bl_value_getmethodtype(method) == TYPE_PRIVATE)
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

static ObjUpvalue* bl_vm_captureupvalue(VMState* vm, Value* local)
{
    ObjUpvalue* prevupvalue = NULL;
    ObjUpvalue* upvalue = vm->openupvalues;
    while(upvalue != NULL && upvalue->location > local)
    {
        prevupvalue = upvalue;
        upvalue = upvalue->next;
    }
    if(upvalue != NULL && upvalue->location == local)
    {
        return upvalue;
    }
    ObjUpvalue* createdupvalue = bl_object_makeupvalue(vm, local);
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
    while(vm->openupvalues != NULL && vm->openupvalues->location >= last)
    {
        ObjUpvalue* upvalue = vm->openupvalues;
        upvalue->closed = *upvalue->location;
        upvalue->location = &upvalue->closed;
        vm->openupvalues = upvalue->next;
    }
}

static void bl_vm_classdefmethod(VMState* vm, ObjString* name)
{
    Value method = bl_vm_peekvalue(vm, 0);
    ObjClass* klass = AS_CLASS(bl_vm_peekvalue(vm, 1));
    bl_hashtable_set(vm, &klass->methods, OBJ_VAL(name), method);
    if(bl_value_getmethodtype(method) == TYPE_INITIALIZER)
    {
        klass->initializer = method;
    }
    bl_vm_popvalue(vm);
}

static void bl_vm_classdefproperty(VMState* vm, ObjString* name, bool isstatic)
{
    Value property = bl_vm_peekvalue(vm, 0);
    ObjClass* klass = AS_CLASS(bl_vm_peekvalue(vm, 1));
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

static ObjString* bl_vmdo_stringmultiply(VMState* vm, ObjString* str, double number)
{
    int times = (int)number;
    if(times <= 0)
    {// 'str' * 0 == '', 'str' * -1 == ''
        return bl_string_copystring(vm, "", 0);
    }
    else if(times == 1)
    {// 'str' * 1 == 'str'
        return str;
    }
    int totallength = str->length * times;
    char* result = ALLOCATE(char, (size_t)totallength + 1);
    for(int i = 0; i < times; i++)
    {
        memcpy(result + (str->length * i), str->chars, str->length);
    }
    result[totallength] = '\0';
    return bl_string_takestring(vm, result, totallength);
}

static ObjArray* bl_array_addarray(VMState* vm, ObjArray* a, ObjArray* b)
{
    ObjArray* list = bl_object_makelist(vm);
    bl_vm_pushvalue(vm, OBJ_VAL(list));
    for(int i = 0; i < a->items.count; i++)
    {
        bl_valarray_push(vm, &list->items, a->items.values[i]);
    }
    for(int i = 0; i < b->items.count; i++)
    {
        bl_valarray_push(vm, &list->items, b->items.values[i]);
    }
    bl_vm_popvalue(vm);
    return list;
}

static ObjBytes* bl_bytes_addbytes(VMState* vm, ObjBytes* a, ObjBytes* b)
{
    ObjBytes* bytes = bl_object_makebytes(vm, a->bytes.count + b->bytes.count);
    memcpy(bytes->bytes.bytes, a->bytes.bytes, a->bytes.count);
    memcpy(bytes->bytes.bytes + a->bytes.count, b->bytes.bytes, b->bytes.count);
    return bytes;
}

static void bl_vmdo_listmultiply(VMState* vm, ObjArray* a, ObjArray* newlist, int times)
{
    for(int i = 0; i < times; i++)
    {
        for(int j = 0; j < a->items.count; j++)
        {
            bl_valarray_push(vm, &newlist->items, a->items.values[j]);
        }
    }
}

static bool bl_vmdo_modulegetindex(VMState* vm, ObjModule* module, bool willassign)
{
    Value index = bl_vm_peekvalue(vm, 0);
    Value result;
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

static bool bl_vmdo_stringgetindex(VMState* vm, ObjString* string, bool willassign)
{
    Value lower = bl_vm_peekvalue(vm, 0);
    if(!bl_value_isnumber(lower))
    {
        bl_vm_popvaluen(vm, 1);
        return bl_vm_throwexception(vm, false, "strings are numerically indexed");
    }
    int index = AS_NUMBER(lower);
    int length = string->isascii ? string->length : string->utf8length;
    int realindex = index;
    if(index < 0)
    {
        index = length + index;
    }
    if(index < length && index >= 0)
    {
        int start = index;
        int end = index + 1;
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
    else
    {
        bl_vm_popvaluen(vm, 1);
        return bl_vm_throwexception(vm, false, "string index %d out of range", realindex);
    }
}

static bool bl_vmdo_stringgetrangedindex(VMState* vm, ObjString* string, bool willassign)
{
    Value upper = bl_vm_peekvalue(vm, 0);
    Value lower = bl_vm_peekvalue(vm, 1);
    if(!(bl_value_isnil(lower) || bl_value_isnumber(lower)) || !(bl_value_isnumber(upper) || bl_value_isnil(upper)))
    {
        bl_vm_popvaluen(vm, 2);
        return bl_vm_throwexception(vm, false, "string are numerically indexed");
    }
    int length = string->isascii ? string->length : string->utf8length;
    int lowerindex = bl_value_isnumber(lower) ? AS_NUMBER(lower) : 0;
    int upperindex = bl_value_isnil(upper) ? length : AS_NUMBER(upper);
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
    int start = lowerindex;
    int end = upperindex;
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

static bool bl_vmdo_bytesgetindex(VMState* vm, ObjBytes* bytes, bool willassign)
{
    Value lower = bl_vm_peekvalue(vm, 0);
    if(!bl_value_isnumber(lower))
    {
        bl_vm_popvaluen(vm, 1);
        return bl_vm_throwexception(vm, false, "bytes are numerically indexed");
    }
    int index = AS_NUMBER(lower);
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

static bool bl_vmdo_bytesgetrangedindex(VMState* vm, ObjBytes* bytes, bool willassign)
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

static bool bl_vmdo_listgetindex(VMState* vm, ObjArray* list, bool willassign)
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

static bool bl_vmdo_listgetrangedindex(VMState* vm, ObjArray* list, bool willassign)
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

static void bl_vmdo_modulesetindex(VMState* vm, ObjModule* module, Value index, Value value)
{
    bl_hashtable_set(vm, &module->values, index, value);
    bl_vm_popvaluen(vm, 3);// pop the value, index and dict out
    // leave the value on the stack for consumption
    // e.g. variable = dict[index] = 10
    bl_vm_pushvalue(vm, value);
}

static bool bl_vmdo_listsetindex(VMState* vm, ObjArray* list, Value index, Value value)
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

static bool bl_vmdo_bytessetindex(VMState* vm, ObjBytes* bytes, Value index, Value value)
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

static bool bl_vmdo_concat(VMState* vm)
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

#define BINARY_OP(vm, frame, type, op)                                                                                                                        \
    do                                                                                                                                                        \
    {                                                                                                                                                         \
        if((!bl_value_isnumber(bl_vm_peekvalue(vm, 0)) && !bl_value_isbool(bl_vm_peekvalue(vm, 0)))                                                           \
           || (!bl_value_isnumber(bl_vm_peekvalue(vm, 1)) && !bl_value_isbool(bl_vm_peekvalue(vm, 1))))                                                       \
        {                                                                                                                                                     \
            runtime_error("unsupported operand %s for %s and %s", #op, bl_value_typename(bl_vm_peekvalue(vm, 0)), bl_value_typename(bl_vm_peekvalue(vm, 1))); \
            break;                                                                                                                                            \
        }                                                                                                                                                     \
        Value _b = bl_vm_popvalue(vm);                                                                                                                        \
        double b = bl_value_isbool(_b) ? (AS_BOOL(_b) ? 1 : 0) : AS_NUMBER(_b);                                                                               \
        Value _a = bl_vm_popvalue(vm);                                                                                                                        \
        double a = bl_value_isbool(_a) ? (AS_BOOL(_a) ? 1 : 0) : AS_NUMBER(_a);                                                                               \
        bl_vm_pushvalue(vm, type(a op b));                                                                                                                    \
    } while(false)

#define BINARY_BIT_OP(vm, frame, op)                                                                                                                          \
    do                                                                                                                                                        \
    {                                                                                                                                                         \
        if((!bl_value_isnumber(bl_vm_peekvalue(vm, 0)) && !bl_value_isbool(bl_vm_peekvalue(vm, 0)))                                                           \
           || (!bl_value_isnumber(bl_vm_peekvalue(vm, 1)) && !bl_value_isbool(bl_vm_peekvalue(vm, 1))))                                                       \
        {                                                                                                                                                     \
            runtime_error("unsupported operand %s for %s and %s", #op, bl_value_typename(bl_vm_peekvalue(vm, 0)), bl_value_typename(bl_vm_peekvalue(vm, 1))); \
            break;                                                                                                                                            \
        }                                                                                                                                                     \
        long b = AS_NUMBER(bl_vm_popvalue(vm));                                                                                                               \
        long a = AS_NUMBER(bl_vm_popvalue(vm));                                                                                                               \
        bl_vm_pushvalue(vm, INTEGER_VAL(a op b));                                                                                                             \
    } while(false)

#define BINARY_MOD_OP(type, op)                                                                                                                               \
    do                                                                                                                                                        \
    {                                                                                                                                                         \
        if((!bl_value_isnumber(bl_vm_peekvalue(vm, 0)) && !bl_value_isbool(bl_vm_peekvalue(vm, 0)))                                                           \
           || (!bl_value_isnumber(bl_vm_peekvalue(vm, 1)) && !bl_value_isbool(bl_vm_peekvalue(vm, 1))))                                                       \
        {                                                                                                                                                     \
            runtime_error("unsupported operand %s for %s and %s", #op, bl_value_typename(bl_vm_peekvalue(vm, 0)), bl_value_typename(bl_vm_peekvalue(vm, 1))); \
            break;                                                                                                                                            \
        }                                                                                                                                                     \
        Value _b = bl_vm_popvalue(vm);                                                                                                                        \
        double b = bl_value_isbool(_b) ? (AS_BOOL(_b) ? 1 : 0) : AS_NUMBER(_b);                                                                               \
        Value _a = bl_vm_popvalue(vm);                                                                                                                        \
        double a = bl_value_isbool(_a) ? (AS_BOOL(_a) ? 1 : 0) : AS_NUMBER(_a);                                                                               \
        bl_vm_pushvalue(vm, type(op(a, b)));                                                                                                                  \
    } while(false)

PtrResult bl_vm_run(VMState* vm)
{
    CallFrame* frame = &vm->frames[vm->framecount - 1];
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
        uint8_t instruction;
        switch(instruction = READ_BYTE(frame))
        {
            case OP_CONSTANT:
            {
                Value constant = READ_CONSTANT(frame);
                bl_vm_pushvalue(vm, constant);
                break;
            }
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
                    BINARY_OP(vm, frame, NUMBER_VAL, +);
                }
                break;
            }
            case OP_SUBTRACT:
            {
                BINARY_OP(vm, frame, NUMBER_VAL, -);
                break;
            }
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
                BINARY_OP(vm, frame, NUMBER_VAL, *);
                break;
            }
            case OP_DIVIDE:
            {
                BINARY_OP(vm, frame, NUMBER_VAL, /);
                break;
            }
            case OP_REMINDER:
            {
                BINARY_MOD_OP(NUMBER_VAL, bl_util_modulo);
                break;
            }
            case OP_POW:
            {
                BINARY_MOD_OP(NUMBER_VAL, pow);
                break;
            }
            case OP_F_DIVIDE:
            {
                BINARY_MOD_OP(NUMBER_VAL, bl_util_floordiv);
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
            case OP_AND:
            {
                BINARY_BIT_OP(vm, frame, &);
                break;
            }
            case OP_OR:
            {
                BINARY_BIT_OP(vm, frame, |);
                break;
            }
            case OP_XOR:
            {
                BINARY_BIT_OP(vm, frame, ^);
                break;
            }
            case OP_LSHIFT:
            {
                BINARY_BIT_OP(vm, frame, <<);
                break;
            }
            case OP_RSHIFT:
            {
                BINARY_BIT_OP(vm, frame, >>);
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
            case OP_GREATER:
            {
                BINARY_OP(vm, frame, BOOL_VAL, >);
                break;
            }
            case OP_LESS:
            {
                BINARY_OP(vm, frame, BOOL_VAL, <);
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
                    runtime_error(ERR_CANT_ASSIGN_EMPTY);
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
                    runtime_error(ERR_CANT_ASSIGN_EMPTY);
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
                    runtime_error(ERR_CANT_ASSIGN_EMPTY);
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
                                if(bl_value_getmethodtype(value) == TYPE_STATIC)
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
                            if(bl_class_bindmethod(vm, instance->klass, name))
                            {
                                break;
                            }
                            runtime_error("instance of class %s does not have a property or method named '%s'",
                                          AS_INSTANCE(bl_vm_peekvalue(vm, 0))->klass->name->chars, name->chars);
                            break;
                        }
                        case OBJ_STRING:
                        {
                            if(bl_hashtable_get(&vm->methodsstring, OBJ_VAL(name), &value))
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
                            if(bl_hashtable_get(&vm->methodslist, OBJ_VAL(name), &value))
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
                            if(bl_hashtable_get(&vm->methodsrange, OBJ_VAL(name), &value))
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
                            if(bl_hashtable_get(&AS_DICT(bl_vm_peekvalue(vm, 0))->items, OBJ_VAL(name), &value) || bl_hashtable_get(&vm->methodsdict, OBJ_VAL(name), &value))
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
                            if(bl_hashtable_get(&vm->methodsbytes, OBJ_VAL(name), &value))
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
                            if(bl_hashtable_get(&vm->methodsfile, OBJ_VAL(name), &value))
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
                    if(bl_class_bindmethod(vm, instance->klass, name))
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
                        if(bl_value_getmethodtype(value) == TYPE_STATIC)
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
                    runtime_error(ERR_CANT_ASSIGN_EMPTY);
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
                    runtime_error(ERR_CANT_ASSIGN_EMPTY);
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
                bl_vm_pushvalue(vm, OBJ_VAL(bl_object_makeclass(vm, name)));
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
                if(!bl_class_bindmethod(vm, klass->superclass, name))
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
                if(!bl_instance_invokefromclass(vm, klass, method, argcount))
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
                if(!bl_instance_invokefromclass(vm, klass, klass->name, argcount))
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
                        runtime_error(ERR_CANT_ASSIGN_EMPTY);
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

#undef BINARY_OP
#undef BINARY_MOD_OP
}

PtrResult bl_vm_interpsource(VMState* vm, ObjModule* module, const char* source)
{
    BinaryBlob blob;
    bl_blob_init(&blob);
    vm->allowgc = false;
    bl_vm_initexceptions(vm, module);
    vm->allowgc = true;
    ObjFunction* function = bl_compiler_compilesource(vm, module, source, &blob);
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
    ObjClosure* closure = bl_object_makeclosure(vm, function);
    bl_vm_popvalue(vm);
    bl_vm_pushvalue(vm, OBJ_VAL(closure));
    bl_vmdo_docall(vm, closure, 0);
    PtrResult result = bl_vm_run(vm);
    return result;
}

#undef ERR_CANT_ASSIGN_EMPTY
static bool continuerepl = true;

static void repl(VMState* vm)
{
    vm->isrepl = true;
    printf("Blade %s (running on BladeVM %s), REPL/Interactive mode = ON\n", BLADE_VERSION_STRING, BVM_VERSION);
    printf("%s, (Build time = %s, %s)\n", COMPILER, __DATE__, __TIME__);
    printf("Type \".exit\" to quit or \".credits\" for more information\n");
    char* source = (char*)malloc(sizeof(char));
    memset(source, 0, sizeof(char));
    int bracecount = 0;
    int parencount = 0;
    int bracketcount = 0;
    int singlequotecount = 0;
    int doublequotecount = 0;
    ObjModule* module = bl_object_makemodule(vm, strdup(""), strdup("<repl>"));
    bl_state_addmodule(vm, module);
    for(;;)
    {
        if(!continuerepl)
        {
            bracecount = 0;
            parencount = 0;
            bracketcount = 0;
            singlequotecount = 0;
            doublequotecount = 0;
            // reset source...
            memset(source, 0, strlen(source));
            continuerepl = true;
        }
        const char* cursor = "%> ";
        if(bracecount > 0 || bracketcount > 0 || parencount > 0)
        {
            cursor = ".. ";
        }
        else if(singlequotecount == 1 || doublequotecount == 1)
        {
            cursor = "";
        }
        char buffer[1024];
        printf("%s", cursor);
        char* line = fgets(buffer, 1024, stdin);
        int linelength = 0;
        if(line != NULL)
        {
            linelength = strcspn(line, "\r\n");
            line[linelength] = 0;
        }
        // terminate early if we receive a terminating command such as exit() or Ctrl+D
        if(line == NULL || strcmp(line, ".exit") == 0)
        {
            free(source);
            return;
        }
        if(strcmp(line, ".credits") == 0)
        {
            printf("\n" BLADE_COPYRIGHT "\n\n");
            memset(source, 0, sizeof(char));
            continue;
        }
        if(linelength > 0 && line[0] == '#')
        {
            continue;
        }
        // find count of { and }, ( and ), [ and ], " and '
        for(int i = 0; i < linelength; i++)
        {
            // scope openers...
            if(line[i] == '{')
            {
                bracecount++;
            }
            if(line[i] == '(')
            {
                parencount++;
            }
            if(line[i] == '[')
            {
                bracketcount++;
            }
            // quotes
            if(line[i] == '\'' && doublequotecount == 0)
            {
                if(singlequotecount == 0)
                {
                    singlequotecount++;
                }
                else
                {
                    singlequotecount--;
                }
            }
            if(line[i] == '"' && singlequotecount == 0)
            {
                if(doublequotecount == 0)
                {
                    doublequotecount++;
                }
                else
                {
                    doublequotecount--;
                }
            }
            if(line[i] == '\\' && (singlequotecount > 0 || doublequotecount > 0))
            {
                i++;
            }
            // scope closers...
            if(line[i] == '}' && bracecount > 0)
            {
                bracecount--;
            }
            if(line[i] == ')' && parencount > 0)
            {
                parencount--;
            }
            if(line[i] == ']' && bracketcount > 0)
            {
                bracketcount--;
            }
        }
        source = bl_util_appendstring(source, line);
        if(linelength > 0)
        {
            source = bl_util_appendstring(source, "\n");
        }
        if(bracketcount == 0 && parencount == 0 && bracecount == 0 && singlequotecount == 0 && doublequotecount == 0)
        {
            bl_vm_interpsource(vm, module, source);
            fflush(stdout);// flush all outputs
            // reset source...
            memset(source, 0, strlen(source));
        }
    }
    free(source);
}

static void run_source(VMState* vm, const char* source, const char* filename)
{
    ObjModule* module = bl_object_makemodule(vm, strdup(""), strdup(filename));
    bl_state_addmodule(vm, module);
    PtrResult result = bl_vm_interpsource(vm, module, source);
    fflush(stdout);
    if(result == PTR_COMPILE_ERR)
    {
        exit(EXIT_COMPILE);
    }
    if(result == PTR_RUNTIME_ERR)
    {
        exit(EXIT_RUNTIME);
    }
}

static void run_file(VMState* vm, char* file)
{
    char* source = bl_util_readfile(file);
    if(source == NULL)
    {
        // check if it's a Blade library directory by attempting to read the index file.
        char* oldfile = file;
        file = bl_util_appendstring((char*)strdup(file), "/" LIBRARY_DIRECTORY_INDEX BLADE_EXTENSION);
        source = bl_util_readfile(file);
        if(source == NULL)
        {
            fprintf(stderr, "(Blade):\n  Launch aborted for %s\n  Reason: %s\n", oldfile, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
    run_source(vm, source, file);
    free(source);
}

void show_usage(char* argv[], bool fail)
{
    FILE* out = fail ? stderr : stdout;
    fprintf(out, "Usage: %s [-[h | d | j | v | g]] [filename]\n", argv[0]);
    fprintf(out, "   -h    Show this help message.\n");
    fprintf(out, "   -v    Show version string.\n");
    fprintf(out, "   -b    Buffer terminal outputs.\n");
    fprintf(out, "         [This will cause the output to be buffered with 1kb]\n");
    fprintf(out, "   -d    Show generated bytecode.\n");
    fprintf(out, "   -j    Show stack objects during execution.\n");
    fprintf(out, "   -e<s> eval <s>\n");
    fprintf(out,
            "   -g    Sets the minimum heap size in kilobytes before the GC\n"
            "         can start. [Default = %d (%dmb)]\n",
            DEFAULT_GC_START / 1024, DEFAULT_GC_START / (1024 * 1024));
    exit(fail ? EXIT_FAILURE : EXIT_SUCCESS);
}

int main(int argc, char* argv[])
{
    bool shoulddebugstack;
    bool shouldbufferstdout;
    bool shouldprintbytecode;
    int i;
    int opt;
    int next;
    int nextgcstart;
    char** stdargs;
    const char* codeline;
    VMState* vm;
    vm = (VMState*)malloc(sizeof(VMState));
    if(vm == NULL)
    {
        fprintf(stderr, "failed to allocate vm.\n");
        return 1;
    }
    memset(vm, 0, sizeof(VMState));
    init_vm(vm);
    fprintf(stderr, "STACK_MAX = %d\n", STACK_MAX);
    shoulddebugstack = false;
    shouldprintbytecode = false;
    shouldbufferstdout = false;
    nextgcstart = DEFAULT_GC_START;
    codeline = NULL;
    if(argc > 1)
    {
        while((opt = getopt(argc, argv, "hdbjvg:e:")) != -1)
        {
            switch(opt)
            {
                case 'h':
                {
                    show_usage(argv, false);// exits
                }
                break;
                case 'd':
                {
                    shouldprintbytecode = true;
                }
                break;
                case 'b':
                {
                    shouldbufferstdout = true;
                }
                break;
                case 'j':
                {
                    shoulddebugstack = true;
                }
                break;
                case 'v':
                {
                    printf("Blade " BLADE_VERSION_STRING " (running on BladeVM " BVM_VERSION ")\n");
                    return EXIT_SUCCESS;
                }
                break;
                case 'g':
                {
                    next = (int)strtol(optarg, NULL, 10);
                    if(next > 0)
                    {
                        nextgcstart = next * 1024;// expected value is in kilobytes
                    }
                }
                break;
                case 'e':
                {
                    codeline = optarg;
                }
                break;
                default:
                {
                    show_usage(argv, true);// exits
                }
                break;
            }
        }
    }
    if(vm != NULL)
    {
        // set vm options...
        vm->shoulddebugstack = shoulddebugstack;
        vm->shouldprintbytecode = shouldprintbytecode;
        vm->nextgc = nextgcstart;
        if(shouldbufferstdout)
        {
            // forcing printf buffering for TTYs and terminals
            if(isatty(fileno(stdout)))
            {
                char buffer[1024];
                setvbuf(stdout, buffer, _IOFBF, sizeof(buffer));
            }
        }
#ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);
#endif
        stdargs = (char**)calloc(argc, sizeof(char*));
        if(stdargs != NULL)
        {
            for(i = 0; i < argc; i++)
            {
                stdargs[i] = argv[i];
            }
            vm->stdargs = stdargs;
            vm->stdargscount = argc;
        }
        // always do this last so that we can have access to everything else
        bind_native_modules(vm);
        if(codeline != NULL)
        {
            run_source(vm, codeline, "<-e>");
        }
        else if(argc == 1 || argc <= optind)
        {
            repl(vm);
        }
        else
        {
            run_file(vm, argv[optind]);
        }
        fprintf(stderr, "freeing up memory?\n");
        bl_mem_collectgarbage(vm);
        free(stdargs);
        bl_vm_freevm(vm);
        free(vm);
        return EXIT_SUCCESS;
    }
    fprintf(stderr, "Device out of memory.");
    exit(EXIT_FAILURE);
}
