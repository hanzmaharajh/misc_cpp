#pragma once
namespace misc {
struct CopyRecorder {
  CopyRecorder() {}
  ~CopyRecorder() {}
  CopyRecorder(const CopyRecorder& o)
      : copy_constructed{o.copy_constructed + 1},
        move_constructed{o.move_constructed},
        copy_assigned{o.copy_assigned},
        move_assigned{o.move_assigned} {}

  CopyRecorder(CopyRecorder&& o)
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

  CopyRecorder& operator=(CopyRecorder&& o) {
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
}  // namespace misc
