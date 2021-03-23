#include <iostream>
#include <functional>
#include <cassert>
#include <memory>
#include <utility>
#include <vector>
//signal or listener or observers

/*
 * Unix-signals другая вещь.
 *
 * Сигналы - механизм, как то реализующийся в библиотеке, который позволяет нескольким независимым чувакам подписаться
 * на событие и получать уведомление о том что данное событие произошло.
 *
 * std::function кого то вызывает. Мы можем засунуть ее в какую то функцию и чувак будет ее вызывать, когда какое то
 * событие будет случаться. В каком то смысле сигнал - то же самое, но засунуть туда можно сразу n функций.
 *
 * Что за события могут быть?
 * Нажали кнопку, таймер истек(вызов всех заинтересованных сторон при истечении времени), изменение какой то переменной
 * (например изменилось какое то состояние)
 *
 * Сигналы в очень упрощенном представлении - вектор функций.
 */
namespace ex1 {
    struct signal {
        using slot_t = std::function<void()>; // слот - чувак, которого вызывают.

        signal() = default;

        void connect(slot_t slot) {
            slots.push_back(std::move(slot));
        }

        void operator()() const {// эмитит сигнал(т.е.вызывает все слоты)
            for (slot_t const &slot : slots) {
                slot();
            }
        }

    private :
        std::vector<slot_t> slots;
    };
}

/*
 * Часто сигналы используют не совсем там, где это необходимо. Т.е. люди пытаются как то обобщить код
 * Когда событие срабатывает, нужно передать какой то обработчик. Достаточно function. Но давайте обобщим, что если у
 * нас будет n незавимых чуваков, поэтому давайте обобщим и сделаем сигнал. Делают сигнал даже в тех местах
 * где по смыслу обработчик только один. Допустим в сокет идут данные, и событие что пришли данные нужны только одному
 * чуваку, т.к. только один может прочитать эти данные, остальные если независимые, то не могут читать из одного потока.
 * Примерно так же, как с shared_ptr, когда используют чуть шире, чем действительно они нужны.
 *
 * Как можно было бы сделать дизконнект?
 * Что точно не сработает.
 * Адрес слота который в векторе, и который приходит в фукнцию connect разные.
 *      (тк мы муваем функцию) Поэтому сравшение указателей не сработает
 * Идея - возвращать айди для слотов
 * Идея - использовать двусязный список
 */

/* использование айдишника */
namespace ex2 {
    struct signal {
        using id_t = uint64_t;
        using slot_t = std::function<void()>; // слот - чувак, которого вызывают.

        struct connection {
            connection() = default;

            connection(signal *sig, id_t id) : sig(sig), id(id) {}

            void disconnect() {
                sig->slots.erase(id);
            }

        private:
            signal *sig;
            id_t id;
        };

        signal() = default;

        signal(signal const &) = delete;

        signal &operator=(signal const &) = delete;

        connection connect(slot_t slot) {
            id_t id = next_id++;
            slots.insert({id, std::move(slot)});
            return connection{this, id};
        }

        void operator()() const {// эмитит сигнал(т.е.вызывает все слоты)
            for (auto const &p : slots) {
                p.second();
            }
            /* Перепишем в нормальный фор:
            for (auto i = slots.begin(); i != slots.end();+ i++) {// обратимся к невалиндному итератору после удаления
                i -> second();
            }
            for (auto i = slots.begin(); i != slots.end();) {
                auto j = std::next(i); // попробуем сохранить следующий итератор(если нас удалят)
                i -> second();
                i=j;
            Все может быть плохо в случае, если мы во время вызова может отписать другого чувака, итератор не валиден*/
        }
        /* Во всех списочных контейнерах при удалении элемента итератор на него инвалидируется. Замена unordered_map
         на map не работает.
         */

        /* Решение проблемы как копирвание очереди слотов имеет пробелму, что будем вызывать тех чуваков что отписались уже.
         * А если сделать некую очередь удаления, вносить в неё тех, кто хочет удалиться во время обхода, а после
         * вызова всех подписчиков чистить тех, кто в очереди? -  Та же самая проблема
         * В библиотеках с такого рода организацей проблема в том, что если мы коннектимся в конструкторе, а
         * дисконетимся в деструкторе, то т.к. нас могут вызвать после дисконекта, мы не можем удаляться.
         * Казалось бы, дисконнект - но одно событие может еще прийти(не логично).
         */
    private :
        id_t next_id = 0;
        std::unordered_map<id_t, slot_t> slots;
    };

/*
 * Во многих кейсах этот класс невозможно будет применять, он не поддерживает частный кейс
 * Может мы хотим вызвать поднабор функций?
 *      Нет. Почему мы складывает все слоты в вектор или мап? Потому что мы не различаем слоты. Иначе бы почему нам
 *      не использовать 3 function или 2 сигнала. Тот кто использует сигнал, ничего не знает про слоты.
 *
 * Проблема
 */

    struct timer {

        signal::connection connect_on_timer(signal::slot_t slot) {
            return on_timeout.connect(std::move(slot));
        }

    private :
        signal on_timeout;
    };

    struct user {
        user(timer &global_timer) : global_timer(global_timer) {};

        void foo() {
            conn = global_timer.connect_on_timer([this] { timer_elapsed(); });
        }

        void timer_elapsed() {
            conn.disconnect();//нас вызывали когда событие произошло, и мы удаляем наш слот во время прохода по loop
            //do smth
        }

    private:
        timer &global_timer;
        signal::connection conn;
    };

    namespace ex3 {
        struct user {
            user(timer &global_timer) : global_timer(global_timer),
                                        conn(global_timer.connect_on_timer([this] { timer_elapsed(); })) {};

/* (***)
 * чтобы решить проблему с вызовом после отписки передаем вместо this какой то объект, который засунули в шаред птр, т е
 * засовывают его и в слот, и сами его хранят, и в этом объекте булевский флаг отписались мы или нет.
 * В тот момемнт когда отписываемся выставляем флаг и говорим дисконнект.
 *
 * В некоторых случаях делают счетчик ссылок на сам user объект. Но если сам объект - сигнал, то там короче ужас(я не
 * что Ваня имел в виду, с чем то сталкивался на работе таком, но ладно, идем дальше)
 */
            virtual ~user() {
                conn.disconnect(); /* если мы знаем, что нас могу вызвать после дисконнект, то тогда не имеет право
                делать это в деструкторе. Сказали дисконнект - но еще может какое то событие прийти.
                Чтобы решить эту проблему, см (***)
                */
            }

            void timer_elapsed() {
                //do smth
            }

        private:
            timer &global_timer;
            signal::connection conn;
        };
    }
}

/*
           * можно сделать мапу disconnected[id] -> bool и не трогать удалённых, а в конце обхода почистить от
           * disconnected'ов
           * Но сделаем даже лучше. Если у нас std::function empty, то это значит что удалили
           */
namespace ex4 {
    struct signal {
        using id_t = uint64_t;
        using slot_t = std::function<void()>;

        struct connection {
            connection() = default;

            connection(signal *sig, id_t id) : sig(sig), id(id) {}

            void disconnect() {
                auto it = sig->slots.find(id);
                assert(it != sig->slots.end());
                if (sig->inside_emit) {
                    it->second = slot_t();
                } else {
                    sig->slots.erase(it);
                }

            }

        private:
            signal *sig;
            id_t id;
        };

        signal() = default;

        signal(signal const &) = delete;

        signal &operator=(signal const &) = delete;

        connection connect(slot_t slot) {
            assert(slot);//проверка на не пустоту слота - пустой - удаленный
            id_t id = next_id++;
            slots.insert({id, std::move(slot)});
            return connection{this, id};
        }

        void operator()() const {// эмитит сигнал(т.е.вызывает все слоты)
            inside_emit = true;
//            используем flag, потому что без него если мы захотим дисконектится вне эммита, то у нас не прочистится мапа
            try {
                for (auto i = slots.begin(); i != slots.end(); ++i)
                    if (i->second)
                        i->second(); // в случае нового коннекта мы не получим невалидный итератор(листы с этим норм работают)
            } catch (...) {
                leave_emit();
                throw;
                /* стоит создать инвариант, что если мы не находимся в эммите, то у нас нет пустых слотов.*/
            }
            leave_emit();

        }

    private :

        void leave_emit() const noexcept {
            inside_emit = false;               // безопасность с точки зрения исключений! Восстанавливаем флаги!
            for (auto i = slots.begin(); i != slots.end(); ++i) {
                if (i->second)
                    ++i;
                else
                    i = slots.erase(i);
            }
        }

        id_t next_id = 0;
        mutable std::unordered_map<id_t, slot_t> slots;
        mutable bool inside_emit = false;
    };
}

/*поговорим теперь про удаление сигнала во время эммита.
 * Это даже не очень безумное использование сигнала
 * */
namespace ex5 {
    struct signal {
        using id_t = uint64_t;
        using slot_t = std::function<void()>;

        struct connection {
            connection() = default;

            connection(signal *sig, id_t id) : sig(sig), id(id) {}

            void disconnect() {
                auto it = sig->slots.find(id);
                assert(it != sig->slots.end());
                if (sig->inside_emit) {

                    it->second = slot_t();
                } else {
                    sig->slots.erase(it);
                }

            }

        private:
            signal *sig;
            id_t id;
        };

        signal() = default;

        signal(signal const &) = delete;

        signal &operator=(signal const &) = delete;

        connection connect(slot_t slot) {
            assert(slot);//проверка на не пустоту слота - пустой - удаленный
            id_t id = next_id++;
            slots.insert({id, std::move(slot)});
            return connection{this, id};
        }

        void operator()() const {
            inside_emit = true;
            try {
                for (auto i = slots.begin(); i != slots.end(); ++i)
                    if (i->second)
                        i->second();
            } catch (...) {
                leave_emit();
                throw;
            }
            leave_emit();

        }

    private :

        void leave_emit() const noexcept {
            inside_emit = false;               // безопасность с точки зрения исключений! Восстанавливаем флаги!
            for (auto i = slots.begin(); i != slots.end(); ++i) {
                if (i->second)
                    ++i;
                else
                    i = slots.erase(i);
            }
        }

        id_t next_id = 0;
        mutable std::unordered_map<id_t, slot_t> slots;
        mutable bool inside_emit = false;
    };

    struct timer {
        timer(unsigned timeout_ms);

        signal::connection connect_on_timer(signal::slot_t slot) {
            return on_timeout.connect(std::move(slot));
        }

    private :
        signal on_timeout;
    };

    struct user {

        user() {};

        //пусть мы сами завели таймер
        void foo() {
            timer.reset(new struct timer(100));
            timer->connect_on_timer([this] { timer_elapsed(); });
        }

        ~user() {}

        void timer_elapsed() {
            //do smth and delete timer -> delete signal
            timer.reset();
        }

    private:
        std::unique_ptr<timer> timer;
    };
}

/*
 * Про рекурсивные эммиты
 */
namespace ex6 {
    struct signal {
        using id_t = uint64_t;
        using slot_t = std::function<void()>;

        struct connection {
            connection() = default;

            connection(signal *sig, id_t id) : sig(sig), id(id) {}

            void disconnect() {
                auto it = sig->slots.find(id);
                assert(it != sig->slots.end());
                if (sig->inside_emit) {

                    it->second = slot_t();
                } else {
                    sig->slots.erase(it);
                }

            }

        private:
            signal *sig;
            id_t id;
        };

        signal() = default;

        signal(signal const &) = delete;

        signal &operator=(signal const &) = delete;

        connection connect(slot_t slot) {
            assert(slot);//проверка на не пустоту слота - пустой - удаленный
            id_t id = next_id++;
            slots.insert({id, std::move(slot)});
            return connection{this, id};
        }

        /* Problem in previous version if we have inside_emit as bool
         * emit
         *      slot1
         *      slot2
         *      slot3
         *      slot4
         *              ...
         *                  emit
         *                      slot1
         *                      slot2
         *                      slot3
         *                      ...
         *                      leave emit
         *                          erase
         *      slot5 // error invalid iterator
         *      slot6
         *
         * Solutions -> inside_emit as counter
         *
         */
        void operator()() const {
            ++inside_emit;
            try {
                for (auto i = slots.begin(); i != slots.end(); ++i)
                    if (i->second)
                        i->second();
            } catch (...) {
                leave_emit();
                throw;
            }
            leave_emit();
            /*
             * Но что делать, если мы удаляем сам сигнал в этой операции. Мы не можем завести переменную is_destroyed
             * т.к. она сама удалится, но надо быстро продетектить что нас удалили и выйти из функции
             * Продетектить, удален ли объект вызвав ++i и поймать ошибку нельзя, т.к. мы просто обратимся к невалидной
             * памяти, это будет undefined behaviour
             *
             * Что делают?
             *
             * 1) Часть объекта, которую использует operator()() выделяют в отдельный объект и хранят на нее
             * шаред поинтер в самом сигнале и в когда заходят в эмит, то делают копию шаред поинтера.
             *
             * 2) Без шаред поинтеров
             * см ex7
             */
        }

    private :

        void leave_emit() const noexcept {
            --inside_emit;
            if (inside_emit == 0)
                for (auto i = slots.begin(); i != slots.end(); ++i) {
                    if (i->second)
                        ++i;
                    else
                        i = slots.erase(i);
                }
        }

        id_t next_id = 0;
        mutable std::unordered_map<id_t, slot_t> slots;
        mutable size_t inside_emit = 0;
    };
}

namespace ex7 {
    struct signal {
        using id_t = uint64_t;
        using slot_t = std::function<void()>;

        struct connection {
            connection() = default;

            connection(signal *sig, id_t id) : sig(sig), id(id) {}

            void disconnect() {
                auto it = sig->slots.find(id);
                assert(it != sig->slots.end());
                if (sig->inside_emit) {

                    it->second = slot_t();
                } else {
                    sig->slots.erase(it);
                }

            }

        private:
            signal *sig;
            id_t id;
        };

        signal() = default;

        ~signal() {
            if (destroyed) {
                *destroyed = true;
            }
        }

        signal(signal const &) = delete;

        signal &operator=(signal const &) = delete;

        connection connect(slot_t slot) {
            assert(slot);//проверка на не пустоту слота - пустой - удаленный
            id_t id = next_id++;
            slots.insert({id, std::move(slot)});
            return connection{this, id};
        }

        void operator()() const {
            ++inside_emit;
            bool is_destroyed = false; /* добавили  обозначалку, что нас удалили, меняем в деструкторе */
            destroyed = &is_destroyed;

            try {
                for (auto i = slots.begin(); i != slots.end(); ++i) {
                    if (i->second) {
                        i->second();
                        if (is_destroyed) {
                            return;
                        }
                    }
                }

            } catch (...) {
                leave_emit();
                throw;
            }
            leave_emit();
        }

    private :

        void leave_emit() const noexcept {
            destroyed = nullptr;
            --inside_emit;
            if (inside_emit == 0)
                for (auto i = slots.begin(); i != slots.end(); ++i) {
                    if (i->second)
                        ++i;
                    else
                        i = slots.erase(i);
                }
        }

        id_t next_id = 0;
        mutable std::unordered_map<id_t, slot_t> slots;
        mutable size_t inside_emit = 0;
        mutable bool *destroyed = nullptr;
    };
}

//destroyed recursive
namespace ex8 {
    struct signal {
        using id_t = uint64_t;
        using slot_t = std::function<void()>;

        struct connection {
            connection() = default;

            connection(signal *sig, id_t id) : sig(sig), id(id) {}

            void disconnect() {
                auto it = sig->slots.find(id);
                assert(it != sig->slots.end());
                if (sig->inside_emit) {

                    it->second = slot_t();
                } else {
                    sig->slots.erase(it);
                }

            }

        private:
            signal *sig;
            id_t id;
        };

        signal() = default;

        ~signal() {
            if (destroyed) {
                *destroyed = true;
            }
        }

        signal(signal const &) = delete;

        signal &operator=(signal const &) = delete;

        connection connect(slot_t slot) {
            assert(slot);//проверка на не пустоту слота - пустой - удаленный
            id_t id = next_id++;
            slots.insert({id, std::move(slot)});
            return connection{this, id};
        }

        void operator()() const {
            ++inside_emit;
            bool *old_destroyed = destroyed;
            bool is_destroyed = false; /* добавили  обозначалку, что нас удалили, меняем в деструкторе */
            destroyed = &is_destroyed;

            try {
                for (auto i = slots.begin(); i != slots.end(); ++i) {
                    if (i->second) {
                        i->second();
                        if (is_destroyed) {
                            *old_destroyed = true;
                            return;
                        }
                    }
                }

            } catch (...) {
                leave_emit(old_destroyed);
                throw;
            }
            leave_emit(old_destroyed);
        }

    private :

        void leave_emit(bool *old_destoyed) const noexcept {
            destroyed = old_destoyed;
            --inside_emit;
            if (inside_emit == 0)
                for (auto i = slots.begin(); i != slots.end(); ++i) {
                    if (i->second)
                        ++i;
                    else
                        i = slots.erase(i);
                }
        }

        id_t next_id = 0;
        mutable std::unordered_map<id_t, slot_t> slots;
        mutable size_t inside_emit = 0;
        mutable bool *destroyed = nullptr;
    };
}

/*
 * Ваня призывает не городить самим сигналы, а пользоваться библиотечным - boost or qt signals
 * Они позволяют отписываться внутри слота или удалять сигнал внутри слота.
 * А если бы добавили многопоточку, то вообще еще больше подводных камней.
 * Обязательно изучить библиотеку, какие возможноти она предоставляет.(будут ли вызывать меня после дисконнекта и т.д.)
 *
 * Для конктретно своих целей можно ограничить использование. Например запретить рекурсивные эммиты, потребовать, чтобы
 * в момент удаления нельзя все слоты отписались, или нельзя удаляться во время эммита и т.д.
 * А те случаи, что не могут происходить заассертить.
 *
 * Reentrancy - семейство проблем, которые мы фиксили - мы находимся внутри одной функции, она вызывает другую
 *      функцию, которая портит наше состояние. (внутри эмита вызвали коннект, эмит сломался)
 *      Reentrancy используют в разных смыслах, в том числе когда говорят о функции, которая не готова вызываться
 *      рекурсивно, или вызывать дисконнект внутри эмита.
 *
 *      Сейчас про эту проблему говорят реже, но в 80х когда все держали в глобальных переменных, потому что рекурсивно
 *      нельзя вызвать функцию, она портила свои данные.
 *
 *      Сейчас реже пишут код на глобальных переменных, но эта проблема может остаться. Она возникает, когда мы вызываем
 *      чужой код.
 *          Например, разрабатываем свой класс, вызвали внешний класс. Вопрос: что от нас может вызывать другой код?
 *          Например, нас вызывают откуда то, какие мы функции можем вызывать у этого чувака?
 *
 *      Очень часто такие проблемы возникают, но их не называют reentancy. Например мы внутри функции которая считаешь
 *      хэш, полезли менять саму хэш таблицу, для которой считаем хэш. Тогда вставляем элемент, вычисляем хэш, лезем в
 *      таблицу... Вопрос скорее в том, что вы вообще хотите, если пишите такое.
 *      Но в общем случае не всегда это так очевидно глупо.
 *
 *      Вот мы сделали сигнал, который реентерабельный, защищает сам себя.
 *
 *      Но возможно, что мы используем наш реентерабельный сигнал внутри класса, который тоже должен защищать свои данные
 *      В этом случае то, что мы защитили внутренности нашего класса не гарантирует то, что у внешнего класса все будет
 *      хорошо.
 *
 *      А еще можно было не использовать мапу, а просто лист, и в качестве id использовать итератор.
 */
