#include "optiq/cpp/lexer.hpp"

#include <limits>
#include <stdexcept>

#include "scanner_state.hpp"

struct yy_buffer_state;
using YY_BUFFER_STATE = yy_buffer_state*;
using yyscan_t = void*;

int yylex_init_extra(optiq::cpp::ScannerState* state, yyscan_t* scanner);
YY_BUFFER_STATE yy_scan_bytes(const char* bytes, int length, yyscan_t scanner);
void yy_delete_buffer(YY_BUFFER_STATE buffer, yyscan_t scanner);
int yylex(yyscan_t scanner);
int yylex_destroy(yyscan_t scanner);

namespace optiq::cpp {

LexResult tokenize(const std::string& source) {
    if (source.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw std::length_error("Source file is too large for the Flex scanner");
    }

    ScannerState state;
    yyscan_t scanner = nullptr;
    if (yylex_init_extra(&state, &scanner) != 0) {
        throw std::runtime_error("Unable to initialize the C/C++ lexer");
    }

    YY_BUFFER_STATE buffer = nullptr;
    try {
        buffer = yy_scan_bytes(source.data(), static_cast<int>(source.size()), scanner);
        if (!buffer) throw std::runtime_error("Unable to create the lexer input buffer");
        yylex(scanner);
        yy_delete_buffer(buffer, scanner);
        buffer = nullptr;
        yylex_destroy(scanner);
        scanner = nullptr;
    } catch (...) {
        if (buffer) yy_delete_buffer(buffer, scanner);
        if (scanner) yylex_destroy(scanner);
        throw;
    }

    return state.result;
}

} // namespace optiq::cpp
