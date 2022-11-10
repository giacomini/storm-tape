#ifndef FS_STORAGE_HPP
#define FS_STORAGE_HPP

#include <filesystem>
#include <fstream>
#include <iostream>
#include <ostream>

namespace storm {
class FsStorageTest
{
 public:
  static void SetUp()
  {
    std::filesystem::create_directories("/tmp/foo/bar");
    std::filesystem::create_directories("/tmp/bar/foo");
    std::filesystem::create_directories("/tmp/not/foo/bar");
    std::ofstream ofs("/tmp/foo/bar.txt");
    std::ofstream ofs1("/tmp/bar/foo.txt");
    std::ofstream ofs2("/tmp/not/foo/bar.txt");
  }
  static void TearDown()
  {
    try {
      std::filesystem::remove_all("/tmp/foo");
      std::filesystem::remove_all("/tmp/bar");
    } catch (std::filesystem::filesystem_error& e) {
      std::cerr << "fs_storage testing: " << e.what() << '\n';
    }
  }
};

} // namespace storm

#endif