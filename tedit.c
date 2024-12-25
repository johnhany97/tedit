#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

struct termios orig_termios;

void disableRawMode() { tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios); }

void enableRawMode() {
  tcgetattr(STDIN_FILENO, &orig_termios);
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

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int main() {
  enableRawMode();
  char c;
  while (1) {
      char c = '\0';
      read(STDIN_FILENO, &c, 1);
      if (iscntrl(c)) {
          printf("%d\r\n", c);
      } else {
          printf("%d ('%c')\r\n", c, c);
      }
      if (c == 'q') break;
  }

  return 0;
}
