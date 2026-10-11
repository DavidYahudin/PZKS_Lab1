// Лексическая и синтаксическая проверка математического выражения.
//
// Грамматика (БНФ):
//   expr    := term   { ('+' | '-') term }
//   term    := unary  { ('*' | '/') unary }
//   unary   := ('+' | '-') unary | power
//   power   := primary [ '^' unary ]
//   primary := NUMBER | IDENT | IDENT '(' expr ')' | '(' expr ')'
//
// Сборка:  g++ -std=c++17 -o expr_checker expr_checker.cpp

#include <cctype>
#include <iostream>
#include <string>
#include <vector>

// ======================= ЛЕКСЕР =======================

enum class TokType { Number, Ident, Plus, Minus, Mul, Div, Pow, LParen, RParen, End };

struct Token {
    TokType type;
    std::string text;
    size_t pos;  // позиция в исходной строке (с 0)
};

struct Error {
    size_t pos;
    std::string msg;
};

const char* typeName(TokType t) {
    switch (t) {
        case TokType::Number: return "число";
        case TokType::Ident:  return "идентификатор";
        case TokType::Plus:   return "'+'";
        case TokType::Minus:  return "'-'";
        case TokType::Mul:    return "'*'";
        case TokType::Div:    return "'/'";
        case TokType::Pow:    return "'^'";
        case TokType::LParen: return "'('";
        case TokType::RParen: return "')'";
        case TokType::End:    return "конец выражения";
    }
    return "?";
}

// Разбивает строку на токены. Лексические ошибки складывает в errors.
std::vector<Token> tokenize(const std::string& s, std::vector<Error>& errors) 
{
    std::vector<Token> tokens;
    size_t i = 0;
    while (i < s.size()) {
        unsigned char c = s[i]; ////??????

        if (std::isspace(c)) { ++i; continue; }

        if (std::isdigit(c) || c == '.') 
        {
            size_t start = i;
            bool dot = false, bad = false;
            while (i < s.size() && (std::isdigit((unsigned char)s[i]) || s[i] == '.')) 
            {
                if (s[i] == '.') 
                {
                    if (dot) bad = true;  // вторая точка: 1.2.3
                    dot = true;
                }
                ++i;
            }
            std::string num = s.substr(start, i - start);
            // "5." и ".5" считаем ошибкой: требуем цифры по обе стороны точки
            if (num.front() == '.' || num.back() == '.') bad = true;
            if (bad) errors.push_back({start, "некорректное число '" + num + "'"});
            else     tokens.push_back({TokType::Number, num, start});
            continue;
        }

        if (std::isalpha(c) || c == '_') {
            size_t start = i;
            while (i < s.size() && (std::isalnum((unsigned char)s[i]) || s[i] == '_')) ++i;
            tokens.push_back({TokType::Ident, s.substr(start, i - start), start});
            continue;
        }

        TokType t;
        switch (c) {
            case '+': t = TokType::Plus;   break;
            case '-': t = TokType::Minus;  break;
            case '*': t = TokType::Mul;    break;
            case '/': t = TokType::Div;    break;
            case '^': t = TokType::Pow;    break;
            case '(': t = TokType::LParen; break;
            case ')': t = TokType::RParen; break;
            default:
                errors.push_back({i, std::string("недопустимый символ '") + (char)c + "'"});
                ++i;
                continue;
        }
        tokens.push_back({t, std::string(1, (char)c), i});
        ++i;
    }
    tokens.push_back({TokType::End, "", s.size()});
    return tokens;
}

// ======================= ПАРСЕР =======================

// Рекурсивный спуск: на каждое правило грамматики - одна функция.
// При первой синтаксической ошибке бросаем исключение Error.
class Parser 
{
public:
    explicit Parser(const std::vector<Token>& t) : toks(t) {}

    void parse() {
        expr();
        // После полного выражения должен идти конец
        if (cur().type == TokType::RParen)
            throw Error{cur().pos, "лишняя закрывающая скобка ')'"};
        if (cur().type != TokType::End)
            throw Error{cur().pos, std::string("неожиданный токен: ") + typeName(cur().type)};
    }

private:
    const std::vector<Token>& toks;
    size_t p = 0;

    const Token& cur() const 
    { 
        return toks[p]; 
    }
    
    void next() { if (cur().type != TokType::End) ++p; }

    void expr() 
    {
        term();
        while (cur().type == TokType::Plus || cur().type == TokType::Minus) 
        {
            next();
            term();
        }
    }

    void term() 
    {
        unary();
        while (cur().type == TokType::Mul || cur().type == TokType::Div) {
            next();
            unary();
        }
    }

    void unary() {
        if (cur().type == TokType::Plus || cur().type == TokType::Minus) {
            next();
            unary();
        } else {
            power();
        }
    }

    void power() {
        primary();
        if (cur().type == TokType::Pow) {
            next();
            unary();
        }
    }

    void primary() {
        const Token& t = cur();
        switch (t.type) {
            case TokType::Number:
                next();
                return;
            case TokType::Ident:
                next();
                if (cur().type == TokType::LParen) {  // вызов функции: sin(x)
                    next();
                    expr();
                    expect(TokType::RParen, "ожидалась ')' после аргумента функции");
                }
                return;
            case TokType::LParen:
                next();
                expr();
                expect(TokType::RParen, "ожидалась закрывающая скобка ')'");
                return;
            case TokType::End:
                throw Error{t.pos, "неожиданный конец выражения, ожидался операнд"};
            default:
                throw Error{t.pos, std::string("ожидался операнд (число, переменная или '('), "
                                               "а найдено ") + typeName(t.type)};
        }
    }

    void expect(TokType type, const std::string& msg) {
        if (cur().type != type) throw Error{cur().pos, msg};
        next();
    }
};

// ======================= ПРОВЕРКА =======================

void printError(const std::string& src, const Error& e) {
    std::cout << "  " << src << "\n  " << std::string(e.pos, ' ') << "^\n";
    std::cout << "  Ошибка (позиция " << e.pos + 1 << "): " << e.msg << "\n";
}

bool check(const std::string& src) {
    std::vector<Error> lexErrors;
    std::vector<Token> tokens = tokenize(src, lexErrors);

    // Этап 1: лексический анализ
    if (!lexErrors.empty()) 
    {
        std::cout << "Лексические ошибки:\n";
        for (const Error& e : lexErrors) printError(src, e);
        return false;
    }

    // Этап 2: синтаксический анализ
    try 
    {
        Parser(tokens).parse();
    } catch (const Error& e) 
    {
        std::cout << "Синтаксическая ошибка:\n";
        printError(src, e);
        return false;
    }

    std::cout << "  Выражение корректно.\n";
    return true;
}

int main() {
    std::cout << "Введите выражение (пустая строка - выход):\n";
    std::string line;
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line) || line.empty()) break;
        check(line);
    }
    return 0;
}