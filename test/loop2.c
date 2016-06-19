#include <stdio.h>

int main()
{
  int n1, n2;
  int i, j, prod = 1;
  scanf("%d %d", &n1, &n2);
  for (i = 1; i <= n1; i++)
  {
    for (j = 1; j <= n2; j++)
    {
      prod = prod * j;
    }
  }
  printf("%d\n", prod);
  return 0;
}
