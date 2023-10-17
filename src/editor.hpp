#ifndef Editor_H
#define Editor_H

#include <termios.h>
#include <iostream>
#include "buffer.hpp"


typedef struct erow {
    int size;
    char* chars;
} erow;

/* definitions */
#define CTRL_KEY(k) ((k) & 0x1f)


class Editor {

    public:
        int screenrows;
        int screencols;
        int numrows;
        erow row;
        int cx;
        int cy;
        struct termios orig_termios;

        Editor();
        /* input */
        int readKey();
        void processKeypress();

        /* output */
        void clearScreen();
        void drawRows(Buffer *ab);
        void refresh();

        /* utils */
        void die(char* message);
        int getWindowSize();
        int getCursorPosition();
        void moveCursor(int key);

        /* file i/o */
        void open(char *filename);
};

#endif
