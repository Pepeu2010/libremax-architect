#include "expression.h"
#include <cctype>
#include <cmath>
#include <stdexcept>
#include <string>

namespace lmx {
namespace {
class Parser {
    std::string text;
    std::size_t pos = 0;
    int depth = 0;
    void space() {
        while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos])))
            ++pos;
    }
    bool eat(char c) {
        space();
        if (pos < text.size() && text[pos] == c) {
            ++pos;
            return true;
        }
        return false;
    }
    double atom() {
        if (++depth > 32)
            throw std::invalid_argument("Expressão complexa demais");
        double v = 0;
        if (eat('+'))
            v = atom();
        else if (eat('-'))
            v = -atom();
        else if (eat('(')) {
            v = sum();
            if (!eat(')'))
                throw std::invalid_argument("Parêntese não fechado");
        } else {
            space();
            auto start = pos;
            while (pos < text.size() &&
                   (std::isdigit(static_cast<unsigned char>(text[pos])) || text[pos] == '.'))
                ++pos;
            if (start == pos)
                throw std::invalid_argument("Número esperado");
            auto number = text.substr(start, pos - start);
            std::size_t used = 0;
            v = std::stod(number, &used);
            if (used != number.size())
                throw std::invalid_argument("Número inválido");
        }
        --depth;
        return v;
    }
    double product() {
        double v = atom();
        for (;;) {
            if (eat('*'))
                v *= atom();
            else if (eat('/')) {
                double d = atom();
                if (d == 0)
                    throw std::invalid_argument("Divisão por zero");
                v /= d;
            } else
                return v;
        }
    }
    double sum() {
        double v = product();
        for (;;) {
            if (eat('+'))
                v += product();
            else if (eat('-'))
                v -= product();
            else
                return v;
        }
    }

  public:
    explicit Parser(std::string_view s) : text(s) {
        if (s.size() > 128)
            throw std::invalid_argument("Expressão longa demais");
        for (char &c : text)
            if (c == ',')
                c = '.';
    }
    double run() {
        double v = sum();
        space();
        if (pos != text.size() || !std::isfinite(v) || std::abs(v) > 1e7)
            throw std::invalid_argument("Expressão inválida ou fora do limite");
        return v;
    }
};
} // namespace
double evaluate(std::string_view text) {
    return Parser(text).run();
}
} // namespace lmx
