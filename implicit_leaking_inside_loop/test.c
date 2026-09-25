#include <stdio.h>
#include <stddef.h>

#include <sanitizer/dfsan_interface.h>

int main(void) {

  int secret = 5;

  dfsan_set_label(1, &secret, sizeof(secret));

  for (int i = 0; i < 5; ++i) {

    for (int j = 0; j < 5; ++j) {

      int x,y;

      if (i + j > 3) {
        x = secret + i + j;
        y = i;

        if (x > 5) {
          printf("LEAK X\n");
        }

        if (y > 4) {
          printf("LEAK Y\n");
        }
      } else {
        x = i+j;
        y = secret + i;

        if (x > 5) {
          printf("LEAK X\n");
        }

        if (y > 4) {
          printf("LEAK Y\n");
        }
      }
    }
  }

  return 0;
}