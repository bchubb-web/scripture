#ifndef Editor_H
#define Editor_H

#include <termios.h>
#include <iostream>
#include "buffer.hpp"


/* definitions */
#define CTRL_KEY(k) ((k) & 0x1f)


class Editor {

    public:
        int screenrows;
        int screencols;
        int cx;
        int cy;
        struct termios orig_termios;

        Editor();
        char readKey();
        void processKeypress();
        void clearScreen();
        void drawRows(Buffer *ab);
        void refresh();
        int getWindowSize();
        int getCursorPosition();
        void moveCursor(char key);
};

#endif
