#include <memory>
#include <cassert>
#include <map>
#include <iostream>

struct object {
    int value;

    explicit object(int a) : value(a) {};
};

struct cache {
private:
    typedef std::map<int, std::weak_ptr<object>> objects_t;

    struct deleter;

public:

    cache()=default;

    cache &operator=(cache const &) = delete;

    cache(cache const &) = delete;

    size_t size();

    std::shared_ptr<object> alloc(int id);

private :
    objects_t objects;
};

struct cache::deleter {

    deleter(cache *ptr, objects_t::const_iterator itr) : ptr_to_cache(ptr), iterator(itr) {}

    void operator()(object *obj) {
        ptr_to_cache->objects.erase(iterator);
        delete obj;
    }

private:
    cache *ptr_to_cache;
    objects_t::const_iterator iterator;
};

std::shared_ptr<object> cache::alloc(int id) {
    std::shared_ptr<object> obj;
    auto it = objects.find(id);
    if (it != objects.end()) {
        obj = it->second.lock();
        assert(obj != nullptr);
        return obj;
    }
    it = objects.insert(it, {id, std::weak_ptr<object>()});
    try {
        obj = std::shared_ptr<object>(new object(id), deleter(this, it));
        //if shptr бросит исключение, он все равно удалит переданный ему объект
    } catch (...){
        objects.erase(it);
        throw;
    }
    it->second = obj;
    return obj;
}

size_t cache::size() {
    return objects.size();
}


int main()
{
    cache p;
    p.alloc(3);
    assert(p.size() == 0);

    auto four = p.alloc(4);
    assert(p.size() == 1);
    auto five = p.alloc(5);
    assert(p.size() == 2);
    auto four2 = p.alloc(4);
    assert(four == four2);
    five.reset();
    assert(p.size() == 1);
}