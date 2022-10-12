#include "stage_request.hpp"
#include <algorithm>

storm::StageRequest::StageRequest(std::vector<storm::File> files)
    : m_created_at(std::chrono::system_clock::now())
    , m_started_at(m_created_at)
    , m_files(std::move(files))
{}

std::vector<storm::File> const& storm::StageRequest::files() const
{
  return m_files;
}

std::vector<storm::File>& storm::StageRequest::files()
{
  return m_files;
}

std::chrono::system_clock::time_point const&
storm::StageRequest::created_at() const
{
  return m_created_at;
}

std::chrono::system_clock::time_point const&
storm::StageRequest::started_at() const
{
  return m_started_at;
}