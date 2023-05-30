
/*
*
* adapted from: https://github.com/KrokodileGlue/kdg/blob/master/src/ktre.c
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "ktre.h"

#define KTRE_WHITESPACECHARS " \t\r\n\v\f"
#define KTRE_DIGIT "0123456789"
#define KTRE_WORD "_0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"

#ifdef KTRE_DEBUG
    #define DBG(...) fprintf(stderr, __VA_ARGS__)
#else
    #define DBG(x, ...)
#endif /* KTRE_DEBUG */

/* memory functions */
#ifndef KTRE_MALLOC
    #define KTRE_MALLOC malloc
    #define KTRE_FREE free
#endif

#define _malloc(n) ktrepriv_malloc(re, n, __FILE__, __LINE__)
#define _realloc(ptr, n) ktrepriv_realloc(re, ptr, n, __FILE__, __LINE__)
#define _free(ptr) ktrepriv_free(re, ptr)

static const struct
{
    const char* name;
    const char* rawclassvalue;
} pclasses[] = {
    //clang-format off
    { "[:upper:]", "ABCDEFGHIJKLMNOPQRSTUVWXYZ" },
    { "[:lower:]", "abcdefghijklmnopqrstuvwxyz" },
    { "[:alpha:]", "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ" },
    { "[:digit:]", "0123456789" },
    { "[:xdigit:]", "0123456789ABCDEFabcdef" },
    { "[:alnum:]", "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ" },
    { "[:punct:]", "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~" },
    { "[:blank:]", " \t" },
    { "[:space:]", KTRE_WHITESPACECHARS },
    { "[:cntrl:]", "\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b"
                   "\x0c\x0d\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19"
                   "\x1a\x1b\x1c\x1d\x1e\x1f\x7F" },
    { "[:graph:]", "\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2a\x2b"
                   "\x2c\x2d\x2e\x2f\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3a"
                   "\x3b\x3c\x3d\x3e\x3f\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49"
                   "\x4a\x4b\x4c\x4d\x4e\x4f\x50\x51\x52\x53\x54\x55\x56\x57\x58"
                   "\x59\x5a\x5b\x5c\x5d\x5e\x5f\x60\x61\x62\x63\x64\x65\x66\x67"
                   "\x68\x69\x6a\x6b\x6c\x6d\x6e\x6f\x70\x71\x72\x73\x74\x75\x76"
                   "\x77\x78\x79\x7a\x7b\x7c\x7d\x7e" },
    { "[:print:]", "\x20\x21\x22\x23\x24\x25\x26\x27\x28\x29\x2a\x2b"
                   "\x2c\x2d\x2e\x2f\x30\x31\x32\x33\x34\x35\x36\x37\x38\x39\x3a"
                   "\x3b\x3c\x3d\x3e\x3f\x40\x41\x42\x43\x44\x45\x46\x47\x48\x49"
                   "\x4a\x4b\x4c\x4d\x4e\x4f\x50\x51\x52\x53\x54\x55\x56\x57\x58"
                   "\x59\x5a\x5b\x5c\x5d\x5e\x5f\x60\x61\x62\x63\x64\x65\x66\x67"
                   "\x68\x69\x6a\x6b\x6c\x6d\x6e\x6f\x70\x71\x72\x73\x74\x75\x76"
                   "\x77\x78\x79\x7a\x7b\x7c\x7d\x7e" }
    // clang-format on
};

void ktre_printnode(ktrecontext_t* re, ktrenode_t* n);
static void ktrepriv_error(ktrecontext_t* re, ktreerror_t err, int loc, const char* fmt, ...);
static void* ktrepriv_malloc(ktrecontext_t* re, size_t n, const char* file, int line);
static void* ktrepriv_realloc(ktrecontext_t* re, void* ptr, size_t n, const char* file, int line);
static void ktrepriv_free(ktrecontext_t* re, void* ptr);
static ktrenode_t* ktrepriv_parse(ktrecontext_t* re);
static ktrenode_t* ktrepriv_term(ktrecontext_t* re);


static void* ktrepriv_malloc(ktrecontext_t* re, size_t n, const char* file, int line)
{
    ktrematch_t* mi;
    if(re->info.allocdbytes + n + sizeof(ktrecontext_t) > KTRE_MEM_CAP)
    {
        ktrepriv_error(re, KTRE_ERROR_OUT_OF_MEMORY, 0, NULL);
        DBG("\nrunning out of memory at %d bytes:\n\trequest for %zd bytes at %s:%d", re->info.ba, n, file, line);
        return NULL;
    }
    mi = (ktrematch_t*)KTRE_MALLOC(n + sizeof(ktrematch_t));
    if(!mi)
    {
        ktrepriv_error(re, KTRE_ERROR_OUT_OF_MEMORY, 0, NULL);
        return NULL;
    }
    re->info.numallocs++;
    re->info.allocdbytes += n + sizeof(ktrematch_t);
    re->info.maxalloc += n + sizeof(ktrematch_t);
    mi->file = file;
    mi->line = line;
    mi->next = re->minfo;
    if(re->minfo)
    {
        re->minfo->prev = mi;
    }
    mi->prev = NULL;
    mi->size = (int)n;
    re->minfo = mi;
    return mi + 1;
}

static void* ktrepriv_realloc(ktrecontext_t* re, void* ptr, size_t n, const char* file, int line)
{
    int diff;
    void* p;
    ktrematch_t* mi;
    if(!ptr)
    {
        return ktrepriv_malloc(re, n, file, line);
    }
    mi = (ktrematch_t*)ptr - 1;
    diff = n - mi->size;
    /*
    * We don't need to grow the block, so just return early.
    */
    if(diff <= 0)
    {
        return ptr;
    }
    if(re->info.allocdbytes + diff > KTRE_MEM_CAP)
    {
        KTRE_FREE(ptr);
        ktrepriv_error(re, KTRE_ERROR_OUT_OF_MEMORY, 0, NULL);
        return NULL;
    }
    p = ktrepriv_malloc(re, n, file, line);
    if(p)
    {
        memcpy(p, ptr, (int)n > mi->size ? mi->size : (int)n);
    }
    ktrepriv_free(re, ptr);
    return p;
}

static void ktrepriv_free(ktrecontext_t* re, void* ptr)
{
    ktrematch_t* mi;
    if(!ptr)
    {
        return;
    }
    mi = (ktrematch_t*)ptr - 1;
    re->info.allocdbytes -= mi->size + sizeof(ktrematch_t);
    re->info.freedbytes += mi->size + sizeof(ktrematch_t);
    re->info.numfrees++;
    if(!mi->prev)
    {
        re->minfo = mi->next;
    }
    else
    {
        mi->prev->next = mi->next;
    }
    if(mi->next)
    {
        mi->next->prev = mi->prev;
    }
    KTRE_FREE(mi);
}

static void dbgf(const char* str)
{
    int i;
    (void)str;
    (void)i;
#ifdef KTRE_DEBUG
    for(i = 0; i < (int)strlen(str); i++)
    {
        if((strchr(KTRE_WHITESPACECHARS, str[i]) || str[i] == '\\') && str[i] != ' ')
        {
            fputc('\\', stderr);
            switch(str[i])
            {
                case '\t':
                    {
                        fputc('t', stderr);
                    }
                    break;
                case '\r':
                    {
                        fputc('r', stderr);
                    }
                    break;
                case '\n':
                    {
                        fputc('n', stderr);
                    }
                    break;
                case '\v':
                    {
                        fputc('v', stderr);
                    }
                    break;
                case '\f':
                    {
                        fputc('f', stderr);
                    }
                    break;
                case '\\':
                    {
                        fputc('\\', stderr);
                    }
                    break;
                default:
                    {
                        fputc(str[i], stderr);
                    }
                    break;
            }
        }
        else
        {
            fputc(str[i], stderr);
        }
    }
#endif
}

static int ktrepriv_addgroup(ktrecontext_t* re)
{
    if(re->groupptr >= KTRE_MAX_GROUPS)
    {
        ktrepriv_error(re, KTRE_ERROR_TOO_MANY_GROUPS, re->stackptr - re->sourcepattern, "regex contains too many groups");
        return -1;
    }
    re->group = (ktregroup_t*)_realloc(re->group, (re->groupptr + 1) * sizeof re->group[0]);
    if(!re->group)
    {
        return -1;
    }
    re->group[re->groupptr].iscompiled = false;
    re->group[re->groupptr].address = -1;
    re->group[re->groupptr].iscalled = false;
    re->group[re->groupptr].name = NULL;
    return re->groupptr++;
}

static void ktrepriv_growcode(ktrecontext_t* re, int n)
{
    if(!re->info.allocdinstr)
    {
        re->info.allocdinstr = 25;
        re->compiled = (ktreinstr_t*)_malloc(sizeof re->compiled[0] * re->info.allocdinstr);
    }
    if(!re->compiled)
    {
        return;
    }
    if(re->instrpos + n >= re->info.allocdinstr)
    {
        if(re->instrpos + n >= re->info.allocdinstr * 2)
        {
            re->info.allocdinstr = re->instrpos + n;
        }
        else
        {
            re->info.allocdinstr *= 2;
        }
        re->compiled = (ktreinstr_t*)_realloc(re->compiled, sizeof re->compiled[0] * re->info.allocdinstr);
    }
}

static void ktrepriv_emitab(ktrecontext_t* re, int instr, int a, int b, int loc)
{
    ktrepriv_growcode(re, 1);
    if(!re->compiled)
    {
        return;
    }
    re->compiled[re->instrpos].op = (ktreoptype_t)instr;
    re->compiled[re->instrpos].firstval = a;
    re->compiled[re->instrpos].secondval = b;
    re->compiled[re->instrpos].loc = loc;
    re->instrpos++;
}

static void ktrepriv_emitc(ktrecontext_t* re, int instr, int c, int loc)
{
    ktrepriv_growcode(re, 1);
    if(!re->compiled)
    {
        return;
    }
    re->compiled[re->instrpos].op = (ktreoptype_t)instr;
    re->compiled[re->instrpos].thirdval = c;
    re->compiled[re->instrpos].loc = loc;
    re->instrpos++;
}

static void ktrepriv_emitclass(ktrecontext_t* re, int instr, char* chclass, int loc)
{
    ktrepriv_growcode(re, 1);
    if(!re->compiled)
    {
        return;
    }
    re->compiled[re->instrpos].op = (ktreoptype_t)instr;
    re->compiled[re->instrpos].charclass = chclass;
    re->compiled[re->instrpos].loc = loc;
    re->instrpos++;
}

static void ktrepriv_emit(ktrecontext_t* re, int instr, int loc)
{
    ktrepriv_growcode(re, 1);
    if(!re->compiled)
    {
        return;
    }
    re->compiled[re->instrpos].op = (ktreoptype_t)instr;
    re->compiled[re->instrpos].loc = loc;
    re->instrpos++;
}

static void ktrepriv_freenode(ktrecontext_t* re, ktrenode_t* n)
{
    /* sometimes parse errors can produce NULL nodes */
    if(!n)
    {
        return;
    }
    switch(n->type)
    {
        case NODE_SEQUENCE:
        case NODE_OR:
            {
                ktrepriv_freenode(re, n->firstnode);
                ktrepriv_freenode(re, n->secondnode);
            }
            break;
        case NODE_QUESTION:
        case NODE_REP:
        case NODE_ASTERISK:
        case NODE_PLUS:
        case NODE_GROUP:
        case NODE_ATOM:
        case NODE_PLA:
        case NODE_NLA:
        case NODE_PLB:
        case NODE_NLB:
            {
                ktrepriv_freenode(re, n->firstnode);
            }
            break;
        case NODE_CLASS:
        case NODE_NOT:
        case NODE_STR:
            {
                _free(n->nodecharclass);
            }
            break;
        default:
            break;
    }
    _free(n);
}

static void ktrepriv_nextchar(ktrecontext_t* re)
{
    if(*re->stackptr)
    {
        re->stackptr++;
    }
}

static void ktrepriv_error(ktrecontext_t* re, ktreerror_t err, int loc, const char* fmt, ...)
{
    va_list args;
    if(re->errcode)
    {
        return;
    }
    re->errcode = err;
    re->loc = loc;
    if(!fmt)
    {
        re->errstr = NULL;
        return;
    }
    re->errstr = (char*)_malloc(KTRE_MAX_ERROR_LEN);
    va_start(args, fmt);
    vsnprintf(re->errstr, KTRE_MAX_ERROR_LEN, fmt, args);
    va_end(args);
}

static inline char ktrepriv_lc(char c)
{
    if((c >= 'A') && (c <= 'Z'))
    {
        return ((c - 'A') + 'a');
    }
    return c;
}

static inline char ktrepriv_uc(char c)
{
    if((c >= 'a') && (c <= 'z'))
    {
        return ((c - 'a') + 'A');
    }
    return c;
}

static inline void ktrepriv_lcstr(char* s)
{
    int i;
    for(i = 0; i < (int)strlen(s); i++)
    {
        s[i] = ktrepriv_lc(s[i]);
    }
}

static inline void ktrepriv_ucstr(char* s)
{
    int i;
    for(i = 0; i < (int)strlen(s); i++)
    {
        s[i] = ktrepriv_uc(s[i]);
    }
}

static void ktrepriv_appendchar(ktrecontext_t* re, char** chclassptr, char c)
{
    size_t len;
    len = *chclassptr ? strlen(*chclassptr) : 0;
    *chclassptr = (char*)_realloc(*chclassptr, len + 2);
    if(!*chclassptr)
    {
        return;
    }
    (*chclassptr)[len] = c;
    (*chclassptr)[len + 1] = 0;
}

static void ktrepriv_appendstr(ktrecontext_t* re, char** chclassptr, const char* c)
{
    size_t len;
    len = *chclassptr ? strlen(*chclassptr) : 0;
    *chclassptr = (char*)_realloc(*chclassptr, len + strlen(c) + 1);
    if(!*chclassptr)
    {
        return;
    }
    strcpy(*chclassptr + len, c);
}

static char* ktrepriv_strclone(ktrecontext_t* re, const char* str)
{
    char* ret;
    ret = (char*)_malloc(strlen(str) + 1);
    if(!ret)
    {
        return NULL;
    }
    re->info.parser_alloc += strlen(str) + 1;
    strcpy(ret, str);
    return ret;
}

static ktrenode_t* ktrepriv_newnode(ktrecontext_t* re)
{
    ktrenode_t* n;
    n = (ktrenode_t*)_malloc(sizeof *n);
    if(!n)
    {
        return NULL;
    }
    re->info.parser_alloc += sizeof *n;
    memset(n, 0, sizeof *n);
    n->loc = re->stackptr - re->sourcepattern;
    return n;
}

static int ktrepriv_decnum(const char** s)
{
    int n;
    n = -1;
    while(**s && isdigit(**s))
    {
        if(n < 0)
        {
            n = 0;
        }
        n = n * 10 + (**s - '0');
        (*s)++;
    }
    return n;
}

static int ktrepriv_hexnum(const char** s)
{
    int n;
    char c;
    n = -1;
    while(**s && ((ktrepriv_lc(**s) >= 'a' && ktrepriv_lc(**s) <= 'f') || (**s >= '0' && **s <= '9')))
    {
        if(n < 0)
        {
            n = 0;
        }
        c = ktrepriv_lc(**s);
        n *= 16;
        n += (c >= 'a' && c <= 'f') ? c - 'a' + 10 : c - '0';
        (*s)++;
    }
    return n;
}

static int ktrepriv_octnum(const char** s)
{
    int n;
    n = -1;
    while(**s >= '0' && **s <= '7')
    {
        if(n < 0)
        {
            n = 0;
        }
        n *= 8;
        n += **s - '0';
        (*s)++;
    }
    return n;
}

static int ktrepriv_parsedecnum(ktrecontext_t* re)
{
    int n;
    n = ktrepriv_decnum(&re->stackptr);
    if(n < 0 && !re->errcode)
    {
        ktrepriv_error(re, KTRE_ERROR_SYNTAX_ERROR, re->stackptr - re->sourcepattern, "expected a number");
    }
    return n;
}

static int ktrepriv_parsehexnum(ktrecontext_t* re)
{
    int n;
    n = ktrepriv_hexnum(&re->stackptr);
    if(n < 0 && !re->errcode)
    {
        ktrepriv_error(re, KTRE_ERROR_SYNTAX_ERROR, re->stackptr - re->sourcepattern, "expected a number");
    }
    return n;
}

static int ktrepriv_parseoctnum(ktrecontext_t* re)
{
    int n;
    n = ktrepriv_octnum(&re->stackptr);
    if(n < 0 && !re->errcode)
    {
        ktrepriv_error(re, KTRE_ERROR_SYNTAX_ERROR, re->stackptr - re->sourcepattern, "expected a number");
    }
    return n;
}

static ktrenode_t* ktrepriv_parsemodemodifiers(ktrecontext_t* re)
{
    int old;
    int opt;
    int bit;
    bool off;
    bool neg;
    ktrenode_t* left;
    left = ktrepriv_newnode(re);
    left->type = NODE_SETOPT;
    neg = false;
    /*
    * Here we preserve the original options in case they
    * ever need to be restored.
    */
    old = re->popt;
    opt = re->popt;
    while(*re->stackptr && *re->stackptr != ')' && *re->stackptr != ':')
    {
        off = false;
        bit = 0;
        switch(*re->stackptr)
        {
            case 'c':
                {
                    off = true;
                }
                /* fallthrough */
                break;
            case 'i':
                {
                    bit = KTRE_INSENSITIVE;
                }
                break;
            case 't':
                {
                    off = true;
                }
                /* fallthrough */
                break;
            case 'x':
                {
                    bit = KTRE_EXTENDED;
                }
                break;
            case 'm':
                {
                    bit = KTRE_MULTILINE;
                }
                break;
            case '-':
                {
                    neg = true;
                    ktrepriv_nextchar(re);
                }
                continue;
            default:
                {
                    ktrepriv_error(re, KTRE_ERROR_SYNTAX_ERROR, re->stackptr - re->sourcepattern, "invalid mode modifier");
                    ktrepriv_freenode(re, left);
                    return NULL;
                }
                break;
        }
        if(off || neg)
        {
            opt &= ~bit;
        }
        else
        {
            opt |= bit;
        }
        re->stackptr++;
    }
    re->popt = opt;
    left->othergroupindex = opt;
    if(*re->stackptr == ':')
    {
        ktrepriv_nextchar(re);
        /*
        * These are inline mode modifiers: to handle these,
        * we'll have to put a SETOPT instruction at the
        * beginning of the group, and another SETOPT at the
        * end to undo what was done by the first. We also
        * have to set and restore the parser's options.
        *
        * Because the only way we have of stringing nodes
        * together is by creating SEQUENCE nodes, the code
        * here is a little ugly.
        */
        ktrenode_t* tmp1 = ktrepriv_newnode(re);
        ktrenode_t* tmp2 = ktrepriv_newnode(re);
        ktrenode_t* tmp3 = ktrepriv_newnode(re);
        tmp1->type = NODE_SEQUENCE;
        tmp2->type = NODE_SEQUENCE;
        tmp3->type = NODE_SETOPT;
        tmp3->othergroupindex = old;
        tmp2->firstnode = ktrepriv_parse(re);
        tmp2->secondnode = tmp3;
        tmp1->firstnode = left;
        tmp1->secondnode = tmp2;
        left = tmp1;
        re->popt = old;
    }
    return left;
}

static ktrenode_t* ktrepriv_parsebranchreset(ktrecontext_t* re)
{
    int top;
    int bottom;
    ktrenode_t* tmp;
    ktrenode_t* left;
    left = NULL;
    bottom = re->groupptr;
    top = -1;
    do
    {
        if(*re->stackptr == '|')
        {
            re->stackptr++;
        }
        tmp = ktrepriv_newnode(re);
        if(left)
        {
            tmp->type = NODE_OR;
            tmp->firstnode = left;
            tmp->secondnode = ktrepriv_term(re);
        }
        else
        {
            ktrepriv_freenode(re, tmp);
            tmp = ktrepriv_term(re);
        }
        left = tmp;
        top = top > re->groupptr ? top : re->groupptr;
        re->groupptr = bottom;
    } while(*re->stackptr == '|');
    re->groupptr = top;
    if(*re->stackptr != ')')
    {
        ktrepriv_error(re, KTRE_ERROR_SYNTAX_ERROR, re->stackptr - re->sourcepattern, "expected ')'");
        ktrepriv_freenode(re, left);
        return NULL;
    }
    return left;
}

static ktrenode_t* ktrepriv_parsespecialgroup(ktrecontext_t* re)
{
    const char* sptr;
    const char* newsptr;
    ktrenode_t* left;
    left = ktrepriv_newnode(re);
    re->stackptr++;
    switch(re->stackptr[-1])
    {
        case '#':
            {
                left->type = NODE_NONE;
                while(*re->stackptr && *re->stackptr != ')')
                {
                    re->stackptr++;
                }
            }
            break;
        case '<':
            {
                if(strchr(KTRE_WORD, *re->stackptr))
                {
                    sptr = re->stackptr;
                    while(*re->stackptr && strchr(KTRE_WORD, *re->stackptr))
                    {
                        re->stackptr++;
                    }
                    newsptr = re->stackptr;
                    if(*re->stackptr != '>')
                    {
                        ktrepriv_error(re, KTRE_ERROR_SYNTAX_ERROR, re->stackptr - re->sourcepattern, "expected '>'");
                        ktrepriv_freenode(re, left);
                        return NULL;
                    }
                    ktrepriv_nextchar(re);
                    left->type = NODE_GROUP;
                    left->groupindex = ktrepriv_addgroup(re);
                    if(left->groupindex < 0)
                    {
                        ktrepriv_freenode(re, left);
                        return NULL;
                    }
                    re->group[left->groupindex].iscalled = false;
                    re->group[left->groupindex].name = (char*)_malloc(newsptr - sptr + 1);
                    strncpy(re->group[left->groupindex].name, sptr, newsptr - sptr);
                    re->group[left->groupindex].name[newsptr - sptr] = 0;
                    left->firstnode = ktrepriv_parse(re);
                    break;
                }
                if(*re->stackptr == '=')
                {
                    left->type = NODE_PLB;
                }
                else if(*re->stackptr == '!')
                {
                    left->type = NODE_NLB;
                }
                else
                {
                    ktrepriv_error(re, KTRE_ERROR_SYNTAX_ERROR, left->loc, "invalid group syntax");
                    ktrepriv_freenode(re, left);
                    return NULL;
                }
                ktrepriv_nextchar(re);
                left->firstnode = ktrepriv_parse(re);
            }
            break;
        case '\'':
            {
                sptr = re->stackptr;
                while(*re->stackptr && strchr(KTRE_WORD, *re->stackptr))
                {
                    re->stackptr++;
                }
                newsptr = re->stackptr;
                if(*re->stackptr != '\'')
                {
                    ktrepriv_error(re, KTRE_ERROR_SYNTAX_ERROR, re->stackptr - re->sourcepattern, "expected '\''");
                    ktrepriv_freenode(re, left);
                    return NULL;
                }
                ktrepriv_nextchar(re);
                left->type = NODE_GROUP;
                left->groupindex = ktrepriv_addgroup(re);
                if(left->groupindex < 0)
                {
                    ktrepriv_freenode(re, left);
                    return NULL;
                }
                re->group[left->groupindex].iscalled = false;
                re->group[left->groupindex].name = (char*)_malloc(newsptr - sptr + 1);
                strncpy(re->group[left->groupindex].name, sptr, newsptr - sptr);
                re->group[left->groupindex].name[newsptr - sptr] = 0;
                left->firstnode = ktrepriv_parse(re);
            }
            break;
        case ':':
            {
                ktrepriv_freenode(re, left);
                left = ktrepriv_parse(re);
            }
            break;
        case '|':
            {
                ktrepriv_freenode(re, left);
                left = ktrepriv_parsebranchreset(re);
            }
            break;
        case '>':
            {
                left->type = NODE_ATOM;
                left->firstnode = ktrepriv_parse(re);
            }
            break;
        case '=':
            {
                left->type = NODE_PLA;
                left->firstnode = ktrepriv_parse(re);
            }
            break;
        case '!':
            {
                left->type = NODE_NLA;
                left->firstnode = ktrepriv_parse(re);
            }
            break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            {
                left->type = NODE_CALL;
                re->stackptr--;
                left->othergroupindex = ktrepriv_parsedecnum(re);
                if(left->othergroupindex >= 0 && left->othergroupindex < re->groupptr)
                {
                    re->group[left->othergroupindex].iscalled = true;
                }
            }
            break;
        case 'P':
            {
                if(*re->stackptr == '=')
                {
                    /* This is a backreference to a named group */
                    ktrepriv_nextchar(re);
                    sptr = re->stackptr;
                    while(*re->stackptr && strchr(KTRE_WORD, *re->stackptr))
                    {
                        re->stackptr++;
                    }
                    newsptr = re->stackptr;
                    if(*re->stackptr != ')')
                    {
                        ktrepriv_error(re, KTRE_ERROR_SYNTAX_ERROR, re->stackptr - re->sourcepattern, "expected ')'");
                        ktrepriv_freenode(re, left);
                        return NULL;
                    }
                    left->type = NODE_BACKREF;
                    left->othergroupindex = -1;
                    for(int i = 0; i < re->groupptr; i++)
                    {
                        if(!re->group[i].name)
                        {
                            continue;
                        }
                        if(!strncmp(re->group[i].name, sptr, newsptr - sptr) && newsptr - sptr == (int)strlen(re->group[i].name))
                        {
                            left->othergroupindex = i;
                            break;
                        }
                    }
                    if(left->othergroupindex < 0)
                    {
                        ktrepriv_error(re, KTRE_ERROR_SYNTAX_ERROR, re->stackptr - re->sourcepattern, "name references a group that does not exist");
                        ktrepriv_freenode(re, left);
                        return NULL;
                    }
                    break;
                }
                if(*re->stackptr != '<')
                {
                    ktrepriv_error(re, KTRE_ERROR_SYNTAX_ERROR, re->stackptr - re->sourcepattern, "expected '<'");
                    ktrepriv_freenode(re, left);
                    return NULL;
                }
                ktrepriv_nextchar(re);
                sptr = re->stackptr;
                while(*re->stackptr && strchr(KTRE_WORD, *re->stackptr))
                {
                    re->stackptr++;
                }
                newsptr = re->stackptr;
                if(*re->stackptr != '>')
                {
                    ktrepriv_error(re, KTRE_ERROR_SYNTAX_ERROR, re->stackptr - re->sourcepattern, "expected '>'");
                    ktrepriv_freenode(re, left);
                    return NULL;
                }
                ktrepriv_nextchar(re);
                left->type = NODE_GROUP;
                left->groupindex = ktrepriv_addgroup(re);
                if(left->groupindex < 0)
                {
                    ktrepriv_freenode(re, left);
                    return NULL;
                }
                re->group[left->groupindex].iscalled = false;
                re->group[left->groupindex].name = (char*)_malloc(newsptr - sptr + 1);
                strncpy(re->group[left->groupindex].name, sptr, newsptr - sptr);
                re->group[left->groupindex].name[newsptr - sptr] = 0;
                left->firstnode = ktrepriv_parse(re);
            }
            break;
        default:
            {
                ktrepriv_freenode(re, left);
                re->stackptr--;
                left = ktrepriv_parsemodemodifiers(re);
            }
            break;
    }
    return left;
}

static ktrenode_t* ktrepriv_parsegroup(ktrecontext_t* re)
{
    ktrenode_t* left;
    left = ktrepriv_newnode(re);
    ktrepriv_nextchar(re);
    if(!strncmp(re->stackptr, "?R", 2))
    {
        left->type = NODE_RECURSE;
        re->stackptr += 2;
        re->group[0].iscalled = true;
    }
    else if(*re->stackptr == '?')
    {
        ktrepriv_nextchar(re);
        ktrepriv_freenode(re, left);
        left = ktrepriv_parsespecialgroup(re);
    }
    else
    {
        left->type = NODE_GROUP;
        left->groupindex = ktrepriv_addgroup(re);
        if(left->groupindex < 0)
        {
            ktrepriv_freenode(re, left);
            return NULL;
        }
        re->group[left->groupindex].iscalled = false;
        left->firstnode = ktrepriv_parse(re);
    }
    if(*re->stackptr != ')' && !re->errcode)
    {
        ktrepriv_error(re, KTRE_ERROR_SYNTAX_ERROR, left->loc, "unmatched '('");
        ktrepriv_freenode(re, left);
        return NULL;
    }
    /* skip over the `)` */
    ktrepriv_nextchar(re);
    return left;
}

static char* ktrepriv_parsecharacterclasscharacter(ktrecontext_t* re)
{
    int subloc;
    size_t pi;
    int i;
    int j;
    char* chclass;
    chclass = NULL;
    if(*re->stackptr == '[')
    {
        for(pi = 0; pi < sizeof pclasses / sizeof pclasses[0]; pi++)
        {
            if(!strncmp(re->stackptr, pclasses[pi].name, strlen(pclasses[pi].name)))
            {
                ktrepriv_appendstr(re, &chclass, pclasses[pi].rawclassvalue);
                re->stackptr += strlen(pclasses[pi].name);
                return chclass;
            }
        }
        ktrepriv_appendchar(re, &chclass, *re->stackptr);
        ktrepriv_nextchar(re);
        return chclass;
    }
    else if(*re->stackptr != '\\')
    {
        ktrepriv_appendchar(re, &chclass, *re->stackptr);
        ktrepriv_nextchar(re);
        return chclass;
    }
    /* skip over the `\` */
    ktrepriv_nextchar(re);
    switch(*re->stackptr)
    {
        case 'x':
            {
                ktrepriv_nextchar(re);
                subloc = re->stackptr - re->sourcepattern;
                bool bracketed = *re->stackptr == '{';
                if(bracketed)
                {
                    ktrepriv_nextchar(re);
                    ktrepriv_appendchar(re, &chclass, ktrepriv_parsehexnum(re));
                    if(*re->stackptr != '}' && !re->errcode)
                    {
                        if(chclass)
                        {
                            _free(chclass);
                        }
                        ktrepriv_error(re, KTRE_ERROR_SYNTAX_ERROR, subloc, "incomplete token");
                        return NULL;
                    }
                }
                else
                {
                    ktrepriv_appendchar(re, &chclass, ktrepriv_parsehexnum(re));
                    re->stackptr--;
                }
            }
            break;
        case '0':
            {
                ktrepriv_nextchar(re);
                ktrepriv_appendchar(re, &chclass, ktrepriv_parseoctnum(re));
                re->stackptr--;
            }
            break;
        case 'o':
            {
                ktrepriv_nextchar(re);
                subloc = re->stackptr - re->sourcepattern;
                if(*re->stackptr != '{' && !re->errcode)
                {
                    ktrepriv_error(re, KTRE_ERROR_SYNTAX_ERROR, subloc, "incomplete token");
                    return NULL;
                }
                ktrepriv_nextchar(re);
                ktrepriv_appendchar(re, &chclass, ktrepriv_parseoctnum(re));
                if(*re->stackptr != '}' && !re->errcode)
                {
                    ktrepriv_error(re, KTRE_ERROR_SYNTAX_ERROR, subloc, "unmatched '{'");
                    return NULL;
                }
            }
            break;
        case 's':
            {
                ktrepriv_appendstr(re, &chclass, KTRE_WHITESPACECHARS);
            }
            break;
        case 'w':
            {
                ktrepriv_appendstr(re, &chclass, KTRE_WORD);
            }
            break;
        case 'a':
            {
                ktrepriv_appendchar(re, &chclass, '\a');
            }
            break;
        case 'b':
            {
                ktrepriv_appendchar(re, &chclass, '\b');
            }
            break;
        case 'f':
            {
                ktrepriv_appendchar(re, &chclass, '\f');
            }
            break;
        case 'n':
            {
                ktrepriv_appendchar(re, &chclass, '\n');
            }
            break;
        case 't':
            {
                ktrepriv_appendchar(re, &chclass, '\t');
            }
            break;
        case 'r':
            {
                ktrepriv_appendchar(re, &chclass, '\r');
            }
            break;
        case 'd':
            {
                ktrepriv_appendstr(re, &chclass, KTRE_DIGIT);
            }
            break;
        case 'h':
            {
                ktrepriv_appendstr(re, &chclass, " \t");
            }
            break;
        case 'e':
            {
                ktrepriv_appendchar(re, &chclass, 7);
            }
            break;
        case 'D':
            {
                chclass = (char*)_malloc(256);
                i = 0;
                for(j = 1; j < '0'; j++)
                {
                    chclass[i++] = j;
                }
                for(j = '9' + 1; j < 256; j++)
                {
                    chclass[i++] = j;
                }
                chclass[i] = 0;
            }
            break;
        case 'H':
            {
                chclass = (char*)_malloc(256);
                i = 0;
                for(j = 1; j < 256; j++)
                {
                    if(!strchr("\t ", j))
                    {
                        chclass[i++] = j;
                    }
                }
                chclass[i] = 0;
            }
            break;
        case 'N':
            {
                chclass = (char*)_malloc(256);
                i = 0;
                for(j = 1; j < 256; j++)
                {
                    if(j != '\n')
                    {
                        chclass[i++] = j;
                    }
                }
                chclass[i] = 0;
            }
            break;
        case 'S':
            {
                chclass = (char*)_malloc(256);
                i = 0;
                for(j = 1; j < 256; j++)
                {
                    if(!strchr(KTRE_WHITESPACECHARS, j))
                    {
                        chclass[i++] = j;
                    }
                }
                chclass[i] = 0;
            }
            break;
        default:
            {
                ktrepriv_appendchar(re, &chclass, *re->stackptr);
            }
            break;
    }
    ktrepriv_nextchar(re);
    return chclass;
}



static ktrenode_t* ktrepriv_parsecharacterclass(ktrecontext_t* re)
{
    int i;
    bool range;
    char* chclass;
    char* tmpclass;
    char* newtmpclass;
    ktrenode_t* left;
    left = ktrepriv_newnode(re);
    chclass = NULL;
    if(*re->stackptr == '^')
    {
        left->type = NODE_NOT;
        ktrepriv_nextchar(re);
    }
    else
    {
        left->type = NODE_CLASS;
    }
    while(*re->stackptr && *re->stackptr != ']')
    {
        tmpclass = NULL;
        newtmpclass = NULL;
        range = false;
        tmpclass = ktrepriv_parsecharacterclasscharacter(re);
        if(!tmpclass)
        {
            ktrepriv_freenode(re, left);
            return NULL;
        }
        range = (*re->stackptr && (*re->stackptr == '-') && (re->stackptr[1] != ']') && (strlen(tmpclass) == 1));
        if(range)
        {
            ktrepriv_nextchar(re);
            newtmpclass = ktrepriv_parsecharacterclasscharacter(re);
            if(!newtmpclass)
            {
                _free(tmpclass);
                ktrepriv_freenode(re, left);
                return NULL;
            }
            if(strlen(newtmpclass) != 1)
            {
                ktrepriv_appendstr(re, &chclass, tmpclass);
                ktrepriv_appendstr(re, &chclass, newtmpclass);
                _free(tmpclass);
                _free(newtmpclass);
                continue;
            }
            for(i = *tmpclass; i <= *newtmpclass; i++)
            {
                ktrepriv_appendchar(re, &chclass, i);
            }
            _free(newtmpclass);
        }
        else
        {
            ktrepriv_appendstr(re, &chclass, tmpclass);
        }
        _free(tmpclass);
    }
    if(*re->stackptr != ']')
    {
        ktrepriv_error(re, KTRE_ERROR_SYNTAX_ERROR, left->loc, "unterminated character class");
        _free(chclass);
        ktrepriv_freenode(re, left);
        return NULL;
    }
    if(!chclass)
    {
        ktrepriv_error(re, KTRE_ERROR_SYNTAX_ERROR, left->loc, "empty character class");
        ktrepriv_freenode(re, left);
        return NULL;
    }
    left->nodecharclass = chclass;
    /*
	 * Skip over the `]`.
	 */
    ktrepriv_nextchar(re);
    return left;
}

static ktrenode_t* ktrepriv_parseg(ktrecontext_t* re)
{
    int n;
    bool neg;
    bool pos;
    bool bracketed;
    ktrenode_t* left;
    left = ktrepriv_newnode(re);
    ktrepriv_nextchar(re);
    bracketed = *re->stackptr == '{';
    neg = false;
    pos = false;
    if(bracketed)
    {
        ktrepriv_nextchar(re);
        if(*re->stackptr == '+')
        {
            pos = true;
        }
        if(*re->stackptr == '-')
        {
            neg = true;
        }
        /* skip over the sign */
        if(pos || neg)
        {
            ktrepriv_nextchar(re);
        }
        n = ktrepriv_parsedecnum(re);
        if(*re->stackptr != '}' && !re->errcode)
        {
            ktrepriv_freenode(re, left);
            ktrepriv_error(re, KTRE_ERROR_SYNTAX_ERROR, left->loc, "incomplete token");
            return NULL;
        }
    }
    else
    {
        if(*re->stackptr == '+')
        {
            pos = true;
        }
        if(*re->stackptr == '-')
        {
            neg = true;
        }
        /* skip over the sign */
        if(pos || neg)
        {
            ktrepriv_nextchar(re);
        }
        n = ktrepriv_parsedecnum(re);
        re->stackptr--;
    }
    if(pos)
    {
        n = re->groupptr + n;
    }
    if(neg)
    {
        n = re->groupptr - n;
    }
    left->type = NODE_BACKREF;
    left->othergroupindex = n;
    return left;
}

static ktrenode_t* ktrepriv_parsek(ktrecontext_t* re)
{
    int i;
    bool bracketed;
    const char* sptr;
    const char* newsptr;
    ktrenode_t* left;
    left = ktrepriv_newnode(re);
    bracketed = *re->stackptr == '<';
    ktrepriv_nextchar(re);
    sptr = re->stackptr;
    while(*re->stackptr && strchr(KTRE_WORD, *re->stackptr))
    {
        re->stackptr++;
    }
    newsptr = re->stackptr;
    if((bracketed && *re->stackptr != '>') || (!bracketed && *re->stackptr != '\'') || sptr == newsptr)
    {
        ktrepriv_error(re, KTRE_ERROR_SYNTAX_ERROR, re->stackptr - re->sourcepattern, "expected '>' or '");
        ktrepriv_freenode(re, left);
        return NULL;
    }
    left->type = NODE_BACKREF;
    left->othergroupindex = -1;
    for(i = 0; i < re->groupptr; i++)
    {
        if(!re->group[i].name)
        {
            continue;
        }
        if(!strncmp(re->group[i].name, sptr, newsptr - sptr) && newsptr - sptr == (int)strlen(re->group[i].name))
        {
            left->othergroupindex = i;
            break;
        }
    }
    if(left->othergroupindex < 0)
    {
        ktrepriv_error(re, KTRE_ERROR_SYNTAX_ERROR, re->stackptr - re->sourcepattern, "name references a group that does not exist");
        ktrepriv_freenode(re, left);
        return NULL;
    }
    return left;
}

static ktrenode_t* ktrepriv_parseprimary(ktrecontext_t* re)
{
    int i;
    int j;
    int loc;
    int anum;
    bool neg;
    bool pos;
    bool bracketed;
    char* tmpstr;
    ktrenode_t* left;
    left = ktrepriv_newnode(re);
    if(!left)
    {
        return NULL;
    }
    loc = re->stackptr - re->sourcepattern;
    if(*re->stackptr == ')')
    {
        ktrepriv_freenode(re, left);
        return NULL;
    }
again:
    if(re->shouldescapemeta)
    {
        if(*re->stackptr == '\\' && re->stackptr[1] == 'E')
        {
            re->shouldescapemeta = false;
            re->stackptr += 2;
            goto again;
        }
        left->type = NODE_CHAR;
        left->othergroupindex = *re->stackptr;
        ktrepriv_nextchar(re);
        return left;
    }
    if(*re->stackptr != '\\')
    {
        switch(*re->stackptr)
        {
            case '[':
                { /* character classes */
                    ktrepriv_freenode(re, left);
                    ktrepriv_nextchar(re);
                    left = ktrepriv_parsecharacterclass(re);
                }
                break;
            case '(':
                {
                    ktrepriv_freenode(re, left);
                    left = ktrepriv_parsegroup(re);
                }
                break;
            case '.':
                {
                    ktrepriv_nextchar(re);
                    left->type = NODE_ANY;
                }
                break;
            case '^':
                {
                    ktrepriv_nextchar(re);
                    left->type = NODE_BOL;
                }
                break;
            case '$':
                {
                    ktrepriv_nextchar(re);
                    left->type = NODE_EOL;
                }
                break;
            case '#': /* extended mode comments */
                {
                    if(re->popt & KTRE_EXTENDED)
                    {
                        while(*re->stackptr && *re->stackptr != '\n')
                        {
                            ktrepriv_nextchar(re);
                        }
                    }
                    else
                    {
                        left->type = NODE_CHAR;
                        left->othergroupindex = *re->stackptr;
                        ktrepriv_nextchar(re);
                    }
                }
                break;
            default:
                {
                    if(re->popt & KTRE_EXTENDED && strchr(KTRE_WHITESPACECHARS, *re->stackptr))
                    {
                        /* ignore whitespace */
                        while(*re->stackptr && strchr(KTRE_WHITESPACECHARS, *re->stackptr))
                        {
                            ktrepriv_nextchar(re);
                        }
                        if(*re->stackptr)
                        {
                            goto again;
                        }
                    }
                    else
                    {
                        left->type = NODE_CHAR;
                        left->othergroupindex = *re->stackptr;
                        ktrepriv_nextchar(re);
                    }
                }
                /* fallthrough (?) */
        }
        if(left)
        {
            left->loc = loc;
        }
        return left;
    }
    left->type = NODE_CLASS;
    tmpstr = NULL;
    ktrepriv_nextchar(re);
    switch(*re->stackptr)
    {
        case 'x':
            {
                ktrepriv_nextchar(re);
                loc = re->stackptr - re->sourcepattern;
                bracketed= *re->stackptr == '{';
                if(bracketed)
                {
                    ktrepriv_nextchar(re);
                    ktrepriv_appendchar(re, &tmpstr, ktrepriv_parsehexnum(re));
                    if(*re->stackptr != '}' && !re->errcode)
                    {
                        if(tmpstr)
                        {
                            _free(tmpstr);
                        }
                        ktrepriv_error(re, KTRE_ERROR_SYNTAX_ERROR, loc, "incomplete token");
                        return NULL;
                    }
                }
                else
                {
                    ktrepriv_appendchar(re, &tmpstr, ktrepriv_parsehexnum(re));
                    re->stackptr--;
                }
            }
            break;
        case '-': /* backreferences */
        case '+':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            {

                neg= (*re->stackptr == '-');
                pos = (*re->stackptr == '+');
                if(neg || pos)
                {
                    /* skip over the sign */
                    ktrepriv_nextchar(re);
                }
                anum = ktrepriv_parsedecnum(re);
                if(neg)
                {
                    anum = re->groupptr - anum;
                }
                if(pos)
                {
                    anum = re->groupptr + anum;
                }
                left->type = NODE_BACKREF;
                left->othergroupindex = anum;
                re->stackptr--;
            }
            break;
        case 'o':
            {
                ktrepriv_nextchar(re);
                loc = re->stackptr - re->sourcepattern;
                if(*re->stackptr != '{' && !re->errcode)
                {
                    ktrepriv_freenode(re, left);
                    ktrepriv_error(re, KTRE_ERROR_SYNTAX_ERROR, loc, "expected '{'");
                    return NULL;
                }
                ktrepriv_nextchar(re);
                ktrepriv_appendchar(re, &tmpstr, ktrepriv_parseoctnum(re));
                if(*re->stackptr != '}' && !re->errcode)
                {
                    ktrepriv_freenode(re, left);
                    ktrepriv_error(re, KTRE_ERROR_SYNTAX_ERROR, loc, "unmatched '{'");
                    return NULL;
                }
            }
            break;
        case 'a':
            {
                ktrepriv_appendchar(re, &tmpstr, '\a');
            }
            break;
        case 'f':
            {
                ktrepriv_appendchar(re, &tmpstr, '\f');
            }
            break;
        case 'n':
            {
                ktrepriv_appendchar(re, &tmpstr, '\n');
            }
            break;
        case 't':
            {
                ktrepriv_appendchar(re, &tmpstr, '\t');
            }
            break;
        case 'r':
            {
                ktrepriv_appendchar(re, &tmpstr, '\r');
            }
            break;
        case 'h':
            {
                ktrepriv_appendstr(re, &tmpstr, " \t");
            }
            break;
        case 'e':
            {
                ktrepriv_appendchar(re, &tmpstr, 7);
            }
            break;
        case 's':
            {
                left->type = NODE_SPACE;
            }
            break;
        case 'S':
            {
                left->type = NODE_NOT;
                left->nodecharclass = ktrepriv_strclone(re, KTRE_WHITESPACECHARS);
            }
            break;
        case 'd':
            {
                left->type = NODE_DIGIT;
            }
            break;
        case 'D':
            {
                left->type = NODE_NOT;
                left->nodecharclass = ktrepriv_strclone(re, KTRE_DIGIT);
            }
            break;
        case 'w':
            {
                left->type = NODE_WORD;
            }
            break;
        case 'W':
            {
                left->type = NODE_NOT;
                left->nodecharclass = ktrepriv_strclone(re, KTRE_WORD);
            }
            break;
        case 'K':
            {
                left->type = NODE_SET_START;
            }
            break;
        case 'b':
            {
                left->type = NODE_WB;
            }
            break;
        case 'B':
            {
                left->type = NODE_NWB;
            }
            break;
        case 'A':
            {
                left->type = NODE_BOS;
            }
            break;
        case 'Z':
            {
                left->type = NODE_EOS;
            }
            break;
        case 'Q':
            {
                re->shouldescapemeta = true;
                ktrepriv_nextchar(re);
                goto again;
            }
            break;
        case 'E':
            {
                re->shouldescapemeta = false;
                ktrepriv_nextchar(re);
                goto again;
            }
            break;
        case 'H':
            {
                tmpstr = (char*)_malloc(256);
                i = 0;
                for(j = 1; j < 256; j++)
                {
                    if(!strchr("\t ", j))
                    {
                        tmpstr[i++] = j;
                    }
                }
                tmpstr[i] = 0;
            }
            break;
        case 'N':
            {
                tmpstr = (char*)_malloc(256);
                i = 0;
                for(j = 1; j < 256; j++)
                {
                    if(j != '\n')
                    {
                        tmpstr[i++] = j;
                    }
                }
                tmpstr[i] = 0;
            }
            break;
        case 'g':
            {
                ktrepriv_freenode(re, left);
                ktrepriv_nextchar(re);
                left = ktrepriv_parseg(re);
            }
            break;
        case 'k':
            {
                ktrepriv_freenode(re, left);
                ktrepriv_nextchar(re);
                left = ktrepriv_parsek(re);
            }
            break;
        default:
            {
                ktrepriv_appendchar(re, &tmpstr, *re->stackptr);
            }
            break;
    }
    ktrepriv_nextchar(re);
    if(left && !left->nodecharclass)
    {
        left->nodecharclass = tmpstr;
    }
    if(left)
    {
        left->loc = loc;
    }
    return left;
}


static ktrenode_t* ktrepriv_factor(ktrecontext_t* re)
{
    ktrenode_t* left;
    ktrenode_t* nnode;
    if(!*re->stackptr)
    {
        return NULL;
    }
    left = ktrepriv_parseprimary(re);
    while(*re->stackptr && (*re->stackptr == '*' || *re->stackptr == '+' || *re->stackptr == '?' || *re->stackptr == '{'))
    {
        nnode = ktrepriv_newnode(re);
        switch(*re->stackptr)
        {
            case '*':
                {
                    nnode->type = NODE_ASTERISK;
                    ktrepriv_nextchar(re);
                }
                break;
            case '?':
                {
                    nnode->type = NODE_QUESTION;
                    ktrepriv_nextchar(re);
                }
                break;
            case '+':
                {
                    nnode->type = NODE_PLUS;
                    ktrepriv_nextchar(re);
                }
                break;
            case '{':
            { /* counted repetition */
                nnode->type = NODE_REP;
                nnode->othergroupindex = -1;
                nnode->decnum = 0;
                ktrepriv_nextchar(re);
                nnode->othergroupindex = ktrepriv_parsedecnum(re);
                if(*re->stackptr == ',')
                {
                    ktrepriv_nextchar(re);
                    if(isdigit(*re->stackptr))
                    {
                        nnode->decnum = ktrepriv_parsedecnum(re);
                    }
                    else
                    {
                        nnode->decnum = -1;
                    }
                }
                if(*re->stackptr != '}')
                {
                    ktrepriv_error(re, KTRE_ERROR_SYNTAX_ERROR, re->stackptr - re->sourcepattern - 1, "unmatched '{'");
                    ktrepriv_freenode(re, nnode);
                    ktrepriv_freenode(re, left);
                    return NULL;
                }
                ktrepriv_nextchar(re);
            }
            break;
        }
        nnode->firstnode = left;
        left = nnode;
    }
    if(left)
    {
        left->loc = re->stackptr - re->sourcepattern - 1;
    }
    return left;
}

static ktrenode_t* ktrepriv_term(ktrecontext_t* re)
{
    char ogidx;
    ktrenode_t* left;
    ktrenode_t* right;
    left = ktrepriv_newnode(re);
    left->type = NODE_NONE;
    while(*re->stackptr && *re->stackptr != '|' && *re->stackptr != ')')
    {
        right = ktrepriv_factor(re);
        if(re->errcode)
        {
            ktrepriv_freenode(re, left);
            ktrepriv_freenode(re, right);
            return NULL;
        }
        if(left->type == NODE_NONE)
        {
            _free(left);
            left = right;
        }
        else
        {
            if((left->type == NODE_CHAR || left->type == NODE_STR) && right->type == NODE_CHAR)
            {
                if(left->type == NODE_CHAR)
                {
                    ogidx = left->othergroupindex;
                    left->type = NODE_STR;
                    left->nodecharclass = (char*)_malloc(3);
                    re->info.parser_alloc += 3;
                    if(re->popt & KTRE_INSENSITIVE)
                    {
                        left->nodecharclass[0] = ktrepriv_lc(ogidx);
                        left->nodecharclass[1] = ktrepriv_lc(right->othergroupindex);
                    }
                    else
                    {
                        left->nodecharclass[0] = ogidx;
                        left->nodecharclass[1] = right->othergroupindex;
                    }
                    left->nodecharclass[2] = 0;
                    ktrepriv_freenode(re, right);
                }
                else
                {
                    if(re->popt & KTRE_INSENSITIVE)
                    {
                        ktrepriv_appendchar(re, &left->nodecharclass, ktrepriv_lc(right->othergroupindex));
                    }
                    else
                    {
                        ktrepriv_appendchar(re, &left->nodecharclass, right->othergroupindex);
                    }
                    ktrepriv_freenode(re, right);
                }
            }
            else if(right->type == NODE_CHAR && left->type == NODE_SEQUENCE && (left->secondnode->type == NODE_CHAR || left->secondnode->type == NODE_STR))
            {
                if(left->secondnode->type == NODE_CHAR)
                {
                    ogidx = left->secondnode->othergroupindex;
                    left->secondnode->type = NODE_STR;
                    left->secondnode->nodecharclass = (char*)_malloc(3);
                    re->info.parser_alloc += 3;
                    if(re->popt & KTRE_INSENSITIVE)
                    {
                        left->secondnode->nodecharclass[0] = ktrepriv_lc(ogidx);
                        left->secondnode->nodecharclass[1] = ktrepriv_lc(right->othergroupindex);
                    }
                    else
                    {
                        left->secondnode->nodecharclass[0] = ogidx;
                        left->secondnode->nodecharclass[1] = right->othergroupindex;
                    }
                    left->secondnode->nodecharclass[2] = 0;
                    ktrepriv_freenode(re, right);
                }
                else
                {
                    if(re->popt & KTRE_INSENSITIVE)
                    {
                        ktrepriv_appendchar(re, &left->secondnode->nodecharclass, ktrepriv_lc(right->othergroupindex));
                    }
                    else
                    {
                        ktrepriv_appendchar(re, &left->secondnode->nodecharclass, right->othergroupindex);
                    }
                    ktrepriv_freenode(re, right);
                }
            }
            else
            {
                ktrenode_t* tmp = ktrepriv_newnode(re);
                if(!tmp)
                {
                    ktrepriv_freenode(re, left);
                    ktrepriv_freenode(re, right);
                    return NULL;
                }
                tmp->loc = re->stackptr - re->sourcepattern;
                tmp->firstnode = left;
                tmp->secondnode = right;
                tmp->type = NODE_SEQUENCE;
                left = tmp;
            }
        }
    }
    return left;
}

static ktrenode_t* ktrepriv_parse(ktrecontext_t* re)
{
    ktrenode_t* node;
    ktrenode_t* newnode;
    node = ktrepriv_term(re);
    if(*re->stackptr && *re->stackptr == '|')
    {
        ktrepriv_nextchar(re);
        newnode = ktrepriv_newnode(re);
        newnode->type = NODE_OR;
        newnode->firstnode = node;
        newnode->secondnode = ktrepriv_parse(re);
        if(re->errcode)
        {
            ktrepriv_freenode(re, newnode->secondnode);
            newnode->secondnode = NULL;
            return newnode;
        }
        return newnode;
    }
    return node;
}

/*/
#define N0(...)             \
    do                      \
    {                       \
        DBG(__VA_ARGS__);   \
        DBG(" %d", n->loc); \
    } while(0)
*/
#define N0(...)

#define N1(...) \
    do \
    { \
        DBG(__VA_ARGS__); \
        DBG(" %d", n->loc); \
        l++; \
        ktre_printnode(re, n->firstnode); \
        l--; \
    } while(0)

#define N2(...) \
    do \
    { \
        DBG(__VA_ARGS__); \
        DBG(" %d", n->loc); \
        l++; \
        arm[l - 1] = 1; \
        ktre_printnode(re, n->firstnode); \
        arm[l - 1] = 0; \
        ktre_printnode(re, n->secondnode); \
        l--; \
    } while(0)

void ktre_printnode(ktrecontext_t* re, ktrenode_t* n)
{
    int i;
    static int depth = 0;
    static int l = 0;
    static int arm[2048] = { 0 };
    if(depth > 100)
    {
        return;
    }
    depth++;
    DBG("\n");
    arm[l] = 0;
    for(i = 0; i < l - 1; i++)
    {
        if(arm[i])
        {
            DBG("|   ");
        }
        else
        {
            DBG("    ");
        }
    }
    if(l)
    {
        if(arm[l - 1])
        {
            DBG("|-- ");
        }
        else
        {
            DBG("`-- ");
        }
    }

    if(!n)
    {
        DBG("(null)");
        return;
    }
    switch(n->type)
    {
        case NODE_ANY:
            {
                N0("(any)");
            }
            break;
        case NODE_MANY:
            {
                N0("(m_any)");
            }
            break;
        case NODE_DIGIT:
            {
                N0("(digit)");
            }
            break;
        case NODE_WORD:
            {
                N0("(word)");
            }
            break;
        case NODE_SPACE:
            {
                N0("(space)");
            }
            break;
        case NODE_NONE:
            {
                N0("(none)");
            }
            break;
        case NODE_CHAR:
            {
                N0("(char '%c')", n->c);
            }
            break;
        case NODE_WB:
            {
                N0("(word boundary)");
            }
            break;
        case NODE_NWB:
            {
                N0("(negated word boundary)");
            }
            break;
        case NODE_BACKREF:
            {
                N0("(backreference to %d)", n->c);
            }
            break;
        case NODE_CLASS:
            {
                DBG("(class '");
                dbgf(n->nodecharclass);
                N0("')");
            }
            break;
        case NODE_STR:
            {
                DBG("(string '");
                dbgf(n->nodecharclass);
                N0("')");
            }
            break;
        case NODE_NOT:
            {
                DBG("(not '");
                dbgf(n->nodecharclass);
                N0("')");
            }
            break;
        case NODE_BOL:
            {
                N0("(bol)");
            }
            break;
        case NODE_EOL:
            {
                N0("(eol)");
            }
            break;
        case NODE_BOS:
            {
                N0("(bos)");
            }
            break;
        case NODE_EOS:
            {
                N0("(eos)");
            }
            break;
        case NODE_RECURSE:
            {
                N0("(recurse)");
            }
            break;
        case NODE_SET_START:
            {
                N0("(set_start)");
            }
            break;
        case NODE_SETOPT:
            {
                N0("(setopt %d)", n->c);
            }
            break;
        case NODE_CALL:
            {
                N0("(call %d)", n->c);
            }
            break;
        case NODE_SEQUENCE:
            {
                N2("(sequence)");
            }
            break;
        case NODE_OR:
            {
                N2("(or)");
            }
            break;
        case NODE_REP:
            {
                N1("(counted repetition %d - %d)", n->c, n->d);
            }
            break;
        case NODE_ASTERISK:
            {
                N1("(asterisk)");
            }
            break;
        case NODE_PLUS:
            {
                N1("(plus)");
            }
            break;
        case NODE_QUESTION:
            {
                N1("(question)");
            }
            break;
        case NODE_ATOM:
            {
                N1("(atom)");
            }
            break;
        case NODE_PLA:
            {
                N1("(lookahead)");
            }
            break;
        case NODE_NLA:
            {
                N1("(negative lookahead)");
            }
            break;
        case NODE_PLB:
            {
                N1("(lookbehind)");
            }
            break;
        case NODE_NLB:
            {
                N1("(negative lookbehind)");
            }
            break;
        case NODE_GROUP:
            {
                if(re->group[n->groupindex].name)
                {
                    N1("(group '%s')", re->group[n->gi].name);
                }
                else
                {
                    N1("(group %d)", n->gi);
                }
            }
            break;
        default:
            {
                DBG("\nunimplemented printer for node of type %d\n", n->type);
                assert(false);
            }
            break;
    }
    depth--;
}

static bool ktrepriv_isiteratable(ktrenode_t* n)
{
    switch(n->type)
    {
        case NODE_SETOPT:
            {
                return false;
            }
            break;
        default:
            break;
    }
    return true;
}

#define PATCH_A(loc, _a) \
    if(re->compiled) \
    { \
        re->compiled[loc].firstval = _a; \
    }

#define PATCH_B(loc, _b) \
    if(re->compiled) \
    { \
        re->compiled[loc].secondval = _b; \
    }

#define PATCH_C(loc, _c) \
    if(re->compiled)     \
    { \
        re->compiled[loc].thirdval = _c; \
    }
    
static void ktrepriv_compile(ktrecontext_t* re, ktrenode_t* n, bool rev)
{
    int i;
    int a;
    int b;
    char* str;
    a = -1;
    b = -1;
    if(!n)
    {
        return;
    }
    if(n->type == NODE_ASTERISK || n->type == NODE_PLUS || n->type == NODE_QUESTION)
    {
        if(!ktrepriv_isiteratable(n->firstnode))
        {
            ktrepriv_error(re, KTRE_ERROR_SYNTAX_ERROR, n->loc, "iteration on non-iteratable value");
            return;
        }
    }
    switch(n->type)
    {
        case NODE_ASTERISK:
            {
                a = re->instrpos;
                ktrepriv_emitab(re, KTRE_INSTR_BRANCH, re->instrpos + 1, -1, n->loc);
                ktrepriv_emitc(re, KTRE_INSTR_PROG, re->compcount++, n->loc);
                ktrepriv_compile(re, n->firstnode, rev);
                ktrepriv_emitab(re, KTRE_INSTR_BRANCH, a + 1, re->instrpos + 1, n->loc);
                PATCH_B(a, re->instrpos);
            }
            break;
        case NODE_QUESTION:
            {
                switch(n->firstnode->type)
                {
                    case NODE_ASTERISK:
                        {
                            a = re->instrpos;
                            ktrepriv_emitab(re, KTRE_INSTR_BRANCH, -1, re->instrpos + 1, n->loc);
                            ktrepriv_emitc(re, KTRE_INSTR_PROG, re->compcount++, n->loc);
                            ktrepriv_compile(re, n->firstnode->firstnode, rev);
                            ktrepriv_emitab(re, KTRE_INSTR_BRANCH, re->instrpos + 1, a + 1, n->loc);
                            PATCH_A(a, re->instrpos);
                        }
                        break;
                    case NODE_PLUS:
                        {
                            a = re->instrpos;
                            ktrepriv_emitc(re, KTRE_INSTR_PROG, re->compcount++, n->loc);
                            ktrepriv_compile(re, n->firstnode->firstnode, rev);
                            ktrepriv_emitab(re, KTRE_INSTR_BRANCH, re->instrpos + 1, a, n->loc);
                        }
                        break;
                    case NODE_QUESTION:
                        {
                            a = re->instrpos;
                            ktrepriv_emitab(re, KTRE_INSTR_BRANCH, -1, re->instrpos + 1, n->loc);
                            ktrepriv_emitc(re, KTRE_INSTR_PROG, re->compcount++, n->loc);
                            ktrepriv_compile(re, n->firstnode->firstnode, rev);
                            PATCH_A(a, re->instrpos);
                        }
                        break;
                    default:
                        {
                            a = re->instrpos;
                            ktrepriv_emitab(re, KTRE_INSTR_BRANCH, re->instrpos + 1, -1, n->loc);
                            ktrepriv_compile(re, n->firstnode, rev);
                            PATCH_B(a, re->instrpos);
                        }
                        break;
                }
            }
            break;
        case NODE_GROUP:
            {
                if(re->group[n->groupindex].iscalled && !re->group[n->groupindex].iscompiled)
                {
                    ktrepriv_emitc(re, KTRE_INSTR_CALL, re->instrpos + 3, n->loc);
                    ktrepriv_emitc(re, KTRE_INSTR_SAVE, n->groupindex * 2 + 1, n->loc);
                    a = re->instrpos;
                    ktrepriv_emitc(re, KTRE_INSTR_JMP, -1, n->loc);
                    ktrepriv_emitc(re, KTRE_INSTR_SAVE, n->groupindex * 2, n->loc);
                    re->group[n->groupindex].address = re->instrpos - 1;
                    re->numgroups++;
                    ktrepriv_compile(re, n->firstnode, rev);
                    ktrepriv_emit(re, KTRE_INSTR_RET, n->loc);
                    PATCH_C(a, re->instrpos);
                    re->group[n->groupindex].iscompiled = true;
                }
                else if(re->group[n->groupindex].iscompiled)
                {
                    ktrepriv_emitc(re, KTRE_INSTR_SAVE, n->groupindex * 2, n->loc);
                    ktrepriv_compile(re, n->firstnode, rev);
                    ktrepriv_emitc(re, KTRE_INSTR_SAVE, n->groupindex * 2 + 1, n->loc);
                }
                else
                {
                    ktrepriv_emitc(re, KTRE_INSTR_SAVE, n->groupindex * 2, n->loc);
                    re->numgroups++;
                    re->group[n->groupindex].address = re->instrpos - 1;
                    ktrepriv_compile(re, n->firstnode, rev);
                    ktrepriv_emitc(re, KTRE_INSTR_SAVE, n->groupindex * 2 + 1, n->loc);
                    re->group[n->groupindex].iscompiled = true;
                }
            }
            break;
        case NODE_CALL:
            {
                ktrepriv_emitc(re, KTRE_INSTR_CALL, re->group[n->othergroupindex].address + 1, n->loc);
            }
            break;
        case NODE_PLUS:
            {
                switch(n->firstnode->type)
                {
                    case NODE_ASTERISK: /* fall through */
                    case NODE_PLUS:
                    case NODE_QUESTION:
                    case NODE_REP:
                        {
                            ktrepriv_emit(re, KTRE_INSTR_TRY, n->loc);
                            ktrepriv_emitc(re, KTRE_INSTR_PROG, re->compcount++, n->loc);
                            ktrepriv_compile(re, n->firstnode, rev);
                            ktrepriv_emit(re, KTRE_INSTR_CATCH, n->loc);
                        }
                        break;
                    default:
                        {
                            a = re->instrpos;
                            ktrepriv_emitc(re, KTRE_INSTR_PROG, re->compcount++, n->loc);
                            ktrepriv_compile(re, n->firstnode, rev);
                            ktrepriv_emitab(re, KTRE_INSTR_BRANCH, a, re->instrpos + 1, n->loc);
                        }
                        break;
                }
            }
            break;
        case NODE_OR:
            {
                a = re->instrpos;
                ktrepriv_emitab(re, KTRE_INSTR_BRANCH, re->instrpos + 1, -1, n->loc);
                ktrepriv_compile(re, n->firstnode, rev);
                b = re->instrpos;
                ktrepriv_emitc(re, KTRE_INSTR_JMP, -1, n->loc);
                PATCH_B(a, re->instrpos);
                ktrepriv_compile(re, n->secondnode, rev);
                PATCH_C(b, re->instrpos);
            }
            break;
        case NODE_SEQUENCE:
            {
                if(rev)
                {
                    ktrepriv_compile(re, n->secondnode, rev);
                    ktrepriv_compile(re, n->firstnode, rev);
                }
                else
                {
                    ktrepriv_compile(re, n->firstnode, rev);
                    ktrepriv_compile(re, n->secondnode, rev);
                }
            }
            break;
        case NODE_RECURSE:
            {
                ktrepriv_emitc(re, KTRE_INSTR_CALL, re->group[0].address + 1, n->loc);
            }
            break;
        case NODE_BACKREF:
            {
                if(n->othergroupindex <= 0 || n->othergroupindex >= re->numgroups)
                {
                    ktrepriv_error(re, KTRE_ERROR_SYNTAX_ERROR, n->loc, "backreference number is invalid or references a group that does not yet exist");
                    return;
                }
                if(!re->group[n->othergroupindex].iscompiled)
                {
                    ktrepriv_error(re, KTRE_ERROR_SYNTAX_ERROR, n->loc, "backreferences may not reference the group they occur in");
                    return;
                }
                ktrepriv_emitc(re, KTRE_INSTR_BACKREF, n->othergroupindex, n->loc);
            }
            break;
        case NODE_REP:
            {
                a = 0;
                for(i = 0; i < n->othergroupindex; i++)
                {
                    a = re->instrpos;
                    if(n->firstnode->type == NODE_GROUP && !re->group[n->firstnode->groupindex].iscompiled)
                    {
                        ktrepriv_compile(re, n->firstnode, rev);
                    }
                    else if(n->firstnode->type == NODE_GROUP)
                    {
                        ktrepriv_emitc(re, KTRE_INSTR_CALL, re->group[n->firstnode->groupindex].address + 1, n->loc);
                    }
                    else
                    {
                        if(n->firstnode->type == NODE_CHAR)
                        {
                            str = (char*)_malloc(n->othergroupindex + 1);
                            if(!str)
                            {
                                return;
                            }
                            re->info.parser_alloc += n->othergroupindex + 1;
                            for(i = 0; i < n->othergroupindex; i++)
                            {
                                str[i] = n->firstnode->othergroupindex;
                            }
                            str[n->othergroupindex] = 0;
                            ktrepriv_emitclass(re, KTRE_INSTR_TSTR, str, n->loc);
                            break;
                        }
                        else
                        {
                            ktrepriv_compile(re, n->firstnode, rev);
                        }
                    }
                }
                if(n->decnum == -1)
                {
                    if(n->firstnode->type == NODE_GROUP)
                    {
                        /* basically just manually ktrepriv_emit the
                     * bytecode for * */
                        ktrepriv_emitab(re, KTRE_INSTR_BRANCH, re->instrpos + 1, re->instrpos + 2, n->loc);
                        ktrepriv_emitc(re, KTRE_INSTR_CALL, re->group[n->firstnode->groupindex].address + 1, n->loc);
                        ktrepriv_emitab(re, KTRE_INSTR_BRANCH, re->instrpos - 1, re->instrpos + 1, n->loc);
                    }
                    else
                    {
                        ktrepriv_emitab(re, KTRE_INSTR_BRANCH, a, re->instrpos + 1, n->loc);
                    }
                }
                else
                {
                    for(i = 0; i < n->decnum - n->othergroupindex; i++)
                    {
                        a = re->instrpos;
                        ktrepriv_emitab(re, KTRE_INSTR_BRANCH, re->instrpos + 1, -1, n->loc);
                        if(n->firstnode->type == NODE_GROUP)
                        {
                            ktrepriv_emitc(re, KTRE_INSTR_CALL, re->group[n->firstnode->groupindex].address + 1, n->loc);
                        }
                        else
                        {
                            ktrepriv_compile(re, n->firstnode, rev);
                        }
                        PATCH_B(a, re->instrpos);
                    }
                }
            }
            break;
        case NODE_ATOM:
            {
                ktrepriv_emit(re, KTRE_INSTR_TRY, n->loc);
                ktrepriv_compile(re, n->firstnode, rev);
                ktrepriv_emit(re, KTRE_INSTR_CATCH, n->loc);
            }
            break;
        case NODE_PLA:
            {
                ktrepriv_emit(re, KTRE_INSTR_PLA, n->loc);
                ktrepriv_compile(re, n->firstnode, false);
                ktrepriv_emit(re, KTRE_INSTR_PLA_WIN, n->loc);
            }
            break;
        case NODE_NLA:
            {
                a = re->instrpos;
                ktrepriv_emit(re, KTRE_INSTR_NLA, n->loc);
                ktrepriv_compile(re, n->firstnode, false);
                ktrepriv_emit(re, KTRE_INSTR_NLA_FAIL, n->loc);
                PATCH_C(a, re->instrpos);
            }
            break;
        case NODE_PLB:
            {
                ktrepriv_emit(re, KTRE_INSTR_PLB, n->loc);
                ktrepriv_compile(re, n->firstnode, true);
                ktrepriv_emit(re, KTRE_INSTR_PLB_WIN, n->loc);
            }
            break;
        case NODE_NLB:
            {
                a = re->instrpos;
                ktrepriv_emitc(re, KTRE_INSTR_NLB, -1, n->loc);
                ktrepriv_compile(re, n->firstnode, true);
                ktrepriv_emit(re, KTRE_INSTR_NLB_FAIL, n->loc);
                PATCH_C(a, re->instrpos);
            }
            break;
        case NODE_CLASS:
            {
                ktrepriv_emitclass(re, KTRE_INSTR_CLASS, n->nodecharclass, n->loc);
            }
            break;
        case NODE_STR:
            {
                ktrepriv_emitclass(re, KTRE_INSTR_STR, n->nodecharclass, n->loc);
            }
            break;
        case NODE_NOT:
            {
                ktrepriv_emitclass(re, KTRE_INSTR_NOT, n->nodecharclass, n->loc);
            }
            break;
        case NODE_SETOPT:
            {
                ktrepriv_emitc(re, KTRE_INSTR_SETOPT, n->othergroupindex, n->loc);
            }
            break;
        case NODE_CHAR:
            {
                ktrepriv_emitc(re, KTRE_INSTR_CHAR, n->othergroupindex, n->loc);
            }
            break;
        case NODE_BOL:
            {
                ktrepriv_emit(re, KTRE_INSTR_BOL, n->loc);
            }
            break;
        case NODE_EOL:
            {
                ktrepriv_emit(re, KTRE_INSTR_EOL, n->loc);
            }
            break;
        case NODE_BOS:
            {
                ktrepriv_emit(re, KTRE_INSTR_BOS, n->loc);
            }
            break;
        case NODE_EOS:
            {
                ktrepriv_emit(re, KTRE_INSTR_EOS, n->loc);
            }
            break;
        case NODE_ANY:
            {
                ktrepriv_emit(re, KTRE_INSTR_ANY, n->loc);
            }
            break;
        case NODE_SET_START:
            {
                ktrepriv_emit(re, KTRE_INSTR_SET_START, n->loc);
            }
            break;
        case NODE_WB:
            {
                ktrepriv_emit(re, KTRE_INSTR_WB, n->loc);
            }
            break;
        case NODE_NWB:
            {
                ktrepriv_emit(re, KTRE_INSTR_NWB, n->loc);
            }
            break;
        case NODE_DIGIT:
            {
                ktrepriv_emit(re, KTRE_INSTR_DIGIT, n->loc);
            }
            break;
        case NODE_WORD:
            {
                ktrepriv_emit(re, KTRE_INSTR_WORD, n->loc);
            }
            break;
        case NODE_SPACE:
            {
                ktrepriv_emit(re, KTRE_INSTR_SPACE, n->loc);
            }
            break;
        case NODE_NONE:
            {
            }
            break;
        default:
            {
                DBG("\nunimplemented compiler for node of type %d\n", n->type);
                #ifdef KTRE_DEBUG
                    assert(false);
                #endif
            }
            break;
    }
}

void ktre_printcomperror(ktrecontext_t* re)
{
    int i;
    (void)re;
    (void)i;
#if KTRE_DEBUG
    DBG("\nfailed to compile with ktrepriv_error code %d: %s\n", re->err, re->err_str ? re->err_str : "no ktrepriv_error message");
    DBG("\t%s\n\t", re->pat);
    for(i = 0; i < re->loc; i++)
    {
        if(re->pat[i] == '\t')
        {
            DBG("\t");
        }
        else
        {
            DBG(" ");
        }
    }
    DBG("^");
#endif
}

static inline void ktre_printfinish(ktrecontext_t* re, const char* subject, const char* regex, bool ret, int** vec, const char* replaced)
{
    int i;
    int j;
    (void)i;
    (void)j;
    (void)re;
    (void)subject;
    (void)regex;
    (void)ret;
    (void)vec;
    (void)replaced;
#ifdef KTRE_DEBUG
    if(ret)
    {
        for(i = 0; i < re->num_matches; i++)
        {
            DBG("\nmatch %d: `%.*s`", i + 1, vec[i][1], subject + vec[i][0]);
            for(j = 1; j < re->num_groups; j++)
            {
                if(vec[i][j * 2 + 1] && vec[i][j * 2] != (int)strlen(subject))
                {
                    DBG("\ngroup %d: `%.*s`", j, vec[i][j * 2 + 1], subject + vec[i][j * 2]);
                }
            }
        }
        if(replaced)
        {
            DBG("\nreplace: `%s`", replaced);
        }
    }
    else if(re->err)
    {
        DBG("\nfailed at runtime with ktrepriv_error code %d: %s\n", re->err, re->err_str ? re->err_str : "no ktrepriv_error message");
        DBG("\t%s\n\t", regex);
        for(i = 0; i < re->loc; i++)
        {
            DBG(" ");
        }
        DBG("^");
    }
    else
    {
        DBG("\nno matches.");
    }
#endif
}

ktrecontext_t* ktre_compile(const char* pat, int opt)
{
    int i;
    int j;
    size_t oi;
    ktrecontext_t* re;
    ktrenode_t* n;
    (void)oi;
    (void)i;
    (void)j;
    if(!pat)
    {
        return NULL;
    }
    re = (ktrecontext_t*)KTRE_MALLOC(sizeof *re);
    memset(re, 0, sizeof *re);
    if(opt & KTRE_GLOBAL)
    {
        opt |= KTRE_UNANCHORED;
    }
#ifdef KTRE_DEBUG
    DBG("regexpr: %s", pat);
    if(opt)
    {
        DBG("\noptions:");
    }
    for(oi = 0; i < sizeof opt; oi++)
    {
        switch(opt & 1 << oi)
        {
            case KTRE_INSENSITIVE:
                DBG("\n\tINSENSITIVE");
                break;
            case KTRE_UNANCHORED:
                DBG("\n\tUNANCHORED");
                break;
            case KTRE_EXTENDED:
                DBG("\n\tEXTENDED");
                break;
            case KTRE_GLOBAL:
                DBG("\n\tGLOBAL");
                break;
            case KTRE_CONTINUE:
                DBG("\n\tCONTINUE");
                break;
            default:
                continue;
        }
    }
#endif
    re->sourcepattern = pat;
    re->opt = opt;
    re->stackptr = pat;
    re->popt = opt;
    re->max_tp = -1;
    //re->errstr = "no ktrepriv_error";
    re->errstr = NULL;
    re->headnode = ktrepriv_newnode(re);
    if((opt & KTRE_CONTINUE) && (opt & KTRE_GLOBAL))
    {
        ktrepriv_error(re, KTRE_ERROR_INVALID_OPTIONS, 0, "invalid option configuration: /c and /g may not be used together");
        re->headnode = NULL;
        ktre_printcomperror(re);
        return re;
    }
    if(!re->headnode)
    {
        re->headnode = NULL;
        ktre_printcomperror(re);
        return re;
    }
    re->headnode->loc = 0;
    re->headnode->type = NODE_GROUP;
    re->headnode->groupindex = ktrepriv_addgroup(re);
    if(re->headnode->groupindex < 0)
    {
        re->headnode = NULL;
        ktre_printcomperror(re);
        return re;
    }
    re->group[0].iscompiled = false;
    re->group[0].iscalled = false;
    re->headnode->firstnode = NULL;
    n = ktrepriv_parse(re);
    re->headnode->firstnode = n;
    if(re->errcode)
    {
        ktrepriv_freenode(re, re->headnode);
        re->headnode = NULL;
        ktre_printcomperror(re);
        return re;
    }
    if(*re->stackptr)
    {
        ktrepriv_error(re, KTRE_ERROR_SYNTAX_ERROR, re->stackptr - re->sourcepattern, "unmatched righthand delimiter");
        ktre_printcomperror(re);
        return re;
    }
#ifdef KTRE_DEBUG
    ktre_printnode(re, re->n);
#endif
    if(re->opt & KTRE_UNANCHORED)
    {
        /*
		 * To implement unanchored matching (the default for
		 * most engines) we could have the VM just try to
		 * match the regex at every position in the string,
		 * but that's inefficient. Instead, we'll just bake
		 * the unanchored matching right into the bytecode by
		 * manually emitting the instructions for `.*?`.
		 */
        ktrepriv_emitab(re, KTRE_INSTR_BRANCH, 3, 1, 0);
        ktrepriv_emit(re, KTRE_INSTR_MANY, 0);
        ktrepriv_emitab(re, KTRE_INSTR_BRANCH, 3, 1, 0);
    }
    ktrepriv_compile(re, re->headnode, false);
    re->numgroups = re->groupptr;
    if(re->errcode)
    {
        ktre_printcomperror(re);
        return re;
    }
    ktrepriv_emit(re, KTRE_INSTR_MATCH, re->stackptr - re->sourcepattern);
#ifdef KTRE_DEBUG
    for(i = 0; i < re->ip; i++)
    {
        for(j = 0; j < re->num_groups; j++)
        {
            if(re->group[j].address == i)
                DBG("\ngroup %d:", j);
        }
        DBG("\n%3d: [%3d] ", i, re->compiled[i].loc);
        switch(re->compiled[i].op)
        {
            case KTRE_INSTR_CLASS:
                DBG("CLASS   '");
                dbgf(re->compiled[i].charclass);
                DBG("'");
                break;
            case KTRE_INSTR_STR:
                DBG("STR     '");
                dbgf(re->compiled[i].charclass);
                DBG("'");
                break;
            case KTRE_INSTR_NOT:
                DBG("NOT     '");
                dbgf(re->compiled[i].charclass);
                DBG("'");
                break;
            case KTRE_INSTR_TSTR:
                DBG("TSTR    '");
                dbgf(re->compiled[i].charclass);
                DBG("'");
                break;
            case KTRE_INSTR_BRANCH:
                DBG("BRANCH   %d, %d", re->compiled[i].a, re->compiled[i].b);
                break;
            case KTRE_INSTR_CHAR:
                DBG("CHAR    '%c'", re->compiled[i].a);
                break;
            case KTRE_INSTR_SAVE:
                DBG("SAVE     %d", re->compiled[i].a);
                break;
            case KTRE_INSTR_JMP:
                DBG("JMP      %d", re->compiled[i].a);
                break;
            case KTRE_INSTR_SETOPT:
                DBG("SETOPT   %d", re->compiled[i].a);
                break;
            case KTRE_INSTR_BACKREF:
                DBG("BACKREF  %d", re->compiled[i].a);
                break;
            case KTRE_INSTR_CALL:
                DBG("CALL     %d", re->compiled[i].a);
                break;
            case KTRE_INSTR_PROG:
                DBG("PROG     %d", re->compiled[i].a);
                break;
            case KTRE_INSTR_SET_START:
                DBG("SET_START");
                break;
            case KTRE_INSTR_TRY:
                DBG("TRY");
                break;
            case KTRE_INSTR_CATCH:
                DBG("CATCH");
                break;
            case KTRE_INSTR_ANY:
                DBG("ANY");
                break;
            case KTRE_INSTR_MANY:
                DBG("MANY");
                break;
            case KTRE_INSTR_DIGIT:
                DBG("DIGIT");
                break;
            case KTRE_INSTR_WORD:
                DBG("WORD");
                break;
            case KTRE_INSTR_SPACE:
                DBG("SPACE");
                break;
            case KTRE_INSTR_BOL:
                DBG("BOL");
                break;
            case KTRE_INSTR_EOL:
                DBG("EOL");
                break;
            case KTRE_INSTR_BOS:
                DBG("BOS");
                break;
            case KTRE_INSTR_EOS:
                DBG("EOS");
                break;
            case KTRE_INSTR_RET:
                DBG("RET");
                break;
            case KTRE_INSTR_WB:
                DBG("WB");
                break;
            case KTRE_INSTR_NWB:
                DBG("NWB");
                break;
            case KTRE_INSTR_MATCH:
                DBG("MATCH");
                break;
            case KTRE_INSTR_PLA:
                DBG("PLA");
                break;
            case KTRE_INSTR_PLA_WIN:
                DBG("PLA_WIN");
                break;
            case KTRE_INSTR_NLA:
                DBG("NLA      %d", re->compiled[i].a);
                break;
            case KTRE_INSTR_NLA_FAIL:
                DBG("NLA_FAIL");
                break;
            case KTRE_INSTR_PLB:
                DBG("PLB");
                break;
            case KTRE_INSTR_PLB_WIN:
                DBG("PLB_WIN");
                break;
            case KTRE_INSTR_NLB:
                DBG("NLB      %d", re->compiled[i].a);
                break;
            case KTRE_INSTR_NLB_FAIL:
                DBG("NLB_FAIL");
                break;
            default:
                DBG("\nunimplemented instruction printer %d\n", re->compiled[i].op);
                assert(false);
        }
    }
#endif
    return re;
}

ktrecontext_t* ktre_copy(ktrecontext_t* re)
{
    ktrecontext_t* ret;
    ret = (ktrecontext_t*)KTRE_MALLOC(sizeof *ret);
    memset(ret, 0, sizeof *ret);
    ret->compiled = re->compiled;
    re->copied = true;
    return ret;
}

#define TP (re->thrptr)

#define THREAD (re->thread)

#define VEC (re->vec)

#define MAKE_THREAD_VARIABLE(f, p) \
    do \
    { \
        if(!THREAD[TP].f) \
        { \
            THREAD[TP].f = (int*)_malloc((p + 1) * sizeof *THREAD[TP].f); \
            memset(THREAD[TP].f, -1, (p + 1) * sizeof *THREAD[TP].f); \
            re->info.runtime_alloc += (p + 1) * sizeof *THREAD[TP].f; \
        } \
        else if(THREAD[TP].p < p) \
        { \
            THREAD[TP].f = (int*)_realloc(THREAD[TP].f, (p + 1) * sizeof *THREAD[TP].f); \
        } \
        THREAD[TP].p = p; \
        if(TP > 0) \
        { \
            memcpy(THREAD[TP].f, THREAD[TP - 1].f, THREAD[TP - 1].p > p ? p * sizeof THREAD[0].f[0] : THREAD[TP - 1].p * sizeof *THREAD[0].f); \
        } \
    } while(0)

#define MAKE_STATIC_THREAD_VARIABLE(f, s) \
    do \
    { \
        if(!THREAD[TP].f) \
        { \
            THREAD[TP].f = (int*)_malloc((s) * sizeof *THREAD[TP].f); \
            memset(THREAD[TP].f, -1, (s) * sizeof *THREAD[TP].f); \
            re->info.runtime_alloc += (s) * sizeof *THREAD[TP].f; \
        } \
        if(TP > 0) \
        { \
            memcpy(THREAD[TP].f, THREAD[TP - 1].f, (s) * sizeof *THREAD[0].f); \
        } \
    } while(0)

static void ktrepriv_newthread(ktrecontext_t* re, int ip, int sp, int opt, int fp, int la, int ep)
{
    ++TP;
    if(TP >= re->info.allocdthreads)
    {
        if(re->info.allocdthreads * 2 >= KTRE_MAX_THREAD)
        {
            re->info.allocdthreads = KTRE_MAX_THREAD;
            /*
			 * Account for the case where we're just about
			 * to bump up against the thread limit.
			 */
            TP = (TP >= KTRE_MAX_THREAD) ? KTRE_MAX_THREAD - 1 : TP;
        }
        else
        {
            re->info.allocdthreads *= 2;
        }
        re->thread = (ktrethread_t*)_realloc(re->thread, re->info.allocdthreads * sizeof THREAD[0]);
        memset(&THREAD[TP], 0, (re->info.allocdthreads - TP) * sizeof THREAD[0]);
    }
    MAKE_STATIC_THREAD_VARIABLE(vec, re->numgroups * 2);
    MAKE_STATIC_THREAD_VARIABLE(prog, re->compcount);
    MAKE_THREAD_VARIABLE(frame, fp);
    MAKE_THREAD_VARIABLE(las, la);
    MAKE_THREAD_VARIABLE(exception, ep);
    THREAD[TP].thinstrptr = ip;
    THREAD[TP].thstackptr = sp;
    THREAD[TP].opt = opt;
    re->max_tp = (TP > re->max_tp) ? TP : re->max_tp;
}


static bool ktrepriv_run(ktrecontext_t* re, const char* subject, int*** vec)
{
    int i;
    int n;
    int ip;
    int sp;
    int fp;
    int la;
    int ep;
    int opt;
    int loc;
    int cmpres;
    bool rev;
    *vec = NULL;
    re->nummatches = 0;
    TP = -1;
    if(!re->info.allocdthreads)
    {
        re->info.allocdthreads = 25;
        re->thread = (ktrethread_t*)_malloc(re->info.allocdthreads * sizeof THREAD[0]);
        if(re->errcode)
        {
            return false;
        }
        memset(re->thread, 0, re->info.allocdthreads * sizeof THREAD[0]);
    }
    if(re->opt & KTRE_CONTINUE && re->cont >= (int)strlen(subject))
    {
        return false;
    }
    /* push the initial thread */
    if(re->opt & KTRE_CONTINUE)
    {
        ktrepriv_newthread(re, 0, re->cont, re->opt, 0, 0, 0);
    }
    else
    {
        ktrepriv_newthread(re, 0, 0, re->opt, 0, 0, 0);
    }
#ifdef KTRE_DEBUG
    int num_steps = 0;
    DBG("\n|   ip |   sp |   tp |   fp | step |");
#endif
    while(TP >= 0)
    {
        loc = 0;
        ip = THREAD[TP].thinstrptr;
        sp = THREAD[TP].thstackptr;
        fp = THREAD[TP].fp;
        la = THREAD[TP].la;
        ep = THREAD[TP].ep;
        opt = THREAD[TP].opt;
        if(re->compiled)
        {
            loc = re->compiled[ip].loc;
        }
        rev = THREAD[TP].rev;
#ifdef KTRE_DEBUG
        DBG("\n| %4d | %4d | %4d | %4d | %4d | ", ip, sp, TP, fp, num_steps);
        if(sp >= 0)
        {
            dbgf(subject + sp);
        }
        else
        {
            dbgf(subject);
        }
        num_steps++;
#endif
        if(THREAD[TP].die)
        {
            THREAD[TP].die = false;
            --TP;
            continue;
        }
        switch(re->compiled[ip].op)
        {
            case KTRE_INSTR_BACKREF:
                {
                    THREAD[TP].thinstrptr++;
                    if(rev)
                    {
                        if(opt & KTRE_INSENSITIVE)
                        {
                            for(i = 0; i < THREAD[TP].vec[re->compiled[ip].thirdval * 2 + 1]; i++)
                            {
                                if(ktrepriv_lc(subject[sp - i]) != ktrepriv_lc(subject[THREAD[TP].vec[re->compiled[ip].thirdval * 2] + sp - i]))
                                {
                                    --TP;
                                    continue;
                                }
                            }
                            THREAD[TP].thstackptr -= THREAD[TP].vec[re->compiled[ip].thirdval * 2 + 1];
                        }
                        else
                        {
                            cmpres = strncmp(
                                subject + sp + 1 - THREAD[TP].vec[re->compiled[ip].thirdval * 2 + 1],
                                &subject[THREAD[TP].vec[re->compiled[ip].thirdval * 2]],
                                THREAD[TP].vec[re->compiled[ip].thirdval * 2 + 1]);
                            if(!cmpres)
                            {
                                THREAD[TP].thstackptr += THREAD[TP].vec[re->compiled[ip].thirdval * 2 + 1];
                            }
                            else
                            {
                                --TP;
                            }
                        }
                    }
                    else
                    {
                        if(opt & KTRE_INSENSITIVE)
                        {
                            for(i = 0; i < THREAD[TP].vec[re->compiled[ip].thirdval * 2 + 1]; i++)
                            {
                                if(ktrepriv_lc(subject[sp + i]) != ktrepriv_lc(subject[THREAD[TP].vec[re->compiled[ip].thirdval * 2] + i]))
                                {
                                    --TP;
                                    continue;
                                }
                            }
                            THREAD[TP].thstackptr += THREAD[TP].vec[re->compiled[ip].thirdval * 2 + 1];
                        }
                        else
                        {
                            cmpres = strncmp(
                                subject + sp,
                                &subject[THREAD[TP].vec[re->compiled[ip].thirdval * 2]],
                                THREAD[TP].vec[re->compiled[ip].thirdval * 2 + 1]);
                            if(!cmpres)
                            {
                                THREAD[TP].thstackptr += THREAD[TP].vec[re->compiled[ip].thirdval * 2 + 1];
                            }
                            else
                            {
                                --TP;
                            }
                        }
                    }
                }
                break;
            case KTRE_INSTR_CLASS:
                {
                    THREAD[TP].thinstrptr++;
                    if(!(subject[sp] && sp >= 0))
                    {
                        --TP;
                        continue;
                    }
                    if(strchr(re->compiled[ip].charclass, subject[sp]))
                    {
                        THREAD[TP].thstackptr++;
                    }
                    else if(opt & KTRE_INSENSITIVE && strchr(re->compiled[ip].charclass, ktrepriv_lc(subject[sp])))
                    {
                        THREAD[TP].thstackptr++;
                    }
                    else
                    {
                        --TP;
                    }
                }
                break;
            case KTRE_INSTR_STR:
            case KTRE_INSTR_TSTR:
                {
                    THREAD[TP].thinstrptr++;
                    if(rev)
                    {
                        if(opt & KTRE_INSENSITIVE)
                        {
                            for(i = 0; i < (int)strlen(re->compiled[ip].charclass); i++)
                            {
                                if(ktrepriv_lc(subject[sp + 1 - strlen(re->compiled[ip].charclass) + i]) != re->compiled[ip].charclass[i])
                                {
                                    --TP;
                                    break;
                                }
                            }
                            THREAD[TP].thstackptr -= strlen(re->compiled[ip].charclass);
                        }
                        else
                        {
                            if(!strncmp(subject + sp + 1 - strlen(re->compiled[ip].charclass), re->compiled[ip].charclass, strlen(re->compiled[ip].charclass)))
                            {
                                THREAD[TP].thstackptr -= strlen(re->compiled[ip].charclass);
                            }
                            else
                            {
                                --TP;
                            }
                        }
                    }
                    else
                    {
                        if(opt & KTRE_INSENSITIVE)
                        {
                            for(i = 0; i < (int)strlen(re->compiled[ip].charclass); i++)
                            {
                                if(ktrepriv_lc(subject[sp + i]) != re->compiled[ip].charclass[i])
                                {
                                    --TP;
                                    break;
                                }
                            }
                            THREAD[TP].thstackptr -= strlen(re->compiled[ip].charclass);
                        }
                        else
                        {
                            if(!strncmp(subject + sp, re->compiled[ip].charclass, strlen(re->compiled[ip].charclass)))
                            {
                                THREAD[TP].thstackptr += strlen(re->compiled[ip].charclass);
                            }
                            else
                            {
                                --TP;
                            }
                        }
                    }
                }
                break;
            case KTRE_INSTR_NOT:
                {
                    THREAD[TP].thinstrptr++;
                    if(!strchr(re->compiled[ip].charclass, subject[sp]) && subject[sp] && sp >= 0)
                    {
                        THREAD[TP].thstackptr++;
                    }
                    else
                    {
                        --TP;
                    }
                }
                break;
            case KTRE_INSTR_BOL:
                {
                    if((sp > 0 && subject[sp - 1] == '\n') || sp == 0)
                    {
                        THREAD[TP].thinstrptr++;
                    }
                    else
                    {
                        --TP;
                    }
                }
                break;
            case KTRE_INSTR_EOL:
                {
                    if((sp >= 0 && subject[sp] == '\n') || sp == (int)strlen(subject))
                    {
                        THREAD[TP].thinstrptr++;
                    }
                    else
                    {
                        --TP;
                    }
                }
                break;
            case KTRE_INSTR_BOS:
                {
                    if(sp == 0)
                    {
                        THREAD[TP].thinstrptr++;
                    }
                    else
                    {
                        --TP;
                    }
                }
                break;
            case KTRE_INSTR_EOS:
                {
                    if(sp >= 0 && sp == (int)strlen(subject))
                    {
                        THREAD[TP].thinstrptr++;
                    }
                    else
                    {
                        --TP;
                    }
                }
                break;
            case KTRE_INSTR_WB:
                {
                    THREAD[TP].thinstrptr++;
                    if(sp < 0 || sp >= (int)strlen(subject))
                    {
                        --TP;
                        continue;
                    }
                    if(sp == 0 && strchr(KTRE_WORD, subject[sp]))
                    {
                        continue;
                    }
                    if(sp > 0)
                    {
                        if(strchr(KTRE_WORD, subject[sp]) && !strchr(KTRE_WORD, subject[sp - 1]))
                        {
                            continue;
                        }
                        if(!strchr(KTRE_WORD, subject[sp]) && strchr(KTRE_WORD, subject[sp - 1]))
                        {
                            continue;
                        }
                    }
                    --TP;
                }
                break;
            case KTRE_INSTR_NWB:
                {
                    THREAD[TP].thinstrptr++;
                    if(sp < 0 || sp >= (int)strlen(subject))
                    {
                        --TP;
                        continue;
                    }
                    if(sp == 0 && !strchr(KTRE_WORD, subject[sp]))
                    {
                        continue;
                    }
                    if(sp > 0)
                    {
                        if(!(strchr(KTRE_WORD, subject[sp]) && !strchr(KTRE_WORD, subject[sp - 1])))
                        {
                            continue;
                        }
                        if(!(!strchr(KTRE_WORD, subject[sp]) && strchr(KTRE_WORD, subject[sp - 1])))
                        {
                            continue;
                        }
                    }
                    --TP;
                }
                break;
            case KTRE_INSTR_CHAR:
                THREAD[TP].thinstrptr++;
                if(sp < 0 || sp >= (int)strlen(subject))
                {
                    --TP;
                    continue;
                }
                if(opt & KTRE_INSENSITIVE)
                {
                    if(ktrepriv_lc(subject[sp]) != ktrepriv_lc(re->compiled[ip].thirdval))
                    {
                        --TP;
                    }
                    else
                    {
                        if(rev)
                        {
                            THREAD[TP].thstackptr--;
                        }
                        else
                        {
                            THREAD[TP].thstackptr++;
                        }
                    }
                }
                else
                {
                    if(subject[sp] != re->compiled[ip].thirdval)
                    {
                        --TP;
                    }
                    else
                    {
                        if(rev)
                        {
                            THREAD[TP].thstackptr--;
                        }
                        else
                        {
                            THREAD[TP].thstackptr++;
                        }
                    }
                }
                break;
            case KTRE_INSTR_ANY:
                {
                    THREAD[TP].thinstrptr++;
                    if(opt & KTRE_MULTILINE)
                    {
                        if(sp >= 0 && subject[sp])
                        {
                            if(rev)
                            {
                                THREAD[TP].thstackptr--;
                            }
                            else
                            {
                                THREAD[TP].thstackptr++;
                            }
                        }
                        else
                        {
                            --TP;
                        }
                    }
                    else
                    {
                        if(sp >= 0 && subject[sp] && subject[sp] != '\n')
                        {
                            if(rev)
                            {
                                THREAD[TP].thstackptr--;
                            }
                            else
                            {
                                THREAD[TP].thstackptr++;
                            }
                        }
                        else
                        {
                            --TP;
                        }
                    }
                }
                break;
            case KTRE_INSTR_MANY:
                {
                    THREAD[TP].thinstrptr++;
                    if(sp >= 0 && subject[sp])
                    {
                        if(rev)
                        {
                            THREAD[TP].thstackptr--;
                        }
                        else
                        {
                            THREAD[TP].thstackptr++;
                        }
                    }
                    else
                    {
                        --TP;
                    }
                }
                break;
            case KTRE_INSTR_BRANCH:
                {
                    THREAD[TP].thinstrptr = re->compiled[ip].secondval;
                    ktrepriv_newthread(re, re->compiled[ip].firstval, sp, opt, fp, la, ep);
                }
                break;
            case KTRE_INSTR_MATCH:
                {
                    n = 0;
                    for(i = 0; i < re->nummatches; i++)
                    {
                        if(VEC[i][0] == sp)
                        {
                            n++;
                        }
                    }
                    if(n)
                    {
                        --TP;
                        continue;
                    }
                    if((opt & KTRE_UNANCHORED) || (sp >= 0 && !subject[sp]))
                    {
                        VEC = (int**)_realloc(VEC, (re->nummatches + 1) * sizeof *VEC);
                        re->cont = sp;
                        if(!VEC)
                        {
                            ktrepriv_error(re, KTRE_ERROR_OUT_OF_MEMORY, loc, "out of memory");
                            return false;
                        }
                        VEC[re->nummatches] = (int*)_malloc(re->numgroups * 2 * sizeof VEC[0]);
                        if(!VEC[re->nummatches])
                        {
                            ktrepriv_error(re, KTRE_ERROR_OUT_OF_MEMORY, loc, "out of memory");
                            return false;
                        }
                        memcpy(VEC[re->nummatches++], THREAD[TP].vec, re->numgroups * 2 * sizeof VEC[0][0]);
                        if(vec)
                        {
                            *vec = VEC;
                        }
                        if(!(opt & KTRE_GLOBAL))
                        {
                            return true;
                        }
                        else
                        {
                            TP = 0;
                            THREAD[TP].thinstrptr = 0;
                            THREAD[TP].thstackptr = sp;
                            if(THREAD[TP].thstackptr > (int)strlen(subject))
                            {
                                return true;
                            }
                            continue;
                        }
                    }
                    --TP;
                }
                break;
            case KTRE_INSTR_SAVE:
                {
                    THREAD[TP].thinstrptr++;
                    if(re->compiled[ip].thirdval % 2 == 0)
                    {
                        THREAD[TP].vec[re->compiled[ip].thirdval] = sp;
                    }
                    else
                    {
                        THREAD[TP].vec[re->compiled[ip].thirdval] = sp - THREAD[TP].vec[re->compiled[ip].thirdval - 1];
                    }
                }
                break;
            case KTRE_INSTR_JMP:
                {
                    THREAD[TP].thinstrptr = re->compiled[ip].thirdval;
                }
                break;
            case KTRE_INSTR_SETOPT:
                {
                    THREAD[TP].thinstrptr++;
                    THREAD[TP].opt = re->compiled[ip].thirdval;
                }
                break;
            case KTRE_INSTR_SET_START:
                {
                    THREAD[TP].thinstrptr++;
                    THREAD[TP].vec[0] = sp;
                }
                break;
            case KTRE_INSTR_CALL:
                THREAD[TP].thinstrptr = re->compiled[ip].thirdval;
                THREAD[TP].frame = (int*)_realloc(THREAD[TP].frame, (fp + 1) * sizeof THREAD[TP].frame[0]);
                THREAD[TP].frame[THREAD[TP].fp++] = ip + 1;
                break;
            case KTRE_INSTR_RET:
                THREAD[TP].thinstrptr = THREAD[TP].frame[--THREAD[TP].fp];
                break;
            case KTRE_INSTR_PROG:
                THREAD[TP].thinstrptr++;
                if(THREAD[TP].prog[re->compiled[ip].thirdval] == sp)
                {
                    --TP;
                }
                else
                {
                    THREAD[TP].prog[re->compiled[ip].thirdval] = sp;
                }
                break;
            case KTRE_INSTR_DIGIT:
                THREAD[TP].thinstrptr++;
                if(strchr(KTRE_DIGIT, subject[sp]) && subject[sp])
                {
                    if(rev)
                    {
                        THREAD[TP].thstackptr--;
                    }
                    else
                    {
                        THREAD[TP].thstackptr++;
                    }
                }
                else
                {
                    --TP;
                }
                break;
            case KTRE_INSTR_WORD:
                THREAD[TP].thinstrptr++;
                if(strchr(KTRE_WORD, subject[sp]) && subject[sp])
                {
                    if(rev)
                    {
                        THREAD[TP].thstackptr--;
                    }
                    else
                    {
                        THREAD[TP].thstackptr++;
                    }
                }
                else
                {
                    --TP;
                }
                break;
            case KTRE_INSTR_SPACE:
                {
                    THREAD[TP].thinstrptr++;
                    if(strchr(KTRE_WHITESPACECHARS, subject[sp]) && subject[sp])
                    {
                        if(rev)
                        {
                            THREAD[TP].thstackptr--;
                        }
                        else
                        {
                            THREAD[TP].thstackptr++;
                        }
                    }
                    else
                    {
                        --TP;
                    }
                }
                break;
            case KTRE_INSTR_TRY:
                {
                    THREAD[TP].thinstrptr++;
                    THREAD[TP].exception = (int*)_realloc(THREAD[TP].exception, (ep + 1) * sizeof THREAD[TP].exception[0]);
                    THREAD[TP].exception[THREAD[TP].ep++] = TP;
                }
                break;
            case KTRE_INSTR_CATCH:
                TP = THREAD[TP].exception[ep - 1];
                THREAD[TP].thinstrptr = ip + 1;
                THREAD[TP].thstackptr = sp;
                break;
            case KTRE_INSTR_PLB:
                THREAD[TP].die = true;
                ktrepriv_newthread(re, ip + 1, sp - 1, opt, fp, la, ep + 1);
                THREAD[TP].exception[ep] = TP - 1;
                THREAD[TP].rev = true;
                break;
            case KTRE_INSTR_PLB_WIN:
                TP = THREAD[TP].exception[--THREAD[TP].ep];
                THREAD[TP].rev = false;
                THREAD[TP].die = false;
                THREAD[TP].thinstrptr = ip + 1;
                break;
            case KTRE_INSTR_NLB:
                THREAD[TP].thinstrptr = re->compiled[ip].thirdval;
                ktrepriv_newthread(re, ip + 1, sp - 1, opt, fp, la, ep + 1);
                THREAD[TP].exception[ep] = TP - 1;
                THREAD[TP].rev = true;
                break;
            case KTRE_INSTR_NLB_FAIL:
                TP = THREAD[TP].exception[--THREAD[TP].ep] - 1;
                break;
            case KTRE_INSTR_PLA:
                THREAD[TP].die = true;
                ktrepriv_newthread(re, ip + 1, sp, opt, fp, la, ep + 1);
                THREAD[TP].exception[ep] = TP - 1;
                break;
            case KTRE_INSTR_PLA_WIN:
                TP = THREAD[TP].exception[--THREAD[TP].ep];
                THREAD[TP].die = false;
                THREAD[TP].thinstrptr = ip + 1;
                break;
            case KTRE_INSTR_NLA:
                THREAD[TP].thinstrptr = re->compiled[ip].firstval;
                ktrepriv_newthread(re, ip + 1, sp, opt, fp, la, ep + 1);
                THREAD[TP].exception[ep] = TP - 1;
                break;
            case KTRE_INSTR_NLA_FAIL:
                TP = THREAD[TP].exception[--THREAD[TP].ep] - 1;
                break;
            default:
                DBG("\nunimplemented instruction %d\n", re->compiled[ip].op);
#ifdef KTRE_DEBUG
                assert(false);
#endif
                return false;
        }
        if(TP >= KTRE_MAX_THREAD - 1)
        {
            ktrepriv_error(re, KTRE_ERROR_STACK_OVERFLOW, loc, "regex exceeded the maximum number of executable threads");
            return false;
        }
        if(fp >= KTRE_MAX_CALL_DEPTH - 1)
        {
            ktrepriv_error(re, KTRE_ERROR_CALL_OVERFLOW, loc, "regex exceeded the maximum depth for subroutine calls");
            return false;
        }
    }
    return !!re->nummatches;
}

ktreinfo_t ktre_free(ktrecontext_t* re)
{
    if(re->copied)
    {
        return re->info;
    }
    ktrepriv_freenode(re, re->headnode);
    if(re->errcode)
    {
        _free(re->errstr);
    }
    if(re->compiled)
    {
        for(int i = 0; i < re->instrpos; i++)
        {
            if(re->compiled[i].op == KTRE_INSTR_TSTR)
            {
                _free(re->compiled[i].charclass);
            }
        }
        _free(re->compiled);
    }
    for(int i = 0; i <= re->max_tp; i++)
    {
        _free(THREAD[i].vec);
        _free(THREAD[i].prog);
        _free(THREAD[i].frame);
        _free(THREAD[i].las);
        _free(THREAD[i].exception);
    }
    if(re->vec)
    {
        for(int i = 0; i < re->nummatches; i++)
        {
            _free((re->vec)[i]);
        }
        _free(re->vec);
    }
    for(int i = 0; i < re->groupptr; i++)
    {
        if(re->group[i].name)
        {
            _free(re->group[i].name);
        }
    }
    _free(re->group);
    _free(re->thread);
    ktreinfo_t info = re->info;
#if defined(_MSC_VER) && defined(KTRE_DEBUG)
    _CrtDumpMemoryLeaks();
#endif
#ifdef KTRE_DEBUG
    DBG("\nFinished with %d allocations, %d frees, and %zd bytes allocated.", info.num_alloc, info.num_free, info.mba + sizeof(ktrecontext_t));
    DBG("\n%5zd bytes were allocated for the main structure.", sizeof(ktrecontext_t));
    DBG("\n%5zd bytes were allocated for the bytecode.", info.instr_alloc * sizeof(ktreinstr_t));
    DBG("\n%5zd bytes were allocated for the runtime.", info.runtime_alloc + info.thread_alloc * sizeof(ktrethread_t));
    DBG("\n%5d bytes were allocated for the parser.", info.parser_alloc);
    if(info.ba)
    {
        DBG("\nThere were %zd leaked bytes from %d unmatched allocations.", info.ba - (info.num_alloc - info.num_free) * sizeof(ktrematch_t),
            info.num_alloc - info.num_free);
        ktrematch_t* mi = re->minfo;
        int i = 0;
        while(mi->next)
            mi = mi->next;
        while(mi)
        {
            DBG("\n\tleak %d: %d bytes at %s:%d", i + 1, mi->size, mi->file, mi->line);
            mi = mi->prev;
            i++;
        }
    }
#endif
    ktrematch_t* mi = re->minfo;
    while(mi)
    {
        ktrematch_t* mi2 = mi;
        mi = mi->next;
        KTRE_FREE(mi2);
    }
    KTRE_FREE(re);
    return info;
}

bool ktre_exec(ktrecontext_t* re, const char* subject, int*** vec)
{
    DBG("\nsubject: %s", subject);
    if(re->errcode)
    {
        if(re->errstr)
        {
            _free(re->errstr);
        }
        re->errcode = KTRE_ERROR_NO_ERROR;
    }
    int** v = NULL;
    bool ret = false;
    if(vec)
    {
        ret = ktrepriv_run(re, subject, vec);
    }
    else
    {
        ret = ktrepriv_run(re, subject, &v);
    }
    if(vec)
    {
        ktre_printfinish(re, subject, re->sourcepattern, ret, *vec, NULL);
    }
    else
    {
        ktre_printfinish(re, subject, re->sourcepattern, ret, v, NULL);
    }
    return ret;
}

bool ktre_match(const char* subject, const char* pat, int opt, int*** vec)
{
    ktrecontext_t* re = ktre_compile(pat, opt);
    if(re->errcode)
    {
        ktre_free(re);
        return false;
    }
    int** v = NULL;
    bool ret = ktrepriv_run(re, subject, vec ? vec : &v);
    ktre_printfinish(re, subject, pat, ret, vec ? *vec : v, NULL);
    ktre_free(re);
    return ret;
}

char* ktre_replace(const char* subject, const char* pat, const char* replacement, const char* indicator, int opt)
{
    ktrecontext_t* re = ktre_compile(pat, opt);
    if(!re->errcode)
    {
        char* ret = ktre_filter(re, subject, replacement, indicator);
        ktre_free(re);
        return ret;
    }
    ktre_free(re);
    return NULL;
}

static void ktrepriv_smartcopy(char* dest, const char* src, size_t n, bool u, bool uch, bool l, bool lch)
{
    for(int i = 0; i < (int)n; i++)
    {
        if(i == 0 && uch)
        {
            dest[i] = ktrepriv_uc(src[i]);
            continue;
        }
        if(i == 0 && lch)
        {
            dest[i] = ktrepriv_lc(src[i]);
            continue;
        }
        if(u)
        {
            dest[i] = ktrepriv_uc(src[i]);
        }
        else if(l)
        {
            dest[i] = ktrepriv_lc(src[i]);
        }
        else
        {
            dest[i] = src[i];
        }
    }
}

#define SIZE_STRING(ptr, n) ptr = (char*)_realloc(ptr, n * sizeof *ptr)
char* ktre_filter(ktrecontext_t* re, const char* subject, const char* replacement, const char* indicator)
{
    DBG("\nsubject: %s", subject);
    int** vec = NULL;
    if(!ktrepriv_run(re, subject, &vec) || re->errcode)
    {
        ktre_printfinish(re, subject, re->sourcepattern, false, vec, NULL);
        return NULL;
    }
    char* ret = (char*)_malloc(16);
    *ret = 0;
    int idx = 0;
    for(int i = 0; i < re->nummatches; i++)
    {
        bool u = false, l = false;
        bool uch = false, lch = false;
        if(i > 0)
        {
            int len = vec[i][0] - (vec[i - 1][0] + vec[i - 1][1]);
            SIZE_STRING(ret, idx + len + 1);
            strncpy(ret + idx, subject + vec[i - 1][0] + vec[i - 1][1], len);
            idx += len;
        }
        else
        {
            idx = vec[i][0];
            SIZE_STRING(ret, idx + 1);
            strncpy(ret, subject, idx);
        }
        ret[idx] = 0;
        char* match = NULL;
        int j = 0;
        const char* r = replacement;
        while(*r)
        {
            if(!strncmp(r, indicator, strlen(indicator)))
            {
                const char* t = r;
                t += strlen(indicator);
                int n = ktrepriv_decnum(&t);
                if(n < 0 || n >= re->numgroups)
                {
                    goto skip_capture;
                }
                r = t;
                /* ignore unintialized groups */
                if(vec[i][n * 2] < 0 || vec[i][n * 2 + 1] < 0)
                {
                    continue;
                }
                SIZE_STRING(match, j + vec[i][n * 2 + 1] + 1);
                ktrepriv_smartcopy(match + j, subject + vec[i][n * 2], vec[i][n * 2 + 1], u, uch, l, lch);
                j += vec[i][n * 2 + 1];
                uch = lch = false;
                continue;
            }
        skip_capture:
            if(*r != '\\')
            {
                SIZE_STRING(match, j + 2);
                match[j] = *r;
                if(uch || u)
                {
                    match[j] = ktrepriv_uc(*r);
                }
                if(lch || l)
                {
                    match[j] = ktrepriv_lc(*r);
                }
                j++;
                r++;
                lch = uch = false;
                continue;
            }
            r++;
            switch(*r)
            {
                case 'U':
                    u = true;
                    r++;
                    break;
                case 'L':
                    l = true;
                    r++;
                    break;
                case 'E':
                    l = u = false;
                    r++;
                    break;
                case 'l':
                    lch = true;
                    r++;
                    break;
                case 'u':
                    uch = true;
                    r++;
                    break;
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                {
                    int n = ktrepriv_decnum(&r);
                    SIZE_STRING(match, j + 2);
                    match[j] = n;
                }
                break;
            }
        }
        if(match)
        {
            match[j] = 0;
            SIZE_STRING(ret, idx + j + 1);
            strcpy(ret + idx, match);
            ret[idx + j] = 0;
            idx += j;
            _free(match);
        }
    }
    int end = vec[re->nummatches - 1][0] + vec[re->nummatches - 1][1];
    SIZE_STRING(ret, idx + strlen(subject) - end + 1);
    strncpy(ret + idx, subject + end, strlen(subject) - end);
    idx += strlen(subject) - end;
    ret[idx] = 0;
    char* a = (char*)KTRE_MALLOC(strlen(ret) + 1);
    strcpy(a, ret);
    _free(ret);
    ktre_printfinish(re, subject, re->sourcepattern, ret, vec, a);
    return a;
}

char** ktre_split(ktrecontext_t* re, const char* subject, int* len)
{
    int j;
    int i;
    int** vec;
    char** r;
    DBG("\nsubject: %s", subject);
    *len = 0;
    vec = NULL;
    if(!ktrepriv_run(re, subject, &vec) || re->errcode)
    {
        ktre_printfinish(re, subject, re->sourcepattern, false, vec, NULL);
        return NULL;
    }
    r = NULL;
    j = 0;
    for(i = 0; i < re->nummatches; i++)
    {
        if(vec[i][0] == 0 || vec[i][0] == (int)strlen(subject))
        {
            continue;
        }
        r = (char**)realloc(r, (*len + 1) * sizeof *r);
        r[*len] = (char*)malloc(vec[i][0] - j + 1);
        strncpy(r[*len], subject + j, vec[i][0] - j);
        r[*len][vec[i][0] - j] = 0;
        j = vec[i][0] + vec[i][1];
        (*len)++;
    }
    if((int)strlen(subject) >= j)
    {
        r = (char**)realloc(r, (*len + 1) * sizeof *r);
        r[*len] = (char*)malloc(strlen(subject) - j + 1);
        strcpy(r[*len], subject + j);
        (*len)++;
    }
    return r;
}

int** ktre_getvec(const ktrecontext_t* re)
{
    int i;
    int** vec;
    vec = (int**)KTRE_MALLOC(re->nummatches * sizeof re->vec[0]);
    for(i = 0; i < re->nummatches; i++)
    {
        vec[i] = (int*)KTRE_MALLOC(re->numgroups * 2 * sizeof re->vec[0][0]);
        memcpy(vec[i], re->vec[i], re->numgroups * 2 * sizeof re->vec[0][0]);
    }
    return vec;
}

