#pragma once
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cctype>
#include <stdexcept>

class FormulaParser {
public:
    FormulaParser() = default;

    bool setFormula(const std::string& formulaStr) {
        formula = "";
        for (char c : formulaStr) {
            if (!std::isspace(static_cast<unsigned char>(c))) {
                formula += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
        }
        return !formula.empty();
    }

    double evaluate(double xVal, double yVal) {
        x = xVal;
        y = yVal;
        pos = 0;
        try {
            double result = parseExpression();
            return std::isnan(result) || !std::isfinite(result) ? 0.0 : result;
        }
        catch (...) {
            return 0.0;
        }
    }

    bool validate(std::string& outErrorMessage) {
        pos = 0;
        x = 0.0;
        y = 0.0;
        try {
            parseExpression();
            if (pos < formula.length()) {
                outErrorMessage = "Syntax error: Extra characters at end: " + formula.substr(pos);
                return false;
            }
            return true;
        }
        catch (const std::exception& e) {
            outErrorMessage = e.what();
            return false;
        }
        catch (...) {
            outErrorMessage = "Syntax error or undefined function.";
            return false;
        }
    }

private:
    std::string formula;
    size_t pos = 0;
    double x = 0.0;
    double y = 0.0;

    char peek() const {
        if (pos < formula.length()) return formula[pos];
        return '\0';
    }

    char get() {
        if (pos < formula.length()) return formula[pos++];
        return '\0';
    }

    void consume(char expected) {
        if (get() != expected) {
            throw std::runtime_error("Unexpected character");
        }
    }

    double parseExpression() {
        double result = parseTerm();
        while (true) {
            char c = peek();
            if (c == '+') {
                consume('+');
                result += parseTerm();
            }
            else if (c == '-') {
                consume('-');
                result -= parseTerm();
            }
            else {
                break;
            }
        }
        return result;
    }

    double parseTerm() {
        double result = parseFactor();
        while (true) {
            char c = peek();
            if (c == '*') {
                consume('*');
                result *= parseFactor();
            }
            else if (c == '/') {
                consume('/');
                double denom = parseFactor();
                result = (denom == 0.0) ? 0.0 : (result / denom);
            }
            else {
                break;
            }
        }
        return result;
    }

    double parseFactor() {
        if (peek() == '+') {
            consume('+');
            return parseFactor();
        }
        if (peek() == '-') {
            consume('-');
            return -parseFactor();
        }

        double result = parsePrimary();
        if (peek() == '^') {
            consume('^');
            result = std::pow(result, parseFactor());
        }
        return result;
    }

    double parsePrimary() {
        char c = peek();

        if (c == '(') {
            consume('(');
            double result = parseExpression();
            consume(')');
            return result;
        }

        if (std::isdigit(static_cast<unsigned char>(c)) || c == '.') {
            std::string numStr = "";
            while (std::isdigit(static_cast<unsigned char>(peek())) || peek() == '.') {
                numStr += get();
            }
            try {
                return std::stod(numStr);
            }
            catch (...) {
                return 0.0;
            }
        }

        if (std::isalpha(static_cast<unsigned char>(c))) {
            std::string name = "";
            while (std::isalpha(static_cast<unsigned char>(peek()))) {
                name += get();
            }

            if (name == "x") return x;
            if (name == "y") return y;
            if (name == "pi") return 3.14159265358979323846;

            if (peek() == '(') {
                consume('(');
                std::vector<double> args;
                args.push_back(parseExpression());
                while (peek() == ',') {
                    consume(',');
                    args.push_back(parseExpression());
                }
                consume(')');

                if (name == "sin" && args.size() >= 1) return std::sin(args[0]);
                if (name == "cos" && args.size() >= 1) return std::cos(args[0]);
                if (name == "tan" && args.size() >= 1) return std::tan(args[0]);
                if (name == "sinh" && args.size() >= 1) return std::sinh(args[0]);
                if (name == "cosh" && args.size() >= 1) return std::cosh(args[0]);
                if (name == "tanh" && args.size() >= 1) return std::tanh(args[0]);
                if (name == "asin" && args.size() >= 1) return std::asin(args[0]);
                if (name == "acos" && args.size() >= 1) return std::acos(args[0]);
                if (name == "atan" && args.size() >= 1) return std::atan(args[0]);
                if (name == "abs" && args.size() >= 1) return std::abs(args[0]);
                if (name == "sqrt" && args.size() >= 1) return std::sqrt(std::max(0.0, args[0]));
                if (name == "exp" && args.size() >= 1) return std::exp(args[0]);
                if (name == "log" && args.size() >= 1) return std::log(std::max(1e-9, args[0]));
                if (name == "log10" && args.size() >= 1) return std::log10(std::max(1e-9, args[0]));
                if (name == "sgn" && args.size() >= 1) return (args[0] > 0.0) ? 1.0 : ((args[0] < 0.0) ? -1.0 : 0.0);
                if (name == "sign" && args.size() >= 1) return (args[0] > 0.0) ? 1.0 : ((args[0] < 0.0) ? -1.0 : 0.0);
                if (name == "max" && args.size() >= 2) return std::max(args[0], args[1]);
                if (name == "min" && args.size() >= 2) return std::min(args[0], args[1]);
                if (name == "pow" && args.size() >= 2) return std::pow(args[0], args[1]);
                if (name == "mod" && args.size() >= 2) return (args[1] == 0.0) ? 0.0 : std::fmod(args[0], args[1]);
                if (name == "rnd" && args.size() >= 1) {
                    double seed = args[0];
                    double noise = std::sin(seed * 12.9898) * 43758.5453;
                    return (noise - std::floor(noise)) * seed;
                }
            }
            throw std::runtime_error("Unknown function or token: " + name);
        }

        throw std::runtime_error("Unexpected token");
    }
};
