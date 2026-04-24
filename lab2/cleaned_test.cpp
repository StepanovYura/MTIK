#include<iostream>
int add(int x, int y) {
return x + y;
}
int main() {
int a = 10;
int b = 20;
int result = 0;
result = a + b;
std::cout << "a + b = " << result << std::endl;
if (a < b && b > 0) {
std::cout << "a is less than b" << std::endl;
} else {
std::cout << "a is not less than b" << std::endl;
}
for (int i = 0; i < 3; i++) {
std::cout << "i = " << i << std::endl;
}
int sum = add(a, b);
std::cout << "Sum via function: " << sum << std::endl;
return 0;
}