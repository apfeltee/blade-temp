
#include "blade.h"

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
        case OP_GREATERTHAN:
            return bl_blob_disaspriminst("gt", offset);
        case OP_LESSTHAN:
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
        case OP_BITAND:
            return bl_blob_disaspriminst("band", offset);
        case OP_BITOR:
            return bl_blob_disaspriminst("bor", offset);
        case OP_BITXOR:
            return bl_blob_disaspriminst("bxor", offset);
        case OP_LEFTSHIFT:
            return bl_blob_disaspriminst("lshift", offset);
        case OP_RIGHTSHIFT:
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



