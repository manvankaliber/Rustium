#include "net/dns/dns_names_ffi.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <new>
#include <stdexcept>
#include <type_traits>
#if __cplusplus >= 202002L
#include <ranges>
#endif

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#ifdef __clang__
#pragma clang diagnostic ignored "-Wdollar-in-identifier-extension"
#endif // __clang__
#endif // __GNUC__

namespace rust {
inline namespace cxxbridge1 {
// #include "rust/cxx.h"

#ifndef CXXBRIDGE1_PANIC
#define CXXBRIDGE1_PANIC
template <typename Exception>
void panic [[noreturn]] (const char *msg);
#endif // CXXBRIDGE1_PANIC

namespace {
template <typename T>
class impl;
} // namespace

class Opaque;

template <typename T>
::std::size_t size_of();
template <typename T>
::std::size_t align_of();

#ifndef CXXBRIDGE1_RUST_SLICE
#define CXXBRIDGE1_RUST_SLICE
namespace detail {
template <bool>
struct copy_assignable_if {};

template <>
struct copy_assignable_if<false> {
  copy_assignable_if() noexcept = default;
  copy_assignable_if(const copy_assignable_if &) noexcept = default;
  copy_assignable_if &operator=(const copy_assignable_if &) & noexcept = delete;
  copy_assignable_if &operator=(copy_assignable_if &&) & noexcept = default;
};
} // namespace detail

template <typename T>
class Slice final
    : private detail::copy_assignable_if<std::is_const<T>::value> {
public:
  using value_type = T;

  Slice() noexcept;
  Slice(T *, std::size_t count) noexcept;

  template <typename C>
  explicit Slice(C &c) : Slice(c.data(), c.size()) {}

  Slice &operator=(const Slice<T> &) & noexcept = default;
  Slice &operator=(Slice<T> &&) & noexcept = default;

  T *data() const noexcept;
  std::size_t size() const noexcept;
  std::size_t length() const noexcept;
  bool empty() const noexcept;

  T &operator[](std::size_t n) const noexcept;
  T &at(std::size_t n) const;
  T &front() const noexcept;
  T &back() const noexcept;

  Slice(const Slice<T> &) noexcept = default;
  ~Slice() noexcept = default;

  class iterator;
  iterator begin() const noexcept;
  iterator end() const noexcept;

  void swap(Slice &) noexcept;

private:
  class uninit;
  Slice(uninit) noexcept;
  friend impl<Slice>;
  friend void sliceInit(void *, const void *, std::size_t) noexcept;
  friend void *slicePtr(const void *) noexcept;
  friend std::size_t sliceLen(const void *) noexcept;

  std::array<std::uintptr_t, 2> repr;
};

#ifdef __cpp_deduction_guides
template <typename C>
explicit Slice(C &c)
    -> Slice<std::remove_reference_t<decltype(*std::declval<C>().data())>>;
#endif // __cpp_deduction_guides

template <typename T>
class Slice<T>::iterator final {
public:
#if __cplusplus >= 202002L
  using iterator_category = std::contiguous_iterator_tag;
#else
  using iterator_category = std::random_access_iterator_tag;
#endif
  using value_type = T;
  using difference_type = std::ptrdiff_t;
  using pointer = typename std::add_pointer<T>::type;
  using reference = typename std::add_lvalue_reference<T>::type;

  reference operator*() const noexcept;
  pointer operator->() const noexcept;
  reference operator[](difference_type) const noexcept;

  iterator &operator++() noexcept;
  iterator operator++(int) noexcept;
  iterator &operator--() noexcept;
  iterator operator--(int) noexcept;

  iterator &operator+=(difference_type) noexcept;
  iterator &operator-=(difference_type) noexcept;
  iterator operator+(difference_type) const noexcept;
  friend inline iterator operator+(difference_type lhs, iterator rhs) noexcept {
    return rhs + lhs;
  }
  iterator operator-(difference_type) const noexcept;
  difference_type operator-(const iterator &) const noexcept;

  bool operator==(const iterator &) const noexcept;
  bool operator!=(const iterator &) const noexcept;
  bool operator<(const iterator &) const noexcept;
  bool operator<=(const iterator &) const noexcept;
  bool operator>(const iterator &) const noexcept;
  bool operator>=(const iterator &) const noexcept;

private:
  friend class Slice;
  void *pos;
  std::size_t stride;
};

template <typename T>
Slice<T>::Slice() noexcept {
  sliceInit(this, reinterpret_cast<void *>(align_of<T>()), 0);
}

template <typename T>
Slice<T>::Slice(T *s, std::size_t count) noexcept {
  assert(s != nullptr || count == 0);
  sliceInit(this,
            s == nullptr && count == 0
                ? reinterpret_cast<void *>(align_of<T>())
                : const_cast<typename std::remove_const<T>::type *>(s),
            count);
}

template <typename T>
T *Slice<T>::data() const noexcept {
  return reinterpret_cast<T *>(slicePtr(this));
}

template <typename T>
std::size_t Slice<T>::size() const noexcept {
  return sliceLen(this);
}

template <typename T>
std::size_t Slice<T>::length() const noexcept {
  return this->size();
}

template <typename T>
bool Slice<T>::empty() const noexcept {
  return this->size() == 0;
}

template <typename T>
T &Slice<T>::operator[](std::size_t n) const noexcept {
  assert(n < this->size());
  auto ptr = static_cast<char *>(slicePtr(this)) + size_of<T>() * n;
  return *reinterpret_cast<T *>(ptr);
}

template <typename T>
T &Slice<T>::at(std::size_t n) const {
  if (n >= this->size()) {
    panic<std::out_of_range>("rust::Slice index out of range");
  }
  return (*this)[n];
}

template <typename T>
T &Slice<T>::front() const noexcept {
  assert(!this->empty());
  return (*this)[0];
}

template <typename T>
T &Slice<T>::back() const noexcept {
  assert(!this->empty());
  return (*this)[this->size() - 1];
}

template <typename T>
typename Slice<T>::iterator::reference
Slice<T>::iterator::operator*() const noexcept {
  return *static_cast<T *>(this->pos);
}

template <typename T>
typename Slice<T>::iterator::pointer
Slice<T>::iterator::operator->() const noexcept {
  return static_cast<T *>(this->pos);
}

template <typename T>
typename Slice<T>::iterator::reference Slice<T>::iterator::operator[](
    typename Slice<T>::iterator::difference_type n) const noexcept {
  auto ptr = static_cast<char *>(this->pos) + this->stride * n;
  return *reinterpret_cast<T *>(ptr);
}

template <typename T>
typename Slice<T>::iterator &Slice<T>::iterator::operator++() noexcept {
  this->pos = static_cast<char *>(this->pos) + this->stride;
  return *this;
}

template <typename T>
typename Slice<T>::iterator Slice<T>::iterator::operator++(int) noexcept {
  auto ret = iterator(*this);
  this->pos = static_cast<char *>(this->pos) + this->stride;
  return ret;
}

template <typename T>
typename Slice<T>::iterator &Slice<T>::iterator::operator--() noexcept {
  this->pos = static_cast<char *>(this->pos) - this->stride;
  return *this;
}

template <typename T>
typename Slice<T>::iterator Slice<T>::iterator::operator--(int) noexcept {
  auto ret = iterator(*this);
  this->pos = static_cast<char *>(this->pos) - this->stride;
  return ret;
}

template <typename T>
typename Slice<T>::iterator &Slice<T>::iterator::operator+=(
    typename Slice<T>::iterator::difference_type n) noexcept {
  this->pos = static_cast<char *>(this->pos) + this->stride * n;
  return *this;
}

template <typename T>
typename Slice<T>::iterator &Slice<T>::iterator::operator-=(
    typename Slice<T>::iterator::difference_type n) noexcept {
  this->pos = static_cast<char *>(this->pos) - this->stride * n;
  return *this;
}

template <typename T>
typename Slice<T>::iterator Slice<T>::iterator::operator+(
    typename Slice<T>::iterator::difference_type n) const noexcept {
  auto ret = iterator(*this);
  ret.pos = static_cast<char *>(this->pos) + this->stride * n;
  return ret;
}

template <typename T>
typename Slice<T>::iterator Slice<T>::iterator::operator-(
    typename Slice<T>::iterator::difference_type n) const noexcept {
  auto ret = iterator(*this);
  ret.pos = static_cast<char *>(this->pos) - this->stride * n;
  return ret;
}

template <typename T>
typename Slice<T>::iterator::difference_type
Slice<T>::iterator::operator-(const iterator &other) const noexcept {
  auto diff = std::distance(static_cast<char *>(other.pos),
                            static_cast<char *>(this->pos));
  return diff / static_cast<typename Slice<T>::iterator::difference_type>(
                    this->stride);
}

template <typename T>
bool Slice<T>::iterator::operator==(const iterator &other) const noexcept {
  return this->pos == other.pos;
}

template <typename T>
bool Slice<T>::iterator::operator!=(const iterator &other) const noexcept {
  return this->pos != other.pos;
}

template <typename T>
bool Slice<T>::iterator::operator<(const iterator &other) const noexcept {
  return this->pos < other.pos;
}

template <typename T>
bool Slice<T>::iterator::operator<=(const iterator &other) const noexcept {
  return this->pos <= other.pos;
}

template <typename T>
bool Slice<T>::iterator::operator>(const iterator &other) const noexcept {
  return this->pos > other.pos;
}

template <typename T>
bool Slice<T>::iterator::operator>=(const iterator &other) const noexcept {
  return this->pos >= other.pos;
}

template <typename T>
typename Slice<T>::iterator Slice<T>::begin() const noexcept {
  iterator it;
  it.pos = slicePtr(this);
  it.stride = size_of<T>();
  return it;
}

template <typename T>
typename Slice<T>::iterator Slice<T>::end() const noexcept {
  iterator it = this->begin();
  it.pos = static_cast<char *>(it.pos) + it.stride * this->size();
  return it;
}

template <typename T>
void Slice<T>::swap(Slice &rhs) noexcept {
  std::swap(*this, rhs);
}
#endif // CXXBRIDGE1_RUST_SLICE

#ifndef CXXBRIDGE1_IS_COMPLETE
#define CXXBRIDGE1_IS_COMPLETE
namespace detail {
namespace {
template <typename T, typename = std::size_t>
struct is_complete : std::false_type {};
template <typename T>
struct is_complete<T, decltype(sizeof(T))> : std::true_type {};
} // namespace
} // namespace detail
#endif // CXXBRIDGE1_IS_COMPLETE

#ifndef CXXBRIDGE1_LAYOUT
#define CXXBRIDGE1_LAYOUT
class layout {
  template <typename T>
  friend std::size_t size_of();
  template <typename T>
  friend std::size_t align_of();
  template <typename T>
  static typename std::enable_if<std::is_base_of<Opaque, T>::value,
                                 std::size_t>::type
  do_size_of() {
    return T::layout::size();
  }
  template <typename T>
  static typename std::enable_if<!std::is_base_of<Opaque, T>::value,
                                 std::size_t>::type
  do_size_of() {
    return sizeof(T);
  }
  template <typename T>
  static
      typename std::enable_if<detail::is_complete<T>::value, std::size_t>::type
      size_of() {
    return do_size_of<T>();
  }
  template <typename T>
  static typename std::enable_if<std::is_base_of<Opaque, T>::value,
                                 std::size_t>::type
  do_align_of() {
    return T::layout::align();
  }
  template <typename T>
  static typename std::enable_if<!std::is_base_of<Opaque, T>::value,
                                 std::size_t>::type
  do_align_of() {
    return alignof(T);
  }
  template <typename T>
  static
      typename std::enable_if<detail::is_complete<T>::value, std::size_t>::type
      align_of() {
    return do_align_of<T>();
  }
};

template <typename T>
std::size_t size_of() {
  return layout::size_of<T>();
}

template <typename T>
std::size_t align_of() {
  return layout::align_of<T>();
}
#endif // CXXBRIDGE1_LAYOUT
} // namespace cxxbridge1
} // namespace rust

namespace net {
  namespace dns_names_internal {
    struct NameBuffer;
    struct LabelChunk;
    using CanonicalName = ::net::dns_names_internal::CanonicalName;
    using NameReader = ::net::dns_names_internal::NameReader;
  }
}

namespace net {
namespace dns_names_internal {
#ifndef CXXBRIDGE1_STRUCT_net$dns_names_internal$NameBuffer
#define CXXBRIDGE1_STRUCT_net$dns_names_internal$NameBuffer
struct NameBuffer final {
  ::std::array<::std::uint8_t, 255> bytes;

  using IsRelocatable = ::std::true_type;
};
#endif // CXXBRIDGE1_STRUCT_net$dns_names_internal$NameBuffer

#ifndef CXXBRIDGE1_STRUCT_net$dns_names_internal$LabelChunk
#define CXXBRIDGE1_STRUCT_net$dns_names_internal$LabelChunk
struct LabelChunk final {
  ::std::array<::std::uint8_t, 256> bytes;

  using IsRelocatable = ::std::true_type;
};
#endif // CXXBRIDGE1_STRUCT_net$dns_names_internal$LabelChunk

extern "C" {
bool net$dns_names_internal$cxxbridge1$205$host_compliant(::rust::Slice<::std::uint8_t const> input) noexcept {
  bool (*host_compliant$)(::rust::Slice<::std::uint8_t const>) = ::net::dns_names_internal::host_compliant;
  return host_compliant$(input);
}

bool net$dns_names_internal$cxxbridge1$205$host_localhost(::rust::Slice<::std::uint8_t const> input) noexcept {
  bool (*host_localhost$)(::rust::Slice<::std::uint8_t const>) = ::net::dns_names_internal::host_localhost;
  return host_localhost$(input);
}

bool net$dns_names_internal$cxxbridge1$205$host_ip_literal(::rust::Slice<::std::uint8_t const> input) noexcept {
  bool (*host_ip_literal$)(::rust::Slice<::std::uint8_t const>) = ::net::dns_names_internal::host_ip_literal;
  return host_ip_literal$(input);
}

bool net$dns_names_internal$cxxbridge1$205$host_url_address(::rust::Slice<::std::uint8_t const> input) noexcept {
  bool (*host_url_address$)(::rust::Slice<::std::uint8_t const>) = ::net::dns_names_internal::host_url_address;
  return host_url_address$(input);
}

bool net$dns_names_internal$cxxbridge1$205$host_canonicalize(::rust::Slice<::std::uint8_t const> input, ::net::dns_names_internal::CanonicalName &output) noexcept {
  bool (*host_canonicalize$)(::rust::Slice<::std::uint8_t const>, ::net::dns_names_internal::CanonicalName &) = ::net::dns_names_internal::host_canonicalize;
  return host_canonicalize$(input, output);
}

void net$dns_names_internal$cxxbridge1$205$canonical_use_original(::rust::Slice<::std::uint8_t const> input, ::net::dns_names_internal::CanonicalName &output) noexcept {
  void (*canonical_use_original$)(::rust::Slice<::std::uint8_t const>, ::net::dns_names_internal::CanonicalName &) = ::net::dns_names_internal::canonical_use_original;
  canonical_use_original$(input, output);
}

void net$dns_names_internal$cxxbridge1$205$reader_remaining(::net::dns_names_internal::NameReader const &reader, ::rust::Slice<::std::uint8_t const> *return$) noexcept {
  ::rust::Slice<::std::uint8_t const> (*reader_remaining$)(::net::dns_names_internal::NameReader const &) = ::net::dns_names_internal::reader_remaining;
  new (return$) ::rust::Slice<::std::uint8_t const>(reader_remaining$(reader));
}

::net::dns_names_internal::LabelChunk net$dns_names_internal$cxxbridge1$205$reader_take(::net::dns_names_internal::NameReader &reader, ::std::size_t count) noexcept {
  ::net::dns_names_internal::LabelChunk (*reader_take$)(::net::dns_names_internal::NameReader &, ::std::size_t) = ::net::dns_names_internal::reader_take;
  return reader_take$(reader, count);
}

::std::uint64_t net$dns_names_internal$cxxbridge1$205$dotted_name(::rust::Slice<::std::uint8_t const> input, bool require_hostname, ::net::dns_names_internal::NameBuffer &output) noexcept;

bool net$dns_names_internal$cxxbridge1$205$valid_name(::rust::Slice<::std::uint8_t const> input) noexcept;

bool net$dns_names_internal$cxxbridge1$205$valid_record(::rust::Slice<::std::uint8_t const> input) noexcept;

::std::uint64_t net$dns_names_internal$cxxbridge1$205$network_name(::rust::Slice<::std::uint8_t const> input, bool complete, ::net::dns_names_internal::NameBuffer &output) noexcept;

::std::uint64_t net$dns_names_internal$cxxbridge1$205$network_name_reader(::net::dns_names_internal::NameReader &reader, bool complete, ::net::dns_names_internal::NameBuffer &output) noexcept;

::std::uint32_t net$dns_names_internal$cxxbridge1$205$read_length(::rust::Slice<::std::uint8_t const> input, bool wide) noexcept;

void net$dns_names_internal$cxxbridge1$205$canonical_name(::rust::Slice<::std::uint8_t const> input, ::net::dns_names_internal::CanonicalName &output) noexcept;
} // extern "C"

::std::uint64_t dotted_name(::rust::Slice<::std::uint8_t const> input, bool require_hostname, ::net::dns_names_internal::NameBuffer &output) noexcept {
  return net$dns_names_internal$cxxbridge1$205$dotted_name(input, require_hostname, output);
}

bool valid_name(::rust::Slice<::std::uint8_t const> input) noexcept {
  return net$dns_names_internal$cxxbridge1$205$valid_name(input);
}

bool valid_record(::rust::Slice<::std::uint8_t const> input) noexcept {
  return net$dns_names_internal$cxxbridge1$205$valid_record(input);
}

::std::uint64_t network_name(::rust::Slice<::std::uint8_t const> input, bool complete, ::net::dns_names_internal::NameBuffer &output) noexcept {
  return net$dns_names_internal$cxxbridge1$205$network_name(input, complete, output);
}

::std::uint64_t network_name_reader(::net::dns_names_internal::NameReader &reader, bool complete, ::net::dns_names_internal::NameBuffer &output) noexcept {
  return net$dns_names_internal$cxxbridge1$205$network_name_reader(reader, complete, output);
}

::std::uint32_t read_length(::rust::Slice<::std::uint8_t const> input, bool wide) noexcept {
  return net$dns_names_internal$cxxbridge1$205$read_length(input, wide);
}

void canonical_name(::rust::Slice<::std::uint8_t const> input, ::net::dns_names_internal::CanonicalName &output) noexcept {
  net$dns_names_internal$cxxbridge1$205$canonical_name(input, output);
}
} // namespace dns_names_internal
} // namespace net
