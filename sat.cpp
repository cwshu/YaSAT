#include <iostream>
#include <fstream>
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
    // print_clauses(clauses);

    SatSolver solver;
    solver.set_clauses(clauses, max_var_index);
    bool is_sat = solver.solve();

    if( is_sat ){
        output_stream << "s SATISFIABLE" << std::endl;
        output_stream << "v ";
        std::vector<BoolVal> answer = solver.answer();
        for( int i = 0; i < answer.size(); i++ ){
            if( answer[i] == BoolVal::TRUE ){
                output_stream << i + 1 << " ";
            }
            else if( answer[i] == BoolVal::FALSE ){
                output_stream << "-" << i + 1 << " ";
            }
            else{
                // error, not assigned literal
                output_stream << "@" << i + 1 << " ";
            }
            // output_stream << i + 1 << ": " << answer[i] << std::endl;
        }
        output_stream << "0" << std::endl;
    }
    else{
        output_stream << "s UNSATISFIABLE" << std::endl;
    }

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
