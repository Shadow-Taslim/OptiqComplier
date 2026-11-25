// Nested blocks with proper scoping: the inner x shadows the outer x.
int x;
x = 1;

if (true) {
  int x;
  x = 2;
  print x;
}

print x;
