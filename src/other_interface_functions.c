//
// Created by peter on 7/9/20.
//
#include "copyright.h"
#include "config.h"

#include "cgi.h"
#include "defaults.h"
#include "db.h"
#include "interface.h"
#include "params.h"
#include "tune.h"
#include "props.h"
#include "match.h"
#include "reg.h"
#ifdef MCP_SUPPORT
# include "mcp.h"
# include "mcpgui.h"
#endif
#include "mpi.h"
#include "reg.h"
#include "externs.h"
#include "mufevent.h"
#include "interp.h"
#include "newhttp.h"            /* hinoserm */
#include "netresolve.h"         /* hinoserm */
#ifdef UTF8_SUPPORT
# include <locale.h>
#endif
extern struct descriptor_data *descriptor_list;
char *
html_escape2(char *msg, int addbr);

int
queue_html(struct descriptor_data *d, char *msg)
{
    char buf[BUFFER_LEN];

    if ((d->connected
         && OkObj(d->player)) ? (Html(d->player)) : (d->type == CT_PUEBLO)) {
        strncpy(buf, msg, BUFFER_LEN);
        return queue_ansi(d, buf);
    } else {
/*		strcpy(buf, parse_html(msg)); */
        return 0;
    }
}

int
queue_unhtml(struct descriptor_data *d, char *msg)
{
    char buf[BUFFER_LEN];

    if ((d->connected
         && OkObj(d->player)) ? (Html(d->player)) : (d->type == CT_PUEBLO)) {
/*		if(strlen(msg) >= (BUFFER_LEN/6)) { */
        strncpy(buf, html_escape2(msg, 0), BUFFER_LEN);
/*		} else {
			strcpy(buf, html_escape(msg));
			if (d->type == CT_HTTP) {
				strcat(buf, "<BR>");
			}
			strcpy(buf, html_escape2(msg, 0));
		} */
    } else {
        strncpy(buf, msg, BUFFER_LEN);
    }
    return queue_ansi(d, buf);
}


/* Just like notify_descriptor, except no changes are made to the buffer
 * at all. This is for the purpos of sending unaltered binaries.
 * Still experimental at this point. */
void
notify_descriptor_raw(int descr, const char *msg, int length)
{
    struct descriptor_data *d;

    if (!msg || !*msg)
        return;
    for (d = descriptor_list; d && (d->descriptor != descr); d = d->next) ;
    if (!d || d->descriptor != descr)
        return;
    //add_to_queue(&d->output, msg, length);
    add_to_queue(&d->output, msg, length, -2);
    d->output_size += length;
}

/* To go with the descriptor_notify_char prim, this has a singular
 * purpose in an effort to write binary pass through support into
 * a MUF based webserver. It will probably come into play down
 * the line if we write an in-server version of the MUF one in
 * progress.
 */
void
notify_descriptor_char(int descr, char c)
{
    struct descriptor_data *d;
    char cbuf[1];

    for (d = descriptor_list; d && (d->descriptor != descr); d = d->next) ;
    if (!d || d->descriptor != descr)
        return;
    cbuf[0] = c;
    queue_write(d, cbuf, 1);
}

void
anotify_descriptor(int descr, const char *msg)
{
    /* Just like notify descriptor. But leaves ANSI in for
     * connections not connected to a player that have a
     * DR_COLOR flag set.
     */
    char buf3[BUFFER_LEN + 2];

    parse_ansi(NOTHING, buf3, msg, ANSINORMAL);
    notify_descriptor(descr, buf3);

}


int
notify_nolisten(dbref player, const char *msg, int isprivate)
{
    struct descriptor_data *d;
    int retval = 0;
    char buf[BUFFER_LEN + 2];
    char buf2[BUFFER_LEN + 2];
    int firstpass = 1;
    char *ptr1;
    const char *ptr2;
    dbref ref;
    int *darr;
    int di;
    int dcount;

    if (player < 0)
        return retval;          /* no one to notify */
    ptr2 = msg;
    while (ptr2 && *ptr2) {
        ptr1 = buf;
        while ((ptr2) && (*ptr2) && (*ptr2 != '\r') && (*ptr2 != '\r'))
            *(ptr1++) = *(ptr2++);
        *(ptr1++) = '\r';
        *(ptr1++) = '\n';
        *(ptr1++) = '\0';
        while ((*ptr2 == '\r') || (*ptr2 == '\n'))
            ptr2++;

        darr = get_player_descrs(player, &dcount);

        for (di = 0; di < dcount; di++) {
            d = descrdata_by_index(darr[di]);
            if (d->connected && d->player == player) {
                if ((d->linelen > 0) && !(FLAGS(player) & CHOWN_OK)) {
                    if (d)
                        queue_unhtml(d, buf);
                } else if (d)
                    queue_unhtml(d, buf);

                if (firstpass)
                    retval++;
            }
        }
        if (tp_zombies) {
            if ((Typeof(player) == TYPE_THING) && (FLAGS(player) & ZOMBIE) &&
                !(FLAGS(OWNER(player)) & ZOMBIE) && !(FLAGS(player) & QUELL)) {
                ref = getloc(player);
                if (Mage(OWNER(player)) || ref == NOTHING ||
                    Typeof(ref) != TYPE_ROOM || !(FLAGS(ref) & ZOMBIE)) {
                    if (isprivate || getloc(player) != getloc(OWNER(player))) {
                        char pbuf[BUFFER_LEN];
                        const char *prefix;

                        prefix = GETPECHO(player);

                        if (prefix && *prefix) {
                            char ch = *match_args;

                            *match_args = '\0';
                            prefix = do_parse_mesg(-1, player, player, prefix,
                                                   "(@Pecho)", pbuf,
                                                   MPI_ISPRIVATE);
                            *match_args = ch;
                        }
                        if (!prefix || !*prefix) {
                            prefix = NAME(player);
                            sprintf(buf2, "%s> %.*s", prefix,
                                    (int) (BUFFER_LEN - (strlen(prefix) + 3)),
                                    buf);
                        } else {
                            sprintf(buf2, "%s %.*s", prefix,
                                    (int) (BUFFER_LEN - (strlen(prefix) + 2)),
                                    buf);
                        }
                        darr = get_player_descrs(OWNER(player), &dcount);

                        for (di = 0; di < dcount; di++) {
                            d = descrdata_by_index(darr[di]);
                            if (Html(OWNER(player)) && d)
                                queue_ansi(d, html_escape(buf2));
                            else if (d)
                                queue_unhtml(d, buf2);
                            if (firstpass)
                                retval++;
                        }
                    }
                }
            }
        }
        firstpass = 0;
    }
    return retval;
}


int
notify_html_nolisten(dbref player, const char *msg, int isprivate)
{
    struct descriptor_data *d;
    int retval = 0;
    char buf[BUFFER_LEN + 2];
    char buf2[BUFFER_LEN + 2];
    int firstpass = 1;
    char *ptr1;
    const char *ptr2;
    dbref ref;
    int *darr;
    int di;
    int dcount;

    if (player < 0)
        return retval;          /* no one to notify */
    ptr2 = msg;
    while (ptr2 && *ptr2) {
        ptr1 = buf;
        while ((ptr2) && (*ptr2) && (*ptr2 != '\r') && (*ptr2 != '\n'))
            *(ptr1++) = *(ptr2++);
        if ((*ptr2 == '\r') || (*ptr2 == '\n')) {
            while ((*ptr2 == '\r') || (*ptr2 == '\n'))
                ptr2++;
            *(ptr1++) = '\r';
            *(ptr1++) = '\n';
        }
        *(ptr1++) = '\0';

        darr = get_player_descrs(player, &dcount);

        for (di = 0; di < dcount; di++) {
            d = descrdata_by_index(darr[di]);
            if ((d->linelen > 0) && !(FLAGS(player) & CHOWN_OK)) {
                if (d)
                    queue_html(d, buf);
            } else {
                if (d)
                    queue_html(d, buf);
                if (NHtml(d->player) && d)
                    queue_html(d, "<BR>");
            }
            if (firstpass)
                retval++;
        }
        if (tp_zombies) {
            if ((Typeof(player) == TYPE_THING) && (FLAGS(player) & ZOMBIE) &&
                !(FLAGS(OWNER(player)) & ZOMBIE) && !(FLAGS(player) & QUELL)) {
                ref = getloc(player);
                if (Mage(OWNER(player)) || ref == NOTHING ||
                    Typeof(ref) != TYPE_ROOM || !(FLAGS(ref) & ZOMBIE)) {
                    if (isprivate || getloc(player) != getloc(OWNER(player))) {
                        char pbuf[BUFFER_LEN];
                        const char *prefix;

                        prefix = GETPECHO(player);

                        if (prefix && *prefix) {
                            char ch = *match_args;

                            *match_args = '\0';
                            prefix = do_parse_mesg(-1, player, player, prefix,
                                                   "(@Pecho)", pbuf,
                                                   MPI_ISPRIVATE);
                            *match_args = ch;
                        }
                        if (!prefix || !*prefix) {
                            prefix = NAME(player);
                            sprintf(buf2, "%s> %.*s", prefix,
                                    (int) (BUFFER_LEN - (strlen(prefix) + 3)),
                                    buf);
                        } else {
                            sprintf(buf2, "%s %.*s", prefix,
                                    (int) (BUFFER_LEN - (strlen(prefix) + 2)),
                                    buf);
                        }
                        darr = get_player_descrs(OWNER(player), &dcount);

                        for (di = 0; di < dcount; di++) {
                            d = descrdata_by_index(darr[di]);
                            if (d)
                                queue_html(d, buf2);
                            if (firstpass)
                                retval++;
                        }
                    }
                }
            }
        }
        firstpass = 0;
    }
    return retval;
}


int
notify_from_echo(dbref from, dbref player, const char *msg, int isprivate)
{
#ifdef IGNORE_SUPPORT
    if (ignorance(from, player))
        return 0;
#endif
    return notify_listeners(dbref_first_descr(from), from, NOTHING, player,
                            getloc(from), msg, isprivate);
}

int
notify_html_from_echo(dbref from, dbref player, const char *msg, int isprivate)
{
#ifdef IGNORE_SUPPORT
    if (ignorance(from, player))
        return 0;
#endif
    return notify_html_listeners(dbref_first_descr(from), from, NOTHING,
                                 player, getloc(from), msg, isprivate);
}

int
notify_from(dbref from, dbref player, const char *msg)
{
    return notify_from_echo(from, player, msg, 1);
}

int
notify_html_from(dbref from, dbref player, const char *msg)
{
    return notify_html_from_echo(from, player, msg, 1);
}


int
notify(dbref player, const char *msg) {
    return notify_from_echo(player, player, msg, 1);
}

int
notify_html(dbref player, const char *msg)
{
    return notify_html_from_echo(player, player, msg, 1);
}

int
anotify_nolisten(dbref player, const char *msg, int isprivate)
{
    char buf[BUFFER_LEN + 2];

    if (!OkObj(player))
        return 0;

    if ((FLAGS(OWNER(player)) & CHOWN_OK) && !(FLAG2(OWNER(player)) & F2HTML)) {
        parse_ansi(player, buf, msg, ANSINORMAL);
    } else {
        unparse_ansi(buf, msg);
    }
    return notify_nolisten(player, buf, isprivate);
}

int
anotify_nolisten2(dbref player, const char *msg)
{
    return anotify_nolisten(player, msg, 1);
}

int
anotify_from_echo(dbref from, dbref player, const char *msg, int isprivate)
{
#ifdef IGNORE_SUPPORT
    if (ignorance(from, player))
        return 0;
#endif
    return ansi_notify_listeners(dbref_first_descr(from), from, NOTHING,
                                 player, getloc(from), msg, isprivate);
}


int
anotify_from(dbref from, dbref player, const char *msg)
{
    return anotify_from_echo(from, player, msg, 1);
}


int
anotify(dbref player, const char *msg)
{
    return anotify_from_echo(player, player, msg, 1);
}

char *
html_escape(const char *msg)
{
    char *tempstr;
    static char buf[BUFFER_LEN];
    char buff[BUFFER_LEN];

    buff[0] = '\0';
    tempstr = buff;
    while (*msg) {
        switch (*msg) {
            case '&':{
                *tempstr++ = '&';
                *tempstr++ = 'a';
                *tempstr++ = 'm';
                *tempstr++ = 'p';
                *tempstr++ = ';';
                break;
            }
            case '\"':{
                *tempstr++ = '&';
                *tempstr++ = 'q';
                *tempstr++ = 'u';
                *tempstr++ = 'o';
                *tempstr++ = 't';
                *tempstr++ = ';';
                break;
            }
            case '<':{
                *tempstr++ = '&';
                *tempstr++ = 'l';
                *tempstr++ = 't';
                *tempstr++ = ';';
                break;
            }
            case '>':{
                *tempstr++ = '&';
                *tempstr++ = 'g';
                *tempstr++ = 't';
                *tempstr++ = ';';
                break;
            }
/*			case ' ': {
				*tempstr++ = '&';
				*tempstr++ = '#';
				*tempstr++ = '3';
				*tempstr++ = '2';
				*tempstr++ = ';';
				break;
			} */
            case '\n':{
                break;
            }
            default:
                *tempstr++ = *msg;
        }
        (void) msg++;
    }
    *tempstr = '\0';
/*	strcpy(buf, "<TT><PRE>"); */
    strcpy(buf, buff);
/*	strcat(buf, "</TT></PRE>"); */
    return buf;
}


char *html_escape2(char *msg, int addbr) {
    char buf2[BUFFER_LEN];
    static char buff[BUFFER_LEN];

    strcpy(buf2, msg);
    if (strlen(buf2) >= (BUFFER_LEN - 18))
        buf2[BUFFER_LEN - 18] = '\0';
    strcpy(buff, "<CODE>");
    strcat(buff, html_escape(buf2));
    strcat(buff, "</CODE>");
    if (addbr) {
        strcat(buff, "<BR>");
    }
    return buff;
}
char *
grab_html(int nothtml, char *title)
{
    static char buf[BUFFER_LEN];

    buf[0] = '\0';

    if (!title || !*title) {
        return NULL;
    }
    if (!string_compare(title, "BR")) {
        strcpy(buf, "\r\n");
    }
    if (!string_compare(title, "P")) {
        if (nothtml) {
            strcpy(buf, "\r\n");
        } else {
            strcpy(buf, "\r\n      ");
        }
    }
    if (!string_compare(title, "LI")) {
        strcpy(buf, "\r\n* ");
    }
    return buf;
}


int
queue_ansi(struct descriptor_data *d, const char *msg)
{
    char buf[BUFFER_LEN + 5];
    char buf2[BUFFER_LEN + 5];

    if (!msg || !*msg)
        return 0;

    if (d->connected) {
        if (DR_RAW_FLAGS(d, DF_256COLOR) && (FLAGS(d->player) & CHOWN_OK)) {
            strip_bad_ansi(buf, msg);
        } else if (FLAGS(d->player) & CHOWN_OK) {
            strip_256_ansi(buf2, msg);
            strip_bad_ansi(buf, buf2);
        } else {
            strip_ansi(buf, msg);
        }
    } else {
        if (DR_RAW_FLAGS(d, DF_256COLOR) && DR_RAW_FLAGS(d, DF_COLOR)) {
            strip_bad_ansi(buf, msg);
        } else if (DR_RAW_FLAGS(d, DF_COLOR)) {
            strip_256_ansi(buf2, msg);
            strip_bad_ansi(buf, buf2);
        } else {
            strip_ansi(buf, msg);
        }
    }
#ifdef MCP_SUPPORT
    mcp_frame_output_inband(&d->mcpframe, buf);
#else
    queue_string(d, buf);
#endif
    return strlen(buf);
}
void
notify_descriptor(int descr, const char *msg)
{
    char *ptr1;
    const char *ptr2;
    char buf[BUFFER_LEN + 2];
    struct descriptor_data *d;

    if (!msg || !*msg)
        return;
    for (d = descriptor_list; d && (d->descriptor != descr); d = d->next) ;
    if (!d || d->descriptor != descr)
        return;

    ptr2 = msg;
    while (ptr2 && *ptr2) {
        ptr1 = buf;
        while ((ptr2) && (*ptr2) && (*ptr2 != '\r') && (*ptr2 != '\n'))
            *(ptr1++) = *(ptr2++);
        *(ptr1++) = '\r';
        *(ptr1++) = '\n';
        *(ptr1++) = '\0';
        while ((*ptr2 == '\r') || (*ptr2 == '\n'))
            ptr2++;
        queue_ansi(d, buf);
        process_output(d);
    }
}
