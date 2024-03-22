#include <allocated_arrays.h>
#include <gmock/gmock-spec-builders.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <exception>
#include <iterator>
#include <memory>

TEST(align, unique_arrays) {
  EXPECT_EQ(alignof(char), 1);
  EXPECT_GT(alignof(size_t), 1);

  {
    misc::unique_arrays<char, size_t> arr(1, 1);
    const auto char_range = arr.get<0>();
    const auto size_t_range = arr.get<1>();

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    EXPECT_EQ(reinterpret_cast<const char*>(size_t_range.begin()) -
                  char_range.begin(),
              alignof(size_t));
  }
}

TEST(ranges, unique_arrays) {
  constexpr size_t char_len = 4;
  constexpr size_t size_t_len = 9;
  misc::unique_arrays<char, size_t> arr(char_len, size_t_len);
  const auto char_range = arr.get<0>();
  const auto size_t_range = arr.get<1>();
  EXPECT_EQ(std::distance(char_range.begin(), char_range.end()), char_len);
  EXPECT_EQ(char_range.size(), char_len);
  EXPECT_EQ(std::distance(size_t_range.begin(), size_t_range.end()),
            size_t_len);
  EXPECT_EQ(size_t_range.size(), size_t_len);
}

TEST(set, unique_arrays) {
  constexpr size_t char_len = 4;
  constexpr size_t size_t_len = 9;
  constexpr char char_value = 'a';
  constexpr size_t size_t_value = 1'000;
  misc::unique_arrays<char, size_t> arr(char_len, size_t_len);
  auto char_range = arr.get<0>();
  auto size_t_range = arr.get<1>();
  std::fill(char_range.begin(), char_range.end(), char_value);
  std::fill(size_t_range.begin(), size_t_range.end(), size_t_value);

  for (const auto& element : char_range) {
    EXPECT_EQ(element, char_value);
  }
  for (const auto& element : size_t_range) {
    EXPECT_EQ(element, size_t_value);
  }
}

TEST(deletes, unique_arrays) {
  auto char_ptr = std::make_shared<char>();
  auto size_t_ptr = std::make_shared<size_t>();

  {
    const misc::unique_arrays arr({char_ptr, char_ptr, char_ptr}, {size_t_ptr});
    EXPECT_EQ(char_ptr.use_count(), 4);
    EXPECT_EQ(size_t_ptr.use_count(), 2);
  }

  EXPECT_EQ(char_ptr.use_count(), 1);
  EXPECT_EQ(size_t_ptr.use_count(), 1);
}

TEST(exception_on_init_list, unique_arrays) {
  static size_t constructed, copied, destroyed;
  struct Throws {
    Throws() { ++constructed; }
    Throws(const Throws&) {
      if (copied + 1 == 3) {
        throw std::exception();
      }
      ++copied;
    }
    ~Throws() { ++destroyed; }
  };

  try {
    constructed = 0, copied = 0, destroyed = 0;
    const misc::unique_arrays arr(
        {'a', 'b', 'c'}, {Throws{}, Throws{}, Throws{}, Throws{}, Throws{}});
    FAIL();
  } catch (const std::exception&) {
    EXPECT_EQ(constructed, 5);
    EXPECT_EQ(copied, 2);
    EXPECT_EQ(destroyed, constructed + copied);
  }
}

TEST(exception_on_init, unique_arrays) {
  static size_t constructed;
  static size_t destroyed;

  struct Throws {
    Throws() {
      if (constructed + 1 == 3) {
        throw std::exception();
      }
      ++constructed;
    }
    ~Throws() { ++destroyed; }
  };

  try {
    constructed = 0;
    destroyed = 0;
    const misc::unique_arrays<char, Throws> arr(2, 5);
    FAIL();
  } catch (const std::exception&) {
    EXPECT_EQ(constructed, 2);
    EXPECT_EQ(destroyed, 2);
  }
}

TEST(exception_on_copy, unique_arrays) {
  static size_t constructed;

  struct Throws {
    Throws() {
      if (constructed + 1 == 3) {
        throw std::exception();
      }
      ++constructed;
    }
  };

  try {
    constructed = 0;
    const misc::unique_arrays<char, Throws> arr(2, 5);
    FAIL();
  } catch (const std::exception&) {
    EXPECT_EQ(constructed, 2);
  }
}

TEST(unique_arrays, move) {
  auto arrs =
      std::make_unique<misc::unique_arrays<std::shared_ptr<int>, char>>(5, 4);
  std::shared_ptr<int>* int_ptr = arrs->get<0>().begin();
  char* char_ptr = arrs->get<1>().begin();

  int_ptr->reset(new int(5));

  auto copy_int_ptr = *int_ptr;

  auto arrs2 = std::move(arrs);

  EXPECT_EQ(int_ptr, arrs2->get<0>().begin());
  EXPECT_EQ(char_ptr, arrs2->get<1>().begin());
  EXPECT_EQ(**int_ptr, 5);
  arrs.reset();
  EXPECT_EQ(copy_int_ptr.use_count(), 2);
  arrs2.reset();
  EXPECT_EQ(copy_int_ptr.use_count(), 1);
}

// TODO Test exceptions mid-copy, mid-init
