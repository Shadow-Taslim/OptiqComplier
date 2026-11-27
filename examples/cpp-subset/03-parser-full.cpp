#include <iostream>
#include <vector>

using namespace std;

int add(int a, int b) {
    return a + b;
}

int main() {
    int x = 0;
    int y = 2;
    double total = 25.50;
    bool active = true;
    char grade = 'A';

    int result = add(10, 20);
    total = total + result * 2.0;

    for (int i = 0; i < 3; i++) {
        x += i;
    }

    while (x > 0) {
        x--;
        if (x == 1) {
            ++y;
        } else {
            y += 1;
        }
    }

    do {
        y--;
    } while (y > 2);

    {
        int nested = (x + y) * 2;
        std::cout << "Nested result: " << nested << std::endl;
    }

    if (active && total >= 0.0) {
        cout << "Grade: " << grade << endl;
    }

    std::cin >> x;
    std::cout << x << std::endl;
    return 0;
}
