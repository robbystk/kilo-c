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
struct termios orig_termios;

/*** terminal ***/
void die(const char *s) {
    perror(s);
    exit(1);
}

void disableRawMode() {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1) die("tcsetattr");
}

void enableRawMode() {
    // save original config for restoration
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) die("tcgetattr");
    atexit(disableRawMode);

    struct termios raw = orig_termios;

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

/*** init ***/

int main() {
    enableRawMode();

    // read one character at a time until EOF or 'q' is encountered
    while (1) {
        char c = '\0';
        if (read(STDIN_FILENO, &c, 1) == -1) die("read");
        if (iscntrl(c)) {
            printf("%d\r\n", c);
        } else {
            printf("%c (%d)\r\n", c, c);
        }
        if (c == CTRL_KEY('q')) break;
    }
    return 0;
}
