#include <unistd.h>
#include <termios.h>

void enableRawMode() {
    // Put terminal in raw mode
    struct termios raw;
    tcgetattr(STDIN_FILENO, &raw);  // get terminal attribute struct

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
