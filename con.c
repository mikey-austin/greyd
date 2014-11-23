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
#include "utils.h"

#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

extern void
Con_init(struct Con *con, int fd, struct sockaddr *sa, Con_list_T cons,
         List_T blacklists, Config_T config)
{
    time_t now;
    socklen_t sock_len = sizeof *sa;
    short greylist, grey_stutter;
    int ret, max_black;
    char *human_time;
    struct List_entry *entry;
    Blacklist_T blacklist = NULL;
    struct IP_addr ipaddr;

    time(&now);

    /* Free resources before zeroing out this entry. */
    free(con->out_buf);
    con->out_buf = NULL;
    con->out_size = 0;
    List_destroy(&(con->blacklists));
    free(con->lists);
    con->lists = NULL;
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
    con->ia = &((struct sockaddr_in *) &con->ss)->sin_addr;

    greylist = Config_get_int(config, "enable", "grey", 1);
    grey_stutter = Config_get_int(config, "stutter", "grey", CON_GREY_STUTTER);
    con->stutter = (greylist && !grey_stutter && List_size(con->blacklists) == 0)
        ? 0 : Config_get_int(config, "stutter", NULL, CON_STUTTER);

    ret = getnameinfo(sa, sock_len, con->src_addr, sizeof(con->src_addr), NULL, 0,
                      NI_NUMERICHOST);
    if(ret != 0)
        I_CRIT("getnameinfo: %s", gai_strerror(ret));

    if((human_time = strdup(ctime(&now))) == NULL)
        I_CRIT("strdup failed");

    /* Replace the newline with a \0. */
    human_time[strlen(human_time) - 1] = '\0';
    snprintf(con->out_buf, con->out_size, "220 %s ESMTP %s; %s\r\n",
             Config_get_str(config, "hostname", NULL, NULL),
             Config_get_str(config, "banner", NULL, NULL),
             human_time);
    free(human_time);

    con->out_p = con->out_buf;
    con->ol = strlen(con->out_p);

    /* Set the stuttering time values. */
    con->w = now + con->stutter;
    con->s = now;
    sstrncpy(con->r_end_chars, "\n", CON_REMOTE_END_SIZE);

    /* Lookup any blacklists based on this client's src IP address. */
    LIST_FOREACH(blacklists, entry) {
        blacklist = List_entry_value(entry);
        IP_sockaddr_to_addr(&con->ss, &ipaddr);

        if(Blacklist_match(blacklist, &ipaddr, con->af)) {
            List_insert_after(con->blacklists, blacklist);
        }
    }

    cons->clients++;
    if(List_size(con->blacklists) > 0) {
        cons->black_clients++;
        con->lists = Con_summarize_lists(con);

        max_black = Config_get_int(config, "max_black", NULL, CON_DEFAULT_MAX);
        if(greylist && (cons->black_clients > max_black))
            con->stutter = 0;
    }
    else {
        con->lists = NULL;
    }
}

extern void
Con_close(struct Con *con, Con_list_T cons, int *slow_until)
{
    time_t now;

    close(con->fd);
    con->fd = -1;
    *slow_until = 0;

    time(&now);
    I_INFO("%s: disconnected after %lld seconds.%s%s",
           con->src_addr, (long long) (now - con->s),
           (List_size(con->blacklists) > 0 ? " lists:" : ""),
           (List_size(con->blacklists) > 0 ? con->lists : ""));

    if(con->lists != NULL) {
        free(con->lists);
        con->lists = NULL;
    }

    if(List_size(con->blacklists) > 0) {
        List_remove_all(con->blacklists);
        cons->black_clients--;
    }

    if(con->out_buf != NULL) {
        free(con->out_buf);
        con->out_buf = NULL;
        con->out_size = 0;
    }

    cons->clients--;
}

extern char
*Con_summarize_lists(struct Con *con)
{
    char *lists;
    int out_size;
    struct List_entry *entry;
    Blacklist_T blacklist;

    if(List_size(con->blacklists) == 0)
        return NULL;

    if((lists = malloc(CON_BL_SUMMARY_SIZE + 1)) == NULL)
        I_CRIT("could not malloc list summary");
    *lists = '\0';

    out_size = CON_BL_SUMMARY_SIZE - strlen(CON_BL_SUMMARY_ETC);
    LIST_FOREACH(con->blacklists, entry) {
        blacklist = List_entry_value(entry);

        if(strlen(lists) + strlen(blacklist->name) + 1 >= out_size) {
            strcat(lists, CON_BL_SUMMARY_ETC);
            break;
        }
        else {
            strcat(lists, " ");
            strcat(lists, blacklist->name);
        }
    }

    return lists;
}

extern char
*Con_grow_out_buf(struct Con *con, int off)
{
    char *out_buf;

    out_buf = realloc(con->out_buf, con->out_size + CON_OUT_BUF_SIZE);
    if(out_buf == NULL) {
        free(con->out_buf);
        con->out_buf = NULL;
        con->out_size = 0;
        return NULL;
    }
    else {
        con->out_size += CON_OUT_BUF_SIZE;
        con->out_buf = out_buf;
        return con->out_buf + off;
    }
}
