
#ifndef QUERY_AST_H
#define QUERY_AST_H

#include "set.h"

//#pragma once // ??, tredje svar fra chat

typedef enum {
    AST_WORD,      // Et enkelt søkeord
    AST_AND,       // &&
    AST_OR,        // ||
    AST_AND_NOT    // &!
} ast_type_t;

typedef struct query_ast {  // ast_node istedenfor query_ast?
    ast_type_t type;

    // Hvis type == AST_WORD
    char* word;

    // Hvis type != AST_WORD
    struct query_ast* left;
    struct query_ast* right;
} query_ast_t;

// Funksjoner til å lage og frigjøre AST, første svar fra chat
query_ast_t* ast_create_word(const char* word);
query_ast_t* ast_create_op(ast_type_t type, query_ast_t* left, query_ast_t* right);
void ast_free(query_ast_t* ast);

// én funksjon per nivå i BNF, andre svar fra chat
//query_ast_t *parse_query(TokenStream *ts);
//query_ast_t *parse_andterm(TokenStream *ts);
//query_ast_t *parse_orterm(TokenStream *ts);
//query_ast_t *parse_term(TokenStream *ts);

// Tredje svar fra chat
query_ast_t* ast_create_word(const char* word);
query_ast_t* ast_create_op(ast_type_t type, query_ast_t* left, query_ast_t* right);
void ast_destroy(query_ast_t* root);
void ast_print(query_ast_t* root); // For debugging


typedef enum {
    TOKEN_WORD,
    TOKEN_AND,     // &&
    TOKEN_OR,      // ||
    TOKEN_AND_NOT, // &!
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_END
} TokenType;

typedef struct {
    TokenType type;
    char* value; // only for TOKEN_WORD
} Token;

typedef struct AST {
    TokenType type;
    char* word;               // only used if type == AST_WORD
    struct AST* left;
    struct AST* right;
} AST;


#endif
