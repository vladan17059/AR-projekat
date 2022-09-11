#ifndef __CDCL_HPP__
#define __CDCL_HPP__

#include<iostream>
#include<optional>
#include<algorithm>
#include<deque>
#include"pval.hpp"

class CDCL {

public:

    void load_dimacs(std::istream& is);
    std::optional<PartialValuation> solve();

private:
    Literal unitClause();
    Literal nextLiteral();
    Literal nextLiteralEVSIDS();
    Clause conflict();
    Clause conflictAnalysis(Clause& conflictClause);
    void backjump(unsigned level);
    unsigned backjump_level(Clause& learnedClause);

    // heuristika -  biranje promenljive za grananje (literala odluke)
    std::vector<double> scores;

    // formula (na ovo dodajemo i naucene klazue)
    NormalForm m_formula;

    // parcijalna valuacija
    PartialValuation m_valuation;

    // graf implikacija
    std::vector<std::vector<Literal>> implication_graph;

    // nivoi (kom nivou odluke pripada promenljiva - znacajno za backjump)
    std::vector<unsigned> levels;

    // decide literali - kriterijum zaustavljanja je konflikt + decisions.empty()
    std::vector<Literal> decisions;

    std::vector<bool> is_decision;


    

    // TODO - heuristike za ubrzanje CDCL algoritma (van vremenskog okvira projekta)
    // inicijalni broj klauza (pratimo zbog "zaboravljanja")
    unsigned n_clauses;
    void forget(unsigned ratio=50);
    void restart();

};

#endif //__CDCL_HPP__