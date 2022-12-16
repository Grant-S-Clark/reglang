#ifndef NFA_H
#define NFA_H

#include "Common.h"

#include "DFA.h"
#include "Regex.h"

template < typename S_t, typename Q_t >
class NFA
{
public:
    typedef std::pair< Q_t, S_t > Q_t_S_t;
    typedef std::unordered_map< Q_t_S_t, std::unordered_set< Q_t > > D_t;
    
    NFA(const std::unordered_set< S_t > & sigma,
        const std::unordered_set< Q_t > & states,
        const Q_t & initial_state,
        const std::unordered_set< Q_t > & accept_states,
        const D_t & delta,
        const S_t & epsilon)
        : sigma_(sigma),
          states_(states),
          initial_state_(initial_state),
          accept_states_(accept_states),
          delta_(delta),
          epsilon_(epsilon),
          M_(nullptr)
    {
        if (sigma_.find(epsilon_) == sigma_.end())
            throw NFA_Epsilon_Not_In_Sigma_Error();

        //M_ is now no longer nullptr after calling this function.
        construct_dfa();

        return;
    }

    NFA(const NFA< S_t, Q_t > & N) : epsilon_(N.epsilon_), M_(nullptr)
    { *this = N; }

    NFA< S_t, Q_t > & operator=(const NFA< S_t, Q_t > & N)
    {
        //Assign values.
        sigma_ = N.sigma_;
        states_ = N.states_;
        initial_state_ = N.initial_state_;
        accept_states_ = N.accept_states_;
        delta_ = N.delta_;

        //Assign DFA.
        if (M_ == nullptr)
            M_ = new DFA< S_t, std::unordered_set< Q_t > >(*N.M_);
        else
            *M_ = *N.M_;

        return *this;
    }

    ~NFA()
    {
        if (M_ != nullptr)
            delete M_;
    }

    // Returns the DFA of this NFA.
    DFA< S_t, std::unordered_set< Q_t > > to_dfa() const
    { return *M_; }


    class NFA_To_Regex_Invalid_qi_Error{};
    class NFA_To_Regex_Invalid_qa_Error{};
    /*
     Returns a Regex of this NFA.
     Must provide temporary states for the initial and accepting
     states used in regex creation as state types cant be predicted.
     Must also provide a S_t object to represent the emptyset for
     the regex.
    */
    Regex to_regex(const Q_t & qi, const Q_t & qa,
                   const S_t & emptyset) const
    {
        //Validate initial and accepting states.
        if (states_.find(qi) != states_.end())
            throw NFA_To_Regex_Invalid_qi_Error();
        if (states_.find(qa) != states_.end())
            throw NFA_To_Regex_Invalid_qa_Error();

        // Temporary states
        std::unordered_set< Q_t > t_states = states_;
        t_states.insert(qi);
        t_states.insert(qa);

        // Temporary delta
        D_t new_delta = delta_;
        new_delta[{qi, epsilon_}] = {initial_state_};
        for (const Q_t & q : accept_states_)
            new_delta[{q, epsilon_}] = {qa};

        //Map of a set of all transitions between two states.
        std::unordered_map< std::pair< Q_t, Q_t >,
                            std::unordered_set< std::string > > transitions;
        
        for (const std::pair< Q_t_S_t, std::unordered_set< Q_t > > & pair : new_delta)
        {
            for (const Q_t & q : pair.second)
            {
                std::pair< Q_t, Q_t > from_to = { pair.first.first, q };

                // If a set for this transitioning state pair does not already
                // exist, create one.
                if (transitions.find(from_to) == transitions.end())
                    transitions[from_to] = {};
                
                std::string insertion = std::to_string(pair.first.second);
                // If this transition string is not already in the set for the
                // transitioning state pair, insert it.
                if (transitions[from_to].find(insertion) == transitions[from_to].end())
                {
                    std::string insert_str = std::to_string(pair.first.second);

                    // Add the '/' delimiter before symbols used by regex operations.
                    for (int i = 0; i < insert_str.size(); ++i)
                    {
                        if (Regex::symbols.find(insert_str[i]) != Regex::symbols.end())
                        {
                            insert_str.insert(i, 1, '/');
                            ++i;
                        }
                    }
                    
                    transitions[from_to].insert(insert_str);
                }
            }
        }


        //Map of a transition between two states as a string.
        std::unordered_map< std::pair< Q_t, Q_t >, std::string > old_trans,
                                                                 new_trans;
        
        // adjust old transitions to combine transitions that
        // lead to the same state into a single transition using
        // the '|' OR operator for regex.
        for (const std::pair< std::pair< Q_t, Q_t >,
                 std::unordered_set< std::string > > pair :
                 transitions)
        {
            std::string transition = "", delim = "";
            int num_transitions = 0;
            for (const std::string & s : pair.second)
            {
                transition += delim + s;
                delim = "|";
                ++num_transitions;
            }

            if (num_transitions > 1)
                transition = "(" + transition + ")";

            old_trans[pair.first] = transition;
        }

        // Keep adjusting the transition maps by removing states from t_states
        // until only qi and qa remain.
        while (t_states.size() > 2)
        {
            // Find state to remove from states that is not qi or qa.
            typename std::unordered_set< Q_t >::const_iterator it  = t_states.begin();
            while (*it == qi || *it == qa)
                ++it;

            //Grab state and remove it from t_states.
            Q_t q = *it;
            t_states.erase(it);

            //Create cross product of the remaining t_states.
            std::vector< std::pair< Q_t, Q_t > >
                cross_prod(t_states.size() * t_states.size());
            int n = 0;
            for (const Q_t & q0 : t_states)
                for (const Q_t & q1 : t_states)
                    cross_prod[n++] = {q0, q1};

            //Go through cross product and adjust the new transitions.
            for (const std::pair< Q_t, Q_t > & pair : cross_prod)
            {
                std::string trans1, trans2, transition;
                bool q_path_exists = false,
                     straight_path_exists = false;

                std::pair< Q_t, Q_t >
                    first_to_q = {pair.first, q},
                    q_to_q = {q, q},
                    q_to_second = {q, pair.second},
                    first_to_second = {pair.first, pair.second};
                
                if (old_trans.find(first_to_q) != old_trans.end() &&
                    old_trans[first_to_q] != emptyset &&
                    old_trans.find(q_to_second) != old_trans.end() &&
                    old_trans[q_to_second] != emptyset)
                {
                    q_path_exists = true;

                    trans1 = old_trans[first_to_q];

                    if (old_trans.find(q_to_q) != old_trans.end() &&
                        old_trans[q_to_q] != emptyset &&
                        old_trans[q_to_q] != epsilon_)
                    {
                        trans1 += "(" + old_trans[q_to_q] + ")*";
                    }

                    
                    trans1 += old_trans[q_to_second];
                }

                if (old_trans.find(first_to_second) != old_trans.end() &&
                    old_trans[first_to_second] != emptyset)
                {
                    straight_path_exists = true;

                    trans2 = old_trans[first_to_second];
                }

                if (q_path_exists && straight_path_exists)
                    transition = "(" + trans1 + "|" + trans2 + ")";
                else if (q_path_exists)
                    transition = trans1;
                else if (straight_path_exists)
                    transition = trans2;

                if (q_path_exists || straight_path_exists)
                    new_trans[first_to_second] = transition;
                else
                    new_trans[first_to_second] = emptyset;
            }

            old_trans = new_trans;
            new_trans.clear();
        }
        
        std::string regex_str = old_trans[{qi, qa}];

        return Regex(regex_str, epsilon_, emptyset);
    }

    // Returns the epsilon closure of a given state.
    std::unordered_set< Q_t > epsilon_closure(const Q_t & q) const
    {
        if (states_.find(q) == states_.end())
            throw NFA_Invalid_State_Error();
        
        std::unordered_set< Q_t > ret = { q };
        std::stack< Q_t > check_stack; check_stack.push(q);

        while (!check_stack.empty())
        {
            Q_t check = check_stack.top();
            check_stack.pop();

            //Make sure there is an epsilon edges for the current state.
            if (delta_.find({check, epsilon_}) != delta_.end())
            {
                //Loop through all epsilon edges.
                for (const Q_t & q : delta_.find({check, epsilon_})->second)
                {
                    // If that state is not in the epsilon closure, add
                    // it, and then add it to the stack of states to check.
                    if (ret.find(q) == ret.end())
                    {
                        ret.insert(q);
                        check_stack.push(q);
                    }
                }
            }
        }

        return ret;
    }

    // Returns true if this NFA accepts a given string of characters
    // in sigma, false otherwise.
    bool operator()(const std::vector< S_t > & str) const
    {
        //DFA cannot work with epsilon characters.
        std::vector< S_t > new_str;
        for (const S_t & c : str)
            if (c != epsilon_)
                new_str.push_back(c);
        
        try
        {
            return M_->operator()(new_str);
        }

        // This will ensure the error will throw from inside of the NFA
        // class and not the DFA class to avoid error origin confusion.
        catch (const DFA_Invalid_Sigma_Character_Error & e)
        {
            throw NFA_Invalid_Sigma_Character_Error();
        }
    }


    // Return the Kleene Star of the current NFA with a new
    // given initial accepting state.
    NFA< S_t, Q_t > kleene_star(const Q_t & new_initial_state) const
    {
        if (states_.find(new_initial_state) != states_.end())
            throw NFA_Invalid_Kleene_Star_Initial_State_Error();

        std::unordered_set< Q_t > new_states = states_;
        new_states.insert(new_initial_state);
        std::unordered_set< Q_t > new_accept_states = accept_states_;
        new_accept_states.insert(new_initial_state);
        
        D_t new_delta = delta_;
        for (const Q_t & q : accept_states_)
            new_delta[{q, epsilon_}].insert(initial_state_);
        new_delta[{new_initial_state, epsilon_}] = {initial_state_};

        return NFA< S_t, Q_t >(sigma_,
                               new_states,
                               new_initial_state,
                               new_accept_states,
                               new_delta,
                               epsilon_);
    }

    // Return true if this is a valid NFA.
    bool valid() const
    {
        // Validate size of states.
        if (states_.empty())
            return false;

        // Validate accepting states.
        for (const Q_t & q : accept_states_)
            if (states_.find(q) == states_.end())
                return false;

        // Validate initial state.
        if (states_.find(initial_state_) == states_.end())
            return false;

        for (const std::pair< Q_t_S_t, std::unordered_set< Q_t > > & p : delta_)
        {
            //Check if the state is valid.
            if (states_.find(p.first.first) == states_.end())
                return false;
            
            //Check if the character is within the alphabet.
            if (sigma_.find(p.first.second) == sigma_.end())
                return false;

            //Make sure the states that it travels to exist.
            for (const Q_t & q : p.second)
                if (states_.find(q) == states_.end())
                    return false;
        }

        return true;
    }

    const D_t & delta() const
    { return delta_; }

    const std::unordered_set< S_t > & sigma() const
    { return sigma_; }

    const std::unordered_set< Q_t > & states() const
    { return states_; }

    const std::unordered_set< Q_t > & accept_states() const
    { return accept_states_; }

    const Q_t & initial_state() const
    { return initial_state_; }

    const S_t & epsilon() const
    { return epsilon_; }

    bool is_accepting(const Q_t & q) const
    {
        return accept_states_.find(q) != accept_states_.end();
    }

private:

    // Helper function for construct_dfa().
    const std::unordered_set< Q_t > * get_ptr_to_member(
        const std::unordered_set< std::unordered_set< Q_t > > & set,
        const std::unordered_set< Q_t > & state
        )
    {
        for (const std::unordered_set< Q_t > & set_state : set)
        {
            if (set_state.size() == state.size())
            {
                bool equal = true;
                for (const Q_t & q : state)
                {
                    if (set_state.find(q) == set_state.end())
                    {
                        equal = false;
                        break;
                    }
                }
                
                if (equal)
                    return &set_state;
            }
        }

        return nullptr;
    }

    //Construct DFA of the given NFA, only called in NFA constructor.
    void construct_dfa()
    {
        typedef std::unordered_set< Q_t > New_Q_t;
        typedef std::unordered_map< std::pair< New_Q_t, S_t >, New_Q_t > New_D_t;

        std::unordered_set< S_t > new_sigma = sigma_;
        new_sigma.erase(epsilon_);
        
        New_D_t new_delta;
        New_Q_t new_initial_state = epsilon_closure(initial_state_);
        
        std::unordered_set< New_Q_t > new_states = {new_initial_state};

        std::stack< New_Q_t > to_eval; to_eval.push(new_initial_state);
        while (!to_eval.empty())
        {
            New_Q_t check_state = to_eval.top();
            to_eval.pop();

            //Use new_sigma to avoid epsilon.
            for (const S_t & c : new_sigma)
            {
                New_Q_t new_state = {};
                
                for (const Q_t & q : check_state)
                {
                    if (delta_.find({q, c}) != delta_.end())
                    {
                        //Get all epsilon closures of new states.
                        for (const Q_t & new_q : delta_.find({q, c})->second)
                            helper::merge(new_state, epsilon_closure(new_q));
                    }
                }

                /*
                  Adjust delta with correct new state.
                  This if-else may seem useless, but when constructing the
                  new set through merge, it can reorder elements, making the
                  sets not the same in the eyes of a dictionary index, so, I
                  will instead use the element already in the set if it exists.
                */
                const New_Q_t * member_ptr = get_ptr_to_member(new_states, new_state);
                if (member_ptr == nullptr)
                {
                    new_states.insert(new_state);
                    to_eval.push(new_state);
                    new_delta[{check_state, c}] = new_state;
                }

                else
                    new_delta[{check_state, c}] = *member_ptr;
               
                
            }
        }

        std::unordered_set< New_Q_t > new_accept_states;
        for (const New_Q_t & new_q : new_states)
        {
            for (const Q_t & q : new_q)
            {
                if (accept_states_.find(q) != accept_states_.end())
                {
                    new_accept_states.insert(new_q);
                    break;
                }
            }
        }

        //Create the DFA for the NFA to use.
        M_ = new DFA< S_t, std::unordered_set< Q_t > >(new_sigma,
                                                       new_states,
                                                       new_initial_state,
                                                       new_accept_states,
                                                       new_delta);
        
        return;
    }

    /// Member variables
    std::unordered_set< S_t > sigma_;
    std::unordered_set< Q_t > states_;
    Q_t initial_state_;
    std::unordered_set< Q_t > accept_states_;
    D_t delta_;
    const S_t epsilon_;

    //Inner DFA.
    DFA< S_t, std::unordered_set< Q_t > > * M_;
};


class NFA_Union_Sigma_Mismatch_Error{};

/*
   Return an NFA that is the union of two given NFA's.
   Must include a state for the new initial state.
   
   WARNING: calling this function on two NFA objects whose
   states share names will have an unintended result.
   Sending in a new initial state that is already in one of
   the NFA objects will also have an unintended result.
*/
template< typename S_t, typename Q_t >
NFA< S_t, Q_t > nfa_union(const NFA< S_t, Q_t > & N0,
                          const NFA< S_t, Q_t > & N1,
                          const Q_t & new_initial_state)
{
    // Verify the sets have the same sigma.
    if (N0.sigma().size() != N1.sigma().size())
        throw NFA_Union_Sigma_Mismatch_Error();
    for (const S_t & c : N0.sigma())
        if (N1.sigma().find(c) == N1.sigma().end())
            throw NFA_Union_Sigma_Mismatch_Error();
    
    typedef std::pair< Q_t, S_t > Q_t_S_t;
    typedef std::unordered_map< Q_t_S_t, std::unordered_set< Q_t > > D_t;
    
    std::unordered_set< Q_t > new_states;
    helper::merge(new_states, N0.states());
    helper::merge(new_states, N1.states());
    new_states.insert(new_initial_state);

    std::unordered_set< Q_t > new_accept_states;
    helper::merge(new_accept_states, N0.accept_states());
    helper::merge(new_accept_states, N1.accept_states());

    D_t new_delta;
    for (const std::pair< Q_t_S_t, std::unordered_set< Q_t > > & pair
             : N0.delta())
    {
        new_delta[pair.first] = pair.second;
    }
    for (const std::pair< Q_t_S_t, std::unordered_set< Q_t > > & pair
             : N1.delta())
    {
        new_delta[pair.first] = pair.second;
    }
    new_delta[{new_initial_state, N0.epsilon()}] = {N0.initial_state(),
                                                    N1.initial_state()};

    return NFA< S_t, Q_t >(N0.sigma(),
                           new_states,
                           new_initial_state,
                           new_accept_states,
                           new_delta,
                           N0.epsilon());
}

class NFA_Concatenation_Sigma_Mismatch_Error{};

/*
   Return an NFA that is the concatenation of two given NFA's.
   
   WARNING: calling this function on two NFA objects whose
   states share names will have an unintended result.
*/
template< typename S_t, typename Q_t >
NFA< S_t, Q_t > nfa_concat(const NFA< S_t, Q_t > & N0,
                           const NFA< S_t, Q_t > & N1)
{
    // Verify the sets have the same sigma.
    if (N0.sigma().size() != N1.sigma().size())
        throw NFA_Concatenation_Sigma_Mismatch_Error();
    for (const S_t & c : N0.sigma())
        if (N1.sigma().find(c) == N1.sigma().end())
            throw NFA_Concatenation_Sigma_Mismatch_Error();

    typedef std::pair< Q_t, S_t > Q_t_S_t;
    typedef std::unordered_map< Q_t_S_t, std::unordered_set< Q_t > > D_t;

    std::unordered_set< Q_t > new_states;
    helper::merge(new_states, N0.states());
    helper::merge(new_states, N1.states());

    std::unordered_set< Q_t > new_accept_states = N1.accept_states();

    D_t new_delta;
    for (const std::pair< Q_t_S_t, std::unordered_set< Q_t > > & pair
             : N0.delta())
    {
        new_delta[pair.first] = pair.second;
    }
    for (const std::pair< Q_t_S_t, std::unordered_set< Q_t > > & pair
             : N1.delta())
    {
        new_delta[pair.first] = pair.second;
    }

    for (const Q_t & q : N0.accept_states())
    {
        Q_t_S_t k = {q, N0.epsilon()};
        
        if (new_delta.find(k) != new_delta.end())
            new_delta[k].insert(N1.initial_state());
        else
            new_delta[k] = {N1.initial_state()};
    }

    return NFA< S_t, Q_t >(N0.sigma(),
                           new_states,
                           N0.initial_state(),
                           new_accept_states,
                           new_delta,
                           N0.epsilon());
}

#endif
