#include <stdio.h>
#include <stddef.h>

#include <sanitizer/dfsan_interface.h>

int main(void) {

  int secret = 5;

  dfsan_set_label(1, &secret, sizeof(secret));

  for (int i = 0; i < 3; ++i) {
      int x = secret ,y = secret+2;
      
      if(i > 1){
        if(x > 2) printf("leaking x and y");
      }else{
        if(y) printf("leaking y");
      }
    
  }

  return 0;
}