#include "Regex.h"
#include "NFA.h"

const std::unordered_set< char > Regex::regular_symbols(
    {'(', ')', '|', '*', '/'}
);

const std::unordered_set< char > Regex::symbols(
    {'(', ')', '|', '*', '/', '{', '}', '+', '?', '[', ']'}
);

//////////// CONSTRUCTORS AND DESTRUCTOR \\\\\\\\\\\\

Regex::Regex(const std::string & expression,
             const std::string & epsilon,
             const std::string & emptyset)
    : expression_(expression),
      epsilon_(epsilon),
      emptyset_(emptyset),
      N_(nullptr)
{
    format_expression();
    construct_nfa();
}

Regex::Regex(const Regex & r)
    : expression_(r.expression_),
      regular_expression_(r.regular_expression_),
      epsilon_(r.epsilon_),
      emptyset_(r.emptyset_),
      N_(nullptr)
{
    N_ = new NFA< std::string, std::string >(*r.N_);
    
    return;
}

Regex::~Regex()
{
    if (N_ != nullptr)
        delete N_;
    return;
}

//////////// PUBLIC FUNCTIONS \\\\\\\\\\\\


Regex & Regex::operator=(const Regex & r)
{
    expression_ = r.expression_;
    regular_expression_ = r.regular_expression_;
    epsilon_ = r.epsilon_;
    emptyset_ = r.emptyset_;

    if (N_ != nullptr)
        delete N_;
    N_ = new NFA< std::string, std::string>(*r.N_);
    
    return *this;
}

Regex & Regex::operator=(const std::string & s)
{ return *this = Regex(s); }

bool Regex::operator()(std::string str) const
{
    //Remove Epsilons from the string.
    if (epsilon_ != "")
    {
        int len = epsilon_.size();
        for (int i = 0; i < int(str.size()) - len; ++i)
            if (str.substr(1, len) == epsilon_)
                str.erase(i--, len);
    }

    int n = str.size();
    std::vector< std::string > str_v(n);
    for (int i = 0; i < n; ++i)
        str_v[i] = std::to_string(str[i]);

    return operator()(str_v);
}

bool Regex::operator()(const std::vector< std::string > & str) const
{
    try
    {
        return N_->operator()(str);
    }
    
    // If an "invalid" string is given, obviously it is not
    // in the language of this regex, return false.
    catch (const NFA_Invalid_Sigma_Character_Error & e)
    {
        return false;
    }
}

NFA< std::string, std::string > Regex::to_nfa() const
{ return NFA< std::string, std::string >(*N_); }

//////////// PRIVATE FUNCTIONS \\\\\\\\\\\\

char Regex::get_range_type(const char c) const
{
    if (c >= 'a' && c <= 'z')
        return 'a'; //Lowercase type
    if (c >= 'A' && c <= 'Z')
        return 'A'; //Uppercase type
    if (c >= '0' && c <= '9')
        return '0'; //Numerical type

    return 'i'; //Invalid type
}

/*
  "[1-4]" --> "(1|2|3|4)"
  "[a-d]" --> "(a|b|c|d)"
  "[1-4a-c]" --> "(1|2|a|b|c)"
  "[a-dA-E1-3]" --> "(a|b|c|d|A|B|C|D|E|1|2|3)"
*/
std::string Regex::get_range_string(const std::string & s) const
{
    std::string ret = "(";

    int i = 1, n = s.size();
    while (i < n - 1)
    {
        char val = s[i], type = get_range_type(s[i]);
        if (type == 'i')
            throw Regex_Invalid_Range_Error();

        //Dont put an "or" on the first guy in the range.
        if (i != 1)
            ret.append(1, '|');
        ret.append(1, val);
        
        if (s[i + 1] == ']')
            break;
        if (isalnum(s[i + 1]))
            ++i;
        else if (s[i + 1] == '-')
        {
            i += 2;
            
            if (s[i] == ']')
                throw Regex_Invalid_Range_Error();
            
            char upper_bound = s[i];
            if (get_range_type(upper_bound) != type ||
                val > upper_bound)
            {
                throw Regex_Invalid_Range_Error();
            }
            
            //Start at val + 1 because we already put val into the
            //return string.
            for (int j = val + 1; j <= upper_bound; ++j)
            {
                ret.append(1, '|');
                ret.append(1, (char)j);
            }
            
            i += 1;
        }
        else
            throw Regex_Invalid_Range_Error();
    }
    
    ret.append(1, ')');
    return ret;
}

/*
  a{4} = (aaaa)
  a{2-3} = (aa|aaa)
  a{2,} = aa(a)*
*/
std::string Regex::get_power_string(const std::string & s,
                                    const std::string & p) const
{
    std::string ret = "";

    int i = 1, lower_bound = 0;
    while (p[i] != ',' and p[i] != '}')
    {
        if (!isdigit(p[i]))
            throw Regex_Invalid_Power_Error();
            
        lower_bound *= 10;
        lower_bound += p[i] - '0';

        ++i;
    }

    //Only one power
    if (p[i] == '}')
    {
        for (int j = 0; j < lower_bound; ++j)
            ret += s;
    }

    //Must be at least that power.
    else if (p[i + 1] == '}')
    {
        for (int j = 0; j < lower_bound; ++j)
            ret += s;
        ret += s + "*";
    }

    //Range of powers.
    else
    {
        int upper_bound = 0;
        ++i;

        while (p[i] != '}')
        {
            if (!isdigit(p[i]))
                throw Regex_Invalid_Power_Error();

            upper_bound *= 10;
            upper_bound += p[i] - '0';

            ++i;
        }

        ret.append(1, '(');
        for (int j = lower_bound; j <= upper_bound; ++j)
        {
            if (j != lower_bound)
                ret.append(1, '|');
            
            for (int k = 0; k < j; ++k)
                ret += s;
        }
        ret.append(1, ')');
    }
    
    return ret;
}


std::string Regex::pop_prev_expression(std::string & s,
                                       int & str_begin,
                                       int & str_end )
{
    if (s == "")
        return "";
    
    int balance = 0;
    do
    {
        if (str_begin - 1 >= 0 &&
            regular_symbols.find(s[str_begin]) != regular_symbols.end() &&
            s[str_begin - 1] == '/')
        {
            --str_begin;
        }

        else
        {
            if (s[str_begin] == ')')
                balance += 1;
        
            else if (s[str_begin] == '(')
                balance -= 1;

            if (balance < 0)
                throw Regex_Unbalanced_Parenthesized_Expression_Error();
        }
        
        --str_begin;
                
    } while (balance > 0 && str_begin >= 0);
    ++str_begin;

    if (balance != 0)
        throw Regex_Unbalanced_Parenthesized_Expression_Error();
    
    std::string ret = s.substr(str_begin, str_end - str_begin);

    // pop string.
    s = s.substr(0, s.size() - (str_end - str_begin));

    return ret;
}

void Regex::format_expression()
{
    // Formatted expression
    std::string f_expression = expression_;

    //Remove all epsilon
    if (epsilon_ != "")
    {
        int len = epsilon_.size();
        for (int i = 0; i < int(f_expression.size()) - len; ++i)
            if (f_expression.substr(i, len) == epsilon_)
                f_expression.erase(i--, len);
    }
    
    regular_expression_ = "";
    int global_balance = 0;
    for (int i = 0, n = f_expression.size(); i < n; ++i)
    { 
        if (f_expression[i] == '[')
        {
            int start = i;
            while (f_expression[i] != ']')
            {    
                ++i;
                
                if (i >= n)
                    throw Regex_Invalid_Range_Error();
                if (f_expression[i] == '/')
                    throw Regex_Invalid_Escape_Character_Error();
            }

            regular_expression_.append(
                get_range_string(f_expression.substr(start, i - (start - 1)))
                );
        }

        else if (f_expression[i] == '{')
        {
            int pow_start = i;
            while (f_expression[i] != '}')
            {
                ++i;
                if (i >= n)
                    throw Regex_Invalid_Power_Error();
                if (f_expression[i] == '/')
                    throw Regex_Invalid_Escape_Character_Error();
            }

            std::string pow = f_expression.substr(pow_start, i - (pow_start - 1));

            int str_end = regular_expression_.size(),
                str_begin = regular_expression_.size() - 1;

            //Pop old expression off of the string.
            std::string target = pop_prev_expression(regular_expression_,
                                                     str_begin,
                                                     str_end);

            //Adjust regular_expression_ variable.
            regular_expression_.append(get_power_string(target, pow));
        }

        else if(f_expression[i] == '+')
        {   
            int str_end = regular_expression_.size(),
                str_begin = regular_expression_.size() - 1;
                

            std::string target = pop_prev_expression(regular_expression_,
                                                     str_begin,
                                                     str_end);
            
            regular_expression_.append(target + target + "*");
        }

        else if (f_expression[i] == '?')
        {
            int str_end = regular_expression_.size(),
                str_begin = regular_expression_.size() - 1;
                

            std::string target = pop_prev_expression(regular_expression_,
                                                     str_begin,
                                                     str_end);
            regular_expression_.append("(" + target + "|)");
        }

        else if (f_expression[i] == '/')
        {
            if (i == n - 1)
                throw Regex_Invalid_Escape_Character_Error();
            
            if (regular_symbols.find(f_expression[i + 1]) != regular_symbols.end())
                regular_expression_.append(1, '/');
            
            regular_expression_.append(1, f_expression[i + 1]);
            
            ++i;
        }

        else if (f_expression[i] == ']')
            throw Regex_Invalid_Range_Error();
        else if (f_expression[i] == '}')
            throw Regex_Invalid_Power_Error();
        
        else
        {
            if (i - 1 < 0 || f_expression[i - 1] != '/')
            {
                if (f_expression[i] == '(')
                    ++global_balance;
                else if (f_expression[i] == ')')
                    --global_balance;
            }

            //Append character
            regular_expression_.append(1, f_expression[i]);
        }
    }

    if (global_balance != 0)
        throw Regex_Unbalanced_Parenthesized_Expression_Error();

    std::cout << "regex: " << regular_expression_ << std::endl;
    
    return;
}

NFA< std::string, std::string > Regex::nfa_of_epsilon(
    const std::unordered_set< std::string > & sigma,
    const std::string & epsilon,
    int & state_num
    )
{
    std::string q0 = get_next_state_str(state_num);
    
    std::unordered_set< std::string >
        states = {q0},
        accept_states = {q0};
    
    std::unordered_map< std::pair< std::string, std::string >,
                        std::unordered_set< std::string > > delta;
    delta[{q0, epsilon}] = {q0};
    
    return NFA< std::string, std::string >(sigma,
                                           states,
                                           q0,
                                           accept_states,
                                           delta,
                                           epsilon);
}

NFA< std::string, std::string > Regex::nfa_of_symbol(
    const std::unordered_set< std::string > & sigma,
    const std::string & epsilon,
    const std::string & s,
    int & state_num
    )
{
    std::string q0 = get_next_state_str(state_num),
        q1 = get_next_state_str(state_num);
    
    std::unordered_set< std::string >
        states = {q0, q1},
        accept_states = {q1};
    
    std::unordered_map< std::pair< std::string, std::string >,
                        std::unordered_set< std::string > > delta;

    delta[{q0, s}] = {q1};

    return NFA< std::string, std::string >(sigma,
                                           states,
                                           q0,
                                           accept_states,
                                           delta,
                                           epsilon);
}


NFA< std::string, std::string > Regex::construct_nfa_recursive(
    const std::unordered_set< std::string > & sigma,
    const std::string & epsilon,
    const std::string & s,
    int & state_num
    )
{
    //Kleene start of epsilon or epsilon itself.
    if (s.size() == 0 || s.size() == 1 && s[0] == '*')
    {
        return NFA< std::string, std::string >(
            nfa_of_epsilon(sigma,
                           epsilon,
                           state_num)
            );
    }
    
    NFA< std::string, std::string > * N_ptr = nullptr;
    
    for (int i = 0, n = s.size(); i < n; ++i)
    {
        if (regular_symbols.find(s[i]) == regular_symbols.end() ||
            s[i] == '/')
        {
            if (s[i] == '/')
                ++i;
            
            NFA< std::string, std::string > new_N =
                nfa_of_symbol(sigma,
                              epsilon,
                              std::to_string(s[i]),
                              state_num);
            
            if (i + 1 < n && s[i + 1] == '*')
            {
                new_N = new_N.kleene_star(get_next_state_str(state_num));
                ++i;
            }
            
            if (N_ptr == nullptr)
            {
                N_ptr = new NFA< std::string, std::string >(
                    new_N
                    );
            }
            else
            {
                *N_ptr = nfa_concat< std::string, std::string >(
                    *N_ptr,
                    new_N
                    );
            }
        }

        else if (s[i] == '(')
        {
            int start = i + 1, len = 0, balance = 1;
            while (balance != 0)
            {
                //Skip over escape character.
                if (s[start + len] == '/')
                    ++len;
                
                else if (s[start + len] == '(')
                    ++balance;
                else if (s[start + len] == ')')
                    --balance;
                ++len;
            }
            --len;

            std::string sub_s = s.substr(start, len);

            NFA< std::string, std::string > new_N =
                construct_nfa_recursive(sigma,
                                        epsilon,
                                        sub_s,
                                        state_num);

            i += len + 1;

            if (i + 1 < n && s[i + 1] == '*')
            {
                new_N = new_N.kleene_star(get_next_state_str(state_num));
                ++i;
            }
            
            if (N_ptr == nullptr)
            {
                N_ptr = new NFA< std::string, std::string >(
                    new_N
                    );
            }
            else
            {
                *N_ptr = nfa_concat< std::string, std::string >(
                    *N_ptr,
                    new_N
                    );
            }
        }
        
        else if (s[i] == '|')
        {
            if (N_ptr == nullptr)
            {
                N_ptr = new NFA< std::string, std::string >(
                    nfa_of_epsilon(sigma,
                                   epsilon,
                                   state_num)
                    );
            }
            
            int start = i + 1, len = 0, balance = 0;
            while (start + len < n && !(s[start + len] == '|' && balance == 0))
            {
                if (s[start + len] == '(')
                    ++balance;
                if (s[start + len] == ')')
                    --balance;
                ++len;
            }
            if (balance == -1) 
                --len;

            i += len;

            if (len == 0)
            {
                NFA< std::string, std::string > new_N =
                    nfa_of_epsilon(sigma,
                                   epsilon,
                                   state_num);
                // An NFA must already exist prior, so dont check
                // for nullptr.
                *N_ptr = nfa_union< std::string, std::string >(
                    *N_ptr,
                    new_N,
                    get_next_state_str(state_num)
                    );
            }

            else
            {
                
                std::string sub_s = s.substr(start, len);
                NFA< std::string, std::string > new_N =
                    construct_nfa_recursive(sigma,
                                            epsilon,
                                            sub_s,
                                            state_num);
                // An NFA must already exist prior, so dont check
                // for nullptr.
                *N_ptr = nfa_union< std::string, std::string >(
                    *N_ptr,
                    new_N,
                    get_next_state_str(state_num)
                    );
            }
        }

        else
            throw Regex_NFA_Construction_Error();
  
    }

    NFA< std::string, std::string > ret(*N_ptr);
    delete N_ptr;
    return ret;
}


void Regex::construct_nfa()
{
    std::unordered_set< std::string > sigma;

    //Construct sigma.
    bool delimiter = false;
    for (const char & c : regular_expression_)
    {
        
        if (!delimiter && c == '/')
            delimiter = true;
        else if (delimiter ||
                 regular_symbols.find(c) == regular_symbols.end() &&
                 sigma.find(std::to_string(c)) == sigma.end())
        {
            sigma.insert(std::to_string(c));
            
            if (delimiter)
                delimiter = false;
        }
    }
    
    std::string epsilon = "";
    sigma.insert(epsilon);
    int state_num = 0;

    N_ = new NFA< std::string, std::string >(construct_nfa_recursive(
                                                 sigma,
                                                 epsilon,
                                                 regular_expression_,
                                                 state_num));
    
    return;
}

/////////// NON-MEMBER FUNCTIONS \\\\\\\\\\\\


std::ostream & operator<<(std::ostream & cout, const Regex & r)
{
    cout << r.expression();
    
    return cout;
}
