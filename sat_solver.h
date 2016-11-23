#ifndef __SAT_SOLVER_H__
#define __SAT_SOLVER_H__

#include <vector>
#include <array>
#include <deque>
#include <ostream>

// 2 literal watching

// search
//
//   1. find literal
//      lit_number => watcher[lit_number]
//
//   2. find clause
//      clause_row => clauses[clause_row]
//      2.a. satisfied clause
//           satisfied_clause[clause_row] == true;
//      2.b. watched literal in clause
//           clause_watched_2_lit[clause_row]

using Clause = std::vector<int>;

enum class SatRetValue {
    NORMAL,
    UNIT_CLAUSE,
    CONFLICT,
};

enum class BoolVal {
    NOT_ASSIGNED,
    TRUE,
    FALSE,

};

std::ostream& operator << (std::ostream& os, const BoolVal& value);

static BoolVal to_bool_val(bool value){
    if( value == true ) return BoolVal::TRUE;
    return BoolVal::FALSE;
}

struct LiteralIndex {
    int lit_number;
    int clause_index;
    int lit_index_in_clause;

    LiteralIndex() : lit_number(-1) {}
    LiteralIndex(int lit_number, int clause_index, int lit_index_in_clause) :
        lit_number(lit_number), clause_index(clause_index), lit_index_in_clause(lit_index_in_clause) {}

    bool operator == (const LiteralIndex& other) const {
        if( this->clause_index != other.clause_index ) return false;
        if( this->lit_index_in_clause != other.lit_index_in_clause ) return false;
        if( this->lit_number != other.lit_number ) return false;
        return true;   
    }
};

using LiteralIndexPair = std::array<LiteralIndex, 2>;

struct WatchedLiteral {
    BoolVal value;
    std::vector<LiteralIndex> pos_watched;
    std::vector<LiteralIndex> neg_watched;

};

struct LiteralDecideNode {
    int lit_number;
    bool value;
    int bt_state; // backtrack state
        // state 0 -> invert value and state 1
        // state 1 -> pop LiteralDecideNode;
    LiteralDecideNode() : lit_number(-1) {}
    LiteralDecideNode(int lit_number, bool value, int bt_state) : 
      lit_number(lit_number), value(value), bt_state(bt_state) {}
};

class SatSolver {
public:

    // debug use
    
    void print_clause_watched_2_lit();
    void print_literals();
    
    // APIs

    void set_clauses(const std::vector<Clause>& clauses, int max_var_index);
    bool solve();
    std::vector<BoolVal> answer() const;

    void remove_unit_clause_init();
    void add_2_lit_watch_each_clause();
    bool make_decision();
    SatRetValue imply_by(LiteralIndex lit_index);
    SatRetValue imply_by(int lit_num, bool set_value);
    // conflict();

    SatRetValue update_literal_row(LiteralIndex literal);
    SatRetValue update_literal(int clause_index, int clause_2_lit_offset);

    // helper functions of internal data
    
    int search_next_lit(int lit_counter);
    // add_new_clause();
    void clear_and_resize();

    BoolVal literal_truth_in_clause(int clause_index, int lit_index_in_clause);
    BoolVal literal_truth_in_clause(LiteralIndex lit_index);

    void add_literal_watch(int clause_index, int lit_index, int watched_index);
    void add_literal_watch(LiteralIndex new_lit, int clause_2_lit_offset);
    void remove_literal_watch(LiteralIndex literal);
    void give_literal_value_for_clause_true(int clause_index, int lit_index);

    void backtrack_init();
    bool backtrack_next();
    void backtrack_pop();
    void remove_last_backtrack_data();

    void bt_set_clause_sat(int clause_index);
    void bt_set_literal_value(int lit_num, bool value);
    void bt_set_literal_value(int lit_num, BoolVal value);

    // clauses map
    int max_var_index;
    std::vector<Clause> all_clauses;

    // internal data
    std::vector<WatchedLiteral> literals; // literal use 1-based array
    std::vector<bool> sat_clauses;        // clause  use 0-based array
    std::vector<LiteralIndexPair> clause_watched_2_lit;

    // backtrack
    int backtrack_level;
    std::deque<LiteralIndex> unit_clause_queue; // push the unique not_assigned literal into the queue.
    std::vector<LiteralDecideNode> decision_literals;

    struct BT {
        std::vector<int> updated_literals;
        std::vector<int> updated_sat_clauses;
    };
    std::vector<BT> backtrack_data;
       // decision literal 
       // implied literal
       // true sat_clauses
       // unit_clause_queue
       // -- literal watch --
};

#endif /* end of include guard: __SAT_SOLVER_H__ */
