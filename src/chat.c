/*
 * Toxic -- Tox Curses Client
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "toxic_windows.h"
#include "execute.h"
#include "misc_tools.h"
#include "friendlist.h"
#include "toxic_strings.h"

extern char *DATA_FILE;
extern int store_data(Tox *m, char *path);

extern FileSender file_senders[MAX_FILES];
extern ToxicFriend friends[MAX_FRIENDS_NUM];

#define AC_NUM_CHAT_COMMANDS 17

/* Array of chat command names used for tab completion. */
static const uint8_t chat_cmd_list[AC_NUM_CHAT_COMMANDS][MAX_CMDNAME_SIZE] = {
    { "/accept"     },
    { "/add"        },
    { "/clear"      },
    { "/close"      },
    { "/connect"    },
    { "/exit"       },
    { "/groupchat"  },
    { "/help"       },
    { "/invite"     },
    { "/join"       },
    { "/myid"       },
    { "/nick"       },
    { "/note"       },
    { "/quit"       },
    { "/savefile"   },
    { "/sendfile"   },
    { "/status"     },
};

static void chat_onMessage(ToxWindow *self, Tox *m, int num, uint8_t *msg, uint16_t len)
{
    if (self->num != num)
        return;

    ChatContext *ctx = self->chatwin;

    uint8_t nick[TOX_MAX_NAME_LENGTH] = {'\0'};
    tox_get_name(m, num, nick);
    nick[TOXIC_MAX_NAME_LENGTH] = '\0';

    print_time(ctx->history);
    wattron(ctx->history, COLOR_PAIR(CYAN));
    wprintw(ctx->history, "%s: ", nick);
    wattroff(ctx->history, COLOR_PAIR(CYAN));

    if (msg[0] == '>') {
        wattron(ctx->history, COLOR_PAIR(GREEN));
        wprintw(ctx->history, "%s\n", msg);
        wattroff(ctx->history, COLOR_PAIR(GREEN));
    } else
        wprintw(ctx->history, "%s\n", msg);

    alert_window(self, WINDOW_ALERT_1, true);
}

static void chat_onConnectionChange(ToxWindow *self, Tox *m, int num, uint8_t status)
{
    if (self->num != num)
        return;

    StatusBar *statusbar = self->stb;
    statusbar->is_online = status == 1 ? true : false;
}

static void chat_onAction(ToxWindow *self, Tox *m, int num, uint8_t *action, uint16_t len)
{
    if (self->num != num)
        return;

    ChatContext *ctx = self->chatwin;

    uint8_t nick[TOX_MAX_NAME_LENGTH] = {'\0'};
    tox_get_name(m, num, nick);
    nick[TOXIC_MAX_NAME_LENGTH] = '\0';

    print_time(ctx->history);
    wattron(ctx->history, COLOR_PAIR(YELLOW));
    wprintw(ctx->history, "* %s %s\n", nick, action);
    wattroff(ctx->history, COLOR_PAIR(YELLOW));

    alert_window(self, WINDOW_ALERT_1, true);
}

static void chat_onNickChange(ToxWindow *self, Tox *m, int num, uint8_t *nick, uint16_t len)
{
    if (self->num != num)
        return;

    nick[TOXIC_MAX_NAME_LENGTH] = '\0';
    len = strlen(nick) + 1;
    memcpy(self->name, nick, len);
}

static void chat_onStatusChange(ToxWindow *self, Tox *m, int num, TOX_USERSTATUS status)
{
    if (self->num != num)
        return;

    StatusBar *statusbar = self->stb;
    statusbar->status = status;
}

static void chat_onStatusMessageChange(ToxWindow *self, int num, uint8_t *status, uint16_t len)
{
    if (self->num != num)
        return;

    StatusBar *statusbar = self->stb;
    statusbar->statusmsg_len = len;
    memcpy(statusbar->statusmsg, status, len);
}

static void chat_onFileSendRequest(ToxWindow *self, Tox *m, int num, uint8_t filenum, 
                                   uint64_t filesize, uint8_t *pathname, uint16_t path_len)
{
    if (self->num != num)
        return;

    ChatContext *ctx = self->chatwin;

    uint8_t filename[MAX_STR_SIZE];
    get_file_name(pathname, filename);

    wprintw(ctx->history, "File transfer request for '%s' (%llu bytes).\n", filename, 
            (long long unsigned int)filesize);

    if (filenum >= MAX_FILES) {
        wprintw(ctx->history, "Too many pending file requests; discarding.\n");
        return;
    }

    /* Append a number to duplicate file names */
    FILE *filecheck = NULL;
    int count = 1;
    int len = strlen(filename);

    while ((filecheck = fopen(filename, "r"))) {
        filename[len] = '\0';
        char d[9];
        sprintf(d,"(%d)", count++);
        strcat(filename, d);
        filename[len + strlen(d)] = '\0';

        if (count > 999999) {
            wprintw(ctx->history, "Error saving file to disk.\n");
            return;
        }
    }

    wprintw(ctx->history, "Type '/savefile %d' to accept the file transfer.\n", filenum);

    friends[num].file_receiver.pending[filenum] = true;
    strcpy(friends[num].file_receiver.filenames[filenum], filename);

    alert_window(self, WINDOW_ALERT_2, true);
}

static void chat_onFileControl(ToxWindow *self, Tox *m, int num, uint8_t receive_send, 
                               uint8_t filenum, uint8_t control_type, uint8_t *data, uint16_t length)
{
    if (self->num != num)
        return;

    ChatContext *ctx = self->chatwin;
    uint8_t *filename;

    if (receive_send == 0)
        filename = friends[num].file_receiver.filenames[filenum];
    else
        filename = file_senders[filenum].pathname;

    switch (control_type) {
    case TOX_FILECONTROL_ACCEPT:
        wprintw(ctx->history, "File transfer for '%s' accepted.\n", filename);
        break;
    case TOX_FILECONTROL_PAUSE:
        // wprintw(ctx->history, "File transfer for '%s' paused.\n", filename);
        break;
    case TOX_FILECONTROL_KILL:
        wprintw(ctx->history, "File transfer for '%s' failed.\n", filename);

        if (receive_send == 0)
            friends[num].file_receiver.pending[filenum] = false;
        else
            close_file_sender(filenum);

        break;
    case TOX_FILECONTROL_FINISHED:
        wprintw(ctx->history, "File transfer for '%s' complete.\n", filename);
        break;
    }

    alert_window(self, WINDOW_ALERT_2, true);
}

static void chat_onFileData(ToxWindow *self, Tox *m, int num, uint8_t filenum, uint8_t *data,
                            uint16_t length)
{
    if (self->num != num)
        return;

    ChatContext *ctx = self->chatwin;

    uint8_t *filename = friends[num].file_receiver.filenames[filenum];
    FILE *file_to_save = fopen(filename, "a");

     // we have a problem here, but don't let it segfault
    if (file_to_save == NULL) {
        wattron(ctx->history, COLOR_PAIR(RED));
        wprintw(ctx->history, "* Error writing to file.\n");
        wattroff(ctx->history, COLOR_PAIR(RED));
        tox_file_send_control(m, num, 1, filenum, TOX_FILECONTROL_KILL, 0, 0);
        return;
    }

    if (fwrite(data, length, 1, file_to_save) != 1) {
        wattron(ctx->history, COLOR_PAIR(RED));
        wprintw(ctx->history, "* Error writing to file.\n");
        wattroff(ctx->history, COLOR_PAIR(RED));
        tox_file_send_control(m, num, 1, filenum, TOX_FILECONTROL_KILL, 0, 0);
    }

    fclose(file_to_save);
}

static void chat_onGroupInvite(ToxWindow *self, Tox *m, int friendnumber, uint8_t *group_pub_key)
{
    if (self->num != friendnumber)
        return;

    ChatContext *ctx = self->chatwin;
    uint8_t name[TOX_MAX_NAME_LENGTH] = {'\0'};

    if (tox_get_name(m, friendnumber, name) == -1)
        return;

    wprintw(ctx->history, "%s has invited you to a group chat.\n", name);

    memcpy(friends[friendnumber].pending_groupchat, group_pub_key, TOX_CLIENT_ID_SIZE);
    wprintw(ctx->history, "Type \"/join\" to join the chat.\n");
    alert_window(self, WINDOW_ALERT_2, true);
}

static void send_action(ToxWindow *self, ChatContext *ctx, Tox *m, uint8_t *action) {
    if (action == NULL) {
        wprintw(ctx->history, "Invalid syntax.\n");
        return;
    }

    uint8_t selfname[TOX_MAX_NAME_LENGTH];
    tox_get_self_name(m, selfname, TOX_MAX_NAME_LENGTH);

    print_time(ctx->history);
    wattron(ctx->history, COLOR_PAIR(YELLOW));
    wprintw(ctx->history, "* %s %s\n", selfname, action);
    wattroff(ctx->history, COLOR_PAIR(YELLOW));

    if (tox_send_action(m, self->num, action, strlen(action) + 1) == 0) {
        wattron(ctx->history, COLOR_PAIR(RED));
        wprintw(ctx->history, " * Failed to send action\n");
        wattroff(ctx->history, COLOR_PAIR(RED));
    }
}

static void chat_onKey(ToxWindow *self, Tox *m, wint_t key)
{
    ChatContext *ctx = self->chatwin;
    StatusBar *statusbar = self->stb;

    int x, y, y2, x2;
    getyx(self->window, y, x);
    getmaxyx(self->window, y2, x2);
    int cur_len = 0;

    if (key == 0x107 || key == 0x8 || key == 0x7f) {  /* BACKSPACE key: Remove character behind pos */
        if (ctx->pos > 0) {
            cur_len = MAX(1, wcwidth(ctx->line[ctx->pos - 1]));
            del_char_buf_bck(ctx->line, &ctx->pos, &ctx->len);

            if (x == 0)
                wmove(self->window, y-1, x2 - cur_len);
            else
                wmove(self->window, y, x - cur_len);
        } else {
            beep();
        }
    }

    else if (key == KEY_DC) {      /* DEL key: Remove character at pos */
        if (ctx->pos != ctx->len)
            del_char_buf_frnt(ctx->line, &ctx->pos, &ctx->len);
        else
            beep();
    }

    else if (key == T_KEY_DISCARD) {    /* CTRL-U: Delete entire line behind pos */
        if (ctx->pos > 0) {
            discard_buf(ctx->line, &ctx->pos, &ctx->len);
            wmove(self->window, y2 - CURS_Y_OFFSET, 0);
        } else {
            beep();
        }
    }

    else if (key == T_KEY_KILL) {    /* CTRL-K: Delete entire line in front of pos */
        if (ctx->pos != ctx->len)
            kill_buf(ctx->line, &ctx->pos, &ctx->len);
        else
            beep();
    }

    else if (key == KEY_HOME) {    /* HOME key: Move cursor to beginning of line */
        if (ctx->pos > 0) {
            ctx->pos = 0;
            wmove(self->window, y2 - CURS_Y_OFFSET, 0);
        }
    } 

    else if (key == KEY_END) {     /* END key: move cursor to end of line */
        if (ctx->pos != ctx->len) {
            ctx->pos = ctx->len;
            mv_curs_end(self->window, MAX(0, wcswidth(ctx->line, (CHATBOX_HEIGHT-1)*x2)), y2, x2);
        }
    }

    else if (key == KEY_LEFT) {
        if (ctx->pos > 0) {
            --ctx->pos;
            cur_len = MAX(1, wcwidth(ctx->line[ctx->pos]));

            if (x == 0)
                wmove(self->window, y-1, x2 - cur_len);
            else
                wmove(self->window, y, x - cur_len);
        } else {
            beep();
        }
    } 

    else if (key == KEY_RIGHT) {
        if (ctx->pos < ctx->len) {
            cur_len = MAX(1, wcwidth(ctx->line[ctx->pos]));
            ++ctx->pos;

            if (x == x2-1)
                wmove(self->window, y+1, 0);
            else
                wmove(self->window, y, x + cur_len);
        } else {
            beep();
        }
    } 

    else if (key == KEY_UP) {    /* fetches previous item in history */
        fetch_hist_item(ctx->line, &ctx->pos, &ctx->len, ctx->ln_history, ctx->hst_tot,
                        &ctx->hst_pos, LN_HIST_MV_UP);
        mv_curs_end(self->window, ctx->len, y2, x2);
    }

    else if (key == KEY_DOWN) {    /* fetches next item in history */
        fetch_hist_item(ctx->line, &ctx->pos, &ctx->len, ctx->ln_history, ctx->hst_tot,
                        &ctx->hst_pos, LN_HIST_MV_DWN);
        mv_curs_end(self->window, ctx->len, y2, x2);
    }

    else if (key == '\t') {    /* TAB key: completes command */
        if (ctx->len > 1 && ctx->line[0] == '/') {
            int diff = complete_line(ctx->line, &ctx->pos, &ctx->len, chat_cmd_list, AC_NUM_CHAT_COMMANDS,
                                     MAX_CMDNAME_SIZE);

            if (diff != -1) {
                if (x + diff > x2 - 1) {
                    int ofst = (x + diff - 1) - (x2 - 1);
                    wmove(self->window, y+1, ofst);
                } else {
                    wmove(self->window, y, x+diff);
                }
            } else {
                beep();
            }
        } else {
            beep();
        }
    }

    else
#if HAVE_WIDECHAR
    if (iswprint(key))
#else
    if (isprint(key))
#endif
    {   /* prevents buffer overflows and strange behaviour when cursor goes past the window */
        if ( (ctx->len < MAX_STR_SIZE-1) && (ctx->len < (x2 * (CHATBOX_HEIGHT - 1)-1)) ) {
            add_char_to_buf(ctx->line, &ctx->pos, &ctx->len, key);

            if (x == x2-1)
                wmove(self->window, y+1, 0);
            else
                wmove(self->window, y, x + MAX(1, wcwidth(key)));
        }
    }
    /* RETURN key: Execute command or print line */
    else if (key == '\n') {
        uint8_t line[MAX_STR_SIZE];

        if (wcs_to_mbs_buf(line, ctx->line, MAX_STR_SIZE) == -1)
            memset(&line, 0, sizeof(line));

        wclear(ctx->linewin);
        wmove(self->window, y2 - CURS_Y_OFFSET, 0);
        wclrtobot(self->window);
        bool close_win = false;

        if (!string_is_empty(line))
            add_line_to_hist(ctx->line, ctx->len, ctx->ln_history, &ctx->hst_tot, &ctx->hst_pos);

        if (line[0] == '/') {
            if (close_win = !strcmp(line, "/close")) {
                int f_num = self->num;
                delwin(ctx->linewin);
                delwin(statusbar->topline);
                del_window(self);
                disable_chatwin(f_num);
            } else if (strncmp(line, "/me ", strlen("/me ")) == 0)
                send_action(self, ctx, m, line + strlen("/me "));
              else
                execute(ctx->history, self, m, line, CHAT_COMMAND_MODE);
        } else if (!string_is_empty(line)) {
            uint8_t selfname[TOX_MAX_NAME_LENGTH];
            tox_get_self_name(m, selfname, TOX_MAX_NAME_LENGTH);

            print_time(ctx->history);
            wattron(ctx->history, COLOR_PAIR(GREEN));
            wprintw(ctx->history, "%s: ", selfname);
            wattroff(ctx->history, COLOR_PAIR(GREEN));

            if (line[0] == '>') {
                wattron(ctx->history, COLOR_PAIR(GREEN));
                wprintw(ctx->history, "%s\n", line);
                wattroff(ctx->history, COLOR_PAIR(GREEN));
            } else
                wprintw(ctx->history, "%s\n", line);

            if (!statusbar->is_online || tox_send_message(m, self->num, line, strlen(line) + 1) == 0) {
                wattron(ctx->history, COLOR_PAIR(RED));
                wprintw(ctx->history, " * Failed to send message.\n");
                wattroff(ctx->history, COLOR_PAIR(RED));
            }
        }

        if (close_win) {
            free(ctx);
            free(statusbar);
        } else {
            reset_buf(ctx->line, &ctx->pos, &ctx->len);
        }
    }
}

static void chat_onDraw(ToxWindow *self, Tox *m)
{
    curs_set(1);
    int x2, y2;
    getmaxyx(self->window, y2, x2);

    ChatContext *ctx = self->chatwin;

    wclear(ctx->linewin);

    if (ctx->len > 0) {
        uint8_t line[MAX_STR_SIZE];
        
        if (wcs_to_mbs_buf(line, ctx->line, MAX_STR_SIZE) == -1) {
            reset_buf(ctx->line, &ctx->pos, &ctx->len);
            wmove(self->window, y2 - CURS_Y_OFFSET, 0);
        } else {
            mvwprintw(ctx->linewin, 1, 0, "%s", line);
        }
    }

    /* Draw status bar */
    StatusBar *statusbar = self->stb;
    mvwhline(statusbar->topline, 1, 0, ACS_HLINE, x2);
    wmove(statusbar->topline, 0, 0);

    /* Draw name, status and note in statusbar */
    if (statusbar->is_online) {
        char *status_text = "Unknown";
        int colour = WHITE;

        TOX_USERSTATUS status = statusbar->status;

        switch (status) {
        case TOX_USERSTATUS_NONE:
            status_text = "Online";
            colour = GREEN;
            break;
        case TOX_USERSTATUS_AWAY:
            status_text = "Away";
            colour = YELLOW;
            break;
        case TOX_USERSTATUS_BUSY:
            status_text = "Busy";
            colour = RED;
            break;
        }

        wattron(statusbar->topline, A_BOLD);
        wprintw(statusbar->topline, " %s ", self->name);
        wattroff(statusbar->topline, A_BOLD);
        wattron(statusbar->topline, COLOR_PAIR(colour) | A_BOLD);
        wprintw(statusbar->topline, "[%s]", status_text);
        wattroff(statusbar->topline, COLOR_PAIR(colour) | A_BOLD);
    } else {
        wattron(statusbar->topline, A_BOLD);
        wprintw(statusbar->topline, " %s ", self->name);
        wattroff(statusbar->topline, A_BOLD);
        wprintw(statusbar->topline, "[Offline]");
    }

    /* Reset statusbar->statusmsg on window resize */
    if (x2 != self->x) {
        uint8_t statusmsg[TOX_MAX_STATUSMESSAGE_LENGTH] = {'\0'};
        tox_get_status_message(m, self->num, statusmsg, TOX_MAX_STATUSMESSAGE_LENGTH);
        snprintf(statusbar->statusmsg, sizeof(statusbar->statusmsg), "%s", statusmsg);
        statusbar->statusmsg_len = tox_get_status_message_size(m, self->num);
    }

    self->x = x2;

    /* Truncate note if it doesn't fit in statusbar */
    uint16_t maxlen = x2 - getcurx(statusbar->topline) - 4;
    if (statusbar->statusmsg_len > maxlen) {
        statusbar->statusmsg[maxlen] = '\0';
        statusbar->statusmsg_len = maxlen;
    }

    if (statusbar->statusmsg[0]) {
        wattron(statusbar->topline, A_BOLD);
        wprintw(statusbar->topline, " - %s ", statusbar->statusmsg);
        wattroff(statusbar->topline, A_BOLD);
    }

    wprintw(statusbar->topline, "\n");
    mvwhline(ctx->linewin, 0, 0, ACS_HLINE, x2);
}

static void chat_onInit(ToxWindow *self, Tox *m)
{
    int x2, y2;
    getmaxyx(self->window, y2, x2);
    self->x = x2;

    /* Init statusbar info */
    StatusBar *statusbar = self->stb;
    statusbar->status = tox_get_user_status(m, self->num);
    statusbar->is_online = tox_get_friend_connection_status(m, self->num) == 1;

    uint8_t statusmsg[TOX_MAX_STATUSMESSAGE_LENGTH] = {'\0'};
    tox_get_status_message(m, self->num, statusmsg, TOX_MAX_STATUSMESSAGE_LENGTH);
    snprintf(statusbar->statusmsg, sizeof(statusbar->statusmsg), "%s", statusmsg);
    statusbar->statusmsg_len = tox_get_status_message_size(m, self->num);

    /* Init subwindows */
    ChatContext *ctx = self->chatwin;
    statusbar->topline = subwin(self->window, 2, x2, 0, 0);
    ctx->history = subwin(self->window, y2-CHATBOX_HEIGHT+1, x2, 0, 0);
    scrollok(ctx->history, 1);
    ctx->linewin = subwin(self->window, CHATBOX_HEIGHT, x2, y2-CHATBOX_HEIGHT, 0);
    wprintw(ctx->history, "\n\n");
    execute(ctx->history, self, m, "/help", CHAT_COMMAND_MODE);
    wmove(self->window, y2 - CURS_Y_OFFSET, 0);
}

ToxWindow new_chat(Tox *m, int friendnum)
{
    ToxWindow ret;
    memset(&ret, 0, sizeof(ret));

    ret.active = true;

    ret.onKey = &chat_onKey;
    ret.onDraw = &chat_onDraw;
    ret.onInit = &chat_onInit;
    ret.onMessage = &chat_onMessage;
    ret.onConnectionChange = &chat_onConnectionChange;
    ret.onGroupInvite = &chat_onGroupInvite;
    ret.onNickChange = &chat_onNickChange;
    ret.onStatusChange = &chat_onStatusChange;
    ret.onStatusMessageChange = &chat_onStatusMessageChange;
    ret.onAction = &chat_onAction;
    ret.onFileSendRequest = &chat_onFileSendRequest;
    ret.onFileControl = &chat_onFileControl;
    ret.onFileData = &chat_onFileData;

    uint8_t name[TOX_MAX_NAME_LENGTH] = {'\0'};
    uint16_t len = tox_get_name(m, friendnum, name);
    memcpy(ret.name, name, len);
    ret.name[TOXIC_MAX_NAME_LENGTH] = '\0';

    ChatContext *chatwin = calloc(1, sizeof(ChatContext));
    StatusBar *stb = calloc(1, sizeof(StatusBar));

    if (stb != NULL && chatwin != NULL) {
        ret.chatwin = chatwin;
        ret.stb = stb;
    } else {
        endwin();
        fprintf(stderr, "calloc() failed. Aborting...\n");
        exit(EXIT_FAILURE);
    }

    ret.num = friendnum;

    return ret;
}
