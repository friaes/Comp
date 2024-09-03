#ifndef __TIL_AST_SWEEP_NODE_H__
#define __TIL_AST_SWEEP_NODE_H__

#include <cdk/ast/expression_node.h>
#include <cdk/ast/basic_node.h>

namespace til {

  class sweep_node: public cdk::basic_node {
    cdk::expression_node *_vector, *_low, *_high, *_function, *_condition;

  public:
    sweep_node(int lineno, cdk::expression_node *vector, cdk::expression_node *low, cdk::expression_node *high, 
               cdk::expression_node *function, cdk::expression_node *condition) :
        basic_node(lineno), _vector(vector), _low(low), _high(high), _function(function), _condition(condition) {
    }

  public:
    cdk::expression_node* vector() {
      return _vector;
    }
    cdk::expression_node* low() {
      return _low;
    }
    cdk::expression_node* high() {
      return _high;
    }
    cdk::expression_node* function() {
      return _function;
    }
    cdk::expression_node* condition() {
      return _condition;
    }

  public:
    void accept(basic_ast_visitor *sp, int level) {
      sp->do_sweep_node(this, level);
    }

  };

} // til

#endif
