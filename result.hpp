//
// Created by Youtao Guo on 2023/8/26.
//
#include <any>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <variant>

namespace result {

namespace details {
template <class Tp1, class Tp2>
inline constexpr bool no_same_v = !std::is_same_v<std::decay_t<Tp1>, std::decay_t<Tp2>>;

template <class Op>
struct MemberFunctionPointerTraits {};

template <class R, class Class, class Arg>
struct MemberFunctionPointerTraits<R (Class::*)(Arg)> {
  using class_type = Class;
  using result_type = R;
  using arg_type = Arg;
};

template <class R, class Class, class Arg>
struct MemberFunctionPointerTraits<R (Class::*)(Arg) const> {
  using class_type = Class;
  using result_type = R;
  using arg_type = Arg;
};

template <class R, class Class, class Arg>
struct MemberFunctionPointerTraits<R (Class::*)(Arg) volatile> {
  using class_type = Class;
  using result_type = R;
  using arg_type = Arg;
};

template <class R, class Class, class Arg>
struct MemberFunctionPointerTraits<R (Class::*)(Arg) const volatile> {
  using class_type = Class;
  using result_type = R;
  using arg_type = Arg;
};

template <class R, class Class, class Arg>
struct MemberFunctionPointerTraits<R (Class::*)(Arg) noexcept> {
  using class_type = Class;
  using result_type = R;
  using arg_type = Arg;
};

template <class R, class Class, class Arg>
struct MemberFunctionPointerTraits<R (Class::*)(Arg) const noexcept> {
  using class_type = Class;
  using result_type = R;
  using arg_type = Arg;
};

template <class R, class Class, class Arg>
struct MemberFunctionPointerTraits<R (Class::*)(Arg) volatile noexcept> {
  using class_type = Class;
  using result_type = R;
  using arg_type = Arg;
};

template <class R, class Class, class Arg>
struct MemberFunctionPointerTraits<R (Class::*)(Arg) const volatile noexcept> {
  using class_type = Class;
  using result_type = R;
  using arg_type = Arg;
};

template <class Fn>
struct FunctorTraits : MemberFunctionPointerTraits<decltype(&Fn::operator())> {};

template <class R, class Arg>
struct FunctorTraits<R*(Arg)> {
  using result_type = R;
  using arg_type = Arg;
};

template <class R, class Arg>
struct FunctorTraits<R(Arg)> {
  using result_type = R;
  using arg_type = Arg;
};

template <class Fn, class Arg, bool IsErr = false>
struct Pattern {
  using result_type = std::invoke_result_t<Fn, Arg>;
  using arg_type = Arg;
  static constexpr bool is_err = IsErr;

  result_type operator()(Arg arg) { return f(std::forward<Arg>(arg)); }

  Fn f;
};

}  // namespace details

struct Ok {
  template <class Fn>
  auto operator=(Fn&& f) && -> details::Pattern<
      std::remove_reference_t<Fn>, typename details::FunctorTraits<std::remove_reference_t<Fn>>::arg_type> {
    using OkT = details::Pattern<Fn, typename details::FunctorTraits<std::remove_reference_t<Fn>>::arg_type>;
    return OkT{std::forward<Fn>(f)};
  }

  template <class R, class Arg>
  auto operator=(R (*fptr)(Arg)) && -> details::Pattern<R (*)(Arg), Arg> {
    return details::Pattern<R (*)(Arg), Arg>{fptr};
  }
};

struct Err {
  template <class Fn>
  auto operator=(Fn&& f) && -> details::Pattern<
      std::remove_reference_t<Fn>, typename details::FunctorTraits<std::remove_reference_t<Fn>>::arg_type, true> {
    using ErrT = details::Pattern<Fn, typename details::FunctorTraits<std::remove_reference_t<Fn>>::arg_type, true>;
    return ErrT{std::forward<Fn>(f)};
  }

  template <class R, class Arg>
  auto operator=(R (*fptr)(Arg)) && -> details::Pattern<R (*)(Arg), Arg, true> {
    return details::Pattern<R (*)(Arg), Arg, true>{fptr};
  }
};

class Error;

template <class Tp>
std::optional<Tp> ErrorCast(Error& err) noexcept;

template <class Tp>
std::optional<Tp> ErrorCast(const Error& err) noexcept;

template <class Tp>
std::optional<Tp> ErrorCast(Error&& err) noexcept;

class Error {
  template <class Tp>
  friend std::optional<Tp> ErrorCast(Error& err) noexcept;

  template <class Tp>
  friend std::optional<Tp> ErrorCast(const Error& err) noexcept;

  template <class Tp>
  friend std::optional<Tp> ErrorCast(Error&& err) noexcept;

 public:
  Error() = default;
  Error(const Error&) = default;
  Error(Error&&) = default;
  Error& operator=(const Error&) = default;
  Error& operator=(Error&&) = default;

  template <class Arg, std::enable_if_t<details::no_same_v<Arg, Error> && details::no_same_v<Arg, std::any>, int> = 0>
  explicit Error(Arg&& arg) : err_(std::forward<Arg>(arg)) {}

  template <class Arg, std::enable_if_t<details::no_same_v<Arg, Error> && details::no_same_v<Arg, std::any>, int> = 0>
  Error& operator=(Arg&& arg) {
    err_ = std::forward<Arg>(arg);
    return *this;
  }

  bool has_value() const noexcept { return err_.has_value(); }

 private:
  template <class R, class Self, bool Matched>
  struct InnerPattern {
    static constexpr bool matched = Matched;

    template <bool M>
    decltype(auto) operator|(InnerPattern<R, Self, M>&& y) && {
      if constexpr (Matched) {
        return *this;
      } else {
        using res_t = InnerPattern<R, Self, Matched | M>;
        if (!fn && y.fn) {
          return res_t{std::move(y.fn)};
        } else {
          return res_t{std::move(fn)};
        }
      }
    }

    std::function<R(Self&&)> fn;
  };

  template <class R, class Self, class Fn, class Arg>
  static auto match_pattern(Self&& self, details::Pattern<Fn, Arg, true>&& pattern) {
    if constexpr (std::is_convertible_v<Self, Arg>) {
      using res_t = InnerPattern<R, Self, true>;
      return res_t{[pattern = std::move(pattern)](Self&& s) mutable { return pattern(std::forward<Self>(s)); }};
    } else {
      using res_t = InnerPattern<R, Self, false>;
      using rmed = std::remove_cv_t<std::remove_reference_t<Arg>>;
      auto opt = ErrorCast<rmed>(std::forward<Self>(self));
      if constexpr (std::is_convertible_v<rmed, Arg>) {
        return opt.has_value() ? res_t{[pattern = std::move(pattern)](Self&& s) mutable {
                                   auto opt = ErrorCast<rmed>(std::forward<Self>(s));
                                   return pattern(std::move(opt).value());
                                 }}
                               : res_t{};
      } else {
        return res_t{};
      }
    }
  }

  template <class... Patterns>
  std::common_type_t<typename Patterns::result_type...> match_impl(Patterns&&... patterns) const& {
    using result_type = std::common_type_t<typename Patterns::result_type...>;
    auto reduced = (... | match_pattern<result_type>(*this, std::forward<Patterns>(patterns)));
    static_assert(decltype(reduced)::matched, "pattern matching doesn't cover all the cases");
    return reduced.fn(*this);
  }

  template <class... Patterns>
  std::common_type_t<typename Patterns::result_type...> match_impl(Patterns&&... patterns) && {
    using result_type = std::common_type_t<typename Patterns::result_type...>;
    auto reduced = (... | match_pattern<result_type>(static_cast<Error&&>(*this), std::forward<Patterns>(patterns)));
    static_assert(decltype(reduced)::matched, "pattern matching doesn't cover all the cases");
    return reduced.fn(std::move(*this));
  }

 public:
  template <class... Fn>
  decltype(auto) match(Fn&&... fn) const& {
    return match_impl((Err() = std::forward<Fn>(fn))...);
  }

  template <class... Fn>
  decltype(auto) match(Fn&&... fn) && {
    return match_impl((Err() = std::forward<Fn>(fn))...);
  }

 private:
  std::any err_;
};

template <class Tp>
std::optional<Tp> ErrorCast(Error& err) noexcept {
  if (auto ptr = std::any_cast<Tp>(&err.err_); ptr) {
    return *ptr;
  } else {
    return {};
  }
}

template <class Tp>
std::optional<Tp> ErrorCast(const Error& err) noexcept {
  if (auto ptr = std::any_cast<Tp>(&err.err_); ptr) {
    return *ptr;
  } else {
    return {};
  }
}

template <class Tp>
std::optional<Tp> ErrorCast(Error&& err) noexcept {
  if (auto ptr = std::any_cast<Tp>(&err.err_); ptr) {
    return std::move(*ptr);
  } else {
    return {};
  }
}

struct LogicError {};

struct InvalidArgumentError : LogicError {};
struct LengthError : LogicError {};
struct OutOfRangeError : LogicError {};
struct DivideByZeroError : LogicError {};

struct RuntimeError {};

struct RangeError : RuntimeError {};
struct OverflowError : RuntimeError {};

namespace details {

class TraceableImpl {};

}  // namespace details

class TraceableError {
 public:
  TraceableError() = default;
  TraceableError(const TraceableError&) = default;
  TraceableError(TraceableError&&) = default;
  TraceableError& operator=(const TraceableError&) = default;
  TraceableError& operator=(TraceableError&&) = default;

 private:
  std::shared_ptr<details::TraceableImpl> impl_;
};

template <class Tp, class E>
class Result {
 public:
  using value_type = Tp;
  using error_type = E;

  Result() = delete;

  Result(const Result&) = default;

  Result(Result&&) noexcept = default;

  Result& operator=(const Result&) = default;

  Result& operator=(Result&&) noexcept = default;

  Result(const value_type& val) : var_(std::in_place_index<0>, val) {}

  Result(value_type&& val) : var_(std::in_place_index<0>, std::move(val)) {}

  template <class... Args>
  Result(Ok, Args&&... args) : var_(std::in_place_index<0>, std::forward<Args>(args)...) {}

  template <class... Args>
  Result(Err, Args&&... args) : var_(std::in_place_index<1>, std::forward<Args>(args)...) {}

  value_type& value_or(value_type& other) & noexcept {
    if (var_.index() == 0) {
      return std::get<0>(var_);
    }
    return other;
  }

  const value_type& value_or(const value_type& other) const& noexcept {
    if (var_.index() == 0) {
      return std::get<0>(var_);
    }
    return other;
  }

  value_type&& value_or(value_type&& other) && noexcept {
    if (var_.index() == 0) {
      return std::get<0>(std::forward<decltype(var_)>(var_));
    }
    return std::move(other);
  }

  bool has_value() const noexcept { return var_.index() == 0; }

  decltype(auto) error() const& { return std::get<1>(var_); }
  decltype(auto) error() && { return std::get<1>(std::forward<decltype(var_)>(var_)); }

  value_type& value() & { return std::get<0>(var_); }
  const value_type& value() const& { return std::get<0>(var_); }
  value_type&& value() && { return std::get<0>(std::forward<decltype(var_)>(var_)); }

 private:
  template <class Self, class Fn>
  static auto and_then_impl(Self&& self, Fn&& f) -> std::remove_cv_t<
      std::remove_reference_t<std::invoke_result_t<Fn, decltype(std::forward<Self>(self).value())>>> {
    if (self.has_value()) {
      return std::forward<Fn>(f)(std::forward<Self>(self).value());
    } else {
      return {Err{}, std::forward<Self>(self).error()};
    }
  }

  template <class R, class Self, bool ValueMatched, bool ErrMatched>
  struct InnerPattern {
    using self_value_type = decltype(std::declval<Self>().value());
    static constexpr bool matched = ValueMatched && ErrMatched;

    template <bool VM, bool EM>
    decltype(auto) operator|(InnerPattern<R, Self, VM, EM>&& y) && {
      if constexpr (ValueMatched && ErrMatched) {
        return *this;
      } else {
        using res_t = InnerPattern<R, Self, ValueMatched | VM, ErrMatched | EM>;
        if (!fn && y.fn) {
          return res_t{std::move(y.fn)};
        } else {
          return res_t{std::move(fn)};
        }
      }
    }

    std::function<R(Self&&)> fn;
  };

  template <class R, class Self, class Fn, class Arg, bool IsErr>
  static auto match_pattern(Self&& self, details::Pattern<Fn, Arg, IsErr>&& pattern) {
    using pattern_type = details::Pattern<Fn, Arg, IsErr>;
    using self_value_type = decltype(std::forward<Self>(self).value());
    using self_error_type = decltype(std::forward<Self>(self).error());
    if constexpr (!IsErr && std::is_convertible_v<self_value_type, Arg>) {
      using res_t = InnerPattern<R, Self, true, false>;
      return self.has_value() ? res_t{[pattern = std::move(pattern)](Self&& s) mutable {
                                  return pattern(std::forward<Self>(s).value());
                                }}
                              : res_t{};
    } else if constexpr (IsErr && std::is_convertible_v<self_error_type, Arg>) {
      using res_t = InnerPattern<R, Self, false, true>;
      return self.has_value() ? res_t{}
                              : res_t{[pattern = std::move(pattern)](Self&& s) mutable {
                                  return pattern(std::forward<Self>(s).error());
                                }};
    } else if constexpr (IsErr && std::is_same_v<E, Error>) {
      using res_t = InnerPattern<R, Self, false, false>;
      if (self.has_value()) {
        return res_t{};
      }
      using rmed = std::remove_cv_t<std::remove_reference_t<Arg>>;
      auto opt = ErrorCast<rmed>(std::forward<Self>(self).error());
      if constexpr (std::is_convertible_v<rmed, Arg>) {
        return opt.has_value() ? res_t{[pattern = std::move(pattern)](Self&& s) mutable {
                                   auto opt = ErrorCast<rmed>(std::forward<Self>(s).error());
                                   return pattern(std::move(opt).value());
                                 }}
                               : res_t{};
      } else {
        return res_t{};
      }
    } else {
      return InnerPattern<R, Self, false, false>{};
    }
  }

 public:
  template <class... Patterns>
  std::common_type_t<typename Patterns::result_type...> match(Patterns&&... patterns) const& {
    using result_type = std::common_type_t<typename Patterns::result_type...>;
    auto reduced = (... | match_pattern<result_type>(*this, std::forward<Patterns>(patterns)));
    static_assert(decltype(reduced)::matched, "pattern matching doesn't cover all the cases");
    return reduced.fn(*this);
  }

  template <class... Patterns>
  std::common_type_t<typename Patterns::result_type...> match(Patterns&&... patterns) && {
    using result_type = std::common_type_t<typename Patterns::result_type...>;
    auto reduced = (... | match_pattern<result_type>(static_cast<Result&&>(*this), std::forward<Patterns>(patterns)));
    static_assert(decltype(reduced)::matched, "pattern matching doesn't cover all the cases");
    return reduced.fn(std::move(*this));
  }

  template <class Fn>
  auto and_then(Fn&& f) & {
    return and_then_impl(*this, std::forward<Fn>(f));
  }

  template <class Fn>
  auto and_then(Fn&& f) const& {
    return and_then_impl(*this, std::forward<Fn>(f));
  }

  template <class Fn>
  auto and_then(Fn&& f) && {
    return and_then_impl(static_cast<Result&&>(*this), std::forward<Fn>(f));
  }

  template <class Fn>
  Result or_else(Fn&& f) const& {
    if (has_value()) {
      return *this;
    } else {
      return std::forward<Fn>(f)(error());
    }
  }

  template <class Fn>
  Result or_else(Fn&& f) && {
    if (has_value()) {
      return std::move(*this);
    } else {
      return std::forward<Fn>(f)(error());
    }
  }

 private:
  std::variant<value_type, error_type> var_;
};

}  // namespace result
