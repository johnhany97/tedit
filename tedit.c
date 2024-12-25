/*** includes ***/
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

/*** defines ***/

// ctrl key combined with alphabetic keys mapps to bytes 1-26.\
// This macro bitwsise-ANDs a char with the value 00011111, in binary
// which in other words, sets the upper 3 bits of the char to 0. This
// mirrors what the ctrl key does in the terminal: it strips bits 5 and 6
// from whatever key you press in combination with ctrl, and sends that.
#define CTRL_KEY(k) ((k) & 0x1f)

/*** data ***/

struct termios orig_termios;

/*** terminal ***/

void onExit() {
    // Clear screen
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
}

// Most C library functions that fail will set the global `errno` variable
// to indicate what the error was. `perror()` looks at the global errno variable
// and prints a descriptive error message for it. Also prints the string
// given to it before it prints the error message, which is meant to provide
// context about what part of your code caused the error.
void die(const char *s) {
    onExit();

    perror(s);
    // exit with status 1 to indicate a failure
    exit(1);
}

void disableRawMode() {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == 1) {
        die("tcsetattr");
    }
}

void enableRawMode() {
  if (tcgetattr(STDIN_FILENO, &orig_termios) == 1) {
      die("tcgetattr");
  }
  atexit(disableRawMode);

  struct termios raw = orig_termios;
  // break conditions will cause sigint, disable that,
  // and fix ctrl-m,
  // and enable parity checking,
  // and stop the 8th bit of each input byte being stripped,
  // and turn off ctrl-s / ctrl-q,
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  // turn off all output processing
  raw.c_oflag &= ~(OPOST);
  // CS8 is a bit mask with a multiple bits, which we sit using
  // bitwise-OR unlike all the flags we are turning off.
  // Sets the char size (CS) to 8 bits per byte.
  raw.c_cflag |= (CS8);
  // turn off echoing (akin to sudo mode),
  // and canonical mode (byte-by-byte, instead of line-by-line),
  // and ctrl-v signal,
  // and ctrl-c / ctrl-z signals
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  // Sets the minimum number of bytes of input needed before read()
  // can read. Set it to 0, so that read() returns as soon as there
  // is any input to be read.
  raw.c_cc[VMIN] = 0;
  // Sets the maximum amount of time (in tenth of a second) to wait
  // before read() returns. Effectively set to 100ms (1/10 sec)
  raw.c_cc[VTIME] = 1;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == 1) {
      die("tcsetattr");
  }
}

char editorReadKey() {
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        if (nread == -1 && errno != EAGAIN) die("read");
    }
    return c;
}

/*** output ***/

void editorDrawRows() {
    int y;
    for (y = 0; y < 24; y++) {
        write(STDIN_FILENO, "~\r\n", 3);
    }
}

void editorRefreshScreen() {
    // \x1b is the escape char, or 27 in decimal,

    // We are writing an escape sequence to the terminal, which always
    // start with the escape char, followed by `[` char. Escape sequences
    // instruct the terminal to do various text formatting tasks, like
    // coloring text, moving the cursor around, and clearing parts of the screen.
    // `J` command is [Erase in Display](https://vt100.net/docs/vt100-ug/chapter3.html#ED),
    // which clears the screen. 2 tells it to specifically clear the entire screen.
    write(STDOUT_FILENO, "\x1b[2J", 4);

    // Reposition the cursor at the top-left corner, so that we're ready
    // to draw the editor interface from top to bottom.
    // H command [Cursor Position](https://vt100.net/docs/vt100-ug/chapter3.html#CUP)
    write(STDOUT_FILENO, "\x1b[H", 3);

    editorDrawRows();
    write(STDOUT_FILENO, "\x1b[H", 3);
}


/*** input ***/

void editorProcessKeypress() {
    char c = editorReadKey();
    switch (c) {
        case CTRL_KEY('q'):
            onExit();
            exit(0);
            break;
    }
}

/*** init ***/

int main() {
  enableRawMode();
  char c;
  while (1) {
      editorRefreshScreen();
      editorProcessKeypress();
  }

  return 0;
}
