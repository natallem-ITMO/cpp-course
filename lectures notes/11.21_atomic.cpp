#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <optional>
#include <iostream>
#include <atomic>
#include <variant>

namespace ex1 {
    std::array<std::atomic<uint32_t>, 1000> accounts;

    void transfer(size_t to, size_t from, uint32_t amount) {

        uint32_t old = accounts[to].load();
        do {
            if (old < amount) {
                throw std::runtime_error("insufficient funds");
            }
        } while (!accounts[from].compare_exchange_weak(old, old - amount));
        /*
         * compare_exchange_weak гарантия - это значит что операция может зафейлиться, если внутри находится
         *  действительное нужное значение,
         * но т.к. мы крутимся в цикле сами, то можно и не брать строгую гарантию.
         * compare_exchange_strong гарантируем, что если у нас было внутри old value, то мы его поменяем.
         *
         * Такие штуки нужны, т.к. на каких то архитектурах кроме x86 cas может быть из команд, которые фейлются.
         *
         */
        accounts[to] = +amount;
    }

    uint32_t get_balance(size_t num) {
        return accounts[num].load();
    }

}

namespace ex2 {
    /*
     * Про memory order
     */

    /*
     * Процы любят переставлять команды. atomic позволяет предупредить переупорядочивание слишком неочевидное.
     * В х86 например команды mov могут переупорядочиваться, а вот xchg не могут. Поэтому всякие load() - mov, a store()
     * уже mov + xchg. А еще у интела есть гарантия, что внешние потоки увидят в одном порядке записи других потоков,
     * а вот те потоки, которые непосредственно пишут совместно с другим потоком, могут увидеть разный порядок.
     * Например в примере ниже все будет корректно и thread_1 / thread_2 увидят все правильно. Но вот если бы наблюдали
     * thread_3 /thread_4, то они бы могли увидеть хрень.
     */

    std::atomic<int> x;
    std::atomic<int> y;

    void thread_1() {
//        thread3
        int x_ = x.load(); // 1
        int y_ = y.load(); // 0
//        thread4
        if (x_ == 1 && y_ == 0) {
            std::cout << "thread3 before thread4 100%\n"; // можем быть уверены, что одновременно не выведем такое.
        }
    }

    void thread_2() {
        int y_ = y.load();
        int x_ = x.load();
        if (y_ == 1 && x_ == 0) {
            std::cout << "thread4 before thread3 100%\n";
        }
    }

    void thread_3() {
        x.store(1);
    }

    void thread_4() {
        y.store(1);
    }

    int value;
    std::atomic<bool> value_present;

    void produce() {
        value = 3;
        value_present.store(true,
                            std::memory_order_release); // компилятор не имеет права переставлять то что выше вниз.
        // т е мы зарелизили
    }

    void try_consume() {
        if (value_present.load(std::memory_order_acquire)) { // компилятор не имеет права переставлять то что ниже вверх
            int tmp = value;
        }

        // и + еще все записи потока, которые сделал один поток до load with release будут видны потоку, который сделал
        // store with acquire и все последующие действия

        /*
         * Т е если мы без memory_order, то у нас слишком сильные гарантия, т к не можем вообще переупорядочивать. По сути
         * atomic будет точкой линеаризации
         */

    }

/*
         * memory_order_relaxed - говорим компилятору, что никакие гарантии в порядке нам не нужны.
         *
         * thread.join() гарантирует, что после те изменения которые сделал поток, будут видны в остальных местах.
         * И при создании потока им так же будут видны записи до. Тоже точка линеаризации
         */

    std::atomic<int> number_of_events;
    void thread1(){
            for (;;){
                number_of_events.fetch_add(1, std::memory_order_relaxed);
            }
    };

    void main(){
        number_of_events.store(42, std::memory_order_relaxed);
        std::thread th(&thread1);
        th.join();
        number_of_events.load(std::memory_order_relaxed);
        /*
         * relaxed позволяет видеть потокам неупорядоченные операции
         */
    }

}

namespace ex3{
    /*
     * volatile никак не помогает в многопотоке.
     *
     * Изначальная мотивация использования волотайла.
     * 1) device memory
     *
     * К компьютеру подключены устройства. Записи и чтение в закрепленную за устройством памать интерпретируются как
     *  команды. Чтобы подавть оптимизации компилятора по многократной записи одного и того же, или многократному
     *  чтения, использовали volatile.
     *
     * 2) setjmp/longjmp (сишная штука про исключения, не актуально)
     *
     * 3) UNIX-signal - например хотим чтобы при SIGSEG выполнялся какой то код
     *
     *
     *
     */
}
template <typename T>
struct type_identity{
    typedef T type;
};
template <typename T>
T&& forward(typename type_identity<T>::type& obj){
    return static_cast<T&&>(obj);
}

int foo(int && e){
    int &d = forward<int&>(e);
}

template<typename T>
void foo(T * prt){

}

int main(){
    int const expr1 = 2;
    auto expr2 = expr1;
    static_assert(std::is_same_v<int, decltype(expr2)>);
    std::variant<int, float> d;
    d.emplace<float>(3);
    int a = 2;
    int && ds = forward<int>(a);
    int & sdf = ds;
    foo(0);
    int t = 10;
    foo(t);
}


