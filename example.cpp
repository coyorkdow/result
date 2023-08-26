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


void MoveResult() {
  Result<std::string, int> res(result::Ok{}, 100, '0');
  std::cout << "res is " << (res.value_or("").empty() ? "empty" : "not empty") << " before moved\n";
  auto str = std::move(res).value_or("");
  std::cout << "res is " << (res.value_or("").empty() ? "empty" : "not empty") << " after moved\n";
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

  auto res = ElementaryArithmeticFromZeroToMillion('^', 1, 0);
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

  auto v3 = std::move(res)
                .or_else([](const result::Error&) {
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
                }).value();

  std::cout << v3 << '\n';
  MoveResult();
}
