# result
Rust style error handling for cpp. It provides `result::Result` which supports pattern matching error handling and monadic operation.

cpp17 required

## Basic Usage

```cpp
#include <iostream>
#include <limits>

#include "result.hpp"
using result::Result;

// define a Result of int64, which error type is int
Result<int64_t, int> Multiply(int64_t a, int64_t b) {
    int64_t res = a * b;
    if (a != 0 && res / a != b) {
        return {result::Err{}, -1}; // return error
    }
    return res; // return result
}

int main() {
    // do pattern matching
    auto v = Multiply(1000, 1000).match(
        result::Ok()  = [](int64_t v) { return v; },
        result::Err() = [](int err) { std::cerr << "arithmetic overflow\n"; return err; }
    );
    std::cout << v << '\n';
    v = Multiply(std::numeric_limits<int64_t>::max(), 1000).match(
        result::Ok()  = [](int64_t v) { return v; },
        result::Err() = [](int err) { std::cerr << "arithmetic overflow\n"; return err; }
    );
    std::cout << v << '\n';
}
```

the return type of `result::Result::match` can be automatically deduced through the return type of the pattern handler. 

The output will be
```
1000000
arithmetic overflow
-1
```

## Using result::Error

`result::Error` is an type that designed to receive any type of the error. By using pattern matching it's easy to determine what kind of error that exactly is.

First, let's define a function template that returns `Result<T, result::Error>`
```cpp
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
```

```cpp
auto v1 = ElementaryArithmeticFromZeroToMillion('+', 4, 3).match(
        result::Ok()  = [](int v) { return v; },
        result::Err() = [](result::Error&&) { return 0; });
std::cout << v1 << '\n'; // 7

auto v2 = ElementaryArithmeticFromZeroToMillion('-', 1.5, 10.5).match(
        result::Ok() = [](double v) { return v; },
        result::Err() =
            [](result::OutOfRangeError) {
              std::cout << "the arguments of calculating v2 out of range\n";
              return -1;
            },
        result::Err() = /* match here */
            [](result::RangeError) {
              std::cout << "the result of calculating v2 out of range\n";
              return -2;
            },
        result::Err() = /* the default error pattern. must be contained otherwise compile error */
            [](result::Error&&) {
              std::cout << "unknown error\n";
              return -3;
            });
std::cout << v2 << '\n'; // -2
```

`result::Error` itself also supports pattern matching.

```cpp
auto res = ElementaryArithmeticFromZeroToMillion('^', 1, 0);
res.error().match(
    [](result::OutOfRangeError) {
      std::cout << "the arguments out of range\n";
      return 0;
    },
    [](result::InvalidArgumentError) { /* match here */
      std::cout << "the arguments are invalid\n";
      return 0;
    },
    [](result::Error) {
      std::cout << "unknown error\n";
      return 0;
    });
```

## Monadic operation

```cpp
auto res = ElementaryArithmeticFromZeroToMillion('^', 1, 0);
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
```

The output will be
```
op ^ failed, change to op /
op / failed, change to op +
op + succeed, time the result by 10
the final reuslt is 10
```
