#include <cstdlib>
#include <algorithm>
#include <iostream>
#include "pval.hpp"

PartialValuation::PartialValuation(unsigned varCount) {
    init(varCount);
}

void PartialValuation::init(unsigned varCount) {
    m_value.clear();
    m_value.resize(varCount + 1, TriBool::Undefined);

    m_stack.clear();
}


void PartialValuation::push(Literal l, bool decide) {
    if(decide)
        m_stack.push_back(NullLiteral);
    m_stack.push_back(l);

    m_value[std::abs(l)] = l > 0 ? TriBool::True : TriBool::False;
}

Literal PartialValuation::backtrack() {
    Literal l = NullLiteral;
    while(!m_stack.empty() && m_stack.back() != NullLiteral) {
        l = m_stack.back();

        m_stack.pop_back();
        m_value[std::abs(l)] = TriBool::Undefined;
    }

    if(m_stack.empty())
        return NullLiteral;

    m_stack.pop_back();
    return l;
}

Literal PartialValuation::nextLiteral() const {
    auto it = std::find(begin(m_value) + 1, end(m_value), TriBool::Undefined);
    return it != end(m_value) ? distance(begin(m_value), it) : NullLiteral;
}

bool PartialValuation::isConflict(const Clause &clause) const {
    for(auto& literal : clause) {
        TriBool clausePolarity = literal > 0 ? TriBool::True : TriBool::False;
        TriBool valuationPolarity = m_value[std::abs(literal)];
        if(valuationPolarity == TriBool::Undefined ||
                valuationPolarity == clausePolarity)
            return false;
    }
    return true;
}

Literal PartialValuation::isUnitClause(const Clause &clause) const {
    Literal unit = NullLiteral;
    for(auto& literal : clause) {
        TriBool clausePolarity = literal > 0 ? TriBool::True : TriBool::False;
        TriBool valuationPolarity = m_value[std::abs(literal)];
        if(clausePolarity == valuationPolarity)
            return NullLiteral;
        if(valuationPolarity == TriBool::Undefined) {
            if(unit != NullLiteral)
                return NullLiteral;
            unit = literal;
        }
    }
    return unit;
}

void PartialValuation::print() const {
    for(auto literal : m_stack)
        if(literal == NullLiteral)
            std::cout << "| ";
        else
            std::cout << literal << ' ';
    std::cout << std::endl;
}

TriBool PartialValuation::value_of(Literal l) {
    return m_value[std::abs(l)];
}