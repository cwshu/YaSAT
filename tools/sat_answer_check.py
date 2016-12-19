#!/usr/bin/env python3

import functools
import operator
import sys

def parse_ans(ans_file):
#    answer_content = '''s SATISFIABLE
#v 1 2 -3 4 0'''

    answer_content = ans_file.read()

    answer_content = answer_content.splitlines()[1]
    answer_content = answer_content.split()[1:-1]

    answer_size = len(answer_content)
    answer = { abs(int(value)): int(value) > 0 for value in answer_content }

    return answer, answer_size

def parse_problem(problem_file):
#     problem_content = '''c xd
# p cnf 3 4
# 1 -2 0
# 1 3 0
# 2 -3 0
# -1 0'''

    problem_content = problem_file.read()

    problem = []
    problem_lines = problem_content.splitlines()
    for line in problem_lines:
        if line[0] == 'c':
            continue
        elif line[0] == 'p':
            metadata = line.split()
            assert metadata[1] == 'cnf', 'not cnf'
            problem_clause = metadata[2]
            problem_literal = metadata[3]
        else:
            clause = line.split()
            assert clause[-1] == '0', 'no 0 end'
            clause.pop()
            clause = { abs(int(value)): int(value) > 0 for value in clause }
            problem.append(clause)

    return problem

def check_answer(problem, answer):
    for idx, clause in enumerate(problem, 0):
        # print(clause)
        sat = [ not(clause[k] ^ answer[k]) for k in clause ]

        # print(sat)
        is_sat = functools.reduce(operator.or_, sat)

        if not is_sat:
            print('false at clause {}: '.format(idx))
            print_clause(clause)
            return False

    return True

def print_clause(clause):
    for k, v in clause.items():
        if v:
            print(k, end=' ')
        else:
            print('-', k, sep='', end=' ')
    print('0')

def main():
    if len(sys.argv) != 3:
        print('invalid number of arguments')
        print('./{} <problem> <answer>')
        sys.exit(1)

    problem_file = open(sys.argv[1], 'r')
    answer_file = open(sys.argv[2], 'r')

    problem = parse_problem(problem_file)
    answer, answer_size = parse_ans(answer_file)

    # print("[p]", problem)
    # print("[a]", answer)

    ret = check_answer(problem, answer)
    if ret:
        print('SAT')

main()
