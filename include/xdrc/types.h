// -*- C++ -*-

/** \file types.h Type definitions for xdrc compiler output. */

#ifndef _XDRC_TYPES_H_HEADER_INCLUDED_
#define _XDRC_TYPES_H_HEADER_INCLUDED_ 1

#include <array>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace xdr {

using std::uint32_t;

//! Generic class of XDR unmarshaling errors.
struct xdr_runtime_error : std::runtime_error {
  using std::runtime_error::runtime_error;
};

//! Attempt to exceed the bounds of a variable-length array or string.
struct xdr_overflow : xdr_runtime_error {
  using xdr_runtime_error::xdr_runtime_error;
};

//! Attempt to set invalid value for a union discriminant.
struct xdr_bad_discriminant : xdr_runtime_error {
  using xdr_runtime_error::xdr_runtime_error;
};

//! Padding bytes that should have contained zero don't.
struct xdr_should_be_zero : xdr_runtime_error {
  using xdr_runtime_error::xdr_runtime_error;
};

//! Attempt to access wrong field of a union.
struct xdr_wrong_union : std::logic_error {
  using std::logic_error::logic_error;
};


//! This is used to apply an archive to a field.  It is designed as a
//! template class that can be specialized to various archive formats,
//! since some formats may want the fied name and others not.  Other
//! uses include translating types to supertypes, e.g., so an archive
//! can handle \c std::string instead of \c xdr::xstring.
template<typename Archive> struct archive_adapter {
  template<typename T> static void apply(Archive &ar, const char *, T &&t) {
    ar(std::forward<T>(t));
  }
};

//! By default, this function simply applies \c ar (which must be a
//! function object) to \c t.  However, it does so via the \c
//! xdr::archive_adapter template class, which can be specialized to
//! capture the field name as well.
template<typename Archive, typename T> inline void
archive(Archive &ar, const char *name, T &&t)
{
  archive_adapter<Archive>::apply(ar, name, std::forward<T>(t));
}

//! Metadata for all marshalable XDR types.
template<typename T> struct xdr_traits {
  //! \c T is a valid XDR type that can be serialized.
  static constexpr bool valid = false;
  //! \c T is an \c xstring, \c opaque_array, or \c opaque_vec.
  static constexpr bool is_bytes = false;
  //! \c T is an XDR struct.
  static constexpr bool is_struct = false;
  //! \c T is an XDR union.
  static constexpr bool is_union = false;
  //! \c T is an XDR struct or union.
  static constexpr bool is_class = false;
  //! \c T is an XDR enum or bool (traits have enum_name).
  static constexpr bool is_enum = false;
  //! \c T is an xdr::pointer, xdr::xarray, or xdr::xvector (with load/save).
  static constexpr bool is_container = false;
  //! \c T is one of [u]int{32,64}_t, float, or double.
  static constexpr bool is_numeric = false;
  //! \c T has a fixed size.
  static constexpr bool has_fixed_size = false;
};

template<typename T> std::size_t
xdr_size(const T&t)
{
  return xdr_traits<T>::serial_size(t);
}

//! Default xdr_traits values for actual XDR types.
struct xdr_traits_base {
  static constexpr bool valid = true;
  static constexpr bool is_bytes = false;
  static constexpr bool is_class = false;
  static constexpr bool is_enum = false;
  static constexpr bool is_container = false;
  static constexpr bool is_numeric = false;
  static constexpr bool has_fixed_size = false;
};

template<typename To, typename From> To
xdr_reinterpret(From f)
{
  static_assert(sizeof(To) == sizeof(From),
		"xdr_reinterpret with different sized objects");
  union {
    From from;
    To to;
  };
  from = f;
  return to;
}

template<typename T, typename U> struct xdr_integral_base : xdr_traits_base {
  using type = T;
  using uint_type = U;
  static constexpr bool is_numeric = true;
  static constexpr bool has_fixed_size = true;
  static constexpr std::size_t fixed_size = sizeof(uint_type);
  static constexpr std::size_t serial_size(type) { return fixed_size; }
  static uint_type to_uint(type t) { return t; }
  static type from_uint(uint_type u) { return xdr_reinterpret<type>(u); }
};
template<> struct xdr_traits<std::int32_t>
  : xdr_integral_base<std::int32_t, std::uint32_t> {};
template<> struct xdr_traits<std::uint32_t>
  : xdr_integral_base<std::uint32_t, std::uint32_t> {};
template<> struct xdr_traits<std::int64_t>
  : xdr_integral_base<std::int64_t, std::uint64_t> {};
template<> struct xdr_traits<std::uint64_t>
  : xdr_integral_base<std::uint64_t, std::uint64_t> {};

template<typename T, typename U> struct xdr_fp_base : xdr_traits_base {
  using type = T;
  using uint_type = U;
  static_assert(sizeof(type) == sizeof(uint_type),
		"Cannot reinterpret between float and int of different size");
  static constexpr bool is_numeric = true;
  static constexpr bool has_fixed_size = true;
  static constexpr std::size_t fixed_size = sizeof(uint_type);
  static constexpr std::size_t serial_size(type) { return fixed_size; }

  static type to_uint(type t) { return xdr_reinterpret<uint_type>(t); }
  static type from_uint(uint_type u) { return xdr_reinterpret<type>(u); }
};
template<> struct xdr_traits<float> : xdr_fp_base<float, std::uint32_t> {};
template<> struct xdr_traits<double> : xdr_fp_base<double, std::uint64_t> {};


template<> struct xdr_traits<bool> : xdr_integral_base<bool, std::uint32_t> {
  static constexpr bool is_enum = true;
  static constexpr bool is_numeric = false;
  static type from_uint(uint_type u) { return u; }
  static constexpr const char *enum_name(uint32_t b) {
    return b == 0 ? "FALSE" : b == 1 ? "TRUE" : nullptr;
  }
};

//! Maximum length of vectors.  (The RFC says 0xffffffff, but out of
//! paranoia for integer overflows we chose something that still fits
//! in 32 bits when rounded up to a multiple of four.)
static constexpr uint32_t XDR_MAX_LEN = 0xfffffffc;

template<typename T, bool variable,
	 bool VFixed = xdr_traits<typename T::value_type>::has_fixed_size>
struct xdr_container_base : xdr_traits_base {
  using value_type = typename T::value_type;
  static constexpr bool is_container = true;
  static constexpr bool variable_length = variable;
  static constexpr bool has_fixed_size = false;

  template<typename Archive> static void save(Archive &a, const T &t) {
    if (variable)
      archive(a, nullptr, uint32_t(t.size()));
    for (const value_type &v : t)
      archive(a, nullptr, v);
  }
  template<typename Archive> static void load(Archive &a, T &t) {
    uint32_t n;
    if (variable) {
      archive(a, nullptr, n);
      t.check_size(n);
      if (t.size() > n)
	t.resize(n);
    }
    else
      n = t.size();
    for (uint32_t i = 0; i < n; ++i)
      archive(a, nullptr, t.extend_at(i));
  }
  static std::size_t serial_size(const T &t) {
    std::size_t s = variable ? 4 : 0;
    for (const value_type &v : t)
      s += xdr_traits<value_type>::serial_size(v);
    return s;
  }
};

template<typename T> struct xdr_container_base<T, true, true>
  : xdr_container_base<T, true, false> {
  static std::size_t serial_size(const T &t) {
    return 4 + t.size() * xdr_traits<typename T::value_type>::fixed_size;
  }
};

template<typename T> struct xdr_container_base<T, false, true>
  : xdr_container_base<T, false, false> {
  static constexpr bool has_fixed_size = true;
  static constexpr std::size_t fixed_size =
    T::size() * xdr_traits<typename T::value_type>::fixed_size;
  static std::size_t serial_size(const T &) { return fixed_size; }
};


//! XDR arrays are implemented using std::array as a supertype.
template<typename T, uint32_t N> struct xarray
  : std::array<T, size_t(N)> {
  using array = std::array<T, size_t(N)>;
  using array::array;
  static constexpr std::size_t size() { return N; }
  static void validate() {}
  static void check_size(uint32_t i) {
    if (i != N)
      throw xdr_overflow("invalid size in xdr::xarray");
  }
  static void resize(uint32_t i) {
    if (i != N)
      throw xdr_overflow("invalid resize in xdr::xarray");
  }
  T &extend_at(uint32_t i) {
    if (i >= N)
      throw xdr_overflow("attempt to access invalid position in xdr::xarray");
    return (*this)[i];
  }
};

template<typename T, uint32_t N>
struct xdr_traits<xarray<T,N>> : xdr_container_base<xarray<T,N>, false> {
  static constexpr bool has_fixed_size = xdr_traits<T>::has_fixed_size;
  static constexpr size_t fixed_size = N * xdr_traits<T>::fixed_size;
};

//! XDR \c opaque is represented as std::uint8_t;
template<uint32_t N = XDR_MAX_LEN> using opaque_array = xarray<std::uint8_t,N>;
template<uint32_t N> struct xdr_traits<opaque_array<N>> : xdr_traits_base {
  static constexpr bool is_bytes = true;
  static constexpr std::size_t has_fixed_size = true;
  static constexpr std::size_t fixed_size =
    (std::size_t(N) + std::size_t(3)) & ~std::size_t(3);
  static std::size_t serial_size(const opaque_array<N> &) { return fixed_size; }
  static constexpr bool variable_length = false;
};


//! A vector with a maximum size (returned by xvector::max_size()).
//! Note that you can exceed the size, but an error will happen when
//! marshaling or unmarshaling the data structure.
template<typename T, uint32_t N = XDR_MAX_LEN>
struct xvector : std::vector<T> {
  using vector = std::vector<T>;
  using vector::vector;

  //! Return the maximum size allowed by the type.
  static constexpr uint32_t max_size() { return N; }

  //! Check whether a size is in bounds
  static void check_size(size_t n) {
    if (n > max_size())
      throw xdr_overflow("xvector overflow");
  }

  void append(const T *elems, std::size_t n) {
    check_size(this->size() + n);
    this->insert(this->end(), elems, elems + n);
  }
  T &extend_at(uint32_t i) {
    if (i >= N)
      throw xdr_overflow("attempt to access invalid position in xdr::xvector");
    if (i == this->size())
      this->emplace_back();
    return (*this)[i];
  }
  void resize(uint32_t n) {
    check_size(n);
    vector::resize(n);
  }
};

template<typename T, uint32_t N> struct xdr_traits<xvector<T,N>>
  : xdr_container_base<xvector<T,N>, true> {};

//! Variable-length opaque data is just a vector of std::uint8_t.
template<uint32_t N = XDR_MAX_LEN> using opaque_vec = xvector<std::uint8_t, N>;
template<uint32_t N> struct xdr_traits<opaque_vec<N>> : xdr_traits_base {
  static constexpr bool is_bytes = true;
  static constexpr bool has_fixed_size = false;;
  static constexpr std::size_t serial_size(const opaque_vec<N> &a) {
    return (std::size_t(a.size()) + std::size_t(7)) & ~std::size_t(3);
  }
  static constexpr bool variable_length = true;
};


//! A string with a maximum length (returned by xstring::max_size()).
//! Note that you can exceed the size, but an error will happen when
//! marshaling or unmarshaling the data structure.
template<uint32_t N = XDR_MAX_LEN> struct xstring : std::string {
  using string = std::string;

  //! Return the maximum size allowed by the type.
  static constexpr uint32_t max_size() { return N; }

  //! Check whether a size is in bounds
  static void check_size(size_t n) {
    if (n > max_size())
      throw xdr_overflow("xvector overflow");
  }

  //! Check that the string length is not greater than the maximum
  //! size.  \throws std::out_of_range and clears the contents of the
  //! string if it is too long.
  void validate() const { check_size(size()); }

  xstring() = default;
  xstring(const xstring &) = default;
  xstring(xstring &&) = default;
  xstring &operator=(const xstring &) = default;
  xstring &operator=(xstring &&) = default;

  template<typename...Args> xstring(Args&&...args)
    : string(std::forward<Args>(args)...) { validate(); }

  using string::data;
  char *data() { return &(*this)[0]; } // protobufs does this, so probably ok

//! \hideinitializer
#define ASSIGN_LIKE(method)					\
  template<typename...Args> xstring &method(Args&&...args) {	\
    string::method(std::forward<Args>(args)...);		\
    validate();							\
    return *this;						\
  }
  ASSIGN_LIKE(operator=)
  ASSIGN_LIKE(operator+=)
  ASSIGN_LIKE(append)
  ASSIGN_LIKE(push_back)
  ASSIGN_LIKE(assign)
  ASSIGN_LIKE(insert)
  ASSIGN_LIKE(replace)
  ASSIGN_LIKE(swap)
#undef ASSIGN_LIKE
};

template<uint32_t N> struct xdr_traits<xstring<N>> : xdr_traits_base {
  static constexpr bool is_bytes = true;
  static constexpr bool has_fixed_size = false;;
  static constexpr std::size_t serial_size(const xstring<N> &a) {
    return (std::size_t(a.size()) + std::size_t(7)) & ~std::size_t(3);
  }
  static constexpr bool variable_length = true;
};


//! Optional data (represented with pointer notation in XDR source).
template<typename T> struct pointer : std::unique_ptr<T> {
  using value_type = T;
  using std::unique_ptr<T>::unique_ptr;
  using std::unique_ptr<T>::get;
  static void check_size(uint32_t n) {
    if (n > 1)
      throw xdr_overflow("xdr::pointer size must be 0 or 1");
  }
  uint32_t size() const { return *this ? 1 : 0; }
  T *begin() { return get(); }
  const T *begin() const { return get(); }
  T *end() { return begin() + size(); }
  const T *end() const { return begin() + size(); }
  T &extend_at(uint32_t i) {
    if (i != 0)
      throw xdr_overflow("attempt to access position > 0 in xdr::pointer");
    if (!size())
      this->reset(new T);
    return **this;
  }
  void resize(uint32_t n) {
    if (n == size())
      return;
    switch(n) {
    case 0:
      this->reset();
      break;
    case 1:
      this->reset(new T);
      break;
    default:
      throw xdr_overflow("xdr::pointer::resize: valid sizes are 0 and 1");
    }
  }
  T &activate() {
    if (!*this)
      this->reset(new T);
    return *this->get();
  }
};

template<typename T> struct xdr_traits<pointer<T>>
  : xdr_container_base<pointer<T>, true> {};


//! Type-level representation of a pointer-to-member value.
template<typename T, typename F, F T::*Ptr> struct field_ptr {
  using class_type = T;
  using field_type = F;
  using value_type = F T::*;
  static constexpr value_type value = Ptr;
  F &operator()(T &t) const { return t.*value; }
  const F &operator()(const T &t) const { return t.*value; }
  F &operator()(T &&t) const { return std::move(t.*value); }
};

template<typename ...Fields> struct xdr_struct_base;

template<typename FP, typename ...Fields>
  struct _xdr_struct_base_fs : xdr_struct_base<Fields...> {
  static constexpr bool has_fixed_size = true;
  static constexpr std::size_t fixed_size =
    (xdr_traits<typename FP::field_type>::fixed_size
     + xdr_struct_base<Fields...>::fixed_size);
  static constexpr std::size_t serial_size(const typename FP::class_type &) {
    return fixed_size;
  }
};
template<typename FP, typename ...Fields>
  struct _xdr_struct_base_vs : xdr_struct_base<Fields...> {
  static constexpr bool has_fixed_size = false;
  static std::size_t serial_size(const typename FP::class_type &t) {
    return (xdr_size(t.*(FP::value))
	    + xdr_struct_base<Fields...>::serial_size(t));
  }
};

template<> struct xdr_struct_base<> : xdr_traits_base {
  static constexpr bool is_class = true;
  static constexpr bool has_fixed_size = true;
  static constexpr std::size_t fixed_size = 0;
  template<typename T> static constexpr std::size_t serial_size(const T&) {
    return fixed_size;
  }
};
template<typename FP, typename ...Rest> struct xdr_struct_base<FP, Rest...>
  : std::conditional<(xdr_traits<typename FP::field_type>::has_fixed_size
		      && xdr_struct_base<Rest...>::has_fixed_size),
                     _xdr_struct_base_fs<FP, Rest...>,
                     _xdr_struct_base_vs<FP, Rest...>>::type {};

//! Dereference a pointer to a member of type \c F of a class of type
//! \c T, preserving reference types.  Hence applying to an lvalue
//! reference \c T returns an lvalue reference \c F, while an rvalue
//! reference \c T produces an rvalue reference \c F.
template<typename T, typename F> inline F &
member(T &t, F T::*mp)
{
  return t.*mp;
}
template<typename T, typename F> inline const F &
member(const T &t, F T::*mp)
{
  return t.*mp;
}
template<typename T, typename F> inline F &&
member(T &&t, F T::*mp)
{
  return std::move(t.*mp);
}

struct field_constructor_t {
  constexpr field_constructor_t() {}
  template<typename T, typename F> void operator()(F T::*mp, T &t) const {
    new (&(t.*mp)) F;
  }
  template<typename T, typename F, typename TT> void
  operator()(F T::*mp, T &t, TT &&tt) const {
    new (&(t.*mp)) F (member(std::forward<TT>(tt), mp));
  }
};
constexpr field_constructor_t field_constructor;

struct field_destructor_t {
  constexpr field_destructor_t() {}
  template<typename T, typename F> void
  operator()(F T::*mp, T &t) const { member(t, mp).~F(); }
};
constexpr field_destructor_t field_destructor;

struct field_assigner_t {
  constexpr field_assigner_t() {}
  template<typename T, typename F, typename TT> void
  operator()(F T::*mp, T &t, TT &&tt) const {
    member(t, mp) = member(std::forward<TT>(tt), mp);
  }
};
constexpr field_assigner_t field_assigner;

struct field_archiver_t {
  constexpr field_archiver_t() {}

  template<typename F, typename T, typename Archive> void
  operator()(F T::*mp, Archive &ar, T &t, const char *name) const {
    archive(ar, name, member(t, mp));
  }
  template<typename F, typename T, typename Archive> void
  operator()(F T::*mp, Archive &ar, const T &t, const char *name) const {
    archive(ar, name, member(t, mp));
  }
};
constexpr field_archiver_t field_archiver;

struct field_size_t {
  constexpr field_size_t() {}
  template<typename F, typename T> void
  operator()(F T::*mp, const T &t, std::size_t &size) const {
    size = xdr_traits<F>::serial_size(member(t, mp));
  }
};
constexpr field_size_t field_size;

}

#endif // !_XDRC_TYPES_H_HEADER_INCLUDED_

