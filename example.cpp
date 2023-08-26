//
// Created by Youtao Guo on 2023/8/26.
//

#include <cassert>
#include <iostream>

#include "result.hpp"

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

int invalid_argument_handler(result::InvalidArgumentError) {
  std::cout << "invalid argument\n";
  return 0;
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
                return 0;
              },
          result::Err() =
              [](result::RangeError) {
                std::cout << "the result of calculating v2 out of range\n";
                return 0;
              },
          result::Err() =
              [](result::Error&&) {
                std::cout << "unknown error\n";
                return 0;
              });

  auto res = ElementaryArithmeticFromZeroToMillion('^', 0, 0);
  res.error().match(
      [](result::OutOfRangeError) {
        std::cout << "the arguments out of range\n";
        return 0;
      },
      invalid_argument_handler,
      [](result::Error) {
        std::cout << "unknown error\n";
        return 0;
      });

  res.error().match(
      [](result::Error) {
        std::cout << "unknown error\n";
        return 0;
      },
      invalid_argument_handler
      );

  std::cout << v2 << '\n';
}
