#ifndef UTILS_H
#define UTILS_H

inline bool is_whitespace(char s)
{
    return (s == ' ' || s == '\t' || s == '\n');
}

inline bool is_space(char s)
{
    return (s == ' ' || s == '\t');
}

inline bool is_ident_start(char s)
{
    return ((s >= 'a' && s <= 'z')
            || (s >= 'A' && s <= 'Z')
            || s == '_'
       );
}

inline bool is_ident_char(char s)
{
    return ((s >= 'a' && s <= 'z')
            || (s >= 'A' && s <= 'Z')
            || (s >= '0' && s <= '9')
            || s == '_'
       );
}

inline bool is_digit_char(char s)
{
    return (s >= '0' && s <= '9');
}

inline bool is_hex_char(char s)
{
    return ((s >= 'a' && s <= 'f')
            || (s >= 'A' && s <= 'F')
            || (s >= '0' && s <= '9')
       );
}


#endif
