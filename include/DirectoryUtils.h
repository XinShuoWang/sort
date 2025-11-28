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
  static bool createDir(const std::filesystem::path &path, std::error_code &ec);

  static bool createDir(const std::filesystem::path &path);

  static bool remove(const std::filesystem::path &path,
                     std::error_code &ec) noexcept;

  static bool remove(const std::filesystem::path &path);

  static uintmax_t removeAll(const std::filesystem::path &path,
                             std::error_code &ec) noexcept;

  static uintmax_t removeAll(const std::filesystem::path &path);

  static bool exists(const std::filesystem::path &path,
                     std::error_code &ec) noexcept;

  static bool exists(const std::filesystem::path &path);

  static bool isDir(const std::filesystem::path &path,
                    std::error_code &ec) noexcept;

  static bool isDir(const std::filesystem::path &path);

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
  getDirInfo(const std::filesystem::path &path, std::error_code &ec) noexcept;
  static DirectoryInfo getDirInfo(const std::filesystem::path &path);

  static bool copy(const std::filesystem::path &from,
                   const std::filesystem::path &to, std::error_code &ec);

  static bool copy(const std::filesystem::path &from,
                   const std::filesystem::path &to);

  static bool move(const std::filesystem::path &from,
                   const std::filesystem::path &to,
                   std::error_code &ec) noexcept;

  static bool move(const std::filesystem::path &from,
                   const std::filesystem::path &to);

  static std::optional<uintmax_t>
  getDiskUsage(const std::filesystem::path &path, std::error_code &ec) noexcept;

  static uintmax_t getDiskUsage(const std::filesystem::path &path);
};
