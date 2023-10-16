#include <cstdio>
#include <iostream>
#include <sys/termios.h>
#include <vector>
#include <unistd.h>
#include <ctype.h>
#include <termios.h>

// store terminal flags?
struct termios orig_termios;


void die(const char *s) {
  perror(s);
  exit(1);
}

bool handleCommands(std::vector<char> commands) {
    switch (commands[0]) {
        case 'q':
            return false;
        default:
            std::cout << "command "<< commands[0] << " not found\r\n";
    }
    return true;
}


void disableRawMode() {
    // restore terminal flags
    std::cout << "\r\n";
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1)
        die("tcsetattr");
}

void enableRawMode() {
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) die("tcgetattr");
    atexit(disableRawMode);
    struct termios raw = orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

int main(int argc, char* argv[]) {
    enableRawMode();
    bool commandMode = false;
    std::vector<char> commands;
    while (1) {

        char c = '\0';
        if (read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN) die("read");

        if (c == '.') break;

        if (commandMode && (int)c==13 && c!='\0') {
            if (!handleCommands(commands)) {
                break;
            }
            commands.clear();
            commandMode = false;
        } else if (commandMode && c!='\0') {
            printf("%c", c);
            commands.push_back(c);
        }

        if ((int)c == 13) {
            printf("\r\n");
        } else if (iscntrl(c)) {
            printf("%d\r\n", c);
        } else if (c == ':') {
            commandMode = true;
            printf("%c", c);
        } else {
            printf("%c\r\n", c);
        }
    }
    return 0;
}
