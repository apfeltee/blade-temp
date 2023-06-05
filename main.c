
#include "blade.h"


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
    size_t srclen;
    char* source;
    char* oldfile;
    (void)srclen;
    source = bl_util_readfile(file, &srclen);
    if(source == NULL)
    {
        // check if it's a Blade library directory by attempting to read the index file.
        oldfile = file;
        file = bl_util_appendstring((char*)strdup(file), "/" LIBRARY_DIRECTORY_INDEX BLADE_EXTENSION);
        source = bl_util_readfile(file, &srclen);
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


