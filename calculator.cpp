// Build: g++ -std=c++17 -Wall -Wextra -Wpedantic -O2 calculator.cpp -o calculator

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace {

constexpr std::size_t kPanelWidth = 72;
constexpr std::size_t kMaximumExpressionLength = 4096;
constexpr unsigned kMaximumNesting = 256;

struct HistoryEntry {
    std::size_t index;
    std::string timestamp;
    std::string expression;
    std::string result;
};

class ParseError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class DepthGuard {
public:
    explicit DepthGuard(unsigned& depth) : depth_(depth) {
        if (depth_ >= kMaximumNesting) {
            throw ParseError("Expression nesting is too deep.");
        }
        ++depth_;
    }

    ~DepthGuard() { --depth_; }

    DepthGuard(const DepthGuard&) = delete;
    DepthGuard& operator=(const DepthGuard&) = delete;

private:
    unsigned& depth_;
};

std::string trim(const std::string& text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

std::string formatNumber(double value) {
    std::ostringstream output;
    output << std::setprecision(12) << value;
    return output.str();
}

std::string timestampNow() {
    const std::time_t now = std::time(nullptr);
    std::tm localTime{};
    const std::tm* local = std::localtime(&now);
    if (local != nullptr) {
        localTime = *local;
    }
    std::ostringstream output;
    output << std::put_time(&localTime, "%H:%M:%S");
    return output.str();
}

class ExpressionParser {
public:
    explicit ExpressionParser(const std::string& expression) : expression_(expression) {
        if (expression_.size() > kMaximumExpressionLength) {
            throw ParseError("Expression is too long (maximum 4096 characters).");
        }
    }

    double parse() {
        const double value = parseExpression();
        skipWhitespace();
        if (position_ != expression_.size()) {
            throw error("Unexpected character");
        }
        return value;
    }

private:
    const std::string& expression_;
    std::size_t position_ = 0;
    unsigned nestingDepth_ = 0;
    unsigned powerDepth_ = 0;

    ParseError error(const std::string& message) const {
        return ParseError(message + " at position " + std::to_string(position_ + 1) + ".");
    }

    void skipWhitespace() {
        while (position_ < expression_.size() &&
               (expression_[position_] == ' ' || expression_[position_] == '\t' ||
                expression_[position_] == '\r' || expression_[position_] == '\n')) {
            ++position_;
        }
    }

    bool consume(char character) {
        skipWhitespace();
        if (position_ < expression_.size() && expression_[position_] == character) {
            ++position_;
            return true;
        }
        return false;
    }

    static double requireFinite(double value) {
        if (!std::isfinite(value)) {
            throw ParseError("The operation produced a non-finite result.");
        }
        return value;
    }

    double parseExpression() {
        double value = parseTerm();
        for (;;) {
            if (consume('+')) {
                value = requireFinite(value + parseTerm());
            } else if (consume('-')) {
                value = requireFinite(value - parseTerm());
            } else {
                return value;
            }
        }
    }

    double parseTerm() {
        double value = parseUnary();
        for (;;) {
            if (consume('*')) {
                value = requireFinite(value * parseUnary());
            } else if (consume('/')) {
                const double divisor = parseUnary();
                if (divisor == 0.0) {
                    throw ParseError("Division by zero is not allowed.");
                }
                value = requireFinite(value / divisor);
            } else if (consume('%')) {
                const double divisor = parseUnary();
                if (divisor == 0.0) {
                    throw ParseError("Remainder by zero is not allowed.");
                }
                value = requireFinite(std::fmod(value, divisor));
            } else {
                return value;
            }
        }
    }

    double parseUnary() {
        bool negate = false;
        for (;;) {
            if (consume('+')) {
                continue;
            }
            if (consume('-')) {
                negate = !negate;
                continue;
            }
            break;
        }
        const double value = parsePower();
        return negate ? -value : value;
    }

    double parsePower() {
        const double base = parsePostfix();
        if (!consume('^')) {
            return base;
        }
        DepthGuard guard(powerDepth_);
        const double exponent = parseUnary();
        return requireFinite(std::pow(base, exponent));
    }

    double parsePostfix() {
        double value = parsePrimary();
        while (consume('!')) {
            if (value < 0.0 || std::floor(value) != value) {
                throw ParseError("Factorial requires a nonnegative integer.");
            }
            if (value > 170.0) {
                throw ParseError("Factorial is limited to 170 (double-precision range).");
            }
            double result = 1.0;
            for (unsigned factor = 2; factor <= static_cast<unsigned>(value); ++factor) {
                result *= factor;
            }
            value = result;
        }
        return value;
    }

    std::string parseIdentifier() {
        const std::size_t start = position_;
        while (position_ < expression_.size()) {
            const char character = expression_[position_];
            if (!((character >= 'a' && character <= 'z') ||
                  (character >= 'A' && character <= 'Z'))) {
                break;
            }
            ++position_;
        }
        return expression_.substr(start, position_ - start);
    }

    double parseNumber() {
        const char* start = expression_.c_str() + position_;
        char* end = nullptr;
        const double value = std::strtod(start, &end);
        if (end == start) {
            throw error("Expected a number, constant, function, or '('");
        }
        position_ += static_cast<std::size_t>(end - start);
        if (!std::isfinite(value)) {
            throw ParseError("Numeric literals must be finite.");
        }
        return value;
    }

    double parseFunction(const std::string& name) {
        if (!consume('(')) {
            throw error("Expected '(' after function name");
        }
        DepthGuard guard(nestingDepth_);
        const double argument = parseExpression();
        if (!consume(')')) {
            throw error("Expected ')' after function argument");
        }

        if (name == "sin") {
            return requireFinite(std::sin(argument));
        }
        if (name == "cos") {
            return requireFinite(std::cos(argument));
        }
        if (name == "tan") {
            return requireFinite(std::tan(argument));
        }
        if (name == "log") {
            if (argument <= 0.0) {
                throw ParseError("log() requires a positive argument.");
            }
            return requireFinite(std::log10(argument));
        }
        if (name == "ln") {
            if (argument <= 0.0) {
                throw ParseError("ln() requires a positive argument.");
            }
            return requireFinite(std::log(argument));
        }
        if (name == "sqrt") {
            if (argument < 0.0) {
                throw ParseError("sqrt() requires a nonnegative argument.");
            }
            return requireFinite(std::sqrt(argument));
        }
        if (name == "abs") {
            return std::abs(argument);
        }
        throw ParseError("Unknown function '" + name + "'.");
    }

    double parsePrimary() {
        skipWhitespace();
        if (position_ >= expression_.size()) {
            throw error("Expected an expression");
        }
        if (consume('(')) {
            DepthGuard guard(nestingDepth_);
            const double value = parseExpression();
            if (!consume(')')) {
                throw error("Expected ')'");
            }
            return value;
        }

        const char character = expression_[position_];
        if ((character >= '0' && character <= '9') || character == '.') {
            return parseNumber();
        }
        if ((character >= 'a' && character <= 'z') ||
            (character >= 'A' && character <= 'Z')) {
            const std::string name = parseIdentifier();
            if (name == "pi") {
                return std::acos(-1.0);
            }
            if (name == "e") {
                return std::exp(1.0);
            }
            return parseFunction(name);
        }
        throw error("Expected a number, constant, function, or '('");
    }
};

bool outputIsTerminal() {
#ifdef _WIN32
    return _isatty(_fileno(stdout)) != 0;
#else
    return isatty(STDOUT_FILENO) != 0;
#endif
}

bool enableAnsi() {
    if (!outputIsTerminal()) {
        return false;
    }
#ifdef _WIN32
    const HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    if (output == INVALID_HANDLE_VALUE || output == nullptr) {
        return false;
    }
    DWORD mode = 0;
    if (!GetConsoleMode(output, &mode)) {
        return false;
    }
    if (!SetConsoleMode(output, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
        return false;
    }
    SetConsoleOutputCP(CP_UTF8);
    return true;
#else
    return true;
#endif
}

class TerminalUi {
public:
    explicit TerminalUi(bool ansi) : ansi_(ansi) {}

    void clearScreen() const {
        if (ansi_) {
            std::cout << "\033[2J\033[H";
        } else {
            std::cout << "\n\n";
        }
    }

    void printDashboard(double memory, const std::vector<HistoryEntry>& history,
                        const std::string& latestResult) const {
        clearScreen();
        printBorder("┌", "┬", "┐", cyan());
        printRow("  ORBIT  /  SCIENTIFIC CALCULATOR", boldCyan());
        printRow("  Expression engine  |  radians  |  session ready", dim());
        printBorder("├", "┼", "┤", cyan());
        printRow("  MEMORY   M = " + formatNumber(memory), boldYellow());
        printRow("  RESULT   " + (latestResult.empty() ? std::string("No calculation yet") : latestResult),
                 boldGreen());
        printBorder("├", "┼", "┤", cyan());
        printRow("  RECENT CALCULATIONS  (latest 5)", bold());

        const std::size_t first = history.size() > 5 ? history.size() - 5 : 0;
        if (first == history.size()) {
            printRow("  No calculations in this session yet.", dim());
        } else {
            for (std::size_t index = first; index < history.size(); ++index) {
                const auto& entry = history[index];
                std::ostringstream line;
                line << "  #" << entry.index << "  " << entry.timestamp << "  "
                     << shorten(entry.expression, 38) << " = " << shorten(entry.result, 12);
                printRow(line.str(), green());
            }
        }
        printBorder("└", "┴", "┘", cyan());

        if (!latestResult.empty()) {
            printResultCard(latestResult);
        }
    }

    void printFullHistory(const std::vector<HistoryEntry>& history) const {
        std::cout << boldCyan() << "FULL SESSION HISTORY" << reset() << '\n';
        if (history.empty()) {
            std::cout << dim() << "  No calculations recorded yet." << reset() << '\n';
            return;
        }
        for (const auto& entry : history) {
            std::cout << "  #" << entry.index << "  " << entry.timestamp << "  "
                      << entry.expression << " = " << entry.result << '\n';
        }
    }

    void printHelp() const {
        printBorder("┌", "┬", "┐", cyan());
        printRow("  SYNTAX GUIDE", boldCyan());
        printRow("  Operators: +  -  *  /  %  ^  and parentheses", plain());
        printRow("  Functions: sin cos tan log ln sqrt abs; factorial: 5!", plain());
        printRow("  Constants: pi, e. Trigonometric functions use radians.", plain());
        printRow("  Memory: M+ / M- add or subtract the last result; MR; MC", plain());
        printRow("  Commands: history, help, clear, exit", plain());
        printBorder("└", "┴", "┘", cyan());
    }

    void printMessage(const std::string& message, bool isError = false) const {
        const char* messageStyle = isError ? boldRed() : boldGreen();
        const char* borderStyle = isError ? red() : green();
        printBorder("┌", "┬", "┐", borderStyle);
        printRow("  " + message, messageStyle);
        printBorder("└", "┴", "┘", borderStyle);
    }

    void printPrompt() const {
        std::cout << boldCyan() << "calc" << reset() << " > " << std::flush;
    }

private:
    bool ansi_;

    const char* style(const char* code) const { return ansi_ ? code : ""; }
    const char* reset() const { return style("\033[0m"); }
    const char* bold() const { return style("\033[1m"); }
    const char* dim() const { return style("\033[2m"); }
    const char* cyan() const { return style("\033[36m"); }
    const char* green() const { return style("\033[32m"); }
    const char* red() const { return style("\033[31m"); }
    const char* boldCyan() const { return style("\033[1;36m"); }
    const char* boldGreen() const { return style("\033[1;32m"); }
    const char* boldYellow() const { return style("\033[1;33m"); }
    const char* boldRed() const { return style("\033[1;31m"); }
    const char* plain() const { return ""; }

    static std::string shorten(const std::string& text, std::size_t maximum) {
        if (text.size() <= maximum) {
            return text;
        }
        if (maximum <= 3) {
            return text.substr(0, maximum);
        }
        return text.substr(0, maximum - 3) + "...";
    }

    void printBorder(const char* left, const char* middle, const char* right,
                     const char* color) const {
        const std::size_t segment = kPanelWidth / 2;
        const std::size_t remainder = kPanelWidth - segment;
        std::cout << color << left << reset();
        for (std::size_t index = 0; index < segment; ++index) {
            std::cout << "─";
        }
        std::cout << color << middle << reset();
        for (std::size_t index = 0; index < remainder; ++index) {
            std::cout << "─";
        }
        std::cout << color << right << reset() << '\n';
    }

    void printRow(const std::string& text, const char* color) const {
        std::string visible = text;
        if (visible.size() > kPanelWidth) {
            visible = shorten(visible, kPanelWidth);
        }
        std::cout << cyan() << "│" << reset() << color << visible
                  << std::string(kPanelWidth - visible.size(), ' ') << reset()
                  << cyan() << "│" << reset() << '\n';
    }

    void printResultCard(const std::string& result) const {
        std::cout << '\n';
        printBorder("┌", "┬", "┐", green());
        printRow("  LAST RESULT", boldGreen());
        printRow("  " + result, boldGreen());
        printBorder("└", "┴", "┘", green());
        std::cout << '\n';
    }
};

void addHistory(std::vector<HistoryEntry>& history, const std::string& expression,
                const std::string& result) {
    history.push_back({history.size() + 1, timestampNow(), expression, result});
}

}  // namespace

int main() {
    const TerminalUi ui(enableAnsi());
    std::vector<HistoryEntry> history;
    double memory = 0.0;
    double lastResult = 0.0;
    bool hasLastResult = false;
    bool showFullHistory = false;
    bool showHelp = false;
    std::string latestResult;
    std::string pendingMessage;
    bool pendingError = false;

    for (;;) {
        ui.printDashboard(memory, history, latestResult);
        if (showFullHistory) {
            ui.printFullHistory(history);
            showFullHistory = false;
        }
        if (showHelp) {
            ui.printHelp();
            showHelp = false;
        }
        if (!pendingMessage.empty()) {
            ui.printMessage(pendingMessage, pendingError);
            pendingMessage.clear();
            pendingError = false;
        }

        ui.printPrompt();
        std::string input;
        if (!std::getline(std::cin, input)) {
            std::cout << "\nSession closed.\n";
            break;
        }
        input = trim(input);
        if (input.empty()) {
            continue;
        }

        if (input == "exit") {
            std::cout << "Session closed.\n";
            break;
        }
        if (input == "clear") {
            latestResult.clear();
            continue;
        }
        if (input == "help") {
            showHelp = true;
            continue;
        }
        if (input == "history") {
            showFullHistory = true;
            continue;
        }

        if (input == "MR") {
            lastResult = memory;
            hasLastResult = true;
            latestResult = formatNumber(memory);
            addHistory(history, "MR", latestResult);
            pendingMessage = "Memory recalled: " + latestResult;
            continue;
        }
        if (input == "MC") {
            memory = 0.0;
            addHistory(history, "MC", "M = 0");
            pendingMessage = "Memory cleared.";
            continue;
        }
        if (input == "M+" || input == "M-") {
            if (!hasLastResult) {
                pendingMessage = input + " requires a previous result.";
                pendingError = true;
                continue;
            }
            memory += input == "M+" ? lastResult : -lastResult;
            if (!std::isfinite(memory)) {
                memory = 0.0;
                pendingMessage = "Memory update overflowed; M has been reset to 0.";
                pendingError = true;
                addHistory(history, input, "error");
                continue;
            }
            const std::string memoryValue = formatNumber(memory);
            addHistory(history, input, "M = " + memoryValue);
            pendingMessage = "Memory updated: M = " + memoryValue;
            continue;
        }

        try {
            ExpressionParser parser(input);
            const double value = parser.parse();
            lastResult = value;
            hasLastResult = true;
            latestResult = formatNumber(value);
            addHistory(history, input, latestResult);
        } catch (const std::exception& exception) {
            pendingMessage = exception.what();
            pendingError = true;
            addHistory(history, input, "error: " + pendingMessage);
        }
    }

    return 0;
}