#pragma once
#include <climits>

#include "log2.h"

namespace misc {
template <typename T>
class tagged_ptr {
  // TODO Use log2() when it is constexpr
  static constexpr size_t num_bits(size_t v) {
    return v < 2 ? v : 1 + num_bits(v >> 1);
  }

 public:
  constexpr static const size_t tag_bit_width = num_bits(alignof(T)) - 1;

 protected:
  static_assert(tag_bit_width > 0, "alignment doesn't allow space for tag");
  uintptr_t m_ptr;
  static constexpr const uintptr_t tag_mask = (1 << tag_bit_width) - 1;
  static constexpr const uintptr_t ptr_mask = ~tag_mask;

 public:
  tagged_ptr(T* ptr, size_t tag)
      : m_ptr(reinterpret_cast<uintptr_t>(ptr) | tag) {
    assert((tag & ptr_mask) == 0);
  }
  T* get() const { return reinterpret_cast<T*>(m_ptr & ptr_mask); }
  size_t tag() const { return m_ptr & tag_mask; }
  void set_tag(size_t tag) {
    assert((tag & ptr_mask) == 0);
    m_ptr = (m_ptr & ptr_mask) | tag;
  }
  void reset(T* ptr = nullptr) {
    assert((reinterpret_cast<uintptr_t>(ptr) & tag_mask) == 0);
    m_ptr = (m_ptr & tag_mask) | reinterpret_cast<uintptr_t>(ptr);
  }
  T* operator->() const { return get(); }
};

template <typename T>
class unique_tagged_ptr : private tagged_ptr<T> {
  using base_type = tagged_ptr<T>;

 public:
  unique_tagged_ptr(T* ptr, size_t tag_value) : base_type(ptr, tag_value) {}
  unique_tagged_ptr(const unique_tagged_ptr&) = delete;
  unique_tagged_ptr(unique_tagged_ptr&& o) noexcept {
    this->m_ptr = o.m_ptr;
    o.m_ptr = 0;
  }
  unique_tagged_ptr& operator=(const unique_tagged_ptr&) = delete;
  unique_tagged_ptr& operator=(unique_tagged_ptr&& o) {
    this->m_ptr = o.m_ptr;
    o.m_ptr = 0;
  }
  ~unique_tagged_ptr() { delete get(); }

  using base_type::get;
  using base_type::set_tag;
  using base_type::tag;
  using base_type::operator->;

  void reset(T* ptr = nullptr) {
    delete get();
    base_type::reset(ptr);
  }
};
}  // namespace misc