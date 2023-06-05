
#include "blade.h"

#if defined(__linux__) || defined(__CYGWIN__) || defined(__MINGW32_MAJOR_VERSION)
    #define PROC_SELF_EXE "/proc/self/exe"
#endif

#define pack754_64(f) (pack754((f), 64, 11))
#define unpack754_64(i) (unpack754((i), 64, 11))

uint64_t pack754(long double f, unsigned bits, unsigned expbits)
{
    long double fnorm;
    int shift;
    long long sign;
    long long exp;
    long long significand;
    unsigned significandbits;
    /* -1 for sign bit */
    significandbits = bits - expbits - 1;
    /* get this special case out of the way */
    if(f == 0.0)
    {
        return 0;
    }
    /* check sign and begin normalization */
    if(f < 0)
    {
        sign = 1;
        fnorm = -f;
    }
    else
    {
        sign = 0;
        fnorm = f;
    }
    /* get the normalized form of f and track the exponent */
    shift = 0;
    while(fnorm >= 2.0)
    {
        fnorm /= 2.0;
        shift++;
    }
    while(fnorm < 1.0)
    {
        fnorm *= 2.0;
        shift--;
    }
    fnorm = fnorm - 1.0;
    /* calculate the binary form (non-float) of the significand data */
    significand = fnorm * ((1LL << significandbits) + 0.5f);
    /* get the biased exponent */
    /* shift + bias */
    exp = shift + ((1<<(expbits-1)) - 1);
    /* return the final answer */
    return (
        (sign << (bits - 1)) | (exp << (bits - expbits - 1)) | significand
    );
}

long double unpack754(uint64_t i, unsigned bits, unsigned expbits)
{
    long double result;
    long long shift;
    unsigned bias;
    unsigned significandbits;
    /* -1 for sign bit */
    significandbits = bits - expbits - 1;
    if(i == 0)
    {
        return 0.0;
    }
    /* pull the significand */
    /* mask */
    result = (i&((1LL<<significandbits)-1));
    /* convert back to float */
    result /= (1LL<<significandbits);
    /* add the one back on */
    result += 1.0f;
    /* deal with the exponent */
    bias = ((1 << (expbits - 1)) - 1);
    shift = (((i >> significandbits) & ((1LL << expbits) - 1)) - bias);
    while(shift > 0)
    {
        result *= 2.0;
        shift--;
    }
    while(shift < 0)
    {
        result /= 2.0;
        shift++;
    }
    /* sign it */
    if(((i>>(bits-1)) & 1) == 1)
    {
        result = result * -1.0;
    }
    else
    {
        result = result * 1.0;
    }
    return result;
}

/* this used to be done via type punning, which may not be portable */
double bl_util_uinttofloat(unsigned int val)
{
    return unpack754_64(val);
}

unsigned int bl_util_floattouint(double val)
{
    return pack754_64(val);
}

int bl_util_doubletoint(double n)
{
    if(n == 0)
    {
        return 0;
    }
    if(isnan(n))
    {
        return 0;
    }
    if(n < 0)
    {
        n = -floor(-n);
    }
    else
    {
        n = floor(n);
    }
    if(n < INT_MIN)
    {
        return INT_MIN;
    }
    if(n > INT_MAX)
    {
        return INT_MAX;
    }
    return (int)n;
}

int bl_util_numbertoint32(double n)
{
    /* magic. no idea. */
    bool isf;
    double two32 = 4294967296.0;
    double two31 = 2147483648.0;
    isf = isfinite(n);
    if(!isf || (n == 0))
    {
        return 0;
    }
    n = fmod(n, two32);
    if(n >= 0)
    {
        n = floor(n);
    }
    else
    {
        n = ceil(n) + two32;
    }
    if(n >= two31)
    {
        return n - two32;
    }
    return n;
}

unsigned int bl_util_numbertouint32(double n)
{
    return (unsigned int)bl_util_numbertoint32(n);
}

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


char* bl_util_readhandle(FILE* hnd, size_t* dlen)
{
    long rawtold;
    /*
    * the value returned by ftell() may not necessarily be the same as
    * the amount that can be read.
    * since we only ever read a maximum of $toldlen, there will
    * be no memory trashing.
    */
    size_t toldlen;
    size_t actuallen;
    char* buf;
    if(fseek(hnd, 0, SEEK_END) == -1)
    {
        return NULL;
    }
    if((rawtold = ftell(hnd)) == -1)
    {
        return NULL;
    }
    toldlen = rawtold;
    if(fseek(hnd, 0, SEEK_SET) == -1)
    {
        return NULL;
    }
    buf = (char*)malloc(toldlen + 1);
    memset(buf, 0, toldlen+1);
    if(buf != NULL)
    {
        actuallen = fread(buf, sizeof(char), toldlen, hnd);
        /*
        // optionally, read remainder:
        size_t tmplen;
        if(actuallen < toldlen)
        {
            tmplen = actuallen;
            actuallen += fread(buf+tmplen, sizeof(char), actuallen-toldlen, hnd);
            ...
        }
        // unlikely to be necessary, so not implemented.
        */
        if(dlen != NULL)
        {
            *dlen = actuallen;
        }
        return buf;
    }
    return NULL;
}

char* bl_util_readfile(const char* filename, size_t* dlen)
{
    char* b;
    FILE* fh;
    if((fh = fopen(filename, "rb")) == NULL)
    {
        return NULL;
    }
    b = bl_util_readhandle(fh, dlen);
    fclose(fh);
    return b;
}

char* bl_util_getexepath()
{
    #if defined(__unix__) || defined(__linux__)
        char rawpath[PATH_MAX] = {0};
        long readlength;
        if((readlength = readlink(PROC_SELF_EXE, rawpath, sizeof(rawpath))) > -1 && readlength < PATH_MAX)
        {
            return strndup(rawpath, readlength);
        }
    #endif
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
        // just in case a is const char*
        return strdup(a);
    }
    lena = strlen(a);
    fprintf(stderr, "mergepaths: a=<%s> b=<%s>\n", a, b);
    if(a == NULL || lena == 0)
    {
        free(finalpath);
        // just in case b is const char*
        return strdup(b);
    }
    finalpath = bl_util_appendstring(finalpath, a);
    if(!(lenb == 3 && b[0] == '.' && b[1] == 'b'))
    {
        finalpath = bl_util_appendstring(finalpath, BLADE_PATH_SEPARATOR);
    }
    finalpath = bl_util_appendstring(finalpath, b);
    return finalpath;
}

bool bl_util_fileexists(char* filepath)
{
    struct stat st;
    if(stat(filepath, &st) == -1)
    {
        return false;
    }
    return true;
}

char* bl_util_getbladefilename(const char* filename)
{
    return bl_util_mergepaths(filename, BLADE_EXTENSION);
}

char* bl_util_resolveimportpath(char* modulename, const char* currentfile, bool isrelative)
{
    char* exedir;
    char* path1;
    char* path2;
    char* vendorfile;
    char* rootdir;
    char* bladefilename;
    char* filedirectory;
    char* vendorindexfile;
    char* relativeindexfile;
    char* bladedirectory;
    char* libraryfile;
    char* libraryindexfile;
    char* bladepackagedirectory;
    char* packagefile;
    char* packageindexfile;
    char* relativefile;
    int rootdirlength;
    int filedirectorylength;
    bladefilename = bl_util_getbladefilename(modulename);
    // check relative to the current file...
    filedirectory = dirname((char*)strdup(currentfile));
    // fixing last path / if exists (looking at windows)...
    filedirectorylength = (int)strlen(filedirectory);
    if(filedirectory[filedirectorylength - 1] == '\\')
    {
        filedirectory[filedirectorylength - 1] = '\0';
    }
    // search system library if we are not looking for a relative module.
    if(!isrelative)
    {
        // firstly, search the local vendor directory for a matching module
        rootdir = getcwd(NULL, 0);
        // fixing last path / if exists (looking at windows)...
        rootdirlength = (int)strlen(rootdir);
        if(rootdir[rootdirlength - 1] == '\\')
        {
            rootdir[rootdirlength - 1] = '\0';
        }
        vendorfile = bl_util_mergepaths(bl_util_mergepaths(rootdir, LOCAL_PACKAGES_DIRECTORY LOCAL_SRC_DIRECTORY), bladefilename);
        if(bl_util_fileexists(vendorfile))
        {
            // stop a core library from importing itself
 
            path1 = realpath(vendorfile, NULL);
            path2 = realpath(currentfile, NULL);
            if(path1 != NULL)
            {
                if(path2 == NULL || memcmp(path1, path2, (int)strlen(path2)) != 0)
                {
                    return path1;
                }
            }
        }
        // or a matching package
        vendorindexfile = bl_util_mergepaths(bl_util_mergepaths(bl_util_mergepaths(rootdir, LOCAL_PACKAGES_DIRECTORY LOCAL_SRC_DIRECTORY), modulename),
                                                   LIBRARY_DIRECTORY_INDEX BLADE_EXTENSION);
        if(bl_util_fileexists(vendorindexfile))
        {
            // stop a core library from importing itself
            path1 = realpath(vendorindexfile, NULL);
            path2 = realpath(currentfile, NULL);
            if(path1 != NULL)
            {
                if(path2 == NULL || memcmp(path1, path2, (int)strlen(path2)) != 0)
                {
                    return path1;
                }
            }
        }
        // then, check in blade's default locations
        exedir = bl_util_getexedir();
        bladedirectory = bl_util_mergepaths(exedir, LIBRARY_DIRECTORY);
        // check blade libs directory for a matching module...
        libraryfile = bl_util_mergepaths(bladedirectory, bladefilename);
        if(bl_util_fileexists(libraryfile))
        {
            // stop a core library from importing itself
            path1 = realpath(libraryfile, NULL);
            path2 = realpath(currentfile, NULL);
            if(path1 != NULL)
            {
                if(path2 == NULL || memcmp(path1, path2, (int)strlen(path2)) != 0)
                {
                    return path1;
                }
            }
        }
        // check blade libs directory for a matching package...
        libraryindexfile = bl_util_mergepaths(bl_util_mergepaths(bladedirectory, modulename), bl_util_getbladefilename(LIBRARY_DIRECTORY_INDEX));
        if(bl_util_fileexists(libraryindexfile))
        {
            path1 = realpath(libraryindexfile, NULL);
            path2 = realpath(currentfile, NULL);
            if(path1 != NULL)
            {
                if(path2 == NULL || memcmp(path1, path2, (int)strlen(path2)) != 0)
                {
                    return path1;
                }
            }
        }
        // check blade vendor directory installed module...
        bladepackagedirectory = bl_util_mergepaths(exedir, PACKAGES_DIRECTORY);
        packagefile = bl_util_mergepaths(bladepackagedirectory, bladefilename);
        if(bl_util_fileexists(packagefile))
        {
            path1 = realpath(packagefile, NULL);
            path2 = realpath(currentfile, NULL);
            if(path1 != NULL)
            {
                if(path2 == NULL || memcmp(path1, path2, (int)strlen(path2)) != 0)
                {
                    return path1;
                }
            }
        }
        // check blade vendor directory installed package...
        packageindexfile = bl_util_mergepaths(bl_util_mergepaths(bladepackagedirectory, modulename), LIBRARY_DIRECTORY_INDEX BLADE_EXTENSION);
        if(bl_util_fileexists(packageindexfile))
        {
            path1 = realpath(packageindexfile, NULL);
            path2 = realpath(currentfile, NULL);
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
        relativefile = bl_util_mergepaths(filedirectory, bladefilename);
        if(bl_util_fileexists(relativefile))
        {
            // stop a user module from importing itself
            path1 = realpath(relativefile, NULL);
            path2 = realpath(currentfile, NULL);
            if(path1 != NULL)
            {
                if(path2 == NULL || memcmp(path1, path2, (int)strlen(path2)) != 0)
                {
                    return path1;
                }
            }
        }
        // or a matching package
        relativeindexfile = bl_util_mergepaths(bl_util_mergepaths(filedirectory, modulename), bl_util_getbladefilename(LIBRARY_DIRECTORY_INDEX));
        if(bl_util_fileexists(relativeindexfile))
        {
            path1 = realpath(relativeindexfile, NULL);
            path2 = realpath(currentfile, NULL);
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

uint32_t bl_util_hashbits(uint64_t hash)
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

uint16_t bl_util_reverseint16(uint16_t arg)
{
    return ((arg & 0xFF) << 8) | ((arg >> 8) & 0xFF);
}

uint32_t bl_util_reverseint32(uint32_t arg)
{
    uint32_t result;
    result = ((arg & 0xFF) << 24) | ((arg & 0xFF00) << 8) | ((arg >> 8) & 0xFF00) | ((arg >> 24) & 0xFF);
    return result;
}

uint64_t bl_util_reverseint64(uint64_t arg)
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

void bl_util_copyfloat(int islittleendian, void* dst, float f)
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

void bl_util_copydouble(int islittleendian, void* dst, double d)
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

char* bl_util_ulongtobuffer(char* buf, long num)
{
    *buf = '\0';
    do
    {
        *--buf = (char)((char)(num % 10) + '0');
        num /= 10;
    } while(num > 0);
    return buf;
}

float bl_util_parsefloat(int islittleendian, void* src)
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

double bl_util_parsedouble(int islittleendian, void* src)
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



