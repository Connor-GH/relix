#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>

char *__global_locale[LC_ALL];

locale_t
duplocale(locale_t locale)
{
	errno = ENOSYS;
	return (locale_t)0;
}
void
freelocale(locale_t locale)
{
}

// This is the definition for POSIX locale.
// https://pubs.opengroup.org/onlinepubs/9799919799/basedefs/V1_chap07.html#tag_07_03_04
static struct lconv current_locale = {

	.decimal_point = ".",
	.thousands_sep = "",
	.grouping = "",
	.int_curr_symbol = "",
	.currency_symbol = "",
	.mon_decimal_point = "",
	.mon_thousands_sep = "",
	.mon_grouping = "",
	.positive_sign = "",
	.negative_sign = "",
	.int_frac_digits = CHAR_MAX,
	.frac_digits = CHAR_MAX,
	.p_cs_precedes = CHAR_MAX,
	.p_sep_by_space = CHAR_MAX,
	.n_cs_precedes = CHAR_MAX,
	.n_sep_by_space = CHAR_MAX,
	.p_sign_posn = CHAR_MAX,
	.n_sign_posn = CHAR_MAX,
	.int_p_cs_precedes = CHAR_MAX,
	.int_p_sep_by_space = CHAR_MAX,
	.int_n_cs_precedes = CHAR_MAX,
	.int_n_sep_by_space = CHAR_MAX,
	.int_p_sign_posn = CHAR_MAX,
	.int_n_sign_posn = CHAR_MAX,
};

struct lconv *
localeconv(void)
{
	return &current_locale;
}

locale_t
newlocale(int mask, const char *locale, locale_t base)
{
	errno = ENOSYS;
	return (locale_t)0;
}

static void
default_init_locale_category(int category)
{
	__global_locale[category] = strdup("POSIX");
}

char *
setlocale(int category, const char *locale)
{
	if (category < 0 || category > LC_ALL) {
		errno = EINVAL;
		return NULL;
	}
	if (locale == NULL) {
		return __global_locale[category] != NULL ? __global_locale[category] :
		                                           "POSIX";
	}
	if (category == LC_ALL) {
		setlocale(LC_COLLATE, locale);
		setlocale(LC_CTYPE, locale);
		setlocale(LC_MESSAGES, locale);
		setlocale(LC_MONETARY, locale);
		setlocale(LC_NUMERIC, locale);
		return setlocale(LC_TIME, locale);
	}
	if (__global_locale[category] != NULL) {
		free(__global_locale[category]);
	}
	// "use the default value"
	if (strcmp(locale, "") == 0) {
		default_init_locale_category(category);
	} else {
		__global_locale[category] = strdup(locale);
	}
	return __global_locale[category];
}

locale_t
uselocale(locale_t base)
{
	errno = ENOSYS;
	return (locale_t)0;
}
