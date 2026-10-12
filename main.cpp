#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <algorithm>

// Типи токенів
//  Ident     - ідентифікатор (змінна)
//  Number    - константа
//  Pluss     - +
//  Minus     - -
//  Multyplie - *
//  Divide    - /
//  Fucntion  - ім'я функції (ідентифікатор, за яким іде '(')
//  RBacket   - (  (відкриваюча дужка)
//  LBacket   - )  (закриваюча дужка)
//  Comma     - ,
//  Unnown    - токен із лексичною помилкою (помилка вже занесена в список)
//  End       - кінець виразу
enum class T_Type { Ident, Number, Pluss, Minus, Multyplie, Divide, Fucntion, RBacket, LBacket, Comma, Unnown, End };

struct Token
{
    std::string text;
    T_Type type;
    int pos;
};

struct Error
{
    int pos;
    std::string message;
};

// Список усіх помилок виразу
class ErrorList
{
private:
    std::vector<Error> errors;

public:
    void add(int pos, const std::string& msg) { errors.push_back({pos, msg}); }

    bool empty() const { return errors.empty(); }

    void print(const std::string& source) const
    {
        std::vector<Error> sorted = errors;
        std::stable_sort(sorted.begin(), sorted.end(),
                         [](const Error& a, const Error& b) { return a.pos < b.pos; });

        std::cout << "Знайдено помилок: " << sorted.size() << "\n";
        for (size_t k = 0; k < sorted.size(); k++)
        {
            std::cout << k + 1 << ")  [Позиція " << sorted[k].pos + 1 << "] " << sorted[k].message << "\n"
                      << "    " << source << "\n"
                      << "    " << std::string(sorted[k].pos, ' ') << "^\n";
        }
    }
};

// ============================================================
//                    ЛЕКСИЧНИЙ АНАЛІЗ
// ============================================================
static bool isIdStart(char c) { return std::isalpha((unsigned char)c) || c == '_'; }
static bool isIdPart(char c)  { return std::isalnum((unsigned char)c) || c == '_'; }

std::vector<Token> lexemize(const std::string& input, ErrorList& errors)
{
    std::vector<Token> tokens;
    size_t i = 0;

    while (i < input.size())
    {
        unsigned char c = (unsigned char)input[i];

        // Пропустити пробіли
        if (std::isspace(c)) { i++; continue; }

        // Числа (цілі та дробові)
        if (std::isdigit(c) || c == '.')
        {
            size_t start = i;
            bool dot = false, bad = false;

            while (i < input.size() && (std::isdigit((unsigned char)input[i]) || input[i] == '.'))
            {
                if (input[i] == '.')
                {
                    if (dot) bad = true;   // друга крапка
                    dot = true;
                }
                i++;
            }

            // Число, "приклеєне" до букв: 2abc, 3x
            bool glued = false;
            if (i < input.size() && isIdStart(input[i]))
            {
                glued = true;
                while (i < input.size() && isIdPart(input[i])) i++;
            }

            std::string num = input.substr(start, i - start);

            if (glued)
            {
                errors.add((int)start, "Некоректне ім'я або число '" + num + "' (ім'я не може починатись із цифри)");
                tokens.push_back({num, T_Type::Unnown, (int)start});
            }
            else if (bad || num.front() == '.' || num.back() == '.')
            {
                errors.add((int)start, "Некоректний числовий запис '" + num + "'");
                tokens.push_back({num, T_Type::Unnown, (int)start});
            }
            else
            {
                tokens.push_back({num, T_Type::Number, (int)start});
            }
            continue;
        }

        // Ідентифікатори та функції
        if (isIdStart(input[i]))
        {
            size_t start = i;
            while (i < input.size() && isIdPart(input[i])) i++;
            std::string idnt = input.substr(start, i - start);

            if (idnt.front() == '_' || idnt.back() == '_')
            {
                errors.add((int)start, "Некоректне ім'я '" + idnt + "': символ '_' не може бути на початку або в кінці");
                tokens.push_back({idnt, T_Type::Unnown, (int)start});
                continue;
            }

            // Якщо далі (після пробілів) іде '(' - це функція
            size_t j = i;
            while (j < input.size() && std::isspace((unsigned char)input[j])) j++;
            T_Type t = (j < input.size() && input[j] == '(') ? T_Type::Fucntion : T_Type::Ident;

            tokens.push_back({idnt, t, (int)start});
            continue;
        }

        // Оператори та дужки
        T_Type t;
        switch (input[i])
        {
            case '+': t = T_Type::Pluss;     break;
            case '-': t = T_Type::Minus;     break;
            case '*': t = T_Type::Multyplie; break;
            case '/': t = T_Type::Divide;    break;
            case '(': t = T_Type::RBacket;   break;
            case ')': t = T_Type::LBacket;   break;
            case ',': t = T_Type::Comma;     break;
            default:
            {
                // Невідомий символ (для UTF-8 беремо всі байти символу)
                size_t start = i++;
                while (i < input.size() && (((unsigned char)input[i]) & 0xC0) == 0x80) i++;
                std::string sym = input.substr(start, i - start);
                errors.add((int)start, "Невідомий символ '" + sym + "'");
                tokens.push_back({sym, T_Type::Unnown, (int)start});
                continue;
            }
        }
        tokens.push_back({std::string(1, input[i]), t, (int)i});
        i++;
    }
    return tokens;
}


enum class State { Start, AfterOperand, AfterClose, AfterOp, AfterOpen, AfterComma, AfterFunc };

class FsmParser
{
private:
    struct Paren { int pos; bool isCall; };

    const std::vector<Token>& tokens;
    ErrorList& errors;
    State state = State::Start;
    std::vector<Paren> stack;
    Token last{"", T_Type::End, 0};   // останній оброблений токен

    static bool isOperand(T_Type t) { return t == T_Type::Number || t == T_Type::Ident; }
    static bool isAdd(T_Type t)     { return t == T_Type::Pluss || t == T_Type::Minus; }
    static bool isMul(T_Type t)     { return t == T_Type::Multyplie || t == T_Type::Divide; }

    static std::string q(const Token& t) { return "'" + t.text + "'"; }

    // Стан, у якому очікується операнд
    bool expectsOperand() const
    {
        return state == State::Start || state == State::AfterOp ||
               state == State::AfterOpen || state == State::AfterComma;
    }

    void missingOperator(const Token& t)
    {
        if (last.type == T_Type::Unnown) return;   // помилку вже знайшов лексер
        if (state == State::AfterClose)
            errors.add(t.pos, "Пропущено оператор між ')' та " + q(t));
        else
            errors.add(t.pos, "Пропущено оператор між " + q(last) + " та " + q(t));
    }

    // ---------- перехід за ')' ----------
    void onClose(const Token& t)
    {
        if (stack.empty())
        {
            errors.add(t.pos, state == State::Start
                ? "Вираз не може починатись із закриваючої дужки ')'"
                : "Зайва закриваюча дужка ')'");
            return;     // стан не змінюється
        }

        switch (state)
        {
            case State::AfterOpen:
                errors.add(t.pos, stack.back().isCall
                    ? "Порожній виклик функції: відсутній аргумент"
                    : "Порожні дужки '()'");
                break;
            case State::AfterComma:
                errors.add(t.pos, "Порожній аргумент функції перед ')'");
                break;
            case State::AfterOp:
                errors.add(last.pos, "Оператор " + q(last) + " перед ')' без операнда");
                break;
            default:
                break;
        }
        stack.pop_back();
        state = State::AfterClose;
    }

    // ---------- перехід за ',' ----------
    void onComma(const Token& t)
    {
        bool inCall = !stack.empty() && stack.back().isCall;
        if (!inCall)
        {
            errors.add(t.pos, "Кома поза викликом функції");
            return;     // стан не змінюється
        }

        if (state == State::AfterOpen || state == State::AfterComma)
            errors.add(t.pos, "Порожній аргумент функції перед комою");
        else if (state == State::AfterOp)
            errors.add(last.pos, "Оператор " + q(last) + " перед комою без операнда");

        state = State::AfterComma;
    }

    // ---------- перехід за оператором + - * / ----------
    void onOperator(const Token& t)
    {
        switch (state)
        {
            case State::AfterOperand:
            case State::AfterClose:
                state = State::AfterOp;
                break;

            case State::AfterOp:
                errors.add(t.pos, "Подвійна операція: " + q(t) + " після " + q(last));
                break;      // стан не змінюється

            case State::Start:
            case State::AfterOpen:
            case State::AfterComma:
                if (isAdd(t.type))
                    state = State::AfterOp;        // унарний знак
                else if (state == State::Start)
                    errors.add(t.pos, "Вираз не може починатись з оператора " + q(t));
                else if (state == State::AfterOpen)
                    errors.add(t.pos, "Оператор " + q(t) + " одразу після відкритої дужки");
                else
                    errors.add(t.pos, "Оператор " + q(t) + " одразу після коми");
                break;

            default:
                break;
        }
    }

    // ---------- перехід за операндом / функцією / '(' ----------
    void onOperandLike(const Token& t)
    {
        if (!expectsOperand())
            missingOperator(t);   // AfterOperand / AfterClose

        if (isOperand(t.type))
            state = State::AfterOperand;
        else if (t.type == T_Type::Fucntion)
            state = State::AfterFunc;
        else   // '('
        {
            stack.push_back({t.pos, false});
            state = State::AfterOpen;
        }
    }

public:
    FsmParser(const std::vector<Token>& toks, ErrorList& errs) : tokens(toks), errors(errs) {}

    // Один крок автомата
    void step(const Token& t)
    {
        if (t.type == T_Type::Unnown)
        {
            // Помилка вже в списку від лексера: вважаємо токен операндом,
            // але не створюємо додаткових помилок.
            state = State::AfterOperand;
        }
        else if (state == State::AfterFunc)
        {
            // Лексер гарантує, що після імені функції йде '('
            stack.push_back({t.pos, true});
            state = State::AfterOpen;
        }
        else switch (t.type)
        {
            case T_Type::Number:
            case T_Type::Ident:
            case T_Type::Fucntion:
            case T_Type::RBacket:  onOperandLike(t); break;
            case T_Type::LBacket:  onClose(t);       break;
            case T_Type::Comma:    onComma(t);       break;
            case T_Type::Pluss:
            case T_Type::Minus:
            case T_Type::Multyplie:
            case T_Type::Divide:   onOperator(t);    break;
            default: break;
        }
        last = t;
    }

    // Перевірка заключного стану
    void finish()
    {
        switch (state)
        {
            case State::AfterOp:
                errors.add(last.pos, "Вираз не може закінчуватись оператором " + q(last));
                break;
            case State::AfterOpen:
                errors.add(last.pos, stack.back().isCall
                    ? "Після '(' виклику функції відсутній аргумент"
                    : "Після '(' відсутній операнд");
                break;
            case State::AfterComma:
                errors.add(last.pos, "Після коми відсутній аргумент функції");
                break;
            default:
                break;
        }
        for (const Paren& p : stack)
            errors.add(p.pos, "Відсутня закриваюча дужка для цієї '('");
    }

    void parse()
    {
        if (tokens.empty())
        {
            errors.add(0, "Порожній вираз");
            return;
        }
        for (const Token& t : tokens) step(t);
        finish();
    }
};

int main()
{
    std::string input;
    std::cout << "Введіть рядок: ";
    std::getline(std::cin, input);

    ErrorList errors;
    std::vector<Token> tokens = lexemize(input, errors);

    FsmParser parser(tokens, errors);
    parser.parse();

    if (errors.empty())
        std::cout << "Помилки відсутні :)" << std::endl;
    else
        errors.print(input);

    return 0;
}