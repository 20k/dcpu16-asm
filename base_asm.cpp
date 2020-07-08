#include "base_asm.hpp"

std::pair<std::optional<return_info>, error_info> assemble_fwd(std::string_view text)
{
    return assemble(text);
}
