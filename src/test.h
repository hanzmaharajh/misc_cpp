#pragma once
namespace misc {

struct CopyRecorder {
  CopyRecorder() {}
  CopyRecorder(CopyRecorder* m) : master(m) {}
  ~CopyRecorder() {}
  CopyRecorder(const CopyRecorder& o)
      : master(o.master),
        copy_constructed{o.copy_constructed + 1},
        move_constructed{o.move_constructed},
        copy_assigned{o.copy_assigned},
        move_assigned{o.move_assigned} {
    if (master) ++master->copy_constructed;
  }

  CopyRecorder(CopyRecorder&& o)
      : master(o.master),
        copy_constructed{o.copy_constructed},
        move_constructed{o.move_constructed + 1},
        copy_assigned{o.copy_assigned},
        move_assigned{o.move_assigned} {
    if (master) ++master->move_constructed;
  }

  CopyRecorder& operator=(const CopyRecorder& o) {
    master = o.master;
    copy_constructed = o.copy_constructed;
    move_constructed = o.move_constructed;
    copy_assigned = o.copy_assigned + 1;
    move_assigned = o.move_assigned;
    if (master) ++master->copy_assigned;
    return *this;
  }

  CopyRecorder& operator=(CopyRecorder&& o) {
    master = o.master;
    o.master = nullptr;
    copy_constructed = o.copy_constructed;
    move_constructed = o.move_constructed;
    copy_assigned = o.copy_assigned;
    move_assigned = o.move_assigned + 1;
    if (master) ++master->move_assigned;
    return *this;
  }

  CopyRecorder* master = nullptr;

  size_t copy_constructed = 0;
  size_t move_constructed = 0;
  size_t copy_assigned = 0;
  size_t move_assigned = 0;
};
}  // namespace misc
