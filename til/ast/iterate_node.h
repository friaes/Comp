#ifndef __TIL_AST_ITERATE_H__
#define __TIL_AST_ITERATE_H__

#include <cdk/ast/basic_node.h>
#include <cdk/ast/expression_node.h>

namespace til {

  class iterate_node: public cdk::basic_node {
    cdk::expression_node *_vector, *_count, *_function, *_condition;

  public:
    iterate_node(int lineno, cdk::expression_node *vector, cdk::expression_node *count
                , cdk::expression_node *function, cdk::expression_node *condition) :
        cdk::basic_node(lineno), _vector(vector), _count(count), _function(function), _condition(condition) {
    }

  public:
    cdk::expression_node *vector() {
      return _vector;
    }

    cdk::expression_node *count() {
      return _count;
    }

    cdk::expression_node *function() {
      return _function;
    }

    cdk::expression_node *condition() {
      return _condition;
    }

  public:
    void accept(basic_ast_visitor *sp, int level) {
      sp->do_iterate_node(this, level);
    }

  };

} // til

#endif