#ifndef Editor_H
#define Editor_H

#include <termios.h>
#include <iostream>
#include "buffer.hpp"


typedef struct erow {
    int size;
    int rsize;
    char* chars;
    char* render;
} erow;

/* definitions */
#define CTRL_KEY(k) ((k) & 0x1f)


class Editor {

    public:
        int screenrows;
        int screencols;
        int numrows;

        erow *row;
        int rowoff;
        int coloff;

        int cx, cy;
        int rx;
        
        char* filename;
        char statusmsg[80];
        time_t statusmsg_time;
        struct termios orig_termios;

        Editor();
        /* input */
        int readKey();
        void processKeypress();

        /* output */
        void clearScreen();
        void drawRows(Buffer *ab);
        void drawStatusBar(Buffer *buf);
        void setStatusMessage(const char *fmt, ...);
        void refresh();
        void scroll();

        /* utils */
        void die(const char* message);
        int getWindowSize();
        int getCursorPosition();
        void moveCursor(int key);

        /* file i/o */
        void open(char* filename);

        /* row operations */
        int rowCxToRx(erow *row, int cx);
        void updateRow(erow *row);
        void appendRow(char *s, size_t len);
};

#endif
