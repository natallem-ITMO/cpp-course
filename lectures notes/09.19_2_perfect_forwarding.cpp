#include <type_traits>
#include <memory>
/*
 * Perfect forwarding
 */

struct mytype
{
    mytype (double ,double ){};
    mytype(int,int,int){};
};

int main(){
    //make_shared должна все аргументы передать в конструктор
    auto p = std::make_shared<mytype>(1,3,5);
    auto q = std::make_shared<mytype>(1.,3.);

    //perfect_forwarding - хотим передать все аргументы в другую функцию

}
/*
 * Сложность - аргументы разные по количество
 *             аргументы разные по типу
 *
 */

void f(std::string const &){

}

template <typename T>
void g(T& a){
    f(a);
}

/*
 * Если g такого типа аргументов, то в f не получится передать:
 * g:     T                        T const &                  T&
 * f:    std::string const &        T&                         T
 *      (дорого копировать)     (снимает const)             нельзя принять rvalue
 *
 * С03 подход в 2 перегрузки(увеличивается 2^n по мере добавления аргументов)
 */
template <typename T> // и наверху
void g(T const & a) {
    f(a);
}
/*
 * Т.е. мы хотим сохранить объект как lvalue/rvalue/const/not const - perfect от этого(forwarding изза того что не меняем аргументы)
 */

/*
 * Про машинерию языка
 *
 * Ссылки схлопываются:
 */
typedef int& mytype1;
typedef mytype1& mytype2;

static_assert(std::is_same_v<mytype1, mytype2 >);

template <typename T>
int foo(T&){};
//foo<int&>() <=> foo(int& &) <=> foo(int&)
// & & -> &
// && & -> & // сделали специально, чтобы сохранялось lvalue or rvalue
// & && -> &
// && && -> &&

template <typename T>
void g_new(T && a) { // && - universal reference (rvalue reference + template parameter)
    f(a);// так мы передаем всегда как lvalue
    //простой способ это сделать
    f(static_cast<T&&>(a));
}
/*elementary froward func*/
template <typename T>
T&& forward(T& a){
    return static_cast<T&&>(a);
}
/*
 * Пробелема, что если вызывать forward(a) без указания <T> то просто задедьюсим и будем всегда возвращаеть T&& (т.е. это будет мув оператор)
 */

template <typename T>
struct type_identity{
    typedef T type;
};

template <typename T>
T&& forward(typename type_identity<T>::type& obj){
    return static_cast<T&&>(obj);
}
/*
 * тут уже не задедьюсим
 */
/*
 * Почему использовать perf.forw а не static_cast? потому что привычно для всех, явно показываем, что хотим сделать.
 */
/*
 * форвордим н разных аргументов разных типов Т
 *
 */

template <typename ... T>
void g(T&& ... args){
    f(forward<T>(args) ...);// в эф передаем все отфорварженные
    //f(g(args...)) => f(g(a0, a1, a2, ...))
    //f(g(args)...) => f(g(a0), g(a1) ,g(a2)...)
}