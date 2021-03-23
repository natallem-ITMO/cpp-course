#pragma once

#include <type_traits>
#include <utility>

struct nullopt_t {};

inline constexpr nullopt_t nullopt{};

struct in_place_t {};

inline constexpr in_place_t in_place{};

template <typename T> union storage;

template <typename T,
          bool isTriviallyDestructible = std::is_trivially_destructible_v<T>>
class optional_destructor_base {

  union storage_t {
    constexpr storage_t() noexcept : dummy() {}

    template <typename... Args>
    constexpr storage_t(in_place_t, Args &&... args)
        : value(std::forward<Args>(args)...) {}

    ~storage_t(){};

    char dummy;
    T value;
  };

protected:
  constexpr optional_destructor_base() = default;

  ~optional_destructor_base() { storage_reset(); };

  constexpr optional_destructor_base(optional_destructor_base &&other) =
      default;
  constexpr optional_destructor_base(optional_destructor_base const &other) =
      default;

  optional_destructor_base &
  operator=(optional_destructor_base const &other) = default;
  optional_destructor_base &
  operator=(optional_destructor_base &&other) = default;

  template <typename... Args>
  explicit constexpr optional_destructor_base(in_place_t, Args &&... args)
      : stg(in_place, std::forward<Args>(args)...), is_initialized(true) {}

  constexpr void storage_reset() noexcept {
    if (is_initialized) {
      stg.value.~T();
      is_initialized = false;
    }
  };

  storage_t stg;
  bool is_initialized = false;
};

/**
 * Specialization for trivially destructible
 */
template <typename T> class optional_destructor_base<T, true> {
  union storage_t {
    constexpr storage_t() noexcept : dummy() {}

    template <typename... Args>
    constexpr storage_t(in_place_t, Args &&... args)
        : value(std::forward<Args>(args)...) {}

    ~storage_t() = default;

    char dummy;
    T value;
  };

protected:
  constexpr optional_destructor_base() = default;

  ~optional_destructor_base() = default;

  constexpr optional_destructor_base(optional_destructor_base const &other) =
      default;
  constexpr optional_destructor_base(optional_destructor_base &&other) =
      default;

  optional_destructor_base &
  operator=(optional_destructor_base const &other) = default;
  optional_destructor_base &
  operator=(optional_destructor_base &&other) = default;

  template <typename... Args>
  explicit constexpr optional_destructor_base(in_place_t, Args &&... args)
      : stg(in_place, std::forward<Args>(args)...), is_initialized(true) {}

  void storage_reset() {
    if (is_initialized) {
      stg.value.~T();
      is_initialized = false;
    }
  };

  storage_t stg;
  bool is_initialized = false;
};

template <typename T,
          bool isTriviallyCopyable = std::is_trivially_copyable_v<T>>
class optional_copy_base : optional_destructor_base<T> {
  using base_t = optional_destructor_base<T>;

protected:
  using base_t::is_initialized;
  using base_t::optional_destructor_base;
  using base_t::stg;
  using base_t::storage_reset;

  constexpr optional_copy_base() = default;

  ~optional_copy_base() = default;

  constexpr optional_copy_base(optional_copy_base const &other) noexcept(
      std::is_nothrow_copy_constructible_v<T>) {
    if (other.is_initialized) {
      new (&(stg.value)) T(other.stg.value);
      is_initialized = true;
    }
  };
  constexpr optional_copy_base(optional_copy_base &&other) noexcept(
      std::is_nothrow_move_constructible_v<T>) {
    if (other.is_initialized) {
      new (&stg.value) T(std::move(other.stg.value));
      is_initialized = true;
    }
  };

  optional_copy_base &operator=(optional_copy_base const &other) noexcept(
      std::is_nothrow_copy_assignable_v<T>
          &&std::is_nothrow_copy_constructible_v<T>) {
    if (other.is_initialized) {
      if (is_initialized) {
        stg.value = other.stg.value;
      } else {
        new (&stg.value) T(other.stg.value);
      }
      is_initialized = true;
    } else {
      this->storage_reset();
    }
    return *this;
  }
  optional_copy_base &operator=(optional_copy_base &&other) noexcept(
      std::is_nothrow_move_assignable_v<T>
          &&std::is_nothrow_move_constructible_v<T>) {
    if (other.is_initialized) {
      if (is_initialized) {
        stg.value = std::move(other.stg.value);
      } else {
        new (&stg.value) T(std::move(other.stg.value));
      }
      is_initialized = true;
    } else {
      this->storage_reset();
    }
    return *this;
  }
};

/**
 * Specialization for trivially copyable
 */
template <typename T>
class optional_copy_base<T, true> : optional_destructor_base<T> {
  using base_t = optional_destructor_base<T>;

protected:
  using base_t::is_initialized;
  using base_t::optional_destructor_base;
  using base_t::stg;
  using base_t::storage_reset;

  constexpr optional_copy_base() = default;

  ~optional_copy_base() = default;

  constexpr optional_copy_base(optional_copy_base const &) = default;
  constexpr optional_copy_base(optional_copy_base &&) = default;

  optional_copy_base &operator=(optional_copy_base const &) = default;
  optional_copy_base &operator=(optional_copy_base &&) = default;
};

template <typename T> struct optional : private optional_copy_base<T> {
  using base_t = optional_copy_base<T>;

  constexpr optional() noexcept = default;

  constexpr optional(nullopt_t) noexcept {};

  constexpr optional(T value) : base_t(in_place, std::move(value)) {}

  ~optional() = default;

  constexpr optional(optional const &) = default;
  constexpr optional(optional &&) = default;

  optional &operator=(optional const &) = default;
  optional &operator=(optional &&) = default;

  optional &operator=(nullopt_t) noexcept {
    this->storage_reset();
    return *this;
  }

  template <typename... Args>
  explicit constexpr optional(in_place_t, Args &&... args)
      : base_t(in_place, std::forward<Args>(args)...) {}

  constexpr explicit operator bool() const noexcept {
    return this->is_initialized;
  }

  constexpr T &operator*() noexcept { return this->stg.value; }

  constexpr T const &operator*() const noexcept { return this->stg.value; }

  constexpr T *operator->() noexcept { return &this->stg.value; }

  constexpr T const *operator->() const noexcept { return &this->stg.value; }

  template <typename... Args> void emplace(Args &&... args) {
    this->storage_reset();
    new (&(this->stg.value)) T(std::forward<Args>(args)...);
    this->is_initialized = true;
  }

  void reset() noexcept { this->storage_reset(); }
};

template <typename T>
constexpr bool operator==(optional<T> const &a, optional<T> const &b) {
  if (static_cast<bool>(a) == static_cast<bool>(b)) {
    return !(static_cast<bool>(a)) || *a == *b;
  }
  return false;
}

template <typename T>
constexpr bool operator!=(optional<T> const &a, optional<T> const &b) {
  return !(a == b);
}

template <typename T>
constexpr bool operator<(optional<T> const &a, optional<T> const &b) {
  if (!static_cast<bool>(b))
    return false;
  if (!static_cast<bool>(a))
    return true;
  return *a < *b;
}

template <typename T>
constexpr bool operator<=(optional<T> const &a, optional<T> const &b) {
  if (!static_cast<bool>(a))
    return true;
  if (!static_cast<bool>(b))
    return false;
  return *a <= *b;
}

template <typename T>
constexpr bool operator>(optional<T> const &a, optional<T> const &b) {
  return !(a <= b);
}

template <typename T>
constexpr bool operator>=(optional<T> const &a, optional<T> const &b) {
  return !(a < b);
}
