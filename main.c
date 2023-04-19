
//#include "../bgetopt.h"

#pragma once


#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <math.h>
#include <sys/stat.h>

#include <dlfcn.h>
#include <unistd.h>
#include <dirent.h>
#include <termios.h>
#include <sys/mman.h>
#include <utime.h>
#include <sys/utsname.h>
#include <sys/param.h>

#define PCRE2_STATIC
#define PCRE2_CODE_UNIT_WIDTH 8

#include <pcre2.h>


// special definitions for Cygwin
#define _DEFAULT_SOURCE 1
#define _GNU_SOURCE 1

#define BLADE_EXTENSION ".b"
#define BLADE_VERSION_STRING "0.0.74-rc1"
#define BVM_VERSION "0.0.7"
#define LIBRARY_DIRECTORY "libs"
#define LIBRARY_DIRECTORY_INDEX "index"
#define PACKAGES_DIRECTORY "vendor"
#define LOCAL_PACKAGES_DIRECTORY ".blade"
#define LOCAL_EXT_DIRECTORY "/bin"
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

#define BLADE_PACKAGE_ROOT_ENV "BLADE_PKG_ROOT"

#define HAVE_GETOPT_H
#define HAVE_TERMIOS_H
#define HAVE_UNISTD_H 1
#define HAVE_SYS_TIME_H
#define HAVE_SYS_UTSNAME_H
#define HAVE_SYS_ERRNO_H
#define HAVE_DIRENT_H
#define HAVE_DLFCN_H

#define HAVE_ASPRINTF
#define HAVE_STRSEP
#define HAVE_GETTIMEOFDAY
#define HAVE_UTIME
#define HAVE_BASENAME
#define HAVE_DIRNAME
#define HAVE_SYSCONF
#define HAVE_SYSCTLBYNAME
#define HAVE_SETJMP

#ifndef BYTE_ORDER
    #if(BSD >= 199103) || defined(__MACH__) || defined(__APPLE__)

        #include <machine/endian.h>

    #elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
        #include <sys/endian.h>
    #elif defined(linux) || defined(__linux__)
        #include <endian.h>
    #else
        #define LITTLE_ENDIAN 1234 /* least-significant byte first (vax, pc) */
        #define BIG_ENDIAN 4321 /* most-significant byte first (IBM, net) */
        #define PDP_ENDIAN 3412 /* LSB first in word, MSW first in long (pdp)*/
        /* msvc for intel processors */
        /* msvc code on arm executes in little endian mode */
        #if defined(vax) || defined(ns32000) || defined(sun386) || defined(__i386__) || defined(MIPSEL) || defined(_MIPSEL) || defined(BIT_ZERO_ON_RIGHT) \
        || defined(__alpha__) || defined(__alpha) || defined(_M_IX86) || defined(_M_X64) || defined(_M_IA64) || defined(_M_ARM) || defined(_WIN32)
            #define BYTE_ORDER LITTLE_ENDIAN
        #endif

        #if defined(sel) || defined(pyr) || defined(mc68000) || defined(sparc) || defined(is68k) || defined(tahoe) || defined(ibm032) || defined(ibm370) || defined(MIPSEB) || defined(_MIPSEB) || defined(_IBMR2) || defined(DGUX) || defined(apollo) || defined(__convex__) || defined(_CRAY) || defined(__hppa) || defined(__hp9000) || defined(__hp9000s300) || defined(__hp9000s700) || defined(BIT_ZERO_ON_LEFT) || defined(m68k) || defined(__sparc) || defined(_M_PPC)
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
#define UINT16_COUNT (UINT16_MAX + 1)
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

#define DEFAULT_GC_START (1024*1024)

#define EXIT_COMPILE 10
#define EXIT_RUNTIME 11
#define EXIT_TERMINAL 12

#define BLADE_COPYRIGHT "Copyright (c) 2021 Ore Richard Muyiwa"



// NOTE: METHOD_OBJECT must always be retrieved
// before any call to create an object in a native function.
// failure to do so will lead to the first object created
// within the function to appear as METHOD_OBJECT
#define METHOD_OBJECT args[-1]

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

#define RETURN \
    { \
        args[-1] = EMPTY_VAL; \
        return true; \
    }
#define RETURN_NIL \
    { \
        args[-1] = NIL_VAL; \
        return true; \
    }
#define RETURN_EMPTY \
    { \
        args[-1] = NIL_VAL; \
        return false; \
    }
#define RETURN_ERROR(...) \
    { \
        pop_n(vm, arg_count); \
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
#define RETURN_PTR(v) \
    { \
        args[-1] = OBJ_VAL(new_ptr(vm, (void*)v)); \
        return true; \
    }
#define RETURN_STRING(v) \
    { \
        args[-1] = OBJ_VAL(copy_string(vm, v, (int)strlen(v))); \
        return true; \
    }
#define RETURN_L_STRING(v, l) \
    { \
        args[-1] = OBJ_VAL(copy_string(vm, v, l)); \
        return true; \
    }
#define RETURN_T_STRING(v, l) \
    { \
        args[-1] = OBJ_VAL(take_string(vm, v, l)); \
        return true; \
    }
#define RETURN_TT_STRING(v) \
    { \
        args[-1] = OBJ_VAL(take_string(vm, v, (int)strlen(v))); \
        return true; \
    }
#define RETURN_VALUE(v) \
    { \
        args[-1] = v; \
        return true; \
    }

#define ENFORCE_ARG_COUNT(name, d) \
    if(arg_count != d) \
    { \
        RETURN_ERROR(#name "() expects %d arguments, %d given", d, arg_count); \
    }

#define ENFORCE_MIN_ARG(name, d) \
    if(arg_count < d) \
    { \
        RETURN_ERROR(#name "() expects minimum of %d arguments, %d given", d, arg_count); \
    }

#define ENFORCE_MAX_ARG(name, d) \
    if(arg_count < d) \
    { \
        RETURN_ERROR(#name "() expects maximum of %d arguments, %d given", d, arg_count); \
    }

#define ENFORCE_ARG_RANGE(name, low, up) \
    if(arg_count < (low) || arg_count > (up)) \
    { \
        RETURN_ERROR(#name "() expects between %d and %d arguments, %d given", low, up, arg_count); \
    }

#define ENFORCE_ARG_TYPE(name, i, type) \
    if(!type(args[i])) \
    { \
        RETURN_ERROR(#name "() expects argument %d as " NORMALIZE(type) ", %s given", (i) + 1, value_type(args[i])); \
    }

#define ENFORCE_CONSTRUCTOR_ARG_TYPE(name, i, type) \
    if(!type(args[i])) \
    { \
        RETURN_ERROR(#name "() expects argument %d to class constructor as " NORMALIZE(type) ", %s given", (i) + 1, value_type(args[i])); \
    }

#define EXCLUDE_ARG_TYPE(method_name, arg_type, index) \
    if(arg_type(args[index])) \
    { \
        RETURN_ERROR("invalid type %s() as argument %d in %s()", value_type(args[index]), (index) + 1, #method_name); \
    }

#define METHOD_OVERRIDE(override, i) \
    do \
    { \
        if(IS_INSTANCE(args[0])) \
        { \
            b_obj_instance* instance = AS_INSTANCE(args[0]); \
            if(invoke_from_class(vm, instance->klass, copy_string(vm, "@" #override, (i) + 1), 0)) \
            { \
                args[-1] = TRUE_VAL; \
                return false; \
            } \
        } \
    } while(0);

#define REGEX_COMPILATION_ERROR(re, error_number, error_offset) \
    if((re) == NULL) \
    { \
        PCRE2_UCHAR8 buffer[256]; \
        pcre2_get_error_message_8(error_number, buffer, sizeof(buffer)); \
        RETURN_ERROR("regular expression compilation failed at offset %d: %s", (int)(error_offset), buffer); \
    }

#define REGEX_ASSERTION_ERROR(re, match_data, ovector) \
    if((ovector)[0] > (ovector)[1]) \
    { \
        RETURN_ERROR("match aborted: regular expression used \\K in an assertion %.*s to " \
                     "set match start after its end.", \
                     (int)((ovector)[0] - (ovector)[1]), (char*)(subject + (ovector)[1])); \
        pcre2_match_data_free(match_data); \
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

#define GET_REGEX_COMPILE_OPTIONS(string, regex_show_error) \
    uint32_t compile_options = is_regex(string); \
    if((regex_show_error) && (int)compile_options == -1) \
    { \
        RETURN_ERROR("RegexError: Invalid regex"); \
    } \
    else if((regex_show_error) && (int)compile_options > 1000000) \
    { \
        RETURN_ERROR("RegexError: invalid modifier '%c' ", (char)abs(1000000 - (int)compile_options)); \
    }

#define GC_STRING(o) OBJ_VAL(GC(copy_string(vm, (o), (int)strlen(o))))
#define GC_L_STRING(o, l) OBJ_VAL(GC(copy_string(vm, (o), (l))))
#define GC_T_STRING(o, l) OBJ_VAL(GC(take_string(vm, (o), (l))))
#define GC_TT_STRING(o) OBJ_VAL(GC(take_string(vm, (o), (int)strlen(o))))




typedef enum b_val_type b_val_type;
typedef struct b_value b_value;
typedef struct b_compiler b_compiler;
typedef struct b_obj b_obj;
typedef struct b_obj_string b_obj_string;
typedef struct b_vm b_vm;
typedef struct b_module_reg b_module_reg;


typedef b_value (*b_class_field)(b_vm*);
typedef void (*b_module_loader)(b_vm*);
typedef b_module_reg* (*b_module_init)(b_vm*);
typedef bool (*b_native_fn)(b_vm*, int, b_value*);
typedef void (*b_ptr_free_fn)(void*);

enum b_val_type
{
    VAL_NIL,
    VAL_BOOL,
    VAL_NUMBER,
    VAL_OBJ,
    VAL_EMPTY,
};

struct b_value
{
    b_val_type type;

    union
    {
        bool boolean;
        double number;
        b_obj* obj;
    } as;
};


typedef union b_double_union b_double_union;
union b_double_union
{
    uint64_t bits;
    double num;
};

typedef struct b_func_reg b_func_reg;
struct b_func_reg
{
    const char* name;
    bool is_static;
    b_native_fn natfn;
};

typedef struct b_field_reg b_field_reg;
struct b_field_reg
{
    const char* name;
    bool is_static;
    b_class_field field_value;
};

typedef struct b_class_reg b_class_reg;
struct b_class_reg
{
    const char* name;
    b_field_reg* fields;
    b_func_reg* functions;
};

struct b_module_reg
{
    const char* name;
    b_field_reg* fields;
    b_func_reg* functions;
    b_class_reg* classes;
    b_module_loader preloader;
    b_module_loader unloader;
};

typedef struct
{
    int capacity;
    int count;
    b_value* values;
} b_value_arr;

typedef struct
{
    int count;
    unsigned char* bytes;
} b_byte_arr;

typedef enum
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
} b_code;

typedef struct b_blob b_blob;
struct b_blob
{
    int count;
    int capacity;
    uint8_t* code;
    int* lines;
    b_value_arr constants;
};

typedef struct b_entry b_entry;
struct b_entry
{
    b_value key;
    b_value value;
};

typedef struct b_table b_table;
struct b_table
{
    int count;
    int capacity;
    b_entry* entries;
};

typedef enum b_func_type b_func_type;
enum b_func_type
{
    TYPE_FUNCTION,
    TYPE_METHOD,
    TYPE_INITIALIZER,
    TYPE_PRIVATE,
    TYPE_STATIC,
    TYPE_SCRIPT,
};

typedef enum b_obj_type b_obj_type;
enum b_obj_type
{
    // containers
    OBJ_STRING,
    OBJ_RANGE,
    OBJ_LIST,
    OBJ_DICT,
    OBJ_FILE,
    OBJ_BYTES,

    // base object types
    OBJ_UP_VALUE,
    OBJ_BOUND_METHOD,
    OBJ_CLOSURE,
    OBJ_FUNCTION,
    OBJ_INSTANCE,
    OBJ_NATIVE,
    OBJ_CLASS,

    // non-user objects
    OBJ_MODULE,
    OBJ_SWITCH,
    OBJ_PTR,// object type that can hold any C pointer
};

struct b_obj
{
    b_obj_type type;
    bool mark;
    bool definitelyreal;
    b_obj* sibling;
};

struct b_obj_string
{
    b_obj obj;
    int length;
    int utf8_length;
    bool is_ascii;
    uint32_t hash;
    char* chars;
};

typedef struct b_obj_up_value b_obj_up_value;
struct b_obj_up_value
{
    b_obj obj;
    b_value closed;
    b_value* location;
    b_obj_up_value* next;
};

typedef struct b_obj_module b_obj_module;
struct b_obj_module
{
    b_obj obj;
    bool imported;
    b_table values;
    char* name;
    char* file;
    void* preloader;
    void* unloader;
    void* handle;
};

typedef struct b_obj_func b_obj_func;
struct b_obj_func
{
    b_obj obj;
    b_func_type type;
    int arity;
    int up_value_count;
    bool is_variadic;
    b_blob blob;
    b_obj_string* name;
    b_obj_module* module;
};

typedef struct b_obj_closure b_obj_closure;
struct b_obj_closure
{
    b_obj obj;
    int up_value_count;
    b_obj_func* fnptr;
    b_obj_up_value** up_values;
};

typedef struct b_obj_class b_obj_class;
struct b_obj_class
{
    b_obj obj;
    b_value initializer;
    b_table properties;
    b_table static_properties;
    b_table methods;
    b_obj_string* name;
    struct b_obj_class* superclass;
};

typedef struct b_obj_instance b_obj_instance;
struct b_obj_instance
{
    b_obj obj;
    b_table properties;
    b_obj_class* klass;
};

typedef struct b_obj_bound b_obj_bound;
struct b_obj_bound
{
    b_obj obj;
    b_value receiver;
    b_obj_closure* method;
};

typedef struct b_obj_native b_obj_native;
struct b_obj_native
{
    b_obj obj;
    b_func_type type;
    const char* name;
    b_native_fn natfn;
};

typedef struct
{
    b_obj obj;
    b_value_arr items;
} b_obj_list;

typedef struct
{
    b_obj obj;
    int lower;
    int upper;
    int range;
} b_obj_range;

typedef struct
{
    b_obj obj;
    b_byte_arr bytes;
} b_obj_bytes;

typedef struct
{
    b_obj obj;
    b_value_arr names;
    b_table items;
} b_obj_dict;

typedef struct
{
    b_obj obj;
    bool is_open;
    FILE* file;
    b_obj_string* mode;
    b_obj_string* path;
} b_obj_file;

typedef struct
{
    b_obj obj;
    int default_jump;
    int exit_jump;
    b_table table;
} b_obj_switch;

typedef struct
{
    b_obj obj;
    void* pointer;
    char* name;
    b_ptr_free_fn free_fn;
} b_obj_ptr;

typedef enum
{
    PTR_OK,
    PTR_COMPILE_ERR,
    PTR_RUNTIME_ERR,
} b_ptr_result;

typedef struct
{
    uint16_t address;
    uint16_t finally_address;
    b_obj_class* klass;
} b_exception_frame;

typedef struct
{
    b_obj_closure* closure;
    uint8_t* ip;
    b_value* slots;
    int handlers_count;
    b_exception_frame handlers[MAX_EXCEPTION_HANDLERS];
} b_call_frame;

struct b_vm
{
    bool allowgc;
    b_call_frame frames[FRAMES_MAX];
    int frame_count;

    b_blob* blob;
    uint8_t* ip;
    b_value stack[STACK_MAX];
    b_value* stack_top;
    b_obj_up_value* open_up_values;

    size_t objectcount;
    b_obj* objectlinks;
    b_compiler* compiler;
    b_obj_class* exception_class;

    // gc
    int gray_count;
    int gray_capacity;
    int gc_protected;
    b_obj** gray_stack;
    size_t bytes_allocated;
    size_t next_gc;

    // objects tracker
    b_table modules;
    b_table strings;
    b_table globals;

    // object public methods
    b_table methods_string;
    b_table methods_list;
    b_table methods_dict;
    b_table methods_file;
    b_table methods_bytes;
    b_table methods_range;

    char** std_args;
    int std_args_count;

    // boolean flags
    bool is_repl;
    // for switching through the command line args...
    bool should_debug_stack;
    bool should_print_bytecode;
};

typedef enum
{
    // symbols
    NEWLINE_TOKEN,// \n
    LPAREN_TOKEN,// (
    RPAREN_TOKEN,// )
    LBRACKET_TOKEN,// [
    RBRACKET_TOKEN,// ]
    LBRACE_TOKEN,// {
    RBRACE_TOKEN,// }
    SEMICOLON_TOKEN,// ;
    COMMA_TOKEN,// ,
    BACKSLASH_TOKEN,// '\'
    BANG_TOKEN,// !
    BANG_EQ_TOKEN,// !=
    COLON_TOKEN,// :
    AT_TOKEN,// @
    DOT_TOKEN,// .
    RANGE_TOKEN,// ..
    TRI_DOT_TOKEN,// ...
    PLUS_TOKEN,// +
    PLUS_EQ_TOKEN,// +=
    INCREMENT_TOKEN,// ++
    MINUS_TOKEN,// -
    MINUS_EQ_TOKEN,// -=
    DECREMENT_TOKEN,// --
    MULTIPLY_TOKEN,// *
    MULTIPLY_EQ_TOKEN,// *=
    POW_TOKEN,// **
    POW_EQ_TOKEN,// **=
    DIVIDE_TOKEN,// '/'
    DIVIDE_EQ_TOKEN,// '/='
    FLOOR_TOKEN,// '//'
    FLOOR_EQ_TOKEN,// '//='
    EQUAL_TOKEN,// =
    EQUAL_EQ_TOKEN,// ==
    LESS_TOKEN,// <
    LESS_EQ_TOKEN,// <=
    LSHIFT_TOKEN,// <<
    LSHIFT_EQ_TOKEN,// <<=
    GREATER_TOKEN,// >
    GREATER_EQ_TOKEN,// >=
    RSHIFT_TOKEN,// >>
    RSHIFT_EQ_TOKEN,// >>=
    PERCENT_TOKEN,// %
    PERCENT_EQ_TOKEN,// %=
    AMP_TOKEN,// &
    AMP_EQ_TOKEN,// &=
    BAR_TOKEN,// |
    BAR_EQ_TOKEN,// |=
    TILDE_TOKEN,// ~
    TILDE_EQ_TOKEN,// ~=
    XOR_TOKEN,// ^
    XOR_EQ_TOKEN,// ^=
    QUESTION_TOKEN,// ??

    // keywords
    AND_TOKEN,
    AS_TOKEN,
    ASSERT_TOKEN,
    BREAK_TOKEN,
    CATCH_TOKEN,
    CLASS_TOKEN,
    CONTINUE_TOKEN,
    DEF_TOKEN,
    DEFAULT_TOKEN,
    DIE_TOKEN,
    DO_TOKEN,
    ECHO_TOKEN,
    ELSE_TOKEN,
    FALSE_TOKEN,
    FINALLY_TOKEN,
    FOR_TOKEN,
    IF_TOKEN,
    IMPORT_TOKEN,
    IN_TOKEN,
    ITER_TOKEN,
    NIL_TOKEN,
    OR_TOKEN,
    PARENT_TOKEN,
    RETURN_TOKEN,
    SELF_TOKEN,
    STATIC_TOKEN,
    TRUE_TOKEN,
    TRY_TOKEN,
    USING_TOKEN,
    VAR_TOKEN,
    WHEN_TOKEN,
    WHILE_TOKEN,

    // types token
    LITERAL_TOKEN,
    REG_NUMBER_TOKEN,// regular numbers (inclusive of doubles)
    BIN_NUMBER_TOKEN,// binary numbers
    OCT_NUMBER_TOKEN,// octal numbers
    HEX_NUMBER_TOKEN,// hexadecimal numbers
    IDENTIFIER_TOKEN,
    DECORATOR_TOKEN,
    INTERPOLATION_TOKEN,
    EOF_TOKEN,

    // error
    ERROR_TOKEN,
    EMPTY_TOKEN,
    UNDEFINED_TOKEN,
} b_tkn_type;

typedef struct
{
    b_tkn_type type;
    const char* start;
    int length;
    int line;
} b_token;

typedef struct
{
    const char* start;
    const char* current;
    int line;
    int interpolating_count;
    int interpolating[MAX_INTERPOLATION_NESTING];
} b_scanner;



typedef enum
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
} b_precedence;

typedef struct
{
    b_token name;
    int depth;
    bool is_captured;
} b_local;

typedef struct
{
    uint16_t index;
    bool is_local;
} b_up_value;

struct b_compiler
{
    b_compiler* enclosing;

    // current function
    b_obj_func* currfunc;
    b_func_type type;

    b_local locals[UINT8_COUNT];
    int local_count;
    b_up_value up_values[UINT8_COUNT];
    int scope_depth;
    int handler_count;
};

typedef struct b_class_compiler
{
    struct b_class_compiler* enclosing;
    b_token name;
    bool has_superclass;
} b_class_compiler;

typedef struct
{
    b_scanner* scanner;
    b_vm* vm;

    b_token current;
    b_token previous;
    bool had_error;
    bool panic_mode;
    int block_count;
    bool is_returning;
    bool is_trying;
    bool repl_can_echo;
    b_class_compiler* current_class;
    const char* current_file;

    // used for tracking loops for the continue statement...
    int innermost_loop_start;
    int innermost_loop_scope_depth;
    b_obj_module* module;
} b_parser;

typedef void (*b_parse_prefix_fn)(b_parser*, bool);

typedef void (*b_parse_infix_fn)(b_parser*, b_token, bool);

typedef struct
{
    b_parse_prefix_fn prefix;
    b_parse_infix_fn infix;
    b_precedence precedence;
} b_parse_rule;

typedef struct
{
    void* buffer;
    int length;
} b_array;

typedef struct
{
    pid_t pid;
} BProcess;

typedef struct
{
    char* format;
    char* get_format;
    unsigned char* bytes;
    int format_length;
    int get_format_length;
    int length;
    bool locked;
} BProcessShared;

// promote C values to blade value
#define EMPTY_VAL ((b_value){ VAL_EMPTY, { .number = 0 } })
#define NIL_VAL ((b_value){ VAL_NIL, { .number = 0 } })
#define TRUE_VAL ((b_value){ VAL_BOOL, { .boolean = true } })
#define FALSE_VAL ((b_value){ VAL_BOOL, { .boolean = false } })
#define BOOL_VAL(v) ((b_value){ VAL_BOOL, { .boolean = v } })
#define NUMBER_VAL(v) ((b_value){ VAL_NUMBER, { .number = v } })
#define INTEGER_VAL(v) ((b_value){ VAL_NUMBER, { .number = v } })
#define LONG_VAL(v) ((b_value){ VAL_NUMBER, { .number = v } })
#define OBJ_VAL(v) ((b_value){ VAL_OBJ, { .obj = (b_obj*)v } })

// demote blade values to C value
#define AS_BOOL(v) ((v).as.boolean)
#define AS_NUMBER(v) ((v).as.number)
#define AS_OBJ(v) ((v).as.obj)

// testing blade value types
#define IS_NIL(v) ((v).type == VAL_NIL)
#define IS_BOOL(v) ((v).type == VAL_BOOL)
#define IS_NUMBER(v) ((v).type == VAL_NUMBER)
#define IS_OBJ(v) ((v).type == VAL_OBJ)
#define IS_EMPTY(v) ((v).type == VAL_EMPTY)


#define GROW_CAPACITY(capacity) ((capacity) < 4 ? 4 : (capacity)*2)

#define GROW_ARRAY(type, pointer, old_count, new_count) (type*)reallocate(vm, pointer, sizeof(type) * (old_count), sizeof(type) * (new_count))

#define FREE_ARRAY(type, pointer, old_count) bl_mem_free(vm, pointer, sizeof(type) * (old_count))

#define FREE(type, pointer) bl_mem_free(vm, pointer, sizeof(type))

#define ALLOCATE(type, count) (type*)reallocate(vm, NULL, 0, sizeof(type) * (count))

#define N_ALLOCATE(type, count) (type*)allocate(vm, (count))

#define STRING_VAL(val) OBJ_VAL(copy_string(vm, val, (int)strlen(val)))
#define STRING_L_VAL(val, l) OBJ_VAL(copy_string(vm, val, l))
#define STRING_T_VAL(val, l) OBJ_VAL(take_string(vm, val, l))
#define STRING_TT_VAL(val) OBJ_VAL(take_string(vm, val, (int)strlen(val)))
#define BYTES_VAL(val) OBJ_VAL(take_bytes(vm, (unsigned char*)(val), (int)strlen((char*)(val))))
#define PTR_VAL(val) OBJ_VAL(new_ptr(vm, val))



#define OBJ_TYPE(v) (AS_OBJ(v)->type)

// object type checks
#define IS_STRING(v) is_obj_type(v, OBJ_STRING)
#define IS_NATIVE(v) is_obj_type(v, OBJ_NATIVE)
#define IS_FUNCTION(v) is_obj_type(v, OBJ_FUNCTION)
#define IS_CLOSURE(v) is_obj_type(v, OBJ_CLOSURE)
#define IS_CLASS(v) is_obj_type(v, OBJ_CLASS)
#define IS_INSTANCE(v) is_obj_type(v, OBJ_INSTANCE)
#define IS_BOUND(v) is_obj_type(v, OBJ_BOUND_METHOD)

// containers
#define IS_BYTES(v) is_obj_type(v, OBJ_BYTES)
#define IS_LIST(v) is_obj_type(v, OBJ_LIST)
#define IS_DICT(v) is_obj_type(v, OBJ_DICT)
#define IS_FILE(v) is_obj_type(v, OBJ_FILE)
#define IS_RANGE(v) is_obj_type(v, OBJ_RANGE)

// promote b_value to object
#define AS_STRING(v) ((b_obj_string*)AS_OBJ(v))
#define AS_NATIVE(v) ((b_obj_native*)AS_OBJ(v))
#define AS_FUNCTION(v) ((b_obj_func*)AS_OBJ(v))
#define AS_CLOSURE(v) ((b_obj_closure*)AS_OBJ(v))
#define AS_CLASS(v) ((b_obj_class*)AS_OBJ(v))
#define AS_INSTANCE(v) ((b_obj_instance*)AS_OBJ(v))
#define AS_BOUND(v) ((b_obj_bound*)AS_OBJ(v))

// non-user objects
#define AS_SWITCH(v) ((b_obj_switch*)AS_OBJ(v))
#define IS_SWITCH(v) is_obj_type(v, OBJ_SWITCH)
#define AS_PTR(v) ((b_obj_ptr*)AS_OBJ(v))
#define IS_PTR(v) is_obj_type(v, OBJ_PTR)
#define AS_MODULE(v) ((b_obj_module*)AS_OBJ(v))
#define IS_MODULE(v) is_obj_type(v, OBJ_MODULE)

// containers
#define AS_BYTES(v) ((b_obj_bytes*)AS_OBJ(v))
#define AS_LIST(v) ((b_obj_list*)AS_OBJ(v))
#define AS_DICT(v) ((b_obj_dict*)AS_OBJ(v))
#define AS_FILE(v) ((b_obj_file*)AS_OBJ(v))
#define AS_RANGE(v) ((b_obj_range*)AS_OBJ(v))

// demote blade value to c string
#define AS_C_STRING(v) (((b_obj_string*)AS_OBJ(v))->chars)

#define IS_CHAR(v) (IS_STRING(v) && (AS_STRING(v)->length == 1 || AS_STRING(v)->length == 0))

#define EXIT_VM() return PTR_RUNTIME_ERR

#define runtime_error(...) \
    if(!bl_vm_throwexception(vm, false, ##__VA_ARGS__)) \
    { \
        EXIT_VM(); \
    }


// NOTE:
// any call to GC() within a function/block must accompanied by
// at least one call to CLEAR_GC() before exiting the function/block
// otherwise, expected unexpected behavior
// NOTE as well that the call to CLEAR_GC() will be automatic for
// native functions.
// NOTE as well that METHOD_OBJECT must be retrieved before any call
// to GC() in a native function.
#define GC(o) gc_protect(vm, (b_obj*)(o))
#define CLEAR_GC() gc_clear_protection(vm)

/* main.c */
int utf8_number_bytes(int value);
char *utf8_encode(unsigned int code);
int utf8_decode_num_bytes(uint8_t byte);
int utf8_decode(const uint8_t *bytes, uint32_t length);
char *append_strings(char *old, char *new_str);
int utf8len(char *s);
char *utf8index(char *s, int pos);
void utf8slice(char *s, int *start, int *end);
char *read_file(const char *path);
char *get_exe_path(void);
char *get_exe_dir(void);
char *merge_paths(char *a, char *b);
bool file_exists(char *filepath);
char *get_blade_filename(char *filename);
char *resolve_import_path(char *module_name, const char *current_file, bool is_relative);
char *get_real_file_name(char *path);
void *allocate(b_vm *vm, size_t size);
void *reallocate(b_vm *vm, void *pointer, size_t old_size, size_t new_size);
void mark_object(b_vm *vm, b_obj *object);
void mark_value(b_vm *vm, b_value value);
void blacken_object(b_vm *vm, b_obj *object);
void free_object(b_vm *vm, b_obj**object);
void free_objects(b_vm *vm);
void collect_garbage(b_vm *vm);
void init_blob(b_blob *blob);
void write_blob(b_vm *vm, b_blob *blob, uint8_t byte, int line);
void free_blob(b_vm *vm, b_blob *blob);
int add_constant(b_vm *vm, b_blob *blob, b_value value);
uint32_t is_regex(b_obj_string *string);
char *remove_regex_delimiter(b_vm *vm, b_obj_string *string);
bool native_method_stringlength(b_vm *vm, int arg_count, b_value *args);
bool native_method_stringupper(b_vm *vm, int arg_count, b_value *args);
bool native_method_stringlower(b_vm *vm, int arg_count, b_value *args);
bool native_method_stringis_alpha(b_vm *vm, int arg_count, b_value *args);
bool native_method_stringis_alnum(b_vm *vm, int arg_count, b_value *args);
bool native_method_stringis_number(b_vm *vm, int arg_count, b_value *args);
bool native_method_stringis_lower(b_vm *vm, int arg_count, b_value *args);
bool native_method_stringis_upper(b_vm *vm, int arg_count, b_value *args);
bool native_method_stringis_space(b_vm *vm, int arg_count, b_value *args);
bool native_method_stringtrim(b_vm *vm, int arg_count, b_value *args);
bool native_method_stringltrim(b_vm *vm, int arg_count, b_value *args);
bool native_method_stringrtrim(b_vm *vm, int arg_count, b_value *args);
bool native_method_stringjoin(b_vm *vm, int arg_count, b_value *args);
bool native_method_stringsplit(b_vm *vm, int arg_count, b_value *args);
bool native_method_stringindex_of(b_vm *vm, int arg_count, b_value *args);
bool native_method_stringstarts_with(b_vm *vm, int arg_count, b_value *args);
bool native_method_stringends_with(b_vm *vm, int arg_count, b_value *args);
bool native_method_stringcount(b_vm *vm, int arg_count, b_value *args);
bool native_method_stringto_number(b_vm *vm, int arg_count, b_value *args);
bool native_method_stringascii(b_vm *vm, int arg_count, b_value *args);
bool native_method_stringto_list(b_vm *vm, int arg_count, b_value *args);
bool native_method_stringlpad(b_vm *vm, int arg_count, b_value *args);
bool native_method_stringrpad(b_vm *vm, int arg_count, b_value *args);
bool native_method_stringmatch(b_vm *vm, int arg_count, b_value *args);
bool native_method_stringmatches(b_vm *vm, int arg_count, b_value *args);
bool native_method_stringreplace(b_vm *vm, int arg_count, b_value *args);
bool native_method_stringto_bytes(b_vm *vm, int arg_count, b_value *args);
bool native_method_string__iter__(b_vm *vm, int arg_count, b_value *args);
bool native_method_string__itern__(b_vm *vm, int arg_count, b_value *args);
bool cfn_bytes(b_vm *vm, int arg_count, b_value *args);
bool native_method_byteslength(b_vm *vm, int arg_count, b_value *args);
bool native_method_bytesappend(b_vm *vm, int arg_count, b_value *args);
bool native_method_bytesclone(b_vm *vm, int arg_count, b_value *args);
bool native_method_bytesextend(b_vm *vm, int arg_count, b_value *args);
bool native_method_bytespop(b_vm *vm, int arg_count, b_value *args);
bool native_method_bytesremove(b_vm *vm, int arg_count, b_value *args);
bool native_method_bytesreverse(b_vm *vm, int arg_count, b_value *args);
bool native_method_bytessplit(b_vm *vm, int arg_count, b_value *args);
bool native_method_bytesfirst(b_vm *vm, int arg_count, b_value *args);
bool native_method_byteslast(b_vm *vm, int arg_count, b_value *args);
bool native_method_bytesget(b_vm *vm, int arg_count, b_value *args);
bool native_method_bytesis_alpha(b_vm *vm, int arg_count, b_value *args);
bool native_method_bytesis_alnum(b_vm *vm, int arg_count, b_value *args);
bool native_method_bytesis_number(b_vm *vm, int arg_count, b_value *args);
bool native_method_bytesis_lower(b_vm *vm, int arg_count, b_value *args);
bool native_method_bytesis_upper(b_vm *vm, int arg_count, b_value *args);
bool native_method_bytesis_space(b_vm *vm, int arg_count, b_value *args);
bool native_method_bytesdispose(b_vm *vm, int arg_count, b_value *args);
bool native_method_bytesto_list(b_vm *vm, int arg_count, b_value *args);
bool native_method_bytesto_string(b_vm *vm, int arg_count, b_value *args);
bool native_method_bytes__iter__(b_vm *vm, int arg_count, b_value *args);
bool native_method_bytes__itern__(b_vm *vm, int arg_count, b_value *args);
void init_table(b_table *table);
void free_table(b_vm *vm, b_table *table);
void clean_free_table(b_vm *vm, b_table *table);
bool table_get(b_table *table, b_value key, b_value *value);
bool table_set(b_vm *vm, b_table *table, b_value key, b_value value);
bool table_delete(b_table *table, b_value key);
void table_add_all(b_vm *vm, b_table *from, b_table *to);
void table_copy(b_vm *vm, b_table *from, b_table *to);
b_obj_string *table_find_string(b_table *table, const char *chars, int length, uint32_t hash);
b_value table_find_key(b_table *table, b_value value);
void table_print(b_table *table);
void mark_table(b_vm *vm, b_table *table);
void table_remove_whites(b_vm *vm, b_table *table);
void dict_add_entry(b_vm *vm, b_obj_dict *dict, b_value key, b_value value);
bool dict_get_entry(b_obj_dict *dict, b_value key, b_value *value);
bool dict_set_entry(b_vm *vm, b_obj_dict *dict, b_value key, b_value value);
bool bl_vmdo_dictgetindex(b_vm *vm, b_obj_dict *dict, bool will_assign);
void bl_vmdo_dictsetindex(b_vm *vm, b_obj_dict *dict, b_value index, b_value value);
bool native_method_dictlength(b_vm *vm, int arg_count, b_value *args);
bool native_method_dictadd(b_vm *vm, int arg_count, b_value *args);
bool native_method_dictset(b_vm *vm, int arg_count, b_value *args);
bool native_method_dictclear(b_vm *vm, int arg_count, b_value *args);
bool native_method_dictclone(b_vm *vm, int arg_count, b_value *args);
bool native_method_dictcompact(b_vm *vm, int arg_count, b_value *args);
bool native_method_dictcontains(b_vm *vm, int arg_count, b_value *args);
bool native_method_dictextend(b_vm *vm, int arg_count, b_value *args);
bool native_method_dictget(b_vm *vm, int arg_count, b_value *args);
bool native_method_dictkeys(b_vm *vm, int arg_count, b_value *args);
bool native_method_dictvalues(b_vm *vm, int arg_count, b_value *args);
bool native_method_dictremove(b_vm *vm, int arg_count, b_value *args);
bool native_method_dictis_empty(b_vm *vm, int arg_count, b_value *args);
bool native_method_dictfind_key(b_vm *vm, int arg_count, b_value *args);
bool native_method_dictto_list(b_vm *vm, int arg_count, b_value *args);
bool native_method_dict__iter__(b_vm *vm, int arg_count, b_value *args);
bool native_method_dict__itern__(b_vm *vm, int arg_count, b_value *args);
void write_list(b_vm *vm, b_obj_list *list, b_value value);
b_obj_list *copy_list(b_vm *vm, b_obj_list *list, int start, int length);
bool native_method_listlength(b_vm *vm, int arg_count, b_value *args);
bool native_method_listappend(b_vm *vm, int arg_count, b_value *args);
bool native_method_listclear(b_vm *vm, int arg_count, b_value *args);
bool native_method_listclone(b_vm *vm, int arg_count, b_value *args);
bool native_method_listcount(b_vm *vm, int arg_count, b_value *args);
bool native_method_listextend(b_vm *vm, int arg_count, b_value *args);
bool native_method_listindex_of(b_vm *vm, int arg_count, b_value *args);
bool native_method_listinsert(b_vm *vm, int arg_count, b_value *args);
bool native_method_listpop(b_vm *vm, int arg_count, b_value *args);
bool native_method_listshift(b_vm *vm, int arg_count, b_value *args);
bool native_method_listremove_at(b_vm *vm, int arg_count, b_value *args);
bool native_method_listremove(b_vm *vm, int arg_count, b_value *args);
bool native_method_listreverse(b_vm *vm, int arg_count, b_value *args);
bool native_method_listsort(b_vm *vm, int arg_count, b_value *args);
bool native_method_listcontains(b_vm *vm, int arg_count, b_value *args);
bool native_method_listdelete(b_vm *vm, int arg_count, b_value *args);
bool native_method_listfirst(b_vm *vm, int arg_count, b_value *args);
bool native_method_listlast(b_vm *vm, int arg_count, b_value *args);
bool native_method_listis_empty(b_vm *vm, int arg_count, b_value *args);
bool native_method_listtake(b_vm *vm, int arg_count, b_value *args);
bool native_method_listget(b_vm *vm, int arg_count, b_value *args);
bool native_method_listcompact(b_vm *vm, int arg_count, b_value *args);
bool native_method_listunique(b_vm *vm, int arg_count, b_value *args);
bool native_method_listzip(b_vm *vm, int arg_count, b_value *args);
bool native_method_listto_dict(b_vm *vm, int arg_count, b_value *args);
bool native_method_list__iter__(b_vm *vm, int arg_count, b_value *args);
bool native_method_list__itern__(b_vm *vm, int arg_count, b_value *args);
void init_value_arr(b_value_arr *array);
void init_byte_arr(b_vm *vm, b_byte_arr *array, int length);
void write_value_arr(b_vm *vm, b_value_arr *array, b_value value);
void insert_value_arr(b_vm *vm, b_value_arr *array, b_value value, int index);
void free_value_arr(b_vm *vm, b_value_arr *array);
void free_byte_arr(b_vm *vm, b_byte_arr *array);
void print_value(b_value value);
void echo_value(b_value value);
char *value_to_string(b_vm *vm, b_value value);
const char *value_type(b_value value);
bool values_equal(b_value a, b_value b);
uint32_t hash_double(double value);
uint32_t hash_string(const char *key, int length);
uint32_t hash_value(b_value value);
void sort_values(b_value *values, int count);
b_value copy_value(b_vm *vm, b_value value);
b_obj *allocate_object(b_vm *vm, size_t size, b_obj_type type);
b_obj_ptr *new_ptr(b_vm *vm, void *pointer);
b_obj_module *new_module(b_vm *vm, char *name, char *file);
b_obj_switch *new_switch(b_vm *vm);
b_obj_bytes *new_bytes(b_vm *vm, int length);
b_obj_list *new_list(b_vm *vm);
b_obj_range *new_range(b_vm *vm, int lower, int upper);
b_obj_dict *new_dict(b_vm *vm);
b_obj_file *new_file(b_vm *vm, b_obj_string *path, b_obj_string *mode);
b_obj_bound *new_bound_method(b_vm *vm, b_value receiver, b_obj_closure *method);
b_obj_class *new_class(b_vm *vm, b_obj_string *name);
b_obj_func *new_function(b_vm *vm, b_obj_module *module, b_func_type type);
b_obj_instance *new_instance(b_vm *vm, b_obj_class *klass);
b_obj_native *new_native(b_vm *vm, b_native_fn function, const char *name);
b_obj_closure *new_closure(b_vm *vm, b_obj_func *function);
b_obj_string *bl_string_fromallocated(b_vm *vm, char *chars, int length, uint32_t hash);
b_obj_string *take_string(b_vm *vm, char *chars, int length);
b_obj_string *copy_string(b_vm *vm, const char *chars, int length);
b_obj_up_value *new_up_value(b_vm *vm, b_value *slot);
void print_object(b_value value, bool fix_string);
b_obj_bytes *copy_bytes(b_vm *vm, unsigned char *b, int length);
b_obj_bytes *take_bytes(b_vm *vm, unsigned char *b, int length);
char *object_to_string(b_vm *vm, b_value value);
const char *object_type(b_obj *object);
void bl_scanner_init(b_scanner *s, const char *source);
bool bl_scanner_isatend(b_scanner *s);
b_token bl_scanner_skipblockcomments(b_scanner *s);
b_token bl_scanner_skipwhitespace(b_scanner *s);
b_token bl_scanner_scantoken(b_scanner *s);
b_obj_func *bl_compiler_compilesource(b_vm *vm, b_obj_module *module, const char *source, b_blob *blob);
void mark_compiler_roots(b_vm *vm);
void disassemble_blob(b_blob *blob, const char *name);
int simple_instruction(const char *name, int offset);
int constant_instruction(const char *name, b_blob *blob, int offset);
int short_instruction(const char *name, b_blob *blob, int offset);
int disassemble_instruction(b_blob *blob, int offset);

bool load_module(b_vm *vm, b_module_init init_fn, char *import_name, char *source, void *handle);
void add_native_module(b_vm *vm, b_obj_module *module, const char *as);
void bind_user_modules(b_vm *vm, char *pkg_root);
void bind_native_modules(b_vm *vm);
char *load_user_module(b_vm *vm, const char *path, char *name);
void close_dl_module(void *handle);

void array_free(void *data);
b_obj_ptr *new_array(b_vm *vm, b_array *array);
b_array *new_int16_array(b_vm *vm, int length);

b_array *new_int32_array(b_vm *vm, int length);

b_array *new_int64_array(b_vm *vm, int length);

b_array *new_uint16_array(b_vm *vm, int length);



b_array *new_uint32_array(b_vm *vm, int length);
b_array *new_uint64_array(b_vm *vm, int length);


b_module_reg *blade_module_loader_array(b_vm *vm);
b_module_reg *blade_module_loader_date(b_vm *vm);
void disable_raw_mode(void);
int getch(void);

b_value io_module_stdin(b_vm *vm);
b_value io_module_stdout(b_vm *vm);
b_value io_module_stderr(b_vm *vm);

void __io_module_unload(b_vm *vm);

b_module_reg *blade_module_loader_io(b_vm *vm);
b_module_reg *blade_module_loader_math(b_vm *vm);
b_value get_os_platform(b_vm *vm);
b_value get_blade_os_args(b_vm *vm);
b_value get_blade_os_path_separator(b_vm *vm);

b_value __os_dir_DT_UNKNOWN(b_vm *vm);
b_value __os_dir_DT_REG(b_vm *vm);
b_value __os_dir_DT_DIR(b_vm *vm);
b_value __os_dir_DT_FIFO(b_vm *vm);
b_value __os_dir_DT_SOCK(b_vm *vm);
b_value __os_dir_DT_CHR(b_vm *vm);
b_value __os_dir_DT_BLK(b_vm *vm);
b_value __os_dir_DT_LNK(b_vm *vm);
b_value __os_dir_DT_WHT(b_vm *vm);
void __os_module_preloader(b_vm *vm);
b_module_reg *blade_module_loader_os(b_vm *vm);
b_value __process_cpu_count(b_vm *vm);
void b__free_shared_memory(void *data);
bool native_module_process_Process(b_vm *vm, int arg_count, b_value *args);
bool native_module_process_create(b_vm *vm, int arg_count, b_value *args);
bool native_module_process_is_alive(b_vm *vm, int arg_count, b_value *args);
bool native_module_process_kill(b_vm *vm, int arg_count, b_value *args);
bool native_module_process_wait(b_vm *vm, int arg_count, b_value *args);
bool native_module_process_id(b_vm *vm, int arg_count, b_value *args);
bool native_module_process_new_shared(b_vm *vm, int arg_count, b_value *args);
bool native_module_process_shared_write(b_vm *vm, int arg_count, b_value *args);
bool native_module_process_shared_read(b_vm *vm, int arg_count, b_value *args);
bool native_module_process_shared_lock(b_vm *vm, int arg_count, b_value *args);
bool native_module_process_shared_unlock(b_vm *vm, int arg_count, b_value *args);
bool native_module_process_shared_islocked(b_vm *vm, int arg_count, b_value *args);
b_module_reg *blade_module_loader_process(b_vm *vm);
bool native_module_reflect__hasprop(b_vm *vm, int arg_count, b_value *args);
bool native_module_reflect__getprop(b_vm *vm, int arg_count, b_value *args);
bool native_module_reflect__setprop(b_vm *vm, int arg_count, b_value *args);
bool native_module_reflect__delprop(b_vm *vm, int arg_count, b_value *args);
bool native_module_reflect__hasmethod(b_vm *vm, int arg_count, b_value *args);
bool native_module_reflect__getmethod(b_vm *vm, int arg_count, b_value *args);
bool native_module_reflect__call_method(b_vm *vm, int arg_count, b_value *args);
bool native_module_reflect__bindmethod(b_vm *vm, int arg_count, b_value *args);
bool native_module_reflect__getboundmethod(b_vm *vm, int arg_count, b_value *args);
bool native_module_reflect__gettype(b_vm *vm, int arg_count, b_value *args);
bool native_module_reflect__isptr(b_vm *vm, int arg_count, b_value *args);
bool native_module_reflect__get_function_metadata(b_vm *vm, int arg_count, b_value *args);
b_module_reg *blade_module_loader_reflect(b_vm *vm);
bool native_module_struct_pack(b_vm *vm, int arg_count, b_value *args);
bool native_module_struct_unpack(b_vm *vm, int arg_count, b_value *args);
void __struct_module_preloader(b_vm *vm);
b_module_reg *blade_module_loader_struct(b_vm *vm);
bool bl_vm_propagateexception(b_vm *vm, bool is_assert);
bool bl_vm_pushexceptionhandler(b_vm *vm, b_obj_class *type, int address, int finally_address);
bool bl_vm_throwexception(b_vm *vm, bool is_assert, const char *format, ...);
b_obj_instance *create_exception(b_vm *vm, b_obj_string *message);
void bl_vm_runtimeerror(b_vm *vm, const char *format, ...);
void push(b_vm *vm, b_value value);
b_value pop(b_vm *vm);
b_value pop_n(b_vm *vm, int n);
b_value peek(b_vm *vm, int distance);
void define_native_method(b_vm *vm, b_table *table, const char *name, b_native_fn function);
void init_vm(b_vm *vm);
void free_vm(b_vm *vm);
bool call_value(b_vm *vm, b_value callee, int arg_count);
bool invoke_from_class(b_vm *vm, b_obj_class *klass, b_obj_string *name, int arg_count);
bool is_false(b_value value);
bool bl_class_isinstanceof(b_obj_class *klass1, char *klass2_name);
b_ptr_result bl_vm_run(b_vm *vm);
b_ptr_result bl_vm_interpsource(b_vm *vm, b_obj_module *module, const char *source);
void show_usage(char *argv[], bool fail);
int main(int argc, char *argv[]);



static bool is_obj_type(b_value v, b_obj_type t)
{
    return IS_OBJ(v) && AS_OBJ(v)->type == t;
}

static bool is_std_file(b_obj_file* file)
{
    return file->mode->length == 0;
}

static void add_module(b_vm* vm, b_obj_module* module)
{
    table_set(vm, &vm->modules, STRING_VAL(module->file), OBJ_VAL(module));
    if(vm->frame_count == 0)
    {
        table_set(vm, &vm->globals, STRING_VAL(module->name), OBJ_VAL(module));
    }
    else
    {
        table_set(vm, &vm->frames[vm->frame_count - 1].closure->fnptr->module->values, STRING_VAL(module->name), OBJ_VAL(module));
    }
}

static b_obj* gc_protect(b_vm* vm, b_obj* object)
{
    push(vm, OBJ_VAL(object));
    vm->gc_protected++;
    return object;
}

static void gc_clear_protection(b_vm* vm)
{
    if(vm->gc_protected > 0)
    {
        vm->stack_top -= vm->gc_protected;
    }
    vm->gc_protected = 0;
}


extern b_module_reg* blade_module_loader_base64(b_vm* vm);
extern b_module_reg* blade_module_loader_date(b_vm* vm);
extern b_module_reg* blade_module_loader_io(b_vm* vm);
extern b_module_reg* blade_module_loader_math(b_vm* vm);
extern b_module_reg* blade_module_loader_os(b_vm* vm);
extern b_module_reg* blade_module_loader_socket(b_vm* vm);
extern b_module_reg* blade_module_loader_hash(b_vm* vm);
extern b_module_reg* blade_module_loader_json(b_vm* vm);
extern b_module_reg* blade_module_loader_sqlite(b_vm* vm);
extern b_module_reg* blade_module_loader_reflect(b_vm* vm);
extern b_module_reg* blade_module_loader_array(b_vm* vm);
extern b_module_reg* blade_module_loader_process(b_vm* vm);
extern b_module_reg* blade_module_loader_struct(b_vm* vm);



    #define BLADE_PATH_SEPARATOR "/"

    #if defined(__linux__) || defined(__CYGWIN__) || defined(__MINGW32_MAJOR_VERSION)
        #define PROC_SELF_EXE "/proc/self/exe"
    #endif



// returns the number of bytes contained in a unicode character
int utf8_number_bytes(int value)
{
    if(value < 0)
    {
        return -1;
    }

    if(value <= 0x7f)
        return 1;
    if(value <= 0x7ff)
        return 2;
    if(value <= 0xffff)
        return 3;
    if(value <= 0x10ffff)
        return 4;
    return 0;
}

char* utf8_encode(unsigned int code)
{
    int count = utf8_number_bytes((int)code);
    if(count > 0)
    {
        char* chars = (char*)calloc((size_t)count + 1, sizeof(char));
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
    return (char*)"";
}

int utf8_decode_num_bytes(uint8_t byte)
{
    // If the byte starts with 10xxxxx, it's the middle of a UTF-8 sequence, so
    // don't count it at all.
    if((byte & 0xc0) == 0x80)
        return 0;

    // The first byte's high bits tell us how many bytes are in the UTF-8
    // sequence.
    if((byte & 0xf8) == 0xf0)
        return 4;
    if((byte & 0xf0) == 0xe0)
        return 3;
    if((byte & 0xe0) == 0xc0)
        return 2;
    return 1;
}

int utf8_decode(const uint8_t* bytes, uint32_t length)
{
    // Single byte (i.e. fits in ASCII).
    if(*bytes <= 0x7f)
        return *bytes;

    int value;
    uint32_t remaining_bytes;
    if((*bytes & 0xe0) == 0xc0)
    {
        // Two byte sequence: 110xxxxx 10xxxxxx.
        value = *bytes & 0x1f;
        remaining_bytes = 1;
    }
    else if((*bytes & 0xf0) == 0xe0)
    {
        // Three byte sequence: 1110xxxx	 10xxxxxx 10xxxxxx.
        value = *bytes & 0x0f;
        remaining_bytes = 2;
    }
    else if((*bytes & 0xf8) == 0xf0)
    {
        // Four byte sequence: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx.
        value = *bytes & 0x07;
        remaining_bytes = 3;
    }
    else
    {
        // Invalid UTF-8 sequence.
        return -1;
    }

    // Don't read past the end of the buffer on truncated UTF-8.
    if(remaining_bytes > length - 1)
        return -1;

    while(remaining_bytes > 0)
    {
        bytes++;
        remaining_bytes--;

        // Remaining bytes must be of form 10xxxxxx.
        if((*bytes & 0xc0) != 0x80)
            return -1;

        value = value << 6 | (*bytes & 0x3f);
    }

    return value;
}

char* append_strings(char* old, char* new_str)
{
    // quick exit...
    if(new_str == NULL)
    {
        return old;
    }

    // find the size of the string to allocate
    const size_t old_len = strlen(old), new_len = strlen(new_str);
    const size_t out_len = old_len + new_len;

    // allocate a pointer to the new string
    char* out = (char*)realloc((void*)old, out_len + 1);

    // concat both strings and return
    if(out != NULL)
    {
        memcpy(out + old_len, new_str, new_len);
        out[out_len] = '\0';
        return out;
    }

    return old;
}

/*char *append_strings(char *old, char *new_str) {
  // find the size of the string to allocate
  const size_t out_len = strlen(old) + strlen(new_str);
  char *result = realloc(old, out_len + 1);

  if (result != NULL) {
    strcat(result, new_str);
    result[out_len] = '\0'; // enforce string termination
  }

  return result;
}*/

int utf8len(char* s)
{
    int len = 0;
    for(; *s; ++s)
        if((*s & 0xC0) != 0x80)
            ++len;
    return len;
}

// returns a pointer to the beginning of the pos'th utf8 codepoint
// in the buffer at s
char* utf8index(char* s, int pos)
{
    ++pos;
    for(; *s; ++s)
    {
        if((*s & 0xC0) != 0x80)
            --pos;
        if(pos == 0)
            return s;
    }
    return NULL;
}

// converts codepoint indexes start and end to byte offsets in the buffer at s
void utf8slice(char* s, int* start, int* end)
{
    char* p = utf8index(s, *start);
    *start = p != NULL ? (int)(p - s) : -1;
    p = utf8index(s, *end);
    *end = p != NULL ? (int)(p - s) : (int)strlen(s);
}

char* read_file(const char* path)
{
    FILE* fp = fopen(path, "rb");

    // file not readable (maybe due to permission)
    if(fp == NULL)
    {
        return NULL;
    }

    fseek(fp, 0L, SEEK_END);
    size_t file_size = ftell(fp);
    rewind(fp);

    char* buffer = (char*)malloc(file_size + 1);

    // the system might not have enough memory to read the file.
    if(buffer == NULL)
    {
        fclose(fp);
        return NULL;
    }

    size_t bytes_read = fread(buffer, sizeof(char), file_size, fp);

    // if we couldn't read the entire file
    if(bytes_read < file_size)
    {
        fclose(fp);
        free(buffer);
        return NULL;
    }

    buffer[bytes_read] = '\0';

    fclose(fp);
    return buffer;
}

char* get_exe_path()
{
    char raw_path[PATH_MAX];
    ssize_t read_length;
    if((read_length = readlink(PROC_SELF_EXE, raw_path, sizeof(raw_path))) > -1 && read_length < PATH_MAX)
    {
        return strdup(raw_path);
    }
    return "";
}

char* get_exe_dir()
{
    return dirname(get_exe_path());
}

char* merge_paths(char* a, char* b)
{
    char* final_path = (char*)calloc(1, sizeof(char));

    // by checking b first, we guarantee that b is neither NULL nor
    // empty by the time we are checking a so that we can return a
    // duplicate of b
    int len_b = (int)strlen(b);
    if(b == NULL || len_b == 0)
    {
        free(final_path);
        return strdup(a);// just in case a is const char*
    }
    if(a == NULL || strlen(a) == 0)
    {
        free(final_path);
        return strdup(b);// just in case b is const char*
    }

    final_path = append_strings(final_path, a);

    if(!(len_b == 2 && b[0] == '.' && b[1] == 'b'))
    {
        final_path = append_strings(final_path, BLADE_PATH_SEPARATOR);
    }
    final_path = append_strings(final_path, b);
    return final_path;
}

bool file_exists(char* filepath)
{
    return access(filepath, F_OK) == 0;
}

char* get_blade_filename(char* filename)
{
    return merge_paths(filename, BLADE_EXTENSION);
}

char* resolve_import_path(char* module_name, const char* current_file, bool is_relative)
{
    char* blade_file_name = get_blade_filename(module_name);

    // check relative to the current file...
    char* file_directory = dirname((char*)strdup(current_file));

    // fixing last path / if exists (looking at windows)...
    int file_directory_length = (int)strlen(file_directory);
    if(file_directory[file_directory_length - 1] == '\\')
    {
        file_directory[file_directory_length - 1] = '\0';
    }

    // search system library if we are not looking for a relative module.
    if(!is_relative)
    {
        // firstly, search the local vendor directory for a matching module
        char* root_dir = getcwd(NULL, 0);
        // fixing last path / if exists (looking at windows)...
        int root_dir_length = (int)strlen(root_dir);
        if(root_dir[root_dir_length - 1] == '\\')
        {
            root_dir[root_dir_length - 1] = '\0';
        }

        char* vendor_file = merge_paths(merge_paths(root_dir, LOCAL_PACKAGES_DIRECTORY LOCAL_SRC_DIRECTORY), blade_file_name);
        if(file_exists(vendor_file))
        {
            // stop a core library from importing itself
            char* path1 = realpath(vendor_file, NULL);
            char* path2 = realpath(current_file, NULL);

            if(path1 != NULL)
            {
                if(path2 == NULL || memcmp(path1, path2, (int)strlen(path2)) != 0)
                    return path1;
            }
        }

        // or a matching package
        char* vendor_index_file
        = merge_paths(merge_paths(merge_paths(root_dir, LOCAL_PACKAGES_DIRECTORY LOCAL_SRC_DIRECTORY), module_name), LIBRARY_DIRECTORY_INDEX BLADE_EXTENSION);
        if(file_exists(vendor_index_file))
        {
            // stop a core library from importing itself
            char* path1 = realpath(vendor_index_file, NULL);
            char* path2 = realpath(current_file, NULL);

            if(path1 != NULL)
            {
                if(path2 == NULL || memcmp(path1, path2, (int)strlen(path2)) != 0)
                    return path1;
            }
        }

        // then, check in blade's default locations
        char* exe_dir = get_exe_dir();
        char* blade_directory = merge_paths(exe_dir, LIBRARY_DIRECTORY);

        // check blade libs directory for a matching module...
        char* library_file = merge_paths(blade_directory, blade_file_name);
        if(file_exists(library_file))
        {
            // stop a core library from importing itself
            char* path1 = realpath(library_file, NULL);
            char* path2 = realpath(current_file, NULL);

            if(path1 != NULL)
            {
                if(path2 == NULL || memcmp(path1, path2, (int)strlen(path2)) != 0)
                    return path1;
            }
        }

        // check blade libs directory for a matching package...
        char* library_index_file = merge_paths(merge_paths(blade_directory, module_name), get_blade_filename(LIBRARY_DIRECTORY_INDEX));
        if(file_exists(library_index_file))
        {
            char* path1 = realpath(library_index_file, NULL);
            char* path2 = realpath(current_file, NULL);

            if(path1 != NULL)
            {
                if(path2 == NULL || memcmp(path1, path2, (int)strlen(path2)) != 0)
                    return path1;
            }
        }

        // check blade vendor directory installed module...
        char* blade_package_directory = merge_paths(exe_dir, PACKAGES_DIRECTORY);
        char* package_file = merge_paths(blade_package_directory, blade_file_name);
        if(file_exists(package_file))
        {
            char* path1 = realpath(package_file, NULL);
            char* path2 = realpath(current_file, NULL);

            if(path1 != NULL)
            {
                if(path2 == NULL || memcmp(path1, path2, (int)strlen(path2)) != 0)
                    return path1;
            }
        }

        // check blade vendor directory installed package...
        char* package_index_file = merge_paths(merge_paths(blade_package_directory, module_name), LIBRARY_DIRECTORY_INDEX BLADE_EXTENSION);
        if(file_exists(package_index_file))
        {
            char* path1 = realpath(package_index_file, NULL);
            char* path2 = realpath(current_file, NULL);

            if(path1 != NULL)
            {
                if(path2 == NULL || memcmp(path1, path2, (int)strlen(path2)) != 0)
                    return path1;
            }
        }
    }
    else
    {
        // otherwise, search the relative path for a matching module
        char* relative_file = merge_paths(file_directory, blade_file_name);
        if(file_exists(relative_file))
        {
            // stop a user module from importing itself
            char* path1 = realpath(relative_file, NULL);
            char* path2 = realpath(current_file, NULL);

            if(path1 != NULL)
            {
                if(path2 == NULL || memcmp(path1, path2, (int)strlen(path2)) != 0)
                    return path1;
            }
        }

        // or a matching package
        char* relative_index_file = merge_paths(merge_paths(file_directory, module_name), get_blade_filename(LIBRARY_DIRECTORY_INDEX));
        if(file_exists(relative_index_file))
        {
            char* path1 = realpath(relative_index_file, NULL);
            char* path2 = realpath(current_file, NULL);

            if(path1 != NULL)
            {
                if(path2 == NULL || memcmp(path1, path2, (int)strlen(path2)) != 0)
                    return path1;
            }
        }
    }

    return NULL;
}

char* get_real_file_name(char* path)
{
    return basename(path);
}

#include <stdio.h>
#include <stdlib.h>

void* allocate(b_vm* vm, size_t size)
{
    vm->bytes_allocated += size;

    if(vm->bytes_allocated > vm->next_gc)
    {
        collect_garbage(vm);
    }

    if(size == 0)
    {
        return NULL;
    }
    void* result = malloc(size);

    // just in case reallocation fails... computers ain't infinite!
    if(result == NULL)
    {
        fflush(stdout);// flush out anything on stdout first
        fprintf(stderr, "Exit: device out of memory\n");
        exit(EXIT_TERMINAL);
    }
    return result;
}

void bl_mem_free(b_vm* vm, void* pointer, size_t sz)
{
    vm->bytes_allocated -= sz;
    free(pointer);    
}

void* reallocate(b_vm* vm, void* pointer, size_t old_size, size_t new_size)
{
    vm->bytes_allocated += new_size - old_size;

    if(new_size > old_size && vm->bytes_allocated > vm->next_gc)
    {
        collect_garbage(vm);
    }
    void* result = realloc(pointer, new_size);
    // just in case reallocation fails... computers ain't infinite!
    if(result == NULL)
    {
        fflush(stdout);// flush out anything on stdout first
        fprintf(stderr, "Exit: device out of memory\n");
        exit(EXIT_TERMINAL);
    }
    return result;
}

void mark_object(b_vm* vm, b_obj* object)
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
    //  print_object(OBJ_VAL(object), false);
    //  printf("\n");
    //#endif

    object->mark = true;

    if(vm->gray_capacity < vm->gray_count + 1)
    {
        vm->gray_capacity = GROW_CAPACITY(vm->gray_capacity);
        vm->gray_stack = (b_obj**)realloc(vm->gray_stack, sizeof(b_obj*) * vm->gray_capacity);

        if(vm->gray_stack == NULL)
        {
            fflush(stdout);// flush out anything on stdout first
            fprintf(stderr, "GC encountered an error");
            exit(1);
        }
    }
    vm->gray_stack[vm->gray_count++] = object;
}

void mark_value(b_vm* vm, b_value value)
{
    if(IS_OBJ(value))
        mark_object(vm, AS_OBJ(value));
}

static void mark_array(b_vm* vm, b_value_arr* array)
{
    for(int i = 0; i < array->count; i++)
    {
        mark_value(vm, array->values[i]);
    }
}

void blacken_object(b_vm* vm, b_obj* object)
{
    //#if defined(DEBUG_LOG_GC) && DEBUG_LOG_GC
    //  printf("%p blacken ", (void *)object);
    //  print_object(OBJ_VAL(object), false);
    //  printf("\n");
    //#endif

    switch(object->type)
    {
        case OBJ_MODULE:
        {
            b_obj_module* module = (b_obj_module*)object;
            mark_table(vm, &module->values);
            break;
        }
        case OBJ_SWITCH:
        {
            b_obj_switch* sw = (b_obj_switch*)object;
            mark_table(vm, &sw->table);
            break;
        }
        case OBJ_FILE:
        {
            b_obj_file* file = (b_obj_file*)object;
            mark_object(vm, (b_obj*)file->mode);
            mark_object(vm, (b_obj*)file->path);
            break;
        }
        case OBJ_DICT:
        {
            b_obj_dict* dict = (b_obj_dict*)object;
            mark_array(vm, &dict->names);
            mark_table(vm, &dict->items);
            break;
        }
        case OBJ_LIST:
        {
            b_obj_list* list = (b_obj_list*)object;
            mark_array(vm, &list->items);
            break;
        }

        case OBJ_BOUND_METHOD:
        {
            b_obj_bound* bound = (b_obj_bound*)object;
            mark_value(vm, bound->receiver);
            mark_object(vm, (b_obj*)bound->method);
            break;
        }
        case OBJ_CLASS:
        {
            b_obj_class* klass = (b_obj_class*)object;
            mark_object(vm, (b_obj*)klass->name);
            mark_table(vm, &klass->methods);
            mark_table(vm, &klass->properties);
            mark_table(vm, &klass->static_properties);
            mark_value(vm, klass->initializer);
            if(klass->superclass != NULL)
            {
                mark_object(vm, (b_obj*)klass->superclass);
            }
            break;
        }
        case OBJ_CLOSURE:
        {
            b_obj_closure* closure = (b_obj_closure*)object;
            mark_object(vm, (b_obj*)closure->fnptr);
            for(int i = 0; i < closure->up_value_count; i++)
            {
                mark_object(vm, (b_obj*)closure->up_values[i]);
            }
            break;
        }

        case OBJ_FUNCTION:
        {
            b_obj_func* function = (b_obj_func*)object;
            mark_object(vm, (b_obj*)function->name);
            mark_object(vm, (b_obj*)function->module);
            mark_array(vm, &function->blob.constants);
            break;
        }
        case OBJ_INSTANCE:
        {
            b_obj_instance* instance = (b_obj_instance*)object;
            mark_object(vm, (b_obj*)instance->klass);
            mark_table(vm, &instance->properties);
            break;
        }

        case OBJ_UP_VALUE:
        {
            mark_value(vm, ((b_obj_up_value*)object)->closed);
            break;
        }

        case OBJ_BYTES:
        case OBJ_RANGE:
        case OBJ_NATIVE:
        case OBJ_PTR:
        {
            mark_object(vm, object);
            break;
        }
        case OBJ_STRING:
            break;
    }
}

void free_object(b_vm* vm, b_obj** pobject)
{
    //#if defined(DEBUG_LOG_GC) && DEBUG_LOG_GC
    //  printf("%p free type %d\n", (void *)object, object->type);
    //#endif
    b_obj* object;
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
            b_obj_module* module = (b_obj_module*)object;
            free_table(vm, &module->values);
            free(module->name);
            free(module->file);
            if(module->unloader != NULL && module->imported)
            {
                ((b_module_loader)module->unloader)(vm);
            }
            if(module->handle != NULL)
            {
                close_dl_module(module->handle);// free the shared library...
            }
            FREE(b_obj_module, object);
            break;
        }
        case OBJ_BYTES:
        {
            b_obj_bytes* bytes = (b_obj_bytes*)object;
            free_byte_arr(vm, &bytes->bytes);
            FREE(b_obj_bytes, object);
            break;
        }
        case OBJ_FILE:
        {
            b_obj_file* file = (b_obj_file*)object;
            if(file->mode->length != 0 && !is_std_file(file) && file->file != NULL)
            {
                fclose(file->file);
            }
            FREE(b_obj_file, object);
            break;
        }
        case OBJ_DICT:
        {
            b_obj_dict* dict = (b_obj_dict*)object;
            free_value_arr(vm, &dict->names);
            free_table(vm, &dict->items);
            FREE(b_obj_dict, object);
            break;
        }
        case OBJ_LIST:
        {
            b_obj_list* list = (b_obj_list*)object;
            free_value_arr(vm, &list->items);
            FREE(b_obj_list, object);
            break;
        }

        case OBJ_BOUND_METHOD:
        {
            // a closure may be bound to multiple instances
            // for this reason, we do not free closures when freeing bound methods
            FREE(b_obj_bound, object);
            break;
        }
        case OBJ_CLASS:
        {
            b_obj_class* klass = (b_obj_class*)object;
            //free_object(vm, (b_obj**)&klass->name);
            free_table(vm, &klass->methods);
            free_table(vm, &klass->properties);
            free_table(vm, &klass->static_properties);
            if(!IS_EMPTY(klass->initializer))
            {
                // FIXME: uninitialized
                //free_object(vm, &AS_OBJ(klass->initializer));
            }
            FREE(b_obj_class, object);
            break;
        }
        case OBJ_CLOSURE:
        {
            b_obj_closure* closure = (b_obj_closure*)object;
            FREE_ARRAY(b_obj_up_value*, closure->up_values, closure->up_value_count);
            // there may be multiple closures that all reference the same function
            // for this reason, we do not free functions when freeing closures
            FREE(b_obj_closure, object);
            break;
        }
        case OBJ_FUNCTION:
        {
            b_obj_func* function = (b_obj_func*)object;
            free_blob(vm, &function->blob);
            if(function->name != NULL)
            {
                //free_object(vm, (b_obj**)&function->name);
            }
            FREE(b_obj_func, object);
            break;
        }
        case OBJ_INSTANCE:
        {
            b_obj_instance* instance = (b_obj_instance*)object;
            free_table(vm, &instance->properties);
            FREE(b_obj_instance, object);
            break;
        }
        case OBJ_NATIVE:
        {
            FREE(b_obj_native, object);
            break;
        }
        case OBJ_UP_VALUE:
        {
            FREE(b_obj_up_value, object);
            break;
        }
        case OBJ_RANGE:
        {
            FREE(b_obj_range, object);
            break;
        }
        case OBJ_STRING:
        {
            b_obj_string* string = (b_obj_string*)object;
            if(string->length > 0)
            {
                FREE_ARRAY(char, string->chars, (size_t)string->length + 1);
            }
            FREE(b_obj_string, object);
            break;
        }

        case OBJ_SWITCH:
        {
            b_obj_switch* sw = (b_obj_switch*)object;
            free_table(vm, &sw->table);
            FREE(b_obj_switch, object);
            break;
        }

        case OBJ_PTR:
        {
            b_obj_ptr* ptr = (b_obj_ptr*)object;
            if(ptr->free_fn)
            {
                ptr->free_fn(ptr->pointer);
            }
            FREE(b_obj_ptr, object);
            break;
        }

        default:
            break;
    }
    *pobject = NULL;
}

static void mark_roots(b_vm* vm)
{
    int i;
    int j;
    b_exception_frame* handler;
    b_obj_up_value* up_value;
    b_value* slot;
    for(slot = vm->stack; slot < vm->stack_top; slot++)
    {
        mark_value(vm, *slot);
    }
    for(i = 0; i < vm->frame_count; i++)
    {
        mark_object(vm, (b_obj*)vm->frames[i].closure);
        for(j = 0; j < vm->frames[i].handlers_count; j++)
        {
            handler = &vm->frames[i].handlers[j];
            mark_object(vm, (b_obj*)handler->klass);
        }
    }
    for(up_value = vm->open_up_values; up_value != NULL; up_value = up_value->next)
    {
        mark_object(vm, (b_obj*)up_value);
    }
    mark_table(vm, &vm->globals);
    mark_table(vm, &vm->modules);
    mark_table(vm, &vm->methods_string);
    mark_table(vm, &vm->methods_bytes);
    mark_table(vm, &vm->methods_file);
    mark_table(vm, &vm->methods_list);
    mark_table(vm, &vm->methods_dict);
    mark_table(vm, &vm->methods_range);
    mark_object(vm, (b_obj*)vm->exception_class);
    mark_compiler_roots(vm);
}

static void trace_references(b_vm* vm)
{
    b_obj* object;
    while(vm->gray_count > 0)
    {
        object = vm->gray_stack[--vm->gray_count];
        blacken_object(vm, object);
    }
}

static void sweep(b_vm* vm)
{
    b_obj* previous;
    b_obj* object;
    b_obj* unreached;
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
            free_object(vm, &unreached);
        }
    }
}

void free_objects(b_vm* vm)
{
    size_t i;
    b_obj* next;
    b_obj* object;
    i = 0;
    object = vm->objectlinks;
    while(object != NULL)
    {
        i++;
        fprintf(stderr, "free_objects: index %d of %d\n", i, vm->objectcount);
        next = object->sibling;
        free_object(vm, &object);
        object = next;
    }
    free(vm->gray_stack);
    vm->gray_stack = NULL;
}

void collect_garbage(b_vm* vm)
{
    if(!vm->allowgc)
    {
        return;
    }
#if defined(DEBUG_LOG_GC) && DEBUG_LOG_GC
    printf("-- gc begins\n");
    size_t before = vm->bytes_allocated;
#endif
    vm->allowgc = false;

    /*
        tin_gcmem_vmmarkroots(vm);
    tin_gcmem_vmtracerefs(vm);
    tin_strreg_remwhite(vm->state);
    tin_gcmem_vmsweep(vm);
    vm->state->gcnext = vm->state->gcbytescount * TIN_GC_HEAP_GROW_FACTOR;
    */

    mark_roots(vm);
    trace_references(vm);
    table_remove_whites(vm, &vm->strings);
    table_remove_whites(vm, &vm->modules);
    sweep(vm);

    vm->next_gc = vm->bytes_allocated * GC_HEAP_GROWTH_FACTOR;
    vm->allowgc = true;
#if defined(DEBUG_LOG_GC) && DEBUG_LOG_GC
    printf("-- gc ends\n");
    printf("   collected %zu bytes (from %zu to %zu), next at %zu\n", before - vm->bytes_allocated, before, vm->bytes_allocated, vm->next_gc);
#endif
}

#include <stdlib.h>

void init_blob(b_blob* blob)
{
    blob->count = 0;
    blob->capacity = 0;
    blob->code = NULL;
    blob->lines = NULL;
    init_value_arr(&blob->constants);
}

void write_blob(b_vm* vm, b_blob* blob, uint8_t byte, int line)
{
    if(blob->capacity < blob->count + 1)
    {
        int old_capacity = blob->capacity;
        blob->capacity = GROW_CAPACITY(old_capacity);
        blob->code = GROW_ARRAY(uint8_t, blob->code, old_capacity, blob->capacity);
        blob->lines = GROW_ARRAY(int, blob->lines, old_capacity, blob->capacity);
    }

    blob->code[blob->count] = byte;
    blob->lines[blob->count] = line;
    blob->count++;
}

void free_blob(b_vm* vm, b_blob* blob)
{
    if(blob->code != NULL)
    {
        FREE_ARRAY(uint8_t, blob->code, blob->capacity);
    }
    if(blob->lines != NULL)
    {
        FREE_ARRAY(int, blob->lines, blob->capacity);
    }
    free_value_arr(vm, &blob->constants);
    init_blob(blob);
}

int add_constant(b_vm* vm, b_blob* blob, b_value value)
{
    push(vm, value);// fixing gc corruption
    write_value_arr(vm, &blob->constants, value);
    pop(vm);// fixing gc corruption
    return blob->constants.count - 1;
}

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
uint32_t is_regex(b_obj_string* string)
{
    char start = string->chars[0];
    bool match_found = false;

    uint32_t c_options = 0;// pcre2 options

    for(int i = 1; i < string->length; i++)
    {
        if(string->chars[i] == start)
        {
            match_found = i > 0 && string->chars[i - 1] == '\\' ? false : true;
            continue;
        }

        if(match_found)
        {
            // compile the delimiters
            switch(string->chars[i])
            {
                /* Perl compatible options */
                case 'i':
                    c_options |= PCRE2_CASELESS;
                    break;
                case 'm':
                    c_options |= PCRE2_MULTILINE;
                    break;
                case 's':
                    c_options |= PCRE2_DOTALL;
                    break;
                case 'x':
                    c_options |= PCRE2_EXTENDED;
                    break;

                    /* PCRE specific options */
                case 'A':
                    c_options |= PCRE2_ANCHORED;
                    break;
                case 'D':
                    c_options |= PCRE2_DOLLAR_ENDONLY;
                    break;
                case 'U':
                    c_options |= PCRE2_UNGREEDY;
                    break;
                case 'u':
                    c_options |= PCRE2_UTF;
                    /* In  PCRE,  by  default, \d, \D, \s, \S, \w, and \W recognize only
         ASCII characters, even in UTF-8 mode. However, this can be changed by
         setting the PCRE2_UCP option. */
#ifdef PCRE2_UCP
                    c_options |= PCRE2_UCP;
#endif
                    break;
                case 'J':
                    c_options |= PCRE2_DUPNAMES;
                    break;

                case ' ':
                case '\n':
                case '\r':
                    break;

                default:
                    return c_options = (uint32_t)string->chars[i] + 1000000;
            }
        }
    }

    if(!match_found)
        return -1;
    else
        return c_options;
}

char* remove_regex_delimiter(b_vm* vm, b_obj_string* string)
{
    if(string->length == 0)
        return string->chars;

    char start = string->chars[0];
    int i = string->length - 1;
    for(; i > 0; i--)
    {
        if(string->chars[i] == start)
            break;
    }

    char* str = ALLOCATE(char, i);
    memcpy(str, string->chars + 1, (size_t)i - 1);
    str[i - 1] = '\0';

    return str;
}

bool native_method_stringlength(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(length, 0);
    b_obj_string* string = AS_STRING(METHOD_OBJECT);
    RETURN_NUMBER(string->is_ascii ? string->length : string->utf8_length);
}

bool native_method_stringupper(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(upper, 0);
    char* string = (char*)strdup(AS_C_STRING(METHOD_OBJECT));
    for(char* p = string; *p; p++)
        *p = toupper(*p);
    RETURN_L_STRING(string, AS_STRING(METHOD_OBJECT)->length);
}

bool native_method_stringlower(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(lower, 0);
    char* string = (char*)strdup(AS_C_STRING(METHOD_OBJECT));
    for(char* p = string; *p; p++)
        *p = tolower(*p);
    RETURN_L_STRING(string, AS_STRING(METHOD_OBJECT)->length);
}

bool native_method_stringis_alpha(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(is_alpha, 0);
    b_obj_string* string = AS_STRING(METHOD_OBJECT);
    for(int i = 0; i < string->length; i++)
    {
        if(!isalpha((unsigned char)string->chars[i]))
        {
            RETURN_FALSE;
        }
    }
    RETURN_BOOL(string->length != 0);
}

bool native_method_stringis_alnum(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(is_alnum, 0);
    b_obj_string* string = AS_STRING(METHOD_OBJECT);
    for(int i = 0; i < string->length; i++)
    {
        if(!isalnum((unsigned char)string->chars[i]))
        {
            RETURN_FALSE;
        }
    }
    RETURN_BOOL(string->length != 0);
}

bool native_method_stringis_number(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(is_number, 0);
    b_obj_string* string = AS_STRING(METHOD_OBJECT);
    for(int i = 0; i < string->length; i++)
    {
        if(!isdigit((unsigned char)string->chars[i]))
        {
            RETURN_FALSE;
        }
    }
    RETURN_BOOL(string->length != 0);
}

bool native_method_stringis_lower(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(is_lower, 0);
    b_obj_string* string = AS_STRING(METHOD_OBJECT);
    bool has_alpha;
    for(int i = 0; i < string->length; i++)
    {
        bool isal = isalpha((unsigned char)string->chars[i]);
        if(!has_alpha)
        {
            has_alpha = isal;
        }
        if(isal && !islower((unsigned char)string->chars[i]))
        {
            RETURN_FALSE;
        }
    }
    RETURN_BOOL(string->length != 0 && has_alpha);
}

bool native_method_stringis_upper(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(is_upper, 0);
    b_obj_string* string = AS_STRING(METHOD_OBJECT);
    bool has_alpha;
    for(int i = 0; i < string->length; i++)
    {
        bool isal = isalpha((unsigned char)string->chars[i]);
        if(!has_alpha)
        {
            has_alpha = isal;
        }
        if(isal && !isupper((unsigned char)string->chars[i]))
        {
            RETURN_FALSE;
        }
    }
    RETURN_BOOL(string->length != 0 && has_alpha);
}

bool native_method_stringis_space(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(is_space, 0);
    b_obj_string* string = AS_STRING(METHOD_OBJECT);
    for(int i = 0; i < string->length; i++)
    {
        if(!isspace((unsigned char)string->chars[i]))
        {
            RETURN_FALSE;
        }
    }
    RETURN_BOOL(string->length != 0);
}

bool native_method_stringtrim(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_RANGE(trim, 0, 1);

    char trimmer = '\0';

    if(arg_count == 1)
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
            string++;
    }
    else
    {
        while(trimmer == *string)
            string++;
    }

    if(*string == 0)
    {// All spaces?
        RETURN_OBJ(copy_string(vm, "", 0));
    }

    // Trim trailing space
    end = string + strlen(string) - 1;
    if(trimmer == '\0')
    {
        while(end > string && isspace((unsigned char)*end))
            end--;
    }
    else
    {
        while(end > string && trimmer == *end)
            end--;
    }

    // Write new null terminator character
    end[1] = '\0';

    RETURN_STRING(string);
}

bool native_method_stringltrim(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_RANGE(ltrim, 0, 1);

    char trimmer = '\0';

    if(arg_count == 1)
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
            string++;
    }
    else
    {
        while(trimmer == *string)
            string++;
    }

    if(*string == 0)
    {// All spaces?
        RETURN_OBJ(copy_string(vm, "", 0));
    }

    end = string + strlen(string) - 1;

    // Write new null terminator character
    end[1] = '\0';

    RETURN_STRING(string);
}

bool native_method_stringrtrim(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_RANGE(rtrim, 0, 1);

    char trimmer = '\0';

    if(arg_count == 1)
    {
        ENFORCE_ARG_TYPE(rtrim, 0, IS_CHAR);
        trimmer = (char)AS_STRING(args[0])->chars[0];
    }

    char* string = AS_C_STRING(METHOD_OBJECT);

    char* end = NULL;

    if(*string == 0)
    {// All spaces?
        RETURN_OBJ(copy_string(vm, "", 0));
    }

    end = string + strlen(string) - 1;
    if(trimmer == '\0')
    {
        while(end > string && isspace((unsigned char)*end))
            end--;
    }
    else
    {
        while(end > string && trimmer == *end)
            end--;
    }

    // Write new null terminator character
    end[1] = '\0';

    RETURN_STRING(string);
}

bool native_method_stringjoin(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(join, 1);
    ENFORCE_ARG_TYPE(join, 0, IS_OBJ);

    b_obj_string* method_obj = AS_STRING(METHOD_OBJECT);
    b_value argument = args[0];
    int length = 0;
    char** array = NULL;

    if(IS_STRING(argument))
    {
        // empty argument
        if(method_obj->length == 0)
        {
            RETURN_VALUE(argument);
        }
        else if(AS_STRING(argument)->length == 0)
        {
            RETURN_VALUE(argument);
        }

        b_obj_string* string = AS_STRING(argument);

        char* result = ALLOCATE(char, 2);
        result[0] = string->chars[0];
        result[1] = '\0';

        for(int i = 1; i < string->length; i++)
        {
            if(method_obj->length > 0)
            {
                result = append_strings(result, method_obj->chars);
            }

            char* chr = (char*)calloc(2, sizeof(char));
            chr[0] = string->chars[i];
            chr[1] = '\0';

            result = append_strings(result, chr);
            free(chr);
        }

        RETURN_TT_STRING(result);
    }
    else if(IS_LIST(argument) || IS_DICT(argument))
    {
        b_value* list;
        int count = 0;
        if(IS_DICT(argument))
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
            RETURN_STRING("");
        }

        char* result = value_to_string(vm, list[0]);

        for(int i = 1; i < count; i++)
        {
            if(method_obj->length > 0)
            {
                result = append_strings(result, method_obj->chars);
            }

            char* str = value_to_string(vm, list[i]);
            result = append_strings(result, str);
            free(str);
        }

        RETURN_TT_STRING(result);
    }

    RETURN_ERROR("join() does not support object of type %s", value_type(argument));
}

bool native_method_stringsplit(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(split, 1);
    ENFORCE_ARG_TYPE(split, 0, IS_STRING);

    b_obj_string* object = AS_STRING(METHOD_OBJECT);
    b_obj_string* delimeter = AS_STRING(args[0]);

    if(object->length == 0 || delimeter->length > object->length)
        RETURN_OBJ(new_list(vm));

    b_obj_list* list = (b_obj_list*)GC(new_list(vm));

    // main work here...
    if(delimeter->length > 0)
    {
        int start = 0;
        for(int i = 0; i <= object->length; i++)
        {
            // match found.
            if(memcmp(object->chars + i, delimeter->chars, delimeter->length) == 0 || i == object->length)
            {
                write_list(vm, list, GC_L_STRING(object->chars + start, i - start));
                i += delimeter->length - 1;
                start = i + 1;
            }
        }
    }
    else
    {
        int length = object->is_ascii ? object->length : object->utf8_length;
        for(int i = 0; i < length; i++)
        {
            int start = i, end = i + 1;
            if(!object->is_ascii)
            {
                utf8slice(object->chars, &start, &end);
            }

            write_list(vm, list, GC_L_STRING(object->chars + start, (int)(end - start)));
        }
    }

    RETURN_OBJ(list);
}

bool native_method_stringindex_of(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(index_of, 1);
    ENFORCE_ARG_TYPE(index_of, 0, IS_STRING);

    char* str = AS_C_STRING(METHOD_OBJECT);
    char* result = strstr(str, AS_C_STRING(args[0]));

    if(result != NULL)
        RETURN_NUMBER((int)(result - str));
    RETURN_NUMBER(-1);
}

bool native_method_stringstarts_with(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(starts_with, 1);
    ENFORCE_ARG_TYPE(starts_with, 0, IS_STRING);

    b_obj_string* string = AS_STRING(METHOD_OBJECT);
    b_obj_string* substr = AS_STRING(args[0]);

    if(string->length == 0 || substr->length == 0 || substr->length > string->length)
        RETURN_FALSE;

    RETURN_BOOL(memcmp(substr->chars, string->chars, substr->length) == 0);
}

bool native_method_stringends_with(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(ends_with, 1);
    ENFORCE_ARG_TYPE(ends_with, 0, IS_STRING);

    b_obj_string* string = AS_STRING(METHOD_OBJECT);
    b_obj_string* substr = AS_STRING(args[0]);

    if(string->length == 0 || substr->length == 0 || substr->length > string->length)
        RETURN_FALSE;

    int difference = string->length - substr->length;

    RETURN_BOOL(memcmp(substr->chars, string->chars + difference, substr->length) == 0);
}

bool native_method_stringcount(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(count, 1);
    ENFORCE_ARG_TYPE(count, 0, IS_STRING);

    b_obj_string* string = AS_STRING(METHOD_OBJECT);
    b_obj_string* substr = AS_STRING(args[0]);

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

bool native_method_stringto_number(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(to_number, 0);
    RETURN_NUMBER(strtod(AS_C_STRING(METHOD_OBJECT), NULL));
}

bool native_method_stringascii(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(ascii, 0);
    b_obj_string* string = AS_STRING(METHOD_OBJECT);
    string->is_ascii = true;
    RETURN_OBJ(string);
}

bool native_method_stringto_list(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(to_list, 0);
    b_obj_string* string = AS_STRING(METHOD_OBJECT);
    b_obj_list* list = (b_obj_list*)GC(new_list(vm));
    int length = string->is_ascii ? string->length : string->utf8_length;

    if(length > 0)
    {
        for(int i = 0; i < length; i++)
        {
            int start = i, end = i + 1;
            if(!string->is_ascii)
            {
                utf8slice(string->chars, &start, &end);
            }
            write_list(vm, list, GC_L_STRING(string->chars + start, (int)(end - start)));
        }
    }

    RETURN_OBJ(list);
}

bool native_method_stringlpad(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_RANGE(lpad, 1, 2);
    ENFORCE_ARG_TYPE(lpad, 0, IS_NUMBER);

    b_obj_string* string = AS_STRING(METHOD_OBJECT);
    int width = AS_NUMBER(args[0]);
    char fill_char = ' ';

    if(arg_count == 2)
    {
        ENFORCE_ARG_TYPE(lpad, 1, IS_CHAR);
        fill_char = AS_C_STRING(args[1])[0];
    }

    if(width <= string->utf8_length)
        RETURN_VALUE(METHOD_OBJECT);

    int fill_size = width - string->utf8_length;
    char* fill = ALLOCATE(char, (size_t)fill_size + 1);

    int final_size = string->length + fill_size;
    int final_utf8_size = string->utf8_length + fill_size;

    for(int i = 0; i < fill_size; i++)
        fill[i] = fill_char;

    char* str = ALLOCATE(char, (size_t)string->length + (size_t)fill_size + 1);
    memcpy(str, fill, fill_size);
    memcpy(str + fill_size, string->chars, string->length);
    str[final_size] = '\0';

    b_obj_string* result = take_string(vm, str, final_size);
    result->utf8_length = final_utf8_size;
    result->length = final_size;
    RETURN_OBJ(result);
}

bool native_method_stringrpad(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_RANGE(rpad, 1, 2);
    ENFORCE_ARG_TYPE(rpad, 0, IS_NUMBER);

    b_obj_string* string = AS_STRING(METHOD_OBJECT);
    int width = AS_NUMBER(args[0]);
    char fill_char = ' ';

    if(arg_count == 2)
    {
        ENFORCE_ARG_TYPE(rpad, 1, IS_CHAR);
        fill_char = AS_C_STRING(args[1])[0];
    }

    if(width <= string->utf8_length)
        RETURN_VALUE(METHOD_OBJECT);

    int fill_size = width - string->utf8_length;
    char* fill = ALLOCATE(char, (size_t)fill_size + 1);

    int final_size = string->length + fill_size;
    int final_utf8_size = string->utf8_length + fill_size;

    for(int i = 0; i < fill_size; i++)
        fill[i] = fill_char;

    char* str = ALLOCATE(char, (size_t)string->length + (size_t)fill_size + 1);
    memcpy(str, string->chars, string->length);
    memcpy(str + string->length, fill, fill_size);
    str[final_size] = '\0';

    b_obj_string* result = take_string(vm, str, final_size);
    result->utf8_length = final_utf8_size;
    result->length = final_size;
    RETURN_OBJ(result);
}

bool native_method_stringmatch(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(match, 1);
    ENFORCE_ARG_TYPE(match, 0, IS_STRING);

    b_obj_string* string = AS_STRING(METHOD_OBJECT);
    b_obj_string* substr = AS_STRING(args[0]);

    if(string->length == 0 && substr->length == 0)
    {
        RETURN_TRUE;
    }
    else if(string->length == 0 || substr->length == 0)
    {
        RETURN_FALSE;
    }

    GET_REGEX_COMPILE_OPTIONS(substr, false);

    if((int)compile_options < 0)
    {
        RETURN_BOOL(strstr(string->chars, substr->chars) - string->chars > -1);
    }

    char* real_regex = remove_regex_delimiter(vm, substr);

    int error_number;
    PCRE2_SIZE error_offset;

    PCRE2_SPTR pattern = (PCRE2_SPTR)real_regex;
    PCRE2_SPTR subject = (PCRE2_SPTR)string->chars;
    PCRE2_SIZE subject_length = (PCRE2_SIZE)string->length;

    pcre2_code* re = pcre2_compile(pattern, PCRE2_ZERO_TERMINATED, compile_options, &error_number, &error_offset, NULL);
    free(real_regex);

    REGEX_COMPILATION_ERROR(re, error_number, error_offset);

    pcre2_match_data* match_data = pcre2_match_data_create_from_pattern(re, NULL);

    int rc = pcre2_match(re, subject, subject_length, 0, 0, match_data, NULL);

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

    PCRE2_SIZE* o_vector = pcre2_get_ovector_pointer(match_data);
    uint32_t name_count;

    b_obj_dict* result = (b_obj_dict*)GC(new_dict(vm));
    (void)pcre2_pattern_info(re, PCRE2_INFO_NAMECOUNT, &name_count);

    for(int i = 0; i < rc; i++)
    {
        PCRE2_SIZE substring_length = o_vector[2 * i + 1] - o_vector[2 * i];
        PCRE2_SPTR substring_start = subject + o_vector[2 * i];
        dict_set_entry(vm, result, NUMBER_VAL(i), GC_L_STRING((char*)substring_start, (int)substring_length));
    }

    if(name_count > 0)
    {
        uint32_t name_entry_size;
        PCRE2_SPTR name_table;
        PCRE2_SPTR tab_ptr;
        (void)pcre2_pattern_info(re, PCRE2_INFO_NAMETABLE, &name_table);
        (void)pcre2_pattern_info(re, PCRE2_INFO_NAMEENTRYSIZE, &name_entry_size);

        tab_ptr = name_table;

        for(int i = 0; i < (int)name_count; i++)
        {
            int n = (tab_ptr[0] << 8) | tab_ptr[1];

            int value_length = (int)(o_vector[2 * n + 1] - o_vector[2 * n]);
            int key_length = (int)name_entry_size - 3;

            char* _key = ALLOCATE(char, key_length + 1);
            char* _val = ALLOCATE(char, value_length + 1);

            sprintf(_key, "%*s", key_length, tab_ptr + 2);
            sprintf(_val, "%*s", value_length, subject + o_vector[2 * n]);

            while(isspace((unsigned char)*_key))
                _key++;

            dict_set_entry(vm, result, OBJ_VAL(GC(take_string(vm, _key, key_length))), OBJ_VAL(GC(take_string(vm, _val, value_length))));

            tab_ptr += name_entry_size;
        }
    }

    pcre2_match_data_free(match_data);
    pcre2_code_free(re);

    RETURN_OBJ(result);
}

bool native_method_stringmatches(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(matches, 1);
    ENFORCE_ARG_TYPE(matches, 0, IS_STRING);

    b_obj_string* string = AS_STRING(METHOD_OBJECT);
    b_obj_string* substr = AS_STRING(args[0]);

    if(string->length == 0 && substr->length == 0)
    {
        RETURN_OBJ(new_list(vm));// empty string matches empty string to empty list
    }
    else if(string->length == 0 || substr->length == 0)
    {
        RETURN_FALSE;// if either string or str is empty, return false
    }

    GET_REGEX_COMPILE_OPTIONS(substr, true);

    char* real_regex = remove_regex_delimiter(vm, substr);

    int error_number;
    PCRE2_SIZE error_offset;
    uint32_t option_bits;
    uint32_t newline;
    uint32_t name_count, group_count;
    uint32_t name_entry_size;
    PCRE2_SPTR name_table;

    PCRE2_SPTR pattern = (PCRE2_SPTR)real_regex;
    PCRE2_SPTR subject = (PCRE2_SPTR)string->chars;
    PCRE2_SIZE subject_length = (PCRE2_SIZE)string->length;

    pcre2_code* re = pcre2_compile(pattern, PCRE2_ZERO_TERMINATED, compile_options, &error_number, &error_offset, NULL);
    free(real_regex);

    REGEX_COMPILATION_ERROR(re, error_number, error_offset);

    pcre2_match_data* match_data = pcre2_match_data_create_from_pattern(re, NULL);

    int rc = pcre2_match(re, subject, subject_length, 0, 0, match_data, NULL);

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

    PCRE2_SIZE* o_vector = pcre2_get_ovector_pointer(match_data);

    //   REGEX_VECTOR_SIZE_WARNING();

    // handle edge cases such as /(?=.\K)/
    REGEX_ASSERTION_ERROR(re, match_data, o_vector);

    (void)pcre2_pattern_info(re, PCRE2_INFO_NAMECOUNT, &name_count);
    (void)pcre2_pattern_info(re, PCRE2_INFO_CAPTURECOUNT, &group_count);

    b_obj_dict* result = (b_obj_dict*)GC(new_dict(vm));

    for(int i = 0; i < rc; i++)
    {
        dict_set_entry(vm, result, NUMBER_VAL(0), NIL_VAL);
    }

    // add first set of matches to response
    for(int i = 0; i < rc; i++)
    {
        b_obj_list* list = (b_obj_list*)GC(new_list(vm));
        PCRE2_SIZE substring_length = o_vector[2 * i + 1] - o_vector[2 * i];
        PCRE2_SPTR substring_start = subject + o_vector[2 * i];
        write_list(vm, list, GC_L_STRING((char*)substring_start, (int)substring_length));
        dict_set_entry(vm, result, NUMBER_VAL(i), OBJ_VAL(list));
    }

    if(name_count > 0)
    {
        PCRE2_SPTR tab_ptr;
        (void)pcre2_pattern_info(re, PCRE2_INFO_NAMETABLE, &name_table);
        (void)pcre2_pattern_info(re, PCRE2_INFO_NAMEENTRYSIZE, &name_entry_size);

        tab_ptr = name_table;

        for(int i = 0; i < (int)name_count; i++)
        {
            int n = (tab_ptr[0] << 8) | tab_ptr[1];

            int value_length = (int)(o_vector[2 * n + 1] - o_vector[2 * n]);
            int key_length = (int)name_entry_size - 3;

            char* _key = ALLOCATE(char, key_length + 1);
            char* _val = ALLOCATE(char, value_length + 1);

            sprintf(_key, "%*s", key_length, tab_ptr + 2);
            sprintf(_val, "%*s", value_length, subject + o_vector[2 * n]);

            while(isspace((unsigned char)*_key))
                _key++;

            b_obj_list* list = (b_obj_list*)GC(new_list(vm));
            write_list(vm, list, OBJ_VAL(GC(take_string(vm, _val, value_length))));

            dict_add_entry(vm, result, OBJ_VAL(GC(take_string(vm, _key, key_length))), OBJ_VAL(list));

            tab_ptr += name_entry_size;
        }
    }

    (void)pcre2_pattern_info(re, PCRE2_INFO_ALLOPTIONS, &option_bits);
    int utf8 = (option_bits & PCRE2_UTF) != 0;

    (void)pcre2_pattern_info(re, PCRE2_INFO_NEWLINE, &newline);
    int crlf_is_newline = newline == PCRE2_NEWLINE_ANY || newline == PCRE2_NEWLINE_CRLF || newline == PCRE2_NEWLINE_ANYCRLF;

    // find the other matches
    for(;;)
    {
        uint32_t options = 0;
        PCRE2_SIZE start_offset = o_vector[1];

        // if the previous match was for an empty string
        if(o_vector[0] == o_vector[1])
        {
            if(o_vector[0] == subject_length)
                break;
            options = PCRE2_NOTEMPTY_ATSTART | PCRE2_ANCHORED;
        }
        else
        {
            PCRE2_SIZE start_char = pcre2_get_startchar(match_data);
            if(start_offset > subject_length - 1)
            {
                break;
            }
            if(start_offset <= start_char)
            {
                if(start_char >= subject_length - 1)
                {
                    break;
                }
                start_offset = start_char + 1;
                if(utf8)
                {
                    for(; start_offset < subject_length; start_offset++)
                        if((subject[start_offset] & 0xc0) != 0x80)
                            break;
                }
            }
        }

        rc = pcre2_match(re, subject, subject_length, start_offset, options, match_data, NULL);

        if(rc == PCRE2_ERROR_NOMATCH)
        {
            if(options == 0)
                break;
            o_vector[1] = start_offset + 1;
            if(crlf_is_newline && start_offset < subject_length - 1 && subject[start_offset] == '\r' && subject[start_offset + 1] == '\n')
                o_vector[1] += 1;
            else if(utf8)
            {
                while(o_vector[1] < subject_length)
                {
                    if((subject[o_vector[1]] & 0xc0) != 0x80)
                        break;
                    o_vector[1] += 1;
                }
            }
            continue;
        }

        if(rc < 0 && rc != PCRE2_ERROR_PARTIAL)
        {
            pcre2_match_data_free(match_data);
            pcre2_code_free(re);
            REGEX_ERR("regular expression error %d", rc);
        }

        // REGEX_VECTOR_SIZE_WARNING();
        REGEX_ASSERTION_ERROR(re, match_data, o_vector);

        for(int i = 0; i < rc; i++)
        {
            PCRE2_SIZE substring_length = o_vector[2 * i + 1] - o_vector[2 * i];
            PCRE2_SPTR substring_start = subject + o_vector[2 * i];

            b_value vlist;
            if(dict_get_entry(result, NUMBER_VAL(i), &vlist))
            {
                write_list(vm, AS_LIST(vlist), GC_L_STRING((char*)substring_start, (int)substring_length));
            }
            else
            {
                b_obj_list* list = (b_obj_list*)GC(new_list(vm));
                write_list(vm, list, GC_L_STRING((char*)substring_start, (int)substring_length));
                dict_set_entry(vm, result, NUMBER_VAL(i), OBJ_VAL(list));
            }
        }

        if(name_count > 0)
        {
            PCRE2_SPTR tab_ptr;
            (void)pcre2_pattern_info(re, PCRE2_INFO_NAMETABLE, &name_table);
            (void)pcre2_pattern_info(re, PCRE2_INFO_NAMEENTRYSIZE, &name_entry_size);

            tab_ptr = name_table;

            for(int i = 0; i < (int)name_count; i++)
            {
                int n = (tab_ptr[0] << 8) | tab_ptr[1];

                int value_length = (int)(o_vector[2 * n + 1] - o_vector[2 * n]);
                int key_length = (int)name_entry_size - 3;

                char* _key = ALLOCATE(char, key_length + 1);
                char* _val = ALLOCATE(char, value_length + 1);

                sprintf(_key, "%*s", key_length, tab_ptr + 2);
                sprintf(_val, "%*s", value_length, subject + o_vector[2 * n]);

                while(isspace((unsigned char)*_key))
                    _key++;

                b_obj_string* name = (b_obj_string*)GC(take_string(vm, _key, key_length));
                b_obj_string* value = (b_obj_string*)GC(take_string(vm, _val, value_length));

                b_value nlist;
                if(dict_get_entry(result, OBJ_VAL(name), &nlist))
                {
                    write_list(vm, AS_LIST(nlist), OBJ_VAL(value));
                }
                else
                {
                    b_obj_list* list = (b_obj_list*)GC(new_list(vm));
                    write_list(vm, list, OBJ_VAL(value));
                    dict_set_entry(vm, result, OBJ_VAL(name), OBJ_VAL(list));
                }

                tab_ptr += name_entry_size;
            }
        }
    }

    pcre2_match_data_free(match_data);
    pcre2_code_free(re);

    RETURN_OBJ(result);
}

bool native_method_stringreplace(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(replace, 2);
    ENFORCE_ARG_TYPE(replace, 0, IS_STRING);
    ENFORCE_ARG_TYPE(replace, 1, IS_STRING);

    b_obj_string* string = AS_STRING(METHOD_OBJECT);
    b_obj_string* substr = AS_STRING(args[0]);
    b_obj_string* rep_substr = AS_STRING(args[1]);

    if(string->length == 0 && substr->length == 0)
    {
        RETURN_TRUE;
    }
    else if(string->length == 0 || substr->length == 0)
    {
        RETURN_FALSE;
    }

    GET_REGEX_COMPILE_OPTIONS(substr, false);
    char* real_regex = remove_regex_delimiter(vm, substr);

    PCRE2_SPTR input = (PCRE2_SPTR)string->chars;
    PCRE2_SPTR pattern = (PCRE2_SPTR)real_regex;
    PCRE2_SPTR replacement = (PCRE2_SPTR)rep_substr->chars;

    int result, error_number;
    PCRE2_SIZE error_offset;

    pcre2_code* re = pcre2_compile(pattern, PCRE2_ZERO_TERMINATED, compile_options & PCRE2_MULTILINE, &error_number, &error_offset, 0);
    free(real_regex);

    REGEX_COMPILATION_ERROR(re, error_number, error_offset);

    pcre2_match_context* match_context = pcre2_match_context_create(0);

    PCRE2_SIZE output_length = 0;
    result = pcre2_substitute(re, input, PCRE2_ZERO_TERMINATED, 0, PCRE2_SUBSTITUTE_GLOBAL | PCRE2_SUBSTITUTE_OVERFLOW_LENGTH, 0, match_context, replacement,
                              PCRE2_ZERO_TERMINATED, 0, &output_length);

    if(result < 0 && result != PCRE2_ERROR_NOMEMORY)
    {
        REGEX_ERR("regular expression post-compilation failed for replacement", result);
    }

    PCRE2_UCHAR* output_buffer = ALLOCATE(PCRE2_UCHAR, output_length);

    result = pcre2_substitute(re, input, PCRE2_ZERO_TERMINATED, 0, PCRE2_SUBSTITUTE_GLOBAL | PCRE2_SUBSTITUTE_UNSET_EMPTY, 0, match_context, replacement,
                              PCRE2_ZERO_TERMINATED, output_buffer, &output_length);

    if(result < 0 && result != PCRE2_ERROR_NOMEMORY)
    {
        REGEX_ERR("regular expression error at replacement time", result);
    }

    b_obj_string* response = take_string(vm, (char*)output_buffer, (int)output_length);

    pcre2_match_context_free(match_context);
    pcre2_code_free(re);

    RETURN_OBJ(response);
}

bool native_method_stringto_bytes(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(to_bytes, 0);
    b_obj_string* string = AS_STRING(METHOD_OBJECT);
    RETURN_OBJ(copy_bytes(vm, (unsigned char*)string->chars, string->length));
}

bool native_method_string__iter__(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(__iter__, 1);
    ENFORCE_ARG_TYPE(__iter__, 0, IS_NUMBER);

    b_obj_string* string = AS_STRING(METHOD_OBJECT);
    int length = string->is_ascii ? string->length : string->utf8_length;
    int index = AS_NUMBER(args[0]);

    if(index > -1 && index < length)
    {
        int start = index, end = index + 1;
        if(!string->is_ascii)
        {
            utf8slice(string->chars, &start, &end);
        }

        RETURN_L_STRING(string->chars + start, (int)(end - start));
    }

    RETURN_NIL;
}

bool native_method_string__itern__(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(__itern__, 1);
    b_obj_string* string = AS_STRING(METHOD_OBJECT);
    int length = string->is_ascii ? string->length : string->utf8_length;

    if(IS_NIL(args[0]))
    {
        if(length == 0)
        {
            RETURN_FALSE;
        }
        RETURN_NUMBER(0);
    }

    if(!IS_NUMBER(args[0]))
    {
        RETURN_ERROR("bytes are numerically indexed");
    }

    int index = AS_NUMBER(args[0]);
    if(index < length - 1)
    {
        RETURN_NUMBER((double)index + 1);
    }

    RETURN_NIL;
}

#include <ctype.h>
#include <string.h>

bool cfn_bytes(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(bytes, 1);
    if(IS_NUMBER(args[0]))
    {
        RETURN_OBJ(new_bytes(vm, (int)AS_NUMBER(args[0])));
    }
    else if(IS_LIST(args[0]))
    {
        b_obj_list* list = AS_LIST(args[0]);
        b_obj_bytes* bytes = (b_obj_bytes*)GC(new_bytes(vm, list->items.count));

        for(int i = 0; i < list->items.count; i++)
        {
            if(IS_NUMBER(list->items.values[i]))
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

bool native_method_byteslength(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(length, 0);
    RETURN_NUMBER(AS_BYTES(METHOD_OBJECT)->bytes.count);
}

bool native_method_bytesappend(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(append, 1);

    if(IS_NUMBER(args[0]))
    {
        int byte = (int)AS_NUMBER(args[0]);
        if(byte < 0 || byte > 255)
        {
            RETURN_ERROR("invalid byte. bytes range from 0 to 255");
        }

        // append here...
        b_obj_bytes* bytes = AS_BYTES(METHOD_OBJECT);
        int old_count = bytes->bytes.count;
        bytes->bytes.count++;
        bytes->bytes.bytes = GROW_ARRAY(unsigned char, bytes->bytes.bytes, old_count, bytes->bytes.count);
        bytes->bytes.bytes[bytes->bytes.count - 1] = (unsigned char)byte;
        RETURN;
    }
    else if(IS_LIST(args[0]))
    {
        b_obj_list* list = AS_LIST(args[0]);
        if(list->items.count > 0)
        {
            // append here...
            b_obj_bytes* bytes = AS_BYTES(METHOD_OBJECT);
            bytes->bytes.bytes = GROW_ARRAY(unsigned char, bytes->bytes.bytes, bytes->bytes.count, (size_t)bytes->bytes.count + (size_t)list->items.count);
            if(bytes->bytes.bytes == NULL)
            {
                RETURN_ERROR("out of memory");
            }

            for(int i = 0; i < list->items.count; i++)
            {
                if(!IS_NUMBER(list->items.values[i]))
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
        RETURN;
    }

    RETURN_ERROR("bytes can only append a byte or a list of bytes");
}

bool native_method_bytesclone(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(clone, 0);
    b_obj_bytes* bytes = AS_BYTES(METHOD_OBJECT);
    b_obj_bytes* n_bytes = (b_obj_bytes*)GC(new_bytes(vm, bytes->bytes.count));

    memcpy(n_bytes->bytes.bytes, bytes->bytes.bytes, bytes->bytes.count);

    RETURN_OBJ(n_bytes);
}

bool native_method_bytesextend(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(extend, 1);
    ENFORCE_ARG_TYPE(extend, 0, IS_BYTES);
    b_obj_bytes* bytes = AS_BYTES(METHOD_OBJECT);
    b_obj_bytes* n_bytes = AS_BYTES(args[0]);

    bytes->bytes.bytes = GROW_ARRAY(unsigned char, bytes->bytes.bytes, bytes->bytes.count, bytes->bytes.count + n_bytes->bytes.count);
    if(bytes->bytes.bytes == NULL)
    {
        RETURN_ERROR("out of memory");
    }

    memcpy(bytes->bytes.bytes + bytes->bytes.count, n_bytes->bytes.bytes, n_bytes->bytes.count);
    bytes->bytes.count += n_bytes->bytes.count;
    RETURN_OBJ(bytes);
}

bool native_method_bytespop(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(pop, 0);
    b_obj_bytes* bytes = AS_BYTES(METHOD_OBJECT);
    unsigned char c = bytes->bytes.bytes[bytes->bytes.count - 1];
    bytes->bytes.count--;
    RETURN_NUMBER((double)((int)c));
}

bool native_method_bytesremove(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(remove, 1);
    ENFORCE_ARG_TYPE(remove, 0, IS_NUMBER);

    b_obj_bytes* bytes = AS_BYTES(METHOD_OBJECT);
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

bool native_method_bytesreverse(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(reverse, 0);
    b_obj_bytes* bytes = AS_BYTES(METHOD_OBJECT);

    b_obj_bytes* n_bytes = (b_obj_bytes*)GC(new_bytes(vm, bytes->bytes.count));

    for(int i = 0; i < bytes->bytes.count; i++)
    {
        n_bytes->bytes.bytes[i] = bytes->bytes.bytes[bytes->bytes.count - i - 1];
    }

    RETURN_OBJ(n_bytes);
}

bool native_method_bytessplit(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(split, 1);
    ENFORCE_ARG_TYPE(split, 0, IS_BYTES);

    b_byte_arr object = AS_BYTES(METHOD_OBJECT)->bytes;
    b_byte_arr delimeter = AS_BYTES(args[0])->bytes;

    if(object.count == 0 || delimeter.count > object.count)
        RETURN_OBJ(new_list(vm));

    b_obj_list* list = (b_obj_list*)GC(new_list(vm));

    // main work here...
    if(delimeter.count > 0)
    {
        int start = 0;
        for(int i = 0; i <= object.count; i++)
        {
            // match found.
            if(memcmp(object.bytes + i, delimeter.bytes, delimeter.count) == 0 || i == object.count)
            {
                b_obj_bytes* bytes = (b_obj_bytes*)GC(new_bytes(vm, i - start));
                memcpy(bytes->bytes.bytes, object.bytes + start, i - start);
                write_list(vm, list, OBJ_VAL(bytes));
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
            b_obj_bytes* bytes = (b_obj_bytes*)GC(new_bytes(vm, 1));
            memcpy(bytes->bytes.bytes, object.bytes + i, 1);
            write_list(vm, list, OBJ_VAL(bytes));
        }
    }

    RETURN_OBJ(list);
}

bool native_method_bytesfirst(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(first, 0);
    RETURN_NUMBER((double)((int)AS_BYTES(METHOD_OBJECT)->bytes.bytes[0]));
}

bool native_method_byteslast(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(first, 0);
    b_obj_bytes* bytes = AS_BYTES(METHOD_OBJECT);
    RETURN_NUMBER((double)((int)bytes->bytes.bytes[bytes->bytes.count - 1]));
}

bool native_method_bytesget(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(get, 1);
    ENFORCE_ARG_TYPE(get, 0, IS_NUMBER);

    b_obj_bytes* bytes = AS_BYTES(METHOD_OBJECT);
    int index = AS_NUMBER(args[0]);
    if(index < 0 || index >= bytes->bytes.count)
    {
        RETURN_ERROR("bytes index %d out of range", index);
    }

    RETURN_NUMBER((double)((int)bytes->bytes.bytes[index]));
}

bool native_method_bytesis_alpha(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(is_alpha, 0);
    b_obj_bytes* bytes = AS_BYTES(METHOD_OBJECT);

    for(int i = 0; i < bytes->bytes.count; i++)
    {
        if(!isalpha(bytes->bytes.bytes[i]))
        {
            RETURN_FALSE;
        }
    }
    RETURN_TRUE;
}

bool native_method_bytesis_alnum(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(is_alnum, 0);
    b_obj_bytes* bytes = AS_BYTES(METHOD_OBJECT);

    for(int i = 0; i < bytes->bytes.count; i++)
    {
        if(!isalnum(bytes->bytes.bytes[i]))
        {
            RETURN_FALSE;
        }
    }
    RETURN_TRUE;
}

bool native_method_bytesis_number(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(is_number, 0);
    b_obj_bytes* bytes = AS_BYTES(METHOD_OBJECT);

    for(int i = 0; i < bytes->bytes.count; i++)
    {
        if(!isdigit(bytes->bytes.bytes[i]))
        {
            RETURN_FALSE;
        }
    }
    RETURN_TRUE;
}

bool native_method_bytesis_lower(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(is_lower, 0);
    b_obj_bytes* bytes = AS_BYTES(METHOD_OBJECT);

    for(int i = 0; i < bytes->bytes.count; i++)
    {
        if(!islower(bytes->bytes.bytes[i]))
        {
            RETURN_FALSE;
        }
    }
    RETURN_TRUE;
}

bool native_method_bytesis_upper(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(is_upper, 0);
    b_obj_bytes* bytes = AS_BYTES(METHOD_OBJECT);

    for(int i = 0; i < bytes->bytes.count; i++)
    {
        if(!isupper(bytes->bytes.bytes[i]))
        {
            RETURN_FALSE;
        }
    }
    RETURN_TRUE;
}

bool native_method_bytesis_space(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(is_space, 0);
    b_obj_bytes* bytes = AS_BYTES(METHOD_OBJECT);

    for(int i = 0; i < bytes->bytes.count; i++)
    {
        if(!isspace(bytes->bytes.bytes[i]))
        {
            RETURN_FALSE;
        }
    }
    RETURN_TRUE;
}

bool native_method_bytesdispose(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(dispose, 0);
    b_obj_bytes* bytes = AS_BYTES(METHOD_OBJECT);
    free_byte_arr(vm, &bytes->bytes);
    RETURN;
}

bool native_method_bytesto_list(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(to_list, 0);
    b_obj_bytes* bytes = AS_BYTES(METHOD_OBJECT);
    b_obj_list* list = (b_obj_list*)GC(new_list(vm));

    for(int i = 0; i < bytes->bytes.count; i++)
    {
        write_list(vm, list, NUMBER_VAL((double)((int)bytes->bytes.bytes[i])));
    }

    RETURN_OBJ(list);
}

bool native_method_bytesto_string(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(to_string, 0);
    b_obj_bytes* bytes = AS_BYTES(METHOD_OBJECT);

    char* string = (char*)bytes->bytes.bytes;
    RETURN_L_STRING(string, bytes->bytes.count);
}

bool native_method_bytes__iter__(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(__iter__, 1);
    ENFORCE_ARG_TYPE(__iter__, 0, IS_NUMBER);

    b_obj_bytes* bytes = AS_BYTES(METHOD_OBJECT);

    int index = AS_NUMBER(args[0]);

    if(index > -1 && index < bytes->bytes.count)
    {
        RETURN_NUMBER((int)bytes->bytes.bytes[index]);
    }

    RETURN_NIL;
}

bool native_method_bytes__itern__(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(__itern__, 1);
    b_obj_bytes* bytes = AS_BYTES(METHOD_OBJECT);

    if(IS_NIL(args[0]))
    {
        if(bytes->bytes.count == 0)
            RETURN_FALSE;
        RETURN_NUMBER(0);
    }

    if(!IS_NUMBER(args[0]))
    {
        RETURN_ERROR("bytes are numerically indexed");
    }

    int index = AS_NUMBER(args[0]);
    if(index < bytes->bytes.count - 1)
    {
        RETURN_NUMBER((double)index + 1);
    }

    RETURN_NIL;
}


void init_table(b_table* table)
{
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void free_table(b_vm* vm, b_table* table)
{
    FREE_ARRAY(b_entry, table->entries, table->capacity);
    init_table(table);
}

void clean_free_table(b_vm* vm, b_table* table)
{
    for(int i = 0; i < table->capacity; i++)
    {
        b_entry* entry = &table->entries[i];

        if(entry != NULL)
        {
            /*
            if(IS_OBJ(entry->key))
            {
                free_object(vm, &AS_OBJ(entry->key));
            }
            */
            /*
            if(IS_OBJ(entry->value))
            {
                free_object(vm, &AS_OBJ(entry->value));
            }
            */
            
        }
    }

    FREE_ARRAY(b_entry, table->entries, table->capacity);
    init_table(table);
}

static b_entry* find_entry(b_entry* entries, int capacity, b_value key)
{
    uint32_t hash = hash_value(key);

#if defined(DEBUG_TABLE) && DEBUG_TABLE
    printf("looking for key ");
    print_value(key);
    printf(" with hash %u in table...\n", hash);
#endif

    uint32_t index = hash & (capacity - 1);
    b_entry* tombstone = NULL;

    for(;;)
    {
        b_entry* entry = &entries[index];

        if(IS_EMPTY(entry->key))
        {
            if(IS_NIL(entry->value))
            {
                // empty entry
                return tombstone != NULL ? tombstone : entry;
            }
            else
            {
                // we found a tombstone.
                if(tombstone == NULL)
                    tombstone = entry;
            }
        }
        else if(values_equal(key, entry->key))
        {
#if defined(DEBUG_TABLE) && DEBUG_TABLE
            printf("found entry for key ");
            print_value(key);
            printf(" with hash %u in table as ", hash);
            print_value(entry->value);
            printf("...\n");
#endif

            return entry;
        }

        index = (index + 1) & (capacity - 1);
    }
}

bool table_get(b_table* table, b_value key, b_value* value)
{
    if(table->count == 0 || table->entries == NULL)
        return false;

#if defined(DEBUG_TABLE) && DEBUG_TABLE
    printf("getting entry with hash %u...\n", hash_value(key));
#endif

    b_entry* entry = find_entry(table->entries, table->capacity, key);

    if(IS_EMPTY(entry->key) || IS_NIL(entry->key))
        return false;

#if defined(DEBUG_TABLE) && DEBUG_TABLE
    printf("found entry for hash %u == ", hash_value(entry->key));
    print_value(entry->value);
    printf("\n");
#endif

    *value = entry->value;
    return true;
}

static void adjust_capacity(b_vm* vm, b_table* table, int capacity)
{
    b_entry* entries = ALLOCATE(b_entry, capacity);
    for(int i = 0; i < capacity; i++)
    {
        entries[i].key = EMPTY_VAL;
        entries[i].value = NIL_VAL;
    }

    // repopulate buckets
    table->count = 0;
    for(int i = 0; i < table->capacity; i++)
    {
        b_entry* entry = &table->entries[i];
        if(IS_EMPTY(entry->key))
            continue;
        b_entry* dest = find_entry(entries, capacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        table->count++;
    }

    // free the old entries...
    FREE_ARRAY(b_entry, table->entries, table->capacity);

    table->entries = entries;
    table->capacity = capacity;
}

bool table_set(b_vm* vm, b_table* table, b_value key, b_value value)
{
    if(table->count + 1 > table->capacity * TABLE_MAX_LOAD)
    {
        int capacity = GROW_CAPACITY(table->capacity);
        adjust_capacity(vm, table, capacity);
    }

    b_entry* entry = find_entry(table->entries, table->capacity, key);

    bool is_new = IS_EMPTY(entry->key);

    if(is_new && IS_NIL(entry->value))
        table->count++;

    // overwrites existing entries.
    entry->key = key;
    entry->value = value;

    return is_new;
}

bool table_delete(b_table* table, b_value key)
{
    if(table->count == 0)
        return false;

    // find the entry
    b_entry* entry = find_entry(table->entries, table->capacity, key);
    if(IS_EMPTY(entry->key))
        return false;

    // place a tombstone in the entry.
    entry->key = EMPTY_VAL;
    entry->value = BOOL_VAL(true);

    return true;
}

void table_add_all(b_vm* vm, b_table* from, b_table* to)
{
    for(int i = 0; i < from->capacity; i++)
    {
        b_entry* entry = &from->entries[i];
        if(!IS_EMPTY(entry->key))
        {
            table_set(vm, to, entry->key, entry->value);
        }
    }
}

void table_copy(b_vm* vm, b_table* from, b_table* to)
{
    for(int i = 0; i < from->capacity; i++)
    {
        b_entry* entry = &from->entries[i];
        if(!IS_EMPTY(entry->key))
        {
            table_set(vm, to, entry->key, copy_value(vm, entry->value));
        }
    }
}

b_obj_string* table_find_string(b_table* table, const char* chars, int length, uint32_t hash)
{
    if(table->count == 0)
        return NULL;

    uint32_t index = hash & (table->capacity - 1);

    for(;;)
    {
        b_entry* entry = &table->entries[index];

        if(IS_EMPTY(entry->key))
        {
            /* // stop if we find an empty non-tombstone entry
      if (IS_NIL(entry->value)) */
            return NULL;
        }

        // if (IS_STRING(entry->key)) {
        b_obj_string* string = AS_STRING(entry->key);
        if(string->length == length && string->hash == hash && memcmp(string->chars, chars, length) == 0)
        {
            // we found it
            return string;
        }
        // }

        index = (index + 1) & (table->capacity - 1);
    }
}

b_value table_find_key(b_table* table, b_value value)
{
    for(int i = 0; i < table->capacity; i++)
    {
        b_entry* entry = &table->entries[i];
        if(!IS_NIL(entry->key) && !IS_EMPTY(entry->key))
        {
            if(values_equal(entry->value, value))
                return entry->key;
        }
    }
    return NIL_VAL;
}

void table_print(b_table* table)
{
    printf("<HashTable: {");
    for(int i = 0; i < table->capacity; i++)
    {
        b_entry* entry = &table->entries[i];
        if(!IS_EMPTY(entry->key))
        {
            print_value(entry->key);
            printf(": ");
            print_value(entry->value);
            if(i != table->capacity - 1)
            {
                printf(",");
            }
        }
    }
    printf("}>\n");
}

void mark_table(b_vm* vm, b_table* table)
{
    for(int i = 0; i < table->capacity; i++)
    {
        b_entry* entry = &table->entries[i];

        if(entry != NULL)
        {
            mark_value(vm, entry->key);
            mark_value(vm, entry->value);
        }
    }
}

void table_remove_whites(b_vm* vm, b_table* table)
{
    for(int i = 0; i < table->capacity; i++)
    {
        b_entry* entry = &table->entries[i];
        if(IS_OBJ(entry->key) && !AS_OBJ(entry->key)->mark)
        {
            table_delete(table, entry->key);
        }
    }
}

#define ENFORCE_VALID_DICT_KEY(name, index) \
    EXCLUDE_ARG_TYPE(name, IS_LIST, index); \
    EXCLUDE_ARG_TYPE(name, IS_DICT, index); \
    EXCLUDE_ARG_TYPE(name, IS_FILE, index);

void dict_add_entry(b_vm* vm, b_obj_dict* dict, b_value key, b_value value)
{
    write_value_arr(vm, &dict->names, key);
    table_set(vm, &dict->items, key, value);
}

bool dict_get_entry(b_obj_dict* dict, b_value key, b_value* value)
{
    /* // this will be easier to search than the entire tables
  // if the key doesn't exist.
  if (dict->names.count < (int)sizeof(uint8_t)) {
    int i;
    bool found = false;
    for (i = 0; i < dict->names.count; i++) {
      if (values_equal(dict->names.values[i], key)) {
        found = true;
        break;
      }
    }

    if (!found)
      return false;
  } */
    return table_get(&dict->items, key, value);
}

bool dict_set_entry(b_vm* vm, b_obj_dict* dict, b_value key, b_value value)
{
    b_value temp_value;
    if(!table_get(&dict->items, key, &temp_value))
    {
        write_value_arr(vm, &dict->names, key);// add key if it doesn't exist.
    }
    return table_set(vm, &dict->items, key, value);
}

bool bl_vmdo_dictgetindex(b_vm* vm, b_obj_dict* dict, bool will_assign)
{
    b_value index = peek(vm, 0);

    b_value result;
    if(dict_get_entry(dict, index, &result))
    {
        if(!will_assign)
        {
            pop_n(vm, 2);// we can safely get rid of the index from the stack
        }
        push(vm, result);
        return true;
    }

    pop_n(vm, 1);
    return bl_vm_throwexception(vm, false, "invalid index %s", value_to_string(vm, index));
}

void bl_vmdo_dictsetindex(b_vm* vm, b_obj_dict* dict, b_value index, b_value value)
{
    dict_set_entry(vm, dict, index, value);
    pop_n(vm, 3);// pop the value, index and dict out

    // leave the value on the stack for consumption
    // e.g. variable = dict[index] = 10
    push(vm, value);
}

bool native_method_dictlength(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(dictionary.length, 0);
    RETURN_NUMBER(AS_DICT(METHOD_OBJECT)->names.count);
}

bool native_method_dictadd(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(add, 2);
    ENFORCE_VALID_DICT_KEY(add, 0);

    b_obj_dict* dict = AS_DICT(METHOD_OBJECT);

    b_value temp_value;
    if(table_get(&dict->items, args[0], &temp_value))
    {
        RETURN_ERROR("duplicate key %s at add()", value_to_string(vm, args[0]));
    }

    dict_add_entry(vm, dict, args[0], args[1]);
    RETURN;
}

bool native_method_dictset(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(set, 2);
    ENFORCE_VALID_DICT_KEY(set, 0);

    b_obj_dict* dict = AS_DICT(METHOD_OBJECT);
    b_value value;
    if(!table_get(&dict->items, args[0], &value))
    {
        dict_add_entry(vm, dict, args[0], args[1]);
    }
    else
    {
        dict_set_entry(vm, dict, args[0], args[1]);
    }
    RETURN;
}

bool native_method_dictclear(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(dict, 0);

    b_obj_dict* dict = AS_DICT(METHOD_OBJECT);
    free_value_arr(vm, &dict->names);
    free_table(vm, &dict->items);
    RETURN;
}

bool native_method_dictclone(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(clone, 0);
    b_obj_dict* dict = AS_DICT(METHOD_OBJECT);
    b_obj_dict* n_dict = (b_obj_dict*)GC(new_dict(vm));

    table_add_all(vm, &dict->items, &n_dict->items);

    for(int i = 0; i < dict->names.count; i++)
    {
        write_value_arr(vm, &n_dict->names, dict->names.values[i]);
    }

    RETURN_OBJ(n_dict);
}

bool native_method_dictcompact(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(compact, 0);
    b_obj_dict* dict = AS_DICT(METHOD_OBJECT);
    b_obj_dict* n_dict = (b_obj_dict*)GC(new_dict(vm));

    for(int i = 0; i < dict->names.count; i++)
    {
        b_value tmp_value;
        table_get(&dict->items, dict->names.values[i], &tmp_value);
        if(!values_equal(tmp_value, NIL_VAL))
        {
            dict_add_entry(vm, n_dict, dict->names.values[i], tmp_value);
        }
    }

    RETURN_OBJ(n_dict);
}

bool native_method_dictcontains(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(contains, 1);
    ENFORCE_VALID_DICT_KEY(contains, 0);

    b_obj_dict* dict = AS_DICT(METHOD_OBJECT);
    b_value value;
    RETURN_BOOL(table_get(&dict->items, args[0], &value));
}

bool native_method_dictextend(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(extend, 1);
    ENFORCE_ARG_TYPE(extend, 0, IS_DICT);

    b_obj_dict* dict = AS_DICT(METHOD_OBJECT);
    b_obj_dict* dict_cpy = AS_DICT(args[0]);

    for(int i = 0; i < dict_cpy->names.count; i++)
    {
        write_value_arr(vm, &dict->names, dict_cpy->names.values[i]);
    }
    table_add_all(vm, &dict_cpy->items, &dict->items);
    RETURN;
}

bool native_method_dictget(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_RANGE(get, 1, 2);
    ENFORCE_VALID_DICT_KEY(get, 0);

    b_obj_dict* dict = AS_DICT(METHOD_OBJECT);
    b_value value;
    if(!dict_get_entry(dict, args[0], &value))
    {
        if(arg_count == 1)
        {
            RETURN_NIL;
        }
        else
        {
            RETURN_VALUE(args[1]);// return default
        }
    }

    RETURN_VALUE(value);
}

bool native_method_dictkeys(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(keys, 0);
    b_obj_dict* dict = AS_DICT(METHOD_OBJECT);
    b_obj_list* list = (b_obj_list*)GC(new_list(vm));
    for(int i = 0; i < dict->names.count; i++)
    {
        write_list(vm, list, dict->names.values[i]);
    }
    RETURN_OBJ(list);
}

bool native_method_dictvalues(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(values, 0);
    b_obj_dict* dict = AS_DICT(METHOD_OBJECT);
    b_obj_list* list = (b_obj_list*)GC(new_list(vm));
    for(int i = 0; i < dict->names.count; i++)
    {
        b_value tmp_value;
        dict_get_entry(dict, dict->names.values[i], &tmp_value);
        write_list(vm, list, tmp_value);
    }
    RETURN_OBJ(list);
}

bool native_method_dictremove(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(remove, 1);
    ENFORCE_VALID_DICT_KEY(remove, 0);

    b_obj_dict* dict = AS_DICT(METHOD_OBJECT);
    b_value value;
    if(table_get(&dict->items, args[0], &value))
    {
        table_delete(&dict->items, args[0]);
        int index = -1;
        for(int i = 0; i < dict->names.count; i++)
        {
            if(values_equal(dict->names.values[i], args[0]))
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
    RETURN_NIL;
}

bool native_method_dictis_empty(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(is_empty, 0);
    RETURN_BOOL(AS_DICT(METHOD_OBJECT)->names.count == 0);
}

bool native_method_dictfind_key(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(find_key, 1);
    RETURN_VALUE(table_find_key(&AS_DICT(METHOD_OBJECT)->items, args[0]));
}

bool native_method_dictto_list(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(to_list, 0);

    b_obj_dict* dict = AS_DICT(METHOD_OBJECT);
    b_obj_list* name_list = (b_obj_list*)GC(new_list(vm));
    b_obj_list* value_list = (b_obj_list*)GC(new_list(vm));
    for(int i = 0; i < dict->names.count; i++)
    {
        write_list(vm, name_list, dict->names.values[i]);
        b_value value;
        if(table_get(&dict->items, dict->names.values[i], &value))
        {
            write_list(vm, value_list, value);
        }
        else
        {// theoretically impossible
            write_list(vm, value_list, NIL_VAL);
        }
    }

    b_obj_list* list = (b_obj_list*)GC(new_list(vm));
    write_list(vm, list, OBJ_VAL(name_list));
    write_list(vm, list, OBJ_VAL(value_list));

    RETURN_OBJ(list);
}

bool native_method_dict__iter__(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(__iter__, 1);
    b_obj_dict* dict = AS_DICT(METHOD_OBJECT);

    b_value result;
    if(table_get(&dict->items, args[0], &result))
    {
        RETURN_VALUE(result);
    }

    RETURN_NIL;
}

bool native_method_dict__itern__(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(__itern__, 1);
    b_obj_dict* dict = AS_DICT(METHOD_OBJECT);

    if(IS_NIL(args[0]))
    {
        if(dict->names.count == 0)
            RETURN_FALSE;
        RETURN_VALUE(dict->names.values[0]);
    }
    for(int i = 0; i < dict->names.count; i++)
    {
        if(values_equal(args[0], dict->names.values[i]) && (i + 1) < dict->names.count)
        {
            RETURN_VALUE(dict->names.values[i + 1]);
        }
    }

    RETURN_NIL;
}

#undef ENFORCE_VALID_DICT_KEY

void write_list(b_vm* vm, b_obj_list* list, b_value value)
{
    write_value_arr(vm, &list->items, value);
}

b_obj_list* copy_list(b_vm* vm, b_obj_list* list, int start, int length)
{
    b_obj_list* _list = (b_obj_list*)GC(new_list(vm));

    for(int i = 0; i < list->items.count; i++)
    {
        write_list(vm, _list, list->items.values[i]);
    }

    return _list;
}

bool native_method_listlength(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(length, 0);
    RETURN_NUMBER(AS_LIST(METHOD_OBJECT)->items.count);
}

bool native_method_listappend(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(append, 1);
    write_list(vm, AS_LIST(METHOD_OBJECT), args[0]);
    RETURN;
}

bool native_method_listclear(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(clear, 0);
    free_value_arr(vm, &AS_LIST(METHOD_OBJECT)->items);
    RETURN;
}

bool native_method_listclone(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(clone, 0);
    b_obj_list* list = AS_LIST(METHOD_OBJECT);
    RETURN_OBJ(copy_list(vm, list, 0, list->items.count));
}

bool native_method_listcount(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(count, 1);
    b_obj_list* list = AS_LIST(METHOD_OBJECT);

    int count = 0;
    for(int i = 0; i < list->items.count; i++)
    {
        if(values_equal(list->items.values[i], args[0]))
            count++;
    }

    RETURN_NUMBER(count);
}

bool native_method_listextend(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(extend, 1);
    ENFORCE_ARG_TYPE(extend, 0, IS_LIST);

    b_obj_list* list = AS_LIST(METHOD_OBJECT);
    b_obj_list* list2 = AS_LIST(args[0]);

    for(int i = 0; i < list2->items.count; i++)
    {
        write_list(vm, list, list2->items.values[i]);
    }

    RETURN;
}

bool native_method_listindex_of(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(index_of, 1);

    b_obj_list* list = AS_LIST(METHOD_OBJECT);

    for(int i = 0; i < list->items.count; i++)
    {
        if(values_equal(list->items.values[i], args[0]))
        {
            RETURN_NUMBER(i);
        }
    }

    RETURN_NUMBER(-1);
}

bool native_method_listinsert(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(insert, 2);
    ENFORCE_ARG_TYPE(insert, 1, IS_NUMBER);

    b_obj_list* list = AS_LIST(METHOD_OBJECT);
    int index = (int)AS_NUMBER(args[1]);

    insert_value_arr(vm, &list->items, args[0], index);
    RETURN;
}

bool native_method_listpop(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(pop, 0);

    b_obj_list* list = AS_LIST(METHOD_OBJECT);
    if(list->items.count > 0)
    {
        b_value value = list->items.values[list->items.count - 1];// value to pop
        list->items.count--;
        RETURN_VALUE(value);
    }
    RETURN_NIL;
}

bool native_method_listshift(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_RANGE(shift, 0, 1);

    int count = 1;
    if(arg_count == 1)
    {
        ENFORCE_ARG_TYPE(shift, 0, IS_NUMBER);
        count = AS_NUMBER(args[0]);
    }

    b_obj_list* list = AS_LIST(METHOD_OBJECT);
    if(count >= list->items.count || list->items.count == 1)
    {
        list->items.count = 0;
        RETURN_NIL;
    }
    else if(count > 0)
    {
        b_obj_list* n_list = (b_obj_list*)GC(new_list(vm));
        for(int i = 0; i < count; i++)
        {
            write_list(vm, n_list, list->items.values[0]);
            for(int j = 0; j < list->items.count; j++)
            {
                list->items.values[j] = list->items.values[j + 1];
            }
            list->items.count -= 1;
        }

        if(count == 1)
        {
            RETURN_VALUE(n_list->items.values[0]);
        }
        else
        {
            RETURN_OBJ(n_list);
        }
    }
    RETURN_NIL;
}

bool native_method_listremove_at(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(remove_at, 1);
    ENFORCE_ARG_TYPE(remove_at, 0, IS_NUMBER);

    b_obj_list* list = AS_LIST(METHOD_OBJECT);
    int index = AS_NUMBER(args[0]);
    if(index < 0 || index >= list->items.count)
    {
        RETURN_ERROR("list index %d out of range at remove_at()", index);
    }

    b_value value = list->items.values[index];
    for(int i = index; i < list->items.count; i++)
    {
        list->items.values[i] = list->items.values[i + 1];
    }
    list->items.count--;
    RETURN_VALUE(value);
}

bool native_method_listremove(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(remove, 1);

    b_obj_list* list = AS_LIST(METHOD_OBJECT);
    int index = -1;
    for(int i = 0; i < list->items.count; i++)
    {
        if(values_equal(list->items.values[i], args[0]))
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
    RETURN;
}

bool native_method_listreverse(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(reverse, 0);

    b_obj_list* list = AS_LIST(METHOD_OBJECT);
    b_obj_list* nlist = (b_obj_list*)GC(new_list(vm));

    /*// in-place reversal
  int start = 0, end = list->items.count - 1;
  while (start < end) {
    b_value temp = list->items.values[start];
    list->items.values[start] = list->items.values[end];
    list->items.values[end] = temp;
    start++;
    end--;
  }*/

    for(int i = list->items.count - 1; i >= 0; i--)
    {
        write_list(vm, nlist, list->items.values[i]);
    }

    RETURN_OBJ(nlist);
}

bool native_method_listsort(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(sort, 0);

    b_obj_list* list = AS_LIST(METHOD_OBJECT);
    sort_values(list->items.values, list->items.count);
    RETURN;
}

bool native_method_listcontains(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(contains, 1);

    b_obj_list* list = AS_LIST(METHOD_OBJECT);

    for(int i = 0; i < list->items.count; i++)
    {
        if(values_equal(args[0], list->items.values[i]))
        {
            RETURN_TRUE;
        }
    }
    RETURN_FALSE;
}

bool native_method_listdelete(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_RANGE(delete, 1, 2);
    ENFORCE_ARG_TYPE(delete, 0, IS_NUMBER);

    int lower_index = AS_NUMBER(args[0]);
    int upper_index = lower_index;

    if(arg_count == 2)
    {
        ENFORCE_ARG_TYPE(delete, 1, IS_NUMBER);
        upper_index = AS_NUMBER(args[1]);
    }

    b_obj_list* list = AS_LIST(METHOD_OBJECT);

    if(lower_index < 0 || lower_index >= list->items.count)
    {
        RETURN_ERROR("list index %d out of range at delete()", lower_index);
    }
    else if(upper_index < lower_index || upper_index >= list->items.count)
    {
        RETURN_ERROR("invalid upper limit %d at delete()", upper_index);
    }

    for(int i = 0; i < list->items.count - upper_index; i++)
    {
        list->items.values[lower_index + i] = list->items.values[i + upper_index + 1];
    }
    list->items.count -= upper_index - lower_index + 1;
    RETURN_NUMBER((double)upper_index - (double)lower_index + 1);
}

bool native_method_listfirst(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(first, 0);
    b_obj_list* list = AS_LIST(METHOD_OBJECT);
    if(list->items.count > 0)
    {
        RETURN_VALUE(list->items.values[0]);
    }
    else
    {
        RETURN_NIL;
    }
}

bool native_method_listlast(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(last, 0);
    b_obj_list* list = AS_LIST(METHOD_OBJECT);
    if(list->items.count > 0)
    {
        RETURN_VALUE(list->items.values[list->items.count - 1]);
    }
    else
    {
        RETURN_NIL;
    }
}

bool native_method_listis_empty(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(is_empty, 0);
    RETURN_BOOL(AS_LIST(METHOD_OBJECT)->items.count == 0);
}

bool native_method_listtake(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(take, 1);
    ENFORCE_ARG_TYPE(take, 0, IS_NUMBER);

    b_obj_list* list = AS_LIST(METHOD_OBJECT);
    int count = AS_NUMBER(args[0]);
    if(count < 0)
        count = list->items.count + count;

    if(list->items.count < count)
    {
        RETURN_OBJ(copy_list(vm, list, 0, list->items.count));
    }

    RETURN_OBJ(copy_list(vm, list, 0, count));
}

bool native_method_listget(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(get, 1);
    ENFORCE_ARG_TYPE(get, 0, IS_NUMBER);

    b_obj_list* list = AS_LIST(METHOD_OBJECT);
    int index = AS_NUMBER(args[0]);
    if(index < 0 || index >= list->items.count)
    {
        RETURN_ERROR("list index %d out of range at get()", index);
    }

    RETURN_VALUE(list->items.values[index]);
}

bool native_method_listcompact(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(compact, 0);

    b_obj_list* list = AS_LIST(METHOD_OBJECT);
    b_obj_list* n_list = (b_obj_list*)GC(new_list(vm));

    for(int i = 0; i < list->items.count; i++)
    {
        if(!values_equal(list->items.values[i], NIL_VAL))
        {
            write_list(vm, n_list, list->items.values[i]);
        }
    }

    RETURN_OBJ(n_list);
}

bool native_method_listunique(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(unique, 0);

    b_obj_list* list = AS_LIST(METHOD_OBJECT);
    b_obj_list* n_list = (b_obj_list*)GC(new_list(vm));

    for(int i = 0; i < list->items.count; i++)
    {
        bool found = false;
        for(int j = 0; j < n_list->items.count; j++)
        {
            if(values_equal(n_list->items.values[j], list->items.values[i]))
            {
                found = true;
                continue;
            }
        }

        if(!found)
        {
            write_list(vm, n_list, list->items.values[i]);
        }
    }

    RETURN_OBJ(n_list);
}

bool native_method_listzip(b_vm* vm, int arg_count, b_value* args)
{
    b_obj_list* list = AS_LIST(METHOD_OBJECT);
    b_obj_list* n_list = (b_obj_list*)GC(new_list(vm));

    b_obj_list** arg_list = ALLOCATE(b_obj_list*, arg_count);

    for(int i = 0; i < arg_count; i++)
    {
        ENFORCE_ARG_TYPE(zip, i, IS_LIST);
        arg_list[i] = AS_LIST(args[i]);
    }

    for(int i = 0; i < list->items.count; i++)
    {
        b_obj_list* a_list = (b_obj_list*)GC(new_list(vm));
        write_list(vm, a_list, list->items.values[i]);// item of main list

        for(int j = 0; j < arg_count; j++)
        {// item of argument lists
            if(i < arg_list[j]->items.count)
            {
                write_list(vm, a_list, arg_list[j]->items.values[i]);
            }
            else
            {
                write_list(vm, a_list, NIL_VAL);
            }
        }

        write_list(vm, n_list, OBJ_VAL(a_list));
    }

    RETURN_OBJ(n_list);
}

bool native_method_listto_dict(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(to_dict, 0);

    b_obj_dict* dict = (b_obj_dict*)GC(new_dict(vm));
    b_obj_list* list = AS_LIST(METHOD_OBJECT);
    for(int i = 0; i < list->items.count; i++)
    {
        dict_set_entry(vm, dict, NUMBER_VAL(i), list->items.values[i]);
    }
    RETURN_OBJ(dict);
}

bool native_method_list__iter__(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(__iter__, 1);
    ENFORCE_ARG_TYPE(__iter__, 0, IS_NUMBER);

    b_obj_list* list = AS_LIST(METHOD_OBJECT);

    int index = AS_NUMBER(args[0]);

    if(index > -1 && index < list->items.count)
    {
        RETURN_VALUE(list->items.values[index]);
    }

    RETURN_NIL;
}

bool native_method_list__itern__(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(__itern__, 1);
    b_obj_list* list = AS_LIST(METHOD_OBJECT);

    if(IS_NIL(args[0]))
    {
        if(list->items.count == 0)
        {
            RETURN_FALSE;
        }
        RETURN_NUMBER(0);
    }

    if(!IS_NUMBER(args[0]))
    {
        RETURN_ERROR("lists are numerically indexed");
    }

    int index = AS_NUMBER(args[0]);
    if(index < list->items.count - 1)
    {
        RETURN_NUMBER((double)index + 1);
    }

    RETURN_NIL;
}

void init_value_arr(b_value_arr* array)
{
    array->capacity = 0;
    array->count = 0;
    array->values = NULL;
}

void init_byte_arr(b_vm* vm, b_byte_arr* array, int length)
{
    array->count = length;
    array->bytes = (unsigned char*)calloc(length, sizeof(unsigned char));
    vm->bytes_allocated += sizeof(unsigned char) * length;
}

void write_value_arr(b_vm* vm, b_value_arr* array, b_value value)
{
    if(array->capacity < array->count + 1)
    {
        int old_capacity = array->capacity;
        array->capacity = GROW_CAPACITY(old_capacity);
        array->values = GROW_ARRAY(b_value, array->values, old_capacity, array->capacity);
    }

    array->values[array->count] = value;
    array->count++;
}

void insert_value_arr(b_vm* vm, b_value_arr* array, b_value value, int index)
{
    if(array->capacity <= index)
    {
        array->capacity = GROW_CAPACITY(index);
        array->values = GROW_ARRAY(b_value, array->values, array->count, array->capacity);
    }
    else if(array->capacity < array->count + 2)
    {
        int capacity = array->capacity;
        array->capacity = GROW_CAPACITY(capacity);
        array->values = GROW_ARRAY(b_value, array->values, capacity, array->capacity);
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

void free_value_arr(b_vm* vm, b_value_arr* array)
{
    FREE_ARRAY(b_value, array->values, array->capacity);
    init_value_arr(array);
}

void free_byte_arr(b_vm* vm, b_byte_arr* array)
{
    if(array && array->count > 0)
    {
        FREE_ARRAY(unsigned char, array->bytes, array->count);
        array->count = 0;
        array->bytes = NULL;
    }
}

static void do_print_value(b_value value, bool fix_string)
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
            print_object(value, fix_string);
            break;

        default:
            break;
    }
}


void print_value(b_value value)
{
    do_print_value(value, false);
}

void echo_value(b_value value)
{
    do_print_value(value, true);
}


static char* number_to_string(b_vm* vm, double number)
{
    int length = snprintf(NULL, 0, NUMBER_FORMAT, number);
    char* num_str = ALLOCATE(char, length + 1);
    if(num_str != NULL)
    {
        sprintf(num_str, NUMBER_FORMAT, number);
        return num_str;
    }
    return "";
}

char* value_to_string(b_vm* vm, b_value value)
{
    switch(value.type)
    {
        case VAL_NIL:
            return "nil";
        case VAL_BOOL:
            return AS_BOOL(value) ? "true" : "false";
        case VAL_NUMBER:
            return number_to_string(vm, AS_NUMBER(value));
        case VAL_OBJ:
            return object_to_string(vm, value);

        default:
            return "";
    }
}

const char* value_type(b_value value)
{
    if(IS_EMPTY(value))
        return "empty";
    if(IS_NIL(value))
        return "nil";
    else if(IS_BOOL(value))
        return "boolean";
    else if(IS_NUMBER(value))
        return "number";
    else if(IS_OBJ(value))
        return object_type(AS_OBJ(value));
    else
        return "unknown";
}

bool values_equal(b_value a, b_value b)
{
    if(a.type != b.type)
        return false;

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

static uint32_t hash_bits(uint64_t hash)
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

uint32_t hash_double(double value)
{
    b_double_union bits;
    bits.num = value;
    return hash_bits(bits.bits);
}

uint32_t hash_string(const char* key, int length)
{

    uint32_t hash = 2166136261u;
    const char* be = key + length;

    while(key < be)
    {
        hash = (hash ^ *key++) * 16777619;
    }

    return hash;
    // return siphash24(127, 255, key, length);
}

// Generates a hash code for [object].
static uint32_t hash_object(b_obj* object)
{
    switch(object->type)
    {
        case OBJ_CLASS:
            // Classes just use their name.
            return ((b_obj_class*)object)->name->hash;

            // Allow bare (non-closure) functions so that we can use a map to find
            // existing constants in a function's constant table. This is only used
            // internally. Since user code never sees a non-closure function, they
            // cannot use them as map keys.
        case OBJ_FUNCTION:
        {
            b_obj_func* fn = (b_obj_func*)object;
            return hash_double(fn->arity) ^ hash_double(fn->blob.count);
        }

        case OBJ_STRING:
            return ((b_obj_string*)object)->hash;

        case OBJ_BYTES:
        {
            b_obj_bytes* bytes = ((b_obj_bytes*)object);
            return hash_string((const char*)bytes->bytes.bytes, bytes->bytes.count);
        }

        default:
            return 0;
    }
}

uint32_t hash_value(b_value value)
{
    switch(value.type)
    {
        case VAL_BOOL:
            return AS_BOOL(value) ? 3 : 5;

        case VAL_NIL:
            return 7;

        case VAL_NUMBER:
            return hash_double(AS_NUMBER(value));

        case VAL_OBJ:
            return hash_object(AS_OBJ(value));

        default:// VAL_EMPTY
            return 0;
    }
}

/**
 * returns the greater of the two values.
 * this function encapsulates Blade's object hierarchy
 */
static b_value find_max_value(b_value a, b_value b)
{
    if(IS_NIL(a))
    {
        return b;
    }
    else if(IS_BOOL(a))
    {
        if(IS_NIL(b) || (IS_BOOL(b) && AS_BOOL(b) == false))
            return a;// only nil, false and false are lower than numbers
        else
            return b;
    }
    else if(IS_NUMBER(a))
    {
        if(IS_NIL(b) || IS_BOOL(b))
            return a;
        else if(IS_NUMBER(b))
            return AS_NUMBER(a) >= AS_NUMBER(b) ? a : b;
        else
            return b;// every other thing is greater than a number
    }
    else if(IS_OBJ(a))
    {
        if(IS_STRING(a) && IS_STRING(b))
        {
            return strcmp(AS_C_STRING(a), AS_C_STRING(b)) >= 0 ? a : b;
        }
        else if(IS_FUNCTION(a) && IS_FUNCTION(b))
        {
            return AS_FUNCTION(a)->arity >= AS_FUNCTION(b)->arity ? a : b;
        }
        else if(IS_CLOSURE(a) && IS_CLOSURE(b))
        {
            return AS_CLOSURE(a)->fnptr->arity >= AS_CLOSURE(b)->fnptr->arity ? a : b;
        }
        else if(IS_RANGE(a) && IS_RANGE(b))
        {
            return AS_RANGE(a)->lower >= AS_RANGE(b)->lower ? a : b;
        }
        else if(IS_CLASS(a) && IS_CLASS(b))
        {
            return AS_CLASS(a)->methods.count >= AS_CLASS(b)->methods.count ? a : b;
        }
        else if(IS_LIST(a) && IS_LIST(b))
        {
            return AS_LIST(a)->items.count >= AS_LIST(b)->items.count ? a : b;
        }
        else if(IS_DICT(a) && IS_DICT(b))
        {
            return AS_DICT(a)->names.count >= AS_DICT(b)->names.count ? a : b;
        }
        else if(IS_BYTES(a) && IS_BYTES(b))
        {
            return AS_BYTES(a)->bytes.count >= AS_BYTES(b)->bytes.count ? a : b;
        }
        else if(IS_FILE(a) && IS_FILE(b))
        {
            return strcmp(AS_FILE(a)->path->chars, AS_FILE(b)->path->chars) >= 0 ? a : b;
        }
        else if(IS_OBJ(b))
        {
            return AS_OBJ(a)->type >= AS_OBJ(b)->type ? a : b;
        }
        else
        {
            return a;
        }
    }
    else
    {
        return a;
    }
}

/**
 * sorts values in an array using the bubble-sort algorithm
 */
void sort_values(b_value* values, int count)
{
    for(int i = 0; i < count; i++)
    {
        for(int j = 0; j < count; j++)
        {
            if(values_equal(values[j], find_max_value(values[i], values[j])))
            {
                b_value temp = values[i];
                values[i] = values[j];
                values[j] = temp;

                if(IS_LIST(values[i]))
                    sort_values(AS_LIST(values[i])->items.values, AS_LIST(values[i])->items.count);

                if(IS_LIST(values[j]))
                    sort_values(AS_LIST(values[j])->items.values, AS_LIST(values[j])->items.count);
            }
        }
    }
}

b_value copy_value(b_vm* vm, b_value value)
{
    if(IS_OBJ(value))
    {
        switch(AS_OBJ(value)->type)
        {
            case OBJ_STRING:
            {
                b_obj_string* string = AS_STRING(value);
                return OBJ_VAL(copy_string(vm, string->chars, string->length));
            }
            case OBJ_BYTES:
            {
                b_obj_bytes* bytes = AS_BYTES(value);
                return OBJ_VAL(copy_bytes(vm, bytes->bytes.bytes, bytes->bytes.count));
            }
            case OBJ_LIST:
            {
                b_obj_list* list = AS_LIST(value);
                b_obj_list* n_list = new_list(vm);
                push(vm, OBJ_VAL(n_list));

                for(int i = 0; i < list->items.count; i++)
                {
                    write_value_arr(vm, &n_list->items, list->items.values[i]);
                }

                pop(vm);
                return OBJ_VAL(n_list);
            }
            /*case OBJ_DICT: {
        b_obj_dict *dict = AS_DICT(value);
        b_obj_dict *n_dict = new_dict(vm);

        // @TODO: Figure out how to handle dictionary values correctly
        // remember that copying keys is redundant and unnecessary
      }*/
            default:
                return value;
        }
    }

    return value;
}

b_obj* allocate_object(b_vm* vm, size_t size, b_obj_type type)
{
    b_obj* object;
    object = (b_obj*)reallocate(vm, NULL, 0, size);
    object->type = type;
    object->mark = false;
    object->sibling = vm->objectlinks;
    object->definitelyreal = true;
    vm->objectlinks = object;
    vm->objectcount++;
    //#if defined(DEBUG_LOG_GC) && DEBUG_LOG_GC
    //    fprintf(stderr, "allocate_object: size %ld type %d\n", size, type);
    //#endif
    return object;
}

b_obj_ptr* new_ptr(b_vm* vm, void* pointer)
{
    b_obj_ptr* ptr = (b_obj_ptr*)allocate_object(vm, sizeof(b_obj_ptr), OBJ_PTR);
    ptr->pointer = pointer;
    ptr->name = "<void *>";
    ptr->free_fn = NULL;
    return ptr;
}

b_obj_module* new_module(b_vm* vm, char* name, char* file)
{
    b_obj_module* module = (b_obj_module*)allocate_object(vm, sizeof(b_obj_module), OBJ_MODULE);
    init_table(&module->values);
    module->name = name;
    module->file = file;
    module->unloader = NULL;
    module->preloader = NULL;
    module->handle = NULL;
    module->imported = false;
    return module;
}

b_obj_switch* new_switch(b_vm* vm)
{
    b_obj_switch* sw = (b_obj_switch*)allocate_object(vm, sizeof(b_obj_switch), OBJ_SWITCH);
    init_table(&sw->table);
    sw->default_jump = -1;
    sw->exit_jump = -1;
    return sw;
}

b_obj_bytes* new_bytes(b_vm* vm, int length)
{
    b_obj_bytes* bytes = (b_obj_bytes*)allocate_object(vm, sizeof(b_obj_bytes), OBJ_BYTES);
    init_byte_arr(vm, &bytes->bytes, length);
    return bytes;
}

b_obj_list* new_list(b_vm* vm)
{
    b_obj_list* list = (b_obj_list*)allocate_object(vm, sizeof(b_obj_list), OBJ_LIST);
    init_value_arr(&list->items);
    return list;
}

b_obj_range* new_range(b_vm* vm, int lower, int upper)
{
    b_obj_range* range = (b_obj_range*)allocate_object(vm, sizeof(b_obj_range), OBJ_RANGE);
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

b_obj_dict* new_dict(b_vm* vm)
{
    b_obj_dict* dict = (b_obj_dict*)allocate_object(vm, sizeof(b_obj_dict), OBJ_DICT);
    init_value_arr(&dict->names);
    init_table(&dict->items);
    return dict;
}

b_obj_file* new_file(b_vm* vm, b_obj_string* path, b_obj_string* mode)
{
    b_obj_file* file = (b_obj_file*)allocate_object(vm, sizeof(b_obj_file), OBJ_FILE);
    file->is_open = true;
    file->mode = mode;
    file->path = path;
    file->file = NULL;
    return file;
}

b_obj_bound* new_bound_method(b_vm* vm, b_value receiver, b_obj_closure* method)
{
    b_obj_bound* bound = (b_obj_bound*)allocate_object(vm, sizeof(b_obj_bound), OBJ_BOUND_METHOD);
    bound->receiver = receiver;
    bound->method = method;
    return bound;
}

b_obj_class* new_class(b_vm* vm, b_obj_string* name)
{
    b_obj_class* klass = (b_obj_class*)allocate_object(vm, sizeof(b_obj_class), OBJ_CLASS);
    klass->name = name;
    init_table(&klass->properties);
    init_table(&klass->static_properties);
    init_table(&klass->methods);
    klass->initializer = EMPTY_VAL;
    klass->superclass = NULL;
    return klass;
}

b_obj_func* new_function(b_vm* vm, b_obj_module* module, b_func_type type)
{
    b_obj_func* function = (b_obj_func*)allocate_object(vm, sizeof(b_obj_func), OBJ_FUNCTION);
    function->arity = 0;
    function->up_value_count = 0;
    function->is_variadic = false;
    function->name = NULL;
    function->type = type;
    function->module = module;
    init_blob(&function->blob);
    return function;
}

b_obj_instance* new_instance(b_vm* vm, b_obj_class* klass)
{
    b_obj_instance* instance = (b_obj_instance*)allocate_object(vm, sizeof(b_obj_instance), OBJ_INSTANCE);
    push(vm, OBJ_VAL(instance));// gc fix
    instance->klass = klass;
    init_table(&instance->properties);
    if(klass->properties.count > 0)
    {
        table_copy(vm, &klass->properties, &instance->properties);
    }
    pop(vm);// gc fix
    return instance;
}

b_obj_native* new_native(b_vm* vm, b_native_fn function, const char* name)
{
    b_obj_native* native = (b_obj_native*)allocate_object(vm, sizeof(b_obj_native), OBJ_NATIVE);
    native->natfn = function;
    native->name = name;
    native->type = TYPE_FUNCTION;
    return native;
}

b_obj_closure* new_closure(b_vm* vm, b_obj_func* function)
{
    b_obj_up_value** up_values = ALLOCATE(b_obj_up_value*, function->up_value_count);
    for(int i = 0; i < function->up_value_count; i++)
    {
        up_values[i] = NULL;
    }

    b_obj_closure* closure = (b_obj_closure*)allocate_object(vm, sizeof(b_obj_closure), OBJ_CLOSURE);
    closure->fnptr = function;
    closure->up_values = up_values;
    closure->up_value_count = function->up_value_count;
    return closure;
}

b_obj_string* bl_string_fromallocated(b_vm* vm, char* chars, int length, uint32_t hash)
{
    //fprintf(stderr, "call to bl_string_fromallocated! chars=\"%.*s\"\n", length, chars);
    b_obj_string* string = (b_obj_string*)allocate_object(vm, sizeof(b_obj_string), OBJ_STRING);
    string->chars = chars;
    string->length = length;
    string->utf8_length = utf8len(chars);
    string->is_ascii = false;
    string->hash = hash;

    push(vm, OBJ_VAL(string));// fixing gc corruption
    table_set(vm, &vm->strings, OBJ_VAL(string), NIL_VAL);
    pop(vm);// fixing gc corruption

    return string;
}

b_obj_string* take_string(b_vm* vm, char* chars, int length)
{
    uint32_t hash = hash_string(chars, length);

    b_obj_string* interned = table_find_string(&vm->strings, chars, length, hash);
    if(interned != NULL)
    {
        FREE_ARRAY(char, chars, (size_t)length + 1);
        return interned;
    }

    return bl_string_fromallocated(vm, chars, length, hash);
}

b_obj_string* copy_string(b_vm* vm, const char* chars, int length)
{
    uint32_t hash = hash_string(chars, length);

    b_obj_string* interned = table_find_string(&vm->strings, chars, length, hash);
    if(interned != NULL)
        return interned;

    char* heap_chars = ALLOCATE(char, (size_t)length + 1);
    memcpy(heap_chars, chars, length);
    heap_chars[length] = '\0';

    return bl_string_fromallocated(vm, heap_chars, length, hash);
}

b_obj_up_value* new_up_value(b_vm* vm, b_value* slot)
{
    b_obj_up_value* up_value = (b_obj_up_value*)allocate_object(vm, sizeof(b_obj_up_value), OBJ_UP_VALUE);
    up_value->closed = NIL_VAL;
    up_value->location = slot;
    up_value->next = NULL;
    return up_value;
}

static void print_function(b_obj_func* func)
{
    if(func->name == NULL)
    {
        printf("<script at %p>", (void*)func);
    }
    else
    {
        printf(func->is_variadic ? "<function %s(%d...) at %p>" : "<function %s(%d) at %p>", func->name->chars, func->arity, (void*)func);
    }
}

static void print_list(b_obj_list* list)
{
    printf("[");
    for(int i = 0; i < list->items.count; i++)
    {
        print_value(list->items.values[i]);
        if(i != list->items.count - 1)
        {
            printf(", ");
        }
    }
    printf("]");
}

static void print_bytes(b_obj_bytes* bytes)
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

static void print_dict(b_obj_dict* dict)
{
    printf("{");
    for(int i = 0; i < dict->names.count; i++)
    {
        print_value(dict->names.values[i]);

        printf(": ");

        b_value value;
        if(table_get(&dict->items, dict->names.values[i], &value))
        {
            print_value(value);
        }

        if(i != dict->names.count - 1)
        {
            printf(", ");
        }
    }
    printf("}");
}

static void print_file(b_obj_file* file)
{
    printf("<file at %s in mode %s>", file->path->chars, file->mode->chars);
}

void print_object(b_value value, bool fix_string)
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
            b_obj_range* range = AS_RANGE(value);
            printf("<range %d-%d>", range->lower, range->upper);
            break;
        }
        case OBJ_FILE:
        {
            print_file(AS_FILE(value));
            break;
        }
        case OBJ_DICT:
        {
            print_dict(AS_DICT(value));
            break;
        }
        case OBJ_LIST:
        {
            print_list(AS_LIST(value));
            break;
        }
        case OBJ_BYTES:
        {
            print_bytes(AS_BYTES(value));
            break;
        }

        case OBJ_BOUND_METHOD:
        {
            print_function(AS_BOUND(value)->method->fnptr);
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
            print_function(AS_CLOSURE(value)->fnptr);
            break;
        }
        case OBJ_FUNCTION:
        {
            print_function(AS_FUNCTION(value));
            break;
        }
        case OBJ_INSTANCE:
        {
            // @TODO: support the to_string() override
            b_obj_instance* instance = AS_INSTANCE(value);
            printf("<class %s instance at %p>", instance->klass->name->chars, (void*)instance);
            break;
        }
        case OBJ_NATIVE:
        {
            b_obj_native* native = AS_NATIVE(value);
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
            b_obj_string* string = AS_STRING(value);
            if(fix_string)
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

b_obj_bytes* copy_bytes(b_vm* vm, unsigned char* b, int length)
{
    b_obj_bytes* bytes = new_bytes(vm, length);
    memcpy(bytes->bytes.bytes, b, length);
    return bytes;
}

b_obj_bytes* take_bytes(b_vm* vm, unsigned char* b, int length)
{
    b_obj_bytes* bytes = (b_obj_bytes*)allocate_object(vm, sizeof(b_obj_bytes), OBJ_BYTES);
    bytes->bytes.count = length;
    bytes->bytes.bytes = b;
    return bytes;
}

static char* function_to_string(b_obj_func* func)
{
    if(func->name == NULL)
    {
        return strdup("<script 0x00>");
    }

    const char* format = func->is_variadic ? "<function %s(%d...)>" : "<function %s(%d)>";
    char* str = (char*)malloc(sizeof(char) * (snprintf(NULL, 0, format, func->name->chars, func->arity)));
    if(str != NULL)
    {
        sprintf(str, format, func->name->chars, func->arity);
        return str;
    }
    return strdup(func->name->chars);
}

static char* list_to_string(b_vm* vm, b_value_arr* array)
{
    char* str = strdup("[");
    for(int i = 0; i < array->count; i++)
    {
        char* val = value_to_string(vm, array->values[i]);
        if(val != NULL)
        {
            str = append_strings(str, val);
            free(val);
        }
        if(i != array->count - 1)
        {
            str = append_strings(str, ", ");
        }
    }
    str = append_strings(str, "]");
    return str;
}

static char* bytes_to_string(b_vm* vm, b_byte_arr* array)
{
    char* str = strdup("(");
    for(int i = 0; i < array->count; i++)
    {
        char* chars = ALLOCATE(char, snprintf(NULL, 0, "0x%x", array->bytes[i]));
        if(chars != NULL)
        {
            sprintf(chars, "0x%x", array->bytes[i]);
            str = append_strings(str, chars);
        }

        if(i != array->count - 1)
        {
            str = append_strings(str, " ");
        }
    }
    str = append_strings(str, ")");
    return str;
}

static char* dict_to_string(b_vm* vm, b_obj_dict* dict)
{
    char* str = strdup("{");
    for(int i = 0; i < dict->names.count; i++)
    {
        // print_value(dict->names.values[i]);
        b_value key = dict->names.values[i];
        char* _key = value_to_string(vm, key);
        if(_key != NULL)
        {
            str = append_strings(str, _key);
        }
        str = append_strings(str, ": ");

        b_value value;
        table_get(&dict->items, key, &value);
        char* val = value_to_string(vm, value);
        if(val != NULL)
        {
            str = append_strings(str, val);
        }

        if(i != dict->names.count - 1)
        {
            str = append_strings(str, ", ");
        }
    }
    str = append_strings(str, "}");
    return str;
}

char* object_to_string(b_vm* vm, b_value value)
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
            return function_to_string(AS_CLOSURE(value)->fnptr);
        case OBJ_BOUND_METHOD:
        {
            return function_to_string(AS_BOUND(value)->method->fnptr);
        }
        case OBJ_FUNCTION:
            return function_to_string(AS_FUNCTION(value));
        case OBJ_NATIVE:
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
            b_obj_range* range = AS_RANGE(value);
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
            return bytes_to_string(vm, &AS_BYTES(value)->bytes);
        case OBJ_LIST:
            return list_to_string(vm, &AS_LIST(value)->items);
        case OBJ_DICT:
            return dict_to_string(vm, AS_DICT(value));
        case OBJ_FILE:
        {
            b_obj_file* file = AS_FILE(value);
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

const char* object_type(b_obj* object)
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
        case OBJ_LIST:
            return "list";

        case OBJ_CLASS:
            return "class";

        case OBJ_FUNCTION:
        case OBJ_NATIVE:
        case OBJ_CLOSURE:
        case OBJ_BOUND_METHOD:
            return "function";

        case OBJ_INSTANCE:
            return ((b_obj_instance*)object)->klass->name->chars;

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
static b_value number_to_value(double v);
static b_value integer_to_value(int v);
static b_value long_to_value(long v);
static double value_to_number(b_value v);
static bool is_obj_type(b_value v, b_obj_type t);
static bool is_std_file(b_obj_file* file);
static void add_module(b_vm* vm, b_obj_module* module);
static b_obj* gc_protect(b_vm* vm, b_obj* object);
static void gc_clear_protection(b_vm* vm);
static b_token make_token(b_scanner* s, b_tkn_type type);
static b_token error_token(b_scanner* s, const char* message, ...);
static char bl_scanner_advance(b_scanner* s);
static bool bl_scanner_match(b_scanner* s, char expected);
static char bl_scanner_current(b_scanner* s);
static char bl_scanner_previous(b_scanner* s);
static char bl_scanner_next(b_scanner* s);
static b_token bl_scanner_scanstring(b_scanner* s, char quote);
static b_token bl_scanner_scannumber(b_scanner* s);
static b_tkn_type bl_scanner_checkkw(b_scanner* s, int start, int length, const char* rest, b_tkn_type type);
static b_tkn_type bl_scanner_scanidenttype(b_scanner* s);
static b_token bl_scanner_scanidentifier(b_scanner* s);
static b_token bl_scanner_decorator(b_scanner* s);

void bl_scanner_init(b_scanner* s, const char* source)
{
    s->current = source;
    s->start = source;
    s->line = 1;
    s->interpolating_count = -1;
}

bool bl_scanner_isatend(b_scanner* s)
{
    return *s->current == '\0';
}

static b_token make_token(b_scanner* s, b_tkn_type type)
{
    b_token t;
    t.type = type;
    t.start = s->start;
    t.length = (int)(s->current - s->start);
    t.line = s->line;
    return t;
}

static b_token error_token(b_scanner* s, const char* message, ...)
{
    va_list args;
    va_start(args, message);
    char* err = NULL;
    int length = vasprintf(&err, message, args);
    va_end(args);

    b_token t;
    t.type = ERROR_TOKEN;
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

static bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static bool is_binary(char c)
{
    return c == '0' || c == '1';
}

static bool is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool is_octal(char c)
{
    return c >= '0' && c <= '7';
}

static bool is_hexadecimal(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static char bl_scanner_advance(b_scanner* s)
{
    s->current++;
    if(s->current[-1] == '\n')
        s->line++;
    return s->current[-1];
}

static bool bl_scanner_match(b_scanner* s, char expected)
{
    if(bl_scanner_isatend(s))
        return false;
    if(*s->current != expected)
        return false;

    s->current++;
    if(s->current[-1] == '\n')
        s->line++;
    return true;
}

static char bl_scanner_current(b_scanner* s)
{
    return *s->current;
}

static char bl_scanner_previous(b_scanner* s)
{
    return s->current[-1];
}

static char bl_scanner_next(b_scanner* s)
{
    if(bl_scanner_isatend(s))
        return '\0';
    return s->current[1];
}

b_token bl_scanner_skipblockcomments(b_scanner* s)
{
    int nesting = 1;
    while(nesting > 0)
    {
        if(bl_scanner_isatend(s))
        {
            return error_token(s, "unclosed block comment");
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

    return make_token(s, UNDEFINED_TOKEN);
}

b_token bl_scanner_skipwhitespace(b_scanner* s)
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
                    bl_scanner_advance(s);
                break;
            }

            case '/':
                if(bl_scanner_next(s) == '*')
                {
                    bl_scanner_advance(s);
                    bl_scanner_advance(s);
                    b_token result = bl_scanner_skipblockcomments(s);
                    if(result.type != UNDEFINED_TOKEN)
                    {
                        return result;
                    }
                    break;
                }
                else
                {
                    return make_token(s, UNDEFINED_TOKEN);
                }

                // exit as soon as we see a non-whitespace...
            default:
                return make_token(s, UNDEFINED_TOKEN);
        }
    }
}

static b_token bl_scanner_scanstring(b_scanner* s, char quote)
{
    while(bl_scanner_current(s) != quote && !bl_scanner_isatend(s))
    {
        if(bl_scanner_current(s) == '$' && bl_scanner_next(s) == '{' && bl_scanner_previous(s) != '\\')
        {// interpolation started
            if(s->interpolating_count - 1 < MAX_INTERPOLATION_NESTING)
            {
                s->interpolating_count++;
                s->interpolating[s->interpolating_count] = (int)quote;
                s->current++;
                b_token tkn = make_token(s, INTERPOLATION_TOKEN);
                s->current++;
                return tkn;
            }

            return error_token(s, "maximum interpolation nesting of %d exceeded by %d", MAX_INTERPOLATION_NESTING, MAX_INTERPOLATION_NESTING - s->interpolating_count + 1);
        }
        if(bl_scanner_current(s) == '\\' && (bl_scanner_next(s) == quote || bl_scanner_next(s) == '\\'))
        {
            bl_scanner_advance(s);
        }
        bl_scanner_advance(s);
    }

    if(bl_scanner_isatend(s))
        return error_token(s, "unterminated string (opening quote not matched)");

    bl_scanner_match(s, quote);// the closing quote
    return make_token(s, LITERAL_TOKEN);
}

static b_token bl_scanner_scannumber(b_scanner* s)
{
    // handle binary, octal and hexadecimals
    if(bl_scanner_previous(s) == '0')
    {
        if(bl_scanner_match(s, 'b'))
        {// binary number
            while(is_binary(bl_scanner_current(s)))
                bl_scanner_advance(s);

            return make_token(s, BIN_NUMBER_TOKEN);
        }
        else if(bl_scanner_match(s, 'c'))
        {
            while(is_octal(bl_scanner_current(s)))
                bl_scanner_advance(s);

            return make_token(s, OCT_NUMBER_TOKEN);
        }
        else if(bl_scanner_match(s, 'x'))
        {
            while(is_hexadecimal(bl_scanner_current(s)))
                bl_scanner_advance(s);

            return make_token(s, HEX_NUMBER_TOKEN);
        }
    }

    while(is_digit(bl_scanner_current(s)))
        bl_scanner_advance(s);

    // dots(.) are only valid here when followed by a digit
    if(bl_scanner_current(s) == '.' && is_digit(bl_scanner_next(s)))
    {
        bl_scanner_advance(s);

        while(is_digit(bl_scanner_current(s)))
            bl_scanner_advance(s);

        // E or e are only valid here when followed by a digit and occurring after a
        // dot
        if((bl_scanner_current(s) == 'e' || bl_scanner_current(s) == 'E') && (bl_scanner_next(s) == '+' || bl_scanner_next(s) == '-'))
        {
            bl_scanner_advance(s);
            bl_scanner_advance(s);

            while(is_digit(bl_scanner_current(s)))
                bl_scanner_advance(s);
        }
    }

    return make_token(s, REG_NUMBER_TOKEN);
}

static b_tkn_type bl_scanner_checkkw(b_scanner* s, int start, int length, const char* rest, b_tkn_type type)
{
    if((int)(s->current - s->start) == start + length && memcmp(s->start + start, rest, length) == 0)
    {
        return type;
    }
    return IDENTIFIER_TOKEN;
}

static b_tkn_type bl_scanner_scanidenttype(b_scanner* s)
{
    switch(s->start[0])
    {
        case 'a':
            if(s->current - s->start > 1)
            {
                switch(s->start[1])
                {
                    case 'n':
                        return bl_scanner_checkkw(s, 2, 1, "d", AND_TOKEN);
                    case 's':
                        if(s->current - s->start > 2)
                        {
                            return bl_scanner_checkkw(s, 2, 4, "sert", ASSERT_TOKEN);
                        }
                        else
                        {
                            return bl_scanner_checkkw(s, 2, 0, "", AS_TOKEN);
                        }
                }
            }
            break;
        case 'b':
            return bl_scanner_checkkw(s, 1, 4, "reak", BREAK_TOKEN);
        case 'c':
            if(s->current - s->start > 1)
            {
                switch(s->start[1])
                {
                    case 'a':
                        return bl_scanner_checkkw(s, 2, 3, "tch", CATCH_TOKEN);
                    case 'l':
                        return bl_scanner_checkkw(s, 2, 3, "ass", CLASS_TOKEN);
                    case 'o':
                        return bl_scanner_checkkw(s, 2, 6, "ntinue", CONTINUE_TOKEN);
                }
            }
            break;
        case 'd':
            if(s->current - s->start > 1)
            {
                switch(s->start[1])
                {
                    case 'e':
                        if(s->current - s->start > 2)
                        {
                            switch(s->start[2])
                            {
                                case 'f':
                                    if(s->current - s->start > 3)
                                        return bl_scanner_checkkw(s, 3, 4, "ault", DEFAULT_TOKEN);
                                    else
                                        return bl_scanner_checkkw(s, 3, 0, "", DEF_TOKEN);
                            }
                        }
                        break;
                    case 'i':
                        return bl_scanner_checkkw(s, 2, 1, "e", DIE_TOKEN);
                    case 'o':
                        return bl_scanner_checkkw(s, 2, 0, "", DO_TOKEN);
                }
            }
            break;
        case 'e':
            if(s->current - s->start > 1)
            {
                switch(s->start[1])
                {
                    case 'c':
                        return bl_scanner_checkkw(s, 2, 2, "ho", ECHO_TOKEN);
                    case 'l':
                        return bl_scanner_checkkw(s, 2, 2, "se", ELSE_TOKEN);
                        /*case 'm':
            return bl_scanner_checkkw(s, 2, 3, "pty", EMPTY_TOKEN);*/
                }
            }
            break;
        case 'f':
            if(s->current - s->start > 1)
            {
                switch(s->start[1])
                {
                    case 'a':
                        return bl_scanner_checkkw(s, 2, 3, "lse", FALSE_TOKEN);
                    case 'i':
                        return bl_scanner_checkkw(s, 2, 5, "nally", FINALLY_TOKEN);
                    case 'o':
                        return bl_scanner_checkkw(s, 2, 1, "r", FOR_TOKEN);
                }
            }
            break;
        case 'i':
            if(s->current - s->start > 1)
            {
                switch(s->start[1])
                {
                    case 'f':
                        return bl_scanner_checkkw(s, 2, 0, "", IF_TOKEN);
                    case 'm':
                        return bl_scanner_checkkw(s, 2, 4, "port", IMPORT_TOKEN);
                    case 'n':
                        return bl_scanner_checkkw(s, 2, 0, "", IN_TOKEN);
                    case 't':
                        return bl_scanner_checkkw(s, 2, 2, "er", ITER_TOKEN);
                }
            }
            break;
        case 'n':
            return bl_scanner_checkkw(s, 1, 2, "il", NIL_TOKEN);
        case 'o':
            return bl_scanner_checkkw(s, 1, 1, "r", OR_TOKEN);
        case 'p':
            return bl_scanner_checkkw(s, 1, 5, "arent", PARENT_TOKEN);
        case 'r':
            return bl_scanner_checkkw(s, 1, 5, "eturn", RETURN_TOKEN);
        case 's':
            if(s->current - s->start > 1)
            {
                switch(s->start[1])
                {
                    case 'e':
                        return bl_scanner_checkkw(s, 2, 2, "lf", SELF_TOKEN);
                    case 't':
                        return bl_scanner_checkkw(s, 2, 4, "atic", STATIC_TOKEN);
                }
            }
            break;
        case 't':
            if(s->current - s->start > 2 && s->start[1] == 'r')
            {
                switch(s->start[2])
                {
                    case 'u':
                        return bl_scanner_checkkw(s, 3, 1, "e", TRUE_TOKEN);
                    case 'y':
                        return bl_scanner_checkkw(s, 3, 0, "", TRY_TOKEN);
                }
            }
            break;
        case 'u':
        {
            return bl_scanner_checkkw(s, 1, 4, "sing", USING_TOKEN);
        }
        case 'v':
            return bl_scanner_checkkw(s, 1, 2, "ar", VAR_TOKEN);
        case 'w':
            if(s->current - s->start > 2 && s->start[1] == 'h')
            {
                switch(s->start[2])
                {
                    case 'i':
                        return bl_scanner_checkkw(s, 3, 2, "le", WHILE_TOKEN);
                    case 'e':
                        return bl_scanner_checkkw(s, 3, 1, "n", WHEN_TOKEN);
                }
            }
            break;
    }
    return IDENTIFIER_TOKEN;
}

static b_token bl_scanner_scanidentifier(b_scanner* s)
{
    while(is_alpha(bl_scanner_current(s)) || is_digit(bl_scanner_current(s)))
        bl_scanner_advance(s);
    return make_token(s, bl_scanner_scanidenttype(s));
}

static b_token bl_scanner_decorator(b_scanner* s)
{
    while(is_alpha(bl_scanner_current(s)) || is_digit(bl_scanner_current(s)))
        bl_scanner_advance(s);
    return make_token(s, DECORATOR_TOKEN);
}

b_token bl_scanner_scantoken(b_scanner* s)
{
    b_token tk = bl_scanner_skipwhitespace(s);
    if(tk.type != UNDEFINED_TOKEN)
    {
        return tk;
    }

    s->start = s->current;

    if(bl_scanner_isatend(s))
        return make_token(s, EOF_TOKEN);

    char c = bl_scanner_advance(s);

    if(is_digit(c))
        return bl_scanner_scannumber(s);
    else if(is_alpha(c))
        return bl_scanner_scanidentifier(s);

    switch(c)
    {
        case '(':
            return make_token(s, LPAREN_TOKEN);
        case ')':
            return make_token(s, RPAREN_TOKEN);
        case '[':
            return make_token(s, LBRACKET_TOKEN);
        case ']':
            return make_token(s, RBRACKET_TOKEN);
        case '{':
            return make_token(s, LBRACE_TOKEN);
        case '}':
            if(s->interpolating_count > -1)
            {
                b_token token = bl_scanner_scanstring(s, (char)s->interpolating[s->interpolating_count]);
                s->interpolating_count--;
                return token;
            }
            return make_token(s, RBRACE_TOKEN);
        case ';':
            return make_token(s, SEMICOLON_TOKEN);
        case '\\':
            return make_token(s, BACKSLASH_TOKEN);
        case ':':
            return make_token(s, COLON_TOKEN);
        case ',':
            return make_token(s, COMMA_TOKEN);
        case '@':
            return bl_scanner_decorator(s);
        case '!':
            return make_token(s, bl_scanner_match(s, '=') ? BANG_EQ_TOKEN : BANG_TOKEN);
        case '.':
            if(bl_scanner_match(s, '.'))
            {
                return make_token(s, bl_scanner_match(s, '.') ? TRI_DOT_TOKEN : RANGE_TOKEN);
            }
            return make_token(s, DOT_TOKEN);
        case '+':
        {
            if(bl_scanner_match(s, '+'))
                return make_token(s, INCREMENT_TOKEN);
            if(bl_scanner_match(s, '='))
                return make_token(s, PLUS_EQ_TOKEN);
            else
                return make_token(s, PLUS_TOKEN);
        }
        case '-':
        {
            if(bl_scanner_match(s, '-'))
                return make_token(s, DECREMENT_TOKEN);
            if(bl_scanner_match(s, '='))
                return make_token(s, MINUS_EQ_TOKEN);
            else
                return make_token(s, MINUS_TOKEN);
        }
        case '*':
            if(bl_scanner_match(s, '*'))
            {
                return make_token(s, bl_scanner_match(s, '=') ? POW_EQ_TOKEN : POW_TOKEN);
            }
            else
            {
                return make_token(s, bl_scanner_match(s, '=') ? MULTIPLY_EQ_TOKEN : MULTIPLY_TOKEN);
            }
        case '/':
            if(bl_scanner_match(s, '/'))
            {
                return make_token(s, bl_scanner_match(s, '=') ? FLOOR_EQ_TOKEN : FLOOR_TOKEN);
            }
            else
            {
                return make_token(s, bl_scanner_match(s, '=') ? DIVIDE_EQ_TOKEN : DIVIDE_TOKEN);
            }
        case '=':
            return make_token(s, bl_scanner_match(s, '=') ? EQUAL_EQ_TOKEN : EQUAL_TOKEN);
        case '<':
            if(bl_scanner_match(s, '<'))
            {
                return make_token(s, bl_scanner_match(s, '=') ? LSHIFT_EQ_TOKEN : LSHIFT_TOKEN);
            }
            else
            {
                return make_token(s, bl_scanner_match(s, '=') ? LESS_EQ_TOKEN : LESS_TOKEN);
            }
        case '>':
            if(bl_scanner_match(s, '>'))
            {
                return make_token(s, bl_scanner_match(s, '=') ? RSHIFT_EQ_TOKEN : RSHIFT_TOKEN);
            }
            else
            {
                return make_token(s, bl_scanner_match(s, '=') ? GREATER_EQ_TOKEN : GREATER_TOKEN);
            }
        case '%':
            return make_token(s, bl_scanner_match(s, '=') ? PERCENT_EQ_TOKEN : PERCENT_TOKEN);
        case '&':
            return make_token(s, bl_scanner_match(s, '=') ? AMP_EQ_TOKEN : AMP_TOKEN);
        case '|':
            return make_token(s, bl_scanner_match(s, '=') ? BAR_EQ_TOKEN : BAR_TOKEN);
        case '~':
            return make_token(s, bl_scanner_match(s, '=') ? TILDE_EQ_TOKEN : TILDE_TOKEN);
        case '^':
            return make_token(s, bl_scanner_match(s, '=') ? XOR_EQ_TOKEN : XOR_TOKEN);

            // newline
        case '\n':
            return make_token(s, NEWLINE_TOKEN);

        case '"':
            return bl_scanner_scanstring(s, '"');
        case '\'':
            return bl_scanner_scanstring(s, '\'');

        case '?':
            return make_token(s, QUESTION_TOKEN);

            // --- DO NOT MOVE ABOVE OR BELOW THE DEFAULT CASE ---
            // fall-through tokens goes here... this tokens are only valid
            // when the carry another token with them...
            // be careful not to add break after them so that they may use the default
            // case.

        default:
            break;
    }

    return error_token(s, "unexpected character %c", c);
}

static b_value number_to_value(double v);
static b_value integer_to_value(int v);
static b_value long_to_value(long v);
static double value_to_number(b_value v);
static bool is_obj_type(b_value v, b_obj_type t);
static bool is_std_file(b_obj_file* file);
static void add_module(b_vm* vm, b_obj_module* module);
static b_obj* gc_protect(b_vm* vm, b_obj* object);
static void gc_clear_protection(b_vm* vm);
static b_blob* bl_parser_currentblob(b_parser* p);
static void bl_parser_errorat(b_parser* p, b_token* t, const char* message, va_list args);
static void bl_parser_raiseerror(b_parser* p, const char* message, ...);
static void bl_parser_raiseerroratcurrent(b_parser* p, const char* message, ...);
static void bl_parser_advance(b_parser* p);
static void bl_parser_consume(b_parser* p, b_tkn_type t, const char* message);
static void bl_parser_consumeor(b_parser* p, const char* message, const b_tkn_type ts[], int count);
static bool bl_parser_checknumber(b_parser* p);
static bool bl_parser_check(b_parser* p, b_tkn_type t);
static bool bl_parser_match(b_parser* p, b_tkn_type t);
static void bl_parser_consumestmtend(b_parser* p);
static void bl_parser_ignorespace(b_parser* p);
static int bl_parser_getcodeargscount(const uint8_t* bytecode, const b_value* constants, int ip);
static void bl_parser_emitbyte(b_parser* p, uint8_t byte);
static void bl_parser_emitshort(b_parser* p, uint16_t byte);
static void bl_parser_emitbytes(b_parser* p, uint8_t byte, uint8_t byte2);
static void bl_parser_emitbyte_and_short(b_parser* p, uint8_t byte, uint16_t byte2);
static void bl_parser_emitloop(b_parser* p, int loop_start);
static void bl_parser_emitreturn(b_parser* p);
static int bl_parser_makeconstant(b_parser* p, b_value value);
static void bl_parser_emitconstant(b_parser* p, b_value value);
static int bl_parser_emitjump(b_parser* p, uint8_t instruction);
static int bl_parser_emitswitch(b_parser* p);
static int bl_parser_emittry(b_parser* p);
static void bl_parser_patchswitch(b_parser* p, int offset, int constant);
static void bl_parser_patchtry(b_parser* p, int offset, int type, int address, int finally);
static void bl_parser_patchjump(b_parser* p, int offset);
static void bl_compiler_init(b_parser* p, b_compiler* compiler, b_func_type type);
static int bl_parser_identconst(b_parser* p, b_token* name);
static bool bl_parser_identsequal(b_token* a, b_token* b);
static int bl_compiler_resolvelocal(b_parser* p, b_compiler* compiler, b_token* name);
static int bl_compiler_addupvalue(b_parser* p, b_compiler* compiler, uint16_t index, bool is_local);
static int bl_compiler_resolveupvalue(b_parser* p, b_compiler* compiler, b_token* name);
static int bl_parser_addlocal(b_parser* p, b_token name);
static void bl_parser_declvar(b_parser* p);
static int bl_parser_parsevar(b_parser* p, const char* message);
static void bl_parser_markinit(b_parser* p);
static void bl_parser_defvar(b_parser* p, int global);
static b_token bl_parser_synthtoken(const char* name);
static b_obj_func* bl_compiler_end(b_parser* p);
static void bl_parser_beginscope(b_parser* p);
static void bl_parser_endscope(b_parser* p);
static void bl_parser_discardlocal(b_parser* p, int depth);
static void bl_parser_endloop(b_parser* p);
static void bl_parser_rulebinary(b_parser* p, b_token previous, bool can_assign);
static uint8_t bl_parser_parsearglist(b_parser* p);
static void bl_parser_rulecall(b_parser* p, b_token previous, bool can_assign);
static void bl_parser_ruleliteral(b_parser* p, bool can_assign);
static void bl_parser_parseassign(b_parser* p, uint8_t real_op, uint8_t get_op, uint8_t set_op, int arg);
static void bl_parser_doassign(b_parser* p, uint8_t get_op, uint8_t set_op, int arg, bool can_assign);
static void bl_parser_ruledot(b_parser* p, b_token previous, bool can_assign);
static void bl_parser_namedvar(b_parser* p, b_token name, bool can_assign);
static void bl_parser_rulelist(b_parser* p, bool can_assign);
static void bl_parser_ruledict(b_parser* p, bool can_assign);
static void bl_parser_ruleindexing(b_parser* p, b_token previous, bool can_assign);
static void bl_parser_rulevariable(b_parser* p, bool can_assign);
static void bl_parser_ruleself(b_parser* p, bool can_assign);
static void bl_parser_ruleparent(b_parser* p, bool can_assign);
static void bl_parser_rulegrouping(b_parser* p, bool can_assign);
static b_value bl_parser_compilenumber(b_parser* p);
static void bl_parser_rulenumber(b_parser* p, bool can_assign);
static int read_hex_digit(char c);
static int read_hex_escape(b_parser* p, char* str, int index, int count);
static int read_unicode_escape(b_parser* p, char* string, char* real_string, int number_bytes, int real_index, int index);
static char* compile_string(b_parser* p, int* length);
static void bl_parser_rulestring(b_parser* p, bool can_assign);
static void bl_parser_rulestrinterpol(b_parser* p, bool can_assign);
static void bl_parser_ruleunary(b_parser* p, bool can_assign);
static void bl_parser_ruleand(b_parser* p, b_token previous, bool can_assign);
static void bl_parser_ruleor(b_parser* p, b_token previous, bool can_assign);
static void bl_parser_ruleconditional(b_parser* p, b_token previous, bool can_assign);
static void do_parse_precedence(b_parser* p, b_precedence precedence);
static void parse_precedence(b_parser* p, b_precedence precedence);
static void parse_precedence_no_advance(b_parser* p, b_precedence precedence);
static b_parse_rule* get_rule(b_tkn_type type);
static void bl_parser_parseexpr(b_parser* p);
static void bl_parser_parseblock(b_parser* p);
static void function_args(b_parser* p);
static void function_body(b_parser* p, b_compiler* compiler);
static void bl_parser_parsefunction(b_parser* p, b_func_type type);
static void bl_parser_parsemethod(b_parser* p, b_token class_name, bool is_static);
static void bl_parser_ruleanon(b_parser* p, bool can_assign);
static void bl_parser_parsefield(b_parser* p, bool is_static);
static void function_declaration(b_parser* p);
static void class_declaration(b_parser* p);
static void compile_var_declaration(b_parser* p, bool is_initializer);
static void var_declaration(b_parser* p);
static void expression_statement(b_parser* p, bool is_initializer, bool semi);
static void iter_statement(b_parser* p);
static void for_statement(b_parser* p);
static void using_statement(b_parser* p);
static void if_statement(b_parser* p);
static void echo_statement(b_parser* p);
static void die_statement(b_parser* p);
static void parse_specific_import(b_parser* p, char* module_name, int import_constant, bool was_renamed, bool is_native);
static void import_statement(b_parser* p);
static void assert_statement(b_parser* p);
static void try_statement(b_parser* p);
static void return_statement(b_parser* p);
static void while_statement(b_parser* p);
static void do_while_statement(b_parser* p);
static void continue_statement(b_parser* p);
static void break_statement(b_parser* p);
static void synchronize(b_parser* p);
static void declaration(b_parser* p);
static void bl_parser_parsestmt(b_parser* p);

static b_blob* bl_parser_currentblob(b_parser* p)
{
    return &p->vm->compiler->currfunc->blob;
}

static void bl_parser_errorat(b_parser* p, b_token* t, const char* message, va_list args)
{
    fflush(stdout);// flush out anything on stdout first

    // do not cascade error
    // suppress error if already in panic mode
    if(p->panic_mode)
        return;

    p->panic_mode = true;

    fprintf(stderr, "SyntaxError");

    if(t->type == EOF_TOKEN)
    {
        fprintf(stderr, " at end");
    }
    else if(t->type == ERROR_TOKEN)
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

    p->had_error = true;
}

static void bl_parser_raiseerror(b_parser* p, const char* message, ...)
{
    va_list args;
    va_start(args, message);
    bl_parser_errorat(p, &p->previous, message, args);
    va_end(args);
}

static void bl_parser_raiseerroratcurrent(b_parser* p, const char* message, ...)
{
    va_list args;
    va_start(args, message);
    bl_parser_errorat(p, &p->current, message, args);
    va_end(args);
}

static void bl_parser_advance(b_parser* p)
{
    p->previous = p->current;

    for(;;)
    {
        p->current = bl_scanner_scantoken(p->scanner);
        if(p->current.type != ERROR_TOKEN)
            break;

        bl_parser_raiseerroratcurrent(p, p->current.start);
    }
}

static void bl_parser_consume(b_parser* p, b_tkn_type t, const char* message)
{
    if(p->current.type == t)
    {
        bl_parser_advance(p);
        return;
    }

    bl_parser_raiseerroratcurrent(p, message);
}

static void bl_parser_consumeor(b_parser* p, const char* message, const b_tkn_type ts[], int count)
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

static bool bl_parser_checknumber(b_parser* p)
{
    if(p->previous.type == REG_NUMBER_TOKEN || p->previous.type == OCT_NUMBER_TOKEN || p->previous.type == BIN_NUMBER_TOKEN || p->previous.type == HEX_NUMBER_TOKEN)
        return true;
    return false;
}

static bool bl_parser_check(b_parser* p, b_tkn_type t)
{
    return p->current.type == t;
}

static bool bl_parser_match(b_parser* p, b_tkn_type t)
{
    if(!bl_parser_check(p, t))
        return false;
    bl_parser_advance(p);
    return true;
}

static void bl_parser_consumestmtend(b_parser* p)
{
    // allow block last statement to omit statement end
    if(p->block_count > 0 && bl_parser_check(p, RBRACE_TOKEN))
        return;

    if(bl_parser_match(p, SEMICOLON_TOKEN))
    {
        while(bl_parser_match(p, SEMICOLON_TOKEN) || bl_parser_match(p, NEWLINE_TOKEN))
            ;
        return;
    }

    if(bl_parser_match(p, EOF_TOKEN) || p->previous.type == EOF_TOKEN)
        return;

    bl_parser_consume(p, NEWLINE_TOKEN, "end of statement expected");
    while(bl_parser_match(p, SEMICOLON_TOKEN) || bl_parser_match(p, NEWLINE_TOKEN))
        ;
}

static void bl_parser_ignorespace(b_parser* p)
{
    while(bl_parser_match(p, NEWLINE_TOKEN))
        ;
}

static int bl_parser_getcodeargscount(const uint8_t* bytecode, const b_value* constants, int ip)
{
    b_code code = (b_code)bytecode[ip];

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
            b_obj_func* fn = AS_FUNCTION(constants[constant]);

            // There is two byte for the constant, then three for each up value.
            return 2 + (fn->up_value_count * 3);
        }

            //    default:
            //      return 0;
    }
    return 0;
}

static void bl_parser_emitbyte(b_parser* p, uint8_t byte)
{
    write_blob(p->vm, bl_parser_currentblob(p), byte, p->previous.line);
}

static void bl_parser_emitshort(b_parser* p, uint16_t byte)
{
    write_blob(p->vm, bl_parser_currentblob(p), (byte >> 8) & 0xff, p->previous.line);
    write_blob(p->vm, bl_parser_currentblob(p), byte & 0xff, p->previous.line);
}

static void bl_parser_emitbytes(b_parser* p, uint8_t byte, uint8_t byte2)
{
    write_blob(p->vm, bl_parser_currentblob(p), byte, p->previous.line);
    write_blob(p->vm, bl_parser_currentblob(p), byte2, p->previous.line);
}

static void bl_parser_emitbyte_and_short(b_parser* p, uint8_t byte, uint16_t byte2)
{
    write_blob(p->vm, bl_parser_currentblob(p), byte, p->previous.line);
    write_blob(p->vm, bl_parser_currentblob(p), (byte2 >> 8) & 0xff, p->previous.line);
    write_blob(p->vm, bl_parser_currentblob(p), byte2 & 0xff, p->previous.line);
}

/* static void bl_parser_emitbyte_and_long(b_parser *p, uint8_t byte, uint16_t byte2) {
  write_blob(p->vm, bl_parser_currentblob(p), byte, p->previous.line);
  write_blob(p->vm, bl_parser_currentblob(p), (byte2 >> 16) & 0xff, p->previous.line);
  write_blob(p->vm, bl_parser_currentblob(p), (byte2 >> 8) & 0xff, p->previous.line);
  write_blob(p->vm, bl_parser_currentblob(p), byte2 & 0xff, p->previous.line);
} */

static void bl_parser_emitloop(b_parser* p, int loop_start)
{
    bl_parser_emitbyte(p, OP_LOOP);

    int offset = bl_parser_currentblob(p)->count - loop_start + 2;
    if(offset > UINT16_MAX)
        bl_parser_raiseerror(p, "loop body too large");

    bl_parser_emitbyte(p, (offset >> 8) & 0xff);
    bl_parser_emitbyte(p, offset & 0xff);
}

static void bl_parser_emitreturn(b_parser* p)
{
    if(p->is_trying)
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

static int bl_parser_makeconstant(b_parser* p, b_value value)
{
    int constant = add_constant(p->vm, bl_parser_currentblob(p), value);
    if(constant >= UINT16_MAX)
    {
        bl_parser_raiseerror(p, "too many constants in current scope");
        return 0;
    }
    return constant;
}

static void bl_parser_emitconstant(b_parser* p, b_value value)
{
    int constant = bl_parser_makeconstant(p, value);
    bl_parser_emitbyte_and_short(p, OP_CONSTANT, (uint16_t)constant);
}

static int bl_parser_emitjump(b_parser* p, uint8_t instruction)
{
    bl_parser_emitbyte(p, instruction);

    // placeholders
    bl_parser_emitbyte(p, 0xff);
    bl_parser_emitbyte(p, 0xff);

    return bl_parser_currentblob(p)->count - 2;
}

static int bl_parser_emitswitch(b_parser* p)
{
    bl_parser_emitbyte(p, OP_SWITCH);

    // placeholders
    bl_parser_emitbyte(p, 0xff);
    bl_parser_emitbyte(p, 0xff);

    return bl_parser_currentblob(p)->count - 2;
}

static int bl_parser_emittry(b_parser* p)
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

static void bl_parser_patchswitch(b_parser* p, int offset, int constant)
{
    bl_parser_currentblob(p)->code[offset] = (constant >> 8) & 0xff;
    bl_parser_currentblob(p)->code[offset + 1] = constant & 0xff;
}

static void bl_parser_patchtry(b_parser* p, int offset, int type, int address, int finally)
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

static void bl_parser_patchjump(b_parser* p, int offset)
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

static void bl_compiler_init(b_parser* p, b_compiler* compiler, b_func_type type)
{
    compiler->enclosing = p->vm->compiler;
    compiler->currfunc = NULL;
    compiler->type = type;
    compiler->local_count = 0;
    compiler->scope_depth = 0;
    compiler->handler_count = 0;

    compiler->currfunc = new_function(p->vm, p->module, type);
    p->vm->compiler = compiler;

    if(type != TYPE_SCRIPT)
    {
        push(p->vm, OBJ_VAL(compiler->currfunc));
        p->vm->compiler->currfunc->name = copy_string(p->vm, p->previous.start, p->previous.length);
        pop(p->vm);
    }

    // claiming slot zero for use in class methods
    b_local* local = &p->vm->compiler->locals[p->vm->compiler->local_count++];
    local->depth = 0;
    local->is_captured = false;

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

static int bl_parser_identconst(b_parser* p, b_token* name)
{
    return bl_parser_makeconstant(p, OBJ_VAL(copy_string(p->vm, name->start, name->length)));
}

static bool bl_parser_identsequal(b_token* a, b_token* b)
{
    return a->length == b->length && memcmp(a->start, b->start, a->length) == 0;
}

static int bl_compiler_resolvelocal(b_parser* p, b_compiler* compiler, b_token* name)
{
    for(int i = compiler->local_count - 1; i >= 0; i--)
    {
        b_local* local = &compiler->locals[i];
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

static int bl_compiler_addupvalue(b_parser* p, b_compiler* compiler, uint16_t index, bool is_local)
{
    int up_value_count = compiler->currfunc->up_value_count;

    for(int i = 0; i < up_value_count; i++)
    {
        b_up_value* up_value = &compiler->up_values[i];
        if(up_value->index == index && up_value->is_local == is_local)
        {
            return i;
        }
    }

    if(up_value_count == UINT8_COUNT)
    {
        bl_parser_raiseerror(p, "too many closure variables in function");
        return 0;
    }

    compiler->up_values[up_value_count].is_local = is_local;
    compiler->up_values[up_value_count].index = index;
    return compiler->currfunc->up_value_count++;
}

static int bl_compiler_resolveupvalue(b_parser* p, b_compiler* compiler, b_token* name)
{
    if(compiler->enclosing == NULL)
        return -1;

    int local = bl_compiler_resolvelocal(p, compiler->enclosing, name);
    if(local != -1)
    {
        compiler->enclosing->locals[local].is_captured = true;
        return bl_compiler_addupvalue(p, compiler, (uint16_t)local, true);
    }

    int up_value = bl_compiler_resolveupvalue(p, compiler->enclosing, name);
    if(up_value != -1)
    {
        return bl_compiler_addupvalue(p, compiler, (uint16_t)up_value, false);
    }

    return -1;
}

static int bl_parser_addlocal(b_parser* p, b_token name)
{
    if(p->vm->compiler->local_count == UINT8_COUNT)
    {
        // we've reached maximum local variables per scope
        bl_parser_raiseerror(p, "too many local variables in scope");
        return -1;
    }

    b_local* local = &p->vm->compiler->locals[p->vm->compiler->local_count++];
    local->name = name;
    local->depth = -1;
    local->is_captured = false;
    return p->vm->compiler->local_count;
}

static void bl_parser_declvar(b_parser* p)
{
    // global variables are implicitly declared...
    if(p->vm->compiler->scope_depth == 0)
        return;

    b_token* name = &p->previous;

    for(int i = p->vm->compiler->local_count - 1; i >= 0; i--)
    {
        b_local* local = &p->vm->compiler->locals[i];
        if(local->depth != -1 && local->depth < p->vm->compiler->scope_depth)
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

static int bl_parser_parsevar(b_parser* p, const char* message)
{
    bl_parser_consume(p, IDENTIFIER_TOKEN, message);

    bl_parser_declvar(p);
    if(p->vm->compiler->scope_depth > 0)// we are in a local scope...
        return 0;

    return bl_parser_identconst(p, &p->previous);
}

static void bl_parser_markinit(b_parser* p)
{
    if(p->vm->compiler->scope_depth == 0)
        return;

    p->vm->compiler->locals[p->vm->compiler->local_count - 1].depth = p->vm->compiler->scope_depth;
}

static void bl_parser_defvar(b_parser* p, int global)
{
    if(p->vm->compiler->scope_depth > 0)
    {// we are in a local scope...
        bl_parser_markinit(p);
        return;
    }

    bl_parser_emitbyte_and_short(p, OP_DEFINE_GLOBAL, global);
}

static b_token bl_parser_synthtoken(const char* name)
{
    b_token token;
    token.start = name;
    token.length = (int)strlen(name);
    return token;
}

static b_obj_func* bl_compiler_end(b_parser* p)
{
    bl_parser_emitreturn(p);
    b_obj_func* function = p->vm->compiler->currfunc;

    if(!p->had_error && p->vm->should_print_bytecode)
    {
        disassemble_blob(bl_parser_currentblob(p), function->name == NULL ? p->module->file : function->name->chars);
    }

    p->vm->compiler = p->vm->compiler->enclosing;
    return function;
}

static void bl_parser_beginscope(b_parser* p)
{
    p->vm->compiler->scope_depth++;
}

static void bl_parser_endscope(b_parser* p)
{
    p->vm->compiler->scope_depth--;

    // remove all variables declared in scope while exiting...
    while(p->vm->compiler->local_count > 0 && p->vm->compiler->locals[p->vm->compiler->local_count - 1].depth > p->vm->compiler->scope_depth)
    {
        if(p->vm->compiler->locals[p->vm->compiler->local_count - 1].is_captured)
        {
            bl_parser_emitbyte(p, OP_CLOSE_UP_VALUE);
        }
        else
        {
            bl_parser_emitbyte(p, OP_POP);
        }
        p->vm->compiler->local_count--;
    }
}

static void bl_parser_discardlocal(b_parser* p, int depth)
{
    if(p->vm->compiler->scope_depth == -1)
    {
        bl_parser_raiseerror(p, "cannot exit top-level scope");
    }
    for(int i = p->vm->compiler->local_count - 1; i >= 0 && p->vm->compiler->locals[i].depth > depth; i--)
    {
        if(p->vm->compiler->locals[i].is_captured)
        {
            bl_parser_emitbyte(p, OP_CLOSE_UP_VALUE);
        }
        else
        {
            bl_parser_emitbyte(p, OP_POP);
        }
    }
}

static void bl_parser_endloop(b_parser* p)
{
    // find all OP_BREAK_PL placeholder and replace with the appropriate jump...
    int i = p->innermost_loop_start;

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

static void bl_parser_rulebinary(b_parser* p, b_token previous, bool can_assign)
{
    b_tkn_type op = p->previous.type;

    // compile the right operand
    b_parse_rule* rule = get_rule(op);
    parse_precedence(p, (b_precedence)(rule->precedence + 1));

    // emit the operator instruction
    switch(op)
    {
        case PLUS_TOKEN:
            bl_parser_emitbyte(p, OP_ADD);
            break;
        case MINUS_TOKEN:
            bl_parser_emitbyte(p, OP_SUBTRACT);
            break;
        case MULTIPLY_TOKEN:
            bl_parser_emitbyte(p, OP_MULTIPLY);
            break;
        case DIVIDE_TOKEN:
            bl_parser_emitbyte(p, OP_DIVIDE);
            break;
        case PERCENT_TOKEN:
            bl_parser_emitbyte(p, OP_REMINDER);
            break;
        case POW_TOKEN:
            bl_parser_emitbyte(p, OP_POW);
            break;
        case FLOOR_TOKEN:
            bl_parser_emitbyte(p, OP_F_DIVIDE);
            break;

            // equality
        case EQUAL_EQ_TOKEN:
            bl_parser_emitbyte(p, OP_EQUAL);
            break;
        case BANG_EQ_TOKEN:
            bl_parser_emitbytes(p, OP_EQUAL, OP_NOT);
            break;
        case GREATER_TOKEN:
            bl_parser_emitbyte(p, OP_GREATER);
            break;
        case GREATER_EQ_TOKEN:
            bl_parser_emitbytes(p, OP_LESS, OP_NOT);
            break;
        case LESS_TOKEN:
            bl_parser_emitbyte(p, OP_LESS);
            break;
        case LESS_EQ_TOKEN:
            bl_parser_emitbytes(p, OP_GREATER, OP_NOT);
            break;

            // bitwise
        case AMP_TOKEN:
            bl_parser_emitbyte(p, OP_AND);
            break;

        case BAR_TOKEN:
            bl_parser_emitbyte(p, OP_OR);
            break;

        case XOR_TOKEN:
            bl_parser_emitbyte(p, OP_XOR);
            break;

        case LSHIFT_TOKEN:
            bl_parser_emitbyte(p, OP_LSHIFT);
            break;

        case RSHIFT_TOKEN:
            bl_parser_emitbyte(p, OP_RSHIFT);
            break;

            // range
        case RANGE_TOKEN:
            bl_parser_emitbyte(p, OP_RANGE);
            break;

        default:
            break;
    }
}

static uint8_t bl_parser_parsearglist(b_parser* p)
{
    uint8_t arg_count = 0;
    if(!bl_parser_check(p, RPAREN_TOKEN))
    {
        do
        {
            bl_parser_ignorespace(p);
            bl_parser_parseexpr(p);
            if(arg_count == MAX_FUNCTION_PARAMETERS)
            {
                bl_parser_raiseerror(p, "cannot have more than %d arguments to a function", MAX_FUNCTION_PARAMETERS);
            }
            arg_count++;
        } while(bl_parser_match(p, COMMA_TOKEN));
    }
    bl_parser_ignorespace(p);
    bl_parser_consume(p, RPAREN_TOKEN, "expected ')' after argument list");
    return arg_count;
}

static void bl_parser_rulecall(b_parser* p, b_token previous, bool can_assign)
{
    uint8_t arg_count = bl_parser_parsearglist(p);
    bl_parser_emitbytes(p, OP_CALL, arg_count);
}

static void bl_parser_ruleliteral(b_parser* p, bool can_assign)
{
    switch(p->previous.type)
    {
        case NIL_TOKEN:
            bl_parser_emitbyte(p, OP_NIL);
            break;
        case TRUE_TOKEN:
            bl_parser_emitbyte(p, OP_TRUE);
            break;
        case FALSE_TOKEN:
            bl_parser_emitbyte(p, OP_FALSE);
            break;
        default:
            return;
    }
}

static void bl_parser_parseassign(b_parser* p, uint8_t real_op, uint8_t get_op, uint8_t set_op, int arg)
{
    p->repl_can_echo = false;
    if(get_op == OP_GET_PROPERTY || get_op == OP_GET_SELF_PROPERTY)
    {
        bl_parser_emitbyte(p, OP_DUP);
    }

    if(arg != -1)
    {
        bl_parser_emitbyte_and_short(p, get_op, arg);
    }
    else
    {
        bl_parser_emitbytes(p, get_op, 1);
    }

    bl_parser_parseexpr(p);
    bl_parser_emitbyte(p, real_op);
    if(arg != -1)
    {
        bl_parser_emitbyte_and_short(p, set_op, (uint16_t)arg);
    }
    else
    {
        bl_parser_emitbyte(p, set_op);
    }
}

static void bl_parser_doassign(b_parser* p, uint8_t get_op, uint8_t set_op, int arg, bool can_assign)
{
    if(can_assign && bl_parser_match(p, EQUAL_TOKEN))
    {
        p->repl_can_echo = false;
        bl_parser_parseexpr(p);
        if(arg != -1)
        {
            bl_parser_emitbyte_and_short(p, set_op, (uint16_t)arg);
        }
        else
        {
            bl_parser_emitbyte(p, set_op);
        }
    }
    else if(can_assign && bl_parser_match(p, PLUS_EQ_TOKEN))
    {
        bl_parser_parseassign(p, OP_ADD, get_op, set_op, arg);
    }
    else if(can_assign && bl_parser_match(p, MINUS_EQ_TOKEN))
    {
        bl_parser_parseassign(p, OP_SUBTRACT, get_op, set_op, arg);
    }
    else if(can_assign && bl_parser_match(p, MULTIPLY_EQ_TOKEN))
    {
        bl_parser_parseassign(p, OP_MULTIPLY, get_op, set_op, arg);
    }
    else if(can_assign && bl_parser_match(p, DIVIDE_EQ_TOKEN))
    {
        bl_parser_parseassign(p, OP_DIVIDE, get_op, set_op, arg);
    }
    else if(can_assign && bl_parser_match(p, POW_EQ_TOKEN))
    {
        bl_parser_parseassign(p, OP_POW, get_op, set_op, arg);
    }
    else if(can_assign && bl_parser_match(p, PERCENT_EQ_TOKEN))
    {
        bl_parser_parseassign(p, OP_REMINDER, get_op, set_op, arg);
    }
    else if(can_assign && bl_parser_match(p, FLOOR_EQ_TOKEN))
    {
        bl_parser_parseassign(p, OP_F_DIVIDE, get_op, set_op, arg);
    }
    else if(can_assign && bl_parser_match(p, AMP_EQ_TOKEN))
    {
        bl_parser_parseassign(p, OP_AND, get_op, set_op, arg);
    }
    else if(can_assign && bl_parser_match(p, BAR_EQ_TOKEN))
    {
        bl_parser_parseassign(p, OP_OR, get_op, set_op, arg);
    }
    else if(can_assign && bl_parser_match(p, TILDE_EQ_TOKEN))
    {
        bl_parser_parseassign(p, OP_BIT_NOT, get_op, set_op, arg);
    }
    else if(can_assign && bl_parser_match(p, XOR_EQ_TOKEN))
    {
        bl_parser_parseassign(p, OP_XOR, get_op, set_op, arg);
    }
    else if(can_assign && bl_parser_match(p, LSHIFT_EQ_TOKEN))
    {
        bl_parser_parseassign(p, OP_LSHIFT, get_op, set_op, arg);
    }
    else if(can_assign && bl_parser_match(p, RSHIFT_EQ_TOKEN))
    {
        bl_parser_parseassign(p, OP_RSHIFT, get_op, set_op, arg);
    }
    else if(can_assign && bl_parser_match(p, INCREMENT_TOKEN))
    {
        p->repl_can_echo = false;
        if(get_op == OP_GET_PROPERTY || get_op == OP_GET_SELF_PROPERTY)
        {
            bl_parser_emitbyte(p, OP_DUP);
        }

        if(arg != -1)
        {
            bl_parser_emitbyte_and_short(p, get_op, arg);
        }
        else
        {
            bl_parser_emitbytes(p, get_op, 1);
        }

        bl_parser_emitbytes(p, OP_ONE, OP_ADD);
        bl_parser_emitbyte_and_short(p, set_op, (uint16_t)arg);
    }
    else if(can_assign && bl_parser_match(p, DECREMENT_TOKEN))
    {
        p->repl_can_echo = false;
        if(get_op == OP_GET_PROPERTY || get_op == OP_GET_SELF_PROPERTY)
        {
            bl_parser_emitbyte(p, OP_DUP);
        }

        if(arg != -1)
        {
            bl_parser_emitbyte_and_short(p, get_op, arg);
        }
        else
        {
            bl_parser_emitbytes(p, get_op, 1);
        }

        bl_parser_emitbytes(p, OP_ONE, OP_SUBTRACT);
        bl_parser_emitbyte_and_short(p, set_op, (uint16_t)arg);
    }
    else
    {
        if(arg != -1)
        {
            if(get_op == OP_GET_INDEX || get_op == OP_GET_RANGED_INDEX)
            {
                bl_parser_emitbytes(p, get_op, (uint8_t)0);
            }
            else
            {
                bl_parser_emitbyte_and_short(p, get_op, (uint16_t)arg);
            }
        }
        else
        {
            bl_parser_emitbytes(p, get_op, (uint8_t)0);
        }
    }
}

static void bl_parser_ruledot(b_parser* p, b_token previous, bool can_assign)
{
    bl_parser_ignorespace(p);
    bl_parser_consume(p, IDENTIFIER_TOKEN, "expected property name after '.'");
    int name = bl_parser_identconst(p, &p->previous);

    if(bl_parser_match(p, LPAREN_TOKEN))
    {
        uint8_t arg_count = bl_parser_parsearglist(p);
        if(p->current_class != NULL && (previous.type == SELF_TOKEN || bl_parser_identsequal(&p->previous, &p->current_class->name)))
        {
            bl_parser_emitbyte_and_short(p, OP_INVOKE_SELF, name);
        }
        else
        {
            bl_parser_emitbyte_and_short(p, OP_INVOKE, name);
        }
        bl_parser_emitbyte(p, arg_count);
    }
    else
    {
        b_code get_op = OP_GET_PROPERTY, set_op = OP_SET_PROPERTY;

        if(p->current_class != NULL && (previous.type == SELF_TOKEN || bl_parser_identsequal(&p->previous, &p->current_class->name)))
        {
            get_op = OP_GET_SELF_PROPERTY;
        }

        bl_parser_doassign(p, get_op, set_op, name, can_assign);
    }
}

static void bl_parser_namedvar(b_parser* p, b_token name, bool can_assign)
{
    uint8_t get_op, set_op;
    int arg = bl_compiler_resolvelocal(p, p->vm->compiler, &name);
    if(arg != -1)
    {
        get_op = OP_GET_LOCAL;
        set_op = OP_SET_LOCAL;
    }
    else if((arg = bl_compiler_resolveupvalue(p, p->vm->compiler, &name)) != -1)
    {
        get_op = OP_GET_UP_VALUE;
        set_op = OP_SET_UP_VALUE;
    }
    else
    {
        arg = bl_parser_identconst(p, &name);
        get_op = OP_GET_GLOBAL;
        set_op = OP_SET_GLOBAL;
    }

    bl_parser_doassign(p, get_op, set_op, arg, can_assign);
}

static void bl_parser_rulelist(b_parser* p, bool can_assign)
{
    bl_parser_emitbyte(p, OP_NIL);// placeholder for the list

    int count = 0;
    if(!bl_parser_check(p, RBRACKET_TOKEN))
    {
        do
        {
            bl_parser_ignorespace(p);
            bl_parser_parseexpr(p);
            bl_parser_ignorespace(p);
            count++;
        } while(bl_parser_match(p, COMMA_TOKEN));
    }
    bl_parser_ignorespace(p);
    bl_parser_consume(p, RBRACKET_TOKEN, "expected ']' at end of list");

    bl_parser_emitbyte_and_short(p, OP_LIST, count);
}

static void bl_parser_ruledict(b_parser* p, bool can_assign)
{
    bl_parser_emitbyte(p, OP_NIL);// placeholder for the dictionary

    int item_count = 0;
    if(!bl_parser_check(p, RBRACE_TOKEN))
    {
        do
        {
            bl_parser_ignorespace(p);

            if(!bl_parser_check(p, RBRACE_TOKEN))
            {// allow last pair to end with a comma
                if(bl_parser_check(p, IDENTIFIER_TOKEN))
                {
                    bl_parser_consume(p, IDENTIFIER_TOKEN, "");
                    bl_parser_emitconstant(p, OBJ_VAL(copy_string(p->vm, p->previous.start, p->previous.length)));
                }
                else
                {
                    bl_parser_parseexpr(p);
                }
                bl_parser_ignorespace(p);
                bl_parser_consume(p, COLON_TOKEN, "expected ':' after dictionary key");
                bl_parser_ignorespace(p);

                bl_parser_parseexpr(p);
                item_count++;
            }
        } while(bl_parser_match(p, COMMA_TOKEN));
    }
    bl_parser_ignorespace(p);
    bl_parser_consume(p, RBRACE_TOKEN, "expected '}' after dictionary");

    bl_parser_emitbyte_and_short(p, OP_DICT, item_count);
}

static void bl_parser_ruleindexing(b_parser* p, b_token previous, bool can_assign)
{
    bool assignable = true, comma_match = false;
    uint8_t get_op = OP_GET_INDEX;
    if(bl_parser_match(p, COMMA_TOKEN))
    {
        bl_parser_emitbyte(p, OP_NIL);
        comma_match = true;
        get_op = OP_GET_RANGED_INDEX;
    }
    else
    {
        bl_parser_parseexpr(p);
    }

    if(!bl_parser_match(p, RBRACKET_TOKEN))
    {
        get_op = OP_GET_RANGED_INDEX;
        if(!comma_match)
        {
            bl_parser_consume(p, COMMA_TOKEN, "expecting ',' or ']'");
        }
        if(bl_parser_match(p, RBRACKET_TOKEN))
        {
            bl_parser_emitbyte(p, OP_NIL);
        }
        else
        {
            bl_parser_parseexpr(p);
            bl_parser_consume(p, RBRACKET_TOKEN, "expected ']' after indexing");
        }
        assignable = false;
    }
    else
    {
        if(comma_match)
        {
            bl_parser_emitbyte(p, OP_NIL);
        }
    }

    bl_parser_doassign(p, get_op, OP_SET_INDEX, -1, assignable);
}

static void bl_parser_rulevariable(b_parser* p, bool can_assign)
{
    bl_parser_namedvar(p, p->previous, can_assign);
}

static void bl_parser_ruleself(b_parser* p, bool can_assign)
{
    if(p->current_class == NULL)
    {
        bl_parser_raiseerror(p, "cannot use keyword 'self' outside of a class");
        return;
    }
    bl_parser_rulevariable(p, false);
}

static void bl_parser_ruleparent(b_parser* p, bool can_assign)
{
    if(p->current_class == NULL)
    {
        bl_parser_raiseerror(p, "cannot use keyword 'parent' outside of a class");
    }
    else if(!p->current_class->has_superclass)
    {
        bl_parser_raiseerror(p, "cannot use keyword 'parent' in a class without a parent");
    }

    int name = -1;
    bool invself = false;

    if(!bl_parser_check(p, LPAREN_TOKEN))
    {
        bl_parser_consume(p, DOT_TOKEN, "expected '.' or '(' after parent");
        bl_parser_consume(p, IDENTIFIER_TOKEN, "expected parent class method name after .");
        name = bl_parser_identconst(p, &p->previous);
    }
    else
    {
        invself = true;
    }

    bl_parser_namedvar(p, bl_parser_synthtoken("self"), false);

    if(bl_parser_match(p, LPAREN_TOKEN))
    {
        uint8_t arg_count = bl_parser_parsearglist(p);
        bl_parser_namedvar(p, bl_parser_synthtoken("parent"), false);
        if(!invself)
        {
            bl_parser_emitbyte_and_short(p, OP_SUPER_INVOKE, name);
            bl_parser_emitbyte(p, arg_count);
        }
        else
        {
            bl_parser_emitbytes(p, OP_SUPER_INVOKE_SELF, arg_count);
        }
    }
    else
    {
        bl_parser_namedvar(p, bl_parser_synthtoken("parent"), false);
        bl_parser_emitbyte_and_short(p, OP_GET_SUPER, name);
    }
}

static void bl_parser_rulegrouping(b_parser* p, bool can_assign)
{
    bl_parser_ignorespace(p);
    bl_parser_parseexpr(p);
    bl_parser_ignorespace(p);
    bl_parser_consume(p, RPAREN_TOKEN, "expected ')' after grouped expression");
}

static b_value bl_parser_compilenumber(b_parser* p)
{
    if(p->previous.type == BIN_NUMBER_TOKEN)
    {
        long long value = strtoll(p->previous.start + 2, NULL, 2);
        return NUMBER_VAL(value);
    }
    else if(p->previous.type == OCT_NUMBER_TOKEN)
    {
        long value = strtol(p->previous.start + 2, NULL, 8);
        return NUMBER_VAL(value);
    }
    else if(p->previous.type == HEX_NUMBER_TOKEN)
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

static void bl_parser_rulenumber(b_parser* p, bool can_assign)
{
    bl_parser_emitconstant(p, bl_parser_compilenumber(p));
}

// Reads the next character, which should be a hex digit (0-9, a-f, or A-F) and
// returns its numeric value. If the character isn't a hex digit, returns -1.
static int read_hex_digit(char c)
{
    if(c >= '0' && c <= '9')
        return c - '0';
    if(c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if(c >= 'A' && c <= 'F')
        return c - 'A' + 10;

    return -1;
}

// Reads [digits] hex digits in a string literal and returns their number value.
static int read_hex_escape(b_parser* p, char* str, int index, int count)
{
    int value = 0;
    int i = 0, digit = 0;
    for(; i < count; i++)
    {
        digit = read_hex_digit(str[index + i + 2]);
        if(digit == -1)
        {
            bl_parser_raiseerror(p, "invalid escape sequence");
        }
        value = (value * 16) | digit;
    }
    if(count == 4 && (digit = read_hex_digit(str[index + i + 2])) != -1)
    {
        value = (value * 16) | digit;
    }
    return value;
}

static int read_unicode_escape(b_parser* p, char* string, char* real_string, int number_bytes, int real_index, int index)
{
    int value = read_hex_escape(p, real_string, real_index, number_bytes);
    int count = utf8_number_bytes(value);
    if(count == -1)
    {
        bl_parser_raiseerror(p, "cannot encode a negative unicode value");
    }
    if(value > 65535)// check for greater that \uffff
        count++;
    if(count != 0)
    {
        memcpy(string + index, utf8_encode(value), (size_t)count + 1);
    }
    /* if (value > 65535) // but greater than \uffff doesn't occupy any extra byte
    count--; */
    return count;
}

static char* compile_string(b_parser* p, int* length)
{
    char* str = (char*)malloc((((size_t)p->previous.length - 2) + 1) * sizeof(char));
    char* real = (char*)p->previous.start + 1;

    int real_length = p->previous.length - 2, k = 0;

    for(int i = 0; i < real_length; i++, k++)
    {
        char c = real[i];
        if(c == '\\' && i < real_length - 1)
        {
            switch(real[i + 1])
            {
                case '0':
                    c = '\0';
                    break;
                case '$':
                    c = '$';
                    break;
                case '\'':
                    c = '\'';
                    break;
                case '"':
                    c = '"';
                    break;
                case 'a':
                    c = '\a';
                    break;
                case 'b':
                    c = '\b';
                    break;
                case 'f':
                    c = '\f';
                    break;
                case 'n':
                    c = '\n';
                    break;
                case 'r':
                    c = '\r';
                    break;
                case 't':
                    c = '\t';
                    break;
                case '\\':
                    c = '\\';
                    break;
                case 'v':
                    c = '\v';
                    break;
                case 'x':
                {
                    k += read_unicode_escape(p, str, real, 2, i, k) - 1;
                    i += 3;
                    continue;
                }
                case 'u':
                {
                    int count = read_unicode_escape(p, str, real, 4, i, k);
                    k += count > 4 ? count - 2 : count - 1;
                    i += count > 4 ? 6 : 5;
                    continue;
                }
                case 'U':
                {
                    int count = read_unicode_escape(p, str, real, 8, i, k);
                    k += count > 4 ? count - 2 : count - 1;
                    i += 9;
                    continue;
                }
                default:
                    i--;
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

static void bl_parser_rulestring(b_parser* p, bool can_assign)
{
    int length;
    char* str = compile_string(p, &length);
    bl_parser_emitconstant(p, OBJ_VAL(take_string(p->vm, str, length)));
}

static void bl_parser_rulestrinterpol(b_parser* p, bool can_assign)
{
    int count = 0;
    do
    {
        bool do_add = false;
        bool string_matched = false;

        if(p->previous.length - 2 > 0)
        {
            bl_parser_rulestring(p, can_assign);
            do_add = true;
            string_matched = true;

            if(count > 0)
            {
                bl_parser_emitbyte(p, OP_ADD);
            }
        }

        bl_parser_parseexpr(p);
        bl_parser_emitbyte(p, OP_STRINGIFY);

        if(do_add || (count >= 1 && string_matched == false))
        {
            bl_parser_emitbyte(p, OP_ADD);
        }
        count++;
    } while(bl_parser_match(p, INTERPOLATION_TOKEN));

    bl_parser_consume(p, LITERAL_TOKEN, "unterminated string interpolation");

    if(p->previous.length - 2 > 0)
    {
        bl_parser_rulestring(p, can_assign);
        bl_parser_emitbyte(p, OP_ADD);
    }
}

static void bl_parser_ruleunary(b_parser* p, bool can_assign)
{
    b_tkn_type op = p->previous.type;

    // compile the expression
    parse_precedence(p, PREC_UNARY);

    // emit instruction
    switch(op)
    {
        case MINUS_TOKEN:
            bl_parser_emitbyte(p, OP_NEGATE);
            break;
        case BANG_TOKEN:
            bl_parser_emitbyte(p, OP_NOT);
            break;
        case TILDE_TOKEN:
            bl_parser_emitbyte(p, OP_BIT_NOT);
            break;

        default:
            break;
    }
}

static void bl_parser_ruleand(b_parser* p, b_token previous, bool can_assign)
{
    int end_jump = bl_parser_emitjump(p, OP_JUMP_IF_FALSE);

    bl_parser_emitbyte(p, OP_POP);
    parse_precedence(p, PREC_AND);

    bl_parser_patchjump(p, end_jump);
}

static void bl_parser_ruleor(b_parser* p, b_token previous, bool can_assign)
{
    int else_jump = bl_parser_emitjump(p, OP_JUMP_IF_FALSE);
    int end_jump = bl_parser_emitjump(p, OP_JUMP);

    bl_parser_patchjump(p, else_jump);
    bl_parser_emitbyte(p, OP_POP);

    parse_precedence(p, PREC_OR);
    bl_parser_patchjump(p, end_jump);
}

static void bl_parser_ruleconditional(b_parser* p, b_token previous, bool can_assign)
{
    int then_jump = bl_parser_emitjump(p, OP_JUMP_IF_FALSE);
    bl_parser_emitbyte(p, OP_POP);

    bl_parser_ignorespace(p);
    // compile the then expression
    parse_precedence(p, PREC_CONDITIONAL);
    bl_parser_ignorespace(p);

    int else_jump = bl_parser_emitjump(p, OP_JUMP);

    bl_parser_patchjump(p, then_jump);
    bl_parser_emitbyte(p, OP_POP);

    bl_parser_consume(p, COLON_TOKEN, "expected matching ':' after '?' conditional");
    bl_parser_ignorespace(p);
    // compile the else expression
    // here we parse at PREC_ASSIGNMENT precedence as
    // linear conditionals can be nested.
    parse_precedence(p, PREC_ASSIGNMENT);

    bl_parser_patchjump(p, else_jump);
}

static const b_parse_rule parse_rules[] = {
    // symbols
    [NEWLINE_TOKEN] = { NULL, NULL, PREC_NONE },// (
    [LPAREN_TOKEN] = { bl_parser_rulegrouping, bl_parser_rulecall, PREC_CALL },// (
    [RPAREN_TOKEN] = { NULL, NULL, PREC_NONE },// )
    [LBRACKET_TOKEN] = { bl_parser_rulelist, bl_parser_ruleindexing, PREC_CALL },// [
    [RBRACKET_TOKEN] = { NULL, NULL, PREC_NONE },// ]
    [LBRACE_TOKEN] = { bl_parser_ruledict, NULL, PREC_NONE },// {
    [RBRACE_TOKEN] = { NULL, NULL, PREC_NONE },// }
    [SEMICOLON_TOKEN] = { NULL, NULL, PREC_NONE },// ;
    [COMMA_TOKEN] = { NULL, NULL, PREC_NONE },// ,
    [BACKSLASH_TOKEN] = { NULL, NULL, PREC_NONE },// '\'
    [BANG_TOKEN] = { bl_parser_ruleunary, NULL, PREC_NONE },// !
    [BANG_EQ_TOKEN] = { NULL, bl_parser_rulebinary, PREC_EQUALITY },// !=
    [COLON_TOKEN] = { NULL, NULL, PREC_NONE },// :
    [AT_TOKEN] = { NULL, NULL, PREC_NONE },// @
    [DOT_TOKEN] = { NULL, bl_parser_ruledot, PREC_CALL },// .
    [RANGE_TOKEN] = { NULL, bl_parser_rulebinary, PREC_RANGE },// ..
    [TRI_DOT_TOKEN] = { NULL, NULL, PREC_NONE },// ...
    [PLUS_TOKEN] = { bl_parser_ruleunary, bl_parser_rulebinary, PREC_TERM },// +
    [PLUS_EQ_TOKEN] = { NULL, NULL, PREC_NONE },// +=
    [INCREMENT_TOKEN] = { NULL, NULL, PREC_NONE },// ++
    [MINUS_TOKEN] = { bl_parser_ruleunary, bl_parser_rulebinary, PREC_TERM },// -
    [MINUS_EQ_TOKEN] = { NULL, NULL, PREC_NONE },// -=
    [DECREMENT_TOKEN] = { NULL, NULL, PREC_NONE },// --
    [MULTIPLY_TOKEN] = { NULL, bl_parser_rulebinary, PREC_FACTOR },// *
    [MULTIPLY_EQ_TOKEN] = { NULL, NULL, PREC_NONE },// *=
    [POW_TOKEN] = { NULL, bl_parser_rulebinary, PREC_FACTOR },// **
    [POW_EQ_TOKEN] = { NULL, NULL, PREC_NONE },// **=
    [DIVIDE_TOKEN] = { NULL, bl_parser_rulebinary, PREC_FACTOR },// '/'
    [DIVIDE_EQ_TOKEN] = { NULL, NULL, PREC_NONE },// '/='
    [FLOOR_TOKEN] = { NULL, bl_parser_rulebinary, PREC_FACTOR },// '//'
    [FLOOR_EQ_TOKEN] = { NULL, NULL, PREC_NONE },// '//='
    [EQUAL_TOKEN] = { NULL, NULL, PREC_NONE },// =
    [EQUAL_EQ_TOKEN] = { NULL, bl_parser_rulebinary, PREC_EQUALITY },// ==
    [LESS_TOKEN] = { NULL, bl_parser_rulebinary, PREC_COMPARISON },// <
    [LESS_EQ_TOKEN] = { NULL, bl_parser_rulebinary, PREC_COMPARISON },// <=
    [LSHIFT_TOKEN] = { NULL, bl_parser_rulebinary, PREC_SHIFT },// <<
    [LSHIFT_EQ_TOKEN] = { NULL, NULL, PREC_NONE },// <<=
    [GREATER_TOKEN] = { NULL, bl_parser_rulebinary, PREC_COMPARISON },// >
    [GREATER_EQ_TOKEN] = { NULL, bl_parser_rulebinary, PREC_COMPARISON },// >=
    [RSHIFT_TOKEN] = { NULL, bl_parser_rulebinary, PREC_SHIFT },// >>
    [RSHIFT_EQ_TOKEN] = { NULL, NULL, PREC_NONE },// >>=
    [PERCENT_TOKEN] = { NULL, bl_parser_rulebinary, PREC_FACTOR },// %
    [PERCENT_EQ_TOKEN] = { NULL, NULL, PREC_NONE },// %=
    [AMP_TOKEN] = { NULL, bl_parser_rulebinary, PREC_BIT_AND },// &
    [AMP_EQ_TOKEN] = { NULL, NULL, PREC_NONE },// &=
    [BAR_TOKEN] = { bl_parser_ruleanon, bl_parser_rulebinary, PREC_BIT_OR },// |
    [BAR_EQ_TOKEN] = { NULL, NULL, PREC_NONE },// |=
    [TILDE_TOKEN] = { bl_parser_ruleunary, NULL, PREC_UNARY },// ~
    [TILDE_EQ_TOKEN] = { NULL, NULL, PREC_NONE },// ~=
    [XOR_TOKEN] = { NULL, bl_parser_rulebinary, PREC_BIT_XOR },// ^
    [XOR_EQ_TOKEN] = { NULL, NULL, PREC_NONE },// ^=
    [QUESTION_TOKEN] = { NULL, bl_parser_ruleconditional, PREC_CONDITIONAL },// ??

    // keywords
    [AND_TOKEN] = { NULL, bl_parser_ruleand, PREC_AND },
    [AS_TOKEN] = { NULL, NULL, PREC_NONE },
    [ASSERT_TOKEN] = { NULL, NULL, PREC_NONE },
    [BREAK_TOKEN] = { NULL, NULL, PREC_NONE },
    [CLASS_TOKEN] = { NULL, NULL, PREC_NONE },
    [CONTINUE_TOKEN] = { NULL, NULL, PREC_NONE },
    [DEF_TOKEN] = { NULL, NULL, PREC_NONE },
    [DEFAULT_TOKEN] = { NULL, NULL, PREC_NONE },
    [DIE_TOKEN] = { NULL, NULL, PREC_NONE },
    [DO_TOKEN] = { NULL, NULL, PREC_NONE },
    [ECHO_TOKEN] = { NULL, NULL, PREC_NONE },
    [ELSE_TOKEN] = { NULL, NULL, PREC_NONE },
    [FALSE_TOKEN] = { bl_parser_ruleliteral, NULL, PREC_NONE },
    [FOR_TOKEN] = { NULL, NULL, PREC_NONE },
    [IF_TOKEN] = { NULL, NULL, PREC_NONE },
    [IMPORT_TOKEN] = { NULL, NULL, PREC_NONE },
    [IN_TOKEN] = { NULL, NULL, PREC_NONE },
    [ITER_TOKEN] = { NULL, NULL, PREC_NONE },
    [VAR_TOKEN] = { NULL, NULL, PREC_NONE },
    [NIL_TOKEN] = { bl_parser_ruleliteral, NULL, PREC_NONE },
    [OR_TOKEN] = { NULL, bl_parser_ruleor, PREC_OR },
    [PARENT_TOKEN] = { bl_parser_ruleparent, NULL, PREC_NONE },
    [RETURN_TOKEN] = { NULL, NULL, PREC_NONE },
    [SELF_TOKEN] = { bl_parser_ruleself, NULL, PREC_NONE },
    [STATIC_TOKEN] = { NULL, NULL, PREC_NONE },
    [TRUE_TOKEN] = { bl_parser_ruleliteral, NULL, PREC_NONE },
    [USING_TOKEN] = { NULL, NULL, PREC_NONE },
    [WHEN_TOKEN] = { NULL, NULL, PREC_NONE },
    [WHILE_TOKEN] = { NULL, NULL, PREC_NONE },
    [TRY_TOKEN] = { NULL, NULL, PREC_NONE },
    [CATCH_TOKEN] = { NULL, NULL, PREC_NONE },
    [FINALLY_TOKEN] = { NULL, NULL, PREC_NONE },

    // types token
    [LITERAL_TOKEN] = { bl_parser_rulestring, NULL, PREC_NONE },
    [REG_NUMBER_TOKEN] = { bl_parser_rulenumber, NULL, PREC_NONE },// regular numbers
    [BIN_NUMBER_TOKEN] = { bl_parser_rulenumber, NULL, PREC_NONE },// binary numbers
    [OCT_NUMBER_TOKEN] = { bl_parser_rulenumber, NULL, PREC_NONE },// octal numbers
    [HEX_NUMBER_TOKEN] = { bl_parser_rulenumber, NULL, PREC_NONE },// hexadecimal numbers
    [IDENTIFIER_TOKEN] = { bl_parser_rulevariable, NULL, PREC_NONE },
    [INTERPOLATION_TOKEN] = { bl_parser_rulestrinterpol, NULL, PREC_NONE },
    [EOF_TOKEN] = { NULL, NULL, PREC_NONE },

    // error
    [ERROR_TOKEN] = { NULL, NULL, PREC_NONE },
    [EMPTY_TOKEN] = { bl_parser_ruleliteral, NULL, PREC_NONE },
    [UNDEFINED_TOKEN] = { NULL, NULL, PREC_NONE },
};

static void do_parse_precedence(b_parser* p, b_precedence precedence)
{
    b_parse_prefix_fn prefix_rule = get_rule(p->previous.type)->prefix;

    if(prefix_rule == NULL)
    {
        bl_parser_raiseerror(p, "expected expression");
        return;
    }

    bool can_assign = precedence <= PREC_ASSIGNMENT;
    prefix_rule(p, can_assign);

    while(precedence <= get_rule(p->current.type)->precedence)
    {
        b_token previous = p->previous;
        bl_parser_ignorespace(p);
        bl_parser_advance(p);
        b_parse_infix_fn infix_rule = get_rule(p->previous.type)->infix;
        infix_rule(p, previous, can_assign);
    }

    if(can_assign && bl_parser_match(p, EQUAL_TOKEN))
    {
        bl_parser_raiseerror(p, "invalid assignment target");
    }
}

static void parse_precedence(b_parser* p, b_precedence precedence)
{
    if(bl_scanner_isatend(p->scanner) && p->vm->is_repl)
        return;

    bl_parser_ignorespace(p);

    if(bl_scanner_isatend(p->scanner) && p->vm->is_repl)
        return;

    bl_parser_advance(p);

    do_parse_precedence(p, precedence);
}

static void parse_precedence_no_advance(b_parser* p, b_precedence precedence)
{
    if(bl_scanner_isatend(p->scanner) && p->vm->is_repl)
        return;

    bl_parser_ignorespace(p);

    if(bl_scanner_isatend(p->scanner) && p->vm->is_repl)
        return;

    do_parse_precedence(p, precedence);
}

static b_parse_rule* get_rule(b_tkn_type type)
{
    return &parse_rules[type];
}

static void bl_parser_parseexpr(b_parser* p)
{
    parse_precedence(p, PREC_ASSIGNMENT);
}

static void bl_parser_parseblock(b_parser* p)
{
    p->block_count++;
    bl_parser_ignorespace(p);
    while(!bl_parser_check(p, RBRACE_TOKEN) && !bl_parser_check(p, EOF_TOKEN))
    {
        declaration(p);
    }
    p->block_count--;
    bl_parser_consume(p, RBRACE_TOKEN, "expected '}' after block");
}

static void function_args(b_parser* p)
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

        if(bl_parser_match(p, TRI_DOT_TOKEN))
        {
            p->vm->compiler->currfunc->is_variadic = true;
            bl_parser_addlocal(p, bl_parser_synthtoken("__args__"));
            bl_parser_defvar(p, 0);
            break;
        }

        int param_constant = bl_parser_parsevar(p, "expected parameter name");
        bl_parser_defvar(p, param_constant);
        bl_parser_ignorespace(p);
    } while(bl_parser_match(p, COMMA_TOKEN));
}

static void function_body(b_parser* p, b_compiler* compiler)
{
    // compile the body
    bl_parser_ignorespace(p);
    bl_parser_consume(p, LBRACE_TOKEN, "expected '{' before function body");
    bl_parser_parseblock(p);

    // create the function object
    b_obj_func* function = bl_compiler_end(p);

    bl_parser_emitbyte_and_short(p, OP_CLOSURE, bl_parser_makeconstant(p, OBJ_VAL(function)));

    for(int i = 0; i < function->up_value_count; i++)
    {
        bl_parser_emitbyte(p, compiler->up_values[i].is_local ? 1 : 0);
        bl_parser_emitshort(p, compiler->up_values[i].index);
    }
}

static void bl_parser_parsefunction(b_parser* p, b_func_type type)
{
    b_compiler compiler;
    bl_compiler_init(p, &compiler, type);
    bl_parser_beginscope(p);

    // compile parameter list
    bl_parser_consume(p, LPAREN_TOKEN, "expected '(' after function name");
    if(!bl_parser_check(p, RPAREN_TOKEN))
    {
        function_args(p);
    }
    bl_parser_consume(p, RPAREN_TOKEN, "expected ')' after function parameters");

    function_body(p, &compiler);
}

static void bl_parser_parsemethod(b_parser* p, b_token class_name, bool is_static)
{
    b_tkn_type tkns[] = { IDENTIFIER_TOKEN, DECORATOR_TOKEN };

    bl_parser_consumeor(p, "method name expected", tkns, 2);
    int constant = bl_parser_identconst(p, &p->previous);

    b_func_type type = is_static ? TYPE_STATIC : TYPE_METHOD;
    if(p->previous.length == class_name.length && memcmp(p->previous.start, class_name.start, class_name.length) == 0)
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

static void bl_parser_ruleanon(b_parser* p, bool can_assign)
{
    b_compiler compiler;
    bl_compiler_init(p, &compiler, TYPE_FUNCTION);
    bl_parser_beginscope(p);

    // compile parameter list
    if(!bl_parser_check(p, BAR_TOKEN))
    {
        function_args(p);
    }
    bl_parser_consume(p, BAR_TOKEN, "expected '|' after anonymous function parameters");

    function_body(p, &compiler);
}

static void bl_parser_parsefield(b_parser* p, bool is_static)
{
    bl_parser_consume(p, IDENTIFIER_TOKEN, "class property name expected");
    int field_constant = bl_parser_identconst(p, &p->previous);

    if(bl_parser_match(p, EQUAL_TOKEN))
    {
        bl_parser_parseexpr(p);
    }
    else
    {
        bl_parser_emitbyte(p, OP_NIL);
    }

    bl_parser_emitbyte_and_short(p, OP_CLASS_PROPERTY, field_constant);
    bl_parser_emitbyte(p, is_static ? 1 : 0);

    bl_parser_consumestmtend(p);
    bl_parser_ignorespace(p);
}

static void function_declaration(b_parser* p)
{
    int global = bl_parser_parsevar(p, "function name expected");
    bl_parser_markinit(p);
    bl_parser_parsefunction(p, TYPE_FUNCTION);
    bl_parser_defvar(p, global);
}

static void class_declaration(b_parser* p)
{
    bl_parser_consume(p, IDENTIFIER_TOKEN, "class name expected");
    int name_constant = bl_parser_identconst(p, &p->previous);
    b_token class_name = p->previous;
    bl_parser_declvar(p);

    bl_parser_emitbyte_and_short(p, OP_CLASS, name_constant);
    bl_parser_defvar(p, name_constant);

    b_class_compiler class_compiler;
    class_compiler.name = p->previous;
    class_compiler.has_superclass = false;
    class_compiler.enclosing = p->current_class;
    p->current_class = &class_compiler;

    if(bl_parser_match(p, LESS_TOKEN))
    {
        bl_parser_consume(p, IDENTIFIER_TOKEN, "name of superclass expected");
        bl_parser_rulevariable(p, false);

        if(bl_parser_identsequal(&class_name, &p->previous))
        {
            bl_parser_raiseerror(p, "class %.*s cannot inherit from itself", class_name.length, class_name.start);
        }

        bl_parser_beginscope(p);
        bl_parser_addlocal(p, bl_parser_synthtoken("parent"));
        bl_parser_defvar(p, 0);

        bl_parser_namedvar(p, class_name, false);
        bl_parser_emitbyte(p, OP_INHERIT);
        class_compiler.has_superclass = true;
    }

    bl_parser_namedvar(p, class_name, false);

    bl_parser_ignorespace(p);
    bl_parser_consume(p, LBRACE_TOKEN, "expected '{' before class body");
    bl_parser_ignorespace(p);
    while(!bl_parser_check(p, RBRACE_TOKEN) && !bl_parser_check(p, EOF_TOKEN))
    {
        bool is_static = false;
        if(bl_parser_match(p, STATIC_TOKEN))
            is_static = true;

        if(bl_parser_match(p, VAR_TOKEN))
        {
            bl_parser_parsefield(p, is_static);
        }
        else
        {
            bl_parser_parsemethod(p, class_name, is_static);
            bl_parser_ignorespace(p);
        }
    }
    bl_parser_consume(p, RBRACE_TOKEN, "expected '}' after class body");
    bl_parser_emitbyte(p, OP_POP);

    if(class_compiler.has_superclass)
    {
        bl_parser_endscope(p);
    }

    p->current_class = p->current_class->enclosing;
}

static void compile_var_declaration(b_parser* p, bool is_initializer)
{
    int total_parsed = 0;

    do
    {
        if(total_parsed > 0)
        {
            bl_parser_ignorespace(p);
        }
        int global = bl_parser_parsevar(p, "variable name expected");

        if(bl_parser_match(p, EQUAL_TOKEN))
        {
            bl_parser_parseexpr(p);
        }
        else
        {
            bl_parser_emitbyte(p, OP_NIL);
        }

        bl_parser_defvar(p, global);
        total_parsed++;
    } while(bl_parser_match(p, COMMA_TOKEN));

    if(!is_initializer)
    {
        bl_parser_consumestmtend(p);
    }
    else
    {
        bl_parser_consume(p, SEMICOLON_TOKEN, "expected ';' after initializer");
        bl_parser_ignorespace(p);
    }
}

static void var_declaration(b_parser* p)
{
    compile_var_declaration(p, false);
}

static void expression_statement(b_parser* p, bool is_initializer, bool semi)
{
    if(p->vm->is_repl && p->vm->compiler->scope_depth == 0)
    {
        p->repl_can_echo = true;
    }
    if(!semi)
    {
        bl_parser_parseexpr(p);
    }
    else
    {
        parse_precedence_no_advance(p, PREC_ASSIGNMENT);
    }
    if(!is_initializer)
    {
        if(p->repl_can_echo && p->vm->is_repl)
        {
            bl_parser_emitbyte(p, OP_ECHO);
            p->repl_can_echo = false;
        }
        else
        {
            bl_parser_emitbyte(p, OP_POP);
        }
        bl_parser_consumestmtend(p);
    }
    else
    {
        bl_parser_consume(p, SEMICOLON_TOKEN, "expected ';' after initializer");
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
static void iter_statement(b_parser* p)
{
    bl_parser_beginscope(p);

    // parse initializer...
    if(bl_parser_match(p, SEMICOLON_TOKEN))
    {
        // no initializer
    }
    else if(bl_parser_match(p, VAR_TOKEN))
    {
        compile_var_declaration(p, true);
    }
    else
    {
        expression_statement(p, true, false);
    }

    // keep a copy of the surrounding loop's start and depth
    int surrounding_loop_start = p->innermost_loop_start;
    int surrounding_scope_depth = p->innermost_loop_scope_depth;

    // update the parser's loop start and depth to the current
    p->innermost_loop_start = bl_parser_currentblob(p)->count;
    p->innermost_loop_scope_depth = p->vm->compiler->scope_depth;

    int exit_jump = -1;
    if(!bl_parser_match(p, SEMICOLON_TOKEN))
    {// the condition is optional
        bl_parser_parseexpr(p);
        bl_parser_consume(p, SEMICOLON_TOKEN, "expected ';' after condition");
        bl_parser_ignorespace(p);

        // jump out of the loop if the condition is false...
        exit_jump = bl_parser_emitjump(p, OP_JUMP_IF_FALSE);
        bl_parser_emitbyte(p, OP_POP);// pop the condition
    }

    // the iterator...
    if(!bl_parser_check(p, LBRACE_TOKEN))
    {
        int body_jump = bl_parser_emitjump(p, OP_JUMP);

        int increment_start = bl_parser_currentblob(p)->count;
        bl_parser_parseexpr(p);
        bl_parser_ignorespace(p);
        bl_parser_emitbyte(p, OP_POP);

        bl_parser_emitloop(p, p->innermost_loop_start);
        p->innermost_loop_start = increment_start;
        bl_parser_patchjump(p, body_jump);
    }

    bl_parser_parsestmt(p);

    bl_parser_emitloop(p, p->innermost_loop_start);

    if(exit_jump != -1)
    {
        bl_parser_patchjump(p, exit_jump);
        bl_parser_emitbyte(p, OP_POP);
    }

    bl_parser_endloop(p);

    // reset the loop start and scope depth to the surrounding value
    p->innermost_loop_start = surrounding_loop_start;
    p->innermost_loop_scope_depth = surrounding_scope_depth;

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
static void for_statement(b_parser* p)
{
    bl_parser_beginscope(p);

    // define @iter and @itern constant
    int iter__ = bl_parser_makeconstant(p, OBJ_VAL(copy_string(p->vm, "@iter", 5)));
    int iter_n__ = bl_parser_makeconstant(p, OBJ_VAL(copy_string(p->vm, "@itern", 6)));

    bl_parser_consume(p, IDENTIFIER_TOKEN, "expected variable name after 'for'");
    b_token key_token, value_token;

    if(!bl_parser_check(p, COMMA_TOKEN))
    {
        key_token = bl_parser_synthtoken(" _ ");
        value_token = p->previous;
    }
    else
    {
        key_token = p->previous;

        bl_parser_consume(p, COMMA_TOKEN, "");
        bl_parser_consume(p, IDENTIFIER_TOKEN, "expected variable name after ','");
        value_token = p->previous;
    }

    bl_parser_consume(p, IN_TOKEN, "expected 'in' after for loop variable(s)");
    bl_parser_ignorespace(p);

    // The space in the variable name ensures it won't collide with a user-defined
    // variable.
    b_token iterator_token = bl_parser_synthtoken(" iterator ");

    // Evaluate the sequence expression and store it in a hidden local variable.
    bl_parser_parseexpr(p);

    if(p->vm->compiler->local_count + 3 > UINT8_COUNT)
    {
        bl_parser_raiseerror(p, "cannot declare more than %d variables in one scope", UINT8_COUNT);
        return;
    }

    // add the iterator to the local scope
    int iterator_slot = bl_parser_addlocal(p, iterator_token) - 1;
    bl_parser_defvar(p, 0);

    // Create the key local variable.
    bl_parser_emitbyte(p, OP_NIL);
    int key_slot = bl_parser_addlocal(p, key_token) - 1;
    bl_parser_defvar(p, key_slot);

    // create the local value slot
    bl_parser_emitbyte(p, OP_NIL);
    int value_slot = bl_parser_addlocal(p, value_token) - 1;
    bl_parser_defvar(p, 0);

    int surrounding_loop_start = p->innermost_loop_start;
    int surrounding_scope_depth = p->innermost_loop_scope_depth;

    // we'll be jumping back to right before the
    // expression after the loop body
    p->innermost_loop_start = bl_parser_currentblob(p)->count;
    p->innermost_loop_scope_depth = p->vm->compiler->scope_depth;

    // key = iterable.iter_n__(key)
    bl_parser_emitbyte_and_short(p, OP_GET_LOCAL, iterator_slot);
    bl_parser_emitbyte_and_short(p, OP_GET_LOCAL, key_slot);
    bl_parser_emitbyte_and_short(p, OP_INVOKE, iter_n__);
    bl_parser_emitbyte(p, 1);
    bl_parser_emitbyte_and_short(p, OP_SET_LOCAL, key_slot);

    int false_jump = bl_parser_emitjump(p, OP_JUMP_IF_FALSE);
    bl_parser_emitbyte(p, OP_POP);

    // value = iterable.iter__(key)
    bl_parser_emitbyte_and_short(p, OP_GET_LOCAL, iterator_slot);
    bl_parser_emitbyte_and_short(p, OP_GET_LOCAL, key_slot);
    bl_parser_emitbyte_and_short(p, OP_INVOKE, iter__);
    bl_parser_emitbyte(p, 1);

    // Bind the loop value in its own scope. This ensures we get a fresh
    // variable each iteration so that closures for it don't all see the same one.
    bl_parser_beginscope(p);

    // update the value
    bl_parser_emitbyte_and_short(p, OP_SET_LOCAL, value_slot);
    bl_parser_emitbyte(p, OP_POP);

    bl_parser_parsestmt(p);

    bl_parser_endscope(p);

    bl_parser_emitloop(p, p->innermost_loop_start);

    bl_parser_patchjump(p, false_jump);
    bl_parser_emitbyte(p, OP_POP);

    bl_parser_endloop(p);

    p->innermost_loop_start = surrounding_loop_start;
    p->innermost_loop_scope_depth = surrounding_scope_depth;

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
static void using_statement(b_parser* p)
{
    bl_parser_parseexpr(p);// the expression
    bl_parser_consume(p, LBRACE_TOKEN, "expected '{' after using expression");
    bl_parser_ignorespace(p);

    int state = 0;// 0: before all cases, 1: before default, 2: after default
    int case_ends[MAX_USING_CASES];
    int case_count = 0;

    b_obj_switch* sw = new_switch(p->vm);
    push(p->vm, OBJ_VAL(sw));

    int switch_code = bl_parser_emitswitch(p);
    // bl_parser_emitbyte_and_short(p, OP_SWITCH, bl_parser_makeconstant(p, OBJ_VAL(sw)));
    int start_offset = bl_parser_currentblob(p)->count;

    while(!bl_parser_match(p, RBRACE_TOKEN) && !bl_parser_check(p, EOF_TOKEN))
    {
        if(bl_parser_match(p, WHEN_TOKEN) || bl_parser_match(p, DEFAULT_TOKEN))
        {
            b_tkn_type case_type = p->previous.type;

            if(state == 2)
            {
                bl_parser_raiseerror(p, "cannot have another case after a default case");
            }

            if(state == 1)
            {
                // at the end of the previous case, jump over the others...
                case_ends[case_count++] = bl_parser_emitjump(p, OP_JUMP);
            }

            if(case_type == WHEN_TOKEN)
            {
                state = 1;
                do
                {
                    bl_parser_advance(p);

                    b_value jump = NUMBER_VAL((double)bl_parser_currentblob(p)->count - (double)start_offset);

                    if(p->previous.type == TRUE_TOKEN)
                    {
                        table_set(p->vm, &sw->table, TRUE_VAL, jump);
                    }
                    else if(p->previous.type == FALSE_TOKEN)
                    {
                        table_set(p->vm, &sw->table, FALSE_VAL, jump);
                    }
                    else if(p->previous.type == LITERAL_TOKEN)
                    {
                        int length;
                        char* str = compile_string(p, &length);
                        b_obj_string* string = copy_string(p->vm, str, length);
                        push(p->vm, OBJ_VAL(string));// gc fix
                        table_set(p->vm, &sw->table, OBJ_VAL(string), jump);
                        pop(p->vm);// gc fix
                    }
                    else if(bl_parser_checknumber(p))
                    {
                        table_set(p->vm, &sw->table, bl_parser_compilenumber(p), jump);
                    }
                    else
                    {
                        pop(p->vm);// pop the switch
                        bl_parser_raiseerror(p, "only constants can be used in when expressions");
                        return;
                    }
                } while(bl_parser_match(p, COMMA_TOKEN));
            }
            else
            {
                state = 2;
                sw->default_jump = bl_parser_currentblob(p)->count - start_offset;
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
        case_ends[case_count++] = bl_parser_emitjump(p, OP_JUMP);
    }

    // patch all the case jumps to the end
    for(int i = 0; i < case_count; i++)
    {
        bl_parser_patchjump(p, case_ends[i]);
    }

    sw->exit_jump = bl_parser_currentblob(p)->count - start_offset;

    bl_parser_patchswitch(p, switch_code, bl_parser_makeconstant(p, OBJ_VAL(sw)));
    pop(p->vm);// pop the switch
}

static void if_statement(b_parser* p)
{
    bl_parser_parseexpr(p);

    int then_jump = bl_parser_emitjump(p, OP_JUMP_IF_FALSE);
    bl_parser_emitbyte(p, OP_POP);
    bl_parser_parsestmt(p);

    int else_jump = bl_parser_emitjump(p, OP_JUMP);

    bl_parser_patchjump(p, then_jump);
    bl_parser_emitbyte(p, OP_POP);

    if(bl_parser_match(p, ELSE_TOKEN))
    {
        bl_parser_parsestmt(p);
    }

    bl_parser_patchjump(p, else_jump);
}

static void echo_statement(b_parser* p)
{
    bl_parser_parseexpr(p);
    bl_parser_emitbyte(p, OP_ECHO);
    bl_parser_consumestmtend(p);
}

static void die_statement(b_parser* p)
{
    bl_parser_parseexpr(p);
    bl_parser_emitbyte(p, OP_DIE);
    bl_parser_consumestmtend(p);
}

static void parse_specific_import(b_parser* p, char* module_name, int import_constant, bool was_renamed, bool is_native)
{
    if(bl_parser_match(p, LBRACE_TOKEN))
    {
        if(was_renamed)
        {
            bl_parser_raiseerror(p, "selective import on renamed module");
            return;
        }

        bl_parser_emitbyte_and_short(p, OP_CONSTANT, import_constant);
        bool same_name_selective_exist = false;

        do
        {
            bl_parser_ignorespace(p);

            // terminate on all (*)
            if(bl_parser_match(p, MULTIPLY_TOKEN))
            {
                bl_parser_emitbyte(p, is_native ? OP_IMPORT_ALL_NATIVE : OP_IMPORT_ALL);
                break;
            }

            int name = bl_parser_parsevar(p, "module object name expected");

            if(module_name != NULL && p->previous.length == (int)strlen(module_name) && memcmp(module_name, p->previous.start, p->previous.length) == 0)
            {
                same_name_selective_exist = true;
            }

            bl_parser_emitbyte_and_short(p, is_native ? OP_SELECT_NATIVE_IMPORT : OP_SELECT_IMPORT, name);
        } while(bl_parser_match(p, COMMA_TOKEN));
        bl_parser_ignorespace(p);

        bl_parser_consume(p, RBRACE_TOKEN, "expected '}' at end of selective import");

        if(!same_name_selective_exist)
        {
            bl_parser_emitbyte_and_short(p, is_native ? OP_EJECT_NATIVE_IMPORT : OP_EJECT_IMPORT, import_constant);
        }
        bl_parser_emitbyte(p, OP_POP);// pop the module constant from stack
        bl_parser_consumestmtend(p);
    }
}

static void import_statement(b_parser* p)
{
    //  bl_parser_consume(p, LITERAL_TOKEN, "expected module name");
    //  int module_name_length;
    //  char *module_name = compile_string(p, &module_name_length);

    char* module_name = NULL;
    char* module_file = NULL;

    int part_count = 0;

    bool is_relative = bl_parser_match(p, DOT_TOKEN);

    // allow for import starting with ..
    if(!is_relative)
    {
        if(bl_parser_match(p, RANGE_TOKEN))
        {
        }
    }
    else
    {
        if(bl_parser_match(p, RANGE_TOKEN))
        {
            bl_parser_raiseerror(p, "conflicting module path. Parent or current directory?");
            return;
        }
    }

    do
    {
        if(p->previous.type == RANGE_TOKEN)
        {
            is_relative = true;
            if(module_file == NULL)
            {
                module_file = strdup("/../");
            }
            else
            {
                module_file = append_strings(module_file, "/../");
            }
        }

        if(module_name != NULL)
        {
            free(module_name);
        }

        bl_parser_consume(p, IDENTIFIER_TOKEN, "module name expected");

        module_name = (char*)calloc(p->previous.length + 1, sizeof(char));
        memcpy(module_name, p->previous.start, p->previous.length);
        module_name[p->previous.length] = '\0';

        // handle native modules
        if(part_count == 0 && module_name[0] == '_' && !is_relative)
        {
            int module = bl_parser_makeconstant(p, OBJ_VAL(copy_string(p->vm, module_name, (int)strlen(module_name))));
            bl_parser_emitbyte_and_short(p, OP_NATIVE_MODULE, module);

            parse_specific_import(p, module_name, module, false, true);
            return;
        }

        if(module_file == NULL)
        {
            module_file = strdup(module_name);
        }
        else
        {
            if(module_file[strlen(module_file) - 1] != BLADE_PATH_SEPARATOR[0])
            {
                module_file = append_strings(module_file, BLADE_PATH_SEPARATOR);
            }
            module_file = append_strings(module_file, module_name);
        }

        part_count++;
    } while(bl_parser_match(p, DOT_TOKEN) || bl_parser_match(p, RANGE_TOKEN));

    bool was_renamed = false;

    if(bl_parser_match(p, AS_TOKEN))
    {
        bl_parser_consume(p, IDENTIFIER_TOKEN, "module name expected");
        free(module_name);
        module_name = (char*)calloc(p->previous.length + 1, sizeof(char));
        if(module_name == NULL)
        {
            bl_parser_raiseerror(p, "could not allocate memory for module name");
            return;
        }
        memcpy(module_name, p->previous.start, p->previous.length);
        module_name[p->previous.length] = '\0';
        was_renamed = true;
    }

    char* module_path = resolve_import_path(module_file, p->module->file, is_relative);

    if(module_path == NULL)
    {
        // check if there is one in the vm's registry
        // handle native modules
        b_value md;
        b_obj_string* final_module_name = copy_string(p->vm, module_name, (int)strlen(module_name));
        if(table_get(&p->vm->modules, OBJ_VAL(final_module_name), &md))
        {
            int module = bl_parser_makeconstant(p, OBJ_VAL(final_module_name));
            bl_parser_emitbyte_and_short(p, OP_NATIVE_MODULE, module);

            parse_specific_import(p, module_name, module, false, true);
            return;
        }

        free(module_path);
        bl_parser_raiseerror(p, "module not found");
        return;
    }

    if(!bl_parser_check(p, LBRACE_TOKEN))
    {
        bl_parser_consumestmtend(p);
    }

    // do the import here...
    char* source = read_file(module_path);
    if(source == NULL)
    {
        bl_parser_raiseerror(p, "could not read import file %s", module_path);
        return;
    }

    b_blob blob;
    init_blob(&blob);

    b_obj_module* module = new_module(p->vm, module_name, module_path);

    push(p->vm, OBJ_VAL(module));
    b_obj_func* function = bl_compiler_compilesource(p->vm, module, source, &blob);
    pop(p->vm);

    free(source);

    if(function == NULL)
    {
        bl_parser_raiseerror(p, "failed to import %s", module_name);
        return;
    }

    function->name = NULL;

    push(p->vm, OBJ_VAL(function));
    b_obj_closure* closure = new_closure(p->vm, function);
    pop(p->vm);

    int import_constant = bl_parser_makeconstant(p, OBJ_VAL(closure));
    bl_parser_emitbyte_and_short(p, OP_CALL_IMPORT, import_constant);

    parse_specific_import(p, module_name, import_constant, was_renamed, false);
}

static void assert_statement(b_parser* p)
{
    bl_parser_parseexpr(p);
    if(bl_parser_match(p, COMMA_TOKEN))
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

static void try_statement(b_parser* p)
{
    if(p->vm->compiler->handler_count == MAX_EXCEPTION_HANDLERS)
    {
        bl_parser_raiseerror(p, "maximum exception handler in scope exceeded");
    }
    p->vm->compiler->handler_count++;
    p->is_trying = true;

    bl_parser_ignorespace(p);
    int try_begins = bl_parser_emittry(p);

    bl_parser_parsestmt(p);// compile the try body
    bl_parser_emitbyte(p, OP_POP_TRY);
    int exit_jump = bl_parser_emitjump(p, OP_JUMP);
    p->is_trying = false;

    // we can safely use 0 because a program cannot start with a
    // catch or finally block
    int address = 0, type = -1, finally = 0;

    bool catch_exists = false, final_exists = false;

    // catch body must maintain its own scope
    if(bl_parser_match(p, CATCH_TOKEN))
    {
        catch_exists = true;
        bl_parser_beginscope(p);

        bl_parser_consume(p, IDENTIFIER_TOKEN, "missing exception class name");
        type = bl_parser_identconst(p, &p->previous);
        address = bl_parser_currentblob(p)->count;

        if(bl_parser_match(p, IDENTIFIER_TOKEN))
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
        type = bl_parser_makeconstant(p, OBJ_VAL(copy_string(p->vm, "Exception", 9)));
    }

    bl_parser_patchjump(p, exit_jump);

    if(bl_parser_match(p, FINALLY_TOKEN))
    {
        final_exists = true;
        // if we arrived here from either the try or handler block,
        // we don't want to continue propagating the exception
        bl_parser_emitbyte(p, OP_FALSE);
        finally = bl_parser_currentblob(p)->count;

        bl_parser_ignorespace(p);
        bl_parser_parsestmt(p);

        int continue_execution_address = bl_parser_emitjump(p, OP_JUMP_IF_FALSE);
        bl_parser_emitbyte(p, OP_POP);// pop the bool off the stack
        bl_parser_emitbyte(p, OP_PUBLISH_TRY);
        bl_parser_patchjump(p, continue_execution_address);
        bl_parser_emitbyte(p, OP_POP);
    }

    if(!final_exists && !catch_exists)
    {
        bl_parser_raiseerror(p, "try block must contain at least one of catch or finally");
    }

    bl_parser_patchtry(p, try_begins, type, address, finally);
}

static void return_statement(b_parser* p)
{
    p->is_returning = true;
    if(p->vm->compiler->type == TYPE_SCRIPT)
    {
        bl_parser_raiseerror(p, "cannot return from top-level code");
    }

    if(bl_parser_match(p, SEMICOLON_TOKEN) || bl_parser_match(p, NEWLINE_TOKEN))
    {
        bl_parser_emitreturn(p);
    }
    else
    {
        if(p->vm->compiler->type == TYPE_INITIALIZER)
        {
            bl_parser_raiseerror(p, "cannot return value from constructor");
        }

        if(p->is_trying)
        {
            bl_parser_emitbyte(p, OP_POP_TRY);
        }

        bl_parser_parseexpr(p);
        bl_parser_emitbyte(p, OP_RETURN);
        bl_parser_consumestmtend(p);
    }
    p->is_returning = false;
}

static void while_statement(b_parser* p)
{
    int surrounding_loop_start = p->innermost_loop_start;
    int surrounding_scope_depth = p->innermost_loop_scope_depth;

    // we'll be jumping back to right before the
    // expression after the loop body
    p->innermost_loop_start = bl_parser_currentblob(p)->count;

    bl_parser_parseexpr(p);

    int exit_jump = bl_parser_emitjump(p, OP_JUMP_IF_FALSE);
    bl_parser_emitbyte(p, OP_POP);

    bl_parser_parsestmt(p);

    bl_parser_emitloop(p, p->innermost_loop_start);

    bl_parser_patchjump(p, exit_jump);
    bl_parser_emitbyte(p, OP_POP);

    bl_parser_endloop(p);

    p->innermost_loop_start = surrounding_loop_start;
    p->innermost_loop_scope_depth = surrounding_scope_depth;
}

static void do_while_statement(b_parser* p)
{
    int surrounding_loop_start = p->innermost_loop_start;
    int surrounding_scope_depth = p->innermost_loop_scope_depth;

    // we'll be jumping back to right before the
    // statements after the loop body
    p->innermost_loop_start = bl_parser_currentblob(p)->count;

    bl_parser_parsestmt(p);

    bl_parser_consume(p, WHILE_TOKEN, "expecting 'while' statement");

    bl_parser_parseexpr(p);

    int exit_jump = bl_parser_emitjump(p, OP_JUMP_IF_FALSE);
    bl_parser_emitbyte(p, OP_POP);

    bl_parser_emitloop(p, p->innermost_loop_start);

    bl_parser_patchjump(p, exit_jump);
    bl_parser_emitbyte(p, OP_POP);

    bl_parser_endloop(p);

    p->innermost_loop_start = surrounding_loop_start;
    p->innermost_loop_scope_depth = surrounding_scope_depth;
}

static void continue_statement(b_parser* p)
{
    if(p->innermost_loop_start == -1)
    {
        bl_parser_raiseerror(p, "'continue' can only be used in a loop");
    }

    // discard local variables created in the loop
    bl_parser_discardlocal(p, p->innermost_loop_scope_depth);

    // go back to the top of the loop
    bl_parser_emitloop(p, p->innermost_loop_start);
    bl_parser_consumestmtend(p);
}

static void break_statement(b_parser* p)
{
    if(p->innermost_loop_start == -1)
    {
        bl_parser_raiseerror(p, "'break' can only be used in a loop");
    }

    // discard local variables created in the loop
    //  bl_parser_discardlocal(p, p->innermost_loop_scope_depth);
    bl_parser_emitjump(p, OP_BREAK_PL);
    bl_parser_consumestmtend(p);
}

static void synchronize(b_parser* p)
{
    p->panic_mode = false;

    while(p->current.type != EOF_TOKEN)
    {
        if(p->current.type == NEWLINE_TOKEN || p->current.type == SEMICOLON_TOKEN)
            return;

        switch(p->current.type)
        {
            case CLASS_TOKEN:
            case DEF_TOKEN:
            case VAR_TOKEN:
            case FOR_TOKEN:
            case IF_TOKEN:
            case USING_TOKEN:
            case WHEN_TOKEN:
            case ITER_TOKEN:
            case DO_TOKEN:
            case WHILE_TOKEN:
            case ECHO_TOKEN:
            case ASSERT_TOKEN:
            case TRY_TOKEN:
            case CATCH_TOKEN:
            case DIE_TOKEN:
            case RETURN_TOKEN:
            case STATIC_TOKEN:
            case SELF_TOKEN:
            case PARENT_TOKEN:
            case FINALLY_TOKEN:
            case IN_TOKEN:
            case IMPORT_TOKEN:
            case AS_TOKEN:
                return;

            default:;// do nothing
        }

        bl_parser_advance(p);
    }
}

static void declaration(b_parser* p)
{
    bl_parser_ignorespace(p);

    if(bl_parser_match(p, CLASS_TOKEN))
    {
        class_declaration(p);
    }
    else if(bl_parser_match(p, DEF_TOKEN))
    {
        function_declaration(p);
    }
    else if(bl_parser_match(p, VAR_TOKEN))
    {
        var_declaration(p);
    }
    else if(bl_parser_match(p, LBRACE_TOKEN))
    {
        if(!bl_parser_check(p, NEWLINE_TOKEN) && p->vm->compiler->scope_depth == 0)
        {
            expression_statement(p, false, true);
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

    if(p->panic_mode)
        synchronize(p);

    bl_parser_ignorespace(p);
}

static void bl_parser_parsestmt(b_parser* p)
{
    p->repl_can_echo = false;
    bl_parser_ignorespace(p);

    if(bl_parser_match(p, ECHO_TOKEN))
    {
        echo_statement(p);
    }
    else if(bl_parser_match(p, IF_TOKEN))
    {
        if_statement(p);
    }
    else if(bl_parser_match(p, DO_TOKEN))
    {
        do_while_statement(p);
    }
    else if(bl_parser_match(p, WHILE_TOKEN))
    {
        while_statement(p);
    }
    else if(bl_parser_match(p, ITER_TOKEN))
    {
        iter_statement(p);
    }
    else if(bl_parser_match(p, FOR_TOKEN))
    {
        for_statement(p);
    }
    else if(bl_parser_match(p, USING_TOKEN))
    {
        using_statement(p);
    }
    else if(bl_parser_match(p, CONTINUE_TOKEN))
    {
        continue_statement(p);
    }
    else if(bl_parser_match(p, BREAK_TOKEN))
    {
        break_statement(p);
    }
    else if(bl_parser_match(p, RETURN_TOKEN))
    {
        return_statement(p);
    }
    else if(bl_parser_match(p, ASSERT_TOKEN))
    {
        assert_statement(p);
    }
    else if(bl_parser_match(p, DIE_TOKEN))
    {
        die_statement(p);
    }
    else if(bl_parser_match(p, LBRACE_TOKEN))
    {
        bl_parser_beginscope(p);
        bl_parser_parseblock(p);
        bl_parser_endscope(p);
    }
    else if(bl_parser_match(p, IMPORT_TOKEN))
    {
        import_statement(p);
    }
    else if(bl_parser_match(p, TRY_TOKEN))
    {
        try_statement(p);
    }
    else
    {
        expression_statement(p, false, false);
    }

    bl_parser_ignorespace(p);
}

b_obj_func* bl_compiler_compilesource(b_vm* vm, b_obj_module* module, const char* source, b_blob* blob)
{
    b_scanner scanner;
    bl_scanner_init(&scanner, source);

    b_parser parser;

    parser.vm = vm;
    parser.scanner = &scanner;

    parser.had_error = false;
    parser.panic_mode = false;
    parser.block_count = 0;
    parser.repl_can_echo = false;
    parser.is_returning = false;
    parser.is_trying = false;
    parser.innermost_loop_start = -1;
    parser.innermost_loop_scope_depth = 0;
    parser.current_class = NULL;
    parser.module = module;

    b_compiler compiler;
    bl_compiler_init(&parser, &compiler, TYPE_SCRIPT);

    bl_parser_advance(&parser);
    bl_parser_ignorespace(&parser);

    while(!bl_parser_match(&parser, EOF_TOKEN))
    {
        declaration(&parser);
    }

    b_obj_func* function = bl_compiler_end(&parser);

    return parser.had_error ? NULL : function;
}

void mark_compiler_roots(b_vm* vm)
{
    b_compiler* compiler = vm->compiler;
    while(compiler != NULL)
    {
        mark_object(vm, (b_obj*)compiler->currfunc);
        compiler = compiler->enclosing;
    }
}

void disassemble_blob(b_blob* blob, const char* name)
{
    printf("== %s ==\n", name);

    for(int offset = 0; offset < blob->count;)
    {
        offset = disassemble_instruction(blob, offset);
    }
}

int simple_instruction(const char* name, int offset)
{
    printf("%s\n", name);
    return offset + 1;
}

int constant_instruction(const char* name, b_blob* blob, int offset)
{
    uint16_t constant = (blob->code[offset + 1] << 8) | blob->code[offset + 2];
    printf("%-16s %8d '", name, constant);
    print_value(blob->constants.values[constant]);
    printf("'\n");
    return offset + 3;
}

int short_instruction(const char* name, b_blob* blob, int offset)
{
    uint16_t slot = (blob->code[offset + 1] << 8) | blob->code[offset + 2];
    printf("%-16s %8d\n", name, slot);
    return offset + 3;
}

static int byte_instruction(const char* name, b_blob* blob, int offset)
{
    uint8_t slot = blob->code[offset + 1];
    printf("%-16s %8d\n", name, slot);
    return offset + 2;
}

static int jump_instruction(const char* name, int sign, b_blob* blob, int offset)
{
    uint16_t jump = (uint16_t)(blob->code[offset + 1] << 8);
    jump |= blob->code[offset + 2];

    printf("%-16s %8d -> %d\n", name, offset, offset + 3 + sign * jump);
    return offset + 3;
}

static int try_instruction(const char* name, b_blob* blob, int offset)
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

static int invoke_instruction(const char* name, b_blob* blob, int offset)
{
    uint16_t constant = (uint16_t)(blob->code[offset + 1] << 8);
    constant |= blob->code[offset + 2];
    uint8_t arg_count = blob->code[offset + 3];

    printf("%-16s (%d args) %8d '", name, arg_count, constant);
    print_value(blob->constants.values[constant]);
    printf("'\n");
    return offset + 4;
}

int disassemble_instruction(b_blob* blob, int offset)
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
            return jump_instruction("f_jump", 1, blob, offset);
        case OP_JUMP:
            return jump_instruction("jump", 1, blob, offset);
        case OP_TRY:
            return try_instruction("i_try", blob, offset);
        case OP_LOOP:
            return jump_instruction("loop", -1, blob, offset);

        case OP_DEFINE_GLOBAL:
            return constant_instruction("d_glob", blob, offset);
        case OP_GET_GLOBAL:
            return constant_instruction("g_glob", blob, offset);
        case OP_SET_GLOBAL:
            return constant_instruction("s_glob", blob, offset);

        case OP_GET_LOCAL:
            return short_instruction("g_loc", blob, offset);
        case OP_SET_LOCAL:
            return short_instruction("s_loc", blob, offset);

        case OP_GET_PROPERTY:
            return constant_instruction("g_prop", blob, offset);
        case OP_GET_SELF_PROPERTY:
            return constant_instruction("g_props", blob, offset);
        case OP_SET_PROPERTY:
            return constant_instruction("s_prop", blob, offset);

        case OP_GET_UP_VALUE:
            return short_instruction("g_up_v", blob, offset);
        case OP_SET_UP_VALUE:
            return short_instruction("s_up_v", blob, offset);

        case OP_POP_TRY:
            return simple_instruction("p_try", offset);
        case OP_PUBLISH_TRY:
            return simple_instruction("pub_try", offset);

        case OP_CONSTANT:
            return constant_instruction("load", blob, offset);

        case OP_EQUAL:
            return simple_instruction("eq", offset);

        case OP_GREATER:
            return simple_instruction("gt", offset);
        case OP_LESS:
            return simple_instruction("less", offset);
        case OP_EMPTY:
            return simple_instruction("em", offset);
        case OP_NIL:
            return simple_instruction("nil", offset);
        case OP_TRUE:
            return simple_instruction("true", offset);
        case OP_FALSE:
            return simple_instruction("false", offset);
        case OP_ADD:
            return simple_instruction("add", offset);
        case OP_SUBTRACT:
            return simple_instruction("sub", offset);
        case OP_MULTIPLY:
            return simple_instruction("mul", offset);
        case OP_DIVIDE:
            return simple_instruction("div", offset);
        case OP_F_DIVIDE:
            return simple_instruction("f_div", offset);
        case OP_REMINDER:
            return simple_instruction("r_mod", offset);
        case OP_POW:
            return simple_instruction("pow", offset);
        case OP_NEGATE:
            return simple_instruction("neg", offset);
        case OP_NOT:
            return simple_instruction("not", offset);
        case OP_BIT_NOT:
            return simple_instruction("b_not", offset);
        case OP_AND:
            return simple_instruction("b_and", offset);
        case OP_OR:
            return simple_instruction("b_or", offset);
        case OP_XOR:
            return simple_instruction("b_xor", offset);
        case OP_LSHIFT:
            return simple_instruction("l_shift", offset);
        case OP_RSHIFT:
            return simple_instruction("r_shift", offset);
        case OP_ONE:
            return simple_instruction("one", offset);

        case OP_CALL_IMPORT:
            return short_instruction("c_import", blob, offset);
        case OP_NATIVE_MODULE:
            return short_instruction("f_import", blob, offset);
        case OP_SELECT_IMPORT:
            return short_instruction("s_import", blob, offset);
        case OP_SELECT_NATIVE_IMPORT:
            return short_instruction("sn_import", blob, offset);
        case OP_EJECT_IMPORT:
            return short_instruction("e_import", blob, offset);
        case OP_EJECT_NATIVE_IMPORT:
            return short_instruction("en_import", blob, offset);
        case OP_IMPORT_ALL:
            return simple_instruction("a_import", offset);
        case OP_IMPORT_ALL_NATIVE:
            return simple_instruction("an_import", offset);

        case OP_ECHO:
            return simple_instruction("echo", offset);
        case OP_STRINGIFY:
            return simple_instruction("str", offset);
        case OP_CHOICE:
            return simple_instruction("cho", offset);
        case OP_DIE:
            return simple_instruction("die", offset);
        case OP_POP:
            return simple_instruction("pop", offset);
        case OP_CLOSE_UP_VALUE:
            return simple_instruction("cl_up_v", offset);
        case OP_DUP:
            return simple_instruction("dup", offset);
        case OP_ASSERT:
            return simple_instruction("assrt", offset);
        case OP_POP_N:
            return short_instruction("pop_n", blob, offset);

            // non-user objects...
        case OP_SWITCH:
            return short_instruction("sw", blob, offset);

            // data container manipulators
        case OP_RANGE:
            return short_instruction("rng", blob, offset);
        case OP_LIST:
            return short_instruction("list", blob, offset);
        case OP_DICT:
            return short_instruction("dict", blob, offset);
        case OP_GET_INDEX:
            return byte_instruction("g_ind", blob, offset);
        case OP_GET_RANGED_INDEX:
            return byte_instruction("gr_ind", blob, offset);
        case OP_SET_INDEX:
            return simple_instruction("s_ind", offset);

        case OP_CLOSURE:
        {
            offset++;
            uint16_t constant = blob->code[offset++] << 8;
            constant |= blob->code[offset++];
            printf("%-16s %8d ", "clsur", constant);
            print_value(blob->constants.values[constant]);
            printf("\n");

            b_obj_func* function = AS_FUNCTION(blob->constants.values[constant]);
            for(int j = 0; j < function->up_value_count; j++)
            {
                int is_local = blob->code[offset++];
                uint16_t index = blob->code[offset++] << 8;
                index |= blob->code[offset++];
                printf("%04d      |                     %s %d\n", offset - 3, is_local ? "local" : "up-value", (int)index);
            }

            return offset;
        }
        case OP_CALL:
            return byte_instruction("call", blob, offset);
        case OP_INVOKE:
            return invoke_instruction("invk", blob, offset);
        case OP_INVOKE_SELF:
            return invoke_instruction("invk_s", blob, offset);
        case OP_RETURN:
            return simple_instruction("ret", offset);

        case OP_CLASS:
            return constant_instruction("class", blob, offset);
        case OP_METHOD:
            return constant_instruction("meth", blob, offset);
        case OP_CLASS_PROPERTY:
            return constant_instruction("cl_prop", blob, offset);
        case OP_GET_SUPER:
            return constant_instruction("g_sup", blob, offset);
        case OP_INHERIT:
            return simple_instruction("inher", offset);
        case OP_SUPER_INVOKE:
            return invoke_instruction("s_invk", blob, offset);
        case OP_SUPER_INVOKE_SELF:
            return byte_instruction("s_invk_s", blob, offset);

        default:
            printf("unknown opcode %d\n", instruction);
            return offset + 1;
    }
}

#define FILE_ERROR(type, message) \
    file_close(file); \
    RETURN_ERROR(#type " -> %s", message, file->path->chars);

#define RETURN_STATUS(status) \
    if((status) == 0) \
    { \
        RETURN_TRUE; \
    } \
    else \
    { \
        FILE_ERROR(File, strerror(errno)); \
    }

#define DENY_STD() \
    if(file->mode->length == 0) \
        RETURN_ERROR("method not supported for std files");

#define SET_DICT_STRING(d, n, l, v) dict_add_entry(vm, d, GC_L_STRING(n, l), v)

static int file_close(b_obj_file* file)
{
    if(file->file != NULL && !is_std_file(file))
    {
        fflush(file->file);
        int result = fclose(file->file);
        file->file = NULL;
        file->is_open = false;
        return result;
    }
    return -1;
}

static void file_open(b_obj_file* file)
{
    if((file->file == NULL || !file->is_open) && !is_std_file(file))
    {
        char* mode = file->mode->chars;
        if(strstr(file->mode->chars, "w") != NULL && strstr(file->mode->chars, "+") != NULL)
        {
            mode = (char*)"a+";
        }

        file->file = fopen(file->path->chars, mode);
        file->is_open = true;
    }
}

bool cfn_file(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_RANGE(file, 1, 2);
    ENFORCE_ARG_TYPE(file, 0, IS_STRING);
    b_obj_string* path = AS_STRING(args[0]);

    if(path->length == 0)
    {
        RETURN_ERROR("file path cannot be empty");
    }

    b_obj_string* mode = NULL;

    if(arg_count == 2)
    {
        ENFORCE_ARG_TYPE(file, 1, IS_STRING);
        mode = AS_STRING(args[1]);
    }
    else
    {
        mode = (b_obj_string*)GC(copy_string(vm, "r", 1));
    }

    b_obj_file* file = (b_obj_file*)GC(new_file(vm, path, mode));
    file_open(file);

    RETURN_OBJ(file);
}

bool native_method_fileexists(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(exists, 0);
    b_obj_file* file = AS_FILE(METHOD_OBJECT);
    DENY_STD();
    RETURN_BOOL(file_exists(file->path->chars));
}

bool native_method_fileclose(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(close, 0);
    file_close(AS_FILE(METHOD_OBJECT));
    RETURN;
}

bool native_method_fileopen(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(open, 0);
    file_open(AS_FILE(METHOD_OBJECT));
    RETURN;
}

bool native_method_fileis_open(b_vm* vm, int arg_count, b_value* args)
{
    b_obj_file* file = AS_FILE(METHOD_OBJECT);
    RETURN_BOOL(is_std_file(file) || (file->is_open && file->file != NULL));
}

bool native_method_fileis_closed(b_vm* vm, int arg_count, b_value* args)
{
    b_obj_file* file = AS_FILE(METHOD_OBJECT);
    RETURN_BOOL(!is_std_file(file) && !file->is_open && file->file == NULL);
}

bool native_method_fileread(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_RANGE(read, 0, 1);
    size_t file_size = -1;
    size_t file_size_real = -1;
    if(arg_count == 1)
    {
        ENFORCE_ARG_TYPE(read, 0, IS_NUMBER);
        file_size = (size_t)AS_NUMBER(args[0]);
    }

    b_obj_file* file = AS_FILE(METHOD_OBJECT);

    bool in_binary_mode = strstr(file->mode->chars, "b") != NULL;

    if(!is_std_file(file))
    {
        // file is in read mode and file does not exist
        if(strstr(file->mode->chars, "r") != NULL && !file_exists(file->path->chars))
        {
            FILE_ERROR(NotFound, "no such file or directory");
        }
        // file is in write only mode
        else if(strstr(file->mode->chars, "w") != NULL && strstr(file->mode->chars, "+") == NULL)
        {
            FILE_ERROR(Unsupported, "cannot read file in write mode");
        }

        if(!file->is_open)
        {// open the file if it isn't open
            file_open(file);
        }

        if(file->file == NULL)
        {
            FILE_ERROR(Read, "could not read file");
        }

        // Get file size
        struct stat stats;// stats is super faster on large files
        if(lstat(file->path->chars, &stats) == 0)
        {
            file_size_real = (size_t)stats.st_size;
        }
        else
        {
            // fallback
            fseek(file->file, 0L, SEEK_END);
            file_size_real = ftell(file->file);
            rewind(file->file);
        }

        if(file_size == (size_t)-1 || file_size > file_size_real)
        {
            file_size = file_size_real;
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
        if(file_size == (size_t)-1)
        {
            file_size = 1;
        }
    }

    char* buffer = (char*)ALLOCATE(char, file_size + 1);// +1 for terminator '\0'

    if(buffer == NULL && file_size != 0)
    {
        FILE_ERROR(Buffer, "not enough memory to read file");
    }

    size_t bytes_read = fread(buffer, sizeof(char), file_size, file->file);

    if(bytes_read == 0 && file_size != 0 && file_size == file_size_real)
    {
        FILE_ERROR(Read, "could not read file contents");
    }

    // we made use of +1 so we can terminate the string.
    if(buffer != NULL)
        buffer[bytes_read] = '\0';

    // close file
    /*if (bytes_read == file_size) {
    file_close(file);
  }*/
    file_close(file);

    if(!in_binary_mode)
    {
        RETURN_T_STRING(buffer, bytes_read);
    }

    RETURN_OBJ(take_bytes(vm, (unsigned char*)buffer, bytes_read));
}

bool native_method_filegets(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_RANGE(gets, 0, 1);
    size_t length = -1;
    if(arg_count == 1)
    {
        ENFORCE_ARG_TYPE(read, 0, IS_NUMBER);
        length = (size_t)AS_NUMBER(args[0]);
    }

    b_obj_file* file = AS_FILE(METHOD_OBJECT);

    bool in_binary_mode = strstr(file->mode->chars, "b") != NULL;

    if(!is_std_file(file))
    {
        // file is in read mode and file does not exist
        if(strstr(file->mode->chars, "r") != NULL && !file_exists(file->path->chars))
        {
            FILE_ERROR(NotFound, "no such file or directory");
        }
        // file is in write only mode
        else if(strstr(file->mode->chars, "w") != NULL && strstr(file->mode->chars, "+") == NULL)
        {
            FILE_ERROR(Unsupported, "cannot read file in write mode");
        }

        if(!file->is_open)
        {// open the file if it isn't open
            FILE_ERROR(Read, "file not open");
        }

        if(file->file == NULL)
        {
            FILE_ERROR(Read, "could not read file");
        }

        if(length == -1)
        {
            long current_pos = ftell(file->file);
            fseek(file->file, 0L, SEEK_END);
            long end = ftell(file->file);

            // go back to where we were before.
            fseek(file->file, current_pos, SEEK_SET);

            length = end - current_pos;
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
        if(length == (size_t)-1)
        {
            length = 1;
        }
    }

    char* buffer = (char*)ALLOCATE(char, length + 1);// +1 for terminator '\0'

    if(buffer == NULL && length != 0)
    {
        FILE_ERROR(Buffer, "not enough memory to read file");
    }

    size_t bytes_read = fread(buffer, sizeof(char), length, file->file);

    if(bytes_read == 0 && length != 0)
    {
        FILE_ERROR(Read, "could not read file contents");
    }

    // we made use of +1, so we can terminate the string.
    if(buffer != NULL)
        buffer[bytes_read] = '\0';

    if(!in_binary_mode)
    {
        RETURN_T_STRING(buffer, bytes_read);
    }

    RETURN_OBJ(take_bytes(vm, (unsigned char*)buffer, bytes_read));
}

bool native_method_filewrite(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(write, 1);

    b_obj_file* file = AS_FILE(METHOD_OBJECT);
    b_obj_string* string = NULL;
    b_obj_bytes* bytes = NULL;

    bool in_binary_mode = strstr(file->mode->chars, "b") != NULL;

    unsigned char* data;
    int length;

    if(!in_binary_mode)
    {
        ENFORCE_ARG_TYPE(write, 0, IS_STRING);
        string = AS_STRING(args[0]);
        data = (unsigned char*)string->chars;
        length = string->length;
    }
    else
    {
        ENFORCE_ARG_TYPE(write, 0, IS_BYTES);
        bytes = AS_BYTES(args[0]);
        data = bytes->bytes.bytes;
        length = bytes->bytes.count;
    }

    // file is in read only mode
    if(!is_std_file(file))
    {
        if(strstr(file->mode->chars, "r") != NULL && strstr(file->mode->chars, "+") == NULL)
        {
            FILE_ERROR(Unsupported, "cannot write into non-writable file");
        }

        if(length == 0)
        {
            FILE_ERROR(Write, "cannot write empty buffer to file");
        }

        if(file->file == NULL || !file->is_open)
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

bool native_method_fileputs(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(puts, 1);

    b_obj_file* file = AS_FILE(METHOD_OBJECT);
    b_obj_string* string = NULL;
    b_obj_bytes* bytes = NULL;

    bool in_binary_mode = strstr(file->mode->chars, "b") != NULL;

    unsigned char* data;
    int length;

    if(!in_binary_mode)
    {
        ENFORCE_ARG_TYPE(write, 0, IS_STRING);
        string = AS_STRING(args[0]);
        data = (unsigned char*)string->chars;
        length = string->length;
    }
    else
    {
        ENFORCE_ARG_TYPE(write, 0, IS_BYTES);
        bytes = AS_BYTES(args[0]);
        data = bytes->bytes.bytes;
        length = bytes->bytes.count;
    }

    // file is in read only mode
    if(!is_std_file(file))
    {
        if(strstr(file->mode->chars, "r") != NULL && strstr(file->mode->chars, "+") == NULL)
        {
            FILE_ERROR(Unsupported, "cannot write into non-writable file");
        }

        if(length == 0)
        {
            FILE_ERROR(Write, "cannot write empty buffer to file");
        }

        if(!file->is_open)
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

bool native_method_filenumber(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(number, 0);
    b_obj_file* file = AS_FILE(METHOD_OBJECT);
    if(file->file == NULL)
    {
        RETURN_NUMBER(-1);
    }
    else
    {
        RETURN_NUMBER(fileno(file->file));
    }
}

bool native_method_fileis_tty(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(is_tty, 0);
    b_obj_file* file = AS_FILE(METHOD_OBJECT);
    if(is_std_file(file))
    {
        RETURN_BOOL(isatty(fileno(file->file)) && fileno(file->file) == fileno(stdout));
    }
    RETURN_FALSE;
}

bool native_method_fileflush(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(flush, 0);
    b_obj_file* file = AS_FILE(METHOD_OBJECT);

    if(!file->is_open)
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
    RETURN;
}

bool native_method_filestats(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(stats, 0);

    b_obj_file* file = AS_FILE(METHOD_OBJECT);
    b_obj_dict* dict = (b_obj_dict*)GC(new_dict(vm));

    if(!is_std_file(file))
    {
        if(file_exists(file->path->chars))
        {
            struct stat stats;
            if(lstat(file->path->chars, &stats) == 0)
            {
                // read mode
                SET_DICT_STRING(dict, "is_readable", 11, BOOL_VAL(((stats.st_mode & S_IRUSR) != 0)));

                // write mode
                SET_DICT_STRING(dict, "is_writable", 11, BOOL_VAL(((stats.st_mode & S_IWUSR) != 0)));

                // execute mode
                SET_DICT_STRING(dict, "is_executable", 13, BOOL_VAL(((stats.st_mode & S_IXUSR) != 0)));

                // is symbolic link
                SET_DICT_STRING(dict, "is_symbolic", 11, BOOL_VAL((S_ISLNK(stats.st_mode) != 0)));


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
            SET_DICT_STRING(dict, "is_readable", 11, TRUE_VAL);
            SET_DICT_STRING(dict, "is_writable", 11, FALSE_VAL);
        }
        else if(fileno(stdout) == fileno(file->file) || fileno(stderr) == fileno(file->file))
        {
            SET_DICT_STRING(dict, "is_readable", 11, FALSE_VAL);
            SET_DICT_STRING(dict, "is_writable", 11, TRUE_VAL);
        }
        SET_DICT_STRING(dict, "is_executable", 13, FALSE_VAL);
        SET_DICT_STRING(dict, "size", 4, NUMBER_VAL(1));
    }
    RETURN_OBJ(dict);
}

bool native_method_filesymlink(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(symlink, 1);
    ENFORCE_ARG_TYPE(symlink, 0, IS_STRING);
    b_obj_file* file = AS_FILE(METHOD_OBJECT);
    DENY_STD();

    if(file_exists(file->path->chars))
    {
        b_obj_string* path = AS_STRING(args[0]);
        RETURN_BOOL(symlink(file->path->chars, path->chars) == 0);
    }
    else
    {
        RETURN_ERROR("symlink to file not found");
    }
}

bool native_method_filedelete(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(delete, 0);
    b_obj_file* file = AS_FILE(METHOD_OBJECT);
    DENY_STD();

    if(file_close(file) != 0)
    {
        RETURN_ERROR("error closing file.");
    }
    RETURN_STATUS(unlink(file->path->chars));
}

bool native_method_filerename(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(rename, 1);
    ENFORCE_ARG_TYPE(rename, 0, IS_STRING);

    b_obj_file* file = AS_FILE(METHOD_OBJECT);
    DENY_STD();

    if(file_exists(file->path->chars))
    {
        b_obj_string* new_name = AS_STRING(args[0]);
        if(new_name->length == 0)
        {
            FILE_ERROR(Operation, "file name cannot be empty");
        }
        file_close(file);
        RETURN_STATUS(rename(file->path->chars, new_name->chars));
    }
    else
    {
        RETURN_ERROR("file not found");
    }
}

bool native_method_filepath(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(path, 0);
    b_obj_file* file = AS_FILE(METHOD_OBJECT);
    DENY_STD();
    RETURN_OBJ(file->path);
}

bool native_method_filemode(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(mode, 0);
    b_obj_file* file = AS_FILE(METHOD_OBJECT);
    DENY_STD();
    RETURN_OBJ(file->mode);
}

bool native_method_filename(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(name, 0);
    b_obj_file* file = AS_FILE(METHOD_OBJECT);
    DENY_STD();
    char* name = get_real_file_name(file->path->chars);
    RETURN_STRING(name);
}

bool native_method_fileabs_path(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(abs_path, 0);
    b_obj_file* file = AS_FILE(METHOD_OBJECT);
    DENY_STD();

    char* abs_path = realpath(file->path->chars, NULL);
    if(abs_path != NULL)
        RETURN_STRING(abs_path);
    RETURN_STRING("");
}

bool native_method_filecopy(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(copy, 1);
    ENFORCE_ARG_TYPE(copy, 0, IS_STRING);
    b_obj_file* file = AS_FILE(METHOD_OBJECT);
    DENY_STD();

    if(file_exists(file->path->chars))
    {
        b_obj_string* name = AS_STRING(args[0]);

        if(strstr(file->mode->chars, "r") == NULL)
        {
            FILE_ERROR(Unsupported, "file not open for reading");
        }

        char* mode = "w";
        // if we are dealing with a binary file
        if(strstr(file->mode->chars, "b") != NULL)
        {
            mode = "wb";
        }

        FILE* fp = fopen(name->chars, mode);
        if(fp == NULL)
        {
            FILE_ERROR(Permission, "unable to create new file");
        }

        size_t n_read, n_write;
        unsigned char buffer[8192];
        do
        {
            n_read = fread(buffer, 1, sizeof(buffer), file->file);
            if(n_read > 0)
            {
                n_write = fwrite(buffer, 1, n_read, fp);
            }
            else
            {
                n_write = 0;
            }
        } while((n_read > 0) && (n_read == n_write));

        if(n_write > 0)
        {
            FILE_ERROR(Operation, "error copying file");
        }

        fflush(fp);
        fclose(fp);
        file_close(file);

        RETURN_BOOL(n_read == n_write);
    }
    else
    {
        RETURN_ERROR("file not found");
    }
}

bool native_method_filetruncate(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_RANGE(truncate, 0, 1);

    off_t final_size = 0;
    if(arg_count == 1)
    {
        ENFORCE_ARG_TYPE(truncate, 0, IS_NUMBER);
        final_size = (off_t)AS_NUMBER(args[0]);
    }
    b_obj_file* file = AS_FILE(METHOD_OBJECT);
    DENY_STD();

    RETURN_STATUS(truncate(file->path->chars, final_size));
}

bool native_method_filechmod(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(chmod, 1);
    ENFORCE_ARG_TYPE(chmod, 0, IS_NUMBER);

    b_obj_file* file = AS_FILE(METHOD_OBJECT);
    DENY_STD();

    if(file_exists(file->path->chars))
    {
        int mode = AS_NUMBER(args[0]);

        RETURN_STATUS(chmod(file->path->chars, (mode_t)mode));
    }
    else
    {
        RETURN_ERROR("file not found");
    }
}

bool native_method_fileset_times(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(set_times, 2);
    ENFORCE_ARG_TYPE(set_times, 0, IS_NUMBER);
    ENFORCE_ARG_TYPE(set_times, 1, IS_NUMBER);

#ifdef HAVE_UTIME
    b_obj_file* file = AS_FILE(METHOD_OBJECT);
    DENY_STD();

    if(file_exists(file->path->chars))
    {
        time_t atime = (time_t)AS_NUMBER(args[0]);
        time_t mtime = (time_t)AS_NUMBER(args[1]);

        struct stat stats;
        int status = lstat(file->path->chars, &stats);
        if(status == 0)
        {
            struct utimbuf new_times;

    #if !defined(_WIN32) && (!defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE))
            if(atime == (time_t)-1)
                new_times.actime = stats.st_atimespec.tv_sec;
            else
                new_times.actime = atime;

            if(mtime == (time_t)-1)
                new_times.modtime = stats.st_mtimespec.tv_sec;
            else
                new_times.modtime = mtime;
    #else
            if(atime == (time_t)-1)
                new_times.actime = stats.st_atime;
            else
                new_times.actime = atime;

            if(mtime == (time_t)-1)
                new_times.modtime = stats.st_mtime;
            else
                new_times.modtime = mtime;
    #endif

            RETURN_STATUS(utime(file->path->chars, &new_times));
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

bool native_method_fileseek(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(seek, 2);
    ENFORCE_ARG_TYPE(seek, 0, IS_NUMBER);
    ENFORCE_ARG_TYPE(seek, 1, IS_NUMBER);

    b_obj_file* file = AS_FILE(METHOD_OBJECT);
    DENY_STD();

    long position = (long)AS_NUMBER(args[0]);
    int seek_type = AS_NUMBER(args[1]);
    RETURN_STATUS(fseek(file->file, position, seek_type));
}

bool native_method_filetell(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(tell, 0);
    b_obj_file* file = AS_FILE(METHOD_OBJECT);
    DENY_STD();
    RETURN_NUMBER(ftell(file->file));
}

#undef FILE_ERROR
#undef RETURN_STATUS
#undef SET_DICT_STRING
#undef DENY_STD

static const b_module_init builtin_modules[] = {
    &blade_module_loader_os,//
    &blade_module_loader_io,//
    //&blade_module_loader_base64,//
    &blade_module_loader_math,//
    &blade_module_loader_date,//
    //&blade_module_loader_socket,//
    //&blade_module_loader_hash,//
    &blade_module_loader_reflect,//
    &blade_module_loader_array,//
    &blade_module_loader_process,//
    &blade_module_loader_struct,//
    NULL,
};

bool load_module(b_vm* vm, b_module_init init_fn, char* import_name, char* source, void* handle)
{
    b_module_reg* module = init_fn(vm);

    if(module != NULL)
    {
        b_obj_module* the_module = (b_obj_module*)GC(new_module(vm, strdup(module->name), source));

        the_module->preloader = module->preloader;
        the_module->unloader = module->unloader;

        if(module->fields != NULL)
        {
            for(int j = 0; module->fields[j].name != NULL; j++)
            {
                b_field_reg field = module->fields[j];
                b_value field_name = GC_STRING(field.name);

                b_value v = field.field_value(vm);
                push(vm, v);
                table_set(vm, &the_module->values, field_name, v);
                pop(vm);
            }
        }

        if(module->functions != NULL)
        {
            for(int j = 0; module->functions[j].name != NULL; j++)
            {
                b_func_reg func = module->functions[j];
                b_value func_name = GC_STRING(func.name);

                b_value func_real_value = OBJ_VAL(GC(new_native(vm, func.natfn, func.name)));
                push(vm, func_real_value);
                table_set(vm, &the_module->values, func_name, func_real_value);
                pop(vm);
            }
        }

        if(module->classes != NULL)
        {
            for(int j = 0; module->classes[j].name != NULL; j++)
            {
                b_class_reg klass_reg = module->classes[j];

                b_obj_string* class_name = (b_obj_string*)GC(copy_string(vm, klass_reg.name, (int)strlen(klass_reg.name)));

                b_obj_class* klass = (b_obj_class*)GC(new_class(vm, class_name));

                if(klass_reg.functions != NULL)
                {
                    for(int k = 0; klass_reg.functions[k].name != NULL; k++)
                    {
                        b_func_reg func = klass_reg.functions[k];

                        b_value func_name = GC_STRING(func.name);

                        b_obj_native* native = (b_obj_native*)GC(new_native(vm, func.natfn, func.name));

                        if(func.is_static)
                        {
                            native->type = TYPE_STATIC;
                        }
                        else if(strlen(func.name) > 0 && func.name[0] == '_')
                        {
                            native->type = TYPE_PRIVATE;
                        }

                        table_set(vm, &klass->methods, func_name, OBJ_VAL(native));
                    }
                }

                if(klass_reg.fields != NULL)
                {
                    for(int k = 0; klass_reg.fields[k].name != NULL; k++)
                    {
                        b_field_reg field = klass_reg.fields[k];
                        b_value field_name = GC_STRING(field.name);

                        b_value v = field.field_value(vm);
                        push(vm, v);
                        table_set(vm, field.is_static ? &klass->static_properties : &klass->properties, field_name, v);
                        pop(vm);
                    }
                }

                table_set(vm, &the_module->values, OBJ_VAL(class_name), OBJ_VAL(klass));
            }
        }

        if(handle != NULL)
        {
            the_module->handle = handle;// set handle for shared library modules
        }
        add_native_module(vm, the_module, the_module->name);

        CLEAR_GC();
        return true;
    }
    else
    {
        // @TODO: Warn about module loading error...
        printf("Error loading module: _%s\n", import_name);
    }

    return false;
}

void add_native_module(b_vm* vm, b_obj_module* module, const char* as)
{
    if(as != NULL)
    {
        module->name = strdup(as);
    }
    b_value name = STRING_VAL(module->name);
    push(vm, name);
    push(vm, OBJ_VAL(module));
    table_set(vm, &vm->modules, name, OBJ_VAL(module));
    pop_n(vm, 2);
}

void bind_user_modules(b_vm* vm, char* pkg_root)
{
    if(pkg_root == NULL)
        return;

    DIR* dir;
    if((dir = opendir(pkg_root)) != NULL)
    {
        struct dirent* ent;
        while((ent = readdir(dir)) != NULL)
        {
            int ext_length = (int)strlen(LIBRARY_FILE_EXTENSION);

            // skip . and .. in path
            if((strlen(ent->d_name) == 1 && ent->d_name[0] == '.')// .
               || (strlen(ent->d_name) == 2 && ent->d_name[0] == '.' && ent->d_name[1] == '.')// ..
               || strlen(ent->d_name) < ext_length + 1)
            {
                continue;
            }

            char* path = merge_paths(pkg_root, ent->d_name);
            if(!path)
                continue;

            int path_length = (int)strlen(path);

            struct stat sb;
            if(stat(path, &sb) == 0)
            {
                // it's not a directory
                if(S_ISDIR(sb.st_mode) < 1)
                {
                    if(memcmp(path + (path_length - ext_length), LIBRARY_FILE_EXTENSION, ext_length) == 0)
                    {// library file

                        char* filename = get_real_file_name(path);

                        int name_length = (int)strlen(filename) - ext_length;
                        char* name = ALLOCATE(char, name_length + 1);
                        memcpy(name, filename, name_length);
                        name[name_length] = '\0';

                        char* error = load_user_module(vm, path, name);
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

    CLEAR_GC();
}

void bind_native_modules(b_vm* vm)
{
    for(int i = 0; builtin_modules[i] != NULL; i++)
    {
        load_module(vm, builtin_modules[i], NULL, strdup("<__native__>"), NULL);
    }
    //bind_user_modules(vm, merge_paths(get_exe_dir(), "dist"));
    //bind_user_modules(vm, merge_paths(getcwd(NULL, 0), LOCAL_PACKAGES_DIRECTORY LOCAL_EXT_DIRECTORY));
}

char* load_user_module(b_vm* vm, const char* path, char* name)
{
    int length = (int)strlen(name) + 20;// 20 == strlen("blade_module_loader_")
    char* fn_name = ALLOCATE(char, length + 1);

    if(fn_name == NULL)
    {
        return "failed to load module";
    }

    sprintf(fn_name, "blade_module_loader_%s", name);
    fn_name[length] = '\0';// terminate the raw string

    void* handle;
    if((handle = dlopen(path, RTLD_LAZY)) == NULL)
    {
        return (char*)dlerror();
    }

    b_module_init fn = dlsym(handle, fn_name);
    if(fn == NULL)
    {
        return (char*)dlerror();
    }

    int path_length = (int)strlen(path);
    char* module_file = ALLOCATE(char, path_length + 1);
    memcpy(module_file, path, path_length);
    module_file[path_length] = '\0';

    if(!load_module(vm, fn, name, module_file, handle))
    {
        FREE_ARRAY(char, fn_name, length + 1);
        FREE_ARRAY(char, module_file, path_length + 1);
        dlclose(handle);
        return "failed to call module loader";
    }

    return NULL;
}

void close_dl_module(void* handle)
{
    dlclose(handle);
}

static b_obj_string* bin_to_string(b_vm* vm, long n)
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

    char new_str[1027];// assume maximum of 1024 bits + 0b (indicator) + sign (-).
    int length = 0;

    if(n < 0)
        new_str[length++] = '-';

    new_str[length++] = '0';
    new_str[length++] = 'b';

    for(int i = count - 1; i >= 0; i--)
    {
        new_str[length++] = str[i];
    }

    new_str[length++] = 0;

    return copy_string(vm, new_str, length);

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
    //  return copy_string(vm, str, length);
}

static b_obj_string* number_to_oct(b_vm* vm, long long n, bool numeric)
{
    char str[66];// assume maximum of 64 bits + 2 octal indicators (0c)
    int length = sprintf(str, numeric ? "0c%llo" : "%llo", n);

    return copy_string(vm, str, length);
}

static b_obj_string* number_to_hex(b_vm* vm, long long n, bool numeric)
{
    char str[66];// assume maximum of 64 bits + 2 hex indicators (0x)
    int length = sprintf(str, numeric ? "0x%llx" : "%llx", n);

    return copy_string(vm, str, length);
}

/**
 * time()
 *
 * returns the current timestamp in seconds
 */
bool cfn_time(b_vm* vm, int arg_count, b_value* args)
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
bool cfn_microtime(b_vm* vm, int arg_count, b_value* args)
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
bool cfn_id(b_vm* vm, int arg_count, b_value* args)
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
bool cfn_hasprop(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(hasprop, 2);
    ENFORCE_ARG_TYPE(hasprop, 0, IS_INSTANCE);
    ENFORCE_ARG_TYPE(hasprop, 1, IS_STRING);

    b_obj_instance* instance = AS_INSTANCE(args[0]);
    b_value dummy;
    RETURN_BOOL(table_get(&instance->properties, args[1], &dummy));
}

/**
 * getprop(object: instance, name: string)
 *
 * returns the property of the object matching the given name
 * or nil if the object contains no property with a matching
 * name
 */
bool cfn_getprop(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(getprop, 2);
    ENFORCE_ARG_TYPE(getprop, 0, IS_INSTANCE);
    ENFORCE_ARG_TYPE(getprop, 1, IS_STRING);

    b_obj_instance* instance = AS_INSTANCE(args[0]);
    b_value value;
    if(table_get(&instance->properties, args[1], &value) || table_get(&instance->klass->methods, args[1], &value))
    {
        RETURN_VALUE(value);
    }
    RETURN_NIL;
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
bool cfn_setprop(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(setprop, 3);
    ENFORCE_ARG_TYPE(setprop, 0, IS_INSTANCE);
    ENFORCE_ARG_TYPE(setprop, 1, IS_STRING);

    b_obj_instance* instance = AS_INSTANCE(args[0]);
    RETURN_BOOL(table_set(vm, &instance->properties, args[1], args[2]));
}

/**
 * delprop(object: instance, name: string)
 *
 * deletes the named property from the object
 * @returns bool
 */
bool cfn_delprop(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(delprop, 2);
    ENFORCE_ARG_TYPE(delprop, 0, IS_INSTANCE);
    ENFORCE_ARG_TYPE(delprop, 1, IS_STRING);

    b_obj_instance* instance = AS_INSTANCE(args[0]);
    RETURN_BOOL(table_delete(&instance->properties, args[1]));
}

/**
 * max(number...)
 *
 * returns the greatest of the number arguments
 */
bool cfn_max(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_MIN_ARG(max, 2);
    ENFORCE_ARG_TYPE(max, 0, IS_NUMBER);

    double max = AS_NUMBER(args[0]);

    for(int i = 1; i < arg_count; i++)
    {
        ENFORCE_ARG_TYPE(max, i, IS_NUMBER);
        double number = AS_NUMBER(args[i]);
        if(number > max)
            max = number;
    }

    RETURN_NUMBER(max);
}

/**
 * min(number...)
 *
 * returns the least of the number arguments
 */
bool cfn_min(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_MIN_ARG(min, 2);
    ENFORCE_ARG_TYPE(min, 0, IS_NUMBER);

    double min = AS_NUMBER(args[0]);

    for(int i = 1; i < arg_count; i++)
    {
        ENFORCE_ARG_TYPE(min, i, IS_NUMBER);
        double number = AS_NUMBER(args[i]);
        if(number < min)
            min = number;
    }

    RETURN_NUMBER(min);
}

/**
 * sum(number...)
 *
 * returns the summation of all numbers given
 */
bool cfn_sum(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_MIN_ARG(sum, 2);

    double sum = 0;
    for(int i = 0; i < arg_count; i++)
    {
        ENFORCE_ARG_TYPE(sum, i, IS_NUMBER);
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
bool cfn_abs(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(abs, 1);

    // handle classes that define a to_abs() method.
    METHOD_OVERRIDE(to_abs, 6);

    ENFORCE_ARG_TYPE(abs, 0, IS_NUMBER);
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
bool cfn_int(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_RANGE(int, 0, 1);

    if(arg_count == 0)
    {
        RETURN_NUMBER(0);
    }

    // handle classes that define a to_number() method.
    METHOD_OVERRIDE(to_number, 9);

    ENFORCE_ARG_TYPE(int, 0, IS_NUMBER);
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
bool cfn_bin(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(bin, 1);

    // handle classes that define a to_bin() method.
    METHOD_OVERRIDE(to_bin, 6);

    ENFORCE_ARG_TYPE(bin, 0, IS_NUMBER);
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
bool cfn_oct(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(oct, 1);

    // handle classes that define a to_oct() method.
    METHOD_OVERRIDE(to_oct, 6);

    ENFORCE_ARG_TYPE(oct, 0, IS_NUMBER);
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
bool cfn_hex(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(hex, 1);

    // handle classes that define a to_hex() method.
    METHOD_OVERRIDE(to_hex, 6);

    ENFORCE_ARG_TYPE(hex, 0, IS_NUMBER);
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
bool cfn_tobool(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(to_bool, 1);
    METHOD_OVERRIDE(to_bool, 7);
    RETURN_BOOL(!is_false(args[0]));
}

/**
 * to_string(value: any)
 *
 * convert a value into a string.
 *
 * native classes may override the return value by declaring a @to_string()
 * function.
 */
bool cfn_tostring(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(to_string, 1);
    METHOD_OVERRIDE(to_string, 9);
    char* result = value_to_string(vm, args[0]);
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
bool cfn_tonumber(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(to_number, 1);
    METHOD_OVERRIDE(to_number, 9);

    if(IS_NUMBER(args[0]))
    {
        RETURN_VALUE(args[0]);
    }
    else if(IS_BOOL(args[0]))
    {
        RETURN_NUMBER(AS_BOOL(args[0]) ? 1 : 0);
    }
    else if(IS_NIL(args[0]))
    {
        RETURN_NUMBER(-1);
    }

    const char* v = (const char*)value_to_string(vm, args[0]);
    int length = (int)strlen(v);

    int start = 0, end = 1, multiplier = 1;
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
bool cfn_toint(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(to_int, 1);
    METHOD_OVERRIDE(to_int, 6);
    ENFORCE_ARG_TYPE(to_int, 0, IS_NUMBER);
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
bool cfn_tolist(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(to_list, 1);
    METHOD_OVERRIDE(to_list, 0);

    if(IS_LIST(args[0]))
    {
        RETURN_VALUE(args[0]);
    }

    b_obj_list* list = (b_obj_list*)GC(new_list(vm));

    if(IS_DICT(args[0]))
    {
        b_obj_dict* dict = AS_DICT(args[0]);
        for(int i = 0; i < dict->names.count; i++)
        {
            b_obj_list* n_list = (b_obj_list*)GC(new_list(vm));
            write_value_arr(vm, &n_list->items, dict->names.values[i]);

            b_value value;
            table_get(&dict->items, dict->names.values[i], &value);
            write_value_arr(vm, &n_list->items, value);

            write_value_arr(vm, &list->items, OBJ_VAL(n_list));
        }
    }
    else if(IS_STRING(args[0]))
    {
        b_obj_string* str = AS_STRING(args[0]);
        for(int i = 0; i < str->utf8_length; i++)
        {
            int start = i, end = i + 1;
            utf8slice(str->chars, &start, &end);

            write_list(vm, list, STRING_L_VAL(str->chars + start, (int)(end - start)));
        }
    }
    else if(IS_RANGE(args[0]))
    {
        b_obj_range* range = AS_RANGE(args[0]);
        if(range->upper > range->lower)
        {
            for(int i = range->lower; i < range->upper; i++)
            {
                write_list(vm, list, NUMBER_VAL(i));
            }
        }
        else
        {
            for(int i = range->lower; i > range->upper; i--)
            {
                write_list(vm, list, NUMBER_VAL(i));
            }
        }
    }
    else
    {
        write_value_arr(vm, &list->items, args[0]);
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
bool cfn_todict(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(to_dict, 1);
    METHOD_OVERRIDE(to_dict, 7);

    if(IS_DICT(args[0]))
    {
        RETURN_VALUE(args[0]);
    }

    b_obj_dict* dict = (b_obj_dict*)GC(new_dict(vm));
    dict_set_entry(vm, dict, NUMBER_VAL(0), args[0]);

    RETURN_OBJ(dict);
}

/**
 * chr(i: number)
 *
 * return the string representing a character whose Unicode
 * code point is the number i.
 */
bool cfn_chr(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(chr, 1);
    ENFORCE_ARG_TYPE(chr, 0, IS_NUMBER);
    char* string = utf8_encode((int)AS_NUMBER(args[0]));
    RETURN_STRING(string);
}

/**
 * ord(ch: char)
 *
 * return the code point value of a unicode character.
 */
bool cfn_ord(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(ord, 1);
    ENFORCE_ARG_TYPE(ord, 0, IS_STRING);
    b_obj_string* string = AS_STRING(args[0]);

    int max_length = string->length > 1 && (int)string->chars[0] < 1 ? 3 : 1;

    if(string->length > max_length)
    {
        RETURN_ERROR("ord() expects character as argument, string given");
    }

    const uint8_t* bytes = (uint8_t*)string->chars;
    if((bytes[0] & 0xc0) == 0x80)
    {
        RETURN_NUMBER(-1);
    }

    // Decode the UTF-8 sequence.
    RETURN_NUMBER(utf8_decode((uint8_t*)string->chars, string->length));
}

/**
 * rand([limit: number, [upper: number]])
 *
 * - returns a random number between 0 and 1 if no argument is given
 * - returns a random number between 0 and limit if one argument is given
 * - returns a random number between limit and upper if two arguments is
 * given
 */
bool cfn_rand(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_RANGE(rand, 0, 2);
    int lower_limit = 0;
    int upper_limit = 1;

    if(arg_count > 0)
    {
        ENFORCE_ARG_TYPE(rand, 0, IS_NUMBER);
        lower_limit = AS_NUMBER(args[0]);
    }
    if(arg_count == 2)
    {
        ENFORCE_ARG_TYPE(rand, 1, IS_NUMBER);
        upper_limit = AS_NUMBER(args[1]);
    }

    if(lower_limit > upper_limit)
    {
        int tmp = upper_limit;
        upper_limit = lower_limit;
        lower_limit = tmp;
    }

    int n = upper_limit - lower_limit + 1;
    int remainder = RAND_MAX % n;
    int x;

    struct timeval tv;
    gettimeofday(&tv, NULL);
    srand((unsigned int)(10000000 * tv.tv_sec + tv.tv_usec + time(NULL)));

    if(lower_limit == 0 && upper_limit == 1)
    {
        RETURN_NUMBER((double)rand() / RAND_MAX);
    }
    else
    {
        do
        {
            x = rand();
        } while(x >= RAND_MAX - remainder);

        RETURN_NUMBER((double)lower_limit + x % n);
    }
}

/**
 * type(value: any)
 *
 * returns the name of the type of value
 */
bool cfn_typeof(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(typeof, 1);
    char* result = (char*)value_type(args[0]);
    RETURN_STRING(result);
}

/**
 * is_callable(value: any)
 *
 * returns true if the value is a callable function or class and false otherwise
 */
bool cfn_iscallable(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(is_callable, 1);
    RETURN_BOOL(IS_CLASS(args[0]) || IS_FUNCTION(args[0]) || IS_CLOSURE(args[0]) || IS_BOUND(args[0]) || IS_NATIVE(args[0]));
}

/**
 * is_bool(value: any)
 *
 * returns true if the value is a boolean or false otherwise
 */
bool cfn_isbool(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(is_bool, 1);
    RETURN_BOOL(IS_BOOL(args[0]));
}

/**
 * is_number(value: any)
 *
 * returns true if the value is a number or false otherwise
 */
bool cfn_isnumber(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(is_number, 1);
    RETURN_BOOL(IS_NUMBER(args[0]));
}

/**
 * is_int(value: any)
 *
 * returns true if the value is an integer or false otherwise
 */
bool cfn_isint(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(is_int, 1);
    RETURN_BOOL(IS_NUMBER(args[0]) && (((int)AS_NUMBER(args[0])) == AS_NUMBER(args[0])));
}

/**
 * is_string(value: any)
 *
 * returns true if the value is a string or false otherwise
 */
bool cfn_isstring(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(is_string, 1);
    RETURN_BOOL(IS_STRING(args[0]));
}

/**
 * is_bytes(value: any)
 *
 * returns true if the value is a bytes or false otherwise
 */
bool cfn_isbytes(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(is_bytes, 1);
    RETURN_BOOL(IS_BYTES(args[0]));
}

/**
 * is_list(value: any)
 *
 * returns true if the value is a list or false otherwise
 */
bool cfn_islist(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(is_list, 1);
    RETURN_BOOL(IS_LIST(args[0]));
}

/**
 * is_dict(value: any)
 *
 * returns true if the value is a dictionary or false otherwise
 */
bool cfn_isdict(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(is_dict, 1);
    RETURN_BOOL(IS_DICT(args[0]));
}

/**
 * is_object(value: any)
 *
 * returns true if the value is an object or false otherwise
 */
bool cfn_isobject(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(is_object, 1);
    RETURN_BOOL(IS_OBJ(args[0]));
}

/**
 * is_function(value: any)
 *
 * returns true if the value is a function or false otherwise
 */
bool cfn_isfunction(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(is_function, 1);
    RETURN_BOOL(IS_FUNCTION(args[0]) || IS_CLOSURE(args[0]) || IS_BOUND(args[0]) || IS_NATIVE(args[0]));
}

/**
 * is_iterable(value: any)
 *
 * returns true if the value is an iterable or false otherwise
 */
bool cfn_isiterable(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(is_iterable, 1);
    bool is_iterable = IS_LIST(args[0]) || IS_DICT(args[0]) || IS_STRING(args[0]) || IS_BYTES(args[0]);
    if(!is_iterable && IS_INSTANCE(args[0]))
    {
        b_obj_class* klass = AS_INSTANCE(args[0])->klass;
        b_value dummy;
        is_iterable = table_get(&klass->methods, STRING_VAL("@iter"), &dummy) && table_get(&klass->methods, STRING_VAL("@itern"), &dummy);
    }
    RETURN_BOOL(is_iterable);
}

/**
 * is_class(value: any)
 *
 * returns true if the value is a class or false otherwise
 */
bool cfn_isclass(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(is_class, 1);
    RETURN_BOOL(IS_CLASS(args[0]));
}

/**
 * is_file(value: any)
 *
 * returns true if the value is a file or false otherwise
 */
bool cfn_isfile(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(is_file, 1);
    RETURN_BOOL(IS_FILE(args[0]));
}

/**
 * is_instance(value: any)
 *
 * returns true if the value is an instance of a class
 */
bool cfn_isinstance(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(is_instance, 1);
    RETURN_BOOL(IS_INSTANCE(args[0]));
}

/**
 * instance_of(value: any, name: class)
 *
 * returns true if the value is an instance the given class, false
 * otherwise
 */
bool cfn_instanceof(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(instance_of, 2);
    ENFORCE_ARG_TYPE(instance_of, 0, IS_INSTANCE);
    ENFORCE_ARG_TYPE(instance_of, 1, IS_CLASS);

    RETURN_BOOL(bl_class_isinstanceof(AS_INSTANCE(args[0])->klass, AS_CLASS(args[1])->name->chars));
}

//------------------------------------------------------------------------------

/**
 * print(...)
 *
 * prints values to the standard output
 */
bool cfn_print(b_vm* vm, int arg_count, b_value* args)
{
    for(int i = 0; i < arg_count; i++)
    {
        print_value(args[i]);
        if(i != arg_count - 1)
        {
            printf(" ");
        }
    }
    if(vm->is_repl)
    {
        printf("\n");
    }
    RETURN_NUMBER(0);
}

bool native_method_rangelower(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(lower, 0);
    RETURN_NUMBER(AS_RANGE(METHOD_OBJECT)->lower);
}

bool native_method_rangeupper(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(upper, 0);
    RETURN_NUMBER(AS_RANGE(METHOD_OBJECT)->upper);
}

bool native_method_range__iter__(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(__iter__, 1);
    ENFORCE_ARG_TYPE(__iter__, 0, IS_NUMBER);

    b_obj_range* range = AS_RANGE(METHOD_OBJECT);

    int index = AS_NUMBER(args[0]);

    if(index >= 0 && index < range->range)
    {
        if(index == 0)
            RETURN_NUMBER(range->lower);
        RETURN_NUMBER(range->lower > range->upper ? --range->lower : ++range->lower);
    }

    RETURN_NIL;
}

bool native_method_range__itern__(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(__itern__, 1);
    b_obj_range* range = AS_RANGE(METHOD_OBJECT);

    if(IS_NIL(args[0]))
    {
        if(range->range == 0)
        {
            RETURN_NIL;
        }
        RETURN_NUMBER(0);
    }

    if(!IS_NUMBER(args[0]))
    {
        RETURN_ERROR("ranges are numerically indexed");
    }

    int index = (int)AS_NUMBER(args[0]) + 1;
    if(index < range->range)
    {
        RETURN_NUMBER(index);
    }

    RETURN_NIL;
}

void array_free(void* data)
{
    if(data)
    {
        free(data);
    }
}

b_obj_ptr* new_array(b_vm* vm, b_array* array)
{
    b_obj_ptr* ptr = (b_obj_ptr*)GC(new_ptr(vm, array));
    ptr->free_fn = &array_free;
    return ptr;
}

//--------- INT 16 STARTS -------------------------
b_array* new_int16_array(b_vm* vm, int length)
{
    b_array* array = (b_array*)allocate_object(vm, sizeof(b_array), OBJ_BYTES);
    array->length = length;
    array->buffer = ALLOCATE(int16_t, length);
    return array;
}

bool native_module_array__int16array(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(int16array, 1);
    if(IS_NUMBER(args[0]))
    {
        RETURN_OBJ(new_array(vm, new_int16_array(vm, (int)AS_NUMBER(args[0]))));
    }
    else if(IS_LIST(args[0]))
    {
        b_obj_list* list = AS_LIST(args[0]);
        b_array* array = new_int16_array(vm, list->items.count);
        int16_t* values = (int16_t*)array->buffer;

        for(int i = 0; i < list->items.count; i++)
        {
            if(!IS_NUMBER(list->items.values[i]))
            {
                RETURN_ERROR("Int16Array() expects a list of valid int16");
            }

            values[i] = (int16_t)AS_NUMBER(list->items.values[i]);
        }

        RETURN_OBJ(new_array(vm, array));
    }

    RETURN_ERROR("expected array size or int16 list as argument");
}

bool native_module_array_int16_append(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(append, 2);
    ENFORCE_ARG_TYPE(append, 0, IS_PTR);
    b_array* array = (b_array*)AS_PTR(args[0])->pointer;

    if(IS_NUMBER(args[1]))
    {
        array->buffer = GROW_ARRAY(int16_t, array->buffer, array->length, array->length++);

        int16_t* values = (int16_t*)array->buffer;
        values[array->length - 1] = (int16_t)AS_NUMBER(args[1]);
    }
    else if(IS_LIST(args[1]))
    {
        b_obj_list* list = AS_LIST(args[1]);
        if(list->items.count > 0)
        {
            array->buffer = GROW_ARRAY(int16_t, array->buffer, array->length, array->length + list->items.count);

            int16_t* values = (int16_t*)array->buffer;

            for(int i = 0; i < list->items.count; i++)
            {
                if(!IS_NUMBER(list->items.values[i]))
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

    RETURN;
}

bool native_module_array_int16_get(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(get, 2);
    ENFORCE_ARG_TYPE(get, 0, IS_PTR);
    ENFORCE_ARG_TYPE(get, 1, IS_NUMBER);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;
    int16_t* data = (int16_t*)array->buffer;

    int index = AS_NUMBER(args[1]);
    if(index < 0 || index >= array->length)
    {
        RETURN_ERROR("Int16Array index %d out of range", index);
    }

    RETURN_NUMBER((double)data[index]);
}

bool native_module_array_int16_reverse(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(reverse, 1);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;
    int16_t* data = (int16_t*)array->buffer;

    b_array* n_array = new_int16_array(vm, array->length);
    int16_t* n_data = (int16_t*)n_array->buffer;

    for(int i = array->length - 1; i >= 0; i--)
    {
        n_data[i] = data[i];
    }

    RETURN_OBJ(new_array(vm, n_array));
}

bool native_module_array_int16_clone(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(clone, 1);
    ENFORCE_ARG_TYPE(clone, 0, IS_PTR);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;

    b_array* n_array = new_int16_array(vm, array->length);
    memcpy(n_array->buffer, array->buffer, array->length);

    RETURN_OBJ(new_array(vm, n_array));
}

bool native_module_array_int16_pop(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(pop, 1);
    ENFORCE_ARG_TYPE(pop, 0, IS_PTR);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;
    int16_t last = ((int16_t*)array->buffer)[array->length - 1];
    array->length--;

    RETURN_NUMBER(last);
}

bool native_module_array_int16_remove(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(remove, 2);
    ENFORCE_ARG_TYPE(remove, 0, IS_PTR);
    ENFORCE_ARG_TYPE(remove, 1, IS_NUMBER);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;
    int16_t* values = (int16_t*)array->buffer;

    int index = AS_NUMBER(args[1]);

    if(index < 0 || index >= array->length)
    {
        RETURN_ERROR("Int16Array index %d out of range", index);
    }

    int16_t val = values[index];

    for(int i = index; i < array->length; i++)
    {
        values[i] = values[i + 1];
    }
    array->length--;

    RETURN_NUMBER(val);
}

bool native_module_array_int16_to_list(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(to_list, 1);
    ENFORCE_ARG_TYPE(to_list, 0, IS_PTR);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;
    int16_t* values = (int16_t*)array->buffer;

    b_obj_list* list = (b_obj_list*)GC(new_list(vm));

    for(int i = 0; i < array->length; i++)
    {
        write_list(vm, list, NUMBER_VAL((double)values[i]));
    }

    RETURN_OBJ(list);
}

bool native_module_array_int16_to_bytes(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(to_list, 1);
    ENFORCE_ARG_TYPE(to_list, 0, IS_PTR);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;

    b_obj_bytes* bytes = (b_obj_bytes*)GC(new_bytes(vm, array->length * 2));
    memcpy(bytes->bytes.bytes, array->buffer, array->length * 2);

    RETURN_OBJ(bytes);
}

bool native_module_array_int16___iter__(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(@iter, 2);
    ENFORCE_ARG_TYPE(@iter, 0, IS_PTR);
    ENFORCE_ARG_TYPE(@iter, 1, IS_NUMBER);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;
    int16_t* values = (int16_t*)array->buffer;

    int index = AS_NUMBER(args[1]);

    if(index > -1 && index < array->length)
    {
        RETURN_NUMBER(values[index]);
    }

    RETURN_NIL;
}

//--------- INT 32 STARTS -------------------------

b_array* new_int32_array(b_vm* vm, int length)
{
    b_array* array = (b_array*)allocate_object(vm, sizeof(b_array), OBJ_BYTES);
    array->length = length;
    array->buffer = ALLOCATE(int32_t, length);
    return array;
}

bool native_module_array__int32array(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(int32array, 1);
    if(IS_NUMBER(args[0]))
    {
        RETURN_OBJ(new_array(vm, new_int32_array(vm, (int)AS_NUMBER(args[0]))));
    }
    else if(IS_LIST(args[0]))
    {
        b_obj_list* list = AS_LIST(args[0]);
        b_array* array = new_int32_array(vm, list->items.count);
        int32_t* values = (int32_t*)array->buffer;

        for(int i = 0; i < list->items.count; i++)
        {
            if(!IS_NUMBER(list->items.values[i]))
            {
                RETURN_ERROR("Int32Array() expects a list of valid int32");
            }

            values[i] = (int32_t)AS_NUMBER(list->items.values[i]);
        }

        RETURN_OBJ(new_array(vm, array));
    }

    RETURN_ERROR("expected array size or int32 list as argument");
}

bool native_module_array_int32_append(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(append, 2);
    ENFORCE_ARG_TYPE(append, 0, IS_PTR);
    b_array* array = (b_array*)AS_PTR(args[0])->pointer;

    if(IS_NUMBER(args[1]))
    {
        array->buffer = GROW_ARRAY(int32_t, array->buffer, array->length, array->length++);

        int32_t* values = (int32_t*)array->buffer;
        values[array->length - 1] = (int32_t)AS_NUMBER(args[1]);
    }
    else if(IS_LIST(args[1]))
    {
        b_obj_list* list = AS_LIST(args[1]);
        if(list->items.count > 0)
        {
            array->buffer = GROW_ARRAY(int32_t, array->buffer, array->length, array->length + list->items.count);

            int32_t* values = (int32_t*)array->buffer;

            for(int i = 0; i < list->items.count; i++)
            {
                if(!IS_NUMBER(list->items.values[i]))
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

    RETURN;
}

bool native_module_array_int32_get(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(get, 2);
    ENFORCE_ARG_TYPE(get, 0, IS_PTR);
    ENFORCE_ARG_TYPE(get, 1, IS_NUMBER);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;
    int32_t* data = (int32_t*)array->buffer;

    int index = AS_NUMBER(args[1]);
    if(index < 0 || index >= array->length)
    {
        RETURN_ERROR("Int32Array index %d out of range", index);
    }

    RETURN_NUMBER((double)data[index]);
}

bool native_module_array_int32_reverse(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(reverse, 1);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;
    int32_t* data = (int32_t*)array->buffer;

    b_array* n_array = new_int32_array(vm, array->length);
    int32_t* n_data = (int32_t*)n_array->buffer;

    for(int i = array->length - 1; i >= 0; i--)
    {
        n_data[i] = data[i];
    }

    RETURN_OBJ(new_array(vm, n_array));
}

bool native_module_array_int32_clone(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(clone, 1);
    ENFORCE_ARG_TYPE(clone, 0, IS_PTR);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;

    b_array* n_array = new_int32_array(vm, array->length);
    memcpy(n_array->buffer, array->buffer, array->length);

    RETURN_OBJ(new_array(vm, n_array));
}

bool native_module_array_int32_pop(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(pop, 1);
    ENFORCE_ARG_TYPE(pop, 0, IS_PTR);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;
    int32_t last = ((int32_t*)array->buffer)[array->length - 1];
    array->length--;

    RETURN_NUMBER(last);
}

bool native_module_array_int32_remove(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(remove, 2);
    ENFORCE_ARG_TYPE(remove, 0, IS_PTR);
    ENFORCE_ARG_TYPE(remove, 1, IS_NUMBER);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;
    int32_t* values = (int32_t*)array->buffer;

    int index = AS_NUMBER(args[1]);

    if(index < 0 || index >= array->length)
    {
        RETURN_ERROR("Int32Array index %d out of range", index);
    }

    int32_t val = values[index];

    for(int i = index; i < array->length; i++)
    {
        values[i] = values[i + 1];
    }
    array->length--;

    RETURN_NUMBER(val);
}

bool native_module_array_int32_to_list(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(to_list, 1);
    ENFORCE_ARG_TYPE(to_list, 0, IS_PTR);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;
    int32_t* values = (int32_t*)array->buffer;

    b_obj_list* list = (b_obj_list*)GC(new_list(vm));

    for(int i = 0; i < array->length; i++)
    {
        write_list(vm, list, NUMBER_VAL((double)values[i]));
    }

    RETURN_OBJ(list);
}

bool native_module_array_int32_to_bytes(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(to_list, 1);
    ENFORCE_ARG_TYPE(to_list, 0, IS_PTR);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;

    b_obj_bytes* bytes = (b_obj_bytes*)GC(new_bytes(vm, array->length * 4));
    memcpy(bytes->bytes.bytes, array->buffer, array->length * 4);

    RETURN_OBJ(bytes);
}

bool native_module_array_int32___iter__(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(@iter, 2);
    ENFORCE_ARG_TYPE(@iter, 0, IS_PTR);
    ENFORCE_ARG_TYPE(@iter, 1, IS_NUMBER);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;
    int32_t* values = (int32_t*)array->buffer;

    int index = AS_NUMBER(args[1]);

    if(index > -1 && index < array->length)
    {
        RETURN_NUMBER(values[index]);
    }

    RETURN_NIL;
}

//--------- INT 64 STARTS -------------------------

b_array* new_int64_array(b_vm* vm, int length)
{
    b_array* array = (b_array*)allocate_object(vm, sizeof(b_array), OBJ_BYTES);
    array->length = length;
    array->buffer = ALLOCATE(int64_t, length);
    return array;
}

bool native_module_array__int64array(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(int64array, 1);
    if(IS_NUMBER(args[0]))
    {
        RETURN_OBJ(new_array(vm, new_int64_array(vm, (int)AS_NUMBER(args[0]))));
    }
    else if(IS_LIST(args[0]))
    {
        b_obj_list* list = AS_LIST(args[0]);
        b_array* array = new_int64_array(vm, list->items.count);
        int64_t* values = (int64_t*)array->buffer;

        for(int i = 0; i < list->items.count; i++)
        {
            if(!IS_NUMBER(list->items.values[i]))
            {
                RETURN_ERROR("Int64Array() expects a list of valid int64");
            }

            values[i] = (int64_t)AS_NUMBER(list->items.values[i]);
        }

        RETURN_OBJ(new_array(vm, array));
    }

    RETURN_ERROR("expected array size or int64 list as argument");
}

bool native_module_array_int64_append(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(append, 2);
    ENFORCE_ARG_TYPE(append, 0, IS_PTR);
    b_array* array = (b_array*)AS_PTR(args[0])->pointer;

    if(IS_NUMBER(args[1]))
    {
        array->buffer = GROW_ARRAY(int64_t, array->buffer, array->length, array->length++);

        int64_t* values = (int64_t*)array->buffer;
        values[array->length - 1] = (int64_t)AS_NUMBER(args[1]);
    }
    else if(IS_LIST(args[1]))
    {
        b_obj_list* list = AS_LIST(args[1]);
        if(list->items.count > 0)
        {
            array->buffer = GROW_ARRAY(int64_t, array->buffer, array->length, array->length + list->items.count);

            int64_t* values = (int64_t*)array->buffer;

            for(int i = 0; i < list->items.count; i++)
            {
                if(!IS_NUMBER(list->items.values[i]))
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

    RETURN;
}

bool native_module_array_int64_get(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(get, 2);
    ENFORCE_ARG_TYPE(get, 0, IS_PTR);
    ENFORCE_ARG_TYPE(get, 1, IS_NUMBER);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;
    int64_t* data = (int64_t*)array->buffer;

    int index = AS_NUMBER(args[1]);
    if(index < 0 || index >= array->length)
    {
        RETURN_ERROR("Int64Array index %d out of range", index);
    }

    RETURN_NUMBER((double)data[index]);
}

bool native_module_array_int64_reverse(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(reverse, 1);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;
    int64_t* data = (int64_t*)array->buffer;

    b_array* n_array = new_int64_array(vm, array->length);
    int64_t* n_data = (int64_t*)n_array->buffer;

    for(int i = array->length - 1; i >= 0; i--)
    {
        n_data[i] = data[i];
    }

    RETURN_OBJ(new_array(vm, n_array));
}

bool native_module_array_int64_clone(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(clone, 1);
    ENFORCE_ARG_TYPE(clone, 0, IS_PTR);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;

    b_array* n_array = new_int64_array(vm, array->length);
    memcpy(n_array->buffer, array->buffer, array->length);

    RETURN_OBJ(new_array(vm, n_array));
}

bool native_module_array_int64_pop(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(pop, 1);
    ENFORCE_ARG_TYPE(pop, 0, IS_PTR);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;
    int64_t last = ((int64_t*)array->buffer)[array->length - 1];
    array->length--;

    RETURN_NUMBER(last);
}

bool native_module_array_int64_remove(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(remove, 2);
    ENFORCE_ARG_TYPE(remove, 0, IS_PTR);
    ENFORCE_ARG_TYPE(remove, 1, IS_NUMBER);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;
    int64_t* values = (int64_t*)array->buffer;

    int index = AS_NUMBER(args[1]);

    if(index < 0 || index >= array->length)
    {
        RETURN_ERROR("Int64Array index %d out of range", index);
    }

    int64_t val = values[index];

    for(int i = index; i < array->length; i++)
    {
        values[i] = values[i + 1];
    }
    array->length--;

    RETURN_NUMBER(val);
}

bool native_module_array_int64_to_list(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(to_list, 1);
    ENFORCE_ARG_TYPE(to_list, 0, IS_PTR);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;
    int64_t* values = (int64_t*)array->buffer;

    b_obj_list* list = (b_obj_list*)GC(new_list(vm));

    for(int i = 0; i < array->length; i++)
    {
        write_list(vm, list, NUMBER_VAL((double)values[i]));
    }

    RETURN_OBJ(list);
}

bool native_module_array_int64_to_bytes(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(to_list, 1);
    ENFORCE_ARG_TYPE(to_list, 0, IS_PTR);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;

    b_obj_bytes* bytes = (b_obj_bytes*)GC(new_bytes(vm, array->length * 8));
    memcpy(bytes->bytes.bytes, array->buffer, array->length * 8);

    RETURN_OBJ(bytes);
}

bool native_module_array_int64___iter__(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(@iter, 2);
    ENFORCE_ARG_TYPE(@iter, 0, IS_PTR);
    ENFORCE_ARG_TYPE(@iter, 1, IS_NUMBER);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;
    int64_t* values = (int64_t*)array->buffer;

    int index = AS_NUMBER(args[1]);

    if(index > -1 && index < array->length)
    {
        RETURN_NUMBER(values[index]);
    }

    RETURN_NIL;
}

//--------- Unsigned INT 16 STARTS ----------------

b_array* new_uint16_array(b_vm* vm, int length)
{
    b_array* array = (b_array*)allocate_object(vm, sizeof(b_array), OBJ_BYTES);
    array->length = length;
    array->buffer = ALLOCATE(uint16_t, length);
    return array;
}

bool native_module_array__uint16array(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(uint16array, 1);
    if(IS_NUMBER(args[0]))
    {
        RETURN_OBJ(new_array(vm, new_uint16_array(vm, (int)AS_NUMBER(args[0]))));
    }
    else if(IS_LIST(args[0]))
    {
        b_obj_list* list = AS_LIST(args[0]);
        b_array* array = new_uint16_array(vm, list->items.count);
        uint16_t* values = (uint16_t*)array->buffer;

        for(int i = 0; i < list->items.count; i++)
        {
            if(!IS_NUMBER(list->items.values[i]))
            {
                RETURN_ERROR("UInt16Array() expects a list of valid uint16");
            }

            values[i] = (uint16_t)AS_NUMBER(list->items.values[i]);
        }

        RETURN_OBJ(new_array(vm, array));
    }

    RETURN_ERROR("expected array size or uint16 list as argument");
}

bool native_module_array_uint16_append(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(append, 2);
    ENFORCE_ARG_TYPE(append, 0, IS_PTR);
    b_array* array = (b_array*)AS_PTR(args[0])->pointer;

    if(IS_NUMBER(args[1]))
    {
        array->buffer = GROW_ARRAY(uint16_t, array->buffer, array->length, array->length++);

        uint16_t* values = (uint16_t*)array->buffer;
        values[array->length - 1] = (uint16_t)AS_NUMBER(args[1]);
    }
    else if(IS_LIST(args[1]))
    {
        b_obj_list* list = AS_LIST(args[1]);
        if(list->items.count > 0)
        {
            array->buffer = GROW_ARRAY(uint16_t, array->buffer, array->length, array->length + list->items.count);

            uint16_t* values = (uint16_t*)array->buffer;

            for(int i = 0; i < list->items.count; i++)
            {
                if(!IS_NUMBER(list->items.values[i]))
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

    RETURN;
}

bool native_module_array_uint16_get(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(get, 2);
    ENFORCE_ARG_TYPE(get, 0, IS_PTR);
    ENFORCE_ARG_TYPE(get, 1, IS_NUMBER);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;
    uint16_t* data = (uint16_t*)array->buffer;

    int index = AS_NUMBER(args[1]);
    if(index < 0 || index >= array->length)
    {
        RETURN_ERROR("UInt16Array index %d out of range", index);
    }

    RETURN_NUMBER((double)data[index]);
}

bool native_module_array_uint16_reverse(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(reverse, 1);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;
    uint16_t* data = (uint16_t*)array->buffer;

    b_array* n_array = new_uint16_array(vm, array->length);
    uint16_t* n_data = (uint16_t*)n_array->buffer;

    for(int i = array->length - 1; i >= 0; i--)
    {
        n_data[i] = data[i];
    }

    RETURN_OBJ(new_array(vm, n_array));
}

bool native_module_array_uint16_clone(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(clone, 1);
    ENFORCE_ARG_TYPE(clone, 0, IS_PTR);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;

    b_array* n_array = new_uint16_array(vm, array->length);
    memcpy(n_array->buffer, array->buffer, array->length);

    RETURN_OBJ(new_array(vm, n_array));
}

bool native_module_array_uint16_pop(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(pop, 1);
    ENFORCE_ARG_TYPE(pop, 0, IS_PTR);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;
    uint16_t last = ((uint16_t*)array->buffer)[array->length - 1];
    array->length--;

    RETURN_NUMBER(last);
}

bool native_module_array_uint16_remove(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(remove, 2);
    ENFORCE_ARG_TYPE(remove, 0, IS_PTR);
    ENFORCE_ARG_TYPE(remove, 1, IS_NUMBER);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;
    uint16_t* values = (uint16_t*)array->buffer;

    int index = AS_NUMBER(args[1]);

    if(index < 0 || index >= array->length)
    {
        RETURN_ERROR("UInt16Array index %d out of range", index);
    }

    uint16_t val = values[index];

    for(int i = index; i < array->length; i++)
    {
        values[i] = values[i + 1];
    }
    array->length--;

    RETURN_NUMBER(val);
}

bool native_module_array_uint16_to_list(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(to_list, 1);
    ENFORCE_ARG_TYPE(to_list, 0, IS_PTR);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;
    uint16_t* values = (uint16_t*)array->buffer;

    b_obj_list* list = (b_obj_list*)GC(new_list(vm));

    for(int i = 0; i < array->length; i++)
    {
        write_list(vm, list, NUMBER_VAL((double)values[i]));
    }

    RETURN_OBJ(list);
}

bool native_module_array_uint16_to_bytes(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(to_list, 1);
    ENFORCE_ARG_TYPE(to_list, 0, IS_PTR);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;

    b_obj_bytes* bytes = (b_obj_bytes*)GC(new_bytes(vm, array->length * 2));
    memcpy(bytes->bytes.bytes, array->buffer, array->length * 2);

    RETURN_OBJ(bytes);
}

bool native_module_array_uint16___iter__(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(@iter, 2);
    ENFORCE_ARG_TYPE(@iter, 0, IS_PTR);
    ENFORCE_ARG_TYPE(@iter, 1, IS_NUMBER);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;
    uint16_t* values = (uint16_t*)array->buffer;

    int index = AS_NUMBER(args[1]);

    if(index > -1 && index < array->length)
    {
        RETURN_NUMBER(values[index]);
    }

    RETURN_NIL;
}

//--------- Unsigned INT 32 STARTS ----------------

b_array* new_uint32_array(b_vm* vm, int length)
{
    b_array* array = (b_array*)allocate_object(vm, sizeof(b_array), OBJ_BYTES);
    array->length = length;
    array->buffer = ALLOCATE(uint32_t, length);
    return array;
}

bool native_module_array__uint32array(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(uint32array, 1);
    if(IS_NUMBER(args[0]))
    {
        RETURN_OBJ(new_array(vm, new_uint32_array(vm, (int)AS_NUMBER(args[0]))));
    }
    else if(IS_LIST(args[0]))
    {
        b_obj_list* list = AS_LIST(args[0]);
        b_array* array = new_uint32_array(vm, list->items.count);
        uint32_t* values = (uint32_t*)array->buffer;

        for(int i = 0; i < list->items.count; i++)
        {
            if(!IS_NUMBER(list->items.values[i]))
            {
                RETURN_ERROR("UInt32Array() expects a list of valid uint32");
            }

            values[i] = (uint32_t)AS_NUMBER(list->items.values[i]);
        }

        RETURN_OBJ(new_array(vm, array));
    }

    RETURN_ERROR("expected array size or uint32 list as argument");
}

bool native_module_array_uint32_append(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(append, 2);
    ENFORCE_ARG_TYPE(append, 0, IS_PTR);
    b_array* array = (b_array*)AS_PTR(args[0])->pointer;

    if(IS_NUMBER(args[1]))
    {
        array->buffer = GROW_ARRAY(uint32_t, array->buffer, array->length, array->length++);

        uint32_t* values = (uint32_t*)array->buffer;
        values[array->length - 1] = (uint32_t)AS_NUMBER(args[1]);
    }
    else if(IS_LIST(args[1]))
    {
        b_obj_list* list = AS_LIST(args[1]);
        if(list->items.count > 0)
        {
            array->buffer = GROW_ARRAY(uint32_t, array->buffer, array->length, array->length + list->items.count);

            uint32_t* values = (uint32_t*)array->buffer;

            for(int i = 0; i < list->items.count; i++)
            {
                if(!IS_NUMBER(list->items.values[i]))
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

    RETURN;
}

bool native_module_array_uint32_get(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(get, 2);
    ENFORCE_ARG_TYPE(get, 0, IS_PTR);
    ENFORCE_ARG_TYPE(get, 1, IS_NUMBER);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;
    uint32_t* data = (uint32_t*)array->buffer;

    int index = AS_NUMBER(args[1]);
    if(index < 0 || index >= array->length)
    {
        RETURN_ERROR("UInt32Array index %d out of range", index);
    }

    RETURN_NUMBER((double)data[index]);
}

bool native_module_array_uint32_reverse(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(reverse, 1);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;
    uint32_t* data = (uint32_t*)array->buffer;

    b_array* n_array = new_uint32_array(vm, array->length);
    uint32_t* n_data = (uint32_t*)n_array->buffer;

    for(int i = array->length - 1; i >= 0; i--)
    {
        n_data[i] = data[i];
    }

    RETURN_OBJ(new_array(vm, n_array));
}

bool native_module_array_uint32_clone(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(clone, 1);
    ENFORCE_ARG_TYPE(clone, 0, IS_PTR);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;

    b_array* n_array = new_uint32_array(vm, array->length);
    memcpy(n_array->buffer, array->buffer, array->length);

    RETURN_OBJ(new_array(vm, n_array));
}

bool native_module_array_uint32_pop(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(pop, 1);
    ENFORCE_ARG_TYPE(pop, 0, IS_PTR);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;
    uint32_t last = ((uint32_t*)array->buffer)[array->length - 1];
    array->length--;

    RETURN_NUMBER(last);
}

bool native_module_array_uint32_remove(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(remove, 2);
    ENFORCE_ARG_TYPE(remove, 0, IS_PTR);
    ENFORCE_ARG_TYPE(remove, 1, IS_NUMBER);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;
    uint32_t* values = (uint32_t*)array->buffer;

    int index = AS_NUMBER(args[1]);

    if(index < 0 || index >= array->length)
    {
        RETURN_ERROR("UInt32Array index %d out of range", index);
    }

    uint32_t val = values[index];

    for(int i = index; i < array->length; i++)
    {
        values[i] = values[i + 1];
    }
    array->length--;

    RETURN_NUMBER(val);
}

bool native_module_array_uint32_to_list(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(to_list, 1);
    ENFORCE_ARG_TYPE(to_list, 0, IS_PTR);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;
    uint32_t* values = (uint32_t*)array->buffer;

    b_obj_list* list = (b_obj_list*)GC(new_list(vm));

    for(int i = 0; i < array->length; i++)
    {
        write_list(vm, list, NUMBER_VAL((double)values[i]));
    }

    RETURN_OBJ(list);
}

bool native_module_array_uint32_to_bytes(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(to_list, 1);
    ENFORCE_ARG_TYPE(to_list, 0, IS_PTR);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;

    b_obj_bytes* bytes = (b_obj_bytes*)GC(new_bytes(vm, array->length * 4));
    memcpy(bytes->bytes.bytes, array->buffer, array->length * 4);

    RETURN_OBJ(bytes);
}

bool native_module_array_uint32___iter__(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(@iter, 2);
    ENFORCE_ARG_TYPE(@iter, 0, IS_PTR);
    ENFORCE_ARG_TYPE(@iter, 1, IS_NUMBER);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;
    uint32_t* values = (uint32_t*)array->buffer;

    int index = AS_NUMBER(args[1]);

    if(index > -1 && index < array->length)
    {
        RETURN_NUMBER(values[index]);
    }

    RETURN_NIL;
}

//--------- Unsigned INT 64 STARTS ----------------

b_array* new_uint64_array(b_vm* vm, int length)
{
    b_array* array = (b_array*)allocate_object(vm, sizeof(b_array), OBJ_BYTES);
    array->length = length;
    array->buffer = ALLOCATE(int64_t, length);
    return array;
}

bool native_module_array__uint64array(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(uint32array, 1);
    if(IS_NUMBER(args[0]))
    {
        RETURN_OBJ(new_array(vm, new_uint64_array(vm, (int)AS_NUMBER(args[0]))));
    }
    else if(IS_LIST(args[0]))
    {
        b_obj_list* list = AS_LIST(args[0]);
        b_array* array = new_uint64_array(vm, list->items.count);
        uint64_t* values = (uint64_t*)array->buffer;

        for(int i = 0; i < list->items.count; i++)
        {
            if(!IS_NUMBER(list->items.values[i]))
            {
                RETURN_ERROR("UInt32Array() expects a list of valid uint64");
            }

            values[i] = (uint64_t)AS_NUMBER(list->items.values[i]);
        }

        RETURN_OBJ(new_array(vm, array));
    }

    RETURN_ERROR("expected array size or uint64 list as argument");
}

bool native_module_array_uint64_append(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(append, 2);
    ENFORCE_ARG_TYPE(append, 0, IS_PTR);
    b_array* array = (b_array*)AS_PTR(args[0])->pointer;

    if(IS_NUMBER(args[1]))
    {
        array->buffer = GROW_ARRAY(uint64_t, array->buffer, array->length, array->length++);

        uint64_t* values = (uint64_t*)array->buffer;
        values[array->length - 1] = (uint64_t)AS_NUMBER(args[1]);
    }
    else if(IS_LIST(args[1]))
    {
        b_obj_list* list = AS_LIST(args[1]);
        if(list->items.count > 0)
        {
            array->buffer = GROW_ARRAY(uint64_t, array->buffer, array->length, array->length + list->items.count);

            uint64_t* values = (uint64_t*)array->buffer;

            for(int i = 0; i < list->items.count; i++)
            {
                if(!IS_NUMBER(list->items.values[i]))
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

    RETURN;
}

bool native_module_array_uint64_get(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(get, 2);
    ENFORCE_ARG_TYPE(get, 0, IS_PTR);
    ENFORCE_ARG_TYPE(get, 1, IS_NUMBER);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;
    uint64_t* data = (uint64_t*)array->buffer;

    int index = AS_NUMBER(args[1]);
    if(index < 0 || index >= array->length)
    {
        RETURN_ERROR("UInt64Array index %d out of range", index);
    }

    RETURN_NUMBER((double)data[index]);
}

bool native_module_array_uint64_reverse(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(reverse, 1);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;
    uint64_t* data = (uint64_t*)array->buffer;

    b_array* n_array = new_uint64_array(vm, array->length);
    uint64_t* n_data = (uint64_t*)n_array->buffer;

    for(int i = array->length - 1; i >= 0; i--)
    {
        n_data[i] = data[i];
    }

    RETURN_OBJ(new_array(vm, n_array));
}

bool native_module_array_uint64_clone(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(clone, 1);
    ENFORCE_ARG_TYPE(clone, 0, IS_PTR);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;

    b_array* n_array = new_uint64_array(vm, array->length);
    memcpy(n_array->buffer, array->buffer, array->length);

    RETURN_OBJ(new_array(vm, n_array));
}

bool native_module_array_uint64_pop(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(pop, 1);
    ENFORCE_ARG_TYPE(pop, 0, IS_PTR);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;
    uint64_t last = ((uint64_t*)array->buffer)[array->length - 1];
    array->length--;

    RETURN_NUMBER(last);
}

bool native_module_array_uint64_remove(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(remove, 2);
    ENFORCE_ARG_TYPE(remove, 0, IS_PTR);
    ENFORCE_ARG_TYPE(remove, 1, IS_NUMBER);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;
    uint64_t* values = (uint64_t*)array->buffer;

    int index = AS_NUMBER(args[1]);

    if(index < 0 || index >= array->length)
    {
        RETURN_ERROR("UInt64Array index %d out of range", index);
    }

    uint64_t val = values[index];

    for(int i = index; i < array->length; i++)
    {
        values[i] = values[i + 1];
    }
    array->length--;

    RETURN_NUMBER(val);
}

bool native_module_array_uint64_to_list(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(to_list, 1);
    ENFORCE_ARG_TYPE(to_list, 0, IS_PTR);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;
    uint64_t* values = (uint64_t*)array->buffer;

    b_obj_list* list = (b_obj_list*)GC(new_list(vm));

    for(int i = 0; i < array->length; i++)
    {
        write_list(vm, list, NUMBER_VAL((double)values[i]));
    }

    RETURN_OBJ(list);
}

bool native_module_array_uint64_to_bytes(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(to_list, 1);
    ENFORCE_ARG_TYPE(to_list, 0, IS_PTR);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;

    b_obj_bytes* bytes = (b_obj_bytes*)GC(new_bytes(vm, array->length * 8));
    memcpy(bytes->bytes.bytes, array->buffer, array->length * 8);

    RETURN_OBJ(bytes);
}

bool native_module_array_uint64___iter__(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(@iter, 2);
    ENFORCE_ARG_TYPE(@iter, 0, IS_PTR);
    ENFORCE_ARG_TYPE(@iter, 1, IS_NUMBER);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;
    uint64_t* values = (uint64_t*)array->buffer;

    int index = AS_NUMBER(args[1]);

    if(index > -1 && index < array->length)
    {
        RETURN_NUMBER(values[index]);
    }

    RETURN_NIL;
}

//--------- COMMON STARTS -------------------------

bool native_module_array_length(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(length, 1);
    ENFORCE_ARG_TYPE(length, 0, IS_PTR);

    b_obj_ptr* ptr = AS_PTR(args[0]);
    RETURN_NUMBER(((b_array*)ptr->pointer)->length);
}

bool native_module_array_first(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(first, 1);
    ENFORCE_ARG_TYPE(first, 0, IS_PTR);
    RETURN_NUMBER(((double*)((b_array*)AS_PTR(args[0])->pointer)->buffer)[0]);
}

bool native_module_array_last(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(first, 1);
    ENFORCE_ARG_TYPE(first, 0, IS_PTR);
    b_array* array = (b_array*)AS_PTR(args[0])->pointer;
    RETURN_NUMBER(((double*)array->buffer)[array->length - 1]);
}

bool native_module_array_extend(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(extend, 2);
    ENFORCE_ARG_TYPE(extend, 0, IS_PTR);
    ENFORCE_ARG_TYPE(extend, 1, IS_PTR);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;
    b_array* array2 = (b_array*)AS_PTR(args[1])->pointer;

    array->buffer = GROW_ARRAY(void, array->buffer, array->length, array->length + array2->length);

    memcpy(array->buffer + array->length, array2->buffer, array2->length);
    array->length += array2->length;
    RETURN;
}

bool native_module_array_to_string(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(to_string, 1);
    ENFORCE_ARG_TYPE(to_string, 0, IS_PTR);
    b_array* array = (b_array*)AS_PTR(args[0])->pointer;
    RETURN_STRING(array->buffer);
}

bool native_module_array___itern__(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(@itern, 2);
    ENFORCE_ARG_TYPE(@itern, 0, IS_PTR);
    ENFORCE_ARG_TYPE(@itern, 1, IS_NUMBER);

    b_array* array = (b_array*)AS_PTR(args[0])->pointer;

    if(IS_NIL(args[1]))
    {
        if(array->length == 0)
            RETURN_FALSE;
        RETURN_NUMBER(0);
    }

    if(!IS_NUMBER(args[1]))
    {
        RETURN_ERROR("Arrays are numerically indexed");
    }

    int index = AS_NUMBER(args[0]);
    if(index < array->length - 1)
    {
        RETURN_NUMBER((double)index + 1);
    }

    RETURN_NIL;
}

b_module_reg* blade_module_loader_array(b_vm* vm)
{
    static b_func_reg module_functions[] = {
        // int16
        { "Int16Array", false, native_module_array__int16array },
        { "int16_append", false, native_module_array_int16_append },
        { "int16_get", false, native_module_array_int16_get },
        { "int16_reverse", false, native_module_array_int16_reverse },
        { "int16_clone", false, native_module_array_int16_clone },
        { "int16_pop", false, native_module_array_int16_pop },
        { "int16_to_list", false, native_module_array_int16_to_list },
        { "int16_to_bytes", false, native_module_array_int16_to_bytes },
        { "int16___iter__", false, native_module_array_int16___iter__ },

        // int32
        { "Int32Array", false, native_module_array__int32array },
        { "int32_append", false, native_module_array_int32_append },
        { "int32_get", false, native_module_array_int32_get },
        { "int32_reverse", false, native_module_array_int32_reverse },
        { "int32_clone", false, native_module_array_int32_clone },
        { "int32_pop", false, native_module_array_int32_pop },
        { "int32_to_list", false, native_module_array_int32_to_list },
        { "int32_to_bytes", false, native_module_array_int32_to_bytes },
        { "int32___iter__", false, native_module_array_int32___iter__ },

        // int64
        { "Int64Array", false, native_module_array__int64array },
        { "int64_append", false, native_module_array_int64_append },
        { "int64_get", false, native_module_array_int64_get },
        { "int64_reverse", false, native_module_array_int64_reverse },
        { "int64_clone", false, native_module_array_int64_clone },
        { "int64_pop", false, native_module_array_int64_pop },
        { "int64_to_list", false, native_module_array_int64_to_list },
        { "int64_to_bytes", false, native_module_array_int64_to_bytes },
        { "int64___iter__", false, native_module_array_int64___iter__ },

        // uint16
        { "UInt16Array", false, native_module_array__uint16array },
        { "uint16_append", false, native_module_array_uint16_append },
        { "uint16_get", false, native_module_array_uint16_get },
        { "uint16_reverse", false, native_module_array_uint16_reverse },
        { "uint16_clone", false, native_module_array_uint16_clone },
        { "uint16_pop", false, native_module_array_uint16_pop },
        { "uint16_to_list", false, native_module_array_uint16_to_list },
        { "uint16_to_bytes", false, native_module_array_uint16_to_bytes },
        { "uint16___iter__", false, native_module_array_uint16___iter__ },

        // uint32
        { "UInt32Array", false, native_module_array__uint32array },
        { "uint32_append", false, native_module_array_uint32_append },
        { "uint32_get", false, native_module_array_uint32_get },
        { "uint32_reverse", false, native_module_array_uint32_reverse },
        { "uint32_clone", false, native_module_array_uint32_clone },
        { "uint32_pop", false, native_module_array_uint32_pop },
        { "uint32_to_list", false, native_module_array_uint32_to_list },
        { "uint32_to_bytes", false, native_module_array_uint32_to_bytes },
        { "uint32___iter__", false, native_module_array_uint32___iter__ },

        // uint64
        { "UInt64Array", false, native_module_array__uint64array },
        { "uint64_append", false, native_module_array_uint64_append },
        { "uint64_get", false, native_module_array_uint64_get },
        { "uint64_reverse", false, native_module_array_uint64_reverse },
        { "uint64_clone", false, native_module_array_uint64_clone },
        { "uint64_pop", false, native_module_array_uint64_pop },
        { "uint64_to_list", false, native_module_array_uint64_to_list },
        { "uint64_to_bytes", false, native_module_array_uint64_to_bytes },
        { "uint64___iter__", false, native_module_array_uint64___iter__ },

        // common
        { "length", false, native_module_array_length },
        { "first", false, native_module_array_first },
        { "last", false, native_module_array_last },
        { "extend", false, native_module_array_extend },
        { "to_string", false, native_module_array_to_string },
        { "itern", false, native_module_array___itern__ },
        { NULL, false, NULL },
    };

    static b_module_reg module = { .name = "_array", .fields = NULL, .functions = module_functions, .classes = NULL, .preloader = NULL, .unloader = NULL };

    return &module;
}

#define ADD_TIME(n, l, v) dict_add_entry(vm, dict, STRING_L_VAL(n, l), NUMBER_VAL(v))

#define ADD_B_TIME(n, l, v) dict_add_entry(vm, dict, STRING_L_VAL(n, l), BOOL_VAL(v))

#define ADD_S_TIME(n, l, v, g) dict_add_entry(vm, dict, STRING_L_VAL(n, l), STRING_L_VAL(v, g))

#define ADD_G_TIME(n, l, v) dict_add_entry(vm, dict, STRING_L_VAL(n, l), STRING_L_VAL(v, (int)strlen(v)))

bool native_module_date____mktime(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_RANGE(mktime, 1, 8);

    if(arg_count < 7)
    {
        for(int i = 0; i < arg_count; i++)
        {
            ENFORCE_ARG_TYPE(mktime, i, IS_NUMBER);
        }
    }
    else
    {
        for(int i = 0; i < 6; i++)
        {
            ENFORCE_ARG_TYPE(mktime, i, IS_NUMBER);
        }
        ENFORCE_ARG_TYPE(mktime, 6, IS_BOOL);
    }

    int year = -1900, month = 1, day = 1, hour = 0, minute = 0, seconds = 0, is_dst = 0;
    year += AS_NUMBER(args[0]);

    if(arg_count > 1)
        month = AS_NUMBER(args[1]);
    if(arg_count > 2)
        day = AS_NUMBER(args[2]);
    if(arg_count > 3)
        hour = AS_NUMBER(args[3]);
    if(arg_count > 4)
        minute = AS_NUMBER(args[4]);
    if(arg_count > 5)
        seconds = AS_NUMBER(args[5]);
    if(arg_count > 6)
        is_dst = AS_BOOL(args[5]) ? 1 : 0;

    struct tm t;
    t.tm_year = year;
    t.tm_mon = month - 1;
    t.tm_mday = day;
    t.tm_hour = hour;
    t.tm_min = minute;
    t.tm_sec = seconds;
    t.tm_isdst = is_dst;

    RETURN_NUMBER((long)mktime(&t));
}

bool native_module_date__localtime(b_vm* vm, int arg_count, b_value* args)
{
    struct timeval raw_time;
    gettimeofday(&raw_time, NULL);
    struct tm now;
    localtime_r(&raw_time.tv_sec, &now);

    b_obj_dict* dict = (b_obj_dict*)GC(new_dict(vm));

    ADD_TIME("year", 4, (double)now.tm_year + 1900);
    ADD_TIME("month", 5, (double)now.tm_mon + 1);
    ADD_TIME("day", 3, now.tm_mday);
    ADD_TIME("week_day", 8, now.tm_wday);
    ADD_TIME("year_day", 8, now.tm_yday);
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
    ADD_TIME("microseconds", 12, (double)raw_time.tv_usec);

    ADD_B_TIME("is_dst", 6, now.tm_isdst == 1 ? true : false);

#ifndef _WIN32
    // set time zone
    ADD_G_TIME("zone", 4, now.tm_zone);
    // setting gmt offset
    ADD_TIME("gmt_offset", 10, now.tm_gmtoff);
#else
    // set time zone
    ADD_S_TIME("zone", 4, "", 0);
    // setting gmt offset
    ADD_TIME("gmt_offset", 10, 0);
#endif

    RETURN_OBJ(dict);
}

bool native_module_date__gmtime(b_vm* vm, int arg_count, b_value* args)
{
    struct timeval raw_time;
    gettimeofday(&raw_time, NULL);
    struct tm now;
    gmtime_r(&raw_time.tv_sec, &now);

    b_obj_dict* dict = (b_obj_dict*)GC(new_dict(vm));

    ADD_TIME("year", 4, (double)now.tm_year + 1900);
    ADD_TIME("month", 5, (double)now.tm_mon + 1);
    ADD_TIME("day", 3, now.tm_mday);
    ADD_TIME("week_day", 8, now.tm_wday);
    ADD_TIME("year_day", 8, now.tm_yday);
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
    ADD_TIME("microseconds", 12, (double)raw_time.tv_usec);

    ADD_B_TIME("is_dst", 6, now.tm_isdst == 1 ? true : false);

#ifndef _WIN32
    // set time zone
    ADD_G_TIME("zone", 4, now.tm_zone);
    // setting gmt offset
    ADD_TIME("gmt_offset", 10, now.tm_gmtoff);
#else
    // set time zone
    ADD_S_TIME("zone", 4, "", 0);
    // setting gmt offset
    ADD_TIME("gmt_offset", 10, 0);
#endif

    RETURN_OBJ(dict);
}

b_module_reg* blade_module_loader_date(b_vm* vm)
{
    static b_func_reg module_functions[] = {
        { "localtime", true, native_module_date__localtime },
        { "gmtime", true, native_module_date__gmtime },
        { "mktime", false, native_module_date____mktime },
        { NULL, false, NULL },
    };

    static b_module_reg module = { .name = "_date", .fields = NULL, .functions = module_functions, .classes = NULL, .preloader = NULL, .unloader = NULL };
    return &module;
}

#undef ADD_TIME
#undef ADD_B_TIME
#undef ADD_S_TIME

static struct termios orig_termios;
static bool set_attr_was_called = false;

void disable_raw_mode(void)
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

static struct termios n_term;
static struct termios o_term;

static int cbreak(int fd)
{
    if((tcgetattr(fd, &o_term)) == -1)
        return -1;
    n_term = o_term;
    n_term.c_lflag = n_term.c_lflag & ~(ECHO | ICANON);
    n_term.c_cc[VMIN] = 1;
    n_term.c_cc[VTIME] = 0;
    if((tcsetattr(fd, TCSAFLUSH, &n_term)) == -1)
        return -1;
    return 1;
}

int getch()
{
    int cinput;

    if(cbreak(STDIN_FILENO) == -1)
    {
        fprintf(stderr, "cbreak failure, exiting \n");
        exit(EXIT_FAILURE);
    }
    cinput = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &o_term);

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
            line[nch] = *utf8_encode(c);
            nch = nch + 1;
        }
        else
        {
            break;
        }
    }

    if(c == EOF && nch == 0)
        return EOF;

    line[nch] = '\0';

    return nch;
}

/**
 * tty._tcgetattr()
 *
 * returns the configuration of the current tty input
 */
bool native_module_io_tty__tcgetattr(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(_tcgetattr, 1);
    ENFORCE_ARG_TYPE(_tcsetattr, 0, IS_FILE);

#ifdef HAVE_TERMIOS_H
    b_obj_file* file = AS_FILE(args[0]);

    if(!is_std_file(file))
    {
        RETURN_ERROR("can only use tty on std objects");
    }

    struct termios raw_attr;
    if(tcgetattr(fileno(file->file), &raw_attr) != 0)
    {
        RETURN_ERROR(strerror(errno));
    }

    // we have our attributes already
    b_obj_dict* dict = (b_obj_dict*)GC(new_dict(vm));
    dict_add_entry(vm, dict, NUMBER_VAL(0), NUMBER_VAL(raw_attr.c_iflag));
    dict_add_entry(vm, dict, NUMBER_VAL(1), NUMBER_VAL(raw_attr.c_oflag));
    dict_add_entry(vm, dict, NUMBER_VAL(2), NUMBER_VAL(raw_attr.c_cflag));
    dict_add_entry(vm, dict, NUMBER_VAL(3), NUMBER_VAL(raw_attr.c_lflag));
    dict_add_entry(vm, dict, NUMBER_VAL(4), NUMBER_VAL(raw_attr.c_ispeed));
    dict_add_entry(vm, dict, NUMBER_VAL(5), NUMBER_VAL(raw_attr.c_ospeed));

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
bool native_module_io_tty__tcsetattr(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(_tcsetattr, 3);
    ENFORCE_ARG_TYPE(_tcsetattr, 0, IS_FILE);
    ENFORCE_ARG_TYPE(_tcsetattr, 1, IS_NUMBER);
    ENFORCE_ARG_TYPE(_tcsetattr, 2, IS_DICT);

#ifdef HAVE_TERMIOS_H
    b_obj_file* file = AS_FILE(args[0]);
    int type = AS_NUMBER(args[1]);
    b_obj_dict* dict = AS_DICT(args[2]);

    if(!is_std_file(file))
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
        if(!IS_NUMBER(dict->names.values[i]) || AS_NUMBER(dict->names.values[i]) < 0 ||// c_iflag
           AS_NUMBER(dict->names.values[i]) > 5)
        {// ospeed
            RETURN_ERROR("attributes must be one of io TTY flags");
        }
        b_value dummy_value;
        if(dict_get_entry(dict, dict->names.values[i], &dummy_value))
        {
            if(!IS_NUMBER(dummy_value))
            {
                RETURN_ERROR("TTY attribute cannot be %s", value_type(dummy_value));
            }
        }
    }

    b_value iflag = NIL_VAL, oflag = NIL_VAL, cflag = NIL_VAL, lflag = NIL_VAL, ispeed = NIL_VAL, ospeed = NIL_VAL;

    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);

    struct termios raw = orig_termios;

    if(dict_get_entry(dict, NUMBER_VAL(0), &iflag))
    {
        raw.c_iflag = (long)AS_NUMBER(iflag);
    }
    if(dict_get_entry(dict, NUMBER_VAL(1), &iflag))
    {
        raw.c_oflag = (long)AS_NUMBER(oflag);
    }
    if(dict_get_entry(dict, NUMBER_VAL(2), &iflag))
    {
        raw.c_cflag = (long)AS_NUMBER(cflag);
    }
    if(dict_get_entry(dict, NUMBER_VAL(3), &iflag))
    {
        raw.c_lflag = (long)AS_NUMBER(lflag);
    }
    if(dict_get_entry(dict, NUMBER_VAL(4), &iflag))
    {
        raw.c_ispeed = (long)AS_NUMBER(ispeed);
    }
    if(dict_get_entry(dict, NUMBER_VAL(5), &iflag))
    {
        raw.c_ospeed = (long)AS_NUMBER(ospeed);
    }

    set_attr_was_called = true;

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
bool native_module_io_tty__exit_raw(b_vm* vm, int arg_count, b_value* args)
{
#ifdef HAVE_TERMIOS_H
    ENFORCE_ARG_COUNT(TTY.exit_raw, 0);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    RETURN;
#else
    RETURN_ERROR("exit_raw() is not supported on this platform");
#endif /* HAVE_TERMIOS_H */
}

/**
 * TTY.flush()
 * flushes the standard output and standard error interface
 * @return nil
 */
bool native_module_io_tty__flush(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(TTY.flush, 0);
    fflush(stdout);
    fflush(stderr);
    RETURN;
}

/**
 * flush()
 * flushes the given file handle
 * @return nil
 */
bool native_module_io_flush(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(flush, 1);
    ENFORCE_ARG_TYPE(flush, 0, IS_FILE);
    b_obj_file* file = AS_FILE(args[0]);

    if(file->is_open)
    {
        fflush(file->file);
    }
    RETURN;
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
bool native_module_io_getc(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_RANGE(getc, 0, 1);

    int length = 1;
    if(arg_count == 1)
    {
        ENFORCE_ARG_TYPE(getc, 0, IS_NUMBER);
        length = AS_NUMBER(args[0]);
    }

    char* result = ALLOCATE(char, (size_t)length + 2);
    read_line(result, length + 1);
    RETURN_L_STRING(result, length);
}

/**
 * getch()
 *
 * reads character(s) from standard input without printing to standard output
 *
 * when length is given, gets `length` number of characters
 * else, gets a single character
 * @returns char
 */
bool native_module_io_getch(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(getch, 0);
    char* result = ALLOCATE(char, 2);
    result[0] = (char)getch();
    result[1] = '\0';
    RETURN_L_STRING(result, 1);
}

/**
 * putc(c: char)
 * writes character c to the screen
 * @return nil
 */
bool native_module_io_putc(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(putc, 1);
    ENFORCE_ARG_TYPE(putc, 0, IS_STRING);

    b_obj_string* string = AS_STRING(args[0]);

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
    RETURN;
}

/**
 * stdin()
 *
 * returns the standard input
 */
b_value io_module_stdin(b_vm* vm)
{
    b_obj_file* file;
    b_obj_string* mode;
    b_obj_string* name;
    name = copy_string(vm, "<stdin>", 7);
    mode = copy_string(vm, "rb", 2);
    file = new_file(vm, name, mode);
    file->file = stdin;
    file->is_open = true;
    //file->mode = mode;
    return OBJ_VAL(file);
}

/**
 * stdout()
 *
 * returns the standard output interface
 */
b_value io_module_stdout(b_vm* vm)
{
    b_obj_file* file;
    b_obj_string* mode;
    b_obj_string* name;
    name = copy_string(vm, "<stdout>", 8);
    mode = copy_string(vm, "wb", 2);
    file = new_file(vm, name, mode);
    file->file = stdout;
    file->is_open = true;
    //file->mode = copy_string(vm, "", 0);
    return OBJ_VAL(file);
}

/**
 * stderr()
 *
 * returns the standard error interface
 */
b_value io_module_stderr(b_vm* vm)
{
    b_obj_file* file;
    b_obj_string* mode;
    b_obj_string* name;
    name = copy_string(vm, "<stderr>", 8);
    mode = copy_string(vm, "wb", 2);
    file = new_file(vm, name, mode);
    file->file = stderr;
    file->is_open = true;
    //file->mode = copy_string(vm, "", 0);
    return OBJ_VAL(file);
}

void __io_module_unload(b_vm* vm)
{
#ifdef HAVE_TERMIOS_H
    if(set_attr_was_called)
    {
        disable_raw_mode();
    }
#endif /* ifdef HAVE_TERMIOS_H */
}

b_module_reg* blade_module_loader_io(b_vm* vm)
{
    static b_field_reg io_module_fields[] = {
        { "stdin", false, io_module_stdin },
        { "stdout", false, io_module_stdout },
        { "stderr", false, io_module_stderr },
        { NULL, false, NULL },
    };

    static b_func_reg io_functions[] = {
        { "getc", false, native_module_io_getc },
        { "getch", false, native_module_io_getch },
        { "putc", false, native_module_io_putc },
        { "flush", false, native_module_io_flush },
        { NULL, false, NULL },
    };

    static b_func_reg tty_class_functions[] = {
        { "tcgetattr", false, native_module_io_tty__tcgetattr },
        { "tcsetattr", false, native_module_io_tty__tcsetattr },
        { "flush", false, native_module_io_tty__flush },
        { "exit_raw", false, native_module_io_tty__exit_raw },
        { NULL, false, NULL },
    };

    static b_class_reg classes[] = {
        { "TTY", NULL, tty_class_functions },
        { NULL, NULL, NULL },
    };

    static b_module_reg module
    = { .name = "_io", .fields = io_module_fields, .functions = io_functions, .classes = classes, .preloader = NULL, .unloader = &__io_module_unload };

    return &module;
}

bool native_module_math__sin(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(sin, 1);
    ENFORCE_ARG_TYPE(sin, 0, IS_NUMBER);
    RETURN_NUMBER(sin(AS_NUMBER(args[0])));
}

bool native_module_math__cos(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(cos, 1);
    ENFORCE_ARG_TYPE(cos, 0, IS_NUMBER);
    RETURN_NUMBER(cos(AS_NUMBER(args[0])));
}

bool native_module_math__tan(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(tan, 1);
    ENFORCE_ARG_TYPE(tan, 0, IS_NUMBER);
    RETURN_NUMBER(tan(AS_NUMBER(args[0])));
}

bool native_module_math__sinh(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(sinh, 1);
    ENFORCE_ARG_TYPE(sinh, 0, IS_NUMBER);
    RETURN_NUMBER(sinh(AS_NUMBER(args[0])));
}

bool native_module_math__cosh(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(cosh, 1);
    ENFORCE_ARG_TYPE(cosh, 0, IS_NUMBER);
    RETURN_NUMBER(cosh(AS_NUMBER(args[0])));
}

bool native_module_math__tanh(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(tanh, 1);
    ENFORCE_ARG_TYPE(tanh, 0, IS_NUMBER);
    RETURN_NUMBER(tanh(AS_NUMBER(args[0])));
}

bool native_module_math__asin(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(asin, 1);
    ENFORCE_ARG_TYPE(asin, 0, IS_NUMBER);
    RETURN_NUMBER(asin(AS_NUMBER(args[0])));
}

bool native_module_math__acos(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(acos, 1);
    ENFORCE_ARG_TYPE(acos, 0, IS_NUMBER);
    RETURN_NUMBER(acos(AS_NUMBER(args[0])));
}

bool native_module_math__atan(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(atan, 1);
    ENFORCE_ARG_TYPE(atan, 0, IS_NUMBER);
    RETURN_NUMBER(atan(AS_NUMBER(args[0])));
}

bool native_module_math__atan2(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(atan2, 2);
    ENFORCE_ARG_TYPE(atan2, 0, IS_NUMBER);
    ENFORCE_ARG_TYPE(atan2, 1, IS_NUMBER);
    RETURN_NUMBER(atan2(AS_NUMBER(args[0]), AS_NUMBER(args[1])));
}

bool native_module_math__asinh(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(asinh, 1);
    ENFORCE_ARG_TYPE(asinh, 0, IS_NUMBER);
    RETURN_NUMBER(asinh(AS_NUMBER(args[0])));
}

bool native_module_math__acosh(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(acosh, 1);
    ENFORCE_ARG_TYPE(acosh, 0, IS_NUMBER);
    RETURN_NUMBER(acosh(AS_NUMBER(args[0])));
}

bool native_module_math__atanh(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(atanh, 1);
    ENFORCE_ARG_TYPE(atanh, 0, IS_NUMBER);
    RETURN_NUMBER(atanh(AS_NUMBER(args[0])));
}

bool native_module_math__exp(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(exp, 1);
    ENFORCE_ARG_TYPE(exp, 0, IS_NUMBER);
    RETURN_NUMBER(exp(AS_NUMBER(args[0])));
}

bool native_module_math__expm1(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(expm1, 1);
    ENFORCE_ARG_TYPE(expm1, 0, IS_NUMBER);
    RETURN_NUMBER(expm1(AS_NUMBER(args[0])));
}

bool native_module_math__ceil(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(ceil, 1);
    ENFORCE_ARG_TYPE(ceil, 0, IS_NUMBER);
    RETURN_NUMBER(ceil(AS_NUMBER(args[0])));
}

bool native_module_math__round(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(round, 1);
    ENFORCE_ARG_TYPE(round, 0, IS_NUMBER);
    RETURN_NUMBER(round(AS_NUMBER(args[0])));
}

bool native_module_math__log(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(log, 1);
    ENFORCE_ARG_TYPE(log, 0, IS_NUMBER);
    RETURN_NUMBER(log(AS_NUMBER(args[0])));
}

bool native_module_math__log10(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(log10, 1);
    ENFORCE_ARG_TYPE(log10, 0, IS_NUMBER);
    RETURN_NUMBER(log10(AS_NUMBER(args[0])));
}

bool native_module_math__log2(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(log2, 1);
    ENFORCE_ARG_TYPE(log2, 0, IS_NUMBER);
    RETURN_NUMBER(log2(AS_NUMBER(args[0])));
}

bool native_module_math__log1p(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(log1p, 1);
    ENFORCE_ARG_TYPE(log1p, 0, IS_NUMBER);
    RETURN_NUMBER(log1p(AS_NUMBER(args[0])));
}

bool native_module_math__floor(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(floor, 1);
    if(IS_NIL(args[0]))
    {
        RETURN_NUMBER(0);
    }
    ENFORCE_ARG_TYPE(floor, 0, IS_NUMBER);
    RETURN_NUMBER(floor(AS_NUMBER(args[0])));
}

b_module_reg* blade_module_loader_math(b_vm* vm)
{
    static b_func_reg module_functions[] = {
        { "sin", true, native_module_math__sin },
        { "cos", true, native_module_math__cos },
        { "tan", true, native_module_math__tan },
        { "sinh", true, native_module_math__sinh },
        { "cosh", true, native_module_math__cosh },
        { "tanh", true, native_module_math__tanh },
        { "asin", true, native_module_math__asin },
        { "acos", true, native_module_math__acos },
        { "atan", true, native_module_math__atan },
        { "atan2", true, native_module_math__atan2 },
        { "asinh", true, native_module_math__asinh },
        { "acosh", true, native_module_math__acosh },
        { "atanh", true, native_module_math__atanh },
        { "exp", true, native_module_math__exp },
        { "expm1", true, native_module_math__expm1 },
        { "ceil", true, native_module_math__ceil },
        { "round", true, native_module_math__round },
        { "log", true, native_module_math__log },
        { "log2", true, native_module_math__log2 },
        { "log10", true, native_module_math__log10 },
        { "log1p", true, native_module_math__log1p },
        { "floor", true, native_module_math__floor },
        { NULL, false, NULL },
    };

    static b_module_reg module = { .name = "_math", .fields = NULL, .functions = module_functions, .classes = NULL, .preloader = NULL, .unloader = NULL };

    return &module;
}


bool native_module_os_exec(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(exec, 1);
    ENFORCE_ARG_TYPE(exec, 0, IS_STRING);
    b_obj_string* string = AS_STRING(args[0]);
    if(string->length == 0)
    {
        RETURN_NIL;
    }

    fflush(stdout);
    FILE* fd = popen(string->chars, "r");
    if(!fd)
        RETURN_NIL;

    char buffer[256];
    size_t n_read;
    size_t output_size = 256;
    int length = 0;
    char* output = ALLOCATE(char, output_size);

    if(output != NULL)
    {
        while((n_read = fread(buffer, 1, sizeof(buffer), fd)) != 0)
        {
            if(length + n_read >= output_size)
            {
                size_t old = output_size;
                output_size *= 2;
                char* temp = GROW_ARRAY(char, output, old, output_size);
                if(temp == NULL)
                {
                    RETURN_ERROR("device out of memory");
                }
                else
                {
                    vm->bytes_allocated += output_size / 2;
                    output = temp;
                }
            }
            if((output + length) != NULL)
            {
                strncat(output + length, buffer, n_read);
            }
            length += (int)n_read;
        }

        if(length == 0)
        {
            pclose(fd);
            RETURN_NIL;
        }

        output[length - 1] = '\0';

        pclose(fd);
        RETURN_T_STRING(output, length);
    }

    pclose(fd);
    RETURN_STRING("");
}

bool native_module_os_info(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(info, 0);

#ifdef HAVE_SYS_UTSNAME_H
    struct utsname os;
    if(uname(&os) != 0)
    {
        RETURN_ERROR("could not access os information");
    }

    b_obj_dict* dict = (b_obj_dict*)GC(new_dict(vm));

    dict_add_entry(vm, dict, GC_L_STRING("sysname", 7), GC_STRING(os.sysname));
    dict_add_entry(vm, dict, GC_L_STRING("nodename", 8), GC_STRING(os.nodename));
    dict_add_entry(vm, dict, GC_L_STRING("version", 7), GC_STRING(os.version));
    dict_add_entry(vm, dict, GC_L_STRING("release", 7), GC_STRING(os.release));
    dict_add_entry(vm, dict, GC_L_STRING("machine", 7), GC_STRING(os.machine));

    RETURN_OBJ(dict);
#else
    RETURN_ERROR("not available: OS does not have uname()")
#endif /* HAVE_SYS_UTSNAME_H */
}

bool native_module_os_sleep(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(sleep, 1);
    ENFORCE_ARG_TYPE(sleep, 0, IS_NUMBER);
    sleep((int)AS_NUMBER(args[0]));
    RETURN;
}

b_value get_os_platform(b_vm* vm)
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

    return OBJ_VAL(copy_string(vm, PLATFORM_NAME, (int)strlen(PLATFORM_NAME)));

#undef PLATFORM_NAME
}

b_value get_blade_os_args(b_vm* vm)
{
    b_obj_list* list = (b_obj_list*)GC(new_list(vm));
    if(vm->std_args != NULL)
    {
        for(int i = 0; i < vm->std_args_count; i++)
        {
            write_list(vm, list, GC_STRING(vm->std_args[i]));
        }
    }
    CLEAR_GC();
    return OBJ_VAL(list);
}

b_value get_blade_os_path_separator(b_vm* vm)
{
    return STRING_L_VAL(BLADE_PATH_SEPARATOR, 1);
}

bool native_module_os_getenv(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(get_env, 1);
    ENFORCE_ARG_TYPE(get_env, 0, IS_STRING);

    char* env = getenv(AS_C_STRING(args[0]));
    if(env != NULL)
    {
        RETURN_STRING(env);
    }
    else
    {
        RETURN_NIL;
    }
}

bool native_module_os_setenv(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_RANGE(set_env, 2, 3);
    ENFORCE_ARG_TYPE(set_env, 0, IS_STRING);
    ENFORCE_ARG_TYPE(set_env, 1, IS_STRING);

    int overwrite = 1;
    if(arg_count == 3)
    {
        ENFORCE_ARG_TYPE(setenv, 2, IS_BOOL);
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

bool native_module_os__createdir(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(create_dir, 3);
    ENFORCE_ARG_TYPE(create_dir, 0, IS_STRING);
    ENFORCE_ARG_TYPE(create_dir, 1, IS_NUMBER);
    ENFORCE_ARG_TYPE(create_dir, 2, IS_BOOL);

    b_obj_string* path = AS_STRING(args[0]);
    int mode = AS_NUMBER(args[1]);
    bool is_recursive = AS_BOOL(args[2]);

    char sep = BLADE_PATH_SEPARATOR[0];
    bool exists = false;

    if(is_recursive)
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

bool native_module_os__readdir(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(read_dir, 1);
    ENFORCE_ARG_TYPE(read_dir, 0, IS_STRING);
    b_obj_string* path = AS_STRING(args[0]);

    DIR* dir;
    if((dir = opendir(path->chars)) != NULL)
    {
        b_obj_list* list = (b_obj_list*)GC(new_list(vm));
        struct dirent* ent;
        while((ent = readdir(dir)) != NULL)
        {
            write_list(vm, list, GC_STRING(ent->d_name));
        }
        closedir(dir);
        RETURN_OBJ(list);
    }
    RETURN_ERROR(strerror(errno));
}

static int remove_directory(char* path, int path_length, bool recursive)
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

            int path_string_length = path_length + (int)strlen(ent->d_name) + 2;
            char* path_string = merge_paths(path, ent->d_name);
            if(path_string == NULL)
                return -1;

            struct stat sb;
            if(stat(path_string, &sb) == 0)
            {
                if(S_ISDIR(sb.st_mode) > 0 && recursive)
                {
                    // recurse
                    if(remove_directory(path_string, path_string_length, recursive) == -1)
                    {
                        free(path_string);
                        return -1;
                    }
                }
                else if(unlink(path_string) == -1)
                {
                    free(path_string);
                    return -1;
                }
                else
                {
                    free(path_string);
                }
            }
            else
            {
                free(path_string);
                return -1;
            }
        }
        closedir(dir);
        return rmdir(path);
    }
    return -1;
}

bool native_module_os__removedir(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(remove_dir, 2);
    ENFORCE_ARG_TYPE(remove_dir, 0, IS_STRING);
    ENFORCE_ARG_TYPE(remove_dir, 1, IS_BOOL);

    b_obj_string* path = AS_STRING(args[0]);
    bool recursive = AS_BOOL(args[1]);
    if(remove_directory(path->chars, path->length, recursive) >= 0)
    {
        RETURN_TRUE;
    }
    RETURN_ERROR(strerror(errno));
}

bool native_module_os__chmod(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(chmod, 2);
    ENFORCE_ARG_TYPE(chmod, 0, IS_STRING);
    ENFORCE_ARG_TYPE(chmod, 1, IS_NUMBER);

    b_obj_string* path = AS_STRING(args[0]);
    int mode = AS_NUMBER(args[1]);
    if(chmod(path->chars, mode) != 0)
    {
        RETURN_ERROR(strerror(errno));
    }
    RETURN_TRUE;
}

bool native_module_os__is_dir(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(is_dir, 1);
    ENFORCE_ARG_TYPE(is_dir, 0, IS_STRING);
    b_obj_string* path = AS_STRING(args[0]);
    struct stat sb;
    if(stat(path->chars, &sb) == 0)
    {
        RETURN_BOOL(S_ISDIR(sb.st_mode) > 0);
    }
    RETURN_FALSE;
}

bool native_module_os__exit(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(exit, 1);
    ENFORCE_ARG_TYPE(exit, 0, IS_NUMBER);
    exit((int)AS_NUMBER(args[0]));
    RETURN;
}

bool native_module_os__cwd(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(cwd, 0);
    char* cwd = getcwd(NULL, 0);
    if(cwd != NULL)
    {
        RETURN_TT_STRING(cwd);
    }
    RETURN_L_STRING("", 1);
}

bool native_module_os__realpath(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(_realpath, 1);
    ENFORCE_ARG_TYPE(_realpath, 0, IS_STRING);
    char* path = realpath(AS_C_STRING(args[0]), NULL);
    if(path != NULL)
    {
        RETURN_TT_STRING(path);
    }
    RETURN_VALUE(args[0]);
}

bool native_module_os__chdir(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(chdir, 1);
    ENFORCE_ARG_TYPE(chdir, 0, IS_STRING);
    RETURN_BOOL(chdir(AS_STRING(args[0])->chars) == 0);
}

bool native_module_os__exists(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(exists, 1);
    ENFORCE_ARG_TYPE(exists, 0, IS_STRING);
    struct stat sb;
    if(stat(AS_STRING(args[0])->chars, &sb) == 0 && sb.st_mode & S_IFDIR)
    {
        RETURN_TRUE;
    }
    RETURN_FALSE;
}

bool native_module_os__dirname(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(dirname, 1);
    ENFORCE_ARG_TYPE(dirname, 0, IS_STRING);
    char* dir = dirname(AS_STRING(args[0])->chars);
    if(!dir)
    {
        RETURN_VALUE(args[0]);
    }
    RETURN_STRING(dir);
}

bool native_module_os__basename(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(basename, 1);
    ENFORCE_ARG_TYPE(basename, 0, IS_STRING);
    char* dir = basename(AS_STRING(args[0])->chars);
    if(!dir)
    {
        RETURN_VALUE(args[0]);
    }
    RETURN_STRING(dir);
}

/** DIR TYPES BEGIN */

b_value __os_dir_DT_UNKNOWN(b_vm* vm)
{
    return NUMBER_VAL(DT_UNKNOWN);
}

b_value __os_dir_DT_REG(b_vm* vm)
{
    return NUMBER_VAL(DT_REG);
}

b_value __os_dir_DT_DIR(b_vm* vm)
{
    return NUMBER_VAL(DT_DIR);
}

b_value __os_dir_DT_FIFO(b_vm* vm)
{
    return NUMBER_VAL(DT_FIFO);
}

b_value __os_dir_DT_SOCK(b_vm* vm)
{
    return NUMBER_VAL(DT_SOCK);
}

b_value __os_dir_DT_CHR(b_vm* vm)
{
    return NUMBER_VAL(DT_CHR);
}

b_value __os_dir_DT_BLK(b_vm* vm)
{
    return NUMBER_VAL(DT_BLK);
}

b_value __os_dir_DT_LNK(b_vm* vm)
{
    return NUMBER_VAL(DT_LNK);
}

b_value __os_dir_DT_WHT(b_vm* vm)
{
    return NUMBER_VAL(-1);
}

void __os_module_preloader(b_vm* vm)
{
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

b_module_reg* blade_module_loader_os(b_vm* vm)
{
    static b_func_reg os_module_functions[] = {
        { "info", true, native_module_os_info },          { "exec", true, native_module_os_exec },
        { "sleep", true, native_module_os_sleep },        { "getenv", true, native_module_os_getenv },
        { "setenv", true, native_module_os_setenv },      { "createdir", true, native_module_os__createdir },
        { "readdir", true, native_module_os__readdir },   { "chmod", true, native_module_os__chmod },
        { "isdir", true, native_module_os__is_dir },      { "exit", true, native_module_os__exit },
        { "cwd", true, native_module_os__cwd },           { "removedir", true, native_module_os__removedir },
        { "chdir", true, native_module_os__chdir },       { "exists", true, native_module_os__exists },
        { "realpath", true, native_module_os__realpath }, { "dirname", true, native_module_os__dirname },
        { "basename", true, native_module_os__basename }, { NULL, false, NULL },
    };

    static b_field_reg os_module_fields[] = {
        { "platform", true, get_os_platform },
        { "args", true, get_blade_os_args },
        { "path_separator", true, get_blade_os_path_separator },
        { "DT_UNKNOWN", true, __os_dir_DT_UNKNOWN },
        { "DT_BLK", true, __os_dir_DT_BLK },
        { "DT_CHR", true, __os_dir_DT_CHR },
        { "DT_DIR", true, __os_dir_DT_DIR },
        { "DT_FIFO", true, __os_dir_DT_FIFO },
        { "DT_LNK", true, __os_dir_DT_LNK },
        { "DT_REG", true, __os_dir_DT_REG },
        { "DT_SOCK", true, __os_dir_DT_SOCK },
        { "DT_WHT", true, __os_dir_DT_WHT },
        { NULL, false, NULL },
    };

    static b_module_reg module
    = { .name = "_os", .fields = os_module_fields, .functions = os_module_functions, .classes = NULL, .preloader = &__os_module_preloader, .unloader = NULL };

    return &module;
}



b_value __process_cpu_count(b_vm* vm)
{
    return NUMBER_VAL(1);
}

void b__free_shared_memory(void* data)
{
    BProcessShared* shared = (BProcessShared*)data;
    munmap(shared->format, shared->format_length * sizeof(char));
    munmap(shared->get_format, shared->get_format_length * sizeof(char));
    munmap(shared->bytes, shared->length * sizeof(unsigned char));
    munmap(shared, sizeof(BProcessShared));
}

bool native_module_process_Process(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_RANGE(Process, 0, 1);
    BProcess* process = ALLOCATE(BProcess, 1);
    b_obj_ptr* ptr = (b_obj_ptr*)GC(new_ptr(vm, process));
    ptr->name = "<*Process::Process>";
    process->pid = -1;
    RETURN_OBJ(ptr);
}

bool native_module_process_create(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(create, 1);
    ENFORCE_ARG_TYPE(create, 0, IS_PTR);
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

bool native_module_process_is_alive(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(create, 1);
    ENFORCE_ARG_TYPE(create, 0, IS_PTR);
    BProcess* process = (BProcess*)AS_PTR(args[0])->pointer;
    RETURN_BOOL(waitpid(process->pid, NULL, WNOHANG) == 0);
}

bool native_module_process_kill(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(kill, 1);
    ENFORCE_ARG_TYPE(kill, 0, IS_PTR);
    BProcess* process = (BProcess*)AS_PTR(args[0])->pointer;
    RETURN_BOOL(kill(process->pid, SIGKILL) == 0);
}

bool native_module_process_wait(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(create, 1);
    ENFORCE_ARG_TYPE(create, 0, IS_PTR);
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
                continue;
            break;
        }
    } while(p != process->pid);

    if(p == process->pid)
    {
        RETURN_NUMBER(status);
    }
    RETURN_NUMBER(-1);
}

bool native_module_process_id(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(create, 1);
    ENFORCE_ARG_TYPE(create, 0, IS_PTR);
    BProcess* process = (BProcess*)AS_PTR(args[0])->pointer;
    RETURN_NUMBER(process->pid);
}

bool native_module_process_new_shared(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(new_shared, 0);
    BProcessShared* shared = mmap(NULL, sizeof(BProcessShared), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    shared->bytes = mmap(NULL, sizeof(unsigned char), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    shared->format = mmap(NULL, sizeof(char), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    shared->get_format = mmap(NULL, sizeof(char), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    shared->length = shared->get_format_length = shared->format_length = 0;
    b_obj_ptr* ptr = (b_obj_ptr*)GC(new_ptr(vm, shared));
    ptr->name = "<*Process::SharedValue>";
    ptr->free_fn = b__free_shared_memory;
    RETURN_OBJ(ptr);
}

bool native_module_process_shared_write(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(shared_write, 4);
    ENFORCE_ARG_TYPE(shared_write, 0, IS_PTR);
    ENFORCE_ARG_TYPE(shared_write, 1, IS_STRING);
    ENFORCE_ARG_TYPE(shared_write, 2, IS_STRING);
    ENFORCE_ARG_TYPE(shared_write, 3, IS_BYTES);

    BProcessShared* shared = (BProcessShared*)AS_PTR(args[0])->pointer;
    if(!shared->locked)
    {
        b_obj_string* format = AS_STRING(args[1]);
        b_obj_string* get_format = AS_STRING(args[2]);
        b_byte_arr bytes = AS_BYTES(args[3])->bytes;

        memcpy(shared->format, format->chars, format->length);
        shared->format_length = format->length;

        memcpy(shared->get_format, get_format->chars, get_format->length);
        shared->get_format_length = get_format->length;

        memcpy(shared->bytes, bytes.bytes, bytes.count);
        shared->length = bytes.count;

        // return length written
        RETURN_NUMBER(shared->length);
    }

    RETURN_FALSE;
}

bool native_module_process_shared_read(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(shared_read, 1);
    ENFORCE_ARG_TYPE(shared_read, 0, IS_PTR);
    BProcessShared* shared = (BProcessShared*)AS_PTR(args[0])->pointer;

    if(shared->length > 0 || shared->format_length > 0)
    {
        b_obj_bytes* bytes = (b_obj_bytes*)GC(copy_bytes(vm, shared->bytes, shared->length));

        // return [format, bytes]
        b_obj_list* list = (b_obj_list*)GC(new_list(vm));
        write_list(vm, list, GC_L_STRING(shared->get_format, shared->get_format_length));
        write_list(vm, list, OBJ_VAL(bytes));

        RETURN_OBJ(list);
    }
    RETURN_NIL;
}

bool native_module_process_shared_lock(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(shared_lock, 1);
    ENFORCE_ARG_TYPE(shared_lock, 0, IS_PTR);
    BProcessShared* shared = (BProcessShared*)AS_PTR(args[0])->pointer;
    shared->locked = true;
    RETURN;
}

bool native_module_process_shared_unlock(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(shared_unlock, 1);
    ENFORCE_ARG_TYPE(shared_unlock, 0, IS_PTR);
    BProcessShared* shared = (BProcessShared*)AS_PTR(args[0])->pointer;
    shared->locked = false;
    RETURN;
}

bool native_module_process_shared_islocked(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(shared_islocked, 1);
    ENFORCE_ARG_TYPE(shared_islocked, 0, IS_PTR);
    BProcessShared* shared = (BProcessShared*)AS_PTR(args[0])->pointer;
    RETURN_BOOL(shared->locked);
}

b_module_reg* blade_module_loader_process(b_vm* vm)
{
    static b_func_reg os_module_functions[] = {
        { "Process", false, native_module_process_Process },
        { "create", false, native_module_process_create },
        { "is_alive", false, native_module_process_is_alive },
        { "wait", false, native_module_process_wait },
        { "id", false, native_module_process_id },
        { "kill", false, native_module_process_kill },
        { "new_shared", false, native_module_process_new_shared },
        { "shared_write", false, native_module_process_shared_write },
        { "shared_read", false, native_module_process_shared_read },
        { "shared_lock", false, native_module_process_shared_lock },
        { "shared_unlock", false, native_module_process_shared_unlock },
        { NULL, false, NULL },
    };

    static b_field_reg os_module_fields[] = {
        { "cpu_count", true, __process_cpu_count },
        { NULL, false, NULL },
    };

    static b_module_reg module
    = { .name = "_process", .fields = os_module_fields, .functions = os_module_functions, .classes = NULL, .preloader = NULL, .unloader = NULL };

    return &module;
}

extern bool call_value(b_vm* vm, b_value callee, int arg_count);

/**
 * hasprop(object: instance, name: string)
 *
 * returns true if object has the property name or false if not
 */
bool native_module_reflect__hasprop(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(has_prop, 2);
    ENFORCE_ARG_TYPE(has_prop, 0, IS_INSTANCE);
    ENFORCE_ARG_TYPE(has_prop, 1, IS_STRING);

    b_obj_instance* instance = AS_INSTANCE(args[0]);
    b_value dummy;
    RETURN_BOOL(table_get(&instance->properties, args[1], &dummy));
}

/**
 * getprop(object: instance, name: string)
 *
 * returns the property of the object matching the given name
 * or nil if the object contains no property with a matching
 * name
 */
bool native_module_reflect__getprop(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(get_prop, 2);
    ENFORCE_ARG_TYPE(get_prop, 0, IS_INSTANCE);
    ENFORCE_ARG_TYPE(get_prop, 1, IS_STRING);

    b_obj_instance* instance = AS_INSTANCE(args[0]);
    b_value value;
    if(table_get(&instance->properties, args[1], &value))
    {
        RETURN_VALUE(value);
    }
    RETURN_NIL;
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
bool native_module_reflect__setprop(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(set_prop, 3);
    ENFORCE_ARG_TYPE(set_prop, 0, IS_INSTANCE);
    ENFORCE_ARG_TYPE(set_prop, 1, IS_STRING);
    ENFORCE_ARG_TYPE(set_prop, 2, IS_STRING);

    b_obj_instance* instance = AS_INSTANCE(args[0]);
    RETURN_BOOL(table_set(vm, &instance->properties, args[1], args[2]));
}

/**
 * delprop(object: instance, name: string)
 *
 * deletes the named property from the object
 * @returns bool
 */
bool native_module_reflect__delprop(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(del_prop, 2);
    ENFORCE_ARG_TYPE(del_prop, 0, IS_INSTANCE);
    ENFORCE_ARG_TYPE(del_prop, 1, IS_STRING);

    b_obj_instance* instance = AS_INSTANCE(args[0]);
    RETURN_BOOL(table_delete(&instance->properties, args[1]));
}

/**
 * hasmethod(object: instance, name: string)
 *
 * returns true if class of the instance has the method name or
 * false if not
 */
bool native_module_reflect__hasmethod(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(has_method, 2);
    ENFORCE_ARG_TYPE(has_method, 0, IS_INSTANCE);
    ENFORCE_ARG_TYPE(has_method, 1, IS_STRING);

    b_obj_instance* instance = AS_INSTANCE(args[0]);
    b_value dummy;
    RETURN_BOOL(table_get(&instance->klass->methods, args[1], &dummy));
}

/**
 * getmethod(object: instance, name: string)
 *
 * returns the method in a class instance matching the given name
 * or nil if the class of the instance contains no method with
 * a matching name
 */
bool native_module_reflect__getmethod(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(get_method, 2);
    ENFORCE_ARG_TYPE(get_method, 0, IS_INSTANCE);
    ENFORCE_ARG_TYPE(get_method, 1, IS_STRING);

    b_obj_instance* instance = AS_INSTANCE(args[0]);
    b_value value;
    if(table_get(&instance->klass->methods, args[1], &value))
    {
        RETURN_VALUE(value);
    }
    RETURN_NIL;
}

bool native_module_reflect__call_method(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_MIN_ARG(call_method, 3);
    ENFORCE_ARG_TYPE(call_method, 0, IS_INSTANCE);
    ENFORCE_ARG_TYPE(call_method, 1, IS_STRING);
    ENFORCE_ARG_TYPE(call_method, 2, IS_LIST);

    b_value value;
    if(table_get(&AS_INSTANCE(args[0])->klass->methods, args[1], &value))
    {
        b_obj_bound* bound = (b_obj_bound*)GC(new_bound_method(vm, args[0], AS_CLOSURE(value)));

        b_obj_list* list = AS_LIST(args[2]);

        // remove the args list, the string name and the instance
        // then push the bound method
        pop_n(vm, 3);
        push(vm, OBJ_VAL(bound));

        // convert the list into function args
        for(int i = 0; i < list->items.count; i++)
        {
            push(vm, list->items.values[i]);
        }

        return call_value(vm, OBJ_VAL(bound), list->items.count);
    }

    RETURN;
}

bool native_module_reflect__bindmethod(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(delist, 2);
    ENFORCE_ARG_TYPE(delist, 0, IS_INSTANCE);
    ENFORCE_ARG_TYPE(delist, 1, IS_CLOSURE);

    b_obj_bound* bound = (b_obj_bound*)GC(new_bound_method(vm, args[0], AS_CLOSURE(args[1])));
    RETURN_OBJ(bound);
}

bool native_module_reflect__getboundmethod(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(get_method, 2);
    ENFORCE_ARG_TYPE(get_method, 0, IS_INSTANCE);
    ENFORCE_ARG_TYPE(get_method, 1, IS_STRING);

    b_obj_instance* instance = AS_INSTANCE(args[0]);
    b_value value;
    if(table_get(&instance->klass->methods, args[1], &value))
    {
        b_obj_bound* bound = (b_obj_bound*)GC(new_bound_method(vm, args[0], AS_CLOSURE(value)));
        RETURN_OBJ(bound);
    }
    RETURN_NIL;
}

bool native_module_reflect__gettype(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(get_type, 1);
    ENFORCE_ARG_TYPE(get_type, 0, IS_INSTANCE);
    RETURN_OBJ(AS_INSTANCE(args[0])->klass->name);
}

bool native_module_reflect__isptr(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(is_ptr, 1);
    RETURN_BOOL(IS_PTR(args[0]));
}

bool native_module_reflect__get_function_metadata(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(get_function_metadata, 1);
    ENFORCE_ARG_TYPE(get_function_metadata, 0, IS_CLOSURE);
    b_obj_closure* closure = AS_CLOSURE(args[0]);

    b_obj_dict* result = (b_obj_dict*)GC(new_dict(vm));
    dict_set_entry(vm, result, GC_STRING("name"), OBJ_VAL(closure->fnptr->name));
    dict_set_entry(vm, result, GC_STRING("arity"), NUMBER_VAL(closure->fnptr->arity));
    dict_set_entry(vm, result, GC_STRING("is_variadic"), NUMBER_VAL(closure->fnptr->is_variadic));
    dict_set_entry(vm, result, GC_STRING("captured_vars"), NUMBER_VAL(closure->up_value_count));
    dict_set_entry(vm, result, GC_STRING("module"), STRING_VAL(closure->fnptr->module->name));
    dict_set_entry(vm, result, GC_STRING("file"), STRING_VAL(closure->fnptr->module->file));

    RETURN_OBJ(result);
}

b_module_reg* blade_module_loader_reflect(b_vm* vm)
{
    static b_func_reg module_functions[] = {
        { "hasprop", true, native_module_reflect__hasprop },
        { "getprop", true, native_module_reflect__getprop },
        { "setprop", true, native_module_reflect__setprop },
        { "delprop", true, native_module_reflect__delprop },
        { "hasmethod", true, native_module_reflect__hasmethod },
        { "getmethod", true, native_module_reflect__getmethod },
        { "getboundmethod", true, native_module_reflect__getboundmethod },
        { "callmethod", true, native_module_reflect__call_method },
        { "bindmethod", true, native_module_reflect__bindmethod },
        { "gettype", true, native_module_reflect__gettype },
        { "isptr", true, native_module_reflect__isptr },
        { "getfunctionmetadata", true, native_module_reflect__get_function_metadata },
        { NULL, false, NULL },
    };

    static b_module_reg module = { .name = "_reflect", .fields = NULL, .functions = module_functions, .classes = NULL, .preloader = NULL, .unloader = NULL };

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
   | https://www.php.net/license/3_01.txt                                 |
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
#define LONG_FMT "%" PRId64

#define UNPACK_REAL_NAME() (strspn(real_name, "0123456789") == strlen(real_name) ? NUMBER_VAL(strtod(real_name, NULL)) : (GC_STRING(real_name)))

#ifndef MAX
    #define MAX(a, b) (a > b ? a : b)
#endif

static uint16_t reverse_int16(uint16_t arg)
{
    return ((arg & 0xFF) << 8) | ((arg >> 8) & 0xFF);
}

static uint32_t reverse_int32(uint32_t arg)
{
    uint32_t result;
    result = ((arg & 0xFF) << 24) | ((arg & 0xFF00) << 8) | ((arg >> 8) & 0xFF00) | ((arg >> 24) & 0xFF);

    return result;
}

static uint64_t reverse_int64(uint64_t arg)
{
    union swap_tag
    {
        uint64_t i;
        uint32_t ul[2];
    } tmp, result;

    tmp.i = arg;
    result.ul[0] = reverse_int32(tmp.ul[1]);
    result.ul[1] = reverse_int32(tmp.ul[0]);

    return result.i;
}

static long to_long(b_vm* vm, b_value value)
{
    if(IS_NUMBER(value))
    {
        return (long)AS_NUMBER(value);
    }
    else if(IS_BOOL(value))
    {
        return AS_BOOL(value) ? 1L : 0L;
    }
    else if(IS_NIL(value))
    {
        return -1L;
    }

    const char* v = (const char*)value_to_string(vm, value);
    int length = (int)strlen(v);

    int start = 0, end = 1, multiplier = 1;
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

static double to_double(b_vm* vm, b_value value)
{
    if(IS_NUMBER(value))
    {
        return AS_NUMBER(value);
    }
    else if(IS_BOOL(value))
    {
        return AS_BOOL(value) ? 1 : 0;
    }
    else if(IS_NIL(value))
    {
        return -1;
    }

    const char* v = (const char*)value_to_string(vm, value);
    int length = (int)strlen(v);

    int start = 0, end = 1, multiplier = 1;
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

static void do_pack(b_vm* vm, b_value val, size_t size, const int* map, unsigned char* output)
{
    size_t i;

    long as_long = to_long(vm, val);
    char* v = (char*)&as_long;

    for(i = 0; i < size; i++)
    {
        *output++ = v[map[i]];
    }
}

static void copy_float(int is_little_endian, void* dst, float f)
{
    union float_tag
    {
        float f;
        uint32_t i;
    } m;

    m.f = f;

#if IS_BIG_ENDIAN
    if(is_little_endian)
    {
#else
    if(!is_little_endian)
    {
#endif
        m.i = reverse_int32(m.i);
    }

    memcpy(dst, &m.f, sizeof(float));
}

static void copy_double(int is_little_endian, void* dst, double d)
{
    union double_tag
    {
        double d;
        uint64_t i;
    } m;

    m.d = d;

#if IS_BIG_ENDIAN
    if(is_little_endian)
    {
#else
    if(!is_little_endian)
    {
#endif
        m.i = reverse_int64(m.i);
    }

    memcpy(dst, &m.d, sizeof(double));
}

static char* ulong_to_buffer(char* buf, long num)
{
    *buf = '\0';
    do
    {
        *--buf = (char)((char)(num % 10) + '0');
        num /= 10;
    } while(num > 0);
    return buf;
}

static float parse_float(int is_little_endian, void* src)
{
    union float_tag
    {
        float f;
        uint32_t i;
    } m;

    memcpy(&m.i, src, sizeof(float));

#if IS_BIG_ENDIAN
    if(is_little_endian)
    {
#else
    if(!is_little_endian)
    {
#endif
        m.i = reverse_int32(m.i);
    }

    return m.f;
}

static double parse_double(int is_little_endian, void* src)
{
    union double_tag
    {
        double d;
        uint64_t i;
    } m;

    memcpy(&m.i, src, sizeof(double));

#if IS_BIG_ENDIAN
    if(is_little_endian)
    {
#else
    if(!is_little_endian)
    {
#endif
        m.i = reverse_int64(m.i);
    }

    return m.d;
}

/* Mapping of byte from char (8bit) to long for machine endian */
static int byte_map[1];

/* Mappings of bytes from int (machine dependent) to int for machine endian */
static int int_map[sizeof(int)];

/* Mappings of bytes from shorts (16bit) for all endian environments */
static int machine_endian_short_map[2];
static int big_endian_short_map[2];
static int little_endian_short_map[2];

/* Mappings of bytes from longs (32bit) for all endian environments */
static int machine_endian_long_map[4];
static int big_endian_long_map[4];
static int little_endian_long_map[4];

#if IS_64_BIT
/* Mappings of bytes from quads (64bit) for all endian environments */
static int machine_endian_longlong_map[8];
static int big_endian_longlong_map[8];
static int little_endian_longlong_map[8];
#endif

bool native_module_struct_pack(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(pack, 2);
    ENFORCE_ARG_TYPE(pack, 0, IS_STRING);
    ENFORCE_ARG_TYPE(pack, 1, IS_LIST);

    b_obj_string* string = AS_STRING(args[0]);
    b_obj_list* params = AS_LIST(args[1]);

    b_value* args_list = params->items.values;
    int param_count = params->items.count;

    size_t i;
    int currentarg;
    char* format = string->chars;
    size_t formatlen = string->length;
    size_t formatcount = 0;
    int outputpos = 0, outputsize = 0;

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
            /* Never uses any args_list */
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
                if(currentarg >= param_count)
                {
                    free(formatcodes);
                    free(formatargs);
                    RETURN_ERROR("Type %c: not enough arguments", code);
                }

                if(arg < 0)
                {
                    char* as_string = value_to_string(vm, args_list[currentarg]);
                    arg = (int)strlen(as_string);
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

                /* Use as many args_list as specified */
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
                    arg = param_count - currentarg;
                }
                if(currentarg > INT_MAX - arg)
                {
                    goto too_few_args;
                }
                currentarg += arg;

                if(currentarg > param_count)
                {
                too_few_args:
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

    if(currentarg < param_count)
    {
        // TODO: Give warning...
        //    RETURN_ERROR("%d arguments unused", (param_count - currentarg));
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
                size_t arg_cp = (code != 'Z') ? arg : MAX(0, arg - 1);
                char* str = value_to_string(vm, args_list[currentarg++]);

                memset(&output[outputpos], (code == 'a' || code == 'Z') ? '\0' : ' ', arg);
                memcpy(&output[outputpos], str, (strlen(str) < arg_cp) ? strlen(str) : arg_cp);

                outputpos += arg;
                break;
            }

            case 'h':
            case 'H':
            {
                int nibbleshift = (code == 'h') ? 0 : 4;
                int first = 1;
                char* str = value_to_string(vm, args_list[currentarg++]);

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
                    do_pack(vm, args_list[currentarg++], 1, byte_map, &output[outputpos]);
                    outputpos++;
                }
                break;

            case 's':
            case 'S':
            case 'n':
            case 'v':
            {
                int* map = machine_endian_short_map;

                if(code == 'n')
                {
                    map = big_endian_short_map;
                }
                else if(code == 'v')
                {
                    map = little_endian_short_map;
                }

                while(arg-- > 0)
                {
                    do_pack(vm, args_list[currentarg++], 2, map, &output[outputpos]);
                    outputpos += 2;
                }
                break;
            }

            case 'i':
            case 'I':
                while(arg-- > 0)
                {
                    do_pack(vm, args_list[currentarg++], sizeof(int), int_map, &output[outputpos]);
                    outputpos += sizeof(int);
                }
                break;

            case 'l':
            case 'L':
            case 'N':
            case 'V':
            {
                int* map = machine_endian_long_map;

                if(code == 'N')
                {
                    map = big_endian_long_map;
                }
                else if(code == 'V')
                {
                    map = little_endian_long_map;
                }

                while(arg-- > 0)
                {
                    do_pack(vm, args_list[currentarg++], 4, map, &output[outputpos]);
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
                int* map = machine_endian_longlong_map;

                if(code == 'J')
                {
                    map = big_endian_longlong_map;
                }
                else if(code == 'P')
                {
                    map = little_endian_longlong_map;
                }

                while(arg-- > 0)
                {
                    do_pack(vm, args_list[currentarg++], 8, map, &output[outputpos]);
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
                    float v = (float)to_double(vm, args_list[currentarg++]);
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
                    float v = (float)to_double(vm, args_list[currentarg++]);
                    copy_float(1, &output[outputpos], v);
                    outputpos += sizeof(v);
                }

                break;
            }
            case 'G':
            {
                /* pack big endian float */
                while(arg-- > 0)
                {
                    float v = (float)to_double(vm, args_list[currentarg++]);
                    copy_float(0, &output[outputpos], v);
                    outputpos += sizeof(v);
                }
                break;
            }

            case 'd':
            {
                while(arg-- > 0)
                {
                    double v = to_double(vm, args_list[currentarg++]);
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
                    double v = to_double(vm, args_list[currentarg++]);
                    copy_double(1, &output[outputpos], v);
                    outputpos += sizeof(v);
                }
                break;
            }

            case 'E':
            {
                /* pack big endian double */
                while(arg-- > 0)
                {
                    double v = to_double(vm, args_list[currentarg++]);
                    copy_double(0, &output[outputpos], v);
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

    b_obj_bytes* bytes = (b_obj_bytes*)GC(take_bytes(vm, output, outputpos));
    RETURN_OBJ(bytes);
}

bool native_module_struct_unpack(b_vm* vm, int arg_count, b_value* args)
{
    ENFORCE_ARG_COUNT(unpack, 3);
    ENFORCE_ARG_TYPE(unpack, 0, IS_STRING);
    ENFORCE_ARG_TYPE(unpack, 1, IS_BYTES);
    ENFORCE_ARG_TYPE(unpack, 2, IS_NUMBER);

    int i;
    b_obj_string* string = AS_STRING(args[0]);
    b_obj_bytes* data = AS_BYTES(args[1]);
    int offset = AS_NUMBER(args[2]);

    char* format = string->chars;
    char* input = (char*)data->bytes.bytes;
    size_t formatlen = string->length, inputpos = 0, inputlen = data->bytes.count;

    if(offset < 0 || offset > inputlen)
    {
        RETURN_ERROR("argument 3 (offset) must be within the range of argument 2 (data)");
    }

    input += offset;
    inputlen -= offset;

    b_obj_dict* return_value = (b_obj_dict*)GC(new_dict(vm));

    while(formatlen-- > 0)
    {
        char type = *(format++);
        char c;
        int repetitions = 1, argb;
        char* name;
        int namelen;
        int size = 0;

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
            namelen = 200;

        switch((int)type)
        {
            /* Never use any input */
            case 'X':
                size = -1;
                if(repetitions < 0)
                {
                    // TODO: Give warning...
                    //          RETURN_ERROR("Type %c: '*' ignored", type);
                    repetitions = 1;
                }
                break;

            case '@':
                size = 0;
                break;

            case 'a':
            case 'A':
            case 'Z':
                size = repetitions;
                repetitions = 1;
                break;

            case 'h':
            case 'H':
                size = (repetitions > 0) ? (repetitions + (repetitions % 2)) / 2 : repetitions;
                repetitions = 1;
                break;

                /* Use 1 byte of input */
            case 'c':
            case 'C':
            case 'x':
                size = 1;
                break;

                /* Use 2 bytes of input */
            case 's':
            case 'S':
            case 'n':
            case 'v':
                size = 2;
                break;

                /* Use sizeof(int) bytes of input */
            case 'i':
            case 'I':
                size = sizeof(int);
                break;

                /* Use 4 bytes of input */
            case 'l':
            case 'L':
            case 'N':
            case 'V':
                size = 4;
                break;

                /* Use 8 bytes of input */
            case 'q':
            case 'Q':
            case 'J':
            case 'P':
#if IS_64_BIT
                size = 8;
                break;
#else
                RETURN_ERROR("64-bit format codes are not available for 32-bit Blade");
#endif

                /* Use sizeof(float) bytes of input */
            case 'f':
            case 'g':
            case 'G':
                size = sizeof(float);
                break;

                /* Use sizeof(double) bytes of input */
            case 'd':
            case 'e':
            case 'E':
                size = sizeof(double);
                break;

            default:
                RETURN_ERROR("Invalid format type %c", type);
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
            if(size != 0 && size != -1 && INT_MAX - size + 1 < inputpos)
            {
                // TODO: Give warning...
                //        RETURN_ERROR("Type %c: integer overflow", type);
                RETURN_FALSE;
            }

            if((inputpos + size) <= inputlen)
            {
                char* real_name;

                if(repetitions == 1 && namelen > 0)
                {
                    /* Use a part of the formatarg argument directly as the name. */
                    real_name = ALLOCATE(char, namelen);
                    memcpy(real_name, name, namelen);
                    real_name[namelen] = '\0';
                }
                else
                {
                    /* Need to add the 1-based element number to the name */

                    char buf[MAX_LENGTH_OF_LONG + 1];
                    char* res = ulong_to_buffer(buf + sizeof(buf) - 1, i + 1);
                    size_t digits = buf + sizeof(buf) - 1 - res;

                    real_name = ALLOCATE(char, namelen + digits);
                    if(real_name == NULL)
                    {
                        RETURN_ERROR("out of memory");
                    }

                    memcpy(real_name, name, namelen);
                    memcpy(real_name + namelen, res, digits);
                    real_name[namelen + digits] = '\0';
                }

                switch((int)type)
                {
                    case 'a':
                    {
                        /* a will not strip any trailing whitespace or null padding */
                        size_t len = inputlen - inputpos; /* Remaining string */

                        /* If size was given take minimum of len and size */
                        if((size >= 0) && (len > size))
                        {
                            len = size;
                        }

                        size = (int)len;

                        dict_set_entry(vm, return_value, UNPACK_REAL_NAME(), GC_L_STRING(&input[inputpos], len));
                        break;
                    }
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
                        if((size >= 0) && (len > size))
                        {
                            len = size;
                        }

                        size = (int)len;

                        /* Remove trailing white space and nulls chars from unpacked data */
                        while(--len >= 0)
                        {
                            if(input[inputpos + len] != padn && input[inputpos + len] != pads && input[inputpos + len] != padt && input[inputpos + len] != padc
                               && input[inputpos + len] != padl)
                                break;
                        }

                        dict_set_entry(vm, return_value, UNPACK_REAL_NAME(), GC_L_STRING(&input[inputpos], len + 1));
                        break;
                    }
                        /* New option added for Z to remain in-line with the Perl implementation */
                    case 'Z':
                    {
                        /* Z will strip everything after the first null character */
                        char pad = '\0';
                        size_t s, len = inputlen - inputpos; /* Remaining string */

                        /* If size was given take minimum of len and size */
                        if((size >= 0) && (len > size))
                        {
                            len = size;
                        }

                        size = (int)len;

                        /* Remove everything after the first null */
                        for(s = 0; s < len; s++)
                        {
                            if(input[inputpos + s] == pad)
                                break;
                        }
                        len = s;

                        dict_set_entry(vm, return_value, UNPACK_REAL_NAME(), GC_L_STRING(&input[inputpos], len));
                        break;
                    }

                    case 'h':
                    case 'H':
                    {
                        size_t len = (inputlen - inputpos) * 2; /* Remaining */
                        int nibbleshift = (type == 'h') ? 0 : 4;
                        int first = 1;
                        size_t ipos, opos;

                        /* If size was given take minimum of len and size */
                        if(size >= 0 && len > (size * 2))
                        {
                            len = size * 2;
                        }

                        if(len > 0 && argb > 0)
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

                        dict_set_entry(vm, return_value, UNPACK_REAL_NAME(), GC_L_STRING(buf, len));
                        break;
                    }

                    case 'c': /* signed */
                    case 'C':
                    { /* unsigned */
                        uint8_t x = input[inputpos];
                        long v = (type == 'c') ? (int8_t)x : x;

                        dict_set_entry(vm, return_value, UNPACK_REAL_NAME(), NUMBER_VAL(v));
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
                            v = reverse_int16(x);
                        }
                        else
                        {
                            v = x;
                        }

                        dict_set_entry(vm, return_value, UNPACK_REAL_NAME(), NUMBER_VAL(v));
                        break;
                    }

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

                        dict_set_entry(vm, return_value, UNPACK_REAL_NAME(), NUMBER_VAL(v));
                        break;
                    }

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
                            v = reverse_int32(x);
                        }
                        else
                        {
                            v = x;
                        }

                        dict_set_entry(vm, return_value, UNPACK_REAL_NAME(), NUMBER_VAL(v));
                        break;
                    }

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
                            v = reverse_int64(x);
                        }
                        else
                        {
                            v = x;
                        }

                        dict_set_entry(vm, return_value, UNPACK_REAL_NAME(), NUMBER_VAL(v));
                        break;
#else
                        RETURN_ERROR("q, Q, J and P are only valid on 64 bit build of Blade");
#endif
                    }

                    case 'f': /* float */
                    case 'g': /* little endian float*/
                    case 'G': /* big endian float*/
                    {
                        float v;

                        if(type == 'g')
                        {
                            v = parse_float(1, &input[inputpos]);
                        }
                        else if(type == 'G')
                        {
                            v = parse_float(0, &input[inputpos]);
                        }
                        else
                        {
                            memcpy(&v, &input[inputpos], sizeof(float));
                        }

                        dict_set_entry(vm, return_value, UNPACK_REAL_NAME(), NUMBER_VAL(v));
                        break;
                    }

                    case 'd': /* double */
                    case 'e': /* little endian float */
                    case 'E': /* big endian float */
                    {
                        double v;
                        if(type == 'e')
                        {
                            v = parse_double(1, &input[inputpos]);
                        }
                        else if(type == 'E')
                        {
                            v = parse_double(0, &input[inputpos]);
                        }
                        else
                        {
                            memcpy(&v, &input[inputpos], sizeof(double));
                        }

                        dict_set_entry(vm, return_value, UNPACK_REAL_NAME(), NUMBER_VAL(v));
                        break;
                    }

                    case 'x':
                        /* Do nothing with input, just skip it */
                        break;

                    case 'X':
                        if(inputpos < size)
                        {
                            inputpos = -size;
                            i = repetitions - 1; /* Break out of for loop */

                            if(repetitions >= 0)
                            {
                                // TODO: Give warning...
                                //                RETURN_ERROR("Type %c: outside of string", type);
                            }
                        }
                        break;

                    case '@':
                        if(repetitions <= inputlen)
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
                if(inputpos < 0)
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

    RETURN_OBJ(return_value);
}

void __struct_module_preloader(b_vm* vm)
{
    int i;

#if IS_LITTLE_ENDIAN
    /* Where to get lo to hi bytes from */
    byte_map[0] = 0;

    for(i = 0; i < (int)sizeof(int); i++)
    {
        int_map[i] = i;
    }

    machine_endian_short_map[0] = 0;
    machine_endian_short_map[1] = 1;
    big_endian_short_map[0] = 1;
    big_endian_short_map[1] = 0;
    little_endian_short_map[0] = 0;
    little_endian_short_map[1] = 1;

    machine_endian_long_map[0] = 0;
    machine_endian_long_map[1] = 1;
    machine_endian_long_map[2] = 2;
    machine_endian_long_map[3] = 3;
    big_endian_long_map[0] = 3;
    big_endian_long_map[1] = 2;
    big_endian_long_map[2] = 1;
    big_endian_long_map[3] = 0;
    little_endian_long_map[0] = 0;
    little_endian_long_map[1] = 1;
    little_endian_long_map[2] = 2;
    little_endian_long_map[3] = 3;

    #if IS_64_BIT
    machine_endian_longlong_map[0] = 0;
    machine_endian_longlong_map[1] = 1;
    machine_endian_longlong_map[2] = 2;
    machine_endian_longlong_map[3] = 3;
    machine_endian_longlong_map[4] = 4;
    machine_endian_longlong_map[5] = 5;
    machine_endian_longlong_map[6] = 6;
    machine_endian_longlong_map[7] = 7;
    big_endian_longlong_map[0] = 7;
    big_endian_longlong_map[1] = 6;
    big_endian_longlong_map[2] = 5;
    big_endian_longlong_map[3] = 4;
    big_endian_longlong_map[4] = 3;
    big_endian_longlong_map[5] = 2;
    big_endian_longlong_map[6] = 1;
    big_endian_longlong_map[7] = 0;
    little_endian_longlong_map[0] = 0;
    little_endian_longlong_map[1] = 1;
    little_endian_longlong_map[2] = 2;
    little_endian_longlong_map[3] = 3;
    little_endian_longlong_map[4] = 4;
    little_endian_longlong_map[5] = 5;
    little_endian_longlong_map[6] = 6;
    little_endian_longlong_map[7] = 7;
    #endif
#else
    int size = sizeof(long);

    /* Where to get hi to lo bytes from */
    byte_map[0] = size - 1;

    for(i = 0; i < (int)sizeof(int); i++)
    {
        int_map[i] = size - (sizeof(int) - i);
    }

    machine_endian_short_map[0] = size - 2;
    machine_endian_short_map[1] = size - 1;
    big_endian_short_map[0] = size - 2;
    big_endian_short_map[1] = size - 1;
    little_endian_short_map[0] = size - 1;
    little_endian_short_map[1] = size - 2;

    machine_endian_long_map[0] = size - 4;
    machine_endian_long_map[1] = size - 3;
    machine_endian_long_map[2] = size - 2;
    machine_endian_long_map[3] = size - 1;
    big_endian_long_map[0] = size - 4;
    big_endian_long_map[1] = size - 3;
    big_endian_long_map[2] = size - 2;
    big_endian_long_map[3] = size - 1;
    little_endian_long_map[0] = size - 1;
    little_endian_long_map[1] = size - 2;
    little_endian_long_map[2] = size - 3;
    little_endian_long_map[3] = size - 4;

    #if ISIS_64_BIT
    machine_endian_longlong_map[0] = size - 8;
    machine_endian_longlong_map[1] = size - 7;
    machine_endian_longlong_map[2] = size - 6;
    machine_endian_longlong_map[3] = size - 5;
    machine_endian_longlong_map[4] = size - 4;
    machine_endian_longlong_map[5] = size - 3;
    machine_endian_longlong_map[6] = size - 2;
    machine_endian_longlong_map[7] = size - 1;
    big_endian_longlong_map[0] = size - 8;
    big_endian_longlong_map[1] = size - 7;
    big_endian_longlong_map[2] = size - 6;
    big_endian_longlong_map[3] = size - 5;
    big_endian_longlong_map[4] = size - 4;
    big_endian_longlong_map[5] = size - 3;
    big_endian_longlong_map[6] = size - 2;
    big_endian_longlong_map[7] = size - 1;
    little_endian_longlong_map[0] = size - 1;
    little_endian_longlong_map[1] = size - 2;
    little_endian_longlong_map[2] = size - 3;
    little_endian_longlong_map[3] = size - 4;
    little_endian_longlong_map[4] = size - 5;
    little_endian_longlong_map[5] = size - 6;
    little_endian_longlong_map[6] = size - 7;
    little_endian_longlong_map[7] = size - 8;
    #endif
#endif
}

b_module_reg* blade_module_loader_struct(b_vm* vm)
{
    static b_func_reg module_functions[] = {
        { "pack", true, native_module_struct_pack },
        { "unpack", true, native_module_struct_unpack },
        { NULL, false, NULL },
    };

    static b_module_reg module
    = { .name = "_struct", .fields = NULL, .functions = module_functions, .classes = NULL, .preloader = &__struct_module_preloader, .unloader = NULL };

    return &module;
}

#define ERR_CANT_ASSIGN_EMPTY "empty cannot be assigned."

static void reset_stack(b_vm* vm)
{
    vm->stack_top = vm->stack;
    vm->frame_count = 0;
    vm->open_up_values = NULL;
}

static b_value get_stack_trace(b_vm* vm)
{
    char* trace = calloc(0, sizeof(char));

    if(trace != NULL)
    {
        for(int i = 0; i < vm->frame_count; i++)
        {
            b_call_frame* frame = &vm->frames[i];
            b_obj_func* function = frame->closure->fnptr;

            // -1 because the IP is sitting on the next instruction to be executed
            size_t instruction = frame->ip - function->blob.code - 1;
            int line = function->blob.lines[instruction];

            const char* trace_format = i != vm->frame_count - 1 ? "    %s:%d -> %s()\n" : "    %s:%d -> %s()";
            char* fn_name = function->name == NULL ? "@.script" : function->name->chars;
            size_t trace_line_length = snprintf(NULL, 0, trace_format, function->module->file, line, fn_name);

            char* trace_line = ALLOCATE(char, trace_line_length + 1);
            if(trace_line != NULL)
            {
                sprintf(trace_line, trace_format, function->module->file, line, fn_name);
                trace_line[(int)trace_line_length] = '\0';
            }

            trace = append_strings(trace, trace_line);
            free(trace_line);
        }

        return STRING_TT_VAL(trace);
    }

    return STRING_L_VAL("", 0);
}

bool bl_vm_propagateexception(b_vm* vm, bool is_assert)
{
    b_obj_instance* exception = AS_INSTANCE(peek(vm, 0));

    while(vm->frame_count > 0)
    {
        b_call_frame* frame = &vm->frames[vm->frame_count - 1];
        for(int i = frame->handlers_count; i > 0; i--)
        {
            b_exception_frame handler = frame->handlers[i - 1];
            b_obj_func* function = frame->closure->fnptr;

            if(handler.address != 0 && bl_class_isinstanceof(exception->klass, handler.klass->name->chars))
            {
                frame->ip = &function->blob.code[handler.address];
                return true;
            }
            else if(handler.finally_address != 0)
            {
                push(vm, TRUE_VAL);// continue propagating once the 'finally' block completes
                frame->ip = &function->blob.code[handler.finally_address];
                return true;
            }
        }

        vm->frame_count--;
    }

    fflush(stdout);// flush out anything on stdout first

    b_value message, trace;
    if(!is_assert)
    {
        fprintf(stderr, "Unhandled %s", exception->klass->name->chars);
    }
    else
    {
        fprintf(stderr, "Illegal State");
    }
    if(table_get(&exception->properties, STRING_L_VAL("message", 7), &message))
    {
        char* error_message = value_to_string(vm, message);
        if(strlen(error_message) > 0)
        {
            fprintf(stderr, ": %s", error_message);
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

    if(table_get(&exception->properties, STRING_L_VAL("stacktrace", 10), &trace))
    {
        char* trace_str = value_to_string(vm, trace);
        fprintf(stderr, "  StackTrace:\n%s\n", trace_str);
        free(trace_str);
    }

    return false;
}

bool bl_vm_pushexceptionhandler(b_vm* vm, b_obj_class* type, int address, int finally_address)
{
    b_call_frame* frame = &vm->frames[vm->frame_count - 1];
    if(frame->handlers_count == MAX_EXCEPTION_HANDLERS)
    {
        bl_vm_runtimeerror(vm, "too many nested exception handlers in one function");
        return false;
    }
    frame->handlers[frame->handlers_count].address = address;
    frame->handlers[frame->handlers_count].finally_address = finally_address;
    frame->handlers[frame->handlers_count].klass = type;
    frame->handlers_count++;
    return true;
}

bool bl_vm_throwexception(b_vm* vm, bool is_assert, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    char* message = NULL;
    int length = vasprintf(&message, format, args);
    va_end(args);

    b_obj_instance* instance = create_exception(vm, take_string(vm, message, length));
    push(vm, OBJ_VAL(instance));

    b_value stacktrace = get_stack_trace(vm);
    table_set(vm, &instance->properties, STRING_L_VAL("stacktrace", 10), stacktrace);
    return bl_vm_propagateexception(vm, is_assert);
}

static void initialize_exceptions(b_vm* vm, b_obj_module* module)
{
    size_t slen;
    const char* sstr;
    b_obj_string* class_name;
    sstr = "Exception";
    slen = strlen(sstr);
    //class_name = copy_string(vm, sstr, slen);
    class_name = bl_string_fromallocated(vm, strdup(sstr), slen, hash_string(sstr, slen));
    
    

    push(vm, OBJ_VAL(class_name));
    b_obj_class* klass = new_class(vm, class_name);
    pop(vm);

    push(vm, OBJ_VAL(klass));
    b_obj_func* function = new_function(vm, module, TYPE_METHOD);
    pop(vm);

    function->arity = 1;
    function->is_variadic = false;

    // g_loc 0
    write_blob(vm, &function->blob, OP_GET_LOCAL, 0);
    write_blob(vm, &function->blob, (0 >> 8) & 0xff, 0);
    write_blob(vm, &function->blob, 0 & 0xff, 0);

    // g_loc 1
    write_blob(vm, &function->blob, OP_GET_LOCAL, 0);
    write_blob(vm, &function->blob, (1 >> 8) & 0xff, 0);
    write_blob(vm, &function->blob, 1 & 0xff, 0);

    int message_const = add_constant(vm, &function->blob, STRING_L_VAL("message", 7));

    // s_prop 1
    write_blob(vm, &function->blob, OP_SET_PROPERTY, 0);
    write_blob(vm, &function->blob, (message_const >> 8) & 0xff, 0);
    write_blob(vm, &function->blob, message_const & 0xff, 0);

    // pop
    write_blob(vm, &function->blob, OP_POP, 0);

    // g_loc 0
    write_blob(vm, &function->blob, OP_GET_LOCAL, 0);
    write_blob(vm, &function->blob, (0 >> 8) & 0xff, 0);
    write_blob(vm, &function->blob, 0 & 0xff, 0);

    // ret
    write_blob(vm, &function->blob, OP_RETURN, 0);

    push(vm, OBJ_VAL(function));
    b_obj_closure* closure = new_closure(vm, function);
    pop(vm);

    // set class constructor
    push(vm, OBJ_VAL(closure));
    table_set(vm, &klass->methods, OBJ_VAL(class_name), OBJ_VAL(closure));
    klass->initializer = OBJ_VAL(closure);

    // set class properties
    table_set(vm, &klass->properties, STRING_L_VAL("message", 7), NIL_VAL);
    table_set(vm, &klass->properties, STRING_L_VAL("stacktrace", 10), NIL_VAL);

    table_set(vm, &vm->globals, OBJ_VAL(class_name), OBJ_VAL(klass));

    pop(vm);
    pop(vm);// assert error name

    vm->exception_class = klass;
}

b_obj_instance* create_exception(b_vm* vm, b_obj_string* message)
{
    b_obj_instance* instance = new_instance(vm, vm->exception_class);
    push(vm, OBJ_VAL(instance));
    table_set(vm, &instance->properties, STRING_L_VAL("message", 7), OBJ_VAL(message));
    pop(vm);
    return instance;
}

void bl_vm_runtimeerror(b_vm* vm, const char* format, ...)
{
    fflush(stdout);// flush out anything on stdout first

    b_call_frame* frame = &vm->frames[vm->frame_count - 1];
    b_obj_func* function = frame->closure->fnptr;

    size_t instruction = frame->ip - function->blob.code - 1;
    int line = function->blob.lines[instruction];

    fprintf(stderr, "RuntimeError: ");

    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fprintf(stderr, " -> %s:%d ", function->module->file, line);
    fputs("\n", stderr);

    if(vm->frame_count > 1)
    {
        fprintf(stderr, "StackTrace:\n");
        for(int i = vm->frame_count - 1; i >= 0; i--)
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

    reset_stack(vm);
}

void push(b_vm* vm, b_value value)
{
    *vm->stack_top = value;
    vm->stack_top++;
}

b_value pop(b_vm* vm)
{
    vm->stack_top--;
    return *vm->stack_top;
}

b_value pop_n(b_vm* vm, int n)
{
    vm->stack_top -= n;
    return *vm->stack_top;
}

b_value peek(b_vm* vm, int distance)
{
    return vm->stack_top[-1 - distance];
}

static void define_native(b_vm* vm, const char* name, b_native_fn function)
{
    push(vm, STRING_VAL(name));
    push(vm, OBJ_VAL(new_native(vm, function, name)));
    table_set(vm, &vm->globals, vm->stack[0], vm->stack[1]);
    pop_n(vm, 2);
}

void define_native_method(b_vm* vm, b_table* table, const char* name, b_native_fn function)
{
    push(vm, STRING_VAL(name));
    push(vm, OBJ_VAL(new_native(vm, function, name)));
    table_set(vm, table, vm->stack[0], vm->stack[1]);
    pop_n(vm, 2);
}

static void init_builtin_functions(b_vm* vm)
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

static void init_builtin_methods(b_vm* vm)
{
/*
#define DEFINE._STRING_METHOD(name) DEFINE_METHOD(string, name)
#define DEFINE._LIST_METHOD(name) DEFINE_METHOD(list, name)
#define DEFINE._DICT_METHOD(name) DEFINE_METHOD(dict, name)
#define DEFINE._FILE_METHOD(name) DEFINE_METHOD(file, name)
#define DEFINE._BYTES_METHOD(name) DEFINE_METHOD(bytes, name)
#define DEFINE._RANGE_METHOD(name) DEFINE_METHOD(range, name)
*/
    // string methods
    define_native_method(vm, &vm->methods_string, "length", native_method_stringlength);
    define_native_method(vm, &vm->methods_string, "upper", native_method_stringupper);
    define_native_method(vm, &vm->methods_string, "lower", native_method_stringlower);
    define_native_method(vm, &vm->methods_string, "is_alpha", native_method_stringis_alpha);
    define_native_method(vm, &vm->methods_string, "is_alnum", native_method_stringis_alnum);
    define_native_method(vm, &vm->methods_string, "is_number", native_method_stringis_number);
    define_native_method(vm, &vm->methods_string, "is_lower", native_method_stringis_lower);
    define_native_method(vm, &vm->methods_string, "is_upper", native_method_stringis_upper);
    define_native_method(vm, &vm->methods_string, "is_space", native_method_stringis_space);
    define_native_method(vm, &vm->methods_string, "trim", native_method_stringtrim);
    define_native_method(vm, &vm->methods_string, "ltrim", native_method_stringltrim);
    define_native_method(vm, &vm->methods_string, "rtrim", native_method_stringrtrim);
    define_native_method(vm, &vm->methods_string, "join", native_method_stringjoin);
    define_native_method(vm, &vm->methods_string, "split", native_method_stringsplit);
    define_native_method(vm, &vm->methods_string, "index_of", native_method_stringindex_of);
    define_native_method(vm, &vm->methods_string, "starts_with", native_method_stringstarts_with);
    define_native_method(vm, &vm->methods_string, "ends_with", native_method_stringends_with);
    define_native_method(vm, &vm->methods_string, "count", native_method_stringcount);
    define_native_method(vm, &vm->methods_string, "to_number", native_method_stringto_number);
    define_native_method(vm, &vm->methods_string, "to_list", native_method_stringto_list);
    define_native_method(vm, &vm->methods_string, "to_bytes", native_method_stringto_bytes);
    define_native_method(vm, &vm->methods_string, "lpad", native_method_stringlpad);
    define_native_method(vm, &vm->methods_string, "rpad", native_method_stringrpad);
    define_native_method(vm, &vm->methods_string, "match", native_method_stringmatch);
    define_native_method(vm, &vm->methods_string, "matches", native_method_stringmatches);
    define_native_method(vm, &vm->methods_string, "replace", native_method_stringreplace);
    define_native_method(vm, &vm->methods_string, "ascii", native_method_stringascii);
    define_native_method(vm, &vm->methods_string, "@iter", native_method_string__iter__);
    define_native_method(vm, &vm->methods_string, "@itern", native_method_string__itern__);

    // list methods
    define_native_method(vm, &vm->methods_list, "length", native_method_listlength);
    define_native_method(vm, &vm->methods_list, "append", native_method_listappend);
    define_native_method(vm, &vm->methods_list, "clear", native_method_listclear);
    define_native_method(vm, &vm->methods_list, "clone", native_method_listclone);
    define_native_method(vm, &vm->methods_list, "count", native_method_listcount);
    define_native_method(vm, &vm->methods_list, "extend", native_method_listextend);
    define_native_method(vm, &vm->methods_list, "index_of", native_method_listindex_of);
    define_native_method(vm, &vm->methods_list, "insert", native_method_listinsert);
    define_native_method(vm, &vm->methods_list, "pop", native_method_listpop);
    define_native_method(vm, &vm->methods_list, "shift", native_method_listshift);
    define_native_method(vm, &vm->methods_list, "remove_at", native_method_listremove_at);
    define_native_method(vm, &vm->methods_list, "remove", native_method_listremove);
    define_native_method(vm, &vm->methods_list, "reverse", native_method_listreverse);
    define_native_method(vm, &vm->methods_list, "sort", native_method_listsort);
    define_native_method(vm, &vm->methods_list, "contains", native_method_listcontains);
    define_native_method(vm, &vm->methods_list, "delete", native_method_listdelete);
    define_native_method(vm, &vm->methods_list, "first", native_method_listfirst);
    define_native_method(vm, &vm->methods_list, "last", native_method_listlast);
    define_native_method(vm, &vm->methods_list, "is_empty", native_method_listis_empty);
    define_native_method(vm, &vm->methods_list, "take", native_method_listtake);
    define_native_method(vm, &vm->methods_list, "get", native_method_listget);
    define_native_method(vm, &vm->methods_list, "compact", native_method_listcompact);
    define_native_method(vm, &vm->methods_list, "unique", native_method_listunique);
    define_native_method(vm, &vm->methods_list, "zip", native_method_listzip);
    define_native_method(vm, &vm->methods_list, "to_dict", native_method_listto_dict);
    define_native_method(vm, &vm->methods_list, "@iter", native_method_list__iter__);
    define_native_method(vm, &vm->methods_list, "@itern", native_method_list__itern__);

    // dictionary methods
    define_native_method(vm, &vm->methods_dict, "length", native_method_dictlength);
    define_native_method(vm, &vm->methods_dict, "add", native_method_dictadd);
    define_native_method(vm, &vm->methods_dict, "set", native_method_dictset);
    define_native_method(vm, &vm->methods_dict, "clear", native_method_dictclear);
    define_native_method(vm, &vm->methods_dict, "clone", native_method_dictclone);
    define_native_method(vm, &vm->methods_dict, "compact", native_method_dictcompact);
    define_native_method(vm, &vm->methods_dict, "contains", native_method_dictcontains);
    define_native_method(vm, &vm->methods_dict, "extend", native_method_dictextend);
    define_native_method(vm, &vm->methods_dict, "get", native_method_dictget);
    define_native_method(vm, &vm->methods_dict, "keys", native_method_dictkeys);
    define_native_method(vm, &vm->methods_dict, "values", native_method_dictvalues);
    define_native_method(vm, &vm->methods_dict, "remove", native_method_dictremove);
    define_native_method(vm, &vm->methods_dict, "is_empty", native_method_dictis_empty);
    define_native_method(vm, &vm->methods_dict, "find_key", native_method_dictfind_key);
    define_native_method(vm, &vm->methods_dict, "to_list", native_method_dictto_list);
    define_native_method(vm, &vm->methods_dict, "@iter", native_method_dict__iter__);
    define_native_method(vm, &vm->methods_dict, "@itern", native_method_dict__itern__);

    // file methods
    define_native_method(vm, &vm->methods_file, "exists", native_method_fileexists);
    define_native_method(vm, &vm->methods_file, "close", native_method_fileclose);
    define_native_method(vm, &vm->methods_file, "open", native_method_fileopen);
    define_native_method(vm, &vm->methods_file, "read", native_method_fileread);
    define_native_method(vm, &vm->methods_file, "gets", native_method_filegets);
    define_native_method(vm, &vm->methods_file, "write", native_method_filewrite);
    define_native_method(vm, &vm->methods_file, "puts", native_method_fileputs);
    define_native_method(vm, &vm->methods_file, "number", native_method_filenumber);
    define_native_method(vm, &vm->methods_file, "is_tty", native_method_fileis_tty);
    define_native_method(vm, &vm->methods_file, "is_open", native_method_fileis_open);
    define_native_method(vm, &vm->methods_file, "is_closed", native_method_fileis_closed);
    define_native_method(vm, &vm->methods_file, "flush", native_method_fileflush);
    define_native_method(vm, &vm->methods_file, "stats", native_method_filestats);
    define_native_method(vm, &vm->methods_file, "symlink", native_method_filesymlink);
    define_native_method(vm, &vm->methods_file, "delete", native_method_filedelete);
    define_native_method(vm, &vm->methods_file, "rename", native_method_filerename);
    define_native_method(vm, &vm->methods_file, "path", native_method_filepath);
    define_native_method(vm, &vm->methods_file, "abs_path", native_method_fileabs_path);
    define_native_method(vm, &vm->methods_file, "copy", native_method_filecopy);
    define_native_method(vm, &vm->methods_file, "truncate", native_method_filetruncate);
    define_native_method(vm, &vm->methods_file, "chmod", native_method_filechmod);
    define_native_method(vm, &vm->methods_file, "set_times", native_method_fileset_times);
    define_native_method(vm, &vm->methods_file, "seek", native_method_fileseek);
    define_native_method(vm, &vm->methods_file, "tell", native_method_filetell);
    define_native_method(vm, &vm->methods_file, "mode", native_method_filemode);
    define_native_method(vm, &vm->methods_file, "name", native_method_filename);

    // bytes
    define_native_method(vm, &vm->methods_bytes, "length", native_method_byteslength);
    define_native_method(vm, &vm->methods_bytes, "append", native_method_bytesappend);
    define_native_method(vm, &vm->methods_bytes, "clone", native_method_bytesclone);
    define_native_method(vm, &vm->methods_bytes, "extend", native_method_bytesextend);
    define_native_method(vm, &vm->methods_bytes, "pop", native_method_bytespop);
    define_native_method(vm, &vm->methods_bytes, "remove", native_method_bytesremove);
    define_native_method(vm, &vm->methods_bytes, "reverse", native_method_bytesreverse);
    define_native_method(vm, &vm->methods_bytes, "first", native_method_bytesfirst);
    define_native_method(vm, &vm->methods_bytes, "last", native_method_byteslast);
    define_native_method(vm, &vm->methods_bytes, "get", native_method_bytesget);
    define_native_method(vm, &vm->methods_bytes, "split", native_method_bytessplit);
    define_native_method(vm, &vm->methods_bytes, "dispose", native_method_bytesdispose);
    define_native_method(vm, &vm->methods_bytes, "is_alpha", native_method_bytesis_alpha);
    define_native_method(vm, &vm->methods_bytes, "is_alnum", native_method_bytesis_alnum);
    define_native_method(vm, &vm->methods_bytes, "is_number", native_method_bytesis_number);
    define_native_method(vm, &vm->methods_bytes, "is_lower", native_method_bytesis_lower);
    define_native_method(vm, &vm->methods_bytes, "is_upper", native_method_bytesis_upper);
    define_native_method(vm, &vm->methods_bytes, "is_space", native_method_bytesis_space);
    define_native_method(vm, &vm->methods_bytes, "to_list", native_method_bytesto_list);
    define_native_method(vm, &vm->methods_bytes, "to_string", native_method_bytesto_string);
    define_native_method(vm, &vm->methods_bytes, "@iter", native_method_bytes__iter__);
    define_native_method(vm, &vm->methods_bytes, "@itern", native_method_bytes__itern__);

    // range
    define_native_method(vm, &vm->methods_range, "lower", native_method_rangelower);
    define_native_method(vm, &vm->methods_range, "upper", native_method_rangeupper);
    define_native_method(vm, &vm->methods_range, "@iter", native_method_range__iter__);
    define_native_method(vm, &vm->methods_range, "@itern", native_method_range__itern__);

#undef DEFINE_STRING_METHOD
#undef DEFINE_LIST_METHOD
#undef DEFINE_DICT_METHOD
#undef DEFINE_FILE_METHOD
#undef DEFINE_BYTES_METHOD
#undef DEFINE_RANGE_METHOD
}

void init_vm(b_vm* vm)
{
    fprintf(stderr, "call to init_vm()\n");
    reset_stack(vm);
    vm->compiler = NULL;
    vm->objectlinks = NULL;
    vm->objectcount = 0;
    vm->exception_class = NULL;
    vm->bytes_allocated = 0;
    vm->gc_protected = 0;
    vm->next_gc = DEFAULT_GC_START;// default is 1mb. Can be modified via the -g flag.
    vm->is_repl = false;
    vm->should_debug_stack = false;
    vm->should_print_bytecode = false;

    vm->gray_count = 0;
    vm->gray_capacity = 0;
    vm->frame_count = 0;
    vm->gray_stack = NULL;

    vm->std_args = NULL;
    vm->std_args_count = 0;
    vm->allowgc = false;
    init_table(&vm->modules);
    init_table(&vm->strings);
    init_table(&vm->globals);

    // object methods tables
    init_table(&vm->methods_string);
    init_table(&vm->methods_list);
    init_table(&vm->methods_dict);
    init_table(&vm->methods_file);
    init_table(&vm->methods_bytes);
    init_table(&vm->methods_range);

    init_builtin_functions(vm);
    init_builtin_methods(vm);
    //vm->allowgc = true;
}

void free_vm(b_vm* vm)
{
    fprintf(stderr, "call to free_vm()\n");
    //@TODO: Fix segfault from enabling this...
    free_objects(vm);
    free_table(vm, &vm->strings);
    free_table(vm, &vm->globals);
    // since object in module can exist in globals
    // it must come after
    clean_free_table(vm, &vm->modules);

    free_table(vm, &vm->methods_string);
    free_table(vm, &vm->methods_list);
    free_table(vm, &vm->methods_dict);
    free_table(vm, &vm->methods_file);
    free_table(vm, &vm->methods_bytes);
}

static bool bl_vm_docall(b_vm* vm, b_obj_closure* closure, int arg_count)
{
    // fill empty parameters if not variadic
    for(; !closure->fnptr->is_variadic && arg_count < closure->fnptr->arity; arg_count++)
    {
        push(vm, NIL_VAL);
    }

    // handle variadic arguments...
    if(closure->fnptr->is_variadic && arg_count >= closure->fnptr->arity - 1)
    {
        int va_args_start = arg_count - closure->fnptr->arity;
        b_obj_list* args_list = new_list(vm);
        push(vm, OBJ_VAL(args_list));

        for(int i = va_args_start; i >= 0; i--)
        {
            write_value_arr(vm, &args_list->items, peek(vm, i + 1));
        }
        arg_count -= va_args_start;
        pop_n(vm, va_args_start + 2);// +1 for the gc protection push above
        push(vm, OBJ_VAL(args_list));
    }

    if(arg_count != closure->fnptr->arity)
    {
        pop_n(vm, arg_count);
        if(closure->fnptr->is_variadic)
        {
            return bl_vm_throwexception(vm, false, "expected at least %d arguments but got %d", closure->fnptr->arity - 1, arg_count);
        }
        else
        {
            return bl_vm_throwexception(vm, false, "expected %d arguments but got %d", closure->fnptr->arity, arg_count);
        }
    }

    if(vm->frame_count == FRAMES_MAX)
    {
        pop_n(vm, arg_count);
        return bl_vm_throwexception(vm, false, "stack overflow");
    }

    b_call_frame* frame = &vm->frames[vm->frame_count++];
    frame->closure = closure;
    frame->ip = closure->fnptr->blob.code;

    frame->slots = vm->stack_top - arg_count - 1;
    return true;
}

static bool call_native_method(b_vm* vm, b_obj_native* native, int arg_count)
{
    if(native->natfn(vm, arg_count, vm->stack_top - arg_count))
    {
        CLEAR_GC();
        vm->stack_top -= arg_count;
        return true;
    }/* else {
    CLEAR_GC();
    bool overridden = AS_BOOL(vm->stack_top[-arg_count - 1]);
    *//*if (!overridden) {
      vm->stack_top -= arg_count + 1;
    }*//*
    return overridden;
  }*/
    return true;
}

bool call_value(b_vm* vm, b_value callee, int arg_count)
{
    if(IS_OBJ(callee))
    {
        switch(OBJ_TYPE(callee))
        {
            case OBJ_BOUND_METHOD:
            {
                b_obj_bound* bound = AS_BOUND(callee);
                vm->stack_top[-arg_count - 1] = bound->receiver;
                return bl_vm_docall(vm, bound->method, arg_count);
            }

            case OBJ_CLASS:
            {
                b_obj_class* klass = AS_CLASS(callee);
                vm->stack_top[-arg_count - 1] = OBJ_VAL(new_instance(vm, klass));
                if(!IS_EMPTY(klass->initializer))
                {
                    return bl_vm_docall(vm, AS_CLOSURE(klass->initializer), arg_count);
                }
                else if(klass->superclass != NULL && !IS_EMPTY(klass->superclass->initializer))
                {
                    return bl_vm_docall(vm, AS_CLOSURE(klass->superclass->initializer), arg_count);
                }
                else if(arg_count != 0)
                {
                    return bl_vm_throwexception(vm, false, "%s constructor expects 0 arguments, %d given", klass->name->chars, arg_count);
                }
                return true;
            }

            case OBJ_MODULE:
            {
                b_obj_module* module = AS_MODULE(callee);
                b_value callable;
                if(table_get(&module->values, STRING_VAL(module->name), &callable))
                {
                    return call_value(vm, callable, arg_count);
                }
            }

            case OBJ_CLOSURE:
            {
                return bl_vm_docall(vm, AS_CLOSURE(callee), arg_count);
            }

            case OBJ_NATIVE:
            {
                return call_native_method(vm, AS_NATIVE(callee), arg_count);
            }

            default:// non callable
                break;
        }
    }

    return bl_vm_throwexception(vm, false, "object of type %s is not callable", value_type(callee));
}

static b_func_type get_method_type(b_value method)
{
    switch(OBJ_TYPE(method))
    {
        case OBJ_NATIVE:
            return AS_NATIVE(method)->type;
        case OBJ_CLOSURE:
            return AS_CLOSURE(method)->fnptr->type;
        default:
            return TYPE_FUNCTION;
    }
}

bool invoke_from_class(b_vm* vm, b_obj_class* klass, b_obj_string* name, int arg_count)
{
    b_value method;
    if(table_get(&klass->methods, OBJ_VAL(name), &method))
    {
        if(get_method_type(method) == TYPE_PRIVATE)
        {
            return bl_vm_throwexception(vm, false, "cannot call private method '%s' from instance of %s", name->chars, klass->name->chars);
        }

        return call_value(vm, method, arg_count);
    }

    return bl_vm_throwexception(vm, false, "undefined method '%s' in %s", name->chars, klass->name->chars);
}

static bool invoke_self(b_vm* vm, b_obj_string* name, int arg_count)
{
    b_value receiver = peek(vm, arg_count);
    b_value value;

    if(IS_INSTANCE(receiver))
    {
        b_obj_instance* instance = AS_INSTANCE(receiver);

        if(table_get(&instance->klass->methods, OBJ_VAL(name), &value))
        {
            return call_value(vm, value, arg_count);
        }

        if(table_get(&instance->properties, OBJ_VAL(name), &value))
        {
            vm->stack_top[-arg_count - 1] = value;
            return call_value(vm, value, arg_count);
        }
    }
    else if(IS_CLASS(receiver))
    {
        if(table_get(&AS_CLASS(receiver)->methods, OBJ_VAL(name), &value))
        {
            if(get_method_type(value) == TYPE_STATIC)
            {
                return call_value(vm, value, arg_count);
            }

            return bl_vm_throwexception(vm, false, "cannot call non-static method %s() on non instance", name->chars);
        }
    }

    return bl_vm_throwexception(vm, false, "cannot call method %s on object of type %s", name->chars, value_type(receiver));
}

static bool invoke(b_vm* vm, b_obj_string* name, int arg_count)
{
    b_value receiver = peek(vm, arg_count);
    b_value value;

    if(!IS_OBJ(receiver))
    {
        // @TODO: have methods for non objects as well.
        return bl_vm_throwexception(vm, false, "non-object %s has no method", value_type(receiver));
    }
    else
    {
        switch(AS_OBJ(receiver)->type)
        {
            case OBJ_MODULE:
            {
                b_obj_module* module = AS_MODULE(receiver);
                if(table_get(&module->values, OBJ_VAL(name), &value))
                {
                    if(name->length > 0 && name->chars[0] == '_')
                    {
                        return bl_vm_throwexception(vm, false, "cannot call private module method '%s'", name->chars);
                    }
                    return call_value(vm, value, arg_count);
                }
                return bl_vm_throwexception(vm, false, "module %s does not define class or method %s()", module->name, name->chars);
                break;
            }
            case OBJ_CLASS:
            {
                if(table_get(&AS_CLASS(receiver)->methods, OBJ_VAL(name), &value))
                {
                    if(get_method_type(value) == TYPE_PRIVATE)
                    {
                        return bl_vm_throwexception(vm, false, "cannot call private method %s() on %s", name->chars, AS_CLASS(receiver)->name->chars);
                    }
                    return call_value(vm, value, arg_count);
                }
                else if(table_get(&AS_CLASS(receiver)->static_properties, OBJ_VAL(name), &value))
                {
                    return call_value(vm, value, arg_count);
                }

                return bl_vm_throwexception(vm, false, "unknown method %s() in class %s", name->chars, AS_CLASS(receiver)->name->chars);
            }
            case OBJ_INSTANCE:
            {
                b_obj_instance* instance = AS_INSTANCE(receiver);

                if(table_get(&instance->properties, OBJ_VAL(name), &value))
                {
                    vm->stack_top[-arg_count - 1] = value;
                    return call_value(vm, value, arg_count);
                }

                return invoke_from_class(vm, instance->klass, name, arg_count);
            }
            case OBJ_STRING:
            {
                if(table_get(&vm->methods_string, OBJ_VAL(name), &value))
                {
                    return call_native_method(vm, AS_NATIVE(value), arg_count);
                }
                return bl_vm_throwexception(vm, false, "String has no method %s()", name->chars);
            }
            case OBJ_LIST:
            {
                if(table_get(&vm->methods_list, OBJ_VAL(name), &value))
                {
                    return call_native_method(vm, AS_NATIVE(value), arg_count);
                }
                return bl_vm_throwexception(vm, false, "List has no method %s()", name->chars);
            }
            case OBJ_RANGE:
            {
                if(table_get(&vm->methods_range, OBJ_VAL(name), &value))
                {
                    return call_native_method(vm, AS_NATIVE(value), arg_count);
                }
                return bl_vm_throwexception(vm, false, "Range has no method %s()", name->chars);
            }
            case OBJ_DICT:
            {
                if(table_get(&vm->methods_dict, OBJ_VAL(name), &value))
                {
                    return call_native_method(vm, AS_NATIVE(value), arg_count);
                }
                return bl_vm_throwexception(vm, false, "Dict has no method %s()", name->chars);
            }
            case OBJ_FILE:
            {
                if(table_get(&vm->methods_file, OBJ_VAL(name), &value))
                {
                    return call_native_method(vm, AS_NATIVE(value), arg_count);
                }
                return bl_vm_throwexception(vm, false, "File has no method %s()", name->chars);
            }
            case OBJ_BYTES:
            {
                if(table_get(&vm->methods_bytes, OBJ_VAL(name), &value))
                {
                    return call_native_method(vm, AS_NATIVE(value), arg_count);
                }
                return bl_vm_throwexception(vm, false, "Bytes has no method %s()", name->chars);
            }
            default:
            {
                return bl_vm_throwexception(vm, false, "cannot call method %s on object of type %s", name->chars, value_type(receiver));
            }
        }
    }
}

static bool bind_method(b_vm* vm, b_obj_class* klass, b_obj_string* name)
{
    b_value method;
    if(table_get(&klass->methods, OBJ_VAL(name), &method))
    {
        if(get_method_type(method) == TYPE_PRIVATE)
        {
            return bl_vm_throwexception(vm, false, "cannot get private property '%s' from instance", name->chars);
        }

        b_obj_bound* bound = new_bound_method(vm, peek(vm, 0), AS_CLOSURE(method));
        pop(vm);
        push(vm, OBJ_VAL(bound));
        return true;
    }

    return bl_vm_throwexception(vm, false, "undefined property '%s'", name->chars);
}

static b_obj_up_value* capture_up_value(b_vm* vm, b_value* local)
{
    b_obj_up_value* prev_up_value = NULL;
    b_obj_up_value* up_value = vm->open_up_values;

    while(up_value != NULL && up_value->location > local)
    {
        prev_up_value = up_value;
        up_value = up_value->next;
    }

    if(up_value != NULL && up_value->location == local)
        return up_value;

    b_obj_up_value* created_up_value = new_up_value(vm, local);
    created_up_value->next = up_value;

    if(prev_up_value == NULL)
    {
        vm->open_up_values = created_up_value;
    }
    else
    {
        prev_up_value->next = created_up_value;
    }

    return created_up_value;
}

static void close_up_values(b_vm* vm, const b_value* last)
{
    while(vm->open_up_values != NULL && vm->open_up_values->location >= last)
    {
        b_obj_up_value* up_value = vm->open_up_values;
        up_value->closed = *up_value->location;
        up_value->location = &up_value->closed;
        vm->open_up_values = up_value->next;
    }
}

static void define_method(b_vm* vm, b_obj_string* name)
{
    b_value method = peek(vm, 0);
    b_obj_class* klass = AS_CLASS(peek(vm, 1));

    table_set(vm, &klass->methods, OBJ_VAL(name), method);
    if(get_method_type(method) == TYPE_INITIALIZER)
    {
        klass->initializer = method;
    }
    pop(vm);
}

static void define_property(b_vm* vm, b_obj_string* name, bool is_static)
{
    b_value property = peek(vm, 0);
    b_obj_class* klass = AS_CLASS(peek(vm, 1));

    if(!is_static)
    {
        table_set(vm, &klass->properties, OBJ_VAL(name), property);
    }
    else
    {
        table_set(vm, &klass->static_properties, OBJ_VAL(name), property);
    }
    pop(vm);
}

bool is_false(b_value value)
{
    if(IS_BOOL(value))
        return IS_BOOL(value) && !AS_BOOL(value);
    if(IS_NIL(value) || IS_EMPTY(value))
        return true;

    // -1 is the number equivalent of false in Blade
    if(IS_NUMBER(value))
        return AS_NUMBER(value) < 0;

    // Non-empty strings are true, empty strings are false.
    if(IS_STRING(value))
        return AS_STRING(value)->length < 1;

    // Non-empty bytes are true, empty strings are false.
    if(IS_BYTES(value))
        return AS_BYTES(value)->bytes.count < 1;

    // Non-empty lists are true, empty lists are false.
    if(IS_LIST(value))
        return AS_LIST(value)->items.count == 0;

    // Non-empty dicts are true, empty dicts are false.
    if(IS_DICT(value))
        return AS_DICT(value)->names.count == 0;

    // All classes are true
    // All closures are true
    // All bound methods are true
    // All functions are in themselves true if you do not account for what they
    // return.
    return false;
}

bool bl_class_isinstanceof(b_obj_class* klass1, char* klass2_name)
{
    while(klass1 != NULL)
    {
        if((int)strlen(klass2_name) == klass1->name->length && memcmp(klass1->name->chars, klass2_name, klass1->name->length) == 0)
        {
            return true;
        }
        klass1 = klass1->superclass;
    }

    return false;
}

static b_obj_string* multiply_string(b_vm* vm, b_obj_string* str, double number)
{
    int times = (int)number;

    if(times <= 0)// 'str' * 0 == '', 'str' * -1 == ''
        return copy_string(vm, "", 0);
    else if(times == 1)// 'str' * 1 == 'str'
        return str;

    int total_length = str->length * times;
    char* result = ALLOCATE(char, (size_t)total_length + 1);

    for(int i = 0; i < times; i++)
    {
        memcpy(result + (str->length * i), str->chars, str->length);
    }
    result[total_length] = '\0';
    return take_string(vm, result, total_length);
}

static b_obj_list* add_list(b_vm* vm, b_obj_list* a, b_obj_list* b)
{
    b_obj_list* list = new_list(vm);
    push(vm, OBJ_VAL(list));

    for(int i = 0; i < a->items.count; i++)
    {
        write_value_arr(vm, &list->items, a->items.values[i]);
    }

    for(int i = 0; i < b->items.count; i++)
    {
        write_value_arr(vm, &list->items, b->items.values[i]);
    }

    pop(vm);
    return list;
}

static b_obj_bytes* add_bytes(b_vm* vm, b_obj_bytes* a, b_obj_bytes* b)
{
    b_obj_bytes* bytes = new_bytes(vm, a->bytes.count + b->bytes.count);

    memcpy(bytes->bytes.bytes, a->bytes.bytes, a->bytes.count);
    memcpy(bytes->bytes.bytes + a->bytes.count, b->bytes.bytes, b->bytes.count);

    return bytes;
}

static void multiply_list(b_vm* vm, b_obj_list* a, b_obj_list* new_list, int times)
{
    for(int i = 0; i < times; i++)
    {
        for(int j = 0; j < a->items.count; j++)
        {
            write_value_arr(vm, &new_list->items, a->items.values[j]);
        }
    }
}

static bool module_get_index(b_vm* vm, b_obj_module* module, bool will_assign)
{
    b_value index = peek(vm, 0);

    b_value result;
    if(table_get(&module->values, index, &result))
    {
        if(!will_assign)
        {
            pop_n(vm, 2);// we can safely get rid of the index from the stack
        }
        push(vm, result);
        return true;
    }

    pop_n(vm, 1);
    return bl_vm_throwexception(vm, false, "%s is undefined in module %s", value_to_string(vm, index), module->name);
}

static bool string_get_index(b_vm* vm, b_obj_string* string, bool will_assign)
{
    b_value lower = peek(vm, 0);

    if(!IS_NUMBER(lower))
    {
        pop_n(vm, 1);
        return bl_vm_throwexception(vm, false, "strings are numerically indexed");
    }

    int index = AS_NUMBER(lower);
    int length = string->is_ascii ? string->length : string->utf8_length;
    int real_index = index;
    if(index < 0)
        index = length + index;

    if(index < length && index >= 0)
    {
        int start = index, end = index + 1;
        if(!string->is_ascii)
        {
            utf8slice(string->chars, &start, &end);
        }

        if(!will_assign)
        {
            // we can safely get rid of the index from the stack
            pop_n(vm, 2);// +1 for the string itself
        }

        push(vm, STRING_L_VAL(string->chars + start, end - start));
        return true;
    }
    else
    {
        pop_n(vm, 1);
        return bl_vm_throwexception(vm, false, "string index %d out of range", real_index);
    }
}

static bool string_get_ranged_index(b_vm* vm, b_obj_string* string, bool will_assign)
{
    b_value upper = peek(vm, 0);
    b_value lower = peek(vm, 1);

    if(!(IS_NIL(lower) || IS_NUMBER(lower)) || !(IS_NUMBER(upper) || IS_NIL(upper)))
    {
        pop_n(vm, 2);
        return bl_vm_throwexception(vm, false, "string are numerically indexed");
    }
    int length = string->is_ascii ? string->length : string->utf8_length;

    int lower_index = IS_NUMBER(lower) ? AS_NUMBER(lower) : 0;
    int upper_index = IS_NIL(upper) ? length : AS_NUMBER(upper);

    if(lower_index < 0 || (upper_index < 0 && ((length + upper_index) < 0)))
    {
        // always return an empty string...
        if(!will_assign)
        {
            pop_n(vm, 3);// +1 for the string itself
        }
        push(vm, STRING_L_VAL("", 0));
        return true;
    }

    if(upper_index < 0)
        upper_index = length + upper_index;

    if(upper_index > length)
        upper_index = length;

    int start = lower_index, end = upper_index;
    if(!string->is_ascii)
    {
        utf8slice(string->chars, &start, &end);
    }

    if(!will_assign)
    {
        pop_n(vm, 3);// +1 for the string itself
    }

    push(vm, STRING_L_VAL(string->chars + start, end - start));
    return true;
}

static bool bytes_get_index(b_vm* vm, b_obj_bytes* bytes, bool will_assign)
{
    b_value lower = peek(vm, 0);

    if(!IS_NUMBER(lower))
    {
        pop_n(vm, 1);
        return bl_vm_throwexception(vm, false, "bytes are numerically indexed");
    }

    int index = AS_NUMBER(lower);
    int real_index = index;
    if(index < 0)
        index = bytes->bytes.count + index;

    if(index < bytes->bytes.count && index >= 0)
    {
        if(!will_assign)
        {
            // we can safely get rid of the index from the stack
            pop_n(vm, 2);// +1 for the bytes itself
        }

        push(vm, NUMBER_VAL((int)bytes->bytes.bytes[index]));
        return true;
    }
    else
    {
        pop_n(vm, 1);
        return bl_vm_throwexception(vm, false, "bytes index %d out of range", real_index);
    }
}

static bool bytes_get_ranged_index(b_vm* vm, b_obj_bytes* bytes, bool will_assign)
{
    b_value upper = peek(vm, 0);
    b_value lower = peek(vm, 1);

    if(!(IS_NIL(lower) || IS_NUMBER(lower)) || !(IS_NUMBER(upper) || IS_NIL(upper)))
    {
        pop_n(vm, 2);
        return bl_vm_throwexception(vm, false, "bytes are numerically indexed");
    }

    int lower_index = IS_NUMBER(lower) ? AS_NUMBER(lower) : 0;
    int upper_index = IS_NIL(upper) ? bytes->bytes.count : AS_NUMBER(upper);

    if(lower_index < 0 || (upper_index < 0 && ((bytes->bytes.count + upper_index) < 0)))
    {
        // always return an empty bytes...
        if(!will_assign)
        {
            pop_n(vm, 3);// +1 for the bytes itself
        }
        push(vm, OBJ_VAL(new_bytes(vm, 0)));
        return true;
    }

    if(upper_index < 0)
        upper_index = bytes->bytes.count + upper_index;

    if(upper_index > bytes->bytes.count)
        upper_index = bytes->bytes.count;

    if(!will_assign)
    {
        pop_n(vm, 3);// +1 for the list itself
    }
    push(vm, OBJ_VAL(copy_bytes(vm, bytes->bytes.bytes + lower_index, upper_index - lower_index)));
    return true;
}

static bool list_get_index(b_vm* vm, b_obj_list* list, bool will_assign)
{
    b_value lower = peek(vm, 0);

    if(!IS_NUMBER(lower))
    {
        pop_n(vm, 1);
        return bl_vm_throwexception(vm, false, "list are numerically indexed");
    }

    int index = AS_NUMBER(lower);
    int real_index = index;
    if(index < 0)
        index = list->items.count + index;

    if(index < list->items.count && index >= 0)
    {
        if(!will_assign)
        {
            // we can safely get rid of the index from the stack
            pop_n(vm, 2);// +1 for the list itself
        }

        push(vm, list->items.values[index]);
        return true;
    }
    else
    {
        pop_n(vm, 1);
        return bl_vm_throwexception(vm, false, "list index %d out of range", real_index);
    }
}

static bool list_get_ranged_index(b_vm* vm, b_obj_list* list, bool will_assign)
{
    b_value upper = peek(vm, 0);
    b_value lower = peek(vm, 1);

    if(!(IS_NIL(lower) || IS_NUMBER(lower)) || !(IS_NUMBER(upper) || IS_NIL(upper)))
    {
        pop_n(vm, 2);
        return bl_vm_throwexception(vm, false, "list are numerically indexed");
    }

    int lower_index = IS_NUMBER(lower) ? AS_NUMBER(lower) : 0;
    int upper_index = IS_NIL(upper) ? list->items.count : AS_NUMBER(upper);

    if(lower_index < 0 || (upper_index < 0 && ((list->items.count + upper_index) < 0)))
    {
        // always return an empty list...
        if(!will_assign)
        {
            pop_n(vm, 3);// +1 for the list itself
        }
        push(vm, OBJ_VAL(new_list(vm)));
        return true;
    }

    if(upper_index < 0)
        upper_index = list->items.count + upper_index;

    if(upper_index > list->items.count)
        upper_index = list->items.count;

    b_obj_list* n_list = new_list(vm);
    push(vm, OBJ_VAL(n_list));// gc protect

    for(int i = lower_index; i < upper_index; i++)
    {
        write_value_arr(vm, &n_list->items, list->items.values[i]);
    }
    pop(vm);// clear gc protect

    if(!will_assign)
    {
        pop_n(vm, 3);// +1 for the list itself
    }
    push(vm, OBJ_VAL(n_list));
    return true;
}

static void module_set_index(b_vm* vm, b_obj_module* module, b_value index, b_value value)
{
    table_set(vm, &module->values, index, value);
    pop_n(vm, 3);// pop the value, index and dict out

    // leave the value on the stack for consumption
    // e.g. variable = dict[index] = 10
    push(vm, value);
}

static bool list_set_index(b_vm* vm, b_obj_list* list, b_value index, b_value value)
{
    if(!IS_NUMBER(index))
    {
        pop_n(vm, 3);// pop the value, index and list out
        return bl_vm_throwexception(vm, false, "list are numerically indexed");
    }

    int _position = AS_NUMBER(index);
    int position = _position < 0 ? list->items.count + _position : _position;

    if(position < list->items.count && position > -(list->items.count))
    {
        list->items.values[position] = value;
        pop_n(vm, 3);// pop the value, index and list out

        // leave the value on the stack for consumption
        // e.g. variable = list[index] = 10
        push(vm, value);
        return true;
    }

    pop_n(vm, 3);// pop the value, index and list out
    return bl_vm_throwexception(vm, false, "lists index %d out of range", _position);
}

static bool bytes_set_index(b_vm* vm, b_obj_bytes* bytes, b_value index, b_value value)
{
    if(!IS_NUMBER(index))
    {
        pop_n(vm, 3);// pop the value, index and bytes out
        return bl_vm_throwexception(vm, false, "bytes are numerically indexed");
    }
    else if(!IS_NUMBER(value) || AS_NUMBER(value) < 0 || AS_NUMBER(value) > 255)
    {
        pop_n(vm, 3);// pop the value, index and bytes out
        return bl_vm_throwexception(vm, false, "invalid byte. bytes are numbers between 0 and 255.");
    }

    int _position = AS_NUMBER(index);
    int byte = AS_NUMBER(value);

    int position = _position < 0 ? bytes->bytes.count + _position : _position;

    if(position < bytes->bytes.count && position > -(bytes->bytes.count))
    {
        bytes->bytes.bytes[position] = (unsigned char)byte;
        pop_n(vm, 3);// pop the value, index and bytes out

        // leave the value on the stack for consumption
        // e.g. variable = bytes[index] = 10
        push(vm, value);
        return true;
    }

    pop_n(vm, 3);// pop the value, index and bytes out
    return bl_vm_throwexception(vm, false, "bytes index %d out of range", _position);
}

static bool concatenate(b_vm* vm)
{
    b_value _b = peek(vm, 0);
    b_value _a = peek(vm, 1);

    if(IS_NIL(_a))
    {
        pop_n(vm, 2);
        push(vm, _b);
    }
    else if(IS_NIL(_b))
    {
        pop(vm);
    }
    else if(IS_NUMBER(_a))
    {
        double a = AS_NUMBER(_a);

        char num_str[27];// + 1 for null terminator
        int num_length = sprintf(num_str, NUMBER_FORMAT, a);

        b_obj_string* b = AS_STRING(_b);

        int length = num_length + b->length;
        char* chars = ALLOCATE(char, (size_t)length + 1);
        memcpy(chars, num_str, num_length);
        memcpy(chars + num_length, b->chars, b->length);
        chars[length] = '\0';

        b_obj_string* result = take_string(vm, chars, length);
        result->utf8_length = num_length + b->utf8_length;

        pop_n(vm, 2);
        push(vm, OBJ_VAL(result));
    }
    else if(IS_NUMBER(_b))
    {
        b_obj_string* a = AS_STRING(_a);
        double b = AS_NUMBER(_b);

        char num_str[27];// + 1 for null terminator
        int num_length = sprintf(num_str, NUMBER_FORMAT, b);

        int length = num_length + a->length;
        char* chars = ALLOCATE(char, (size_t)length + 1);
        memcpy(chars, a->chars, a->length);
        memcpy(chars + a->length, num_str, num_length);
        chars[length] = '\0';

        b_obj_string* result = take_string(vm, chars, length);
        result->utf8_length = num_length + a->utf8_length;

        pop_n(vm, 2);
        push(vm, OBJ_VAL(result));
    }
    else if(IS_STRING(_a) && IS_STRING(_b))
    {
        b_obj_string* b = AS_STRING(_b);
        b_obj_string* a = AS_STRING(_a);

        int length = a->length + b->length;
        char* chars = ALLOCATE(char, length + 1);
        memcpy(chars, a->chars, a->length);
        memcpy(chars + a->length, b->chars, b->length);
        chars[length] = '\0';

        b_obj_string* result = take_string(vm, chars, length);
        result->utf8_length = a->utf8_length + b->utf8_length;

        pop_n(vm, 2);
        push(vm, OBJ_VAL(result));
    }
    else
    {
        return false;
    }

    return true;
}

static int floor_div(double a, double b)
{
    int d = (int)a / (int)b;
    return d - ((d * b == a) & ((a < 0) ^ (b < 0)));
}

static double modulo(double a, double b)
{
    double r = fmod(a, b);
    if(r != 0 && ((r < 0) != (b < 0)))
    {
        r += b;
    }
    return r;
}

b_ptr_result bl_vm_run(b_vm* vm)
{
    b_call_frame* frame = &vm->frames[vm->frame_count - 1];

#define READ_BYTE() (*frame->ip++)

#define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))

#define READ_CONSTANT() (frame->closure->fnptr->blob.constants.values[READ_SHORT()])

#define READ_STRING() (AS_STRING(READ_CONSTANT()))

#define BINARY_OP(type, op) \
    do \
    { \
        if((!IS_NUMBER(peek(vm, 0)) && !IS_BOOL(peek(vm, 0))) || (!IS_NUMBER(peek(vm, 1)) && !IS_BOOL(peek(vm, 1)))) \
        { \
            runtime_error("unsupported operand %s for %s and %s", #op, value_type(peek(vm, 0)), value_type(peek(vm, 1))); \
            break; \
        } \
        b_value _b = pop(vm); \
        double b = IS_BOOL(_b) ? (AS_BOOL(_b) ? 1 : 0) : AS_NUMBER(_b); \
        b_value _a = pop(vm); \
        double a = IS_BOOL(_a) ? (AS_BOOL(_a) ? 1 : 0) : AS_NUMBER(_a); \
        push(vm, type(a op b)); \
    } while(false)

#define BINARY_BIT_OP(op) \
    do \
    { \
        if((!IS_NUMBER(peek(vm, 0)) && !IS_BOOL(peek(vm, 0))) || (!IS_NUMBER(peek(vm, 1)) && !IS_BOOL(peek(vm, 1)))) \
        { \
            runtime_error("unsupported operand %s for %s and %s", #op, value_type(peek(vm, 0)), value_type(peek(vm, 1))); \
            break; \
        } \
        long b = AS_NUMBER(pop(vm)); \
        long a = AS_NUMBER(pop(vm)); \
        push(vm, INTEGER_VAL(a op b)); \
    } while(false)

#define BINARY_MOD_OP(type, op) \
    do \
    { \
        if((!IS_NUMBER(peek(vm, 0)) && !IS_BOOL(peek(vm, 0))) || (!IS_NUMBER(peek(vm, 1)) && !IS_BOOL(peek(vm, 1)))) \
        { \
            runtime_error("unsupported operand %s for %s and %s", #op, value_type(peek(vm, 0)), value_type(peek(vm, 1))); \
            break; \
        } \
        b_value _b = pop(vm); \
        double b = IS_BOOL(_b) ? (AS_BOOL(_b) ? 1 : 0) : AS_NUMBER(_b); \
        b_value _a = pop(vm); \
        double a = IS_BOOL(_a) ? (AS_BOOL(_a) ? 1 : 0) : AS_NUMBER(_a); \
        push(vm, type(op(a, b))); \
    } while(false)

    for(;;)
    {
        // try...finally... (i.e. try without a catch but a finally
        // but whose try body raises an exception)
        // can cause us to go into an invalid mode where frame count == 0
        // to fix this, we need to exit with an appropriate mode here.
        if(vm->frame_count == 0)
        {
            return PTR_RUNTIME_ERR;
        }

        if(vm->should_debug_stack)
        {
            printf("          ");
            for(b_value* slot = vm->stack; slot < vm->stack_top; slot++)
            {
                printf("[ ");
                print_value(*slot);
                printf(" ]");
            }
            printf("\n");
            disassemble_instruction(&frame->closure->fnptr->blob, (int)(frame->ip - frame->closure->fnptr->blob.code));
        }

        uint8_t instruction;

        switch(instruction = READ_BYTE())
        {
            case OP_CONSTANT:
            {
                b_value constant = READ_CONSTANT();
                push(vm, constant);
                break;
            }

            case OP_ADD:
            {
                if(IS_STRING(peek(vm, 0)) || IS_STRING(peek(vm, 1)))
                {
                    if(!concatenate(vm))
                    {
                        runtime_error("unsupported operand + for %s and %s", value_type(peek(vm, 0)), value_type(peek(vm, 1)));
                        break;
                    }
                }
                else if(IS_LIST(peek(vm, 0)) && IS_LIST(peek(vm, 1)))
                {
                    b_value result = OBJ_VAL(add_list(vm, AS_LIST(peek(vm, 1)), AS_LIST(peek(vm, 0))));
                    pop_n(vm, 2);
                    push(vm, result);
                }
                else if(IS_BYTES(peek(vm, 0)) && IS_BYTES(peek(vm, 1)))
                {
                    b_value result = OBJ_VAL(add_bytes(vm, AS_BYTES(peek(vm, 1)), AS_BYTES(peek(vm, 0))));
                    pop_n(vm, 2);
                    push(vm, result);
                }
                else
                {
                    BINARY_OP(NUMBER_VAL, +);
                }
                break;
            }
            case OP_SUBTRACT:
            {
                BINARY_OP(NUMBER_VAL, -);
                break;
            }
            case OP_MULTIPLY:
            {
                if(IS_STRING(peek(vm, 1)) && IS_NUMBER(peek(vm, 0)))
                {
                    double number = AS_NUMBER(peek(vm, 0));
                    b_obj_string* string = AS_STRING(peek(vm, 1));
                    b_value result = OBJ_VAL(multiply_string(vm, string, number));
                    pop_n(vm, 2);
                    push(vm, result);
                    break;
                }
                else if(IS_LIST(peek(vm, 1)) && IS_NUMBER(peek(vm, 0)))
                {
                    int number = (int)AS_NUMBER(pop(vm));
                    b_obj_list* list = AS_LIST(peek(vm, 0));
                    b_obj_list* n_list = new_list(vm);
                    push(vm, OBJ_VAL(n_list));
                    multiply_list(vm, list, n_list, number);
                    pop_n(vm, 2);
                    push(vm, OBJ_VAL(n_list));
                    break;
                }
                BINARY_OP(NUMBER_VAL, *);
                break;
            }
            case OP_DIVIDE:
            {
                BINARY_OP(NUMBER_VAL, /);
                break;
            }
            case OP_REMINDER:
            {
                BINARY_MOD_OP(NUMBER_VAL, modulo);
                break;
            }
            case OP_POW:
            {
                BINARY_MOD_OP(NUMBER_VAL, pow);
                break;
            }
            case OP_F_DIVIDE:
            {
                BINARY_MOD_OP(NUMBER_VAL, floor_div);
                break;
            }
            case OP_NEGATE:
            {
                if(!IS_NUMBER(peek(vm, 0)))
                {
                    runtime_error("operator - not defined for object of type %s", value_type(peek(vm, 0)));
                    break;
                }
                push(vm, NUMBER_VAL(-AS_NUMBER(pop(vm))));
                break;
            }
            case OP_BIT_NOT:
            {
                if(!IS_NUMBER(peek(vm, 0)))
                {
                    runtime_error("operator ~ not defined for object of type %s", value_type(peek(vm, 0)));
                    break;
                }
                push(vm, INTEGER_VAL(~((int)AS_NUMBER(pop(vm)))));
                break;
            }
            case OP_AND:
            {
                BINARY_BIT_OP(&);
                break;
            }
            case OP_OR:
            {
                BINARY_BIT_OP(|);
                break;
            }
            case OP_XOR:
            {
                BINARY_BIT_OP(^);
                break;
            }
            case OP_LSHIFT:
            {
                BINARY_BIT_OP(<<);
                break;
            }
            case OP_RSHIFT:
            {
                BINARY_BIT_OP(>>);
                break;
            }
            case OP_ONE:
            {
                push(vm, NUMBER_VAL(1));
                break;
            }

                // comparisons
            case OP_EQUAL:
            {
                b_value b = pop(vm);
                b_value a = pop(vm);
                push(vm, BOOL_VAL(values_equal(a, b)));
                break;
            }
            case OP_GREATER:
            {
                BINARY_OP(BOOL_VAL, >);
                break;
            }
            case OP_LESS:
            {
                BINARY_OP(BOOL_VAL, <);
                break;
            }

            case OP_NOT:
                push(vm, BOOL_VAL(is_false(pop(vm))));
                break;
            case OP_NIL:
                push(vm, NIL_VAL);
                break;
            case OP_EMPTY:
                push(vm, EMPTY_VAL);
                break;
            case OP_TRUE:
                push(vm, BOOL_VAL(true));
                break;
            case OP_FALSE:
                push(vm, BOOL_VAL(false));
                break;

            case OP_JUMP:
            {
                uint16_t offset = READ_SHORT();
                frame->ip += offset;
                break;
            }
            case OP_JUMP_IF_FALSE:
            {
                uint16_t offset = READ_SHORT();
                if(is_false(peek(vm, 0)))
                {
                    frame->ip += offset;
                }
                break;
            }
            case OP_LOOP:
            {
                uint16_t offset = READ_SHORT();
                frame->ip -= offset;
                break;
            }

            case OP_ECHO:
            {
                b_value val = peek(vm, 0);
                if(vm->is_repl)
                {
                    echo_value(val);
                }
                else
                {
                    print_value(val);
                }
                if(!IS_EMPTY(val))
                {
                    printf("\n");
                }
                pop(vm);
                break;
            }

            case OP_STRINGIFY:
            {
                if(!IS_STRING(peek(vm, 0)) && !IS_NIL(peek(vm, 0)))
                {
                    char* value = value_to_string(vm, pop(vm));
                    if((int)strlen(value) != 0)
                    {
                        push(vm, STRING_TT_VAL(value));
                    }
                    else
                    {
                        push(vm, NIL_VAL);
                    }
                }
                break;
            }

            case OP_DUP:
            {
                push(vm, peek(vm, 0));
                break;
            }
            case OP_POP:
            {
                pop(vm);
                break;
            }
            case OP_POP_N:
            {
                pop_n(vm, READ_SHORT());
                break;
            }
            case OP_CLOSE_UP_VALUE:
            {
                close_up_values(vm, vm->stack_top - 1);
                pop(vm);
                break;
            }

            case OP_DEFINE_GLOBAL:
            {
                b_obj_string* name = READ_STRING();
                if(IS_EMPTY(peek(vm, 0)))
                {
                    runtime_error(ERR_CANT_ASSIGN_EMPTY);
                    break;
                }
                table_set(vm, &frame->closure->fnptr->module->values, OBJ_VAL(name), peek(vm, 0));
                pop(vm);

#if defined(DEBUG_TABLE) && DEBUG_TABLE
                table_print(&vm->globals);
#endif
                break;
            }

            case OP_GET_GLOBAL:
            {
                b_obj_string* name = READ_STRING();
                b_value value;
                if(!table_get(&frame->closure->fnptr->module->values, OBJ_VAL(name), &value))
                {
                    if(!table_get(&vm->globals, OBJ_VAL(name), &value))
                    {
                        runtime_error("'%s' is undefined in this scope", name->chars);
                        break;
                    }
                }
                push(vm, value);
                break;
            }

            case OP_SET_GLOBAL:
            {
                if(IS_EMPTY(peek(vm, 0)))
                {
                    runtime_error(ERR_CANT_ASSIGN_EMPTY);
                    break;
                }

                b_obj_string* name = READ_STRING();
                b_table* table = &frame->closure->fnptr->module->values;
                if(table_set(vm, table, OBJ_VAL(name), peek(vm, 0)))
                {
                    table_delete(table, OBJ_VAL(name));
                    runtime_error("%s is undefined in this scope", name->chars);
                    break;
                }
                break;
            }

            case OP_GET_LOCAL:
            {
                uint16_t slot = READ_SHORT();
                push(vm, frame->slots[slot]);
                break;
            }
            case OP_SET_LOCAL:
            {
                uint16_t slot = READ_SHORT();
                if(IS_EMPTY(peek(vm, 0)))
                {
                    runtime_error(ERR_CANT_ASSIGN_EMPTY);
                    break;
                }
                frame->slots[slot] = peek(vm, 0);
                break;
            }

            case OP_GET_PROPERTY:
            {
                b_obj_string* name = READ_STRING();

                if(IS_OBJ(peek(vm, 0)))
                {
                    b_value value;

                    switch(AS_OBJ(peek(vm, 0))->type)
                    {
                        case OBJ_MODULE:
                        {
                            b_obj_module* module = AS_MODULE(peek(vm, 0));
                            if(table_get(&module->values, OBJ_VAL(name), &value))
                            {
                                if(name->length > 0 && name->chars[0] == '_')
                                {
                                    runtime_error("cannot get private module property '%s'", name->chars);
                                    break;
                                }

                                pop(vm);// pop the list...
                                push(vm, value);
                                break;
                            }

                            runtime_error("%s module does not define '%s'", module->name, name->chars);
                            break;
                        }
                        case OBJ_CLASS:
                        {
                            if(table_get(&AS_CLASS(peek(vm, 0))->methods, OBJ_VAL(name), &value))
                            {
                                if(get_method_type(value) == TYPE_STATIC)
                                {
                                    if(name->length > 0 && name->chars[0] == '_')
                                    {
                                        runtime_error("cannot call private property '%s' of class %s", name->chars, AS_CLASS(peek(vm, 0))->name->chars);
                                        break;
                                    }
                                    pop(vm);// pop the class...
                                    push(vm, value);
                                    break;
                                }
                            }
                            else if(table_get(&AS_CLASS(peek(vm, 0))->static_properties, OBJ_VAL(name), &value))
                            {
                                if(name->length > 0 && name->chars[0] == '_')
                                {
                                    runtime_error("cannot call private property '%s' of class %s", name->chars, AS_CLASS(peek(vm, 0))->name->chars);
                                    break;
                                }
                                pop(vm);// pop the class...
                                push(vm, value);
                                break;
                            }

                            runtime_error("class %s does not have a static property or method named '%s'", AS_CLASS(peek(vm, 0))->name->chars, name->chars);
                            break;
                        }
                        case OBJ_INSTANCE:
                        {
                            b_obj_instance* instance = AS_INSTANCE(peek(vm, 0));
                            if(table_get(&instance->properties, OBJ_VAL(name), &value))
                            {
                                if(name->length > 0 && name->chars[0] == '_')
                                {
                                    runtime_error("cannot call private property '%s' from instance of %s", name->chars, instance->klass->name->chars);
                                    break;
                                }
                                pop(vm);// pop the instance...
                                push(vm, value);
                                break;
                            }

                            if(name->length > 0 && name->chars[0] == '_')
                            {
                                runtime_error("cannot bind private property '%s' to instance of %s", name->chars, instance->klass->name->chars);
                                break;
                            }

                            if(bind_method(vm, instance->klass, name))
                            {
                                break;
                            }

                            runtime_error("instance of class %s does not have a property or method named '%s'", AS_INSTANCE(peek(vm, 0))->klass->name->chars,
                                          name->chars);
                            break;
                        }
                        case OBJ_STRING:
                        {
                            if(table_get(&vm->methods_string, OBJ_VAL(name), &value))
                            {
                                pop(vm);// pop the list...
                                push(vm, value);
                                break;
                            }

                            runtime_error("class String has no named property '%s'", name->chars);
                            break;
                        }
                        case OBJ_LIST:
                        {
                            if(table_get(&vm->methods_list, OBJ_VAL(name), &value))
                            {
                                pop(vm);// pop the list...
                                push(vm, value);
                                break;
                            }

                            runtime_error("class List has no named property '%s'", name->chars);
                            break;
                        }
                        case OBJ_RANGE:
                        {
                            if(table_get(&vm->methods_range, OBJ_VAL(name), &value))
                            {
                                pop(vm);// pop the list...
                                push(vm, value);
                                break;
                            }

                            runtime_error("class Range has no named property '%s'", name->chars);
                            break;
                        }
                        case OBJ_DICT:
                        {
                            if(table_get(&AS_DICT(peek(vm, 0))->items, OBJ_VAL(name), &value) || table_get(&vm->methods_dict, OBJ_VAL(name), &value))
                            {
                                pop(vm);// pop the dictionary...
                                push(vm, value);
                                break;
                            }

                            runtime_error("unknown key or class Dict property '%s'", name->chars);
                            break;
                        }
                        case OBJ_BYTES:
                        {
                            if(table_get(&vm->methods_bytes, OBJ_VAL(name), &value))
                            {
                                pop(vm);// pop the list...
                                push(vm, value);
                                break;
                            }

                            runtime_error("class Bytes has no named property '%s'", name->chars);
                            break;
                        }
                        case OBJ_FILE:
                        {
                            if(table_get(&vm->methods_file, OBJ_VAL(name), &value))
                            {
                                pop(vm);// pop the list...
                                push(vm, value);
                                break;
                            }

                            runtime_error("class File has no named property '%s'", name->chars);
                            break;
                        }
                        default:
                        {
                            runtime_error("object of type %s does not carry properties", value_type(peek(vm, 0)));
                            break;
                        }
                    }
                }
                else
                {
                    runtime_error("'%s' of type %s does not have properties", value_to_string(vm, peek(vm, 0)), value_type(peek(vm, 0)));
                    break;
                }
                break;
            }

            case OP_GET_SELF_PROPERTY:
            {
                b_obj_string* name = READ_STRING();
                b_value value;

                if(IS_INSTANCE(peek(vm, 0)))
                {
                    b_obj_instance* instance = AS_INSTANCE(peek(vm, 0));
                    if(table_get(&instance->properties, OBJ_VAL(name), &value))
                    {
                        pop(vm);// pop the instance...
                        push(vm, value);
                        break;
                    }

                    if(bind_method(vm, instance->klass, name))
                    {
                        break;
                    }

                    runtime_error("instance of class %s does not have a property or method named '%s'", AS_INSTANCE(peek(vm, 0))->klass->name->chars, name->chars);
                    break;
                }
                else if(IS_CLASS(peek(vm, 0)))
                {
                    b_obj_class* klass = AS_CLASS(peek(vm, 0));
                    if(table_get(&klass->methods, OBJ_VAL(name), &value))
                    {
                        if(get_method_type(value) == TYPE_STATIC)
                        {
                            pop(vm);// pop the class...
                            push(vm, value);
                            break;
                        }
                    }
                    else if(table_get(&klass->static_properties, OBJ_VAL(name), &value))
                    {
                        pop(vm);// pop the class...
                        push(vm, value);
                        break;
                    }
                    runtime_error("class %s does not have a static property or method named '%s'", klass->name->chars, name->chars);
                    break;
                }
                else if(IS_MODULE(peek(vm, 0)))
                {
                    b_obj_module* module = AS_MODULE(peek(vm, 0));
                    if(table_get(&module->values, OBJ_VAL(name), &value))
                    {
                        pop(vm);// pop the class...
                        push(vm, value);
                        break;
                    }

                    runtime_error("module %s does not define '%s'", module->name, name->chars);
                    break;
                }

                runtime_error("'%s' of type %s does not have properties", value_to_string(vm, peek(vm, 0)), value_type(peek(vm, 0)));
                break;
            }

            case OP_SET_PROPERTY:
            {
                if(!IS_INSTANCE(peek(vm, 1)) && !IS_DICT(peek(vm, 1)))
                {
                    runtime_error("object of type %s can not carry properties", value_type(peek(vm, 1)));
                    break;
                }
                else if(IS_EMPTY(peek(vm, 0)))
                {
                    runtime_error(ERR_CANT_ASSIGN_EMPTY);
                    break;
                }

                b_obj_string* name = READ_STRING();

                if(IS_INSTANCE(peek(vm, 1)))
                {
                    b_obj_instance* instance = AS_INSTANCE(peek(vm, 1));
                    table_set(vm, &instance->properties, OBJ_VAL(name), peek(vm, 0));

                    b_value value = pop(vm);
                    pop(vm);// removing the instance object
                    push(vm, value);
                }
                else
                {
                    b_obj_dict* dict = AS_DICT(peek(vm, 1));
                    dict_set_entry(vm, dict, OBJ_VAL(name), peek(vm, 0));

                    b_value value = pop(vm);
                    pop(vm);// removing the dictionary object
                    push(vm, value);
                }
                break;
            }

            case OP_CLOSURE:
            {
                b_obj_func* function = AS_FUNCTION(READ_CONSTANT());
                b_obj_closure* closure = new_closure(vm, function);
                push(vm, OBJ_VAL(closure));

                for(int i = 0; i < closure->up_value_count; i++)
                {
                    uint8_t is_local = READ_BYTE();
                    int index = READ_SHORT();

                    if(is_local)
                    {
                        closure->up_values[i] = capture_up_value(vm, frame->slots + index);
                    }
                    else
                    {
                        closure->up_values[i] = ((b_obj_closure*)frame->closure)->up_values[index];
                    }
                }

                break;
            }
            case OP_GET_UP_VALUE:
            {
                int index = READ_SHORT();
                push(vm, *((b_obj_closure*)frame->closure)->up_values[index]->location);
                break;
            }
            case OP_SET_UP_VALUE:
            {
                int index = READ_SHORT();
                if(IS_EMPTY(peek(vm, 0)))
                {
                    runtime_error(ERR_CANT_ASSIGN_EMPTY);
                    break;
                }
                *((b_obj_closure*)frame->closure)->up_values[index]->location = peek(vm, 0);
                break;
            }

            case OP_CALL:
            {
                int arg_count = READ_BYTE();
                if(!call_value(vm, peek(vm, arg_count), arg_count))
                {
                    EXIT_VM();
                }
                frame = &vm->frames[vm->frame_count - 1];
                break;
            }
            case OP_INVOKE:
            {
                b_obj_string* method = READ_STRING();
                int arg_count = READ_BYTE();
                if(!invoke(vm, method, arg_count))
                {
                    EXIT_VM();
                }
                frame = &vm->frames[vm->frame_count - 1];
                break;
            }
            case OP_INVOKE_SELF:
            {
                b_obj_string* method = READ_STRING();
                int arg_count = READ_BYTE();
                if(!invoke_self(vm, method, arg_count))
                {
                    EXIT_VM();
                }
                frame = &vm->frames[vm->frame_count - 1];
                break;
            }

            case OP_CLASS:
            {
                b_obj_string* name = READ_STRING();
                push(vm, OBJ_VAL(new_class(vm, name)));
                break;
            }
            case OP_METHOD:
            {
                b_obj_string* name = READ_STRING();
                define_method(vm, name);
                break;
            }
            case OP_CLASS_PROPERTY:
            {
                b_obj_string* name = READ_STRING();
                int is_static = READ_BYTE();
                define_property(vm, name, is_static == 1);
                break;
            }
            case OP_INHERIT:
            {
                if(!IS_CLASS(peek(vm, 1)))
                {
                    runtime_error("cannot inherit from non-class object");
                    break;
                }

                b_obj_class* superclass = AS_CLASS(peek(vm, 1));
                b_obj_class* subclass = AS_CLASS(peek(vm, 0));
                table_add_all(vm, &superclass->properties, &subclass->properties);
                table_add_all(vm, &superclass->methods, &subclass->methods);
                subclass->superclass = superclass;
                pop(vm);// pop the subclass
                break;
            }
            case OP_GET_SUPER:
            {
                b_obj_string* name = READ_STRING();
                b_obj_class* klass = AS_CLASS(peek(vm, 0));
                if(!bind_method(vm, klass->superclass, name))
                {
                    runtime_error("class %s does not define a function %s", klass->name->chars, name->chars);
                }
                break;
            }
            case OP_SUPER_INVOKE:
            {
                b_obj_string* method = READ_STRING();
                int arg_count = READ_BYTE();
                b_obj_class* klass = AS_CLASS(pop(vm));
                if(!invoke_from_class(vm, klass, method, arg_count))
                {
                    EXIT_VM();
                }
                frame = &vm->frames[vm->frame_count - 1];
                break;
            }
            case OP_SUPER_INVOKE_SELF:
            {
                int arg_count = READ_BYTE();
                b_obj_class* klass = AS_CLASS(pop(vm));
                if(!invoke_from_class(vm, klass, klass->name, arg_count))
                {
                    EXIT_VM();
                }
                frame = &vm->frames[vm->frame_count - 1];
                break;
            }

            case OP_LIST:
            {
                int count = READ_SHORT();
                b_obj_list* list = new_list(vm);
                vm->stack_top[-count - 1] = OBJ_VAL(list);

                for(int i = count - 1; i >= 0; i--)
                {
                    write_list(vm, list, peek(vm, i));
                }
                pop_n(vm, count);
                break;
            }
            case OP_RANGE:
            {
                b_value _upper = peek(vm, 0), _lower = peek(vm, 1);

                if(!IS_NUMBER(_upper) || !IS_NUMBER(_lower))
                {
                    runtime_error("invalid range boundaries");
                    break;
                }

                double lower = AS_NUMBER(_lower), upper = AS_NUMBER(_upper);
                pop_n(vm, 2);
                push(vm, OBJ_VAL(new_range(vm, lower, upper)));
                break;
            }
            case OP_DICT:
            {
                int count = READ_SHORT() * 2;// 1 for key, 1 for value
                b_obj_dict* dict = new_dict(vm);
                vm->stack_top[-count - 1] = OBJ_VAL(dict);

                for(int i = 0; i < count; i += 2)
                {
                    b_value name = vm->stack_top[-count + i];
                    if(!IS_STRING(name) && !IS_NUMBER(name) && !IS_BOOL(name))
                    {
                        runtime_error("dictionary key must be one of string, number or boolean");
                    }
                    b_value value = vm->stack_top[-count + i + 1];
                    dict_add_entry(vm, dict, name, value);
                }
                pop_n(vm, count);
                break;
            }

            case OP_GET_RANGED_INDEX:
            {
                uint8_t will_assign = READ_BYTE();

                bool is_gotten = true;
                if(IS_OBJ(peek(vm, 2)))
                {
                    switch(AS_OBJ(peek(vm, 2))->type)
                    {
                        case OBJ_STRING:
                        {
                            if(!string_get_ranged_index(vm, AS_STRING(peek(vm, 2)), will_assign == (uint8_t)1))
                            {
                                EXIT_VM();
                            }
                            break;
                        }
                        case OBJ_LIST:
                        {
                            if(!list_get_ranged_index(vm, AS_LIST(peek(vm, 2)), will_assign == (uint8_t)1))
                            {
                                EXIT_VM();
                            }
                            break;
                        }
                        case OBJ_BYTES:
                        {
                            if(!bytes_get_ranged_index(vm, AS_BYTES(peek(vm, 2)), will_assign == (uint8_t)1))
                            {
                                EXIT_VM();
                            }
                            break;
                        }
                        default:
                        {
                            is_gotten = false;
                            break;
                        }
                    }
                }
                else
                {
                    is_gotten = false;
                }

                if(!is_gotten)
                {
                    runtime_error("cannot range index object of type %s", value_type(peek(vm, 2)));
                }
                break;
            }
            case OP_GET_INDEX:
            {
                uint8_t will_assign = READ_BYTE();

                bool is_gotten = true;
                if(IS_OBJ(peek(vm, 1)))
                {
                    switch(AS_OBJ(peek(vm, 1))->type)
                    {
                        case OBJ_STRING:
                        {
                            if(!string_get_index(vm, AS_STRING(peek(vm, 1)), will_assign == (uint8_t)1))
                            {
                                EXIT_VM();
                            }
                            break;
                        }
                        case OBJ_LIST:
                        {
                            if(!list_get_index(vm, AS_LIST(peek(vm, 1)), will_assign == (uint8_t)1))
                            {
                                EXIT_VM();
                            }
                            break;
                        }
                        case OBJ_DICT:
                        {
                            if(!bl_vmdo_dictgetindex(vm, AS_DICT(peek(vm, 1)), will_assign == (uint8_t)1))
                            {
                                EXIT_VM();
                            }
                            break;
                        }
                        case OBJ_MODULE:
                        {
                            if(!module_get_index(vm, AS_MODULE(peek(vm, 1)), will_assign == (uint8_t)1))
                            {
                                EXIT_VM();
                            }
                            break;
                        }
                        case OBJ_BYTES:
                        {
                            if(!bytes_get_index(vm, AS_BYTES(peek(vm, 1)), will_assign == (uint8_t)1))
                            {
                                EXIT_VM();
                            }
                            break;
                        }
                        default:
                        {
                            is_gotten = false;
                            break;
                        }
                    }
                }
                else
                {
                    is_gotten = false;
                }

                if(!is_gotten)
                {
                    runtime_error("cannot index object of type %s", value_type(peek(vm, 1)));
                }
                break;
            }

            case OP_SET_INDEX:
            {
                bool is_set = true;
                if(IS_OBJ(peek(vm, 2)))
                {
                    b_value value = peek(vm, 0);
                    b_value index = peek(vm, 1);

                    if(IS_EMPTY(value))
                    {
                        runtime_error(ERR_CANT_ASSIGN_EMPTY);
                        break;
                    }

                    switch(AS_OBJ(peek(vm, 2))->type)
                    {
                        case OBJ_LIST:
                        {
                            if(!list_set_index(vm, AS_LIST(peek(vm, 2)), index, value))
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
                            bl_vmdo_dictsetindex(vm, AS_DICT(peek(vm, 2)), index, value);
                            break;
                        }
                        case OBJ_MODULE:
                        {
                            module_set_index(vm, AS_MODULE(peek(vm, 2)), index, value);
                            break;
                        }
                        case OBJ_BYTES:
                        {
                            if(!bytes_set_index(vm, AS_BYTES(peek(vm, 2)), index, value))
                            {
                                EXIT_VM();
                            }
                            break;
                        }
                        default:
                        {
                            is_set = false;
                            break;
                        }
                    }
                }
                else
                {
                    is_set = false;
                }

                if(!is_set)
                {
                    runtime_error("type of %s is not a valid iterable", value_type(peek(vm, 3)));
                }
                break;
            }

            case OP_RETURN:
            {
                b_value result = pop(vm);

                close_up_values(vm, frame->slots);

                vm->frame_count--;
                if(vm->frame_count == 0)
                {
                    pop(vm);
                    return PTR_OK;
                }

                vm->stack_top = frame->slots;
                push(vm, result);

                frame = &vm->frames[vm->frame_count - 1];
                break;
            }

            case OP_CALL_IMPORT:
            {
                b_obj_closure* closure = AS_CLOSURE(READ_CONSTANT());
                add_module(vm, closure->fnptr->module);
                bl_vm_docall(vm, closure, 0);
                frame = &vm->frames[vm->frame_count - 1];
                break;
            }

            case OP_NATIVE_MODULE:
            {
                b_obj_string* module_name = READ_STRING();
                b_value value;
                if(table_get(&vm->modules, OBJ_VAL(module_name), &value))
                {
                    b_obj_module* module = AS_MODULE(value);
                    if(module->preloader != NULL)
                    {
                        ((b_module_loader)module->preloader)(vm);
                    }
                    module->imported = true;
                    table_set(vm, &frame->closure->fnptr->module->values, OBJ_VAL(module_name), value);
                    break;
                }
                runtime_error("module '%s' not found", module_name->chars);
                break;
            }

            case OP_SELECT_IMPORT:
            {
                b_obj_string* entry_name = READ_STRING();
                b_obj_func* function = AS_CLOSURE(peek(vm, 0))->fnptr;
                b_value value;
                if(table_get(&function->module->values, OBJ_VAL(entry_name), &value))
                {
                    table_set(vm, &frame->closure->fnptr->module->values, OBJ_VAL(entry_name), value);
                }
                else
                {
                    runtime_error("module %s does not define '%s'", function->module->name, entry_name->chars);
                }
                break;
            }

            case OP_SELECT_NATIVE_IMPORT:
            {
                b_obj_string* module_name = AS_STRING(peek(vm, 0));
                b_obj_string* value_name = READ_STRING();
                b_value mod;
                if(table_get(&vm->modules, OBJ_VAL(module_name), &mod))
                {
                    b_obj_module* module = AS_MODULE(mod);
                    b_value value;
                    if(table_get(&module->values, OBJ_VAL(value_name), &value))
                    {
                        table_set(vm, &frame->closure->fnptr->module->values, OBJ_VAL(value_name), value);
                    }
                    else
                    {
                        runtime_error("module %s does not define '%s'", module->name, value_name->chars);
                    }
                }
                else
                {
                    runtime_error("module '%s' not found", module_name->chars);
                }
                break;
            }

            case OP_IMPORT_ALL:
            {
                table_add_all(vm, &AS_CLOSURE(peek(vm, 0))->fnptr->module->values, &frame->closure->fnptr->module->values);
                break;
            }

            case OP_IMPORT_ALL_NATIVE:
            {
                b_obj_string* name = AS_STRING(peek(vm, 0));
                b_value mod;
                if(table_get(&vm->modules, OBJ_VAL(name), &mod))
                {
                    table_add_all(vm, &AS_MODULE(mod)->values, &frame->closure->fnptr->module->values);
                }
                break;
            }

            case OP_EJECT_IMPORT:
            {
                b_obj_func* function = AS_CLOSURE(READ_CONSTANT())->fnptr;
                table_delete(&frame->closure->fnptr->module->values, STRING_VAL(function->module->name));
                break;
            }

            case OP_EJECT_NATIVE_IMPORT:
            {
                b_value mod;
                b_obj_string* name = READ_STRING();
                if(table_get(&vm->modules, OBJ_VAL(name), &mod))
                {
                    table_add_all(vm, &AS_MODULE(mod)->values, &frame->closure->fnptr->module->values);
                    table_delete(&frame->closure->fnptr->module->values, OBJ_VAL(name));
                }
                break;
            }

            case OP_ASSERT:
            {
                b_value message = pop(vm);
                b_value expression = pop(vm);
                if(is_false(expression))
                {
                    if(!IS_NIL(message))
                    {
                        bl_vm_throwexception(vm, true, value_to_string(vm, message));
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
                if(!IS_INSTANCE(peek(vm, 0)) || !bl_class_isinstanceof(AS_INSTANCE(peek(vm, 0))->klass, vm->exception_class->name->chars))
                {
                    runtime_error("instance of Exception expected");
                    break;
                }

                b_value stacktrace = get_stack_trace(vm);
                b_obj_instance* instance = AS_INSTANCE(peek(vm, 0));
                table_set(vm, &instance->properties, STRING_L_VAL("stacktrace", 10), stacktrace);
                if(bl_vm_propagateexception(vm, false))
                {
                    frame = &vm->frames[vm->frame_count - 1];
                    break;
                }

                EXIT_VM();
            }

            case OP_TRY:
            {
                b_obj_string* type = READ_STRING();
                uint16_t address = READ_SHORT();
                uint16_t finally_address = READ_SHORT();

                if(address != 0)
                {
                    b_value value;
                    if(!table_get(&vm->globals, OBJ_VAL(type), &value) || !IS_CLASS(value))
                    {
                        runtime_error("object of type '%s' is not an exception", type->chars);
                        break;
                    }
                    bl_vm_pushexceptionhandler(vm, AS_CLASS(value), address, finally_address);
                }
                else
                {
                    bl_vm_pushexceptionhandler(vm, NULL, address, finally_address);
                }
                break;
            }

            case OP_POP_TRY:
            {
                frame->handlers_count--;
                break;
            }

            case OP_PUBLISH_TRY:
            {
                frame->handlers_count--;
                if(bl_vm_propagateexception(vm, false))
                {
                    frame = &vm->frames[vm->frame_count - 1];
                    break;
                }

                EXIT_VM();
            }

            case OP_SWITCH:
            {
                b_obj_switch* sw = AS_SWITCH(READ_CONSTANT());
                b_value expr = peek(vm, 0);

                b_value value;
                if(table_get(&sw->table, expr, &value))
                {
                    frame->ip += (int)AS_NUMBER(value);
                }
                else if(sw->default_jump != -1)
                {
                    frame->ip += sw->default_jump;
                }
                else
                {
                    frame->ip += sw->exit_jump;
                }
                pop(vm);
                break;
            }

            case OP_CHOICE:
            {
                b_value _else = peek(vm, 0);
                b_value _then = peek(vm, 1);
                b_value _condition = peek(vm, 2);

                pop_n(vm, 3);
                if(!is_false(_condition))
                {
                    push(vm, _then);
                }
                else
                {
                    push(vm, _else);
                }
                break;
            }

            default:
                break;
        }
    }

#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef READ_LCONSTANT
#undef READ_STRING
#undef READ_LSTRING
#undef BINARY_OP
#undef BINARY_MOD_OP
}

b_ptr_result bl_vm_interpsource(b_vm* vm, b_obj_module* module, const char* source)
{
    b_blob blob;
    init_blob(&blob);
    vm->allowgc = false;
    initialize_exceptions(vm, module);
    vm->allowgc = true;
    b_obj_func* function = bl_compiler_compilesource(vm, module, source, &blob);

    if(vm->should_print_bytecode)
    {
        return PTR_OK;
    }

    if(function == NULL)
    {
        free_blob(vm, &blob);
        return PTR_COMPILE_ERR;
    }

    push(vm, OBJ_VAL(function));
    b_obj_closure* closure = new_closure(vm, function);
    pop(vm);
    push(vm, OBJ_VAL(closure));
    bl_vm_docall(vm, closure, 0);

    b_ptr_result result = bl_vm_run(vm);

    return result;
}

#undef ERR_CANT_ASSIGN_EMPTY

static bool continue_repl = true;

static void repl(b_vm* vm)
{
    vm->is_repl = true;

    printf("Blade %s (running on BladeVM %s), REPL/Interactive mode = ON\n", BLADE_VERSION_STRING, BVM_VERSION);
    printf("%s, (Build time = %s, %s)\n", COMPILER, __DATE__, __TIME__);
    printf("Type \".exit\" to quit or \".credits\" for more information\n");

    char* source = (char*)malloc(sizeof(char));
    memset(source, 0, sizeof(char));

    int brace_count = 0, paren_count = 0, bracket_count = 0, single_quote_count = 0, double_quote_count = 0;

    b_obj_module* module = new_module(vm, strdup(""), strdup("<repl>"));
    add_module(vm, module);

    for(;;)
    {
        if(!continue_repl)
        {
            brace_count = 0;
            paren_count = 0;
            bracket_count = 0;
            single_quote_count = 0;
            double_quote_count = 0;

            // reset source...
            memset(source, 0, strlen(source));
            continue_repl = true;
        }

        const char* cursor = "%> ";
        if(brace_count > 0 || bracket_count > 0 || paren_count > 0)
        {
            cursor = ".. ";
        }
        else if(single_quote_count == 1 || double_quote_count == 1)
        {
            cursor = "";
        }

        char buffer[1024];
        printf(cursor);
        char* line = fgets(buffer, 1024, stdin);

        int line_length = 0;
        if(line != NULL)
        {
            line_length = strcspn(line, "\r\n");
            line[line_length] = 0;
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

        if(line_length > 0 && line[0] == '#')
        {
            continue;
        }

        // find count of { and }, ( and ), [ and ], " and '
        for(int i = 0; i < line_length; i++)
        {
            // scope openers...
            if(line[i] == '{')
                brace_count++;
            if(line[i] == '(')
                paren_count++;
            if(line[i] == '[')
                bracket_count++;

            // quotes
            if(line[i] == '\'' && double_quote_count == 0)
            {
                if(single_quote_count == 0)
                    single_quote_count++;
                else
                    single_quote_count--;
            }
            if(line[i] == '"' && single_quote_count == 0)
            {
                if(double_quote_count == 0)
                    double_quote_count++;
                else
                    double_quote_count--;
            }

            if(line[i] == '\\' && (single_quote_count > 0 || double_quote_count > 0))
                i++;

            // scope closers...
            if(line[i] == '}' && brace_count > 0)
                brace_count--;
            if(line[i] == ')' && paren_count > 0)
                paren_count--;
            if(line[i] == ']' && bracket_count > 0)
                bracket_count--;
        }

        source = append_strings(source, line);
        if(line_length > 0)
        {
            source = append_strings(source, "\n");
        }

        if(bracket_count == 0 && paren_count == 0 && brace_count == 0 && single_quote_count == 0 && double_quote_count == 0)
        {
            bl_vm_interpsource(vm, module, source);

            fflush(stdout);// flush all outputs

            // reset source...
            memset(source, 0, strlen(source));
        }
    }
    free(source);
}

static void run_file(b_vm* vm, char* file)
{
    char* source = read_file(file);
    if(source == NULL)
    {
        // check if it's a Blade library directory by attempting to read the index file.
        char* old_file = file;
        file = append_strings((char*)strdup(file), "/" LIBRARY_DIRECTORY_INDEX BLADE_EXTENSION);
        source = read_file(file);

        if(source == NULL)
        {
            fprintf(stderr, "(Blade):\n  Launch aborted for %s\n  Reason: %s\n", old_file, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    b_obj_module* module = new_module(vm, strdup(""), strdup(file));
    add_module(vm, module);

    b_ptr_result result = bl_vm_interpsource(vm, module, source);
    free(source);

    fflush(stdout);

    if(result == PTR_COMPILE_ERR)
        exit(EXIT_COMPILE);
    if(result == PTR_RUNTIME_ERR)
        exit(EXIT_RUNTIME);
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
    fprintf(out,
            "   -g    Sets the minimum heap size in kilobytes before the GC\n"
            "         can start. [Default = %d (%dmb)]\n",
            DEFAULT_GC_START / 1024, DEFAULT_GC_START / (1024 * 1024));
    exit(fail ? EXIT_FAILURE : EXIT_SUCCESS);
}

int main(int argc, char* argv[])
{
    b_vm* vm;
    vm = (b_vm*)malloc(sizeof(b_vm));
    if(vm == NULL)
    {
        fprintf(stderr, "failed to allocate vm.\n");
        return 1;
    }
    memset(vm, 0, sizeof(b_vm));
    init_vm(vm);

    fprintf(stderr, "STACK_MAX = %d\n", STACK_MAX);
    bool should_debug_stack = false;
    bool should_print_bytecode = false;
    bool should_buffer_stdout = false;
    int next_gc_start = DEFAULT_GC_START;

    if(argc > 1)
    {
        int opt;
        while((opt = getopt(argc, argv, "hdbjvg:")) != -1)
        {
            switch(opt)
            {
                case 'h':
                    show_usage(argv, false);// exits
                case 'd':
                    should_print_bytecode = true;
                    break;
                case 'b':
                    should_buffer_stdout = true;
                    break;
                case 'j':
                    should_debug_stack = true;
                    break;
                case 'v':
                {
                    printf("Blade " BLADE_VERSION_STRING " (running on BladeVM " BVM_VERSION ")\n");
                    return EXIT_SUCCESS;
                }
                case 'g':
                {
                    int next = (int)strtol(optarg, NULL, 10);
                    if(next > 0)
                    {
                        next_gc_start = next * 1024;// expected value is in kilobytes
                    }
                    break;
                }
                default:
                    show_usage(argv, true);// exits
            }
        }
    }


    if(vm != NULL)
    {


        // set vm options...
        vm->should_debug_stack = should_debug_stack;
        vm->should_print_bytecode = should_print_bytecode;
        vm->next_gc = next_gc_start;

        if(should_buffer_stdout)
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

        char** std_args = (char**)calloc(argc, sizeof(char*));
        if(std_args != NULL)
        {
            for(int i = 0; i < argc; i++)
            {
                std_args[i] = argv[i];
            }
            vm->std_args = std_args;
            vm->std_args_count = argc;
        }

        // always do this last so that we can have access to everything else
        bind_native_modules(vm);

        if(argc == 1 || argc <= optind)
        {
            repl(vm);
        }
        else
        {
            run_file(vm, argv[optind]);
        }
        fprintf(stderr, "freeing up memory?\n");
        collect_garbage(vm);
        free(std_args);
        free_vm(vm);
        free(vm);
        return EXIT_SUCCESS;
    }

    fprintf(stderr, "Device out of memory.");
    exit(EXIT_FAILURE);
}
