#ifndef HF_LINEAR_PTR_H_
#define HF_LINEAR_PTR_H_

/* Linear ownership semantics through shared pointer. */

#include <memory>
#include <atomic>
#include <stdexcept>
#include <mutex>

namespace hf {

struct linear_access_violation: public std::runtime_error {
    template <typename S>
    explicit linear_access_violation(S&& whatstr): std::runtime_error(std::forward<S>(whatstr)) {}
};

namespace impl {
    struct linear_ptr_state {
        template <typename T>
        explicit linear_ptr_state(T* item):
            delete_item([item]() { std::default_delete<T> d; d(item); })
        {}

        template <typename T,typename Deleter>
        linear_ptr_state(T* item, Deleter d):
            delete_item([item,d]() { d(item); })
        {}

        ~linear_ptr_state() {
            if (delete_item) delete_item();
        }

        void lock() { m.lock(); }
        void unlock() { m.unlock(); }

    private:
        std::mutex m;
        // type-erased record of item's deleter
        std::function<void ()> delete_item;
    };

    class linear_ptr_base {
        mutable const linear_ptr_base *prev=nullptr;
        mutable const linear_ptr_base *next=nullptr;
        mutable std::atomic<void *> item{nullptr};
        mutable linear_ptr_state* state=nullptr;

    public:
        linear_ptr_base() {}

        template <typename T>
        explicit linear_ptr_base(T* p): item(p), state(new linear_ptr_state(p)) {}

        template <typename T, typename Deleter>
        linear_ptr_base(T *p, Deleter d): item(p), state(new linear_ptr_state(p, d)) {}

        virtual ~linear_ptr_base() { reset(); }

        void reset() {
            if (!state) return;

            bool last=false;
            state->lock();

            if (prev) prev->next=next;
            if (next) next->prev=prev;

            if (item) {
                if (next) next->item=item.load();
                else last=true;
            }

            state->unlock();
            if (last) delete state;

            state=nullptr;
            item=nullptr;
        }

        template <typename T>
        void reset(T* p) {
            reset();
            item=p;
            state=new linear_ptr_state(p);
        }

        template <typename T, typename Deleter>
        void reset(T* p, Deleter d) {
            reset();
            item=p;
            state=new linear_ptr_state(p, d);
        }

        operator bool() const { return static_cast<bool>(item.load()); }

    protected:
        void assign_(const linear_ptr_base& them) {
            if (this==&them) return;

            reset();
            state=them.state;
            if (!state) return;

            state->lock();

            prev=them.prev;
            if (prev) prev->next=this;

            next=&them;
            them.prev=this;

            if (them.item) {
                item=them.item.load();
                them.item=nullptr;
            }

            state->unlock();
        }

        template <typename T>
        T* get_() const { return (T *)item.load(); }

    };
}

template <typename T>
class linear_ptr: public impl::linear_ptr_base {
public:
    linear_ptr() {}

    explicit linear_ptr(T *p): linear_ptr_base(p) {}

    template <typename Deleter>
    explicit linear_ptr(T *p, Deleter d): linear_ptr_base(p,d) {}

    linear_ptr(linear_ptr&&) =delete;
    linear_ptr& operator=(linear_ptr&&) =delete;

    linear_ptr(const linear_ptr& them) { pre_insert(them); }

    template <typename Y, typename =std::enable_if<std::is_convertible<Y*,T*>::value>>
    linear_ptr(const linear_ptr<Y>& them) { assign_(them); }

    linear_ptr& operator=(const linear_ptr& them) {
        assign_(them);
        return *this;
    }

    template <typename Y, typename =std::enable_if<std::is_convertible<Y*,T*>::value>>
    linear_ptr& operator=(const linear_ptr<Y>& them) {
        assign_(them);
        return *this;
    }

    T* get() const { return get_<T>(); }

    T* operator->() const {
        if (!*this) throw linear_access_violation("not owner");
        return get();
    }

    T& operator*() const {
        if (!*this) throw linear_access_violation("not owner");
        return *get();
    }
};

} // namespace hf

#endif // ndef HF_LINEAR_PTR_H_
