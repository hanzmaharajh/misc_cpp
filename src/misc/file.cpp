#include "file.h"

#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

namespace misc {

namespace {
FILE_uptr null_fuptr() { return FILE_uptr{nullptr, nullptr}; }
}  // namespace

FILE_uptr fopen_ptr(const char* filename, const char* mode) {
  return FILE_uptr(std::fopen(filename, mode), &std::fclose);
}

bool FileOpener::register_ext_handler(std::string extension, Handler func) {
  const auto [_, inserted] =
      handlers.insert_or_assign(std::move(extension), std::move(func));
  return inserted;
}

bool FileOpener::unregister_ext_handler(std::string_view extension) {
  return handlers.erase(extension);
}
bool FileOpener::is_ext_registered(std::string_view extension) const {
  return handlers.count(extension) > 0;
}

FILE_uptr FileOpener::open(const std::filesystem::path& path,
                           const char* mode) const {
  if (const auto it = handlers.find(path.extension()); it != handlers.end()) {
    return it->second(path, mode);
  }
  return null_fuptr();
}

#ifdef __linux__
FILE_uptr popen_ptr(const char* command, const char* mode) {
  return FILE_uptr(popen(command, mode), &pclose);
}

bool CommonCompressedFileOpener::is_executable_on_path(
    std::string_view binary) {
  using namespace std::literals;
  auto which_cmd = "which '"s;
  which_cmd.append(binary).append("' > /dev/null 2>&1 "sv);
  return std::system(which_cmd.c_str()) == 0;
}

bool CommonCompressedFileOpener::maybe_register_handler(
    std::string extension, std::string binary, std::string read_args,
    std::string write_args, std::string append_args) {
  if (!is_executable_on_path(binary)) return false;

  return register_ext_handler(
      std::move(extension),
      popen_handler(std::move(binary), std::move(read_args),
                    std::move(write_args), std::move(append_args)));
}

FileOpener::Handler CommonCompressedFileOpener::popen_handler(
    std::string binary, std::string read_args, std::string write_args,
    std::string append_args) {
  if (!is_executable_on_path(binary)) {
    return [](const std::filesystem::path&, const char*) {
      return null_fuptr();
    };
  }

  return
      [binary = std::move(binary), read_args = std::move(read_args),
       write_cmd = std::move(write_args), append_args = std::move(append_args)](
          const std::filesystem::path& path, const char* mode) {
        if (std::strlen(mode) < 1) return null_fuptr();
        const char* m = "w";
        std::string cmd = binary + " ";
        switch (mode[0]) {
          case 'r':
            cmd.append(read_args);
            m = "r";
            break;
          case 'w':
            cmd.append(write_cmd);
            break;
          case 'a':
            cmd.append(append_args);
            break;
          default:
            return null_fuptr();
        }
        cmd += " ";
        cmd.append(path);
        return popen_ptr(cmd.c_str(), m);
      };
}

CommonCompressedFileOpener::CommonCompressedFileOpener() {
  using namespace std::literals;
  maybe_register_handler(".gz"s, "gzip"s, "-qcd"s, ">"s, ">>"s);
  maybe_register_handler(".zstd"s, "zstd"s, "-qcd"s, ">"s, ">>"s);
  maybe_register_handler(".xz"s, "xz"s, "-qcd"s, ">"s, ">>"s);
}

#endif

}  // namespace misc
