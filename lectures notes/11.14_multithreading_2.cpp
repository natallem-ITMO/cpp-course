#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <optional>
#include <iostream>
#include <functional>

namespace ex1 {
/*
 * Два потока, 1ый продуцирует данные, нам их надо передать во 2ой поток, чтобы он из обработал
 * Что то типа пайма, но для конкретного типа
 */

    template<typename T>
    struct concurrent_queue {
        void push(T value) {
            std::lock_guard<std::mutex> lg(m);
            q.push_back(std::move(value));
        }

        bool empty() const {
            std::lock_guard<std::mutex> lg(m);
            return q.empty();
            /*
             * Мы не знаем, как дек устроен, и + не можем работать с примитивными типами из разных потоков, поэтому все
             *  равно берем лок - иначе ub
             */
        }

        /*
         * Даже если мы перед поп бэком проверим на пустоту, то это не гарантирует, что мы получим объект.
         * Не достаточно навесить локов для какой тос структуры
         */
        T pop_back() {
            std::lock_guard<std::mutex> lg(m);
            T result = q.front();
            q.pop_front();
            return result;
        }

    private:
        mutable std::mutex m;
        std::deque<T> q;
    };

    template
    struct concurrent_queue<int>;

}

/*
 * используем optional
 */
namespace ex2 {
/*
 * Два потока, 1ый продуцирует данные, нам их надо передать во 2ой поток, чтобы он из обработал
 * Что то типа пайма, но для конкретного типа
 */

    template<typename T>
    struct concurrent_queue {
        void push(T value) {
            std::lock_guard<std::mutex> lg(m);
            q.push_back(std::move(value));
        }

        bool empty() const {
            std::lock_guard<std::mutex> lg(m);
            return q.empty();
            /*
             * Мы не знаем, как дек устроен, и + не можем работать с примитивными типами из разных потоков, поэтому все
             *  равно берем лок - иначе ub
             */
        }

        /*
         * Даже если мы перед поп бэком проверим на пустоту, то это не гарантирует, что мы получим объект.
         * Не достаточно навесить локов для какой то структуры
         */
        std::optional<T> try_pop() {
            std::lock_guard<std::mutex> lg(m);
            if (q.empty())
                return std::nullopt;
            T result = q.front();
            q.pop_front();
            return result;
        }

        /*
         * Хотим сделать ждущий поп, если пустая очередь, то хотим подождать, пока данные появятся
         * Крутиться в цикле и проверять, не пустые ли мы - плохо, потому что крутимся на ядре.
         * Хотим если очередь пустая, усыпить и дать другим потокам работать.
         *
         * Для этого есть специальный класс std::condition_variable
         */
    private:
        mutable std::mutex m;
        std::deque<T> q;
    };

}

/*
 * используем condition variable
 */
namespace ex3 {
    template<typename T>
    struct concurrent_queue {
        void push(T value) {
            std::lock_guard<std::mutex> lg(m);
            q.push_back(std::move(value));
//            cv.notify_all();
            cv.notify_one(); // пробуждает
        }

        bool empty() const {
            std::lock_guard<std::mutex> lg(m);
            return q.empty();
        }

        std::optional<T> try_pop() {
            std::lock_guard<std::mutex> lg(m);
            if (q.empty())
                return std::nullopt;
            T result = q.front();
            q.pop_front();
            return result;
        }

        T pop() {
            std::lock_guard<std::mutex> lg(m);
            /*   if (q.empty()){
                   cv.wait(); ждем на condition_variable, но мы заснули с мьютексом
               }*/
        }

    private:
        mutable std::mutex m;
        std::deque<T> q;
        std::condition_variable cv;
    };

    template
    struct concurrent_queue<int>;

}

namespace ex4 {
    template<typename T>
    struct concurrent_queue {
        void push(T value) {
            std::lock_guard<std::mutex> lg(m);
            q.push_back(std::move(value));
            cv.notify_one();
        }

        bool empty() const {
            std::lock_guard<std::mutex> lg(m);
            return q.empty();
        }

        std::optional<T> try_pop() {
            std::lock_guard<std::mutex> lg(m);
            if (q.empty())
                return std::nullopt;
            T result = q.front();
            q.pop_front();
            return result;
        }

        T pop() {
            std::unique_lock<std::mutex> lg(m);
            while (q.empty()) {
                lg.unlock();
                /*здесь просиходит без блокировки все что угодно, поэтому всегда, когда используют condition_variable, используют циклы
                 здесь может кто то сделать push, и тогда мы не получим уведомление.
                 и именно по этой причине wait не существует без аргумента. unlock and then wait -> always error
                 аргумент wait - unique lock*/
                cv.wait(lg);
                /*
                 * Т.е. wait отпускает блокировку и начинает ждать, его пробудили и он снова взял блокировку
                 */
                lg.lock();
            }
        }

    private:
        mutable std::mutex m;
        std::deque<T> q;
        std::condition_variable cv;

    };

    template
    struct concurrent_queue<int>;

}

namespace ex5 {
    template<typename T>
    struct concurrent_queue {
        void push(T value) {
            std::lock_guard<std::mutex> lg(m);
            q.push_back(std::move(value));
            cv.notify_one();
        }

        bool empty() const {
            std::lock_guard<std::mutex> lg(m);
            return q.empty();
        }

        std::optional<T> try_pop() {
            std::lock_guard<std::mutex> lg(m);
            if (q.empty())
                return std::nullopt;
            T result = q.front();
            q.pop_front();
            return result;
        }

        T pop() {
            std::unique_lock<std::mutex> lg(m);
            while (q.empty()) {
                cv.wait(lg);
            }
            /*
             * Конструкция цикл+ wait очень распространена, для нее сделали специальный синтаксис
             */
        }

    private:
        mutable std::mutex m;
        std::deque<T> q;
        std::condition_variable cv;
    };

    template
    struct concurrent_queue<int>;

}


namespace ex6 {
    template<typename T>
    struct concurrent_queue {
        void push(T value) {
            std::lock_guard<std::mutex> lg(m);
            bool was_empty = q.empty();
            q.push_back(std::move(value));
            /*  cv.notify_one();
             * Если у нас очередь на 1000 элементов, нотифаить каждый раз глупо.
             */
            if (was_empty) {
                cv.notify_one();
                /*
                 * Но тут тоже проблема, потому что мы нотифаим один поток, если очередь была пустой. А если у нас на
                 *  пустой очереди 100 ждет попов, то мы разбудем только одного из них
                 * Оптимизация не очень удачна, делать так не стоит
                 */
            }

        }

        bool empty() const {
            std::lock_guard<std::mutex> lg(m);
            return q.empty();
        }

        std::optional<T> try_pop() {
            std::lock_guard<std::mutex> lg(m);
            if (q.empty())
                return std::nullopt;
            T result = q.front();
            q.pop_front();
            return result;
        }

        T pop() {
            std::unique_lock<std::mutex> lg(m);
            cv.wait(lg, [&] {
                return !q.empty();
            });
            T result = q.front();
            q.pop_front();
            return result;
        }

    private:
        mutable std::mutex m;
        std::deque<T> q;
        std::condition_variable cv;
    };

    template
    struct concurrent_queue<int>;

}
/*
 * Если продьюсеры работают быстрее космьюмеров, то очередь будет неограниченно расти.
 *
 * Аналогия с линуксовыми пайпами - если косьюмер работает слишком быстро, то он ждет, если продьюсер - то он тоже ждет.
 *
 * Хотим иметь очередь, имеющую лимит на количество запихиваемых эелементов.
 */


namespace ex7 {
    template<typename T>
    struct concurrent_queue {

        bool try_push(T &&  value) { // 2 перегрузки, т.к. может быть дорого копировать, а мы возвращаем бул и можем
            // не запушить
            std::unique_lock<std::mutex> lg(m);
            if (q.size() == max_size)
                return false;
            q.push_back(std::move(value));
            lg.unlock();
            cv_empty.notify_one();
            return true;
        }
        bool try_push(T const &  value) {
            std::unique_lock<std::mutex> lg(m);
            if (q.size() == max_size)
                return false;
            q.push_back(value);
            lg.unlock();
            cv_empty.notify_one();
            return true;
        }

        void push(T value) {
            std::unique_lock<std::mutex> lg(m);
            cv_full.wait(lg, [&] { return q.size() != max_size; });
            q.push_back(std::move(value));
            lg.unlock();
            cv_empty.notify_one();
        }

        bool empty() const {
            std::lock_guard<std::mutex> lg(m);
            return q.empty();
        }

        std::optional<T> try_pop() {
            std::unique_lock<std::mutex> lg(m);
            if (q.empty())
                return std::nullopt;
            T result = q.front();
            q.pop_front();
            lg.unlock();
            cv_full.notify_one();
            return result;
        }

        T pop() {
            std::unique_lock<std::mutex> lg(m);
            cv_empty.wait(lg, [&] { return !q.empty(); });
            T result = q.front();
            q.pop_front();
            lg.unlock();
            cv_full.notify_one();
            return result;
        }

    private:
        mutable std::mutex m;
        std::deque<T> q;
        std::condition_variable cv_empty;
        std::condition_variable cv_full;
        size_t max_size = 1000;
    };


    template
    struct concurrent_queue<int>;

}

/*
 * Как это все сделано внутри?
 *
 * и condition_variable и mutex должны уметь спать, поэтому нужна поддержка со стороны ОС. Только у нее есть возможность
 *  усыпыть.
 *
 * Иногда при игрушечных реализациях используют sleep или yield(функция говорит, что моему потоку делать нечего,
 *  переключись на другой)
 *
 * ОС может посмотреть на другие потоки и сказать, что не хочет переключаться и делать что то другое. Это может быть
 *      связано с тем, что нам например заново надо будет при переключении на другой прроцесс заполнять кэш, и процессор
 *      будет работать медленно. ОС старается такие вещи минимизировать. И на самом деле yeild вообще может
 *      игнорироваться ОС, хотя программист думает что там что то перераспределиться
 * В ОС есть специальные фукнции, которые говорят "я собираюсь ждать такую то вещь". В линуксе это futex(fast user mutex)
 *      Говорим, что хотим ждать на такой адрес памяти. Пробуди такого то чувака, который ждет на этом адресе памяти.
 *
 *      Такое засыпание требует поддержки со стороны ОС и надо говорить, что мы сейчас ждем, чтобы планировщик принимал
 *          нормальные решения
 *
 * Когда мы говорим о мьетексе, политика переключения на другой поток(попытались залочить, он оказался залоченным,
 *  давайте переключимся), может работать плохо(переключение на другой поток занимает какое то время, и может под
 *  мьютексом исполняется мало кода(чуть подождем и разлочат). И тогда в реализации мьютекса мы некоторое время крутимся
 *  в цикле, проверяя, может мьютекс уже разлочен, и если все таки нет, то просим у ОС переключиться.(это то, как
 *  устроены мьютексы)
 *
 *  Например на x86 есть инструкция процессору, чтобы если мы в цикле что то проверяем, то намекнуть ему, чтобы
 *      сбавил скорость и чекал, если что то в памяти изменится - pause (spin loop hint)
 *
 * Про то, что мы не можем многопоточно читать и писать инты:
 *      для разных архитектур процессора разные операции дают разные гарантии. На x86 все примерно выполняется последо-
 *      вательно, однако на arm или elf вообще может все сильно переставляться. Поэтому какие то строгие гарантии, как
 *      на x86 заставляли бы замелять процессоры, которые, возможно вообще многопоточку не используют.
 * Так же это влияет на то, какие оптимизация может делать компилятор.
 *
 * Так же, не любую переменную мы можем модифицировать атомарно. Атомарность - другие потоки не видят промежуточного
 *      состояния переменной.
 *
 *      Классический пример
 *         практически всегда мы хотим работать с выровненными переменными. Особенно это важно, когда переменная
 *          начинает пересекать границу кэш линии. В этом случае для процессора эффективно реализовать эту вещь может
 *          быть проблематично. Т.е. у нас переменная лежит между двумя кэш линиями и мы хотим ее поменять атомарно,
 *          такое может делать только x86, но там изза этого много ограничений, в плоть до того, что другие ядра не
 *          могут обращаться к памяти.
 *
 *     Поэтому для переменных, с которыми мы хотим работать атомарно, важно, чтобы они были выравнены. По этой причине
 *      в С++11 если нам нужно работать с интом из нескольких потоков, используем std::atomic.
 *
 *      Они реализованы через какие то встренные функции компилятора или что то такое.
 *      Функции с std::atomic имеют какие то специальные(на некоторых архитектурах) инструкции процессору
 *
 *
 * Важный момент.
 *
 * Инструкции процессора и операции С++ с atomic предоставляют какие то гарантии, но нельзя затачиваться на то, во что
 *  транслируется код(например как тут https://wwdw.cl.cam.ac.uk/~pes20/cpp/cpp0xmappings.html описано), потому что
 *  все меняется от компилятора к компилятору. Например в x86 load-relaxed == load-seq-cst, поэтому буду испльзовать
 *  load-relaxed всегда, программа будет работать точно так же (Но это неправда!). Полагаемся только на гарантии плюсов,
 *  потому что можем еще наткнуться на оптимизации компилятора или компиляция под разные архитектуры зафейлится
 *
 *
 */

/*

 Вопрос по прошлой лекции, не понятно, что происходит с мьютексами, когда мы используем std::condition_variable::wait. У нас есть поток1, он держит мьютекс m и делает какое то действие, после которого нотифаит по cond_var, и допустим что дальше у него опять какие то действия, для которых нужно держать мьютекс(он просто уведомил и не отпускал мьютекс m). Поток2 находится в cond_var.wait(m), его уведомил Поток1 по cond_var. Теперь кто будет исполняться?
 Кто будет сидеть и ждать мьютекса? В потоке2 придет уведомление на cond_var и он в wait дождется, пока обратно не получит мьютекс? - верное предположение


 */

