#include <cstring>
#include <unistd.h>
#include <asm-generic/ioctls.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <string>

#include "editor.hpp"

#define BTE_VERSION "0.0.1"

Editor::Editor() {
    int screenrows;
    int screencols;

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

char Editor::readKey() {
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) {
            this->clearScreen();
            perror("read");
            exit(1);
        }
    }
    return c;
};

void Editor::processKeypress() {
    char c = this->readKey();
    switch (c) {
        case CTRL_KEY('q'):
            this->clearScreen();
            exit(0);
            break;
        case 'h':
        case 'j':
        case 'k':
        case 'l':
            this->moveCursor(c);
            break;
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

void Editor::moveCursor(char key) {
    switch (key) {
        case 'h':
            this->cx--;
            break;
        case 'l':
            this->cx++;
            break;
        case 'k':
            this->cy--;
            break;
        case 'j':
            this->cy++;
            break;
    }
}
