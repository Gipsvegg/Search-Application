#include "parser.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static AST* parse_query(Token* tokens, size_t* pos);
static AST* parse_andterm(Token* tokens, size_t* pos);
static AST* parse_orterm(Token* tokens, size_t* pos);
static AST* parse_term(Token* tokens, size_t* pos);

static Token current(Token* tokens, size_t* pos) {
    return tokens[*pos];
}

static void advance(size_t* pos) {
    (*pos)++;
}

static AST* parse_term(Token* tokens, size_t* pos) {
    Token tok = current(tokens, pos);
    if (tok.type == TOKEN_LPAREN) {
        advance(pos); // skip '('
        AST* node = parse_query(tokens, pos);
        if (current(tokens, pos).type != TOKEN_RPAREN) {
            fprintf(stderr, "Feil: mangler høyre parentes\n");
            exit(1);
        }
        advance(pos);
        return node;
    } else if (tok.type == TOKEN_WORD) {
        AST* node = ast_create_word(tok.value);
        advance(pos);
        return node;
    } else {
        fprintf(stderr, "Feil: uventet token i term\n");
        exit(1);
    }
}

static AST* parse_orterm(Token* tokens, size_t* pos) {
    AST* left = parse_term(tokens, pos);
    while (current(tokens, pos).type == TOKEN_OR) {
        advance(pos);
        AST* right = parse_term(tokens, pos);
        left = ast_create_op(AST_OR, left, right);
    }
    return left;
}

static AST* parse_andterm(Token* tokens, size_t* pos) {
    AST* left = parse_orterm(tokens, pos);
    while (current(tokens, pos).type == TOKEN_AND) {
        advance(pos);
        AST* right = parse_orterm(tokens, pos);
        left = ast_create_op(AST_AND, left, right);
    }
    return left;
}

static AST* parse_query(Token* tokens, size_t* pos) {
    AST* left = parse_andterm(tokens, pos);
    if (current(tokens, pos).type == TOKEN_AND_NOT) {
        advance(pos);
        AST* right = parse_query(tokens, pos);
        left = ast_create_op(AST_AND_NOT, left, right);
    }
    return left;
}

AST* parse_tokens(Token* tokens) {
    size_t pos = 0;
    AST* root = parse_query(tokens, &pos);
    if (tokens[pos].type != TOKEN_END) {
        fprintf(stderr, "Feil: uventet token etter fullført spørring\n");
        exit(1);
    }
    return root;
}
