/**
 * @implements index.h
 */

/* set log level for prints in this file */
#define LOG_LEVEL LOG_LEVEL_DEBUG

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h> // for LINE_MAX

#include "printing.h"
#include "index.h"
#include "defs.h"
#include "common.h"
#include "list.h"
#include "map.h"
#include "set.h"

struct index{
    map_t *terms;
    size_t n_docs;
};

/**
 * You may utilize this for lists of query results, or write your own comparison function.
 */
ATTR_MAYBE_UNUSED
int compare_results_by_score(query_result_t *a, query_result_t *b){
    if (a->score > b->score)
    {
        return -1;
    }
    if (a->score < b->score)
    {
        return 1;
    }
    return 0;
}

index_t *index_create(){
    index_t *index = malloc(sizeof(index_t));
    if (index == NULL)
    {
        pr_error("Failed to allocate memory for index\n");
        return NULL;
    }

    index->terms = map_create((cmp_fn)strcmp, hash_string_fnv1a64);
    index->n_docs = 0;

    return index;
}

void index_destroy(index_t *index){
    // Iterate over the map and free each document list
    map_iter_t *iter = map_createiter(index->terms);
    while (map_hasnext(iter)){
        entry_t *map_entry = map_next(iter);
        map_t *doc_freq_map = map_entry->val;

        // Free each key-value pair in the map
        map_iter_t *doc_iter = map_createiter(doc_freq_map);

        while (map_hasnext(doc_iter)){
            entry_t *key_value_entry = map_next(doc_iter);

            char *doc_name = key_value_entry->key;
            query_result_t *result = key_value_entry->val;
            
            free(doc_name);
            free(result->doc_name);
            free(result);
            free(key_value_entry);
        }
        map_destroyiter(doc_iter);

        // Free each term and its map
        free(map_entry->key);
        free(map_entry->val);
        free(map_entry);

    }
    map_destroyiter(iter);
    map_destroy(index->terms, free, free);
    free(index);
}

int index_document(index_t *index, char *doc_name, list_t *terms){
    list_iter_t *iter = list_createiter(terms);
    if (!iter){
        return -1;
    }

    while (list_hasnext(iter)){
        char *term = strdup(list_next(iter));

        entry_t *existing_term = map_get(index->terms, term); // check if term is already in map

        if (existing_term){
            map_t *doc_freq_map = existing_term->val;

            entry_t *node = map_get(doc_freq_map, doc_name);
            if (node){ // If document has been added to the terms map
                // Term is found one more time in the same document, inkrement score
                query_result_t *result = (query_result_t *)node->val;

                result->score += 1.0;
            }
            else{
                // Term has been found for the first time in a document, add document to doc_freq_map
                query_result_t *result = result_create(strdup(doc_name), 1.0);
                if (!result){
                    return -1;
                }
            
                map_insert(doc_freq_map, strdup(doc_name), result);
            }
        }
        else{
            // A map is yet to be created for term
            map_t *doc_freq_map = map_create((cmp_fn)strcmp, hash_string_fnv1a64);
            if (!doc_freq_map){
                return -1;
            }
            query_result_t *result = result_create(strdup(doc_name), 1.0);
            if (!result){
                return -1;
            }

            map_insert(doc_freq_map, strdup(doc_name), result);

            map_insert(index->terms, term, doc_freq_map);
        }

    }
    list_destroyiter(iter);

    index->n_docs++; 

    return 0;
}

list_t *index_query(index_t *index, list_t *query_tokens, char *errmsg){
    UNUSED(errmsg);

    list_iter_t *parse_iter = list_createiter(query_tokens);

    node_t *AST = parse(parse_iter);
    list_destroyiter(parse_iter);

    set_t *matching_docs = evaluate_ast(index, AST); // A set of all documents that match the search

    list_t *doc_list = list_create((cmp_fn)compare_results_by_score);  // List that will hold the results

    // Add the results to the list
    set_iter_t *set_iter = set_createiter(matching_docs);
    while(set_hasnext(set_iter)){
        query_result_t *result = set_next(set_iter);

        list_addlast(doc_list, result);
    }
    set_destroyiter(set_iter);
    set_destroy(matching_docs, NULL);
    
    list_sort(doc_list);

    return doc_list;
}

void index_stat(index_t *index, size_t *n_docs, size_t *n_terms){
    *n_docs = index->n_docs;
    *n_terms = map_length(index->terms);
}

query_result_t *result_create(char *doc_name, double score){
    query_result_t *result = malloc(sizeof(query_result_t));

    result->doc_name = doc_name;
    result->score = score;

    return result;
}

int compare_result_doc_name(query_result_t *item, query_result_t *item2){
    return strcmp(item->doc_name, item2->doc_name);
}

set_t *evaluate_ast(index_t *index, node_t *node){
    if (node->type == WORD){
        entry_t *docs = map_get(index->terms, node->word);  // Get the map that holds the documents that contain the term

        map_t *doc_map = docs->val;  // The map that holds the documents that contain the term

        // Create a set to hold the documents
        set_t *set = set_create((cmp_fn)compare_result_doc_name);

        map_iter_t *doc_iter = map_createiter(doc_map);
        
        // Insert all documents that contain the term into the set
        while (map_hasnext(doc_iter)){
            entry_t *doc = map_next(doc_iter);

            query_result_t *result = doc->val;
            query_result_t *dup_result = result_create(strdup(result->doc_name), result->score);

            set_insert(set, dup_result);
        }

        return set;
    }

    // inspirert av chatGPT: https://chatgpt.com/share/6820db13-1490-8010-be48-4705e6c87214
    set_t *left = evaluate_ast(index, node->left);
    set_t *right = evaluate_ast(index, node->right);

    // Set to hold the results
    set_t *result = NULL;

    if(node->type == OR){
        result = set_union(left, right);
    }
    else if(node->type == AND){
        result = set_intersection(left, right);
    }
    else if(node->type == AND_NOT){
        result = set_difference(left, right);
    }

    set_destroy(left, NULL);
    set_destroy(right, NULL);

    return result;
}

// parser
node_t *parse(list_iter_t *iter){
    node_t *root_node = parse_andnotterm(iter);

    return root_node;
}

// Inspirert av chatGPT: https://chatgpt.com/share/6820db13-1490-8010-be48-4705e6c87214
node_t *parse_andnotterm(list_iter_t *iter){
    node_t *left = parse_andterm(iter);

    if (list_hasnext(iter)){
        char *token = list_peek(iter);

        if (strcmp(token, "&!") == 0) {
            list_next(iter); // skip '&!', gå til neste token
            node_t *right = parse_andterm(iter);
            left = create_node(AND_NOT, NULL, left, right);
        } 
    }
    return left;
}

node_t *parse_andterm(list_iter_t *iter){
    node_t *left = parse_orterm(iter);

    if (list_hasnext(iter)){
        char *token = list_peek(iter);

        if (strcmp(token, "&&") == 0) {
            list_next(iter); // skip '&&', gå til neste token
            node_t *right = parse_orterm(iter);
            left = create_node(AND, NULL, left, right);
        } 
    }
    return left;
}

node_t *parse_orterm(list_iter_t *iter){
    node_t *left = parse_term(iter);

    if (list_hasnext(iter)){
        char *token = list_peek(iter);

        if (strcmp(token, "||") == 0) {
            list_next(iter); // skip '||', gå til neste token
            node_t *right = parse_term(iter);
            left = create_node(OR, NULL, left, right);
        }
    }
    return left;
}

node_t *parse_term(list_iter_t *iter){
    char *token = list_peek(iter);
    if (strcmp(token, "(") == 0){
        list_next(iter); // skip '(', gå til neste token
        node_t *parethesis_AST = parse_andnotterm(iter);
        return parethesis_AST;
    }

    token = list_next(iter);
    node_t *node = create_node(WORD, token, NULL, NULL);
    return node; 
}

node_t *create_node(operator_t type, char* word, struct node* left, struct node* right){
    node_t *node = malloc(sizeof(node_t));

    node->type = type;
    node->word = word;
    node->left = left;
    node->right = right;

    return node; 
}