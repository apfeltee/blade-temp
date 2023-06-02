
#include "blade.h"


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
        mode = (ObjString*)bl_mem_gcprotect(vm, (Object*)bl_string_copystringlen(vm, "r", 1));
    }
    ObjFile* file = (ObjFile*)bl_mem_gcprotect(vm, (Object*)bl_object_makefile(vm, path, mode));
    file_open(file);
    RETURN_OBJ(file);
}

static bool objfn_file_exists(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(exists, 0);
    ObjFile* file = AS_FILE(METHOD_OBJECT);
    DENY_STD();
    RETURN_BOOL(bl_util_fileexists(file->path->chars));
}

static bool objfn_file_close(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(close, 0);
    file_close(AS_FILE(METHOD_OBJECT));
    return bl_value_returnempty(vm, args);
    ;
}

static bool objfn_file_open(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(open, 0);
    file_open(AS_FILE(METHOD_OBJECT));
    return bl_value_returnempty(vm, args);
    ;
}

static bool objfn_file_isopen(VMState* vm, int argcount, Value* args)
{
    ObjFile* file;
    (void)vm;
    (void)argcount;
    file = AS_FILE(METHOD_OBJECT);
    RETURN_BOOL(bl_object_isstdfile(file) || (file->isopen && file->file != NULL));
}

static bool objfn_file_isclosed(VMState* vm, int argcount, Value* args)
{
    ObjFile* file;
    (void)argcount;
    (void)vm;
    file = AS_FILE(METHOD_OBJECT);
    RETURN_BOOL(!bl_object_isstdfile(file) && !file->isopen && file->file == NULL);
}

static bool objfn_file_read(VMState* vm, int argcount, Value* args)
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

static bool objfn_file_gets(VMState* vm, int argcount, Value* args)
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

static bool objfn_file_write(VMState* vm, int argcount, Value* args)
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

static bool objfn_file_puts(VMState* vm, int argcount, Value* args)
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

static bool objfn_file_number(VMState* vm, int argcount, Value* args)
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

static bool objfn_file_istty(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(istty, 0);
    ObjFile* file = AS_FILE(METHOD_OBJECT);
    if(bl_object_isstdfile(file))
    {
        RETURN_BOOL(isatty(fileno(file->file)) && fileno(file->file) == fileno(stdout));
    }
    RETURN_FALSE;
}

static bool objfn_file_flush(VMState* vm, int argcount, Value* args)
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

static bool objfn_file_stats(VMState* vm, int argcount, Value* args)
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

static bool objfn_file_symlink(VMState* vm, int argcount, Value* args)
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

static bool objfn_file_delete(VMState* vm, int argcount, Value* args)
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

static bool objfn_file_rename(VMState* vm, int argcount, Value* args)
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

static bool objfn_file_path(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(path, 0);
    ObjFile* file = AS_FILE(METHOD_OBJECT);
    DENY_STD();
    RETURN_OBJ(file->path);
}

static bool objfn_file_mode(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(mode, 0);
    ObjFile* file = AS_FILE(METHOD_OBJECT);
    DENY_STD();
    RETURN_OBJ(file->mode);
}

static bool objfn_file_name(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(name, 0);
    ObjFile* file = AS_FILE(METHOD_OBJECT);
    DENY_STD();
    char* name = bl_util_getrealfilename(file->path->chars);
    RETURN_L_STRING(name, strlen(name));
}

static bool objfn_file_abspath(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(abspath, 0);
    ObjFile* file = AS_FILE(METHOD_OBJECT);
    DENY_STD();
    char* abspath = realpath(file->path->chars, NULL);
    if(abspath != NULL)
        RETURN_L_STRING(abspath, strlen(abspath));
    RETURN_L_STRING("", 0);
}

static bool objfn_file_copy(VMState* vm, int argcount, Value* args)
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

static bool objfn_file_truncate(VMState* vm, int argcount, Value* args)
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

static bool objfn_file_chmod(VMState* vm, int argcount, Value* args)
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

static bool objfn_file_settimes(VMState* vm, int argcount, Value* args)
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

static bool objfn_file_seek(VMState* vm, int argcount, Value* args)
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

static bool objfn_file_tell(VMState* vm, int argcount, Value* args)
{
    ENFORCE_ARG_COUNT(tell, 0);
    ObjFile* file = AS_FILE(METHOD_OBJECT);
    DENY_STD();
    RETURN_NUMBER(ftell(file->file));
}

void bl_state_initfilemethods(VMState* vm)
{
    // file methods
    bl_object_defnativemethod(vm, &vm->classobjfile->methods, "exists", objfn_file_exists);
    bl_object_defnativemethod(vm, &vm->classobjfile->methods, "close", objfn_file_close);
    bl_object_defnativemethod(vm, &vm->classobjfile->methods, "open", objfn_file_open);
    bl_object_defnativemethod(vm, &vm->classobjfile->methods, "read", objfn_file_read);
    bl_object_defnativemethod(vm, &vm->classobjfile->methods, "gets", objfn_file_gets);
    bl_object_defnativemethod(vm, &vm->classobjfile->methods, "write", objfn_file_write);
    bl_object_defnativemethod(vm, &vm->classobjfile->methods, "puts", objfn_file_puts);
    bl_object_defnativemethod(vm, &vm->classobjfile->methods, "number", objfn_file_number);
    bl_object_defnativemethod(vm, &vm->classobjfile->methods, "istty", objfn_file_istty);
    bl_object_defnativemethod(vm, &vm->classobjfile->methods, "isopen", objfn_file_isopen);
    bl_object_defnativemethod(vm, &vm->classobjfile->methods, "isclosed", objfn_file_isclosed);
    bl_object_defnativemethod(vm, &vm->classobjfile->methods, "flush", objfn_file_flush);
    bl_object_defnativemethod(vm, &vm->classobjfile->methods, "stats", objfn_file_stats);
    bl_object_defnativemethod(vm, &vm->classobjfile->methods, "symlink", objfn_file_symlink);
    bl_object_defnativemethod(vm, &vm->classobjfile->methods, "delete", objfn_file_delete);
    bl_object_defnativemethod(vm, &vm->classobjfile->methods, "rename", objfn_file_rename);
    bl_object_defnativemethod(vm, &vm->classobjfile->methods, "path", objfn_file_path);
    bl_object_defnativemethod(vm, &vm->classobjfile->methods, "abspath", objfn_file_abspath);
    bl_object_defnativemethod(vm, &vm->classobjfile->methods, "copy", objfn_file_copy);
    bl_object_defnativemethod(vm, &vm->classobjfile->methods, "truncate", objfn_file_truncate);
    bl_object_defnativemethod(vm, &vm->classobjfile->methods, "chmod", objfn_file_chmod);
    bl_object_defnativemethod(vm, &vm->classobjfile->methods, "settimes", objfn_file_settimes);
    bl_object_defnativemethod(vm, &vm->classobjfile->methods, "seek", objfn_file_seek);
    bl_object_defnativemethod(vm, &vm->classobjfile->methods, "tell", objfn_file_tell);
    bl_object_defnativemethod(vm, &vm->classobjfile->methods, "mode", objfn_file_mode);
    bl_object_defnativemethod(vm, &vm->classobjfile->methods, "name", objfn_file_name);
}


