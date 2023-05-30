
src=<<__eos__

        // symbols

        // (
        [TOK_NEWLINE] = { NULL, NULL, PREC_NONE },

        // (
        [TOK_LPAREN] = { bl_parser_rulegrouping, bl_parser_rulecall, PREC_CALL },

        // )
        [TOK_RPAREN] = { NULL, NULL, PREC_NONE },

        // [
        [TOK_LBRACKET] = { bl_parser_rulelist, bl_parser_ruleindexing, PREC_CALL },

        // ]
        [TOK_RBRACKET] = { NULL, NULL, PREC_NONE },

        // {
        [TOK_LBRACE] = { bl_parser_ruledict, NULL, PREC_NONE },

        // }
        [TOK_RBRACE] = { NULL, NULL, PREC_NONE },

        // ;
        [TOK_SEMICOLON] = { NULL, NULL, PREC_NONE },

        // ,
        [TOK_COMMA] = { NULL, NULL, PREC_NONE },

        // '\'
        [TOK_BACKSLASH] = { NULL, NULL, PREC_NONE },

        // !
        [TOK_BANG] = { bl_parser_ruleunary, NULL, PREC_NONE },

        // !=
        [TOK_BANGEQ] = { NULL, bl_parser_rulebinary, PREC_EQUALITY },

        // :
        [TOK_COLON] = { NULL, NULL, PREC_NONE },

        // @
        [TOK_AT] = { NULL, NULL, PREC_NONE },

        // .
        [TOK_DOT] = { NULL, bl_parser_ruledot, PREC_CALL },

        // ..
        [TOK_RANGE] = { NULL, bl_parser_rulebinary, PREC_RANGE },

        // ...
        [TOK_TRIDOT] = { NULL, NULL, PREC_NONE },

        // +
        [TOK_PLUS] = { bl_parser_ruleunary, bl_parser_rulebinary, PREC_TERM },

        // +=
        [TOK_PLUSEQ] = { NULL, NULL, PREC_NONE },

        // ++
        [TOK_INCREMENT] = { NULL, NULL, PREC_NONE },

        // -
        [TOK_MINUS] = { bl_parser_ruleunary, bl_parser_rulebinary, PREC_TERM },

        // -=
        [TOK_MINUSEQ] = { NULL, NULL, PREC_NONE },

        // --
        [TOK_DECREMENT] = { NULL, NULL, PREC_NONE },

        // *
        [TOK_MULTIPLY] = { NULL, bl_parser_rulebinary, PREC_FACTOR },

        // *=
        [TOK_MULTIPLYEQ] = { NULL, NULL, PREC_NONE },

        // **
        [TOK_POW] = { NULL, bl_parser_rulebinary, PREC_FACTOR },

        // **=
        [TOK_POWEQ] = { NULL, NULL, PREC_NONE },

        // '/'
        [TOK_DIVIDE] = { NULL, bl_parser_rulebinary, PREC_FACTOR },

        // '/='
        [TOK_DIVIDEEQ] = { NULL, NULL, PREC_NONE },

        // '//'
        [TOK_FLOOR] = { NULL, bl_parser_rulebinary, PREC_FACTOR },

        // '//='
        [TOK_FLOOREQ] = { NULL, NULL, PREC_NONE },

        // =
        [TOK_EQUAL] = { NULL, NULL, PREC_NONE },

        // ==
        [TOK_EQUALEQ] = { NULL, bl_parser_rulebinary, PREC_EQUALITY },

        // <
        [TOK_LESS] = { NULL, bl_parser_rulebinary, PREC_COMPARISON },

        // <=
        [TOK_LESSEQ] = { NULL, bl_parser_rulebinary, PREC_COMPARISON },

        // <<
        [TOK_LSHIFT] = { NULL, bl_parser_rulebinary, PREC_SHIFT },

        // <<=
        [TOK_LSHIFTEQ] = { NULL, NULL, PREC_NONE },

        // >
        [TOK_GREATER] = { NULL, bl_parser_rulebinary, PREC_COMPARISON },

        // >=
        [TOK_GREATEREQ] = { NULL, bl_parser_rulebinary, PREC_COMPARISON },

        // >>
        [TOK_RSHIFT] = { NULL, bl_parser_rulebinary, PREC_SHIFT },

        // >>=
        [TOK_RSHIFTEQ] = { NULL, NULL, PREC_NONE },

        // %
        [TOK_PERCENT] = { NULL, bl_parser_rulebinary, PREC_FACTOR },

        // %=
        [TOK_PERCENTEQ] = { NULL, NULL, PREC_NONE },

        // &
        [TOK_AMP] = { NULL, bl_parser_rulebinary, PREC_BIT_AND },

        // &=
        [TOK_AMPEQ] = { NULL, NULL, PREC_NONE },

        // |
        [TOK_BAR] = { bl_parser_ruleanon, bl_parser_rulebinary, PREC_BIT_OR },

        // |=
        [TOK_BAREQ] = { NULL, NULL, PREC_NONE },

        // ~
        [TOK_TILDE] = { bl_parser_ruleunary, NULL, PREC_UNARY },

        // ~=
        [TOK_TILDEEQ] = { NULL, NULL, PREC_NONE },

        // ^
        [TOK_XOR] = { NULL, bl_parser_rulebinary, PREC_BIT_XOR },

        // ^=
        [TOK_XOREQ] = { NULL, NULL, PREC_NONE },

        // ??
        [TOK_QUESTION] = { NULL, bl_parser_ruleconditional, PREC_CONDITIONAL },
    
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

        // regular numbers
        [TOK_REGNUMBER] = { bl_parser_rulenumber, NULL, PREC_NONE },

        // binary numbers
        [TOK_BINNUMBER] = { bl_parser_rulenumber, NULL, PREC_NONE },

        // octal numbers
        [TOK_OCTNUMBER] = { bl_parser_rulenumber, NULL, PREC_NONE },

        // hexadecimal numbers
        [TOK_HEXNUMBER] = { bl_parser_rulenumber, NULL, PREC_NONE },
        [TOK_IDENTIFIER] = { bl_parser_rulevariable, NULL, PREC_NONE },
        [TOK_INTERPOLATION] = { bl_parser_rulestrinterpol, NULL, PREC_NONE },
        [TOK_EOF] = { NULL, NULL, PREC_NONE },
        // error
        [TOK_ERROR] = { NULL, NULL, PREC_NONE },
        [TOK_EMPTY] = { bl_parser_ruleliteral, NULL, PREC_NONE },
        [TOK_UNDEFINED] = { NULL, NULL, PREC_NONE },
  
__eos__

=begin
struct AstRule
{
    bparseprefixfn prefix;
    bparseinfixfn infix;
    AstPrecedence precedence;
};

static inline AstRule* bl_parser_makerule(AstRule* dest, bparseprefixfn prefix, bparseinfixfn infix, AstPrecedence precedence)
{
    dest.prefix = prefix;
    dest.infix = infix;
    dest.precedence = precedence;
    return dest;
}
=end

space = (" " * 4)
src.split("\n").each do |line|
  m = line.match(/^\s*\[(?<id>\w+)\]\s*=\s*\{(?<data>.+?)\},$/)
  if !m then
    sl = line.strip
    next if sl.empty?
    printf("%s%s\n", space*2, sl)
  else
    id = m["id"]
    data = m["data"]
    printf("%scase %s:\n", space*2, id)
    printf("%sreturn bl_parser_makerule(&rule, %s);\n",  space*3, data.strip)
    printf("%sbreak;\n", space*3);
  end
end


