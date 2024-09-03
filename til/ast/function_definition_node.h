#ifndef __TIL_AST_FUNCTION_DEFINITION_H__
#define __TIL_AST_FUNCTION_DEFINITION_H__

#include <vector>
#include <cdk/ast/expression_node.h>
#include <cdk/ast/sequence_node.h>
#include <cdk/ast/typed_node.h>
#include <cdk/types/basic_type.h>
#include <cdk/types/functional_type.h>
#include <cdk/types/primitive_type.h>
#include "block_node.h"

namespace til {

  /**
   * Class for describing function definitions.
   */
  class function_definition_node: public cdk::expression_node {
    cdk::sequence_node *_arguments;
    til::block_node *_block;
    bool _is_main;

  public:
    function_definition_node(int lineno, std::shared_ptr<cdk::basic_type> fType, 
                             cdk::sequence_node *arguments, 
                             til::block_node *block, bool is_main = false) :
        cdk::expression_node(lineno), _arguments(arguments), _block(block), _is_main(is_main) {   
      std::vector<std::shared_ptr<cdk::basic_type>> argTypes;

      for (size_t i = 0; i < arguments->size(); i++)
        argTypes.push_back(dynamic_cast<cdk::typed_node*>(arguments->node(i))->type());
      
      type(cdk::functional_type::create(argTypes, fType));
    }

    /** Main function constructor. */
    function_definition_node(int lineno, til::block_node *block) :
        cdk::expression_node(lineno), _arguments(new cdk::sequence_node(lineno)), _block(block), _is_main(true) {
      type(cdk::functional_type::create(cdk::primitive_type::create(4, cdk::TYPE_INT)));
    }

  public:
    cdk::sequence_node *arguments() {
      return _arguments;
    }
    
    til::block_node *block() {
      return _block;
    }
    
    bool is_main() {
      return _is_main;
    }

    void accept(basic_ast_visitor *sp, int level) {
      sp->do_function_definition_node(this, level);
    }

  };

} // til

#endif
