#include <stdio.h>
#define WriteLine() printf("\n");
#define WriteLong(x) printf(" %lld", x);
#define ReadLong(a) if (fscanf(stdin, "%lld", &a) != 1) a = 0;
#define long long long

void main()
{
  long a;
  long b;
  long c;
  a = 80;
  b = 2;
  c = 3;

  if (a < 70) {
    while (a < 75){
      a = a+(b+c);
    }
  }
  else {
    while(a<90) {
      a=a+(b+c);
    }
  }
  a = a+(b+c);
  WriteLong(a);
}
