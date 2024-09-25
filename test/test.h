#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>

struct SpecMemberCountingFixture : public testing::Test {
  static struct CallCounter {
    size_t constructor_calls = 0;
    size_t move_constructor_calls = 0;
    size_t copy_constructor_calls = 0;
    size_t destructor_calls = 0;
    size_t allocating_new_calls = 0;
    size_t placement_new_calls = 0;
    size_t copy_assign_calls = 0;
    size_t move_assign_calls = 0;
    size_t swap_calls = 0;
  } call_counts;

  struct TestElement {
    size_t v;
    TestElement() : v{0} { ++call_counts.constructor_calls; }
    TestElement(size_t val) : v{val} { ++call_counts.constructor_calls; }
    TestElement(const TestElement& t) : v{t.v} {
      ++call_counts.constructor_calls;
      ++call_counts.copy_constructor_calls;
    }
    TestElement(TestElement&& t) : v{t.v} {
      ++call_counts.constructor_calls;
      ++call_counts.move_constructor_calls;
    }
    TestElement& operator=(const TestElement& e) {
      v = e.v;
      ++call_counts.copy_assign_calls;
      return *this;
    }
    TestElement& operator=(TestElement&& e) {
      v = e.v;
      ++call_counts.move_assign_calls;
      return *this;
    }
    ~TestElement() { ++call_counts.destructor_calls; }
    void* operator new(size_t size) {
      ++call_counts.allocating_new_calls;
      return ::operator new(size);
    }
    void* operator new(std::size_t size, void* b) {
      ++call_counts.placement_new_calls;
      return ::operator new(size, b);
    }
    friend bool operator==(const TestElement& l, const TestElement& r) {
      return l.v == r.v;
    }
    [[maybe_unused]] friend bool operator!=(const TestElement& l,
                                            const TestElement& r) {
      return !(l == r);
    }

    friend void swap(TestElement& a, TestElement& b) {
      using namespace std;
      swap(a.v, b.v);
      ++call_counts.swap_calls;
    }
  };

  SpecMemberCountingFixture() { call_counts = CallCounter{}; }

  ~SpecMemberCountingFixture() {
    EXPECT_EQ(call_counts.constructor_calls, call_counts.destructor_calls);
  }
};

struct CopyRecorder {
  CopyRecorder() {}
  ~CopyRecorder() {}
  CopyRecorder(const CopyRecorder& o)
      : copy_constructed{o.copy_constructed + 1},
        move_constructed{o.move_constructed},
        copy_assigned{o.copy_assigned},
        move_assigned{o.move_assigned} {}

  CopyRecorder(CopyRecorder&& o) noexcept
      : copy_constructed{o.copy_constructed},
        move_constructed{o.move_constructed + 1},
        copy_assigned{o.copy_assigned},
        move_assigned{o.move_assigned} {}

  CopyRecorder& operator=(const CopyRecorder& o) {
    copy_constructed = o.copy_constructed;
    move_constructed = o.move_constructed;
    copy_assigned = o.copy_assigned + 1;
    move_assigned = o.move_assigned;
    return *this;
  }

  CopyRecorder& operator=(CopyRecorder&& o) noexcept {
    copy_constructed = o.copy_constructed;
    move_constructed = o.move_constructed;
    copy_assigned = o.copy_assigned;
    move_assigned = o.move_assigned + 1;
    return *this;
  }

  size_t copy_constructed = 0;
  size_t move_constructed = 0;
  size_t copy_assigned = 0;
  size_t move_assigned = 0;
};
