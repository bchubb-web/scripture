/* includes */
#include <cstdio>
#include <iostream>
#include <sys/termios.h>
#include <vector>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <termios.h>

#include "src/editor.hpp"
#include "src/buffer.hpp"

Editor editor;

/* functions */

void die(const char *s) {
    editor.clearScreen();
    perror(s);
    exit(1);
}

void disableRawMode() {
    // restore terminal flags
    std::cout << "\r\n";
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &editor.orig_termios) == -1)
        die("tcsetattr");
}

void enableRawMode() {
    if (tcgetattr(STDIN_FILENO, &editor.orig_termios) == -1) die("tcgetattr");
    atexit(disableRawMode);
    struct termios raw = editor.orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}


/* Main */

int main(int argc, char* argv[]) {
    enableRawMode();
    while (1) {
        editor.refresh();
        editor.processKeypress();
    }
    return 0;
}
