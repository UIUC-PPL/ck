#ifndef CK_SINGLETON_HPP
#define CK_SINGLETON_HPP

namespace ck {
namespace detail {
// Helper Singleton Class
template <typename T, typename SingletonClass>
class singleton {
 public:
  using value_type = T;
  using class_type = SingletonClass;

  // Non-copyable, non-movable
  singleton(singleton const&) = delete;
  singleton(singleton&&) = delete;
  singleton& operator=(singleton const&) = delete;
  singleton& operator=(singleton&&) = delete;

  static const std::unique_ptr<value_type>& instance() {
    static std::unique_ptr<value_type> inst{new value_type()};

    return inst;
  }

 protected:
  singleton() = default;
};
}  // namespace detail
}  // namespace ck

#define CK_GENERATE_SINGLETON(type, name)                 \
  class name : public ck::detail::singleton<type, name> { \
   private:                                               \
    name() = default;                                     \
  }

#define CK_ACCESS_SINGLETON(name) (*name::instance())

#endif
