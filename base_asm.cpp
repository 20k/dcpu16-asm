#include "base_asm.hpp"

std::pair<std::optional<return_info>, std::string_view> assemble_fwd(std::string_view text)
{
    return assemble(text);
}
