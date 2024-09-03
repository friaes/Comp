#ifndef __TIL_AST_UNLESS_NODE_H__
#define __TIL_AST_UNLESS_NODE_H__

#include <cdk/ast/expression_node.h>
#include <cdk/ast/basic_node.h>

namespace til {

  class unless_node: public cdk::basic_node {
    cdk::expression_node *_condition, *_vector, *_count, *_function;

  public:
    unless_node(int lineno, cdk::expression_node *condition, cdk::expression_node *vector, 
              cdk::expression_node *count, cdk::expression_node *function) :
        basic_node(lineno), _condition(condition), _vector(vector), _count(count), _function(function) {
    }

  public:
    cdk::expression_node* condition() {
      return _condition;
    }
    cdk::expression_node* vector() {
      return _vector;
    }
    cdk::expression_node* count() {
      return _count;
    }
    cdk::expression_node* function() {
      return _function;
    }

  public:
    void accept(basic_ast_visitor *sp, int level) {
      sp->do_unless_node(this, level);
    }

  };

} // til

#endif
