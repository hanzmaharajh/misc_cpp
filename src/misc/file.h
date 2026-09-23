#pragma once

#ifndef MISC_HEADER_ONLY

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace misc {

using FILE_uptr = std::unique_ptr<FILE, int (*)(FILE*)>;

FILE_uptr fopen_ptr(const char* filename, const char* mode);

class FileOpener {
 protected:
  using Handler =
      std::function<FILE_uptr(const std::filesystem::path&, const char*)>;

  std::unordered_map<std::filesystem::path, Handler> handlers;

 public:
  bool register_ext_handler(std::string extension, Handler func);
  bool unregister_ext_handler(std::string_view extension);
  bool is_ext_registered(std::string_view extension) const;

  FILE_uptr open(const std::filesystem::path& path, const char* mode) const;
};

#ifdef __linux__
FILE_uptr popen_ptr(const char* command, const char* mode);

// TODO This is VERY insecure.
class CommonCompressedFileOpener : public FileOpener {
 protected:
  static bool is_executable_on_path(std::string_view binary);
  static Handler popen_handler(std::string binary, std::string read_args,
                               std::string write_cmd, std::string append_args);
  bool maybe_register_handler(std::string extension, std::string binary,
                              std::string read_args, std::string write_args,
                              std::string append_args);

 public:
  CommonCompressedFileOpener();
};
#endif

}  // namespace misc

#endif