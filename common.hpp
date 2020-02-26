#ifndef COMMON_HPP_INCLUDED
#define COMMON_HPP_INCLUDED

#include <string_view>
#include <cctype>

template<typename T>
inline
constexpr std::string_view trim_start(std::string_view view, const T& matches)
{
    if(view.size() == 0)
        return view;

    int i=0;

    for(; i < (int)view.size() && matches(view[i]); i++)
    {

    }

    view.remove_prefix(i);

    return view;
}

template<typename T>
inline
constexpr std::string_view trim_end(std::string_view view, const T& matches)
{
    if(view.size() == 0)
        return view;

    int fin = (int)view.size();

    for(; (fin - 1) >= 0 && matches(view[fin - 1]); fin--)
    {

    }

    view.remove_suffix((int)view.size() - fin);

    return view;
}

inline
constexpr std::string_view consume_next(std::string_view& in)
{
    if(in.size() == 0)
        return in;

    auto trimmed = trim_start(in, [](char c) {return c == ' ' || c == '\n' || c == ','; });

    if(trimmed.size() == 0)
    {
        in = trimmed;
        return trimmed;
    }

    size_t next = std::min(trimmed.find_first_of(",\n "), trimmed.size());

    auto first = trimmed;
    auto second = trimmed;

    first.remove_suffix(trimmed.size() - next);
    second.remove_prefix(next);

    in = second;

    return trim_end(first, [](char c) {return c == ' ' || c == '\n' || c == ','; });
}

inline
constexpr bool is_label(std::string_view in)
{
    return in.starts_with(':');
}

inline
constexpr bool is_hex_digit(char in)
{
    return
    (in >= '0' && in <= '9') ||
    in == 'a' || in == 'A' ||
    in == 'b' || in == 'B' ||
    in == 'c' || in == 'C' ||
    in == 'd' || in == 'D' ||
    in == 'e' || in == 'E' ||
    in == 'f' || in == 'F';
}

inline
constexpr bool is_digit(char in)
{
    return in >= '0' && in <= '9';
}

inline
constexpr bool is_constant(std::string_view in)
{
    if(in.size() == 0)
        return false;

    if(in.starts_with('-'))
    {
        in.remove_prefix(1);
    }

    if(in.starts_with("0x"))
    {
        for(int i=2; i < (int)in.size(); i++)
        {
            if(!is_hex_digit(in[i]))
                return false;
        }

        return true;
    }

    for(int i=0; i < (int)in.size(); i++)
    {
        if(!is_digit(in[i]))
            return false;
    }

    return true;
}

inline
constexpr int get_digit(char in)
{
    if(!is_digit(in))
        return 0;

    return in - '0';
}

inline
constexpr int get_hex_digit(char in)
{
    if(!is_hex_digit(in))
        return 0;

    if(in >= '0' && in <= '9')
        return in - '0';

    if(in >= 'a' && in <= 'f')
        return 10 + (in - 'a');

    if(in >= 'A' && in <= 'F')
        return 10 + (in - 'A');

    return 0;
}

inline
constexpr int positive_stoi_cxper(std::string_view in, int radix)
{
    if(in.size() == 0)
        return 0;

    int value = 0;
    int base = 1;

    for(int i=(int)in.size() - 1; i >= 0; i--)
    {
        if(radix == 10)
        {
            if(!is_digit(in[i]))
            {
                return value;
            }

            value += base * get_digit(in[i]);
        }

        if(radix == 16)
        {
            if(!is_hex_digit(in[i]))
            {
                return value;
            }

            value += base * get_hex_digit(in[i]);
        }

        base *= radix;
    }

    return value;
}

inline
constexpr int get_constant(std::string_view in)
{
    if(in.size() == 0)
        return 0;

    bool is_neg = false;

    if(in.starts_with('-'))
    {
        is_neg = true;
        in.remove_prefix(1);
    }

    int n = 0;

    if(in.starts_with("0x"))
    {
        n = positive_stoi_cxper(in, 16);
    }
    else
    {
        n = positive_stoi_cxper(in, 10);
    }

    if(is_neg)
        return -n;
    else
        return n;
}

#endif // COMMON_HPP_INCLUDED
