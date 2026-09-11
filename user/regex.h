#ifndef REGEX_H
#define REGEX_H
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "nfa.h"

int matchstr(const char *pattern, const char *str);

#endif
