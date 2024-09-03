#ifndef __TIL_AST_LOOP_NODE_H__
#define __TIL_AST_LOOP_NODE_H__

#include <cdk/ast/expression_node.h>

namespace til {

  class loop_node : public cdk::basic_node {
    cdk::expression_node *_condition;
    cdk::basic_node *_instruction;

  public:
    loop_node(int lineno, cdk::expression_node *condition, cdk::basic_node *instruction) :
        basic_node(lineno), _condition(condition), _instruction(instruction) {
    }

    cdk::expression_node *condition() { return _condition; }

    cdk::basic_node *instruction() { return _instruction; }

    void accept(basic_ast_visitor *sp, int level) { sp->do_loop_node(this, level); }

  };

} // til

#endif
