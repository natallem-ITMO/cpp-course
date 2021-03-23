#include <memory>
#include <cassert>
#include <map>
#include <iostream>

struct object {
    int value;

    object(int a) : value(a) {};
};

struct cache {
private:
    struct object_container;

    typedef std::map<int, std::weak_ptr<object_container>> objects_t;

public:

    cache() = default;

    cache &operator=(cache const &) = delete;

    cache(cache const &) = delete;

    size_t size();

    std::shared_ptr<object> alloc(int id);

private :
    objects_t objects;
};

struct cache::object_container {
    object_container(object_container const &) = delete;

    object_container &operator=(object_container const &) = delete;

    virtual ~object_container() {
        if (itr!=ptr_cache->objects.end()){
            ptr_cache->objects.erase(itr);
        }
    }

    object_container(cache *ptrCache, const object &obj,
                     const objects_t::const_iterator &itr) : ptr_cache(ptrCache), obj(obj),
                                                             itr(itr) {}

private:
    cache *ptr_cache;
    object obj;
    objects_t::const_iterator itr;
    friend cache;
};


std::shared_ptr<object> cache::alloc(int id) {
    std::shared_ptr<object_container> cont;
    auto it = objects.find(id);
    if (it != objects.end()) {
        cont = it->second.lock();
        assert(cont != nullptr);
    } else {
        cont = std::make_shared<object_container>(this, id, objects.end());
        //если произойдет ошибка при инсерте, то контейнер будет пытаться удалить себя из мапы, в которую не вставлен, потому в делите стоит иф
        cont->itr = objects.insert(it, {id, cont});
    }
    std::shared_ptr<object> ret = std::shared_ptr<object>(cont,&(cont->obj));
    return ret;
}

size_t cache::size() {
    return objects.size();
}


int main() {
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