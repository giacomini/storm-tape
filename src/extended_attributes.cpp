// Copyright 2018,2023 Istituto Nazionale di Fisica Nucleare
//
// SPDX-License-Identifier: EUPL-1.2

#include "extended_attributes.hpp"

#include <boost/algorithm/string/split.hpp>
#include <algorithm>
#include <cassert>

namespace storm {

bool XAttrName::valid() const
{
  static std::regex const re{R"(^(user|system|security|trusted)\..+)"};
  return std::regex_match(name_, re);
}

std::ostream& operator<<(std::ostream& os, XAttrName const& n)
{
  return os << n.value();
}

void create_xattr(fs::path const& path, XAttrName const& name,
                  std::error_code& ec)
{
  assert(name.valid());
  std::string const empty;
  auto res = ::setxattr(path.c_str(), name.c_str(), empty.data(), empty.size(),
                        XATTR_CREATE);
  if (res == 0 || errno == EEXIST) {
    ec.clear();
  } else {
    // std::system_category() should be preferred to std::generic_category()
    ec.assign(errno, std::generic_category());
  }
}

void set_xattr(fs::path const& path, XAttrName const& name,
               XAttrValue const& value, std::error_code& ec)
{
  assert(name.valid());
  auto res =
      ::setxattr(path.c_str(), name.c_str(), value.data(), value.size(), 0);
  if (res == 0) {
    ec.clear();
  } else {
    // std::system_category() should be preferred to std::generic_category()
    ec.assign(errno, std::generic_category());
  }
}

void set_xattr(fs::path const& path, XAttrName const& name,
               XAttrValue const& value)
{
  std::error_code ec;
  set_xattr(path, name, value, ec);
  if (ec) {
    throw std::system_error(ec);
  }
}

XAttrValue get_xattr(fs::path const& path, XAttrName const& name,
                     std::error_code& ec)
{
  assert(name.valid());

  std::string value;
  value.resize(value.capacity()); // try to stay in the SSO buffer

  auto res = ::getxattr(path.c_str(), name.c_str(), value.data(), value.size());

  if (res >= 0) {
    value.resize(static_cast<std::size_t>(res));
  } else if (errno == ERANGE) {
    // query the actual size of the attribute value
    auto size = ::getxattr(path.c_str(), name.c_str(), value.data(), 0);
    value.resize(static_cast<std::size_t>(size));
    res = ::getxattr(path.c_str(), name.c_str(), value.data(), value.size());
  }

  if (res >= 0) {
    ec.clear();
    return XAttrValue{value};
  } else {
    ec.assign(errno, std::generic_category());
    return XAttrValue{};
  }
}

XAttrValue get_xattr(fs::path const& path, XAttrName const& name)
{
  std::error_code ec;
  auto result = get_xattr(path, name, ec); //-V821

  if (ec) {
    throw std::system_error{ec};
  } else {
    return result;
  }
}

bool has_xattr(fs::path const& path, XAttrName const& name, std::error_code& ec)
{
  assert(name.valid());

  auto res = ::getxattr(path.c_str(), name.c_str(), nullptr, 0);

  if (res >= 0) {
    ec.clear();
    return true;
  } else if (errno == ENOATTR) {
    ec.clear();
    return false;
  } else {
    ec.assign(errno, std::generic_category());
    return false;
  }
}

bool has_xattr(fs::path const& path, XAttrName const& name)
{
  std::error_code ec;
  auto result = has_xattr(path, name, ec);

  if (ec) {
    throw std::system_error{ec};
  } else {
    return result;
  }
}

XAttrNames list_xattr_names(fs::path const& path, std::error_code& ec)
{
  XAttrNames result;

  std::string list;
  auto size = ::listxattr(path.c_str(), list.data(), 0);
  if (size < 0) {
    ec.assign(errno, std::generic_category());
    return result;
  }

  auto s = static_cast<std::string::size_type>(size);
  list.resize(s);
  if (auto res = ::listxattr(path.c_str(), list.data(), list.size()); res < 0) {
    ec.assign(errno, std::generic_category());
    return result;
  }

  boost::split(
      result, list, [](char c) { return c == '\0'; }, boost::token_compress_on);

  std::erase_if(result,
                [](XAttrName const& name) { return name.value().empty(); });

  ec.clear();
  return result;
}

void remove_xattr(fs::path const& path, XAttrName const& name,
                  std::error_code& ec)
{
  assert(name.valid());

  auto res = ::removexattr(path.c_str(), name.c_str());

  if (res == 0) {
    ec.clear();
  } else {
    ec.assign(errno, std::generic_category());
  }
}

void remove_xattr(fs::path const& path, XAttrName const& name)
{
  std::error_code ec;
  remove_xattr(path, name, ec);
  if (ec) {
    throw std::system_error(ec);
  }
}

} // namespace storm