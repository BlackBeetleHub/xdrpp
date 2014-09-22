// -*- C++ -*-

/** \file cereal.h Interface for cereal serealization back ends.  By
 * including this file, you can archive any XDR data structure with
 * cereal. */

#ifndef _XDRC_CEREAL_H_HEADER_INCLUDED_
#define _XDRC_CEREAL_H_HEADER_INCLUDED_ 1

#include <type_traits>
#include <cereal/cereal.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/details/traits.hpp>
#include <xdrc/types.h>

namespace cereal {
class JSONInputArchive;
class JSONOutputArchive;
class XMLOutputArchive;
class XMLInputArchive;
}

namespace xdr {

template<typename Archive, typename T> typename
std::enable_if<xdr_class<T>::value>::type
save(Archive &ar, const T &t)
{
  xdr_recursive<T>::save(ar, t);
}

template<typename Archive, typename T> typename
std::enable_if<xdr_class<T>::value>::type
load(Archive &ar, T &t)
{
  xdr_recursive<T>::load(ar, t);
}

template<typename Archive, typename T> typename
std::enable_if<cereal::traits::is_output_serializable<
		 cereal::BinaryData<char *>,Archive>::value
               && xdr_bytes<T>::value>::type
save(Archive &ar, const T &t)
{
  if (xdr_bytes<T>::variable)
    ar(cereal::make_size_tag(static_cast<cereal::size_type>(t.size())));
  ar(cereal::binary_data(const_cast<char *>(
         reinterpret_cast<const char *>(t.data())), t.size()));
}

template<typename Archive, typename T> typename
std::enable_if<cereal::traits::is_input_serializable<
		 cereal::BinaryData<char *>,Archive>::value
               && xdr_bytes<T>::value>::type
load(Archive &ar, T &t)
{
  cereal::size_type size;
  if (xdr_bytes<T>::variable)
    ar(cereal::make_size_tag(size));
  else
    size = t.size();
  t.check_size(size);
  t.resize(static_cast<std::uint32_t>(size));
  ar(cereal::binary_data(t.data(), size));
}


template<typename Archive> struct nvp_adapter {
  template<typename T> static void
  apply(Archive &ar, const char *field, T &&t) {
    if (field)
      ar(cereal::make_nvp(field, std::forward<T>(t)));
    else
      ar(std::forward<T>(t));
  }

  template<uint32_t N> static void
  apply(Archive &ar, const char *field, xstring<N> &s) {
    apply(ar, field, static_cast<std::string &>(s));
  }
  template<uint32_t N> static void
  apply(Archive &ar, const char *field, const xstring<N> &s) {
    apply(ar, field, static_cast<const std::string &>(s));
  }
};

//! \hideinitializer
#define CEREAL_ARCHIVE_TAKES_NAME(archive)		\
template<> struct archive_adapter<cereal::archive>	\
  : nvp_adapter<cereal::archive> {}
CEREAL_ARCHIVE_TAKES_NAME(JSONInputArchive);
CEREAL_ARCHIVE_TAKES_NAME(JSONOutputArchive);
CEREAL_ARCHIVE_TAKES_NAME(XMLOutputArchive);
CEREAL_ARCHIVE_TAKES_NAME(XMLInputArchive);
#undef CEREAL_ARCHIVE_TAKES_NAME


}

namespace cereal {
using xdr::load;
using xdr::save;
}

#endif // !_XDRC_CEREAL_H_HEADER_INCLUDED_
