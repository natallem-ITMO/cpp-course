
#include <string>
#include <vector>
#include <memory>
#include <iostream>

int main() {
    std::vector<std::string> v;
    v.push_back("sdf");//стандартные коллекции хранят копии объектов, создается string("sdf"), затем копируется
    //Про требования к классам, которые хранит вектор:  параметры коллекции должны быть копируемы(с конструктором),
    // деструктор, возможно для insert требурется также оператор присваивания
}
//когда вектор расширяется, то он копирует все свои элементы, в данном случае со строками копировались бы их буфферы(что излишне),
//можно было бы переставить сами указатели без переноса буфера

/*
void *memcpy(void *dest, const void *source, size_t count)
 Функция memcpy() копирует count символов из массива, на который указывает source, в массив, на который указывает dest.
 Если массивы перекрываются, поведение memcpy() не определено.

 Не можем использовать, т.к. мы могли передать this в какую то функции, и все сломается
 */
struct my_magic_type;

void bar(my_magic_type *);

struct my_magic_type {
    void foo() {
        bar(this);
    }
};

//что если тип некопируемый?
int main1() {
    std::vector<std::fstream> v; // по смыслу иметь вектор потоков нормально(но что такое копирование потока - не понятно)
    //проблемы с копированием -  как управлять указателем на чтение, когда закрывать файл
}

struct mytype {
    mytype();
    mytype(mytype const &other)//copy
    {
        size = other.size;
        capacity = capacity;
        data = (char *) malloc(capacity + 1);
        memcpy(data, other.data, capacity + 1);//в move делать не надо
    }

    mytype(mytype &other)//move
            : data(other.data), size(other.size), capacity(other.capacity) {
        other.data = nullptr;
        other.size = 0;
        other.capacity = 0;
    }

    mytype &operator=(mytype const &); //copy
    mytype move(mytype &); //move

private:
    char *data;
    size_t size;
    size_t capacity;
};

int main2() {
    std::vector<std::shared_ptr<std::fstream>> v; // решение? есть свои особенности, не совсем идентично
    //shared ptr плох, т.к. в случае сортировки
    //auto pivot = v[i] // зануляло весь массив
    //#check когда делаем свой класс, надо учитывать предсказуемую работу строки выши
}

/*
 * move'ов очень много в программе, нужен механизм языка, который делал бы их автоматически в уместных местах
 * И это то, что позволяют делать rvalue ссылки
 * В С++11 появился новый вид ссылок
 * обычные lvalue ссылки не могут билдиться к rvalue ссылке
 */

template<class T>
struct vector {
    //две функции создаем в случае, если захотим потом перемещать. для ф-ции size это не нужно
    //в подавляющем большинстве
    void push_back(T const &) {
        std::cout << "void push_back(T const &)\n";
    }//copy
    void push_back(T &&) {
        std::cout << "void push_back(T &&)\n";
    };//move
};

int main3() {
    int a;
    int &b = a;
    //int &c = 32; CE (bind to rvalue)

    //правила такие же как для обычных ссылок
    // int&& d = a; CE (bind to lvalue)
    int &&e = 42;

    vector<int> v;
    v.push_back(42);//void push_back(T&&);        (***)
    v.push_back(a);//void push_back(T const &)
    //можем выбирать поведение для разных типов
}

struct mytype4 {
    mytype4(mytype4 const &);//copy
    mytype4(mytype4 &&);//move конструктор

    mytype4 &operator=(mytype4 const &);//copy
    mytype4 &operator=(mytype4 &&);//move оператор присваивания && - один токен, нельзя разбить
};

template<typename T>
T &&move(T &obj) {
    return static_cast<T &&>(obj);
}

struct person {
    person(std::string const &name) : name(name) {}
    //std::string const && move(std::string const &)
    // не сможем вызвать rvalue оператор присваивания для string для name т.к. move вернет const объект, который
    // передасться только в lvalue operator string'a

    person(std::string &&name)
            : name(move(name)) {
        name;
        name;
        name; // для вызывающего конструктор name - rvalue. Внутри нашей фукнции это lvalue;
        // чтобы помувать в name ее надо явно как то сделать lvalue(для после :)
        // есть специальная функция, которая позволяет явно мувать
    }

    person(std::string name, std::string surname)
            : name(move(name))//if name rvalue -> move
            , surname(move(surname)) //if name lvalue -> копию сделала вызывающая функция
            //минус - возможен вызов доп. move конструктора (если у нас было lvalue и мы его скопировали потом мувнули)
    {}

    std::string name;
    std::string surname;
};

struct mytype5 {
    mytype5(int);

    mytype5();

    mytype5(int, int, int);
};

int main4() {
    std::vector<mytype5> v;
    v.push_back(5);//вызывается конструктор, создается объект а потом вызывается push_back от rvalue
    //сначала создается временный объект, а потом делается реаллокация, если не помещается. Поэтому автоматически
    // вызывать конструктор на месте нельзя
    v.emplace_back(4, 5, 2);//вызов конструктора элемента внутри вектора
}

//мой вопрос понимает ли что надо вызывать фукнцию от рвалью если у нас объект локальный временный? Нет, смотрит сугубо по ссылке

//в std если в move передать rvalue, он возвращает rvalue, в нашем move будет CE
//но в большинстве случаев это не нужно, т.к. в коде(кроме шаблонов), знаем что у нас lvalue

//if в person у нас было бы 2 поля, то получается, надо было бы делать несколько констукторов для каждого && or &
//констукция - когда rvalue - делает копию, иначе не делает - передача по значению

std::string&& foo(){//глупые, подумали что произойдет в return копирование,поэтому решили вернуть rvalue ссылку.
    std::string res;
    res="afd";
    return std::move(res);//замували, чтобы сошлось по типам, но вернем уже непоянтно что, т.к. это будет ссылка на
    // локальный объект, котоорый при выходе из фукнции удалится
}
//возвращать rvalue ссылки практически некогда не хотим.
//даже если NRVO не сработал, то возвращая локальную переменную, вызывается move конструктор

//xvalue reference
mytype& lvalue();
mytype prvalue();
mytype&& xvalue();// overloading: prvalue
                  // copy elision: lvalue
void bar(mytype const &);
void bar(mytype &&);

mytype test()
{
    bar(xvalue());//call void bar(mytype &&); act as rvalue
    return xvalue();//as lvalue, no RVO optimization(need to copy or smth)
}

/*
 * copy elision различает prvalue and glvalue
 * overloading различает lvalue and rvalue
 * xvalue посередине
 *                          expression
 *                          /        \
 *                     glvalue    rvalue
 *                     /     \    /    \
 *                lvalue     xvalue    prvalue
 */

void test1(){

    mytype const & a = prvalue();//казалось бы ошибка - сслыка на rvalue, но правило -  биндим к ссылке, то rvalue живет
    // столько, сколько живет ссылка

    mytype && b = prvalue();//то же правило
    //^ зачем такое надо? например, есть фукнция, она возвращает ссылку &&, но если код поменяют и функция начнет
    // возвращать значение (prvalue), код не сломается

    mytype const & c = lvalue(); //не можем продлить время жизни, не контролируем объект из функции
    int const & d = 10;//подлили время жизни 10)
    mytype5 ff(d); // заиспользовали 10
}

struct mytype6
{
    mytype6(mytype6&& other);
    //мы должны оставить объект в согласованном состоянии. Для конкретных типов знаем, но что делать с шаблонным типом,
    // который мы не знаем?
    //зачем приводить к нулевому типу? например очищать строку? вдруг она у нас вся в памяти. лишние операции не нужны

    //если делаем move для стандартного класса, явно потом приводим в пустое состояние
};

int main5(){
    std::vector<std::string> v;
    std::string s;
    v.push_back(std::move(s));
    s.clear();
}

//Сложности move
struct base_man{};

struct man : base_man{
    std::string name;
    std::string surname;
};



int main6(){
    man p;
    std::string n = std::move(p.name);
    //что делать с объектом p? он частично не валиден

    base_man & bm = p;
    base_man moved = std::move(bm);
    //what to do with that?
    //после мува объект может остаться в дефолтном состоянии
}