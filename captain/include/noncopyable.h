#ifndef __CAPTAIN_NONCOPYABLE_H__
#define __CAPTAIN_NONCOPYABLE_H__

namespace captain {

class Noncopyable {
public:
    Noncopyable() = default;
    ~Noncopyable() = default;
    Noncopyable(const Noncopyable&) = delete;
    Noncopyable& operator=(const Noncopyable&) = delete;
};

}

#endif
