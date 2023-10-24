#include <cstdarg>
#include <cstdio>
#include <sys/termios.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <termios.h>
#include <cstring>
#include <string>
//#include <asm-generic/ioctls.h>

#include "editor.hpp"
#include "buffer.hpp"

#define BTE_VERSION "0.0.1"
#define TABSTOP 4

enum editorKeys {
    ARROWUP = 1000,
    ARROWDOWN,
    ARROWLEFT,
    ARROWRIGHT,
    HOME,
    END,
    PAGEUP,
    PAGEDOWN,
    DELETE,
};

Editor::Editor() {
    int screenrows;
    int screencols;

    int numrows = 0;

    erow *row = NULL;

    int rowoff = 0;
    int coloff = 0;

    int cx, cy = 0;

    int rx = 0;

    char *filename = NULL;

    char statusmsg[1] = {'\0'};
    time_t statusmsg_time = 0;

    //Buffer *buffer = b;
    struct termios orig_termios;

    if (this->getWindowSize() == -1) {
        this->die("getWindowSize");
    }
    this->screenrows -= 2;
}


/* INPUT */
int Editor::readKey() {
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN)
            this->die((char*)"read");
    }
    // for keys with escape prefix
    if (c == '\x1b') {
        // read more chars into a buffer
        char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';
        // they all start with [
        if (seq[0] == '[') {
            // if next char is a number
            if (seq[1] >= '0' && seq[1] <= '9') {
                //
                if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '1': return HOME;
                        case '3': return DELETE;
                        case '4': return END;
                        case '5': return PAGEUP;
                        case '6': return PAGEDOWN;
                        case '7': return HOME;
                        case '8': return END;
                    }
                }
            } else {
                switch (seq[1]) {
                    case 'A': return ARROWUP;
                    case 'B': return ARROWDOWN;
                    case 'C': return ARROWRIGHT;
                    case 'D': return ARROWLEFT;
                    case 'F': return END;
                    case 'H': return HOME;
                }
            }
        } else if (seq[0] == 'F') {
            switch (seq[1]) {
                case 'F': return END;
                case 'H': return HOME;
            }
        }
        return '\x1b';
    } else {
        return c;
    }
};

void Editor::processKeypress() {
    int c = this->readKey();
    switch (c) {
        case CTRL_KEY('q'):
            this->clearScreen();
            exit(0);
            break;
        case PAGEUP:
        case PAGEDOWN:
            {
                if (c == PAGEUP) {
                    this->cy = this->rowoff;
                } else if (c == PAGEDOWN) {
                     this->cy = this->rowoff + this->screenrows + 1;
                     if (this->cy > this->numrows) this->cy = this->numrows;
                }

                int times = this->screenrows;
                while (--times) 
                    this->moveCursor(c == PAGEUP ? ARROWUP : ARROWDOWN);

            }
            break;
        case HOME:
            this->cx = 0;
            break;
        case END:
            if (this->cy < this->numrows)
                this->cx = this->row[cy].size;
            break;
        case ARROWLEFT:
        case ARROWDOWN:
        case ARROWUP:
        case ARROWRIGHT:
            this->moveCursor(c);
            break;
    }
}


/* ~~~ OUTPUT ~~~ */

void Editor::scroll() {
    this->rx = 0;
    if (this->cy < this->numrows) {
        this->rx = this->rowCxToRx(&this->row[this->cy], this->cx);
    }

    if (this->cy < this->rowoff) {
        this->rowoff = this->cy;
    }
    if (this->cy >= this->rowoff + this->screenrows) {
        this->rowoff = this->cy - this->screenrows + 1;
    }
    if (this->rx < this->coloff) {
        this->coloff = this->rx;
    }
    if (this->rx >= this->coloff + this->screencols) {
        this->coloff = this->rx - this->screencols + 1;
    }
}

void Editor::clearScreen() {
    // send escape code (J) to erase the display
    write(STDOUT_FILENO, "\x1b[2J", 4);
    // send escape code (H) reposition the cursor at 1,1
    write(STDOUT_FILENO, "\x1b[H", 3);
}

void Editor::drawRows(Buffer *buf) {
    int y;
    for (y=0; y<this->screenrows;y++) {
        int filerow = y + this->rowoff;
        if (filerow >= this->numrows) {
            if (this->numrows == 0 && y == this->screenrows / 3) {
                char welcome[80];
                int welcomelen = snprintf(welcome, sizeof(welcome),
                        "basic text editor -- version %s", BTE_VERSION);
                if (welcomelen > this->screencols) welcomelen = this->screencols;
                int padding = (this->screencols - welcomelen) / 2;
                if (padding) {
                    buf->append("~", 1);
                    padding--;
                }
                while (padding--) buf->append(" ", 1);
                buf->append(welcome, welcomelen);
            } else {
                buf->append("~", 1);
            }
        } else {
            int len = this->row[filerow].rsize - this->coloff;
            if (len < 0) len = 0;
            if (len > this->screencols) len = this->screencols;
            buf->append(&this->row[filerow].render[this->coloff], len);
        }
        buf->append("\x1b[K", 3);
        buf->append("\r\n", 2);
    }
}

void Editor::drawStatusBar(Buffer *buf) {
    buf->append("\x1b[7m", 4);
    char status[80], rstatus[80];
    int len = snprintf(status, sizeof(status), " %.20s - %d lines", 
            this->filename ? this->filename : "[no file]", this->numrows);
    int rlen = snprintf(rstatus, sizeof(rstatus), "%d:%d ", 
            this->cy + 1, this->cx +1);
    if (len > this->screencols) len = this->screencols;
    buf->append(status, len);
    while (len < this->screencols) {
        if (this->screencols - len == rlen) {
            buf->append(rstatus, rlen);
            break;
        } else {
            buf->append(" ", 1);
            len++;
        }
    }
    buf->append("\x1b[m", 3);
    buf->append("\r\n", 2);

}

void Editor::drawMessageBar(Buffer *buf) {
    buf->append("\x1b[K", 3);
    int messageLen = strlen(this->statusmsg);
    if (messageLen > this->screencols) messageLen = this->screencols;
    if (messageLen && time(NULL) - this->statusmsg_time < 5)
        buf->append(this->statusmsg, messageLen);
}

void Editor::setStatusMessage(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(this->statusmsg, sizeof(this->statusmsg), fmt, ap);
    va_end(ap);
    this->statusmsg_time = time(NULL);
}

void Editor::refresh() {

    this->scroll();

    Buffer ab;

    ab.append("\x1b[?25l", 6);
    ab.append("\x1b[H", 3);

    this->drawRows(&ab);
    this->drawStatusBar(&ab);
    this->drawMessageBar(&ab);

    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (this->cy - this->rowoff) +1, 
            (this->rx - this->coloff) +1);
    ab.append(buf, strlen(buf));


    ab.append("\x1b[?25h", 6);

    write(STDOUT_FILENO, ab.b, ab.len);
    ab.free();
}


/* ~~~ UTILS ~~~ */

void Editor::die(const char* message) {
    this->clearScreen();
    perror(message);
    exit(1);

}

int Editor::getWindowSize() {
    struct winsize ws;

    // get window size the easy way
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        // move cursor to far bottom right
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
        // store the cursor position as screen size
        return this->getCursorPosition();
    } else {

        // store the cursor position as screen size
        this->screencols = ws.ws_col;
        this->screenrows = ws.ws_row;
        return 0;
    }
}

int Editor::getCursorPosition() {
    char buf[32];
    unsigned int i = 0;

    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

    while (i < sizeof(buf) - 1) {
        if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
        if (buf[i] == 'R') break;
        i++;
    }
    buf[i] = '\0';
    if (buf[0] != '\x1b' || buf[1] != '[') return -1;
    if (sscanf(&buf[2], "%d;%d", &this->screenrows, &this->screencols) != 2) return -1;
    return 0;
}

void Editor::moveCursor(int key) {
    erow *row = (this->cy >= this->numrows) ? NULL : &this->row[this->cy];

    switch (key) {
        case ARROWLEFT:
            if (this->cx != 0) {
                this->cx--;
            } else if (this->cy > 0) {
                this->cy--;
                this->cx = this->row[this->cy].size;
            }
            break;
        case ARROWRIGHT:
            if (row && this->cx < row->size) {
                this->cx++;
            } else if (row && this->cx == row->size) {
                this->cy++;
                this->cx = 0;
            }
            break;
        case ARROWUP:
            if (this->cy != 0) {
                this->cy--;
            }
            break;
        case ARROWDOWN:
            if (this->cy != this->numrows) {
                this->cy++;
            }
            break;
    }
    row = (this->cy >= this->numrows) ? NULL : &this->row[this->cy];
    int rowlen = row ? row->size : 0;
    if (this->cx > rowlen) {
        this->cx = rowlen;
    }
}


/* ~~~ FILE I/O ~~~ */

void Editor::open(char *filename) {
    free(this->filename);
    this->filename = strdup(filename);
    FILE *fp = fopen(filename, "r");
    if (!fp) this->die("fopen");

    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;

    while ((linelen = getline(&line, &linecap, fp)) != -1) {
        while (linelen > 0 && (line[linelen - 1] == '\n' ||
                    line[linelen - 1] == '\r'))
            linelen--;
        this->appendRow(line, linelen);
    }
    free(line);
    fclose(fp);
}


/* ~~~ ROW OPERATIONS ~~~ */

int Editor::rowCxToRx(erow *row, int cx){
    int rx = 0;
    int j;
    for (j=0;j<cx;j++) {
        if (row->chars[j] == '\t') 
            rx += (TABSTOP - 1) - (rx % TABSTOP );
        rx++;
    }
    return rx;
}

void Editor::updateRow(erow *row) {
    int tabs = 0;
    int j;
    for (j = 0; j < row->size; j++)
        if (row->chars[j] == '\t') tabs++;
    free(row->render);
    row->render = (char*)malloc(row->size + tabs*(TABSTOP - 1) + 1);
    int idx = 0;
    for (j = 0; j < row->size; j++) {
        if (row->chars[j] == '\t') {
            row->render[idx++] = ' ';
            while (idx % TABSTOP != 0) row->render[idx++] = ' ';
        } else {
            row->render[idx++] = row->chars[j];
        }
    }
    row->render[idx] = '\0';
    row->rsize = idx;
}

void Editor::appendRow(char *s, size_t len) {
    this->row = (erow*)realloc(this->row, sizeof(erow) * (this->numrows + 1));
    int at = this->numrows;
    this->row[at].size = len;
    this->row[at].chars = (char*)malloc(len + 1);
    memcpy(this->row[at].chars, s, len);
    this->row[at].chars[len] = '\0';

    this->row[at].rsize = 0;
    this->row[at].render = NULL;

    this->updateRow(&this->row[at]);

    this->numrows++;
}
