#pragma once

#include <allocated_arrays.h>

namespace misc {

template <typename T>
class shared_ptr {
  struct control_block {
    size_t refs;
    size_t weak_ptrs;
    std::function<void(T*)> deleter;
  };
  T* m_ptr = nullptr;
  control_block* m_control_block = nullptr;

  shared_ptr();

  shared_ptr(T* ptr)
      : m_ptr{ptr},
        m_control_block{new control_block{1, 0, [](T* p) {
                                            if constexpr (std::is_array_v<T>) {
                                              delete[] p;
                                            } else {
                                              delete p;
                                            }
                                          }}} {}

  shared_ptr(const shared_ptr& o)
      : m_ptr{o.ptr}, m_control_block{o.m_control_block} {
    if (m_control_block) ++m_control_block->refs;
  }

  shared_ptr(shared_ptr&& o)
      : m_ptr{o.ptr}, m_control_block{o.m_control_block} {
    o.ptr = nullptr;
    o.m_control_block = nullptr;
  }

  shared_ptr& operator=(const shared_ptr& o) {
    if (m_control_block) dec_ref();
    m_ptr = o.m_ptr;
    m_control_block = o.m_control_block;
    if (m_control_block) ++m_control_block->refs;
  }

  shared_ptr& operator=(shared_ptr&& o) {
    if (m_control_block) dec_ref();
    m_ptr = o.m_ptr;
    m_control_block = o.m_control_block;
  }

  ~shared_ptr() {
    if (m_control_block) dec_ref();
  }

  void reset() {
    if (m_control_block) dec_ref();
    m_ptr = nullptr;
    m_control_block = nullptr;
  }

  template <typename T, typename... Args>
  friend shared_ptr<T> make_shared(Args&& args) {
    static_assert(!std::is_array_v<T>(), "T cannot be an array type");
    struct OneBlock {
      control_block block;
      T t;
    };
    shared_ptr<T> retval;

    void* arr = new byte[sizeof(OneBlock)];
    retval.m_control_block = new (arr) control_block;
    retval.m_control_block->deleter = [block_ptr =
                                           retval.m_control_block](T* p) {
      std::destroy_at(p);
      std::free(block_ptr);
    };
    void* t_pos = &reinterpret_cast<OneBlock*>(arr)->t;

    retval.m_ptr = new (t_pos) T(std::forward<Args>(args)...);
    retval.m_refs = 1;
    retval.m_weak_ptrs = 0;
    return retval;
  }

  size_t use_count() const { return m_control_block->m_refs; }
  T* get() const { return m_ptr; }

 private:
  void dec_ref() {
    if (--m_control_block->refs == 0) {
      m_control_block->deleter(m_ptr);
      delete m_control_block;
    }
  }
};
}  // namespace misc
