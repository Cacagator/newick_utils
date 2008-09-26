/* match.c: match a tree to a pattern tree (subgraph) */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/*
#include <assert.h>
#include <math.h>

#include "node_set.h"
*/
#include "parser.h"
#include "to_newick.h"
#include "tree.h"
#include "order_tree.h"
#include "hash.h"
#include "list.h"
#include "rnode.h"
#include "link.h"
#include "nodemap.h"
#include "common.h"
#include "redge.h"

void newick_scanner_set_string_input(char *);
void newick_scanner_clear_string_input();
void newick_scanner_set_file_input(FILE *);

struct parameters {
	char *pattern;
	FILE *target_trees;
	int reverse;
};

void help(char* argv[])
{
	printf(
"Matches a tree to a pattern tree\n"
"\n"
"Synopsis\n"
"--------\n"
"%s [-v] <target tree filename|-> <pattern tree>\n"
"\n"
"Input\n"
"-----\n"
"\n"
"The first argument is the name of the file containing the target tree (to\n"
"which support values are to be attributed), or '-' (in which case the tree\n"
"is read on stdin).\n"
"\n"
"The second argument is a pattern tree\n"
"\n"
"Output\n"
"------\n"
"\n"
"Outputs the target tree if the pattern tree is a subgraph of it.\n"
"\n"
"Options\n"
"-------\n"
"\n"
"    -v: prints tree which do NOT match the pattern.\n"
"\n"
"Limits & Assumptions\n"
"--------------------\n"
"\n"
"Assumes that the labels are leaf labels, and that they are unique in\n"
"all trees (both target and pattern)\n"
"\n"
"Example\n"
"-------\n"
"\n"
"# Prints trees in data/vrt_gen.nw where Tamias is closer to Homo than it is\n"
"# to Vulpes:\n"
"$ %s data/vrt_gen.nw '((Tamias,Homo),Vulpes);'\n",
"\n"
"# Prints trees in data/vrt_gen.nw where Tamias is NOT closer to Homo than it is\n"
"# to Vulpes:\n"
"$ %s -v data/vrt_gen.nw '((Tamias,Homo),Vulpes);'\n",

	argv[0],
	argv[0],
	argv[0]
	      );
}

struct parameters get_params(int argc, char *argv[])
{
	struct parameters params;
	char opt_char;


	params.reverse = FALSE;

	/* parse options and switches */
	while ((opt_char = getopt(argc, argv, "hv")) != -1) {
		switch (opt_char) {
		case 'h':
			help(argv);
			exit(EXIT_SUCCESS);
		/* we keep this for debugging, but not documented */
		case 'v':
			params.reverse = TRUE;
			break;
		}
	}
	/* get arguments */
	if (2 == (argc - optind))	{
		if (0 != strcmp("-", argv[optind])) {
			FILE *fin = fopen(argv[optind], "r");
			if (NULL == fin) {
				perror(NULL);
				exit(EXIT_FAILURE);
			}
			params.target_trees = fin;
		} else {
			params.target_trees = stdin;
		}
		params.pattern = argv[optind+1];
	} else {
		fprintf(stderr, "Usage: %s [-hv] <target trees filename|-> <pattern>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	return params;
}

/* Get pattern tree and order it */

struct rooted_tree *get_ordered_pattern_tree(char *pattern)
{
	struct rooted_tree *pattern_tree;

	newick_scanner_set_string_input(pattern);
	pattern_tree = parse_tree();
	if (NULL == pattern_tree) {
		fprintf (stderr, "Could not parse pattern tree '%s'\n", pattern);
		exit(EXIT_FAILURE);
	}
	newick_scanner_clear_string_input();

	order_tree(pattern_tree);

	return pattern_tree;
}

/* We only consider leaf labels. This might change if keeping internal labels
 * proves useful. */

void remove_inner_node_labels(struct rooted_tree *target_tree)
{
	struct list_elem *el;

	for (el=target_tree->nodes_in_order->head; NULL != el; el=el->next) {
		struct rnode *current = el->data;
		if (is_leaf(current)) continue;
		free(current->label);
		current->label = "";
	}
}

/* Removes all nodes in target tree whose labels are not found in the 'kept'
 * hash */

void prune_extra_labels(struct rooted_tree *target_tree, struct hash *kept)
{
	struct list_elem *el;

	for (el=target_tree->nodes_in_order->head; NULL != el; el=el->next) {
		struct rnode *current = el->data;
		char *label = current->label;
		if (0 == strcmp("", label)) continue;
		if (is_root(current)) continue;
		if (NULL == hash_get(kept, current->label)) {
			/* not in 'kept': remove */
			struct rnode *new_root;
			new_root = unlink_rnode(current);
			if (NULL != new_root)
				target_tree->root = new_root;
		}
	}
}

void remove_branch_lengths(struct rooted_tree *target_tree)
{
	struct list_elem *el;

	for (el=target_tree->nodes_in_order->head; NULL != el; el=el->next) {
		struct rnode *current = el->data;
		struct redge *cur_edge = current->parent_edge;
		if (strcmp("", cur_edge->length_as_string) != 0) {
			free(cur_edge->length_as_string);
			cur_edge->length_as_string = "";
		}
	}
}

int main(int argc, char *argv[])
{
	struct rooted_tree *pattern_tree;	
	struct rooted_tree *tree;	
	char *pattern_newick;
	struct hash *pattern_labels;

	struct parameters params = get_params(argc, argv);

	pattern_tree = get_ordered_pattern_tree(params.pattern);
	pattern_newick = to_newick(pattern_tree->root);
	// printf ("%s\n", pattern_newick);
	pattern_labels = create_label2node_map(pattern_tree->nodes_in_order);

	/* get_ordered_pattern_tree() causes a tree to be read from a string,
	 * which means that we must now tell the lexer to change its input
	 * source. It's not enough to just set the external FILE pointer
	 * 'nwsin' to standard input or the user-supplied file, apparently:
	 * this would segfault. */
	newick_scanner_set_file_input(params.target_trees);

	while (NULL != (tree = parse_tree())) {
		char *original_newick = to_newick(tree->root);
		remove_inner_node_labels(tree);
		prune_extra_labels(tree, pattern_labels);
		remove_branch_lengths(tree);	
		order_tree(tree);
		char *processed_newick = to_newick(tree->root);
		// printf ("%s\n", processed_newick);
		int match = (0 == strcmp(processed_newick, pattern_newick));
		match = params.reverse ? !match : match;
		if (match) printf ("%s\n", original_newick);
		free(processed_newick);
		free(original_newick);
	}

	free(pattern_newick);
	return 0;
}