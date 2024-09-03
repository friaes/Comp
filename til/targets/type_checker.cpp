#include <string>
#include "targets/type_checker.h"
#include ".auto/all_nodes.h"  // automatically generated
#include <cdk/types/primitive_type.h>
#include "til_parser.tab.h"


#define ASSERT_UNSPEC { if (node->type() != nullptr && !node->is_typed(cdk::TYPE_UNSPEC)) return; }

bool til::type_checker::type_comparison(std::shared_ptr<cdk::basic_type> left,
      std::shared_ptr<cdk::basic_type> right, bool lax) {
  if (left->name() == cdk::TYPE_UNSPEC || right->name() == cdk::TYPE_UNSPEC) {
    return false;
  } else if (left->name() == cdk::TYPE_FUNCTIONAL) {
    if (right->name() != cdk::TYPE_FUNCTIONAL) {
      return false;
    }

    auto left_func = cdk::functional_type::cast(left);
    auto right_func = cdk::functional_type::cast(right);

    if (left_func->input_length() != right_func->input_length()
          || left_func->output_length() != right_func->output_length()) {
      return false;
    }

    for (size_t i = 0; i < left_func->input_length(); i++) {
      if (!type_comparison(right_func->input(i), left_func->input(i), lax)) {
        return false;
      }
    }

    for (size_t i = 0; i < left_func->output_length(); i++) {
      if (!type_comparison(left_func->output(i), right_func->output(i), lax)) {
        return false;
      }
    }

    return true;
  } else if (right->name() == cdk::TYPE_FUNCTIONAL) {
    return false;
  } else if (left->name() == cdk::TYPE_POINTER) {
    if (right->name() != cdk::TYPE_POINTER) {
      return false;
    }

    return type_comparison(cdk::reference_type::cast(left)->referenced(),
        cdk::reference_type::cast(right)->referenced(), false);
  } else if (right->name() == cdk::TYPE_POINTER) {
      return false;
  } else if (lax && left->name() == cdk::TYPE_DOUBLE) {
    return right->name() == cdk::TYPE_DOUBLE || right->name() == cdk::TYPE_INT;
  } else {
    return left == right;
  }
}

//---------------------------------------------------------------------------

void til::type_checker::do_sequence_node(cdk::sequence_node *const node, int lvl) {
  for (size_t i = 0; i < node->size(); i++) {
    node->node(i)->accept(this, lvl);
  }
}

//---------------------------------------------------------------------------

void til::type_checker::do_nil_node(cdk::nil_node *const node, int lvl) {
  // EMPTY
}
void til::type_checker::do_data_node(cdk::data_node *const node, int lvl) {
  // EMPTY
}
void til::type_checker::do_next_node(til::next_node * const node, int lvl) {
  // Empty
}
void til::type_checker::do_stop_node(til::stop_node * const node, int lvl) {
  // Empty
}
void til::type_checker::do_block_node(til::block_node * const node, int lvl) {
  // Empty
}

//---------------------------------------------------------------------------

void til::type_checker::do_integer_node(cdk::integer_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}

void til::type_checker::do_string_node(cdk::string_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->type(cdk::primitive_type::create(4, cdk::TYPE_STRING));
}

void til::type_checker::do_double_node(cdk::double_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
}

//---------------------------------------------------------------------------

void til::type_checker::processUnaryExpression(cdk::unary_operation_node *const node, int lvl, bool acceptDoubles) {
  ASSERT_UNSPEC;
  node->argument()->accept(this, lvl + 2);
  if (node->argument()->is_typed(cdk::TYPE_UNSPEC)) {
    node->argument()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  } else if (!node->argument()->is_typed(cdk::TYPE_INT)
        && !(acceptDoubles && node->argument()->is_typed(cdk::TYPE_DOUBLE))) {
    throw std::string("wrong type in argument of unary expression");
  }
  node->type(node->argument()->type());
}

void til::type_checker::do_unary_minus_node(cdk::unary_minus_node *const node, int lvl) {
  processUnaryExpression(node, lvl, true);
}

void til::type_checker::do_unary_plus_node(cdk::unary_plus_node *const node, int lvl) {
  processUnaryExpression(node, lvl, true);
}

void til::type_checker::do_not_node(cdk::not_node *const node, int lvl) {
  processUnaryExpression(node, lvl, false);
}

//---------------------------------------------------------------------------

void til::type_checker::processBinaryArithmeticExpression(cdk::binary_operation_node *const node, int lvl) {
    ASSERT_UNSPEC;
    node->left()->accept(this, lvl + 2);

    if (node->left()->is_typed(cdk::TYPE_INT) || node->left()->is_typed(cdk::TYPE_UNSPEC)) {
        processRightNodeForInt(node, lvl);
        setUnspecType(node->left(), node->type());
    } else if (node->left()->is_typed(cdk::TYPE_DOUBLE)) {
        processRightNodeForDouble(node, lvl);
    } else if (node->left()->is_typed(cdk::TYPE_POINTER)) {
        processRightNodeForPointer(node, lvl);
    } else {
        throw std::string("wrong type in left argument of arithmetic binary expression");
    }
}

void til::type_checker::processRightNodeForInt(cdk::binary_operation_node *const node, int lvl) {
    node->right()->accept(this, lvl + 2);

    if (node->right()->is_typed(cdk::TYPE_INT) || node->right()->is_typed(cdk::TYPE_DOUBLE)) {
        node->type(node->right()->type());
    } else if (node->right()->is_typed(cdk::TYPE_UNSPEC)) {
        node->right()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
        node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
    } else if (node->right()->is_typed(cdk::TYPE_POINTER)) {
        node->type(node->right()->type());
        setUnspecType(node->left(), cdk::primitive_type::create(4, cdk::TYPE_INT));
    } else {
        throw std::string("wrong type in right argument of arithmetic binary expression");
    }
}

void til::type_checker::processRightNodeForDouble(cdk::binary_operation_node *const node, int lvl) {
    node->right()->accept(this, lvl + 2);

    if (node->right()->is_typed(cdk::TYPE_INT) || node->right()->is_typed(cdk::TYPE_DOUBLE)) {
        node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
    } else if (node->right()->is_typed(cdk::TYPE_UNSPEC)) {
        node->right()->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
        node->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
    } else {
        throw std::string("wrong type in right argument of arithmetic binary expression");
    }
}

void til::type_checker::processRightNodeForPointer(cdk::binary_operation_node *const node, int lvl) {
    node->right()->accept(this, lvl + 2);

    if (node->right()->is_typed(cdk::TYPE_INT)) {
        node->type(node->left()->type());
    } else if (node->right()->is_typed(cdk::TYPE_UNSPEC)) {
        node->right()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
        node->type(node->left()->type());
    } else if (type_comparison(node->left()->type(), node->right()->type(), false)) {
        node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
    } else {
        throw std::string("wrong type in right argument of arithmetic binary expression");
    }
}

void til::type_checker::setUnspecType(cdk::typed_node *node, std::shared_ptr<cdk::basic_type> type) {
    if (node->is_typed(cdk::TYPE_UNSPEC)) {
        node->type(type);
    }
}

void til::type_checker::do_add_node(cdk::add_node *const node, int lvl) {
  processBinaryArithmeticExpression(node, lvl);
}
void til::type_checker::do_sub_node(cdk::sub_node *const node, int lvl) {
  processBinaryArithmeticExpression(node, lvl);
}
void til::type_checker::do_mul_node(cdk::mul_node *const node, int lvl) {
  processBinaryArithmeticExpression(node, lvl);
}
void til::type_checker::do_div_node(cdk::div_node *const node, int lvl) {
  processBinaryArithmeticExpression(node, lvl);
}
void til::type_checker::do_mod_node(cdk::mod_node *const node, int lvl) {
  processBinaryArithmeticExpression(node, lvl);
}

void til::type_checker::processBinaryPredicateExpression(cdk::binary_operation_node *const node, int lvl, bool acceptDoubles, bool acceptPointers) {
  ASSERT_UNSPEC;

  node->left()->accept(this, lvl + 2);

  if (node->left()->is_typed(cdk::TYPE_INT)) {
    node->right()->accept(this, lvl + 2);

    if (node->right()->is_typed(cdk::TYPE_UNSPEC)) {
      node->right()->type(node->left()->type());
    } else if (!node->right()->is_typed(cdk::TYPE_INT)
          && !(acceptDoubles && node->right()->is_typed(cdk::TYPE_DOUBLE))
          && !(acceptPointers && node->right()->is_typed(cdk::TYPE_POINTER))) {
      throw std::string("wrong type in right argument of arithmetic binary expression");
    }
  } else if (acceptDoubles && node->left()->is_typed(cdk::TYPE_DOUBLE)) {
    node->right()->accept(this, lvl + 2);

    if (node->right()->is_typed(cdk::TYPE_UNSPEC)) {
      node->right()->type(node->left()->type());
    } else if (!node->right()->is_typed(cdk::TYPE_INT) && !node->right()->is_typed(cdk::TYPE_DOUBLE)) {
      throw std::string("wrong type in right argument of arithmetic binary expression");
    }
  } else if (acceptPointers && node->left()->is_typed(cdk::TYPE_POINTER)) {
    node->right()->accept(this, lvl + 2);

    if (node->right()->is_typed(cdk::TYPE_UNSPEC)) {
      node->right()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
    } else if (!node->right()->is_typed(cdk::TYPE_INT) && !node->right()->is_typed(cdk::TYPE_POINTER)) {
      throw std::string("wrong type in right argument of arithmetic binary expression");
    }
  } else if (node->left()->is_typed(cdk::TYPE_UNSPEC)) {
    node->right()->accept(this, lvl + 2);

    if (node->right()->is_typed(cdk::TYPE_UNSPEC)) {
      node->left()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
      node->right()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
    } else if (node->right()->is_typed(cdk::TYPE_POINTER)) {
      node->left()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
    } else if (node->right()->is_typed(cdk::TYPE_INT) || (acceptDoubles && node->right()->is_typed(cdk::TYPE_DOUBLE))) {
      node->left()->type(node->right()->type());
    } else {
      throw std::string("wrong type in right argument of arithmetic binary expression");
    }
  } else {
    throw std::string("wrong type in left argument of arithmetic binary expression");
  }

  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}

void til::type_checker::do_lt_node(cdk::lt_node *const node, int lvl) {
  processBinaryPredicateExpression(node, lvl, true, false);
}
void til::type_checker::do_le_node(cdk::le_node *const node, int lvl) {
  processBinaryPredicateExpression(node, lvl, true, false);
}
void til::type_checker::do_ge_node(cdk::ge_node *const node, int lvl) {
  processBinaryPredicateExpression(node, lvl, true, false);
}
void til::type_checker::do_gt_node(cdk::gt_node *const node, int lvl) {
  processBinaryPredicateExpression(node, lvl, true, false);
}
void til::type_checker::do_ne_node(cdk::ne_node *const node, int lvl) {
  processBinaryPredicateExpression(node, lvl, true, false);
}
void til::type_checker::do_eq_node(cdk::eq_node *const node, int lvl) {
  processBinaryPredicateExpression(node, lvl, true, false);
}
void til::type_checker::do_and_node(cdk::and_node *const node, int lvl) {
  processBinaryPredicateExpression(node, lvl, false, false);
}
void til::type_checker::do_or_node(cdk::or_node *const node, int lvl) {
  processBinaryPredicateExpression(node, lvl, false, false);
}

//---------------------------------------------------------------------------

void til::type_checker::do_variable_node(cdk::variable_node *const node, int lvl) {
  ASSERT_UNSPEC;
  const std::string &id = node->name();
  std::shared_ptr<til::symbol> symbol = _symtab.find(id);
  if (symbol != nullptr) {
    node->type(symbol->type());
  } else {
    throw id;
  }
}

void til::type_checker::do_rvalue_node(cdk::rvalue_node *const node, int lvl) {
  ASSERT_UNSPEC;
  try {
    node->lvalue()->accept(this, lvl);
    node->type(node->lvalue()->type());
  } catch (const std::string &id) {
    throw std::string("undeclared variable '" + id + "'");
  }
}

void til::type_checker::do_assignment_node(cdk::assignment_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->lvalue()->accept(this, lvl);
  node->rvalue()->accept(this, lvl);

  if (node->rvalue()->is_typed(cdk::TYPE_UNSPEC)) {
    node->rvalue()->type(node->lvalue()->type());
  } else if (node->rvalue()->is_typed(cdk::TYPE_POINTER) && node->lvalue()->is_typed(cdk::TYPE_POINTER)) {
    auto lref = cdk::reference_type::cast(node->lvalue()->type());
    auto rref = cdk::reference_type::cast(node->rvalue()->type());
    if (rref->referenced()->name() == cdk::TYPE_UNSPEC
          || rref->referenced()->name() == cdk::TYPE_VOID
          || lref->referenced()->name() == cdk::TYPE_VOID) {
      node->rvalue()->type(node->lvalue()->type());
    }
  }

  if (!type_comparison(node->lvalue()->type(), node->rvalue()->type(), true)) {
    throw std::string("wrong type in right argument of assignment expression");
  }

  node->type(node->lvalue()->type());
}


//---------------------------------------------------------------------------

void til::type_checker::do_evaluation_node(til::evaluation_node *const node, int lvl) {
  node->argument()->accept(this, lvl);

  if (node->argument()->is_typed(cdk::TYPE_UNSPEC)) {
    node->argument()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  } else if (node->argument()->is_typed(cdk::TYPE_POINTER)) {
    auto ref = cdk::reference_type::cast(node->argument()->type());

    if (ref != nullptr && ref->referenced()->name() == cdk::TYPE_UNSPEC) {
      node->argument()->type(cdk::reference_type::create(4, cdk::primitive_type::create(4, cdk::TYPE_INT)));
    }
  }
}

void til::type_checker::do_print_node(til::print_node *const node, int lvl) {
  for (size_t i = 0; i < node->expressions()->size(); i++) {
    auto child = dynamic_cast<cdk::expression_node*>(node->expressions()->node(i));

    child->accept(this, lvl);

    if (child->is_typed(cdk::TYPE_UNSPEC)) {
      child->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
    } else if (!child->is_typed(cdk::TYPE_INT) && !child->is_typed(cdk::TYPE_DOUBLE)
          && !child->is_typed(cdk::TYPE_STRING)) {
      throw std::string("wrong type for argument " + std::to_string(i + 1) + " of print instruction");
    }
  }
}

//---------------------------------------------------------------------------

void til::type_checker::do_read_node(til::read_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->type(cdk::primitive_type::create(0, cdk::TYPE_UNSPEC));
}

void til::type_checker::do_loop_node(til::loop_node * const node, int lvl) {
  node->condition()->accept(this, lvl + 4);
  if (node->condition()->is_typed(cdk::TYPE_UNSPEC)) {
    node->condition()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  } else if (!node->condition()->is_typed(cdk::TYPE_INT)) {
    throw std::string("wrong type in condition of loop instruction");
  }
}

//---------------------------------------------------------------------------

void til::type_checker::do_if_node(til::if_node *const node, int lvl) {
  node->condition()->accept(this, lvl + 4);
  if (node->condition()->is_typed(cdk::TYPE_UNSPEC)) {
    node->condition()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  } else if (!node->condition()->is_typed(cdk::TYPE_INT)) {
    throw std::string("wrong type in condition of conditional instruction");
  }
}

void til::type_checker::do_if_else_node(til::if_else_node *const node, int lvl) {
  node->condition()->accept(this, lvl + 4);
  if (node->condition()->is_typed(cdk::TYPE_UNSPEC)) {
    node->condition()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  } else if (!node->condition()->is_typed(cdk::TYPE_INT)) {
    throw std::string("wrong type in condition of conditional instruction");
  }
}

//---------------------------------------------------------------------------

void til::type_checker::do_address_of_node(til::address_of_node *const node, int lvl) {
  ASSERT_UNSPEC;

  node->lvalue()->accept(this, lvl + 2);
  if (node->lvalue()->is_typed(cdk::TYPE_POINTER)) {
    auto ref = cdk::reference_type::cast(node->lvalue()->type());
    if (ref->referenced()->name() == cdk::TYPE_VOID) {
      // void!! == void!
      node->type(node->lvalue()->type());
      return;
    }
  }
  node->type(cdk::reference_type::create(4, node->lvalue()->type()));
}

void til::type_checker::do_index_node(til::index_node * const node, int lvl) {
  ASSERT_UNSPEC;

  node->base()->accept(this, lvl + 2);
  if (!node->base()->is_typed(cdk::TYPE_POINTER)) {
    throw std::string("wrong type in pointer index's base (expected pointer)");
  }

  node->index()->accept(this, lvl + 2);
  if (node->index()->is_typed(cdk::TYPE_UNSPEC)) {
    node->index()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  } else if (!node->index()->is_typed(cdk::TYPE_INT)) {
    throw std::string("wrong type in pointer index's index (expected integer)");
  }

  auto basetype = cdk::reference_type::cast(node->base()->type());
  if (basetype->referenced()->name() == cdk::TYPE_UNSPEC) {
    basetype = cdk::reference_type::create(4, cdk::primitive_type::create(4, cdk::TYPE_INT));
    node->base()->type(basetype);
  }

  node->type(basetype->referenced());
}

void til::type_checker::do_objects_node(til::objects_node *const node, int lvl) {
  ASSERT_UNSPEC;
  node->argument()->accept(this, lvl + 2);

  if (node->argument()->is_typed(cdk::TYPE_UNSPEC)) {
    node->argument()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  } else if (!node->argument()->is_typed(cdk::TYPE_INT)) {
    throw std::string("wrong type in argument of unary expression");
  }
  node->type(cdk::reference_type::create(4, cdk::primitive_type::create(0, cdk::TYPE_UNSPEC)));
}

//---------------------------------------------------------------------------

void til::type_checker::do_function_call_node(til::function_call_node * const node, int lvl) {
  ASSERT_UNSPEC;

  std::shared_ptr<cdk::functional_type> function_type;
  if (!node->func()) { 
    auto symbol = _symtab.find("@", 1);
    if (!symbol) {
      throw std::string("Recursive call detected outside of a function");
    } else if (symbol->is_main()) {
      throw std::string("Recursive call found within a main block");
    }

    function_type = cdk::functional_type::cast(symbol->type());
  } else {
    node->func()->accept(this, lvl);

    if (!node->func()->is_typed(cdk::TYPE_FUNCTIONAL)) {
      throw std::string("Invalid type in function call");
    }

    function_type = cdk::functional_type::cast(node->func()->type());
  }

  const size_t num_arguments = node->arguments()->size();
  if (function_type->input()->length() != num_arguments) {
    throw std::string("Incorrect number of arguments for function call");
  }

  for (size_t i = 0; i < num_arguments; ++i) {
    auto argument = dynamic_cast<cdk::expression_node*>(node->arguments()->node(i));
    argument->accept(this, lvl);
    auto parameter_type = function_type->input(i);
    if (argument->is_typed(cdk::TYPE_UNSPEC)) {
      if (parameter_type->name() == cdk::TYPE_DOUBLE) {
        argument->type(cdk::primitive_type::create(8, cdk::TYPE_DOUBLE));
      } else {
        argument->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
      }
    } else if (argument->is_typed(cdk::TYPE_POINTER) && parameter_type->name() == cdk::TYPE_POINTER) {
      auto param_ref_type = cdk::reference_type::cast(parameter_type);
      auto arg_ref_type = cdk::reference_type::cast(argument->type());
      if (arg_ref_type->referenced()->name() == cdk::TYPE_UNSPEC
          || arg_ref_type->referenced()->name() == cdk::TYPE_VOID
          || param_ref_type->referenced()->name() == cdk::TYPE_VOID) {
        argument->type(parameter_type);
      }
    }
    if (!type_comparison(parameter_type, argument->type(), true)) {
      throw std::string("Incorrect type for argument " + std::to_string(i + 1) + " in function call");
    }
  }

  node->type(function_type->output(0));
}

void til::type_checker::do_function_definition_node(til::function_definition_node * const node, int lvl) {
  auto function = til::make_symbol("@", node->type());
  function->is_main(node->is_main());

  if (!_symtab.insert(function->name(), function)) {
    _symtab.replace(function->name(), function);
  }
}

void til::type_checker::do_null_node(til::null_node * const node, int lvl) {
  ASSERT_UNSPEC;

  node->type(cdk::reference_type::create(4, cdk::primitive_type::create(0, cdk::TYPE_UNSPEC)));
}

void til::type_checker::do_return_node(til::return_node * const node, int lvl) {
  auto function_symbol = _symtab.find("@", 1);
  if (!function_symbol) {
    throw std::string("Return statement found outside of a function");
  }

  std::shared_ptr<cdk::functional_type> function_type = cdk::functional_type::cast(function_symbol->type());

  auto return_type = function_type->output(0);
  auto return_type_name = return_type->name();

  if (node->retval() == nullptr) {
    if (return_type_name != cdk::TYPE_VOID) {
      throw std::string("No return value specified for a non-void function");
    }
    return;
  }

  if (return_type_name == cdk::TYPE_VOID) {
    throw std::string("Return value specified for a void function");
  }

  node->retval()->accept(this, lvl + 2);

  if (!type_comparison(return_type, node->retval()->type(), true)) {
    throw std::string("Incorrect type for return expression");
  }
}

void til::type_checker::do_sizeof_node(til::sizeof_node * const node, int lvl) {
  ASSERT_UNSPEC;
  node->expression()->accept(this, lvl + 2);
  if (node->expression()->is_typed(cdk::TYPE_UNSPEC)) {
    node->expression()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  }
  node->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
}


void til::type_checker::do_declaration_node(til::declaration_node * const node, int lvl) {
  if (!node->type()) {
    node->initializer()->accept(this, lvl + 2);

    if (node->initializer()->is_typed(cdk::TYPE_UNSPEC)) {
      node->initializer()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
    } else if (node->initializer()->is_typed(cdk::TYPE_POINTER)) {
      auto ref = cdk::reference_type::cast(node->initializer()->type());
      if (ref->referenced()->name() == cdk::TYPE_UNSPEC) {
        node->initializer()->type(cdk::reference_type::create(4, cdk::primitive_type::create(4, cdk::TYPE_INT)));
      }
    } else if (node->initializer()->is_typed(cdk::TYPE_VOID)) {
      throw std::string("Cannot declare a variable of type void");
    }

    node->type(node->initializer()->type());
  } else {
    if (node->initializer()) {
      node->initializer()->accept(this, lvl + 2);

      if (node->initializer()->is_typed(cdk::TYPE_UNSPEC)) {
        if (node->is_typed(cdk::TYPE_DOUBLE)) {
          node->initializer()->type(node->type());
        } else {
          node->initializer()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
        }
      } else if (node->initializer()->is_typed(cdk::TYPE_POINTER) && node->is_typed(cdk::TYPE_POINTER)) {
        auto node_ref = cdk::reference_type::cast(node->type());
        auto init_ref = cdk::reference_type::cast(node->initializer()->type());
        if (init_ref->referenced()->name() == cdk::TYPE_UNSPEC ||
            init_ref->referenced()->name() == cdk::TYPE_VOID ||
            node_ref->referenced()->name() == cdk::TYPE_VOID) {
          node->initializer()->type(node->type());
        }
      }

      if (!type_comparison(node->type(), node->initializer()->type(), true)) {
        throw std::string("Wrong type in initializer for variable '" + node->identifier() + "'");
      }
    }
  }

  if (node->qualifier() == tEXTERNAL && !node->is_typed(cdk::TYPE_FUNCTIONAL)) {
    throw std::string("Foreign declaration of non-function '" + node->identifier() + "'");
  }

  auto symbol = make_symbol(node->identifier(), node->type(), node->qualifier());
  if (_symtab.insert(node->identifier(), symbol)) {
    _parent->set_new_symbol(symbol);
  } else {
    auto previous_symbol = _symtab.find(node->identifier());

    if (previous_symbol && previous_symbol->qualifier() == tFORWARD) {
      if (type_comparison(previous_symbol->type(), symbol->type(), false)) {
        _symtab.replace(node->identifier(), symbol);
        _parent->set_new_symbol(symbol);
        return;
      }
    }
    throw std::string("Redeclaration of variable '" + node->identifier() + "'");
  }
}

void til::type_checker::do_with_node(til::with_node *const node, int lvl) {
  
  node->vector()->accept(this, lvl);
  if (!node->vector()->is_typed(cdk::TYPE_POINTER)) {
    throw std::string("wrong type for vector in with instruction");
  }

  node->low()->accept(this, lvl);
  if (node->low()->is_typed(cdk::TYPE_UNSPEC)) {
    node->low()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  } else if (!node->low()->is_typed(cdk::TYPE_INT)) {
    throw std::string("wrong type for low in with instruction");
  }

  node->high()->accept(this, lvl);
  if (node->high()->is_typed(cdk::TYPE_UNSPEC)) {
    node->high()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  } else if (!node->high()->is_typed(cdk::TYPE_INT)) {
    throw std::string("wrong type for high in with instruction");
  }

  node->function()->accept(this, lvl);
  if (node->function()->is_typed(cdk::TYPE_FUNCTIONAL)) {
    auto functype = cdk::functional_type::cast(node->function()->type());
    auto vecref = cdk::reference_type::cast(node->vector()->type());

    if (functype->input_length() == 1 && functype->input(0) == vecref->referenced()) {
      return;
    }
  }

  throw std::string("wrong type for function in with instruction");

  // remaining type-checking completed by sub-nodes
}

void til::type_checker::do_unless_node(til::unless_node *const node, int lvl) {
  
  node->condition()->accept(this, lvl);
  if (node->condition()->is_typed(cdk::TYPE_UNSPEC)) {
    node->condition()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  } else if (!node->condition()->is_typed(cdk::TYPE_INT)) {
    throw std::string("wrong type for condition in unless instruction");
  }

  node->vector()->accept(this, lvl);
  if (!node->vector()->is_typed(cdk::TYPE_POINTER)) {
    throw std::string("wrong type for vector in unless instruction");
  }

  node->count()->accept(this, lvl);
  if (node->count()->is_typed(cdk::TYPE_UNSPEC)) {
    node->count()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  } else if (!node->count()->is_typed(cdk::TYPE_INT)) {
    throw std::string("wrong type for count in unless instruction");
  }

  node->function()->accept(this, lvl);
  if (node->function()->is_typed(cdk::TYPE_FUNCTIONAL)) {
    auto functype = cdk::functional_type::cast(node->function()->type());
    auto vecref = cdk::reference_type::cast(node->vector()->type());

    if (functype->input_length() == 1 && functype->input(0) == vecref->referenced()) {
      return;
    }
  }

  throw std::string("wrong type for function in iterate instruction");

  // remaining type-checking completed by sub-nodes
}

void til::type_checker::do_sweep_node(til::sweep_node *const node, int lvl) {
  
  node->vector()->accept(this, lvl);
  if (!node->vector()->is_typed(cdk::TYPE_POINTER)) {
    throw std::string("wrong type for vector in with instruction");
  }

  node->low()->accept(this, lvl);
  if (node->low()->is_typed(cdk::TYPE_UNSPEC)) {
    node->low()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  } else if (!node->low()->is_typed(cdk::TYPE_INT)) {
    throw std::string("wrong type for low in with instruction");
  }

  node->high()->accept(this, lvl);
  if (node->high()->is_typed(cdk::TYPE_UNSPEC)) {
    node->high()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  } else if (!node->high()->is_typed(cdk::TYPE_INT)) {
    throw std::string("wrong type for high in with instruction");
  }

  node->condition()->accept(this, lvl);
  if (node->condition()->is_typed(cdk::TYPE_UNSPEC)) {
    node->condition()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  } else if (!node->condition()->is_typed(cdk::TYPE_INT)) {
    throw std::string("wrong type for high in with instruction");
  }

  node->function()->accept(this, lvl);
  if (node->function()->is_typed(cdk::TYPE_FUNCTIONAL)) {
    auto functype = cdk::functional_type::cast(node->function()->type());
    auto vecref = cdk::reference_type::cast(node->vector()->type());

    if (functype->input_length() == 1 && functype->input(0) == vecref->referenced()) {
      return;
    }
  }

  throw std::string("wrong type for function in with instruction");

  // remaining type-checking completed by sub-nodes
}

void til::type_checker::do_iterate_node(til::iterate_node *const node, int lvl) {

  node->vector()->accept(this, lvl);
  if (!node->vector()->is_typed(cdk::TYPE_POINTER)) {
    throw std::string("wrong type for vector in iterate instruction");
  }

  node->count()->accept(this, lvl);
  if (node->count()->is_typed(cdk::TYPE_UNSPEC)) {
    node->count()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  } else if (!node->count()->is_typed(cdk::TYPE_INT)) {
    throw std::string("wrong type for count in iterate instruction");
  }

  node->condition()->accept(this, lvl);
  if (node->condition()->is_typed(cdk::TYPE_UNSPEC)) {
    node->condition()->type(cdk::primitive_type::create(4, cdk::TYPE_INT));
  } else if (!node->condition()->is_typed(cdk::TYPE_INT)) {
    throw std::string("wrong type for count in iterate instruction");
  }

  node->function()->accept(this, lvl);
  if (node->function()->is_typed(cdk::TYPE_FUNCTIONAL)) {
    auto functype = cdk::functional_type::cast(node->function()->type());
    auto vecref = cdk::reference_type::cast(node->vector()->type());

    if (functype->input_length() == 1 && functype->input(0) == vecref->referenced()) {
      return;
    }
  }

  throw std::string("wrong type for function in iterate instruction");
}