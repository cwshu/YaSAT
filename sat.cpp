#include <iostream>
#include <vector>
#include <deque>

#include "parser.h"
#include "utils.h"
#include "sat_solver.h"

void print_clauses(std::vector<Clause> clauses);

int main(int argc, char *argv[]){
    
    if( argc != 2 ){
        std::cerr << "error input parameter" << std::endl;
        std::cerr << "./yasat [input.cnf]" << std::endl;
    }
    std::string input_name = argv[1];

    vector_2d<int> input_clauses;
    int max_var_index;
    parse_DIMACS_CNF(input_clauses, max_var_index, input_name.c_str());

    std::vector<Clause> clauses(input_clauses);
    // print_clauses(clauses);

    SatSolver solver;
    solver.set_clauses(clauses, max_var_index);
    bool is_sat = solver.solve();

    /*
    if( is_sat ){
        std::cout << "SAT" << std::endl;
        auto& answer = solver.answer();
        for( const auto& value : answer ){
            std::cout << value.number << ": " << value.is_sat << std::endl;
        }
    }
    else{
        std::cout << "UNSAT" << std::endl;
    }
    */

    return 0;
}

void print_clauses(std::vector<Clause> clauses){

    for( const auto& clause : clauses ){
        for( const auto& literal : clause ){
            std::cout << literal << " ";
        }
        std::cout << std::endl;
    }
}
