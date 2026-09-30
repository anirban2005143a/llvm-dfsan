#include <stdio.h>
#include <stddef.h>

#include <sanitizer/dfsan_interface.h>

int main(void) {

  int secret = 5;

  dfsan_set_label(1, &secret, sizeof(secret));
      int x = secret ,y,z;

  for (int i = 0; i < 3; ++i) {
      z=y;
      y=x;
      
      y_label = dfsan_read_label(&y)
      z_label = dfsan_read_label(&z)

      printf("y_label" , )
      // if(i > 1){
      //   // if(x > 2) printf("leaking x and y");
      // }else{
      //   if(y) printf("leaking y");
      // }
  }

  return 0;
}