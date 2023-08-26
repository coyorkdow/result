# result
rust style error handling for cpp

```cpp
#include <result.hpp>
#include <iostream>
using result::Result;

template <class T>
Result<T, result::Error> ElementaryArithmeticFromZeroToMillion(char op, T x, T y) {
  const T million = 1'000'000;
  if (!(x >= 0 && x <= million) || !(y >= 0 && y <= million)) {
    return {result::Err{}, result::OutOfRangeError{}};
  }
  using res_t = Result<T, result::Error>;
  T res{};
  switch (op) {
    case '+':
      res = x + y;
      return (res >= 0 && res <= million) ? res_t{res} : res_t{result::Err{}, result::RangeError{}};
    case '-':
      res = x - y;
      return (res >= 0 && res <= million) ? res_t{res} : res_t{result::Err{}, result::RangeError{}};
    case '*':
      res = x * y;
      return (res >= 0 && res <= million) ? res_t{res} : res_t{result::Err{}, result::RangeError{}};
    case '/':
      if (y == 0) {
        return {result::Err{}, result::DivideByZeroError{}};
      }
      res = x / y;
      return (res >= 0 && res <= million) ? res_t{res} : res_t{result::Err{}, result::RangeError{}};
    default:
      return {result::Err{}, result::InvalidArgumentError{}};
  }
}

int main() {
  auto v1 = ElementaryArithmeticFromZeroToMillion('+', 4, 3).match(
          result::Ok()  = [](int v) { return v; },
          result::Err() = [](result::Error&&) { return 0; });
  std::cout << v1 << '\n';

  auto v2 = ElementaryArithmeticFromZeroToMillion('-', 1.5, 10.5).match(
          result::Ok() = [](double v) { return v; },
          result::Err() =
              [](result::OutOfRangeError) {
                std::cout << "the arguments of calculating v2 out of range\n";
                return -1;
              },
          result::Err() =
              [](result::RangeError) {
                std::cout << "the result of calculating v2 out of range\n";
                return -2;
              },
          result::Err() =
              [](result::Error&&) {
                std::cout << "unknown error\n";
                return -3;
              });
  std::cout << v2 << '\n';

  auto res = ElementaryArithmeticFromZeroToMillion('^', 1, 0);
  res.error().match(
      [](result::OutOfRangeError) {
        std::cout << "the arguments out of range\n";
        return 0;
      },
      [](result::InvalidArgumentError) {
        std::cout << "the arguments are invalid\n";
        return 0;
      },
      [](result::Error) {
        std::cout << "unknown error\n";
        return 0;
      });

  auto v3 = res.or_else([](const result::Error&) {
                 std::cout << "op ^ failed, change to op /\n";
                 return ElementaryArithmeticFromZeroToMillion('/', 1, 0);
               })
                .and_then([](int v) {
                  std::cout << "op / succeed\n";
                  return Result<int, result::Error>{v};
                })
                .or_else([](const result::Error&) {
                  std::cout << "op / failed, change to op +\n";
                  return ElementaryArithmeticFromZeroToMillion('+', 1, 0);
                })
                .and_then([](int v) {
                  std::cout << "op + succeed, time the result by 10\n";
                  return ElementaryArithmeticFromZeroToMillion('*', v, 10);
                })
                .or_else([](const result::Error&) {
                  std::cout << "eventually failed, set value as -1\n";
                  return Result<int, result::Error>{-1};
                })
                .value();
  std::cout << "the final reuslt is " << v3 << '\n';
}
```

The output will be
```
7
the result of calculating v2 out of range
-2
the arguments are invalid
op ^ failed, change to op /
op / failed, change to op +
op + succeed, time the result by 10
the final reuslt is 10
```
