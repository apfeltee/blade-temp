
#pragma once
/*
*
* adapted from: https://github.com/KrokodileGlue/kdg/blob/master/src/ktre.c
*/

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>


#define KTRE_MAX_ERROR_LEN 100
#define KTRE_MAX_GROUPS 100
#define KTRE_MAX_THREAD 200
#define KTRE_MAX_CALL_DEPTH 100
#define KTRE_MEM_CAP 100000000


/* error codes */
enum ktreerror_t
{
    KTRE_ERROR_NO_ERROR,
    KTRE_ERROR_STACK_OVERFLOW,
    KTRE_ERROR_CALL_OVERFLOW,
    KTRE_ERROR_SYNTAX_ERROR,
    KTRE_ERROR_OUT_OF_MEMORY,
    KTRE_ERROR_TOO_MANY_GROUPS,
    KTRE_ERROR_INVALID_OPTIONS
};


enum
{
    KTRE_INSENSITIVE = 1 << 0,
    KTRE_UNANCHORED = 1 << 1,
    KTRE_EXTENDED = 1 << 2,
    KTRE_GLOBAL = 1 << 3,
    KTRE_MULTILINE = 1 << 4,
    KTRE_CONTINUE = 1 << 5
};

enum ktreoptype_t
{
    KTRE_INSTR_MATCH,
    KTRE_INSTR_CHAR,
    KTRE_INSTR_JMP,
    KTRE_INSTR_BRANCH,
    KTRE_INSTR_ANY,
    KTRE_INSTR_MANY,
    KTRE_INSTR_CLASS,
    KTRE_INSTR_TSTR,
    KTRE_INSTR_STR,
    KTRE_INSTR_NOT,
    KTRE_INSTR_BACKREF,
    KTRE_INSTR_BOL,
    KTRE_INSTR_EOL,
    KTRE_INSTR_BOS,
    KTRE_INSTR_EOS,
    KTRE_INSTR_SETOPT,
    KTRE_INSTR_TRY,
    KTRE_INSTR_CATCH,
    KTRE_INSTR_SET_START,
    KTRE_INSTR_WB,
    KTRE_INSTR_NWB,
    KTRE_INSTR_SAVE,
    KTRE_INSTR_CALL,
    KTRE_INSTR_PLA,
    KTRE_INSTR_PLA_WIN,
    KTRE_INSTR_NLA,
    KTRE_INSTR_NLA_FAIL,
    KTRE_INSTR_PLB,
    KTRE_INSTR_PLB_WIN,
    KTRE_INSTR_NLB,
    KTRE_INSTR_NLB_FAIL,
    KTRE_INSTR_PROG,
    KTRE_INSTR_DIGIT,
    KTRE_INSTR_SPACE,
    KTRE_INSTR_WORD,
    KTRE_INSTR_RET
};

enum ktrenodetype_t
{
    NODE_NONE,
    NODE_CHAR,
    NODE_SEQUENCE,
    NODE_ASTERISK,
    NODE_PLUS,
    NODE_OR,
    NODE_GROUP,
    NODE_QUESTION,
    NODE_ANY, /* matches anything */
    NODE_MANY, /* multiline any (internal instruction) */
    NODE_CLASS, /* character class */
    NODE_NOT, /* negated character class */
    NODE_STR,
    NODE_BACKREF, /* backreference to an existing group */
    NODE_BOL,
    NODE_EOL,
    NODE_BOS, /* beginning of string */
    NODE_EOS, /* end of string */

    /*
     * Sets the options for the current thread of
     * execution.
     */
    NODE_SETOPT,
    NODE_REP, /* counted repetition */
    NODE_ATOM, /* ctomic group */
    NODE_SET_START, /* Set the start position of the group
                     * capturing the entire match. */
    NODE_WB, /* word boundary */
    NODE_NWB, /* negated word boundary */
    NODE_CALL,
    NODE_PLA, /* positive lookahead */
    NODE_PLB, /* positive lookbehind */
    NODE_NLA, /* negative lookahead */
    NODE_NLB, /* negative lookbehind */

    /*
     * Attempts to match the entire regex again at the
     * current point.
     */
    NODE_RECURSE,

    NODE_DIGIT,
    NODE_SPACE,
    NODE_WORD
};

typedef enum ktrenodetype_t ktrenodetype_t;
typedef enum ktreoptype_t ktreoptype_t;

typedef enum ktreerror_t ktreerror_t;
typedef struct ktreinfo_t ktreinfo_t;
typedef struct ktrematch_t ktrematch_t;
typedef struct ktrethread_t ktrethread_t;
typedef struct ktregroup_t ktregroup_t;
typedef struct ktrecontext_t ktrecontext_t;
typedef struct ktreinstr_t ktreinstr_t;
typedef struct ktrenode_t ktrenode_t;

struct ktreinstr_t
{
    ktreoptype_t op;
    union
    {
        struct
        {
            int firstval;
            int secondval;
        };
        int thirdval;
        char* charclass;
    };
    int loc;
};


struct ktrenode_t
{
    ktrenodetype_t type;
    ktrenode_t* firstnode;
    ktrenode_t* secondnode;
    int groupindex; /* group index */
    int othergroupindex;
    int decnum;
    char* nodecharclass;
    int loc;
};


struct ktreinfo_t
{
    int allocdbytes; /* bytes allocated */
    int maxalloc; /* max bytes allocated */
    int freedbytes; /* bytes freed */
    int numallocs, numfrees;

    int allocdthreads;
    int allocdinstr;

    int parser_alloc;
    int runtime_alloc;
};

struct ktrematch_t
{
    const char* file;
    int line;
    int size;
    ktrematch_t* prev;
    ktrematch_t* next;
};

struct ktrethread_t
{
    int thinstrptr;
    int thstackptr;
    int fp;
    int la;
    int ep;
    int opt;
    int *frame;
    int *vec;
    int *prog;
    int *las;
    int *exception;
    bool die;
    bool rev;
};

struct ktregroup_t
{
    int address;

    /*
     * These boolean flags indicate whether this group
     * has been compiled or called as a subroutine,
     * respectively. The is_called field is flexible; it's
     * used whenever a group must be called as a
     * subroutine, which a counted repetition may do.
     */
    bool iscompiled;
    bool iscalled;
    char* name;
};

struct ktrecontext_t
{
    /* ===== public fields ===== */
    int nummatches;
    int numgroups;
    int opt;

    /*
	 * String containing an error message in the case of failure
	 * during parsing or compilation.
	 */
    char* errstr;
    ktreerror_t errcode; /* error status code */

    /*
	 * The oaklocation_t of any error that occurred, as an index in the
	 * last subject the regex was run on.
	 */
    int loc;

    /* ===== private fields ===== */
    ktreinstr_t* compiled; /* code */
    int instrpos; /* oakinstruction_t pointer */
    int compcount; /* number of progress instructions */
    char const* sourcepattern; /* pattern that's currently begin compiled */
    char const* stackptr; /* to the character currently being parsed */
    int popt; /* the options, as seen by the oakparser_t */
    int groupptr; /* group pointer */
    ktrenode_t* headnode; /* the head node of the ast */
    bool shouldescapemeta; /* whether to escape metacharacters or not */
    int cont;
    ktregroup_t * group;

    /* runtime */
    ktrethread_t * thread;

    int thrptr, max_tp;
    int** vec;

    ktreinfo_t info;
    ktrematch_t* minfo;

    bool copied;
};

/* ktre.c */
ktrecontext_t *ktre_compile(const char *pat, int opt);
ktrecontext_t *ktre_copy(ktrecontext_t *re);
ktreinfo_t ktre_free(ktrecontext_t *re);
bool ktre_exec(ktrecontext_t *re, const char *subject, int ***vec);
bool ktre_match(const char *subject, const char *pat, int opt, int ***vec);
char *ktre_replace(const char *subject, const char *pat, const char *replacement, const char *indicator, int opt);
char *ktre_filter(ktrecontext_t *re, const char *subject, const char *replacement, const char *indicator);
char **ktre_split(ktrecontext_t *re, const char *subject, int *len);
int **ktre_getvec(const ktrecontext_t *re);

