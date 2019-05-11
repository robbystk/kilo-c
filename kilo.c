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
    raw.c_lflag &= ~(ECHO);     // turn of echoing

    // set attributes in the actual interface or whatever
    // TCSAFLUSH: flush output written, discarding non-read input before change
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main() {
    enableRawMode();

    // read one character at a time until EOF or 'q' is encountered
    char c;
    while(read(STDIN_FILENO, &c, 1) == 1 && c != 'q');
    return 0;
}
