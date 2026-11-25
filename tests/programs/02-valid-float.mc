// Valid: floats, widening int -> float, and unary operators.
float price;
float total;
bool discounted;

price = 19.5;
total = price * 2 + 1;
discounted = !(total > 40);

print total;
print discounted;
