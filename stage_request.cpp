#include "stage_request.hpp"
#include <algorithm>

storm::StageRequest::StageRequest(std::vector<storm::File> files)
    : created_at(Clock::now())
    , started_at(created_at)
    , m_files(std::move(files))
{}

std::vector<storm::File> storm::StageRequest::getFiles() const
{
  return m_files;
}

void storm::StageRequest::setFiles(std::vector<storm::File> fle)
{
  m_files = fle;
}