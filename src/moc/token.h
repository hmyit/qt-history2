#ifndef TOKEN_H
#define TOKEN_H

enum PP_Token {
    PP_NOTOKEN,
    PP_IDENTIFIER,
    PP_INTEGER_LITERAL,
    PP_CHARACTER_LITERAL,
    PP_STRING_LITERAL,
    PP_LANGLE,
    PP_RANGLE,
    PP_LPAREN,
    PP_RPAREN,
    PP_COMMA,
    PP_FIRST_STATEMENT, PP_DEFINE = PP_FIRST_STATEMENT,
    PP_UNDEF,
    PP_IF,
    PP_IFDEF,
    PP_IFNDEF,
    PP_ELIF,
    PP_ELSE,
    PP_ENDIF,
    PP_INCLUDE,
    PP_HASHHASH,
    PP_HASH, PP_LAST_STATEMENT = PP_HASH,
    PP_DEFINED,
    PP_PLUS,
    PP_MINUS,
    PP_STAR,
    PP_SLASH,
    PP_PERCENT,
    PP_HAT,
    PP_AND,
    PP_OR,
    PP_TILDE,
    PP_NOT,
    PP_LTLT,
    PP_GTGT,
    PP_EQEQ,
    PP_NE,
    PP_LE,
    PP_GE,
    PP_ANDAND,
    PP_OROR,
    PP_QUESTION,
    PP_COLON,
    PP_FLOATING_LITERAL,
    PP_QUOTE,
    PP_SINGLEQUOTE,
    PP_DIGIT,
    PP_CHARACTER,
    PP_WHITESPACE,
    PP_NEWLINE,
    PP_CPP_COMMENT,
    PP_C_COMMENT,
    PP_INCOMPLETE
};

enum Token {
    NOTOKEN,
    IDENTIFIER,
    INTEGER_LITERAL,
    CHARACTER_LITERAL,
    STRING_LITERAL,
    BOOLEAN_LITERAL,
    HEADER_NAME,
    LANGLE,
    RANGLE,
    LPAREN,
    RPAREN,
    ELIPSIS,
    COMMMMA,
    LBRACK,
    RBRACK,
    LBRACE,
    RBRACE,
    EQ,
    SCOPE,
    SEMIC,
    COLON,
    DOTSTAR,
    QUESTION,
    DOT,
    DYNAMIC_CAST,
    STATIC_CAST,
    REINTERPRET_CAST,
    CONST_CAST,
    TYPEID,
    THIS,
    TEMPLATE,
    THROW,
    TRY,
    CATCH,
    TYPEDEF,
    FRIEND,
    CLASS,
    NAMESPACE,
    ENUM,
    STRUCT,
    UNION,
    VIRTUAL,
    PRIVATE,
    PROTECTED,
    PUBLIC,
    EXPORT,
    AUTO,
    REGISTER,
    EXTERN,
    MUTABLE,
    ASM,
    USING,
    INLINE,
    EXPLICIT,
    STATIC,
    CONST,
    VOLATILE,
    OPERATOR,
    SIZEOF,
    NEW,
    DELETE,
    PLUS,
    MINUS,
    STAR,
    SLASH,
    PERCENT,
    HAT,
    AND,
    OR,
    TILDE,
    NOT,
    PLUS_EQ,
    MINUS_EQ,
    STAR_EQ,
    SLASH_EQ,
    PERCENT_EQ,
    HAT_EQ,
    AND_EQ,
    OR_EQ,
    LTLT,
    GTGT,
    GTGT_EQ,
    LTLT_EQ,
    EQEQ,
    NE,
    LE,
    GE,
    ANDAND,
    OROR,
    INCR,
    DECR,
    COMMA,
    ARROW_STAR,
    ARROW,
    CHAR,
    WCHAR,
    BOOL,
    SHORT,
    INT,
    LONG,
    SIGNED,
    UNSIGNED,
    FLOAT,
    DOUBLE,
    VOID,
    CASE,
    DEFAULT,
    IF,
    ELSE,
    SWITCH,
    WHILE,
    DO,
    FOR,
    BREAK,
    CONTINUE,
    GOTO,
    RETURN,
    QT_MOC_COMPAT_TOKEN,
    Q_META_TOKEN_BEGIN,
    Q_OBJECT_TOKEN = Q_META_TOKEN_BEGIN,
    Q_PROPERTY_TOKEN,
    Q_OVERRIDE_TOKEN,
    Q_ENUMS_TOKEN,
    Q_FLAGS_TOKEN,
    Q_CLASSINFO_TOKEN,
    Q_INTERFACES_TOKEN,
    SIGNALS,
    SLOTS,
    Q_PRIVATE_SLOT_TOKEN,
    Q_META_TOKEN_END,
    SPECIAL_TREATMENT_MARK = Q_META_TOKEN_END,
    FLOATING_LITERAL,
    HASH,
    QUOTE,
    SINGLEQUOTE,
    DIGIT,
    CHARACTER,
    NEWLINE,
    WHITESPACE,
    INCOMPLETE
};

#endif
