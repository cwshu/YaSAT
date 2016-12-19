#include <cassert>
#include <iostream>

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

    bool is_sat = DPLL_backtrack();
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
            // sat_clauses[i] = true; just remove clause
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

bool SatSolver::DPLL_backtrack(){
    // backtracking for each clause

    bool find_next = true;
    int lit_counter = 0;

    while( 1 ){
        // backtracking by loop

        if( find_next ){
            lit_counter = search_next_lit(lit_counter);

            if( lit_counter > max_var_index ){
                break;
            }

            if( literals[lit_counter].value != BoolVal::NOT_ASSIGNED ){
                continue;
            }
            
            // only init decision_literal and bt level
            backtrack_level += 1;
            backtrack_data.push_back(BT());
            decision_literals.emplace_back(lit_counter, true, 0);
        }

#ifdef DEBUG2
    std::cerr << "[decide] x" << decision_literals.back().lit_number 
              << " = " << decision_literals.back().value << std::endl;
#endif
        SatRetValue ret = imply_by(decision_literals.back().lit_number, decision_literals.back().value);

        if( ret.type == SatRetValue::NORMAL ){
            find_next = true;
            continue;
        }
        else if( ret.type == SatRetValue::CONFLICT ){

#ifdef DEBUG2
    std::cerr << "[conflict] " << ret.conflict_lit << std::endl;
#endif
            bool has_next = backtrack_next();

            if( !has_next ){
                // UNSAT
                return false;
            }

            lit_counter = decision_literals.back().lit_number;
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

    /*
     * 4 condition of each clause
     * 
     *   1. True Clause 
     *   2. Not Assigned Clause
     *   3. Unit Clause
     *   4. Conflict
     */
    
    bt_set_literal_value(lit_num, set_value);

    // do implication
    if( set_value == true ){
        set_watched_literals_true(literals[lit_num].pos_watched);
        SatRetValue ret = set_watched_literals_false(literals[lit_num].neg_watched);

        if( ret.type == SatRetValue::CONFLICT ){
            return ret;
        }
    }
    else{
        set_watched_literals_true(literals[lit_num].neg_watched);
        SatRetValue ret = set_watched_literals_false(literals[lit_num].pos_watched);

        if( ret.type == SatRetValue::CONFLICT ){
            return ret;
        }
    }
     
    // if unit clause exist, imply_by() again
    while( !unit_clause_queue.empty() ){
        LiteralIndex lit = unit_clause_queue.front();
        unit_clause_queue.pop_front();

        if( literals[lit.lit_number].value != BoolVal::NOT_ASSIGNED ){
            // if literal is ASSIGNED by previous unit clause:
                // if assigned literal cause a truth clause, then drop it.
                // else, it's conflict.
            
            if( literal_truth_in_clause(lit) == BoolVal::TRUE ){
                continue;
            }
            else{
                return SatRetValue(SatRetValue::CONFLICT, lit);
            }
        }

        return imply_by(lit);
    }

    return SatRetValue(SatRetValue::NORMAL);
}

SatRetValue SatSolver::imply_by(LiteralIndex lit_index){
    int number = all_clauses[lit_index.clause_index][lit_index.lit_index_in_clause];

#ifdef DEBUG2
    std::cerr << "[imply] " << lit_index 
              << " = " << (number > 0) << std::endl;
#endif

    return imply_by(std::abs(number), number > 0);
}

void SatSolver::set_watched_literals_true(std::vector<LiteralIndex>& watched_lits){
    /* set the value of all watched literal to true */

    for( const auto& lit_clause: watched_lits ){
        if( sat_clauses[lit_clause.clause_index] == false ){
            bt_set_clause_sat(lit_clause.clause_index);
        }
    }
    return;
}

SatRetValue SatSolver::set_watched_literals_false(std::vector<LiteralIndex>& watched_lits){
    /* set the value of all watched literal to false, sometimes it cause Unit clause or Conflict clause */

    int size = watched_lits.size();
    for( int i = 0; i < size; i++ ){
        // each watched literal's clause
        LiteralIndex& false_lit = watched_lits[i];
        SatRetValue ret = update_literal_row(false_lit);

        // HACK: fix size and i when watched_lits array elements are removed in literal updating.
        if( watched_lits.size() != size ){
            size--;
            i--;
        }

        if( ret.type == SatRetValue::CONFLICT ){
            return ret;
        }
        else if( ret.type == SatRetValue::UNIT_CLAUSE ){
            int clause_index = false_lit.clause_index;

            // unit_clause_lit is clause's 2_lit[0] or 2_lit[1]
            LiteralIndex unit_clause_lit = clause_watched_2_lit[clause_index][0];
            if( literal_truth_in_clause(unit_clause_lit) != BoolVal::NOT_ASSIGNED ){
                unit_clause_lit = clause_watched_2_lit[clause_index][1];
            }

            unit_clause_queue.push_back(unit_clause_lit);
        }
    }

    return SatRetValue(SatRetValue::NORMAL);
}

SatRetValue SatSolver::update_literal_row(LiteralIndex literal){
    SatRetValue ret = update_literal(literal.clause_index, 0);
    if( ret.type == SatRetValue::NORMAL ){
        return update_literal(literal.clause_index, 1);
    }
    else if( ret.type == SatRetValue::CONFLICT ){
        return ret;
    }
    else if( ret.type == SatRetValue::UNIT_CLAUSE ){
        return ret;
    }
    else{
        assert("unreachable: update_literal_row\n");
        return SatRetValue(SatRetValue::NORMAL);
    }
}

SatRetValue SatSolver::update_literal(int clause_index, int clause_2_lit_offset){

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

    LiteralIndex& literal = clause_watched_2_lit[clause_index][clause_2_lit_offset];
    LiteralIndex& another_watched_lit = clause_watched_2_lit[literal.clause_index][1 - clause_2_lit_offset];

    if( sat_clauses[clause_index] == true ){
        return SatRetValue(SatRetValue::NORMAL);
    }
    if( literal_truth_in_clause(literal) != BoolVal::FALSE ){
        return SatRetValue(SatRetValue::NORMAL);
    }

    std::vector<int>& this_clause = all_clauses[clause_index];
    int size = this_clause.size();

    int lit_index, last_false_lit_index = -1;
    for( lit_index = 0; lit_index < size; lit_index++ ){
        if( lit_index == literal.lit_index_in_clause ) continue;
        if( lit_index == another_watched_lit.lit_index_in_clause ) continue;

        if( literal_truth_in_clause(clause_index, lit_index) == BoolVal::FALSE ){
            last_false_lit_index = lit_index;
            continue;
        }
        else if( literal_truth_in_clause(clause_index, lit_index) == BoolVal::TRUE ){
            bt_set_clause_sat(clause_index);

            remove_literal_watch(literal);
            add_literal_watch(clause_index, lit_index, clause_2_lit_offset);
            return SatRetValue(SatRetValue::NORMAL);
        }
        else if( literal_truth_in_clause(clause_index, lit_index) == BoolVal::NOT_ASSIGNED ){
            remove_literal_watch(literal);
            add_literal_watch(clause_index, lit_index, clause_2_lit_offset);
            return SatRetValue(SatRetValue::NORMAL);
        }
    }
    
    if( last_false_lit_index != -1 ){
        // move literal to end;
        remove_literal_watch(literal);
        add_literal_watch(clause_index, last_false_lit_index, clause_2_lit_offset);
    }

    if( literal_truth_in_clause(another_watched_lit) == BoolVal::NOT_ASSIGNED ){
        // unit clause
        return SatRetValue(SatRetValue::UNIT_CLAUSE);
    }
    else if( literal_truth_in_clause(another_watched_lit) == BoolVal::FALSE ){
        // conflict
        return SatRetValue(SatRetValue::CONFLICT, literal);
    }
    else{
        assert("unreachable: update_literal\n");
        return SatRetValue(SatRetValue::NORMAL);
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
    backtrack_init();

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

    std::vector<LiteralIndex>* watched;
    if( is_positive ){
        watched = &(literals[literal.lit_number].pos_watched);   
    }
    else{
        watched = &(literals[literal.lit_number].neg_watched);
    }

    // TODO: choose better data structure to improve remove literal performance
    for( auto it = watched->begin(); it != watched->end(); it++ ){
        if( *it == literal ){
            watched->erase(it);
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

        // only init decision_literal, bt level, and empty backtrack_data
        last_decision_lit.bt_state = 1;
        last_decision_lit.value = !last_decision_lit.value;

        backtrack_data.push_back(BT());
    }
    else if( last_decision_lit.bt_state == 1 ){
        backtrack_pop();

        return backtrack_next();
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

// debug use
    
void SatSolver::print_clause_watched_2_lit(){
    int clause_size = all_clauses.size();
    for(int row = 0; row < clause_size; row++){
        printf("[clause %d] %d, %d\n", row, clause_watched_2_lit[row][0].lit_number, clause_watched_2_lit[row][1].lit_number);
    }
}

void SatSolver::print_literals(){
    for(int i = 1; i <= max_var_index; i++){
        char value;
        if( literals[i].value == BoolVal::TRUE ) value = 'T';
        if( literals[i].value == BoolVal::FALSE ) value = 'F';
        if( literals[i].value == BoolVal::NOT_ASSIGNED ) value = 'N';

        printf("[lit %d] %c\n", i, value);
        printf("pos: ");
        for( const auto& pos_lit : literals[i].pos_watched ){
            printf("%d ", pos_lit.clause_index);
        }
        printf("\n");
        printf("neg: ");
        for( const auto& neg_lit : literals[i].neg_watched ){
            printf("%d ", neg_lit.clause_index);
        }
        printf("\n");
    }
}
