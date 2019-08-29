/*** includes ***/
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <sys/ioctl.h>

/*** defines ***/

#define KILO_VERSION "0.0.1"

#define CTRL_KEY(k) ((k) & 0x1f)    // take the 5 least-significant bits of k

/*** data ***/
struct editorConfig {
    struct termios orig_termios;
    int screenrows;
    int screencols;
    int cx, cy;
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
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) die("read");
    }
    return c;
}

int getCursorPosition(int *rows, int *cols) {
    char buf[32];
    unsigned int i = 0;

    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

    printf("\r\n");
    while (i < sizeof(buf)) {
        if (read(STDIN_FILENO, &buf[i], 1) == -1) break;
        if (buf[i] == 'R') break;
        i++;
    }

    buf[i] = '\0';

    if (buf[0] != '\x1b' || buf[1] != '[') return -1;
    if (sscanf(&buf[2], "%d;%dR", rows, cols) != 2) return -1;

    return 0;
}

int getWinSize(int *rows, int *cols) {
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0){
        if (write(STDOUT_FILENO, "\x1b[999B\x1b[999C", 12) != 12) return -1;
        return getCursorPosition(rows, cols);
    } else {
        *rows = ws.ws_row; *cols = ws.ws_col;
        return 0;
    }
}

/*** append buffer ***/

struct abuf {
    char *b;
    int len;
};

#define ABUF_INIT {NULL, 0}

void abAppend(struct abuf *ab, const char *s, int len) {
    char *new = realloc(ab->b, ab->len + len);

    if (new == NULL) return;
    memcpy(&new[ab->len], s, len);
    ab->b = new;
    ab->len += len;
}

void abFree(struct abuf *ab) {
    free(ab->b);
}

/*** output ***/

void editorDrawRows(struct abuf *ab) {
    int y;
    for (y = 0; y < E.screenrows; y++) {
        if (y == E.screenrows / 3) {
            char welcome[80];
            int welcomelen = snprintf(welcome, sizeof(welcome),
                    "Kilo editor -- version %s", KILO_VERSION);
            if (welcomelen > E.screencols) welcomelen = E.screencols;
            int padding = (E.screencols - welcomelen) / 2;
            if (padding) {
                abAppend(ab, "~", 1);
                padding--;
            }
            while (padding--) abAppend(ab, " ", 1);
            abAppend(ab, welcome, welcomelen);
        } else {
            abAppend(ab, "~", 1);
        }

        abAppend(ab, "\x1b[K", 3);

        if (y < E.screenrows - 1) {
            abAppend(ab, "\r\n", 3);
        }

    }
}

void editorRefreshScreen() {
    struct abuf ab = ABUF_INIT;

    abAppend(&ab, "\x1b[?25l", 6);
    abAppend(&ab, "\x1b[H", 3);

    editorDrawRows(&ab);

    // move cursor
    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cy + 1, E.cx + 1);
    abAppend(&ab, buf, strlen(buf));

    abAppend(&ab, "\x1b[?25h", 6);

    write(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);
}

/*** input ***/

void editorMoveCursor(char key) {
    switch (key) {
        case 'k':
            E.cy--;
            break;
        case 'h':
            E.cx--;
            break;
        case 'j':
            E.cy++;
            break;
        case 'l':
            E.cx++;
            break;
    }
}

void editorProcessKeypress() {
    char c = editorReadKey();

    switch (c) {
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;
        case 'h':
        case 'j':
        case 'k':
        case 'l':
            editorMoveCursor(c);
            break;
    }
}

/*** init ***/

void initEditor() {
    E.cx = 0;
    E.cy = 0;

    if (getWinSize(&E.screenrows, &E.screencols) == -1) die("getWinSize");
}

int main() {
    enableRawMode();
    initEditor();

    while (1) {
        editorRefreshScreen();
        editorProcessKeypress();
    }
    return 0;
}
