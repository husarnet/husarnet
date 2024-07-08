// Copyright (c) 2024 Husarnet sp. z o.o.
// Authors: listed in project_root/README.md
// License: specified in project_root/LICENSE.txt

#include "husarnet/ports/fat/filesystem.h"

#include <cstdlib>
#include <filesystem>

#include <fcntl.h>
#include <linux/fs.h>
#include <syscall.h>
#include <unistd.h>

#include "husarnet/logging.h"
#include "husarnet/util.h"

#define RENAME_EXCHANGE (1 << 1)

static bool transformTmpFile(
    const std::string& path,
    std::function<std::string(const std::string&)> transform)
{
  // This path has to be on the same mountpoint as the original file for the
  // rename to work, hence we're adding a suffix
  std::string tmpPath = path + ".tmp";

  // We assume other writers as as nice as we are and won't leave this file
  // mid-written for a single read operation
  auto oldContent = readFile(path);

  if(!oldContent) {
    LOG_DEBUG("Failed to read %s", path.c_str());
    return false;
  }

  std::string newContent = transform(oldContent.value());

  // We assume that there's no traffic on a temporary file
  if(!writeFile(tmpPath, newContent)) {
    LOG_DEBUG("Failed to write to a temporary file %s", tmpPath.c_str());
    return false;
  }

  // RENAME_EXCHANGE makes this an atomic operation
  // Using syscall directly as musl has not yet added a wrapper for this
  if(syscall(
         SYS_renameat2, 0, tmpPath.c_str(), 0, path.c_str(), RENAME_EXCHANGE) !=
     0) {
    LOG_DEBUG("Failed to rename %s to %s", tmpPath.c_str(), path.c_str());
    return false;
  }

  // This is a best effort operation, as in the worst case tmp file can stay
  std::filesystem::remove(tmpPath);

  LOG_DEBUG("Transformed %s using temporary file", path.c_str());

  return true;
}

static bool transformLockFile(
    const std::string& path,
    std::function<std::string(const std::string&)> transform)
{
  std::string oldContent, newContent;

  int ret;
  int fd = open(path.c_str(), O_RDWR);
  if(fd < 0) {
    LOG_ERROR("Failed to open %s", path.c_str());
    return false;
  }

  // This will break `open` calls for other processes for this file hence we
  // need to be super-conscious about releasing this lock This will also make us
  // receive SIGIO for such events that we're expected to handle. In this case
  // though, our goal is to release such lock as soon as possible either way, so
  // we won't be handling it explicitly
  if(fcntl(fd, F_SETLEASE, F_WRLCK) != 0) {
    LOG_ERROR("Failed to take a write lock on %s", path.c_str());
    close(fd);
    return false;
  }

  // Generic C-style file reading to an expanding buffer
  int bufferSize = 32 * 1024;
  char* buffer = (char*)malloc(bufferSize);
  char* ptr = buffer;

  do {
    // -1 for null terminator
    size_t sizeLeft = bufferSize - 1 - (ptr - buffer);

    if(sizeLeft <= 1024) {
      bufferSize *= 2;
      buffer = (char*)realloc(buffer, bufferSize);
    }

    ssize_t bytesRead = read(fd, ptr, sizeLeft);

    if(bytesRead < 0) {
      LOG_ERROR("Failed to read from %s", path.c_str());
      goto abort;
    }
    ptr += bytesRead;

    // EOF indicator
    if(bytesRead == 0) {
      *ptr = 0;  // so it's a valid string
      break;
    }
  } while(true);

  // C to C++ conversion and transformation
  oldContent = std::string(buffer);
  newContent = transform(oldContent);

  // Generic C-style file replacement
  if(lseek(fd, 0, SEEK_SET) != 0) {
    LOG_ERROR("Failed to seek to the beginning of %s", path.c_str());
    goto abort;
  }

  if(write(fd, newContent.c_str(), newContent.size()) != newContent.size()) {
    LOG_ERROR("Failed to write to %s", path.c_str());
    goto abort;
  }

  if(ftruncate(fd, newContent.size()) != 0) {
    LOG_ERROR("Failed to truncate %s", path.c_str());
    goto abort;
  }

  // Finally we can release the lock and file
  ret = fcntl(fd, F_SETLEASE, F_UNLCK);
  LOG_NEGATIVE(ret, "Failed to release a write lock on %s", path.c_str());

  ret = fsync(fd);
  LOG_NEGATIVE(ret, "Failed to sync %s", path.c_str());

  ret = close(fd);
  LOG_NEGATIVE(ret, "Failed to close %s", path.c_str());

  LOG_DEBUG("Transformed %s using lock file", path.c_str());

  return true;

abort:
  ret = fcntl(fd, F_SETLEASE, F_UNLCK);
  LOG_NEGATIVE(ret, "Failed to release a write lock on %s", path.c_str());
  ret = close(fd);
  LOG_NEGATIVE(ret, "Failed to close %s", path.c_str());

  return false;
}

bool transformFile(
    const std::string& path,
    std::function<std::string(const std::string&)> transform)
{
  LOG_DEBUG("Transforming %s", path.c_str());

  // Simplest case - file does not exist so we can assume nobody's waiting on it
  // and do a naive write
  if(!isFile(path)) {
    return writeFile(path, transform(""));
  }

  // Method with temporary file and an atomic rename is preferred but can fail
  // in some cases (i.e. when container mounts it specifically)
  if(transformTmpFile(path, transform)) {
    return true;
  }

  // This is a complex method using file locks
  if(transformLockFile(path, transform)) {
    return true;
  }

  auto contents = readFile(path);
  if(!contents) {
    return false;
  }

  // This is the most basic method as a fallback
  return writeFile(path, transform(contents.value()));
}
