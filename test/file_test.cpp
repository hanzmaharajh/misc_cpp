#include <gtest/gtest.h>
#include <misc/file.h>

#include <boost/scope_exit.hpp>
#include <cstdio>

#ifdef __linux__

class CommonCompressedFileOpenerFixture
    : public misc::CommonCompressedFileOpener,
      public testing::Test {
 protected:
  static std::string new_temp_filename(const char* ext) {
    std::string name =
        ::testing::UnitTest::GetInstance()->current_test_info()->name();
    name += "XXXXXX";
    const auto& r = mkstemp(name.data());
    if (r == -1)
      throw std::runtime_error("Failed to create temp file");
    name += ext;
    return name;
  }
};

TEST_F(CommonCompressedFileOpenerFixture, open_gz) {
  if (!is_executable_on_path("gzip")) GTEST_SKIP() << "gzip not found";
  const auto path = new_temp_filename(".gz");
  BOOST_SCOPE_EXIT_ALL(&) { ASSERT_EQ(std::remove(path.c_str()), 0); };

  const std::string hello = "HELLO WORLD\n";
  {
    const auto w_ptr = open(path, "w");
    ASSERT_TRUE(w_ptr);
    std::fprintf(w_ptr.get(), "%s", hello.c_str());
  }
  {
    const auto r_ptr = open(path, "r");
    ASSERT_TRUE(r_ptr);
    std::array<char, 1024> arr;
    const auto& read =
        std::fread(arr.data(), sizeof(char), arr.size(), r_ptr.get());
    ASSERT_EQ(read, hello.size());
    ASSERT_EQ((std::string_view{arr.data(), read}), hello);
  }
  {
    const auto a_ptr = open(path, "a");
    ASSERT_TRUE(a_ptr);
    std::fprintf(a_ptr.get(), "%s", hello.c_str());
  }
  {
    const auto r_ptr = open(path, "r");
    ASSERT_TRUE(r_ptr);
    std::array<char, 1024> arr;
    const auto& read =
        std::fread(arr.data(), sizeof(char), arr.size(), r_ptr.get());
    ASSERT_EQ(read, hello.size() * 2);
    ASSERT_EQ((std::string_view{arr.data(), read / 2}), hello);
    ASSERT_EQ((std::string_view{arr.data() + read / 2, read / 2}), hello);
  }
  {
    const auto w_ptr = open(path, "w");
    ASSERT_TRUE(w_ptr);
    std::fprintf(w_ptr.get(), "%s", hello.c_str());
  }
  {
    const auto r_ptr = open(path, "r");
    ASSERT_TRUE(r_ptr);
    std::array<char, 1024> arr;
    const auto& read =
        std::fread(arr.data(), sizeof(char), arr.size(), r_ptr.get());
    ASSERT_EQ(read, hello.size());
    ASSERT_EQ((std::string_view{arr.data(), read}), hello);
  }
}

TEST_F(CommonCompressedFileOpenerFixture, missing_binary) {
  register_ext_handler(".xx", popen_handler("XXXX", "-cd", ">", ">>"));

  const auto path = new_temp_filename(".xx");
  BOOST_SCOPE_EXIT_ALL(&) { ASSERT_NE(std::remove(path.c_str()), 0); };

  {
    const auto w_ptr = open(path, "w");
    ASSERT_FALSE(w_ptr);
  }
  {
    const auto r_ptr = open(path, "r");
    ASSERT_FALSE(r_ptr);
  }
  {
    const auto a_ptr = open(path, "a");
    ASSERT_FALSE(a_ptr);
  }
}

#endif