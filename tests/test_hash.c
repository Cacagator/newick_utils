#include <string.h>
#include <stdio.h>

#include "hash.h"
#include "list.h"

struct test_data {
	int value;
	char *name;
};

int test_simple()
{
	char *test_name = "test_simple";
	char *test_value = "a string datum";
	struct hash *h;
	struct test_data td1 = { 0, "myname"} ;

	h = create_hash(100);

	hash_set(h, "one", test_value);
	hash_set(h, "td1", &td1);

	char *str = (char *) hash_get(h, "one");
	if (0 != strcmp(str, test_value)) {
		printf ("%s: expected %s, got %s.\n", test_name, test_value,
				str);
		return 1;
	}
	struct test_data *tdr = (struct test_data *) hash_get(h, "td1");
	if (0 != tdr->value) {
		printf ("%s: expected 0, got %d.\n", test_name, tdr->value);
		return 1;
	}
	if (0 != strcmp("myname", tdr->name)) {
		printf ("%s: expected 'myname', got '%s'.\n", test_name, tdr->name);
		return 1;
	}
	printf ("%s ok.\n", test_name);
	return 0;
}

int test_keys()
{
	char *test_name = "test_keys";

	struct hash *h = create_hash(4);

	hash_set(h, "one", "uno");
	hash_set(h, "two", "dos");
	hash_set(h, "three", "tres");
	hash_set(h, "four", "cuatro");
	hash_set(h, "five", "cinco"); 	/* one more elem than hash size - forces clash */

	struct llist *keys = hash_keys(h);

	if (5 != keys->count) {
		printf ("%s: expected 5 keys, got %d.\n", test_name, keys->count);
		return 1;
	}
	if (-1 == llist_index_of(keys, "one")) {
		printf ("%s: 'one' should be among the keys.\n", test_name);
		return 1;
	}
	if (-1 == llist_index_of(keys, "two")) {
		printf ("%s: 'two' should be among the keys.\n", test_name);
		return 1;
	}
	if (-1 == llist_index_of(keys, "three")) {
		printf ("%s: 'three' should be among the keys.\n", test_name);
		return 1;
	}
	if (-1 == llist_index_of(keys, "four")) {
		printf ("%s: 'four' should be among the keys.\n", test_name);
		return 1;
	}
	if (-1 == llist_index_of(keys, "five")) {
		printf ("%s: 'five' should be among the keys.\n", test_name);
		return 1;
	}

	printf ("%s ok.\n", test_name);
	return 0;
}


int main()
{
	int failures = 0;
	printf("Starting hash test...\n");
	failures += test_simple();
	failures += test_keys();
	if (0 == failures) {
		printf("All tests ok.\n");
	} else {
		printf("%d test(s) FAILED.\n", failures);
		return 1;
	}

	return 0;
}