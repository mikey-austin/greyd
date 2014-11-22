/**
 * @file   con.h
 * @brief  Defines the connection management interface.
 * @author Mikey Austin
 * @date   2014
 */

#ifndef CON_DEFINED
#define CON_DEFINED

#include "grey.h"
#include "list.h"
#include "ip.h"

#define CON_BUF_SIZE        8192
#define CON_DEFAULT_MAX     800
#define CON_REMOTE_END_SIZE 5

/**
 * Main structure encapsulating the state of a single connection.
 */
struct Con {
    int fd;
    int state;
    int last_state;
    short af;

    struct sockaddr_storage ss;
    void *ia;
    char src_addr[INET6_ADDRSTRLEN];
    char dst_addr[INET6_ADDRSTRLEN];
    char helo[GREY_MAX_MAIL], mail[GREY_MAX_MAIL], rcpt[GREY_MAX_MAIL];

    List_T blacklists;  /* Blacklists containing this src address. */
    char *lists;        /* Summary of associated blacklists. */

    /*
     * We will do stuttering by changing these to time_t's of
     * now + n, and only advancing when the time is in the past/now.
     */
    time_t r;
    time_t w;
    time_t s;

    char in_buf[CON_BUF_SIZE];
    char *in_p;
    int in_size;
    char r_end_chars[CON_REMOTE_END_SIZE]; /* Chars causing input termination. */

    char *out_buf;
    size_t out_size;
    char *out_p;

    int ol;
    int data_lines;
    int data_body;
    int stutter;
    int badcmd;
    int seen_cr;
};

/**
 * Structure to contain a list of connections.
 */
typedef struct Con_list_T *Con_list_T;
struct Con_list_T {
    struct Con *connections;
    size_t count;
    size_t size;
};

/**
 * Initialize a connection's internal state.
 */
extern void Con_init(struct Con *con);

/**
 * Cleanup a connection, to be re-initialized later.
 */
extern void Con_close(struct Con *con);

/**
 * Advance a connection's SMTP state machine.
 */
extern void Con_next_state(struct Con *con);

/**
 * Process any connection client input waiting to be
 * read on the connection's file descriptor, then
 * advance the state.
 */
extern void Con_handle_read(struct Con *con);

/**
 * According to the current connection state, write the
 * contents of the connection's output buffer to the
 * configured file descriptor, then advance to the next
 * state.
 */
extern void Con_handle_write(struct Con *con);

/**
 * Attempt to find and set the address the client originally
 * connected to.
 */
extern void Con_get_orig_dst_addr(struct Con *con);

/**
 * Return a suitably sized string summarizing the lists containing,
 * this connection's src address. If there are too many lists,
 * the string will be truncated with a trailing "..." appended.
 */
extern char *Con_summarize_lists(struct Con *con);

/**
 * Construct a reply consisting of each associated blacklist's
 * error string separated by a new line. If there are no associated
 * blacklists, return a generic reply.
 */
extern void Con_build_reply(struct Con *con, char *reply_code);

/**
 * Prepare an error string and append to the supplied connection's
 * output buffer, from the specified offset. Special characters in the
 * error string will be substituted (eg %A, \\, \n, ...).
 *
 * @return Size of appended string.
 * @return -1 on error.
 */
extern int Con_append_error_string(struct Con *con, size_t off, char *fmt, char *reply_code);

/**
 * Increase the size of a connection's output buffer by a fixed
 * amount.
 *
 * @return Offset from the newly allocated buffer.
 * @return NULL on error.
 */
extern char *Con_grow_out_buf(struct Con *con, int off);

#endif
