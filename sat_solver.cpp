#include <cassert>

#include "sat_solver.h"
#include "utils.h"

std::ostream& operator << (std::ostream& os, const BoolVal& value){
    if( value == BoolVal::TRUE )        { os << "true"; }
    if( value == BoolVal::FALSE )       { os << "false"; }
    if( value == BoolVal::NOT_ASSIGNED ){ os << "not assigned"; }
    return os;
}

// SatSolver
void SatSolver::set_clauses(const std::vector<Clause>& clauses, int max_var_index){
    all_clauses = clauses;
    this->max_var_index = max_var_index;
    clear_and_resize();
}

bool SatSolver::solve(){
    remove_unit_clause_init();
    add_2_lit_watch_each_clause();
    bool is_sat = make_decision();
    return is_sat;
}

std::vector<BoolVal> SatSolver::answer() const {
    std::vector<BoolVal> ret;
    for(int i = 1; i <= max_var_index; i++){
        ret.push_back(literals[i].value);
    }
    return ret;
}


void SatSolver::remove_unit_clause_init(){
    std::vector<Clause> copy_clauses;

    int clause_size = all_clauses.size();
    for( int i = 0; i < clause_size; i++ ){
        Clause& this_clause = all_clauses[i];
        if( this_clause.size() == 1 ){
            // unit clause
            give_literal_value_for_clause_true(i, 0);
            sat_clauses[i] = true;
            continue;
        }
        
        copy_clauses.push_back(this_clause);
    }

    all_clauses = copy_clauses;
}


void SatSolver::add_2_lit_watch_each_clause(){
    // 1. add 2 literal watching for every clause
    int clause_size = all_clauses.size();
    for( int clause_index = 0; clause_index < clause_size; clause_index++ ){
        Clause& clause = all_clauses[clause_index];
        assert(clause.size() >= 2);
        
        add_literal_watch(clause_index, 0, 0);
        add_literal_watch(clause_index, 1, 1);
    }
}

bool SatSolver::make_decision(){
    // 2. make decision for each clause

    // vector<bool> accept_clauses;
    // accept_clauses.resize(clauses.size(), false);

    bool find_next = true;
    int lit_counter = 0;

    while( 1 ){
        // backtracking by loop

        if( find_next ){
            int next_lit = search_next_lit(lit_counter);

            if( literals[next_lit].value != BoolVal::NOT_ASSIGNED ){
                continue;
            }

            // only init decision_literal and bt level
            backtrack_level += 1;
            decision_literals.emplace_back(next_lit, true, 0);
        }

        SatRetValue ret = imply_by(decision_literals.back().lit_number, decision_literals.back().value);

        if( ret == SatRetValue::NORMAL ){
            find_next = true;
            continue;
        }
        else if( ret == SatRetValue::CONFLICT ){
            bool is_no_next = backtrack_next();

            if( is_no_next ){
                // UNSAT
                return false;
            }

            find_next = false;
            continue;
        }
    }

    return true;
}

/* based on 2-literal watching */
SatRetValue SatSolver::imply_by(int lit_num, bool set_value){
    /*
     * make implication when the literal is set;
     * return normal, or conflict
     */

    // change value of 2 literal watching => literals[i].pos_watched, clause_watched_2_lit
    // change value of literal value (implication) => literals[i].value

    // set lit = true or false
    // for each pos_watch_lit or neg_watch_lit == true
       // the clause is true.
    // for each pos_watch_lit or neg_watch_lit == false
       // find_watched_literal_in_clause(literal);

    // while !unit_clause_queue.empty()
       // imply_by(unit_clause_queue.pop_front())

    // find_watched_literal_in_clause():
       // [Define] T = True, F = False, N = Not Assigned
       // [Define] (T, F) means another lit = T && current lit = F
       
       // if (T,  )    => return;
       // while
          // pick next literal in clause
          // if ( , T) => return;
          // if (N, N) => set_watched_literal(literal);
          // if ( , F) => continue;

       // if last one literal // (, F)
          // if (N, F) => unit_clause_queue.push_back(literal);
          // if (F, F) => conflict();

    /*
     * 4 condition of each clause
     * 
     *   1. True Clause 
     *   2. Not Assigned Clause
     *   3. Unit Clause
     *   4. Conflict
     */
    
    bt_set_literal_value(lit_num, set_value);
    if( set_value == true ){
        for( const auto& pos_lit_clause: literals[lit_num].pos_watched ){
            bt_set_clause_sat(pos_lit_clause.clause_index);
        }

        int size = literals[lit_num].neg_watched.size();
        for( int i = 0; i < size; ){
            // each negative watched literal's clause
            LiteralIndex& neg_lit_index = literals[lit_num].neg_watched[i];
            SatRetValue ret = update_literal_row(neg_lit_index);

            if( ret == SatRetValue::CONFLICT ){
                return ret;
            }
            else if( ret == SatRetValue::UNIT_CLAUSE ){
                int clause_index = neg_lit_index.clause_index;

                LiteralIndex unit_clause_lit_index = clause_watched_2_lit[clause_index][0];
                if( literal_truth_in_clause(unit_clause_lit_index) != BoolVal::NOT_ASSIGNED ){
                    unit_clause_lit_index = clause_watched_2_lit[clause_index][1];
                }

                unit_clause_queue.push_back(unit_clause_lit_index);
            }
        }
    }
    else{
        for( const auto& neg_lit_clause: literals[lit_num].neg_watched ){
            bt_set_clause_sat(neg_lit_clause.clause_index);
        }

        int size = literals[lit_num].pos_watched.size();
        for( int i = 0; i < size; ){
            // each positive watched literal's clause
            LiteralIndex& pos_lit_index = literals[lit_num].pos_watched[i];
            SatRetValue ret = update_literal_row(pos_lit_index);

            if( ret == SatRetValue::CONFLICT ){
                return ret;
            }
            else if( ret == SatRetValue::UNIT_CLAUSE ){
                int clause_index = pos_lit_index.clause_index;

                LiteralIndex unit_clause_lit_index = clause_watched_2_lit[clause_index][0];
                if( literal_truth_in_clause(unit_clause_lit_index) != BoolVal::NOT_ASSIGNED ){
                    unit_clause_lit_index = clause_watched_2_lit[clause_index][1];
                }

                unit_clause_queue.push_back(unit_clause_lit_index);
            }
        }
    }
    
    if( !unit_clause_queue.empty() ){
        LiteralIndex lit = unit_clause_queue.front();
        unit_clause_queue.pop_front();
        return imply_by(lit);
    }

    return SatRetValue::NORMAL;
}

SatRetValue SatSolver::imply_by(LiteralIndex lit_index){
    int number = all_clauses[lit_index.clause_index][lit_index.lit_index_in_clause];
    return imply_by(std::abs(number), number > 0);
}

SatRetValue SatSolver::update_literal_row(LiteralIndex literal){
    SatRetValue ret = update_literal(literal.clause_index, 0);
    if( ret == SatRetValue::NORMAL ){
        return update_literal(literal.clause_index, 1);
    }
    else if( ret == SatRetValue::CONFLICT ){
        return ret;
    }
    else if( ret == SatRetValue::UNIT_CLAUSE ){
        return ret;
    }
}

SatRetValue SatSolver::update_literal(int clause_index, int clause_2_lit_offset){
    LiteralIndex& literal = clause_watched_2_lit[clause_index][clause_2_lit_offset];
    LiteralIndex& another_watched_lit = clause_watched_2_lit[literal.clause_index][1 - clause_2_lit_offset];

    if( sat_clauses[clause_index] == true ){
        return SatRetValue::NORMAL;
    }
    if( literal_truth_in_clause(literal) != BoolVal::FALSE ){
        return SatRetValue::NORMAL;
    }

    std::vector<int>& this_clause = all_clauses[clause_index];
    int size = this_clause.size();

    int lit_index;
    for( lit_index = 0; lit_index < size; lit_index++ ){
        if( lit_index == literal.lit_index_in_clause ) continue;
        if( lit_index == another_watched_lit.lit_index_in_clause ) continue;

        if( literal_truth_in_clause(clause_index, lit_index) == BoolVal::FALSE ){
            continue;
        }
        else if( literal_truth_in_clause(clause_index, lit_index) == BoolVal::TRUE ){
            bt_set_clause_sat(clause_index);

            remove_literal_watch(literal);
            add_literal_watch(clause_index, lit_index, clause_2_lit_offset);
            return SatRetValue::NORMAL;
        }
        else if( literal_truth_in_clause(clause_index, lit_index) == BoolVal::NOT_ASSIGNED ){
            remove_literal_watch(literal);
            add_literal_watch(clause_index, lit_index, clause_2_lit_offset);
            return SatRetValue::NORMAL;
        }
    }

    // move literal to end;
    remove_literal_watch(literal);
    add_literal_watch(clause_index, lit_index, clause_2_lit_offset);
    if( lit_index == size ){
        if( literal_truth_in_clause(another_watched_lit) == BoolVal::NOT_ASSIGNED ){
            // unit clause
            return SatRetValue::UNIT_CLAUSE;
        }
        else if( literal_truth_in_clause(another_watched_lit) == BoolVal::FALSE ){
            // conflict
            return SatRetValue::CONFLICT;
        }
    }
}

/* helper functions */

int SatSolver::search_next_lit(int lit_counter){
    return lit_counter+1;
}

void SatSolver::clear_and_resize(){
    literals.clear();
    sat_clauses.clear();
    clause_watched_2_lit.clear();

    int clause_size = all_clauses.size();
    literals.resize(max_var_index + 1);
    sat_clauses.resize(clause_size, false);
    clause_watched_2_lit.resize(clause_size);
}

BoolVal SatSolver::literal_truth_in_clause(int clause_index, int lit_index_in_clause){
    int literal = all_clauses[clause_index][lit_index_in_clause];
    BoolVal lit_value = literals[std::abs(literal)].value;

    if( lit_value == BoolVal::NOT_ASSIGNED ){
        return BoolVal::NOT_ASSIGNED;
    }
    if( lit_value == BoolVal::TRUE && literal > 0 ){
        return BoolVal::TRUE;
    }
    if( lit_value == BoolVal::FALSE && literal < 0 ){
        return BoolVal::TRUE;
    }

    return BoolVal::FALSE;
}

BoolVal SatSolver::literal_truth_in_clause(LiteralIndex lit_index){
    return literal_truth_in_clause(lit_index.clause_index, lit_index.lit_index_in_clause);
}


void SatSolver::add_literal_watch(LiteralIndex new_lit, int clause_2_lit_offset){
    // add pos/neg_watched in literals and clause_watched_2_lit
    bool is_positive = all_clauses[new_lit.clause_index][new_lit.lit_index_in_clause] > 0;

    clause_watched_2_lit[new_lit.clause_index][clause_2_lit_offset] = new_lit;

    if( is_positive )
        literals[new_lit.lit_number].pos_watched.push_back(new_lit);
    else
        literals[new_lit.lit_number].neg_watched.push_back(new_lit);

}

void SatSolver::add_literal_watch(int clause_index, int lit_index, int watched_index){
    // add pos/neg_watched in literals and clause_watched_2_lit
    int number = std::abs(all_clauses[clause_index][lit_index]);
    LiteralIndex new_lit(number, clause_index, lit_index);
    add_literal_watch(new_lit, watched_index);
}

void SatSolver::remove_literal_watch(LiteralIndex literal){
    // only remove pos/neg_watched in literals, not remove in clause_watched_2_lit
    bool is_positive = all_clauses[literal.clause_index][literal.lit_index_in_clause] > 0;

    std::vector<LiteralIndex>& watched = literals[literal.lit_number].pos_watched;
    if( !is_positive ){
        watched = literals[literal.lit_number].neg_watched;
    }

    for( auto it = watched.begin(); it != watched.end(); it++ ){
        if( *it == literal ){
            watched.erase(it);
            break;
        }
    }
}

void SatSolver::give_literal_value_for_clause_true(int clause_index, int lit_index){
    int number = all_clauses[clause_index][lit_index];

    if( number > 0 ){
        literals[std::abs(number)].value = BoolVal::TRUE;
    }
    else{
        literals[std::abs(number)].value = BoolVal::FALSE;
    }
}

// backtrack
void SatSolver::backtrack_init(){
    backtrack_level = 0;
    decision_literals.clear();
    unit_clause_queue.clear();
    backtrack_data.clear();
}

bool SatSolver::backtrack_next(){
    if( backtrack_level == 0 ) return false;
        
    LiteralDecideNode& last_decision_lit = decision_literals[backtrack_level-1];

    if( last_decision_lit.bt_state == 0 ){
        remove_last_backtrack_data();
        unit_clause_queue.clear();

        // only init decision_literal and bt level
        last_decision_lit.bt_state = 1;
        last_decision_lit.value = ~ last_decision_lit.value;
    }
    else if( last_decision_lit.bt_state == 1 ){
        backtrack_pop();
        backtrack_next();
    }

    return true;
}

void SatSolver::backtrack_pop(){
    if( backtrack_level == 0 ) return;
        
    remove_last_backtrack_data();
    unit_clause_queue.clear();
    decision_literals.pop_back();
    backtrack_level -= 1;
}

void SatSolver::remove_last_backtrack_data(){
    if( backtrack_level == 0 ) return;
        
    BT& last_layer = backtrack_data[backtrack_level - 1];
    for( auto lit_num : last_layer.updated_literals ){
        literals[lit_num].value = BoolVal::NOT_ASSIGNED;
    }
    for( auto clause_index : last_layer.updated_sat_clauses ){
        sat_clauses[clause_index] = false;
    }

    backtrack_data.pop_back();
}

void SatSolver::bt_set_clause_sat(int clause_index){
    sat_clauses[clause_index] = true;
    backtrack_data[backtrack_level - 1].updated_sat_clauses.push_back(clause_index);
}

void SatSolver::bt_set_literal_value(int lit_num, bool value){
    literals[lit_num].value = to_bool_val(value);
    backtrack_data[backtrack_level - 1].updated_literals.push_back(lit_num);
}

void SatSolver::bt_set_literal_value(int lit_num, BoolVal value){
    literals[lit_num].value = value;
    backtrack_data[backtrack_level - 1].updated_literals.push_back(lit_num);
}
