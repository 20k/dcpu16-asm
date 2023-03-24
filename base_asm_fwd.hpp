#ifndef BASE_ASM_FWD_HPP_INCLUDED
#define BASE_ASM_FWD_HPP_INCLUDED

#include <utility>
#include <optional>
#include <string_view>
#include "stack_vector.hpp"
#include <stdint.h>
#include <vector>

#define MEM_SIZE 0x10000

struct error_info
{
    std::string_view name_in_source;
    std::string_view msg;
    int character = 0;
    int line = 0;
};

struct return_info
{
    stack_vector<uint16_t, MEM_SIZE> mem;
    ///memory cell -> source character index
    stack_vector<uint16_t, MEM_SIZE> translation_map;
    ///memory cell -> source line
    stack_vector<uint16_t, MEM_SIZE> pc_to_source_line;
    ///input line to memory cell
    stack_vector<uint16_t, MEM_SIZE> source_line_to_pc;

    std::vector<std::pair<uint16_t, std::string>> exported_label_names;

    constexpr return_info(){}
};

std::pair<std::optional<return_info>, error_info> assemble_fwd(std::string_view text);

#endif // BASE_ASM_FWD_HPP_INCLUDED
