/**
 * @file   grey.h
 * @brief  Defines the greylisting management interface.
 * @author Mikey Austin
 * @date   2014
 */

#ifndef GREY_DEFINED
#define GREY_DEFINED

#define T Grey_tuple
#define D Grey_data

#define GREY_MAX_MAIL 1024
#define GREY_MAX_KEY  45
#define GREY_PASSTIME (60 * 25) /* pass after first retry seen after 25 mins */
#define GREY_GREYEXP  (60 * 60 * 4) /* remove grey entries after 4 hours */
#define GREY_WHITEEXP (60 * 60 * 24 * 36) /* remove white entries after 36 days */
#define GREY_TRAPEXP  (60 * 60 * 24) /* hitting a spamtrap blacklists for a day */

struct T {
    char ip*;
    char helo*;
    char from*;
    char to*;
};

struct D {
	int64_t first;  /**< When did we see it first. */
	int64_t pass;   /**< When was it whitelisted. */
	int64_t expire; /**< When will we get rid of this entry. */
	int bcount;     /**< How many times have we blocked it. */
	int pcount;     /**< How many times passed, or -1 for spamtrap. */
};

#undef T
#undef D
#endif
