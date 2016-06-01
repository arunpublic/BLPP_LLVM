#include <stdio.h>

int main()
{
  int n;
  int i, prod = 1;
  scanf("%d", &n);
  for (i = 1; i <= n; i++)
  {
    prod *= i;
  }
  printf("%d\n", prod);
  return 0;
}
