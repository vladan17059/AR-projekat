#include<iostream>
#include<vector>
#include<fstream>
#include<ctime>
#include<chrono>
#include<algorithm>
#include<deque>
#include<string>
#include"pval.hpp"
#include"dpll.hpp"
#include"cdcl.hpp"




int main(int argc, char** argv) {
    
    
    std::string filename(argv[1]);
    std::ifstream dimacs_file;

    dimacs_file.open(filename);
    DPLL dpll;
    dpll.load_dimacs(dimacs_file);
    dimacs_file.close();

    dimacs_file.open(filename);
    CDCL cdcl;
    cdcl.load_dimacs(dimacs_file);
    dimacs_file.close();

    
    clock_t start;
    clock_t end;
    std::optional<PartialValuation> optSol;

    
    start = clock();
    std::cout<<"===========\nCDCL\n-----------------\n";
    optSol = cdcl.solve();
    end = clock();

    if(!optSol.has_value())
        std::cout<<"UNSAT\n";
    else {
        std::cout<<"SAT:\n";
        optSol.value().print();
    }
    std::cout<<end-start<<" microseconds\n";
    std::cout<<"===========\n\n\n"; 


    start = clock();
    std::cout<<"===========\nDPLL\n-----------------\n";
    optSol = dpll.solve();
    end = clock();

    if(!optSol.has_value())
        std::cout<<"UNSAT\n";
    else {
        std::cout<<"SAT:\n";
        optSol.value().print();
    }
    std::cout<<end-start<<" microseconds\n";
    std::cout<<"===========\n";
    

    return 0;
}