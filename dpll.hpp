#ifndef __DPLL_HPP__
#define __DPLL_HPP__

#include<iostream>
#include<optional>
#include<algorithm>
#include"pval.hpp"

class DPLL {

public:
    void load_dimacs(std::istream& is);
    std::optional<PartialValuation> solve();

private:
    Literal unitClause() const;
    bool conflict() const;

    NormalForm m_formula;
    PartialValuation m_valuation;

};

#endif //__DPLL_HPP__