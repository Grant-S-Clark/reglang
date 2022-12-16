#ifndef DFA_H
#define DFA_H

#include "Common.h"
#include "NFA.h"

// S_t = Type of values in Sigma (Alphabet).
// Q_t = State type, Type of values in Q.
template < typename S_t, typename Q_t >
class DFA
{
public:
    typedef std::pair< Q_t, S_t > Q_t_S_t;
    typedef std::unordered_map< Q_t_S_t, Q_t > D_t;
    
    DFA(const std::unordered_set< S_t > & sigma,
        const std::unordered_set< Q_t > & states,
        const Q_t & initial_state,
        const std::unordered_set< Q_t > & accept_states,
        const D_t & delta)
        : sigma_(sigma),
          states_(states),
          initial_state_(initial_state),
          accept_states_(accept_states),
          delta_(delta)
    {}

    DFA(const DFA< S_t, Q_t > & M)
    { *this = M; }

    DFA< S_t, Q_t > & operator=(const DFA< S_t, Q_t > & M)
    {
        sigma_ = M.sigma_;
        states_ = M.states_;
        initial_state_ = M.initial_state_;
        accept_states_ = M.accept_states_;
        delta_ = M.delta_;

        return *this;
    }

    ~DFA(){}

    // Returns true if this DFA accepts a given string of characters
    // in sigma, false otherwise.
    bool operator()(const std::vector< S_t > & str) const
    {
        Q_t state = initial_state_;

        for (const S_t & c : str)
        {
            //Make sure this character is in sigma.
            validate(c);

            state = delta_.find({state, c})->second;
        }
        
        return is_accepting(state);
    }

    /*
      Return a vector of strings that each hold a different
      instantaneous descrition of each step in the string
      computation.

      ret[0] = the string by itself as a vector.
      ret[i] = step number (i - 1) of the computation.
    */
    std::vector< std::string > IDs(std::vector< S_t > & str) const
    {
        std::vector< std::string > ret(str.size() + 2);
        int i = 0;
        ret[i] = std::to_string(str);
        ++i;
        
        std::string app;
        Q_t state = initial_state_;
        S_t c;

        while (str.size() > 0)
        {
            ret[i++] = "(" + std::to_string(state) + ", " +
                std::to_string(str) + ")";
            c = str[0];
            str.erase(str.begin());
            validate(c);
            state = delta_.find({state, c})->second;
        }

        ret[i] = "(" + std::to_string(state) + ", [])";

        //Use a '*' to denote if the final state is accepting.
        if (is_accepting(state))
            ret[i] += "*";
        
        return ret;
    }

    // Return a DFA that is the compliment of the original.
    DFA< S_t, Q_t > compliment() const
    {
        DFA< S_t, Q_t > ret = *this;

        std::unordered_set< Q_t > new_accept_states;
        for (const Q_t & q : ret.states_)
            if (!ret.is_accepting(q))
                new_accept_states.insert(q);
        
        ret.accept_states_ = new_accept_states;

        return ret;
    }

    // Return a DFA that is the minimal DFA of the original.
    DFA< Q_t, S_t > minimal() const
    {
        std::unordered_set< Q_t > reachable_states;
        reachable_states.insert(initial_state_);

        //Find all reachable states.
        for (const Q_t & q : states_)
        {
            for (const S_t & c : sigma_)
            {
                Q_t next_state = delta_.find({q, c})->second;

                if (reachable_states.find(next_state) == reachable_states.end())
                    reachable_states.insert(next_state);
            }
        }

        //Remove any states that are not reachable.
        std::unordered_set< Q_t > new_states = states_;
        for (const Q_t & q : new_states)
            if (reachable_states.find(q) == reachable_states.end())
                new_states.erase(q);

        //Old and new partitions.
        std::vector < std::unordered_set < Q_t > > * old_p, *new_p;

        old_p = new std::vector< std::unordered_set < Q_t > >;
        new_p = new std::vector< std::unordered_set < Q_t > >;

        //Divide up the states into 2 partitions,
        //accepting and non-accepting.
        old_p->resize(2);
        for (const Q_t & q : states_)
        {
            if (accept_states_.find(q) != accept_states_.end())
                old_p->at(0).insert(q);
            else
                old_p->at(1).insert(q);
        }

        std::unordered_set< Q_t > new_accept_states;
        D_t new_delta;
        Q_t new_initial_state;

        //Always accepts.
        if (old_p->at(0).size() == 0)
        {
            new_states.insert(initial_state_);
            new_accept_states.insert(initial_state_);
            for (const S_t & c : sigma_)
                new_delta[{initial_state_, c}] = initial_state_;
            new_initial_state = initial_state_;
        }

        //Never accepts.
        else if (old_p->at(1).size() == 0)
        {
            new_states.insert(initial_state_);
            for (const S_t & c : sigma_)
                new_delta[{initial_state_, c}] = initial_state_;
            new_initial_state = initial_state_;
        }

        //Minimize
        else
        {
            std::unordered_map< Q_t, std::unordered_set< Q_t >* > locations;
            while (true)
            {
                //Capture which item is in which set for equivalence checking.
                locations.clear();
                for (int k = 0, n = old_p->size(); k < n; ++k)
                    for (const Q_t & q : old_p->at(k))
                        locations[q] = &(old_p->at(k));

                //Allocate space for equivalent and non-equivalent states.
                new_p->resize(old_p->size() * 2);
                
                //Loop through the sets of the old partition.
                for (int k = 0, n = old_p->size(); k < n; ++k)
                {
                    //insert that state into new_p's (k * 2)th set.
                    new_p->at(k * 2).insert(*(old_p->at(k).begin()));

                    //Loop through other elements to see if they are equivalent.
                    Q_t first = *(old_p->at(k).begin());
                    for (const Q_t & q : old_p->at(k))
                    {
                        //skip first element, we already did this one.
                        if (&q == &(*(old_p->at(k).begin())))
                            continue;

                        bool equivalent = true;
                        for (const S_t & c : sigma_)
                        {
                            if (locations[delta_.find({q, c})->second] !=
                                locations[delta_.find({first, c})->second])
                            {
                                equivalent = false;
                                break;
                            }
                        }

                        if (equivalent)
                            new_p->at(k * 2).insert(q);
                        else
                            new_p->at(k * 2 + 1).insert(q);
                    }
                }

                //Remove any empty sets.
                for (int k = 0; k < new_p->size(); ++k)
                {
                    if (new_p->at(k).size() == 0)
                    {
                        new_p->erase(new_p->begin() + k);
                        --k;
                    }
                }
                
                //Check to see if we are finished.
                if (old_p->size() == new_p->size())
                    break;
                
                //Swap partitions and clear the new partition once swapped.
                auto t = old_p;
                old_p = new_p;
                new_p = t;
                new_p->clear();
            }

            ///Assign new values to variables.
            
            locations.clear(); //Clear locations for usage again.
            
            //find initial state and insert all new states.
            //Also adjust for the accept states.
            for (int k = 0, n = old_p->size(); k < n; ++k)
            {
                Q_t first = *(old_p->at(k).begin());
                
                //capture initial state.
                if (old_p->at(k).find(initial_state_) != old_p->at(k).end())
                    new_initial_state = first;
                
                //Insert the new state.
                new_states.insert(first);

                //Find if this state should be accepting or not
                if (accept_states_.find(first) != accept_states_.end())
                    new_accept_states.insert(first);
                //Capture the location of all old states
                //(record what set they are in for updating delta later)
                for (const Q_t & q : old_p->at(k))
                    locations[q] = &(old_p->at(k));
            }

            //Adjust delta.
            for (const std::unordered_set< Q_t > & set : *(old_p))
            {
                //Capture first element in set.
                Q_t first = *(set.begin());

                //Adjust delta for all c in sigma.
                for (const S_t & c : sigma_)
                {
                    Q_t new_q = delta_.find({first, c})->second;
                    new_delta[{first, c}] = *(locations[new_q]->begin());
                }
            }
        }

        //Free memory
        delete old_p;
        delete new_p;
        
        return DFA< S_t, Q_t >(sigma_,
                               new_states,
                               new_initial_state,
                               new_accept_states,
                               new_delta);
    }


    // Returns this DFA as an NFA. Must provide an epsilon character.
    NFA< S_t, Q_t > to_nfa(const S_t & epsilon) const
    {
        typedef std::unordered_set< Q_t > New_Q_t;
        typedef std::unordered_map< Q_t_S_t, New_Q_t > New_D_t;
        // New epsilon character cannot be in sigma.
        if (sigma_.find(epsilon) != sigma_.end())
            throw DFA_To_NFA_Invalid_Epsilon_Error();

        std::unordered_set< S_t > new_sigma = sigma_;
        new_sigma.insert(epsilon);

        std::unordered_set< std::unordered_set< Q_t > > new_states;
        std::unordered_set< std::unordered_set< Q_t > > new_accept_states;
        for (const Q_t & q : states_)
        {
            new_states.insert({q});
            if (is_accepting(q))
                new_accept_states.insert({q});
        }

        New_D_t new_delta;
        for (const std::pair< Q_t_S_t, Q_t > & pair : delta_)
            new_delta[{pair.first.first, pair.first.second}] = {pair.second};

        return NFA< Q_t, S_t >(new_sigma,
                               new_states,
                               initial_state_,
                               new_accept_states,
                               new_delta,
                               epsilon);
    }

    // Return true if this is a valid DFA.
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

        // Validate all values of delta
        std::unordered_map< Q_t, std::unordered_set< S_t > > state_checks;
        for (const std::pair< Q_t_S_t, Q_t > & p : delta_)
        {
            //Check if the state is valid.
            if (states_.find(p.first.first) == states_.end())
                return false;
            
            //Check if the character is within the alphabet.
            if (sigma_.find(p.first.second) == sigma_.end())
                return false;

            //Check to make sure that state does not already have a
            //transition with that same alphabet character.
            if (state_checks[p.first.first].find(p.first.second) != state_checks[p.first.first].end())
                return false;
            
            //Put that alphabet character into the checking map's
            //set for that specific state.
            state_checks[p.first.first].insert(p.first.second);

            //Make sure the state that it travels to exists.
            if (states_.find(p.second) == states_.end())
                return false;
        }

        //Validate that the correct amount of states are present.
        if (state_checks.size() != states_.size())
            return false;

        //Validate that each state has the correct amount of transitions.
        for (const std::pair< Q_t, std::unordered_set< S_t > > & p : state_checks)
        {
            if (p.second.size() != sigma_.size())
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

    // Returns true if the state is an accepting state.
    inline bool is_accepting(const Q_t & q) const
    {
        return accept_states_.find(q) != accept_states_.end();
    }
private:

    // Validates characters in operator() string.
    inline void validate(const S_t & s) const
    {
        if (sigma_.find(s) == sigma_.end())
            throw DFA_Invalid_Sigma_Character_Error();
        
        return;
    }
    
    std::unordered_set< S_t > sigma_;
    std::unordered_set< Q_t > states_;
    Q_t initial_state_;
    std::unordered_set< Q_t > accept_states_;
    D_t delta_;
};


///// NON-MEMBER FUNCTIONS \\\\\


class DFA_Sigma_Mismatch_Intersection_Error{};

/*
  Returns a DFA that is the interseciton of DFA M0 and M1.
  DFA M0 and M1 MUST have exactly the same sigma.
  The returned DFA does not have unreachable states removed.
*/
template< typename S_t, typename Q_t0, typename Q_t1 >
DFA< S_t, std::pair< Q_t0, Q_t1 > > dfa_intersection(
    const DFA< S_t, Q_t0 > & M0,
    const DFA< S_t, Q_t1 > & M1
    )
{
    //Sigma must be the same for both DFA.
    if (M0.sigma() != M1.sigma())
        throw DFA_Sigma_Mismatch_Intersection_Error();
    
    typedef std::pair< Q_t0, Q_t1 > New_Q_t;
    typedef std::pair< New_Q_t, S_t > New_Q_t_S_t;
    typedef std::unordered_map< New_Q_t_S_t, New_Q_t > New_D_t;

    // Get new set of states.
    std::unordered_set< New_Q_t > new_states;
    for (const Q_t0 & q0 : M0.states())
        for (const Q_t1 & q1 : M1.states())
            new_states.insert({q0, q1});

    //Get new initial state.
    New_Q_t new_initial_state = { M0.initial_state(),
                                  M1.initial_state() };

    //Get new delta.
    New_D_t new_delta;
    for (const New_Q_t & q : new_states)
    {
        Q_t0 old_q0 = q.first, next_q0;
        Q_t1 old_q1 = q.second, next_q1;
        for (const S_t & c : M0.sigma())
        {
            next_q0 = M0.delta().find({old_q0, c})->second;
            next_q1 = M1.delta().find({old_q1, c})->second;

            new_delta[{q, c}] = {next_q0, next_q1};
        }
    }

    std::unordered_set< New_Q_t > new_accept_states;
    for (const New_Q_t & q : new_states)
    {
        Q_t0 q0 = q.first;
        if (M0.is_accepting(q0))
        {
            new_accept_states.insert(q);
            continue;
        }

        Q_t1 q1 = q.second;
        if (M1.is_accepting(q1))
            new_accept_states.insert(q);
    }

    return DFA< S_t, New_Q_t >(M0.sigma(),
                               new_states,
                               new_initial_state,
                               new_accept_states,
                               new_delta);
}

#endif
