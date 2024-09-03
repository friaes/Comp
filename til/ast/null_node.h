#ifndef __TIL_AST_NULLPTR_H__
#define __TIL_AST_NULLPTR_H__

#include <cdk/ast/expression_node.h>

namespace til {

  class null_node: public cdk::expression_node {
  public:
    null_node(int lineno) :
        cdk::expression_node(lineno) {
    }

  public:
    void accept(basic_ast_visitor *sp, int level) {
      sp->do_null_node(this, level);
    }

  };

} //til

#endif
