// Copyright 2018,2023 Istituto Nazionale di Fisica Nucleare
//
// SPDX-License-Identifier: EUPL-1.2

#ifndef STORM_EXTENDED_ATTRIBUTES_HPP
#define STORM_EXTENDED_ATTRIBUTES_HPP

#include <sys/types.h>

#include "types.hpp"
#include <attr/xattr.h>
#include <iosfwd>
#include <regex>
#include <string>
#include <vector>

namespace storm {

class XAttrName
{
  std::string name_;

 public:
  XAttrName() = default;
  explicit XAttrName(std::string n)
      : name_{std::move(n)}
  {}
  template<class InputIterator>
  XAttrName(InputIterator first, InputIterator last)
      : name_(first, last)
  {}
  std::string const& value() const noexcept
  {
    return name_;
  }
  char const* c_str() const noexcept
  {
    return name_.c_str();
  }
  bool valid() const
  {
    static std::regex const re{R"(^(user|system|security|trusted)\..+)"};
    return std::regex_match(name_, re);
  }
};

using XAttrNames = std::vector<XAttrName>;

class XAttrValue
{
  // only strings for the moment, but it should be std::vector<std::byte>
  using Data = std::string;
  Data value_;

 public:
  XAttrValue() = default;
  explicit XAttrValue(Data v)
      : value_(std::move(v))
  {}
  Data const& value() const noexcept
  {
    return value_;
  }
  auto data() noexcept
  {
    return value_.data();
  }
  auto data() const noexcept
  {
    return value_.data();
  }
  auto size() const noexcept
  {
    return value_.size();
  }
};

class XAttr
{
  XAttrName name_;
  XAttrValue value_;

 public:
  XAttr() = default;
  XAttr(XAttrName n, XAttrValue v)
      : name_{std::move(n)}
      , value_{std::move(v)}
  {}
  XAttrName const& name() const noexcept
  {
    return name_;
  }
  XAttrValue const& value() const noexcept
  {
    return value_;
  }
};

std::ostream& operator<<(std::ostream& os, XAttrName const& n);

void create_xattr(fs::path const& path, XAttrName const& name,
                  std::error_code& ec);

void set_xattr(fs::path const& path, XAttrName const& name,
               XAttrValue const& value, std::error_code& ec);

void set_xattr(fs::path const& path, XAttrName const& name,
               XAttrValue const& value);

XAttrValue get_xattr(fs::path const& path, XAttrName const& name,
                     std::error_code& ec);

XAttrValue get_xattr(fs::path const& path, XAttrName const& name);

bool has_xattr(fs::path const& path, XAttrName const& name,
               std::error_code& ec);

bool has_xattr(fs::path const& path, XAttrName const& name);

XAttrNames list_xattr_names(fs::path const& path, std::error_code& ec);

void remove_xattr(fs::path const& path, XAttrName const& name,
                  std::error_code& ec);

void remove_xattr(fs::path const& path, XAttrName const& name);

} // namespace storm

#endif