/**
 * @file   con.c
 * @brief  Implements the connection management interface.
 * @author Mikey Austin
 * @date   2014
 */

#include "failures.h"
#include "con.h"
#include "blacklist.h"
#include "list.h"

#include <time.h>
#include <stdlib.h>
#include <string.h>

extern void
Con_init(struct Con *con, int fd, struct sockaddr *sa, Blacklist_T blacklists)
{
    time_t now;
    socklen_t sock_len = sizeof *sa;

    time(&now);

    /* Free resources before zeroing out this entry. */
    free(con->out_buf);
    con->out_buf = NULL;
    con->out_size = 0;
    List_destroy(&(con->blacklists));
    memset(con, 0, sizeof *con);

    /* Start initializing the connection. */
    if(Con_grow_out_buf(con, 0) == NULL)
        I_CRIT("could not grow connection out buf");

    con->blacklists = List_create(NULL);
    con->fd = fd;

    if(sock_len > sizeof con->ss)
        I_CRIT("sockaddr size mismatch");
    memcpy(&(con->ss), sa, sock_len);
    con->af = sa->sa_family;

    // TODO: lookup blacklists here.

}

extern char
*Con_grow_out_buf(struct Con *con, int off)
{
    return NULL;
}
