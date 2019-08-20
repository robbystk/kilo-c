#include <ctype.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>

struct termios orig_termios;

void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode() {
    // save original config for restoration
    tcgetattr(STDIN_FILENO, &orig_termios);
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

    // set attributes in the actual interface or whatever
    // TCSAFLUSH: flush output written, discarding non-read input before change
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main() {
    enableRawMode();

    // read one character at a time until EOF or 'q' is encountered
    char c;
    while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q') {
        if (iscntrl(c)) {
            printf("%d\r\n", c);
        } else {
            printf("%c (%d)\r\n", c, c);
        }
    }
    return 0;
}
