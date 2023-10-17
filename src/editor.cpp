#include <sys/termios.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <unistd.h>
#include <termios.h>
#include <cstring>
#include <string>
//#include <asm-generic/ioctls.h>

#include "editor.hpp"

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

    erow row;

    int cx = 0;
    int cy = 0;
    //Buffer *buffer = b;
    struct termios orig_termios;

    if (this->getWindowSize() == -1) {
        this->clearScreen();
        perror("getWindowSize");
        exit(1);
    }
}


/* INPUT */
int Editor::readKey() {
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) {
            die("read");
            this->clearScreen();
            perror("read");
            exit(1);
        }
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

void Editor::clearScreen() {
    // send escape code (J) to erase the display
    write(STDOUT_FILENO, "\x1b[2J", 4);
    // send escape code (H) reposition the cursor at 1,1
    write(STDOUT_FILENO, "\x1b[H", 3);
}

void Editor::drawRows(Buffer *buf) {
    int y;
    for (y=0; y<this->screenrows;y++) {
        if (y >= this->numrows) {

            if (y == this->screenrows / 3) {
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
            } else if (y != this->screenrows-1){
                buf->append("~", 1);
            }
        } else {
            int len = this->row.size;
            if (len > this->screencols) len = this->screencols;
            buf->append(this->row.chars, len);
        }
        buf->append("\x1b[K", 3);
        if (y < this->screenrows - 1) {
            buf->append("\r\n", 2);
        } else {

            char cmdline[32];
            snprintf(cmdline, sizeof(cmdline), ": x: %d y: %d", this->cx +1, this->cy +1);
            buf->append(cmdline, strlen(cmdline));
        }
    }
}

void Editor::refresh() {

    Buffer ab;

    ab.append("\x1b[?25l", 6);
    ab.append("\x1b[H", 3);

    this->drawRows(&ab);

    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", this->cy +1, this->cx +1);
    ab.append(buf, strlen(buf));


    ab.append("\x1b[?25h", 6);

    write(STDOUT_FILENO, ab.b, ab.len);
    ab.free();
}


/* ~~~ UTILS ~~~ */

void Editor::die(std::string message) {
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
    switch (key) {
        case ARROWLEFT:
            if (this->cx != 0) {
                this->cx--;
            }
            break;
        case ARROWRIGHT:
            if (this->cx != this->screencols -1) {
                this->cx++;
            }
            break;
        case ARROWUP:
            if (this->cy != 0) {
                this->cy--;
            }
            break;
        case ARROWDOWN:
            if (this->cy != this->screenrows -2) {
                this->cy++;
            }
            break;
    }
}


/* ~~~ FILE I/O ~~~ */

void Editor::open(char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) die()
    const char * line = "Hello world.";
    ssize_t linelen = 12;

    this->row.size = linelen;
    this->row.chars = new char[linelen];
    memcpy(this->row.chars, line, linelen);
    this->row.chars[linelen] = '\0';
    this->numrows = 1;
}
