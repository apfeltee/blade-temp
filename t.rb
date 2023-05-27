
src=<<__eos__
#define bl_value_isstring(v) bl_value_isobjtype(v, OBJ_STRING)
#define bl_value_isnativefunction(v) bl_value_isobjtype(v, OBJ_NATIVEFUNCTION)
#define bl_value_isscriptfunction(v) bl_value_isobjtype(v, OBJ_SCRIPTFUNCTION)
#define bl_value_isclosure(v) bl_value_isobjtype(v, OBJ_CLOSURE)
#define bl_value_isclass(v) bl_value_isobjtype(v, OBJ_CLASS)
#define bl_value_isinstance(v) bl_value_isobjtype(v, OBJ_INSTANCE)
#define bl_value_isboundfunction(v) bl_value_isobjtype(v, OBJ_BOUNDFUNCTION)
#define bl_value_isnil(v) ((v).type == VAL_NIL)
#define bl_value_isbool(v) ((v).type == VAL_BOOL)
#define bl_value_isnumber(v) ((v).type == VAL_NUMBER)
#define bl_value_isobject(v) ((v).type == VAL_OBJ)
#define bl_value_isempty(v) ((v).type == VAL_EMPTY)
#define bl_value_ismodule(v) bl_value_isobjtype(v, OBJ_MODULE)
#define bl_value_ispointer(v) bl_value_isobjtype(v, OBJ_PTR)

// containers
#define bl_value_isbytes(v) bl_value_isobjtype(v, OBJ_BYTES)
#define bl_value_isarray(v) bl_value_isobjtype(v, OBJ_ARRAY)
#define bl_value_isdict(v) bl_value_isobjtype(v, OBJ_DICT)
#define bl_value_isfile(v) bl_value_isobjtype(v, OBJ_FILE)
#define bl_value_isrange(v) bl_value_isobjtype(v, OBJ_RANGE)
__eos__

src.split("\n").each do |line|
  m = line.match(/^\s*#\s*define\s*(?<name>\w+)\((?<va>\w+)\)\s*(?<body>.*)$/)
  if m then
    name = m["name"]
    va = m["va"]
    body = m["body"]
    printf("bool %s(Value %s)\n", name, va)
    printf("{\n    return %s;\n}\n\n", body)
  end
end


