
#include "blade.h"



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
    } keywords[] = {
        { TOK_AND, "and" },
        { TOK_ASSERT, "assert" },
        { TOK_AS, "as" },
        { TOK_BREAK, "break" },
        { TOK_CATCH, "catch" },
        { TOK_CLASS, "class" },
        { TOK_CONTINUE, "continue" },
        { TOK_DEFAULT, "default" },
        { TOK_DEF, "function" },

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
        { 0, NULL }
    };
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
            fprintf(stderr, " at '%.*s' (previous: '%.*s')", t->length, t->start, p->previous.length, p->previous.start);
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
        case OP_GREATERTHAN:
        case OP_LESSTHAN:
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
        case OP_BITAND:
        case OP_BITOR:
        case OP_BITXOR:
        case OP_LEFTSHIFT:
        case OP_RIGHTSHIFT:
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
        p->vm->compiler->currfunc->name = bl_string_copystringlen(p->vm, p->previous.start, p->previous.length);
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
    return bl_parser_makeconstant(p, OBJ_VAL(bl_string_copystringlen(p->vm, name->start, name->length)));
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
            bl_parser_emitbyte(p, OP_GREATERTHAN);
            break;
        case TOK_GREATEREQ:
            bl_parser_emitbytes(p, OP_LESSTHAN, OP_NOT);
            break;
        case TOK_LESS:
            bl_parser_emitbyte(p, OP_LESSTHAN);
            break;
        case TOK_LESSEQ:
            bl_parser_emitbytes(p, OP_GREATERTHAN, OP_NOT);
            break;
            // bitwise
        case TOK_AMP:
            bl_parser_emitbyte(p, OP_BITAND);
            break;
        case TOK_BAR:
            bl_parser_emitbyte(p, OP_BITOR);
            break;
        case TOK_XOR:
            bl_parser_emitbyte(p, OP_BITXOR);
            break;
        case TOK_LSHIFT:
            bl_parser_emitbyte(p, OP_LEFTSHIFT);
            break;
        case TOK_RSHIFT:
            bl_parser_emitbyte(p, OP_RIGHTSHIFT);
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
        bl_parser_parseassign(p, OP_BITAND, getop, setop, arg);
    }
    else if(canassign && bl_parser_match(p, TOK_BAREQ))
    {
        bl_parser_parseassign(p, OP_BITOR, getop, setop, arg);
    }
    else if(canassign && bl_parser_match(p, TOK_TILDEEQ))
    {
        bl_parser_parseassign(p, OP_BIT_NOT, getop, setop, arg);
    }
    else if(canassign && bl_parser_match(p, TOK_XOREQ))
    {
        bl_parser_parseassign(p, OP_BITXOR, getop, setop, arg);
    }
    else if(canassign && bl_parser_match(p, TOK_LSHIFTEQ))
    {
        bl_parser_parseassign(p, OP_LEFTSHIFT, getop, setop, arg);
    }
    else if(canassign && bl_parser_match(p, TOK_RSHIFTEQ))
    {
        bl_parser_parseassign(p, OP_RIGHTSHIFT, getop, setop, arg);
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
                    bl_parser_emitconstant(p, OBJ_VAL(bl_string_copystringlen(p->vm, p->previous.start, p->previous.length)));
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
        // fixme: this breaks '||'
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
 * for i = 0; i < 10; i++ {
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
    /*
    if(!bl_parser_match(p, TOK_LPAREN))
    {
    }
    */
    bl_parser_consume(p, TOK_LPAREN, "expected '(' after 'for' keyword");

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
    /*if(!bl_parser_match(p, TOK_RPAREN))
    {

    }
    */

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

    bl_parser_consume(p, TOK_RPAREN, "expected ')' after 'for' conditionals");


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
    int iter__ = bl_parser_makeconstant(p, OBJ_VAL(bl_string_copystringlen(p->vm, "@iter", 5)));
    int iter_n__ = bl_parser_makeconstant(p, OBJ_VAL(bl_string_copystringlen(p->vm, "@itern", 6)));
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
                        ObjString* string = bl_string_copystringlen(p->vm, str, length);
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
    int modnlen;
    int modconst;
    int partcount;
    int importconstant;
    size_t srclen;
    bool wasrenamed;
    bool isrelative;
    char* source;
    char* modulename;
    char* modulefile;
    char* modulepath;
    BinaryBlob blob;
    ObjFunction* function;
    ObjModule* modobj;
    ObjClosure* closure;
    (void)srclen;
    //  bl_parser_consume(p, TOK_LITERAL, "expected module name");
    //  int modulenamelength;
    //  char *modulename = bl_parser_compilestring(p, &modulenamelength);
    modulename = NULL;
    modulefile = NULL;
    partcount = 0;
    isrelative = bl_parser_match(p, TOK_DOT);
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
        modnlen = p->previous.length+0;
        //modulename = (char*)calloc(modnlen+1, sizeof(char));
        modulename = (char*)malloc(modnlen+1);
        memset(modulename, 0, modnlen+1);
        strncpy(modulename, p->previous.start, modnlen);
        modulename[modnlen] = '\0';
        // handle native modules
        if(partcount == 0 && modulename[0] == '_' && !isrelative)
        {
            modconst = bl_parser_makeconstant(p, OBJ_VAL(bl_string_copystringlen(p->vm, modulename, modnlen)));
            bl_parser_emitbyte_and_short(p, OP_NATIVE_MODULE, modconst);
            bl_parser_parsespecificimport(p, modulename, modconst, false, true);
            return;
        }
        if(modulefile == NULL)
        {
            modulefile = strndup(modulename, modnlen);
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

    fprintf(stderr, "modulename=<%.*s>\n", modnlen, modulename);

    wasrenamed = false;
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
        strncpy(modulename, p->previous.start, p->previous.length);
        modulename[p->previous.length] = '\0';
        wasrenamed = true;
    }
    modulepath = bl_util_resolveimportpath(modulefile, p->module->file, isrelative);
    if(modulepath == NULL)
    {
        // check if there is one in the vm's registry
        // handle native modules
        Value md;
        ObjString* finalmodulename = bl_string_copystringlen(p->vm, modulename, modnlen);
        if(bl_hashtable_get(&p->vm->modules, OBJ_VAL(finalmodulename), &md))
        {
            modconst = bl_parser_makeconstant(p, OBJ_VAL(finalmodulename));
            bl_parser_emitbyte_and_short(p, OP_NATIVE_MODULE, modconst);
            bl_parser_parsespecificimport(p, modulename, modconst, false, true);
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
    source = bl_util_readfile(modulepath, &srclen);
    if(source == NULL)
    {
        bl_parser_raiseerror(p, "could not read import file %s", modulepath);
        return;
    }
    bl_blob_init(&blob);
    modobj = bl_object_makemodule(p->vm, modulename, modulepath);
    bl_vm_pushvalue(p->vm, OBJ_VAL(modobj));
    function = bl_compiler_compilesource(p->vm, modobj, source, &blob);
    bl_vm_popvalue(p->vm);
    free(source);
    if(function == NULL)
    {
        bl_parser_raiseerror(p, "failed to import %s", modulename);
        return;
    }
    function->name = NULL;
    bl_vm_pushvalue(p->vm, OBJ_VAL(function));
    closure = bl_object_makeclosure(p->vm, function);
    bl_vm_popvalue(p->vm);
    importconstant = bl_parser_makeconstant(p, OBJ_VAL(closure));
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
        type = bl_parser_makeconstant(p, OBJ_VAL(bl_string_copystringlen(p->vm, "Exception", 9)));
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


