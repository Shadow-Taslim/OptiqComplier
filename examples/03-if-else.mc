// Relational and logical expressions driving branches.
int score;
bool passed;

score = 72;
passed = score >= 40 && score <= 100;

if (passed == true) {
  print score;
} else {
  print 0;
}
