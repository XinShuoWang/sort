#pragma once

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

class DirectoryUtils {
public:
  static bool createDir(const std::filesystem::path &path,
                        std::error_code &ec) {
    if (exists(path)) {
      ec.clear();
      return isDir(path, ec);
    }
    return std::filesystem::create_directories(path, ec);
  }

  static bool createDir(const std::filesystem::path &path) {
    std::error_code ec;
    bool result = createDir(path, ec);
    if (ec) {
      throw std::filesystem::filesystem_error("Create dir failed.", path, ec);
    }
    return result;
  }

  static bool remove(const std::filesystem::path &path,
                     std::error_code &ec) noexcept {
    if (!exists(path, ec)) {
      ec.clear();
      return true;
    }
    if (!isDir(path, ec)) {
      ec = std::make_error_code(std::errc::not_a_directory);
      return false;
    }
    return std::filesystem::remove(path, ec);
  }

  static bool remove(const std::filesystem::path &path) {
    std::error_code ec;
    bool result = remove(path, ec);
    if (ec) {
      throw std::filesystem::filesystem_error("remove dir failed.", path, ec);
    }
    return result;
  }

  static uintmax_t removeAll(const std::filesystem::path &path,
                             std::error_code &ec) noexcept {
    if (!exists(path, ec)) {
      ec.clear();
      return 0;
    }
    if (!isDir(path, ec)) {
      ec = std::make_error_code(std::errc::not_a_directory);
      return 0;
    }
    return std::filesystem::remove_all(path, ec);
  }

  static uintmax_t removeAll(const std::filesystem::path &path) {
    std::error_code ec;
    uintmax_t result = removeAll(path, ec);
    if (ec) {
      throw std::filesystem::filesystem_error("recursive delete dir failed.",
                                              path, ec);
    }
    return result;
  }

  static bool exists(const std::filesystem::path &path,
                     std::error_code &ec) noexcept {
    return std::filesystem::exists(path, ec);
  }

  static bool exists(const std::filesystem::path &path) {
    std::error_code ec;
    bool result = exists(path, ec);
    if (ec) {
      throw std::filesystem::filesystem_error("pathExists check failed.", path,
                                              ec);
    }
    return result;
  }

  static bool isDir(const std::filesystem::path &path,
                    std::error_code &ec) noexcept {
    return std::filesystem::is_directory(path, ec);
  }

  static bool isDir(const std::filesystem::path &path) {
    std::error_code ec;
    bool result = isDir(path, ec);
    if (ec) {
      throw std::filesystem::filesystem_error("isDir check failed.", path, ec);
    }
    return result;
  }

  struct DirectoryInfo {
    std::filesystem::path path;
    uintmax_t file_count = 0;
    uintmax_t directory_count = 0;
    uintmax_t total_size = 0;
    std::filesystem::file_time_type last_modified;

    std::string to_string() const {
      std::ostringstream oss;
      oss << "Path: " << path.string() << "\n"
          << "Number of file: " << file_count << "\n"
          << "Number of sub-dir: " << directory_count << "\n"
          << "Total size: " << format_size(total_size) << "\n"
          << "Last modified: " << format_time(last_modified);
      return oss.str();
    }

  private:
    static std::string format_size(uintmax_t bytes) {
      const char *units[] = {"B", "KB", "MB", "GB", "TB"};
      double size = static_cast<double>(bytes);
      int unit = 0;

      while (size >= 1024 && unit < 4) {
        size /= 1024;
        unit++;
      }

      std::ostringstream oss;
      oss << std::fixed << std::setprecision(2) << size << " " << units[unit];
      return oss.str();
    }

    static std::string format_time(std::filesystem::file_time_type time) {
      auto sctp =
          std::chrono::time_point_cast<std::chrono::system_clock::duration>(
              time - std::filesystem::file_time_type::clock::now() +
              std::chrono::system_clock::now());
      std::time_t tt = std::chrono::system_clock::to_time_t(sctp);
      std::tm tm = *std::localtime(&tt);

      std::ostringstream oss;
      oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
      return oss.str();
    }
  };

public:
  static std::optional<DirectoryInfo>
  getDirInfo(const std::filesystem::path &path, std::error_code &ec) noexcept {
    if (!exists(path, ec) || !isDir(path, ec)) {
      return std::nullopt;
    }

    DirectoryInfo info;
    info.path = std::filesystem::canonical(path, ec);
    if (ec)
      return std::nullopt;

    info.last_modified = std::filesystem::last_write_time(path, ec);
    if (ec)
      return std::nullopt;

    for (const auto &entry : std::filesystem::recursive_directory_iterator(
             path, std::filesystem::directory_options::skip_permission_denied,
             ec)) {

      if (ec)
        break;

      if (entry.is_regular_file(ec)) {
        info.file_count++;
        info.total_size += entry.file_size(ec);
      } else if (entry.is_directory(ec)) {
        info.directory_count++;
      }
    }

    return ec ? std::nullopt : std::optional<DirectoryInfo>(info);
  }

  static DirectoryInfo getDirInfo(const std::filesystem::path &path) {
    std::error_code ec;
    auto info = getDirInfo(path, ec);
    if (ec || !info) {
      throw std::filesystem::filesystem_error("获取目录信息失败", path, ec);
    }
    return *info;
  }

  static bool copy(const std::filesystem::path &from,
                   const std::filesystem::path &to, std::error_code &ec) {
    if (!exists(from, ec) || !isDir(from, ec)) {
      return false;
    }

    createDir(to, ec); // ensure dir exists
    if (ec)
      return false;

    for (const auto &entry : std::filesystem::directory_iterator(from, ec)) {
      if (ec)
        return false;

      std::filesystem::path target = to / entry.path().filename();
      if (entry.is_directory(ec)) {
        if (!copy(entry.path(), target, ec)) {
          return false;
        }
      } else {
        std::filesystem::copy(entry.path(), target,
                              std::filesystem::copy_options::overwrite_existing,
                              ec);
        if (ec)
          return false;
      }
    }

    return true;
  }

  static bool copy(const std::filesystem::path &from,
                   const std::filesystem::path &to) {
    std::error_code ec;
    bool result = copy(from, to, ec);
    if (ec) {
      throw std::filesystem::filesystem_error("Copy dir failed.", from, to, ec);
    }
    return result;
  }

  static bool move(const std::filesystem::path &from,
                   const std::filesystem::path &to,
                   std::error_code &ec) noexcept {
    if (!exists(from, ec) || !isDir(from, ec)) {
      return false;
    }

    std::filesystem::rename(from, to, ec);
    return !ec;
  }

  static bool move(const std::filesystem::path &from,
                   const std::filesystem::path &to) {
    std::error_code ec;
    bool result = move(from, to, ec);
    if (ec) {
      throw std::filesystem::filesystem_error("move dr failed.", from, to, ec);
    }
    return result;
  }

  static std::optional<uintmax_t>
  getDiskUsage(const std::filesystem::path &path,
               std::error_code &ec) noexcept {
    auto info = getDirInfo(path, ec);
    return info ? std::optional<uintmax_t>(info->total_size) : std::nullopt;
  }

  static uintmax_t getDiskUsage(const std::filesystem::path &path) {
    auto info = getDirInfo(path);
    return info.total_size;
  }
};
