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

    int cx = 0;
    int cy = 0;
    //Buffer *buffer = b;
    struct termios orig_termios;

    if (this->getWindowSize() == -1) {
        this->die("getWindowSize");
    }
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
                int times = this->screenrows;
                while (--times) 
                    this->moveCursor(c == PAGEUP ? ARROWUP : ARROWDOWN);

            }
            break;
        case HOME:
            this->cx = 0;
            break;
        case END:
            this->cx = this->screencols-1;
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
    if (this->cy < this->rowoff) {
        this->rowoff = this->cy;
    }
    if (this->cy >= this->rowoff + this->screenrows) {
        this->rowoff = this->cy - this->screenrows + 1;
    }
    if (this->cx < this->coloff) {
        this->coloff = this->cx;
    }
    if (this->cx >= this->coloff + this->screencols) {
        this->coloff = this->cx - this->screencols + 1;
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
                        "basic text editor -- version %s Hello World.", BTE_VERSION);
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
            int len = this->row[filerow].size - this->coloff;
            if (len < 0) len = 0;
            if (len > this->screencols) len = this->screencols;
            buf->append(&this->row[filerow].chars[this->coloff], len);
        }
        buf->append("\x1b[K", 3);
        if (y < this->screenrows - 1) {
            buf->append("\r\n", 2);
            /*} else {
        // my status line
        char cmdline[32];
        snprintf(cmdline, sizeof(cmdline), ": x: %d y: %d", this->cx +1, this->cy +1);
        buf->append(cmdline, strlen(cmdline));*/
    }
    }
}

void Editor::refresh() {

    this->scroll();

    Buffer ab;

    ab.append("\x1b[?25l", 6);
    ab.append("\x1b[H", 3);

    this->drawRows(&ab);

    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (this->cy - this->rowoff) +1, 
            (this->cx - this->coloff) +1);
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
            }
            break;
        case ARROWRIGHT:
            if (row && this->cx < row->size) {
                this->cx++;
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

void Editor::appendRow(char *s, size_t len) {
    this->row = (erow*)realloc(this->row, sizeof(erow) * (this->numrows + 1));
    int at = this->numrows;
    this->row[at].size = len;
    this->row[at].chars = (char*)malloc(len + 1);
    memcpy(this->row[at].chars, s, len);
    this->row[at].chars[len] = '\0';
    this->numrows++;
}
