#include <stddef.h>
#include <stdlib.h>

void *
bsearch(const void *key, const void *base, size_t nmemb, size_t size,
        int (*compar)(const void *, const void *))
{
	const void *pivot;
	int rc;
	size_t corr;

	// We end up leaving early if nmemb is 0,
	// which fits with what C2y says.
	while (nmemb) {
		/* algorithm needs -1 correction if remaining elements are an even number.
		 */
		corr = nmemb % 2;
		nmemb /= 2;
		pivot = (const char *)base + (nmemb * size);
		rc = compar(key, pivot);

		if (rc > 0) {
			base = (const char *)pivot + size;
			/* applying correction */
			nmemb -= (1 - corr);
		} else if (rc == 0) {
			return (void *)pivot;
		}
	}

	return NULL;
}
