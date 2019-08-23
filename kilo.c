/*** includes ***/
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>

/*** defines ***/
#define CTRL_KEY(k) ((k) & 0x1f)    // take the 5 least-significant bits of k

/*** data ***/
struct editorConfig {
    struct termios orig_termios;
};

struct editorConfig E;

/*** terminal ***/
void die(const char *s) {
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    perror(s);
    exit(1);
}

void disableRawMode() {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1) die("tcsetattr");
}

void enableRawMode() {
    // save original config for restoration
    if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1) die("tcgetattr");
    atexit(disableRawMode);

    struct termios raw = E.orig_termios;

    // set flags appropriately
    // turn of echoing, canonical mode, signals, preprocessing, and parity check
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | BRKINT | IEXTEN | INPCK);
    // turn off flow control and newline translation
    raw.c_iflag &= ~(IXON | ICRNL);
    // turn off output processing
    raw.c_oflag &= ~(OPOST);
    // set character size to 8 bits
    raw.c_cflag |= (CS8);
    // set timeout for read()
    raw.c_cc[VTIME] = 1;
    // set minimum number of characters for read()
    raw.c_cc[VMIN] = 0;

    // set attributes in the actual interface or whatever
    // TCSAFLUSH: flush output written, discarding non-read input before change
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr");
}

char editorReadKey() {
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) == -1) {
        if (nread == -1 && errno != EAGAIN) die("read");
    }
    return c;
}

/*** output ***/

void editorDrawRows() {
    int y;
    for (y = 0; y < 24; y++) {
        write(STDOUT_FILENO, "~\r\n", 3);
    }
}

void editorRefreshScreen() {
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    editorDrawRows();

    write(STDOUT_FILENO, "\x1b[H", 3);
}

/*** input ***/

void editorProcessKeypress() {
    char c = editorReadKey();

    switch (c) {
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;
    }
}

/*** init ***/

int main() {
    enableRawMode();

    while (1) {
        editorRefreshScreen();
        editorProcessKeypress();
    }
    return 0;
}
