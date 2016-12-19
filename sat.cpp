#include <iostream>
#include <fstream>
#include <vector>
#include <deque>
#include <cstdio>

#include "parser.h"
#include "utils.h"
#include "sat_solver.h"

void print_clauses(std::vector<Clause> clauses);
void print_sat_solution(std::ostream& output_stream, std::vector<BoolVal>& answer);

int main(int argc, char *argv[]){
    
    if( argc != 2 ){
        std::cerr << "invalid number of parameter." << std::endl;
        std::cerr << "./yasat [input.cnf]" << std::endl;
        std::exit(1);
    }
    std::string input_name = argv[1];
    std::string output_name = input_name.substr(0, input_name.size()-4);
    output_name += ".sat";

#ifdef DEBUG
    std::ostream& output_stream = std::cout;
#else
    std::fstream output_stream;
    output_stream.open(output_name, std::ios::out);
#endif

    vector_2d<int> input_clauses;
    int max_var_index;
    parse_DIMACS_CNF(input_clauses, max_var_index, input_name.c_str());

    std::vector<Clause> clauses(input_clauses);
#ifdef DEBUG
    // print_clauses(clauses);
#endif

    SatSolver solver;
    solver.set_clauses(clauses, max_var_index);
    // Solve SAT problem
    bool is_sat = solver.solve();

    if( is_sat ){
        output_stream << "s SATISFIABLE" << std::endl;

        std::vector<BoolVal> answer = solver.answer();
        print_sat_solution(output_stream, answer);
    }
    else{
        output_stream << "s UNSATISFIABLE" << std::endl;
    }

    return 0;
}

void print_sat_solution(std::ostream& output_stream, std::vector<BoolVal>& answer){
    /* print answer of SAT solution.
     *
     *   the format of (x1=0, x2=1, x3=0) is
     *   v -1 2 -3 0
     */

    output_stream << "v ";

    for( int i = 0; i < answer.size(); i++ ){
        if( answer[i] == BoolVal::TRUE ){
            output_stream << i + 1 << " ";
        }
        else if( answer[i] == BoolVal::FALSE ){
            output_stream << "-" << i + 1 << " ";
        }
        else{
            // not assigned literal: [don't care condition or error?]
#ifdef DEBUG
            output_stream << "@" << i + 1 << " ";
#else
            output_stream << i + 1 << " ";
#endif
        }
    }
    output_stream << "0" << std::endl;
}

void print_clauses(std::vector<Clause> clauses){

    for( const auto& clause : clauses ){
        for( const auto& literal : clause ){
            std::cout << literal << " ";
        }
        std::cout << std::endl;
    }
}
