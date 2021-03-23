#include<unordered_set>
#include<list>
#include <unordered_map>

struct unit;
//Двусвязные списки
//Cписки, в отличии от массив используются крайне редко
/*
 * Удаление за О(1) малоэффективно, т.к. сначала надо найти элемент. Пробежка по списку медленнее чем по вектору, т.к.
 * можем потенциально на каждом ноде получить cash-miss, в памяти менее компактно лежат.
 * Для удаления за О(1) мало используются
 *
 * Операция splice хороша, но используется крайне редко
 * В некоторых С-шных программах все на списках. Удивительно.
 * 95% программ почти никогда не используют списки.
 * Но есть информация, из-за которой можно использовать списки эффективно и почему их используют в некоторых программах много.
 *
 * Пусть у нас есть unit'ы
 *
 */
/*struct unit{
    unit * next;
    unit * prev;

};*/

// мы должны хранить юниты в каком то множестве. Выделенные. Будем использовать наприме хэш таблицу

std::unordered_set<unit *> selected_units; //может можно что-то дешевле использовать?
//может можно просто списком или вектором обойтись? Выделять и добавлять могли бы. А развыделить юнит? Ни для листа, ни
// для вектора нельзя эффективно, так как сначала надо его найти.

//идея:
//1)
/*struct unit{
    unit * next;
    unit * prev;

    unit * selected_next;
    unit * seleted_prev;
};*/
/*
Вставить можем, перебрать можем, удалить тоже можем: пришел unit *, свяжем просто его соседей.
Т.е. теперь нам не нужно искать сам юнит в листе или векторе, если к нам пришла ссылка на него.

 Бонус:
 1) Нет аллокации/освобождения памяти при вставке/удалении. Т.к. надо было бы выделять память в хештаблице
 2) Возможность использовать двусвязный список вместо хеш-таблицы(более дешевая data structure,т.к. операции со списками
 делают меньше операции. Медленно работают списки, т.к. в памяти не равномерно находятся)
 3) Лучшая локальность ссылок при итерации по элементам (имеется в виду, что если бы мы закидывали в hash_map node *, то
    нам все равно бы пришлось обращаться в другую память для разыменовывания)
*/

//Альтернатива с std::list
//2)

/*struct unit{
    unit * next;
    unit * prev;

    std::list<unit *>::iterator backlink;
};

std::list<unit*> selected;*/

/*
 * В 1 у нас проигрыш по памяти если какие то юниты не используются(т.к. зря храним ссылки на selected next and prev)
 * Во 2 мы храним дополнительно в элементах листа value, который ссылается на сам node, и в ноде храним итератор.
 * Выгодно использовать 1 случай, если выделяем много элементов
 */

/*
 * Все сказанное обощается на другие структуры данных
 * **Двусвязные списки
 * **Деревья
 * **Хеш-таблицы, использующие списки для разрешения коллизий
 */

/*
 * LRU-cash - least recently used - функции foo и unordered_set cache. Хотим хранить в кэше не более 10000 значений например. Что делаем?
 * Есть политика выкидывания наименее используемого.
 */
struct Key {
};
struct Value {
};

Value foo(Key key);//return Value by this key

//std::unordered_map<Key, Value> cash_LRU;

//Сделаем это все просто сами:
struct node {
    Key key;
    Value value;

    //ordered by key in tree
    node *left;
    node *right;
    node *parent;

    //ordered by time access
    node *next;
    node *prev;
};

/*
 *  Если реализовывать с помощью unordered_set and list, то элементы из друг друга должны уметь ссылаться друг на друга
 *  Больше места займет это.
 *
 *  По сути мы противопоставляем разделение нодов на 3 штуки в разных контейнерах, и запихивание все в одну ноду.
 */

//Подходим напрямую к нашей теме.

/*
 * С-style подход
 */


/*
struct list_element {
    list_element *prev;
    list_element *next;
};

struct unit {
    list_element all_units_node;
    //...
    list_element selected_node;
};
 */

/*
 * В памяти будет что то типа
 *
 *                          unit
 *
 * |prev  |   next|        ....    |prev  |   next|
 * |all_units_node|                |selected_node |
 *
 * Как по all_units_node получить unit  понятно(кастуем просто, начала совпадают)
 * Как по selected_node получить unit?
 *
 * * В линксе макрос
   #define container_of(ptr, type, member) ({ \ передали ссылку на мембера, хотим вренуть ссылку на класс
    const typeof( ((type *)0)->member ) \
    *__mptr = (ptr);
    (type *)( (char *)__mptr - offsetof(type,member) );})
 *
 * В с++ есть static_cast
 *
 * |derived           |
 * |base |base2 |
 * |a    |b     |c    |
 * in godbolt.org
 */
struct base {
    int a;
};
struct base2 {
    int b;
};
struct derived : base, base2 {
    int c;
};

base & foo(derived & d)
{
    return d;
}

base2 & bar(derived & d)
{
    return d; // in mem return +4
}

derived & baz(base2 & d)
{
    return static_cast<derived&>(d);
}

/*  можем сделать
 *
 *           unit
 *
 * |prev  |   next|        ....    |prev  |   next|
 * |all_units_node|                |selected_node|
 *                                 Базовый класс
 *
 *  И потом кастовать.
 *
 *  Но есть сложность
 */
        /*struct list_element {
            list_element *prev;
            list_element *next;
        };

        struct unit: list_element*//*, list_element - если хотим быть в нескольких листах, но так нельзя*//*{
            list_element all_units_node;
            //...
            list_element selected_node;
        };*/
/*
 * Способы решения
 * насоздавать много разных стуктур, по ним отнаследоваться
 * сделать шаблонным
 */

template <class T>
struct list_element {
    list_element *prev;
    list_element *next;
};

struct all_units_tag{};
struct selected_tag; // incomplete class

struct unit : list_element<all_units_tag>, list_element<selected_tag>{};
/*
 * non-intrusive контейнеры хранят копию переданных данных. std::vector<T>/std::list<T> - это не интрузивные контейнеры.
 * Пользователь этих контейнеров не думают, как именно все внутри устроено.
 *
 *intrusive containers хранят указатель на данные. К примеру, интрузивный список может хранить указатель на начало и все.
 *А указатель на следующий/предыдущий элемент будет хранить сам "элемент".
 *
 * И если бы мы хотели пользоваться интрусив списком, то выглядело бы это как то так
 *
 * intrusive_list<unit, all_units_tag> all_units;
 * и внутри себя бы лист итерировался, используя базу этого тега
 *
 * container_of не безопаснее кастов, т.к. при передаче не объекта тип производно класса, а просто базового созданного,
 * у нас так же произойдет ошибка.
 *
 *
 * slicing problem не будет, т.к. по опр. она происходит при копировании, а у intrusive list не бывает копирования объектов
 *
 * Класс тега может быть неполным
Неполный тип — это тип, который описывает идентификатор, но не содержит информацию, необходимую для определения размера идентификатора. Неполным типом может быть:
Тип структуры, для которой еще не были определены члены.
Тип объединения для которого еще не были определены члены.
Тип массива, для которого еще не были определены размерности.
Тип void является неполным типом, который невозможно сделать полным. Чтобы дополнить неполный тип, укажите отсутствующие
 данные.

 * Можем даже так
    struct unit : list_element<struct all_units_tag>, list_element<struct selected_tag>{};
 *
 * В библиотеке boost есть реализация intrusive_list, можно с базой тэгами сделать
 *
 * При удалении юнита можно делать разное поведение для list_element
 * 1) ничего не делать(т.е. пользователь был обязан удалить элемент из списка, прежде чем его удалять самостоятельно
 * 2) save_hook - проверка, что мы удалились перед удалением
 * 3) есть хук, который проверяет, что мы не пытаемся юнит по одному хуку в несколько разных списков добавить
 * 4) auto_unlink hook
 * Плюсы минусы
 * https://www.boost.org/doc/libs/1_43_0/doc/html/intrusive/intrusive_vs_nontrusive.html
 * /

Почитать
http://www.codeofhonor.com/blog/tough-times-on-the-road-to-starcraft
https://www.data-structures-in-practice.com/intrusive-linked-lists/
https://www.boost.org/doc/libs/1_74_0/doc/html/intrusive/intrusive_vs_nontrusive.html
Примерные темы
 http://sorokin.github.io/cpp-course/3rd-sem-plan-draft



Буст класс который позволяет определять set с разными листами
 // define a multiply indexed set with indices by id and name
typedef multi_index_container<
  employee,
  indexed_by<
    // sort by employee::operator<
    ordered_unique<identity<employee> >,

    // sort by less<string> on name
    ordered_non_unique<member<employee,std::string,&employee::name> >
  >
> employee_set;
 https://www.boost.org/doc/libs/1_62_0/libs/multi_index/doc/tutorial/basics.html
*/