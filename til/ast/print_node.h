#ifndef __TIL_AST_PRINT_NODE_H__
#define __TIL_AST_PRINT_NODE_H__

#include <cdk/ast/sequence_node.h>

namespace til {

  /**
   * Class for describing print nodes.
   */
  class print_node : public cdk::basic_node {
    cdk::sequence_node *_expressions;
    bool _newline = false;

  public:
    print_node(int lineno, cdk::sequence_node *expressions, bool newline = false) :
        cdk::basic_node(lineno), _expressions(expressions), _newline(newline) {
    }

    cdk::sequence_node *expressions() { 
      return _expressions; 
    }
    bool newline() {
      return _newline;
    }

    void accept(basic_ast_visitor *sp, int level) { sp->do_print_node(this, level); }

  };

} // til

#endif
