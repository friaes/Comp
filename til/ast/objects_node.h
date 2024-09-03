#ifndef __TIL_AST_OBJECTS_NODE_H__
#define __TIL_AST_OBJECTS_NODE_H__

#include <cdk/ast/unary_operation_node.h>

namespace til {

  class objects_node: public cdk::unary_operation_node {
  public:
    objects_node(int lineno, cdk::expression_node *argument) :
        cdk::unary_operation_node(lineno, argument) {
    }

  public:
    void accept(basic_ast_visitor *sp, int level) {
      sp->do_objects_node(this, level);
    }

  };

} // til

#endif
