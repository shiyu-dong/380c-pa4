#include <stdio.h>
#define WriteLine() printf("\n");
#define WriteLong(x) printf(" %lld", x);
#define ReadLong(a) if (fscanf(stdin, "%lld", &a) != 1) a = 0;
#define long long long

long array[12][10];

void local_cse(long b, long c)
{
  long a;
  long i;
  /* The array address computations should be PRE-ed */
  a = (((b+c) + (b+c)) + ((b+c) + (b+c))) + (((b+c) + (b+c)) + ((b+c) + (b+c)));
  WriteLong(a);
  WriteLine();
  array[0][3] = a;
  array[0][4] = a + array[0][3];
  array[0][5] = a + array[0][4];
  a = a + (array[0][3] + array[0][4] + array[0][5]);
  WriteLong(a);
  i = 0;
  a = a + (array[i][3] + array[i][4]);
  WriteLong(a);
  WriteLine();
}

void if_0(long a, long b, long c) {
  long d;
  d = (a + b) + c;
  if (c == 0) {
    WriteLong((a + b) + 1);
  } else {
    WriteLong((a + b) + 2);
    if (a < b) {
      a = a + b;
      b = a + b;
      c = a + b;
      WriteLong(c);
    } else {
      WriteLong(a+b);
    }
    WriteLong((a + b) + 3);
  }
}

void if_1(long a, long b, long c, long d)
{
  /* The comparison should also be PRE-ed */
  if (b < c) {
    a = a + c;
  }

  if (d < (a+c)) {
    if (b < c) {
      WriteLong(a);
    } else {
      WriteLong(a + c);
    }
  }
}

void loop_0(long a)
{
  long i;
  long sum;
  i = 0;
  sum = 0;
  while (i < a) {
    sum = sum + i;
    i = i + 1;
  }
  WriteLong(sum);
}

void loop_1(long a)
{
  long i;
  long sum;
  long array[10];
  i = 0;
  sum = 0;
  while (i < a) {
    sum = sum + i;
    i = i + 1;
    if (i < 10) {
      array[i] = sum;
    }
  }
  WriteLong(sum);
}

void loop_3(long b, long c)
{
  long a;
  a = (b + c);
  while (a < 100) {
    a = (((b+c) + (b+c)) + ((b+c) + (b+c))) + (((b+c) + (b+c)) + ((b+c) + (b+c)));
    if (a < 75) {
      a = a + (b + c);
    } else {
      a = a - (b + c);
    }
    WriteLong(a);
    a = a + a;
  }
}

void loop_4(long b, long c)
{
  long a;
  a = 0;
  while (a < 70) {
    if (a < 75) {
      a = a + (b + c);
    }
  }
  a = a + (b+c);
  while (a < 150) {
    if (a < 160) {
      a = a + (b+c);
    }
  }
  WriteLong(a);
}

void main()
{
  //local_cse(99, 100);
  //WriteLine();
  if_0(3, 4, 10);
  WriteLine();
  //if_0(4, 3, 10);
  //WriteLine();
  //if_0(3, 4, 0);
  //WriteLine();
  //if_1(0, 7, 9, 2);
  //WriteLine();
  //if_1(0, 7, 9, 2);
  //WriteLine();
  //if_1(0, 9, 7, 2);
  //WriteLine();
  //if_1(0, 9, 7, 200);
  //WriteLine();

  //loop_0(5);
  //WriteLine();
  //loop_1(20);
  //WriteLine();

  //loop_3(1, 9);//a < 700, a > 75
  //WriteLine();
  //loop_3(1, 100);// a > 100
  //WriteLine();
  //loop_3(1, 8);//a < 100, a < 75
  //WriteLine();

  //loop_4(3, 4);//enter every branch
  //WriteLine();
  //loop_4(50, 25);//enter first while once, secnod while once, second if once
  //WriteLine();
  //loop_4(75, 80);//enter first while once, second while once, no second if
  //WriteLine();
  //loop_4(80, 80);//enter first while once, no second while
  //WriteLine();
}

