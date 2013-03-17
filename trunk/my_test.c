#include <stdio.h>
#define WriteLine() printf("\n");
#define WriteLong(x) printf(" %lld", x);
#define ReadLong(a) if (fscanf(stdin, "%lld", &a) != 1) a = 0;
#define long long long

void main()
{
  long a;
  long m;
  m = 9;
  a = 2;

  if (a>1) {
    //a=a+1;
    if (a>2) {
      a=a+3;
      while(a>4) {
        a = 1-m;
      }
      a = a+5;
    }
    else {
      a = 1-m;
    }
    a = 1-m;
  }
  else {
    a=a+2;
  }
  WriteLong(a);
  m = 2;
  a = 1-m;
}
