#ifndef __TIL_AST_WITH_NODE_H__
#define __TIL_AST_WITH_NODE_H__

#include <cdk/ast/expression_node.h>
#include <cdk/ast/basic_node.h>

namespace til {

  class with_node: public cdk::basic_node {
    cdk::expression_node *_function, *_vector, *_low, *_high;

  public:
    with_node(int lineno, cdk::expression_node *function, cdk::expression_node *vector, 
              cdk::expression_node *low, cdk::expression_node *high) :
        basic_node(lineno), _function(function), _vector(vector), _low(low), _high(high) {
    }

  public:
    cdk::expression_node* function() {
      return _function;
    }
    cdk::expression_node* vector() {
      return _vector;
    }
    cdk::expression_node* low() {
      return _low;
    }
    cdk::expression_node* high() {
      return _high;
    }

  public:
    void accept(basic_ast_visitor *sp, int level) {
      sp->do_with_node(this, level);
    }

  };

} // til

#endif
