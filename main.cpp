#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <cctype>
#include <algorithm>

//Token Type identefier
//Ident     - ідентифікатор
//Number    - константа
//Pluss     - +
//Minuss    - -
//Multyplie - *
//Divide    - /
//RBacket   - (
//LBacket   - )

enum class T_Type {Ident, Number, Pluss, Minus, Multyplie, Divide, Fucntion, RBacket, LBacket, Comma, Unnown, End};

//Клас Токену
struct Token
{    
    std::string text;
    T_Type type;
    int pos;
};

//Класс Помилки
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
    //Додати помилку
    void add(int pos, const std::string& msg)
    {
        errors.push_back({pos, msg});
    }

    bool empty() const { return errors.empty(); }

    // Підсумковий список
    void print(const std::string& source) const
    {
        std::vector<Error> sorted = errors;
        std::stable_sort(sorted.begin(), sorted.end(), [](const Error& a, const Error& b){ return a.pos < b.pos; });
        
        std::cout << "Знайдено помилок: " << sorted.size() << "\n";
        for (int k = 0; k < sorted.size(); k++)
        {
            std::cout << k + 1 << ")";

            std::cout << "  [Позиція " << sorted[k].pos + 1 << "] " << sorted[k].message << "\n"    //текст помилки
                << "    " << source << "\n"                                                         //вівдит помилки
                << "    " << std::string(sorted[k].pos, ' ') << "^\n";                              //вивід показчика на помилку
        }
    }
};

//Функція розбиття рядка на токени, неправильні тоцени заповнюють ветор error лексичні помилки
std::vector<Token> lexemize (std::string input, ErrorList& errors)
{
    std::vector<Token> tokens;

    int i = 0;

    while (i < input.size()) 
    {   
        //Пропустити пробіли
        if (std::isspace(input[i])) 
        { 
            i++; 
            continue; 
        }

        //Перевірка на цілі та дробові числа
        if (std::isdigit(input[i]) || input[i] == '.') 
        {
            //Зберігаємо початкове значення і для пошуку цілого чилового токена
            int start = i;

            bool dot = false, bad = false;
            
            while (i < input.size() && (std::isdigit(input[i]) || input[i] == '.')) 
            {
                //шукаємо повторбванну точку
                if (input[i] == '.') 
                {
                    if (dot)
                    {
                        bad = true;         //Друга точка
                    }
                    dot = true;
                }
                i++;
            }

            std::string num = input.substr(start, i - start); //Зберігаємо числовий токен
            
            
            //Перевірка чилового токену на правильність, ЧИ є зайві точки та чи правильно вони поставленні
            if (num.front() == '.' || num.back() == '.') bad = true;

            // bad number
            if (bad) 
            { 
                errors.add(start, "Не коректний числовий вираз " + num);
                tokens.push_back({num, T_Type::Unnown, start}); 
            }
            else 
            {
                tokens.push_back({num, T_Type::Number, start});
            }
            
            continue;
        }

        //Первевірка на ім'я ідентифікатора чи функціїї
        if (std::isalpha(input[i])) 
        {
            int start = i;
            
            //finding the end of the identifire
            while (i < input.size() && (std::isalnum(input[i]) || input[i] == '_')) i++;

            std::string idnt = input.substr(start, i - start);
            
            //Чи коректтно поставленні _
            if(idnt.front() == '_' || idnt.back() == '_')
            {
                errors.add(start, "некоретне використанння _ у " + idnt);
                continue;
            }

            //перевірка на фунцію
            int j = i;
            T_Type t = T_Type::Ident;
            
            while(std::isspace(input[j]) && j < input.size()) j++;
            
            //Якщо є скожка - то це функція
            if (input[j] == '(') t = T_Type::Fucntion;

            //Повертення токену
            tokens.push_back({idnt, t, start});
            continue;
        }


        //Перевірка на оператори
        T_Type t;

        switch(input[i])
        {
            case '+': t = T_Type::Pluss; break;
            case '-': t = T_Type::Minus; break;
            case '*': t = T_Type::Multyplie; break;
            case '/': t = T_Type::Divide; break;
            case ')': t = T_Type::LBacket; break;
            case '(': t = T_Type::RBacket; break;
            case ',': t = T_Type::Comma; break;
            default:         
                errors.add(i, "Невідомий символ " + std::string(1, (char)input[i]));    
                tokens.push_back({std::string(1, input[i]), T_Type::Unnown, i});
                i++;
            continue;
            break;
        }
        tokens.push_back({std::string(1, (char)input[i]), t, i});

        //Нове коло
        i++;
    }

    return tokens;
}

//Класс Празер проводить синтактичний аналіз токенів за семантичним спуском
class Praser
{
    private:
    std::vector<Token> tokens;
    ErrorList& errors;
    size_t cur = 0;
    int endPos;
    int lastErrPos = -1;

    Token peek() const
    {
        if (cur < tokens.size()) return tokens[cur];
        return {"", T_Type::End, endPos};
    }

    Token prevToken() const
    {
        if (cur > 0 && cur <= tokens.size()) return tokens[cur - 1];
        return {"", T_Type::End, 0};
    }

    Token next()
    {
        Token t = peek();
        if (cur < tokens.size()) cur++;
        return t;
    }

    bool is(T_Type t) const { return peek().type == t; }

    static std::string describe(const Token& t)
    {
        if (t.type == T_Type::End) return "кінець виразу";
        return "'" + t.text + "'";
    }

    void report(int pos, const std::string& msg)
    {
        if (pos == lastErrPos) return;
        lastErrPos = pos;
        errors.add(pos, msg);
    }

    void reportMissingOperator()
    {
        Token t = peek(), p = prevToken();

        //Помилка вже була знайдена лексером - не дублюємо
        if (t.type == T_Type::Unnown || p.type == T_Type::Unnown) return;

        //2(t) - число не може бути іменем функції
        if (t.type == T_Type::LBacket && p.type == T_Type::Number)
        {
            report(p.pos, "Число '" + p.text + "' не може бути іменем функції");
            return;
        }
        report(t.pos, "Пропущено оператор перед " + describe(t));
    }

    void skipRepeatedOperators(const Token& prev)
    {
        while (is(T_Type::Pluss) || is(T_Type::Minus) ||
               is(T_Type::Multyplie) || is(T_Type::Divide))
        {
            Token t = next();
            report(t.pos, "Повторюваний оператор '" + t.text + "' після '" + prev.text + "'");
        }
    }

    //Зайві операнди та коми перед ')' (або перед ',' якщо це аргументи функції)
    void parseRest(bool allowComma)
    {
        while (!is(T_Type::RBacket) && !is(T_Type::End) &&
               !(allowComma && is(T_Type::Comma)))
        {
            if (is(T_Type::Comma))
            {
                report(peek().pos, "Кома поза викликом функції");
                next();
                if (!is(T_Type::RBacket) && !is(T_Type::End) && !is(T_Type::Comma))
                    parseExpression();
                continue;
            }
            reportMissingOperator();
            parseExpression();
        }
    }

    void expectClose(const Token& open)
    {
        if (is(T_Type::RBacket)) next();
        else report(open.pos, "Відсутня закриваюча дужка для '('");
    }

    void parseExpression()
    {
        parseTerm();
        while (is(T_Type::Pluss) || is(T_Type::Minus))
        {
            Token op = next();
            skipRepeatedOperators(op);
            parseTerm();
        }
    }

    void parseTerm()
    {
        parseUnary();
        while (is(T_Type::Multyplie) || is(T_Type::Divide))
        {
            Token op = next();
            skipRepeatedOperators(op);
            parseUnary();
        }
    }

    void parseUnary()
    {
        if (is(T_Type::Pluss) || is(T_Type::Minus))
        {
            Token sign = next();
            skipRepeatedOperators(sign);
        }
        parsePrimary();
    }

    void parsePrimary()
    {
        Token t = peek();
        switch (t.type)
        {
            case T_Type::Number:
            case T_Type::Ident:
            case T_Type::Unnown:    //помилка вже відома з лексера
                next();
                break;

            case T_Type::Fucntion:
                parseCall();
                break;

            case T_Type::LBacket:
                parseParen();
                break;

            default:
                report(t.pos, "Очікується операнд (число, змінна, функція або '('), а знайдено " + describe(t));
                break;
        }
    }

    //'(' expr ')'
    void parseParen()
    {
        Token open = next();
        parseExpression();
        parseRest(false);
        expectClose(open);
    }

    //name '(' [ expr { ',' expr } ] ')'   - порожні аргументи є помилкою
    void parseCall()
    {
        next();                 //ім'я функції
        Token open = next();    //'(' (лексер гарантує)

        while (true)
        {
            if (is(T_Type::Comma) || is(T_Type::RBacket))
                report(peek().pos, "Порожній аргумент функції");
            else if (!is(T_Type::End))
                parseExpression();

            parseRest(true);

            if (is(T_Type::Comma)) { next(); continue; }
            break;
        }
        expectClose(open);
    }

    public:
    Praser(const std::vector<Token>& toks, ErrorList& errs, int endPosition)
        : tokens(toks), errors(errs), endPos(endPosition) {}

    void parse()
    {
        if (tokens.empty()) { report(0, "Порожній вираз"); return; }

        while (true)
        {
            //Зайві ')' та ',' на верхньому рівні
            while (is(T_Type::RBacket) || is(T_Type::Comma))
            {
                Token t = next();
                report(t.pos, t.type == T_Type::RBacket
                    ? "Зайва закриваюча дужка ')'"
                    : "Кома поза викликом функції");
            }
            if (is(T_Type::End)) break;

            parseExpression();

            if (is(T_Type::End)) break;
            if (!is(T_Type::RBacket) && !is(T_Type::Comma)) reportMissingOperator();
        }
    }
};


int main()
{   
    ///ВВедення рядка
    std::string input;
    std::cout << "Введіть рядок: ";

    std::getline(std::cin, input);

    ErrorList errors;

    std::vector<Token> tokens = lexemize(input, errors);
    Praser parser(tokens, errors, (int)input.size());
    parser.parse();

    if (errors.empty())
        std::cout << "Помилки відсутні :)" << std::endl;
    else
        errors.print(input);

    return 0;
}