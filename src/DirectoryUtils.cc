#include "DirectoryUtils.h"

bool DirectoryUtils::createDir(const std::filesystem::path &path,
                               std::error_code &ec) {
  if (exists(path)) {
    ec.clear();
    return isDir(path, ec);
  }
  return std::filesystem::create_directories(path, ec);
}

bool DirectoryUtils::createDir(const std::filesystem::path &path) {
  std::error_code ec;
  bool result = createDir(path, ec);
  if (ec) {
    throw std::filesystem::filesystem_error("Create dir failed.", path, ec);
  }
  return result;
}

bool DirectoryUtils::remove(const std::filesystem::path &path,
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

bool DirectoryUtils::remove(const std::filesystem::path &path) {
  std::error_code ec;
  bool result = remove(path, ec);
  if (ec) {
    throw std::filesystem::filesystem_error("remove dir failed.", path, ec);
  }
  return result;
}

uintmax_t DirectoryUtils::removeAll(const std::filesystem::path &path,
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

uintmax_t DirectoryUtils::removeAll(const std::filesystem::path &path) {
  std::error_code ec;
  uintmax_t result = removeAll(path, ec);
  if (ec) {
    throw std::filesystem::filesystem_error("recursive delete dir failed.",
                                            path, ec);
  }
  return result;
}

bool DirectoryUtils::exists(const std::filesystem::path &path,
                            std::error_code &ec) noexcept {
  return std::filesystem::exists(path, ec);
}

bool DirectoryUtils::exists(const std::filesystem::path &path) {
  std::error_code ec;
  bool result = exists(path, ec);
  if (ec) {
    throw std::filesystem::filesystem_error("pathExists check failed.", path,
                                            ec);
  }
  return result;
}

bool DirectoryUtils::isDir(const std::filesystem::path &path,
                           std::error_code &ec) noexcept {
  return std::filesystem::is_directory(path, ec);
}

bool DirectoryUtils::isDir(const std::filesystem::path &path) {
  std::error_code ec;
  bool result = isDir(path, ec);
  if (ec) {
    throw std::filesystem::filesystem_error("isDir check failed.", path, ec);
  }
  return result;
}

std::optional<DirectoryUtils::DirectoryInfo>
DirectoryUtils::getDirInfo(const std::filesystem::path &path,
                           std::error_code &ec) noexcept {
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

DirectoryUtils::DirectoryInfo
DirectoryUtils::getDirInfo(const std::filesystem::path &path) {
  std::error_code ec;
  auto info = getDirInfo(path, ec);
  if (ec || !info) {
    throw std::filesystem::filesystem_error("获取目录信息失败", path, ec);
  }
  return *info;
}

bool DirectoryUtils::copy(const std::filesystem::path &from,
                          const std::filesystem::path &to,
                          std::error_code &ec) {
  if (!exists(from, ec) || !isDir(from, ec)) {
    return false;
  }

  createDir(to, ec);
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

bool DirectoryUtils::copy(const std::filesystem::path &from,
                          const std::filesystem::path &to) {
  std::error_code ec;
  bool result = copy(from, to, ec);
  if (ec) {
    throw std::filesystem::filesystem_error("Copy dir failed.", from, to, ec);
  }
  return result;
}

bool DirectoryUtils::move(const std::filesystem::path &from,
                          const std::filesystem::path &to,
                          std::error_code &ec) noexcept {
  if (!exists(from, ec) || !isDir(from, ec)) {
    return false;
  }

  std::filesystem::rename(from, to, ec);
  return !ec;
}

bool DirectoryUtils::move(const std::filesystem::path &from,
                          const std::filesystem::path &to) {
  std::error_code ec;
  bool result = move(from, to, ec);
  if (ec) {
    throw std::filesystem::filesystem_error("move dr failed.", from, to, ec);
  }
  return result;
}

std::optional<uintmax_t>
DirectoryUtils::getDiskUsage(const std::filesystem::path &path,
                             std::error_code &ec) noexcept {
  auto info = getDirInfo(path, ec);
  return info ? std::optional<uintmax_t>(info->total_size) : std::nullopt;
}

uintmax_t DirectoryUtils::getDiskUsage(const std::filesystem::path &path) {
  auto info = getDirInfo(path);
  return info.total_size;
}

