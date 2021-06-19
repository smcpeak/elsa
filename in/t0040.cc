// cc.in40
// provoke exprlist->next double-set

char *strtok(char *str, char *delim);

int main()
{
  char *tok;
  char *delim;
  tok = strtok(((char *)0) , delim);
}
