#pragma once

#ifndef MISC_HEADER_ONLY

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace misc {

/// @brief An owning FILE* that closes itself, carrying the right closer
/// (fclose or pclose) for however it was opened
using FILE_uptr = std::unique_ptr<FILE, int (*)(FILE*)>;

/// @brief Opens a file with std::fopen
/// @param filename The path to open
/// @param mode The mode string, as std::fopen takes
/// @return An owning handle, empty if the open failed
FILE_uptr fopen_ptr(const char* filename, const char* mode);

/// @brief Opens files, dispatching on extension to a registered handler.
///
/// Paths with no registered extension fall back to a plain fopen.
class FileOpener {
 protected:
  using Handler =
      std::function<FILE_uptr(const std::filesystem::path&, const char*)>;

  std::unordered_map<std::filesystem::path, Handler> handlers;

 public:
  /// @brief Registers a handler for an extension
  /// @param extension The extension to handle, including the dot
  /// @param func The handler invoked for paths with that extension
  /// @return true if registered, false if the extension was already taken
  bool register_ext_handler(std::string extension, Handler func);

  /// @brief Removes the handler for an extension
  /// @param extension The extension to stop handling
  /// @return true if a handler was removed
  bool unregister_ext_handler(std::string_view extension);

  /// @brief Reports whether an extension has a handler
  /// @param extension The extension to check
  /// @return true if a handler is registered
  bool is_ext_registered(std::string_view extension) const;

  /// @brief Opens a file through the handler for its extension, or with
  /// fopen if it has none
  /// @param path The path to open
  /// @param mode The mode string, as std::fopen takes
  /// @return An owning handle, empty if the open failed
  FILE_uptr open(const std::filesystem::path& path, const char* mode) const;
};

#ifdef __linux__
/// @brief Runs a command and opens a pipe to it with popen
/// @param command The shell command to run
/// @param mode The mode string, as popen takes
/// @return An owning handle that pcloses itself, empty if the open failed
FILE_uptr popen_ptr(const char* command, const char* mode);

/// @brief A FileOpener that transparently reads and writes compressed
/// files by piping them through whichever command line tools are installed.
///
/// TODO This is VERY insecure.
class CommonCompressedFileOpener : public FileOpener {
 protected:
  static bool is_executable_on_path(std::string_view binary);
  static Handler popen_handler(std::string binary, std::string read_args,
                               std::string write_cmd, std::string append_args);
  bool maybe_register_handler(std::string extension, std::string binary,
                              std::string read_args, std::string write_args,
                              std::string append_args);

 public:
  /// @brief Registers a handler for each compression tool found on PATH
  CommonCompressedFileOpener();
};
#endif

}  // namespace misc

#endif