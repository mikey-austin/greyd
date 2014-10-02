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

struct T {
    char ip[GREY_MAX_KEY];
    char helo[GREY_MAX_MAIL];
    char from[GREY_MAX_MAIL];
    char to[GREY_MAX_MAIL];
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
