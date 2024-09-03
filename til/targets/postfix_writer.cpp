#include <string>
#include <sstream>
#include "targets/type_checker.h"
#include "targets/postfix_writer.h"
#include "targets/frame_size_calculator.h"
#include ".auto/all_nodes.h"  // all_nodes.h is automatically generated
#include "til_parser.tab.h"

void til::postfix_writer::accept_covariant_node(std::shared_ptr<cdk::basic_type> const target_type, cdk::expression_node * const node, int lvl) {
  if (target_type->name() != cdk::TYPE_FUNCTIONAL || !node->is_typed(cdk::TYPE_FUNCTIONAL)) {
    node->accept(this, lvl);
    if (target_type->name() == cdk::TYPE_DOUBLE && node->is_typed(cdk::TYPE_INT)) {
      _pf.I2D();
    }
    return;
  }
  auto lfunc_type = cdk::functional_type::cast(target_type);
  auto rfunc_type = cdk::functional_type::cast(node->type());
  bool needsWrap = false;

  if (lfunc_type->output(0)->name() == cdk::TYPE_DOUBLE && rfunc_type->output(0)->name() == cdk::TYPE_INT) {
    needsWrap = true;
  } else {
    for (size_t i = 0; i < lfunc_type->input_length(); i++) {
      if (lfunc_type->input(i)->name() == cdk::TYPE_INT && rfunc_type->input(i)->name() == cdk::TYPE_DOUBLE) {
        needsWrap = true;
        break;
      }
    }
  }

  if (!needsWrap) {
    node->accept(this, lvl);
    return;
  }

  auto lineno = node->lineno();
  auto aux_global_decl_name = "_wrapper_target_" + std::to_string(_lbl++);
  auto aux_global_decl = new til::declaration_node(lineno, tPRIVATE, rfunc_type, aux_global_decl_name, nullptr);
  auto aux_global_var = new cdk::variable_node(lineno, aux_global_decl_name);

  _outside_func = true;
  aux_global_decl->accept(this, lvl);
  _outside_func = false;


  if (!_function_labels.empty() && !_outside_func) {
    _pf.TEXT(_function_labels.top());
  } else {
    _pf.DATA();
  }
  _pf.ALIGN();

  // we can't pass the target function as an initializer to the declaration, as it
  // may be a non-literal expression, so we need to assign it afterwards
  auto aux_global_assignment = new cdk::assignment_node(lineno, aux_global_var, node);
  aux_global_assignment->accept(this, lvl);

  auto aux_global_rvalue = new cdk::rvalue_node(lineno, aux_global_var);
  //! </aux global declaration and assignment>

  auto args = new cdk::sequence_node(lineno);
  auto call_args = new cdk::sequence_node(lineno);
  for (size_t i = 0; i < lfunc_type->input_length(); i++) {
    auto arg_name = "_arg" + std::to_string(i);

    auto arg_decl = new til::declaration_node(lineno, tPRIVATE, lfunc_type->input(i), arg_name, nullptr);
    args = new cdk::sequence_node(lineno, arg_decl, args);

    auto arg_rvalue = new cdk::rvalue_node(lineno, new cdk::variable_node(lineno, arg_name));
    call_args = new cdk::sequence_node(lineno, arg_rvalue, call_args);
  }

  auto function_call = new til::function_call_node(lineno, aux_global_rvalue, call_args);
  auto return_node = new til::return_node(lineno, function_call);
  auto block = new til::block_node(lineno, new cdk::sequence_node(lineno), new cdk::sequence_node(lineno, return_node));

  auto wrapping_function = new til::function_definition_node(lineno, lfunc_type->output(0), args, block);

  wrapping_function->accept(this, lvl);
}

//---------------------------------------------------------------------------

void til::postfix_writer::do_nil_node(cdk::nil_node * const node, int lvl) {
  // EMPTY
}
void til::postfix_writer::do_data_node(cdk::data_node * const node, int lvl) {
  // EMPTY
}

//---------------------------------------------------------------------------

void til::postfix_writer::do_sequence_node(cdk::sequence_node * const node, int lvl) {
  for (size_t i = 0; i < node->size(); i++) {
    node->node(i)->accept(this, lvl);
  }
}

//---------------------------------------------------------------------------

void til::postfix_writer::do_integer_node(cdk::integer_node * const node, int lvl) {
  if (!_function_labels.empty() && !_outside_func) {
    _pf.INT(node->value());
  } else {
    _pf.SINT(node->value());
  }
}

void til::postfix_writer::do_string_node(cdk::string_node * const node, int lvl) {
  int lbl1;
  /* generate the string literal */
  _pf.RODATA(); // strings are readonly DATA
  _pf.ALIGN(); // make sure the address is aligned
  _pf.LABEL(mklbl(lbl1 = ++_lbl)); // give the string a name
  _pf.SSTRING(node->value()); // output string characters
  if (!_function_labels.empty() && !_outside_func) {
    // local variable initializer
    _pf.TEXT(_function_labels.top());
    _pf.ADDR(mklbl(lbl1));
  } else {
    // global variable initializer
    _pf.DATA();
    _pf.SADDR(mklbl(lbl1));
  }
}

void til::postfix_writer::do_double_node(cdk::double_node * const node, int lvl) {
  if (!_function_labels.empty() && !_outside_func) {
    _pf.DOUBLE(node->value());
  } else {
    _pf.SDOUBLE(node->value());
  }
}

//---------------------------------------------------------------------------

void til::postfix_writer::do_unary_minus_node(cdk::unary_minus_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->argument()->accept(this, lvl); // determine the value
  if (node->is_typed(cdk::TYPE_DOUBLE)) {
    _pf.DNEG();
  } else {
    _pf.NEG();
  }
}

void til::postfix_writer::do_unary_plus_node(cdk::unary_plus_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->argument()->accept(this, lvl); // determine the value
}

void til::postfix_writer::do_not_node(cdk::not_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  node->argument()->accept(this, lvl);
  _pf.INT(0);
  _pf.EQ();
}

//---------------------------------------------------------------------------

void til::postfix_writer::do_add_node(cdk::add_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  node->left()->accept(this, lvl);
  if (node->is_typed(cdk::TYPE_DOUBLE) && node->left()->is_typed(cdk::TYPE_INT)) {
    _pf.I2D();
  } else if (node->is_typed(cdk::TYPE_POINTER) && node->left()->is_typed(cdk::TYPE_INT)) {
    auto ref = cdk::reference_type::cast(node->type());
    _pf.INT(std::max(static_cast<size_t>(1), ref->referenced()->size()));
    _pf.MUL();
  }

  node->right()->accept(this, lvl);
  if (node->is_typed(cdk::TYPE_DOUBLE) && node->right()->is_typed(cdk::TYPE_INT)) {
    _pf.I2D();
  } else if (node->is_typed(cdk::TYPE_POINTER) && node->right()->is_typed(cdk::TYPE_INT)) {
    auto ref = cdk::reference_type::cast(node->type());
    _pf.INT(std::max(static_cast<size_t>(1), ref->referenced()->size()));
    _pf.MUL();
  }

  if (node->is_typed(cdk::TYPE_DOUBLE)) {
    _pf.DADD();
  } else {
    _pf.ADD();
  }
}
void til::postfix_writer::do_sub_node(cdk::sub_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  node->left()->accept(this, lvl);
  if (node->is_typed(cdk::TYPE_DOUBLE) && node->left()->is_typed(cdk::TYPE_INT)) {
    _pf.I2D();
  } else if (node->is_typed(cdk::TYPE_POINTER) && node->left()->is_typed(cdk::TYPE_INT)) {
    auto ref = cdk::reference_type::cast(node->type());
    _pf.INT(std::max(static_cast<size_t>(1), ref->referenced()->size()));
    _pf.MUL();
  }

  node->right()->accept(this, lvl);
  if (node->is_typed(cdk::TYPE_DOUBLE) && node->right()->is_typed(cdk::TYPE_INT)) {
    _pf.I2D();
  } else if (node->is_typed(cdk::TYPE_POINTER) && node->right()->is_typed(cdk::TYPE_INT)) {
    auto ref = cdk::reference_type::cast(node->type());
    _pf.INT(std::max(static_cast<size_t>(1), ref->referenced()->size()));
    _pf.MUL();
  }

  if (node->is_typed(cdk::TYPE_DOUBLE)) {
    _pf.DSUB();
  } else {
    _pf.SUB();
  }

  if (node->left()->is_typed(cdk::TYPE_POINTER) && node->right()->is_typed(cdk::TYPE_POINTER)) {
    // the difference between two pointers must be divided by the size of what they're referencing
    auto lref = cdk::reference_type::cast(node->left()->type());
    _pf.INT(std::max(static_cast<size_t>(1), lref->referenced()->size()));
    _pf.DIV();
  }
}

//mudar nome
void til::postfix_writer::prepareIDBinaryExpression(cdk::binary_operation_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  node->left()->accept(this, lvl);
  if (node->left()->is_typed(cdk::TYPE_INT) && node->right()->is_typed(cdk::TYPE_DOUBLE)) {
    _pf.I2D();
  }

  node->right()->accept(this, lvl);
  if (node->left()->is_typed(cdk::TYPE_DOUBLE) && node->right()->is_typed(cdk::TYPE_INT)) {
    _pf.I2D();
  }
}

void til::postfix_writer::do_mul_node(cdk::mul_node * const node, int lvl) {
  prepareIDBinaryExpression(node, lvl);
  
  if (node->is_typed(cdk::TYPE_DOUBLE)) {
    _pf.DMUL();
  } else {
    _pf.MUL();
  }
}
void til::postfix_writer::do_div_node(cdk::div_node * const node, int lvl) {
  prepareIDBinaryExpression(node, lvl);

  if (node->is_typed(cdk::TYPE_DOUBLE)) {
    _pf.DDIV();
  } else {
    _pf.DIV();
  }
}

void til::postfix_writer::do_mod_node(cdk::mod_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->left()->accept(this, lvl);
  node->right()->accept(this, lvl);
  _pf.MOD();
}

void til::postfix_writer::prepareIDBinaryComparisonExpression(cdk::binary_operation_node * const node, int lvl) {
  prepareIDBinaryExpression(node, lvl);

  if (node->left()->is_typed(cdk::TYPE_DOUBLE) || node->right()->is_typed(cdk::TYPE_DOUBLE)) {
    _pf.DCMP();
    _pf.INT(0);
  }
}

void til::postfix_writer::do_lt_node(cdk::lt_node * const node, int lvl) {
  prepareIDBinaryComparisonExpression(node, lvl);
  _pf.LT();
}
void til::postfix_writer::do_le_node(cdk::le_node * const node, int lvl) {
  prepareIDBinaryComparisonExpression(node, lvl);
  _pf.LE();
}
void til::postfix_writer::do_ge_node(cdk::ge_node * const node, int lvl) {
  prepareIDBinaryComparisonExpression(node, lvl);
  _pf.GE();
}
void til::postfix_writer::do_gt_node(cdk::gt_node * const node, int lvl) {
  prepareIDBinaryComparisonExpression(node, lvl);
  _pf.GT();
}
void til::postfix_writer::do_ne_node(cdk::ne_node * const node, int lvl) {
  prepareIDBinaryComparisonExpression(node, lvl);
  _pf.NE();
}
void til::postfix_writer::do_eq_node(cdk::eq_node * const node, int lvl) {
  prepareIDBinaryComparisonExpression(node, lvl);
  _pf.EQ();
}

void til::postfix_writer::do_and_node(cdk::and_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  int lbl;
  node->left()->accept(this, lvl);
  _pf.DUP32();
  _pf.JZ(mklbl(lbl = ++_lbl)); // short circuit
  node->right()->accept(this, lvl);
  _pf.AND();
  _pf.ALIGN();
  _pf.LABEL(mklbl(lbl));
}
void til::postfix_writer::do_or_node(cdk::or_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  int lbl;
  node->left()->accept(this, lvl);
  _pf.DUP32();
  _pf.JNZ(mklbl(lbl = ++_lbl)); // short circuit
  node->right()->accept(this, lvl);
  _pf.OR();
  _pf.ALIGN();
  _pf.LABEL(mklbl(lbl));
}

//---------------------------------------------------------------------------

void til::postfix_writer::do_variable_node(cdk::variable_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  auto symbol = _symtab.find(node->name()); // type checker already ensured symbol exists

  if (symbol->qualifier() == tEXTERNAL) {
    _external_func_name = symbol->name();
  } else if (symbol->global()) {
    _pf.ADDR(node->name());
  } else {
    _pf.LOCAL(symbol->offset());
  }
}

void til::postfix_writer::do_rvalue_node(cdk::rvalue_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->lvalue()->accept(this, lvl);

  if (_external_func_name) 
    return; 
  
  if (node->is_typed(cdk::TYPE_DOUBLE)) {
    _pf.LDDOUBLE();
  } else {
    _pf.LDINT();
  }
}

void til::postfix_writer::do_assignment_node(cdk::assignment_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  accept_covariant_node(node->type(), node->rvalue(), lvl);
  if (node->is_typed(cdk::TYPE_DOUBLE)) {
    _pf.DUP64();
  } else {
    _pf.DUP32();
  }

  node->lvalue()->accept(this, lvl); // where to store the value
  if (node->is_typed(cdk::TYPE_DOUBLE)) {
    _pf.STDOUBLE(); // store the value at address
  } else {
    _pf.STINT(); // store the value at address
  }
}

//---------------------------------------------------------------------------

void til::postfix_writer::do_evaluation_node(til::evaluation_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  node->argument()->accept(this, lvl);

  if (node->argument()->type()->size() > 0) {
    _pf.TRASH(node->argument()->type()->size());
  }
}

void til::postfix_writer::do_print_node(til::print_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  
  for (size_t ix = 0; ix < node->expressions()->size(); ix++) {
    auto child = dynamic_cast<cdk::expression_node*>(node->expressions()->node(ix));

    child->accept(this, lvl);
    if (child->is_typed(cdk::TYPE_INT)) {
      _external_func_to_declare.insert("printi");
      _pf.CALL("printi");
      _pf.TRASH(4); // delete the printed value
    } else if (child->is_typed(cdk::TYPE_STRING)) {
      _external_func_to_declare.insert("prints");
      _pf.CALL("prints");
      _pf.TRASH(4); // delete the printed value's address
    } else if (child->is_typed(cdk::TYPE_DOUBLE)) {
      _external_func_to_declare.insert("printd");
      _pf.CALL("printd");
      _pf.TRASH(8); // delete the printed value
    }
  }

  if (node->newline()) {
    _external_func_to_declare.insert("println");
    _pf.CALL("println");
  }
  
}

//---------------------------------------------------------------------------

void til::postfix_writer::do_read_node(til::read_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  if(node->is_typed(cdk::TYPE_DOUBLE)) {
    _external_func_to_declare.insert("readd");
    _pf.CALL("readd");
    _pf.LDFVAL64();
  } else {
  _external_func_to_declare.insert("readi");
  _pf.CALL("readi");
  _pf.LDFVAL32();
  }
}

//---------------------------------------------------------------------------

void til::postfix_writer::do_loop_node(til::loop_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  int condlbl, endlbl;
  _pf.LABEL(mklbl(condlbl = ++_lbl));
  node->condition()->accept(this, lvl);
  _pf.JZ(mklbl(endlbl = ++_lbl));

  _cur_func_loop_labels->push_back(std::make_pair(mklbl(condlbl), mklbl(endlbl)));
  node->instruction()->accept(this, lvl + 2);
  _loop_ended = false; 
  _cur_func_loop_labels->pop_back();

  _pf.JMP(mklbl(condlbl));
  _pf.ALIGN();
  _pf.LABEL(mklbl(endlbl));
}

//---------------------------------------------------------------------------

void til::postfix_writer::do_if_node(til::if_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  int lbl1;
  node->condition()->accept(this, lvl);
  _pf.JZ(mklbl(lbl1 = ++_lbl));
  node->block()->accept(this, lvl + 2);
  _loop_ended = false;
  _pf.ALIGN();
  _pf.LABEL(mklbl(lbl1));
}

//---------------------------------------------------------------------------

void til::postfix_writer::do_if_else_node(til::if_else_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  int lbl1, lbl2;
  node->condition()->accept(this, lvl);
  _pf.JZ(mklbl(lbl1 = ++_lbl));
  node->thenblock()->accept(this, lvl + 2);
  _loop_ended = false; 
  _pf.JMP(mklbl(lbl2 = ++_lbl));
  _pf.ALIGN();
  _pf.LABEL(mklbl(lbl1));
  node->elseblock()->accept(this, lvl + 2);
  _loop_ended = false;
  _pf.ALIGN();
  _pf.LABEL(mklbl(lbl1 = lbl2));
}

//---------------------------------------------------------------------------

void til::postfix_writer::do_null_node(til::null_node * const node, int lvl) {
   if (!_function_labels.empty() && !_outside_func) {
    _pf.INT(0);
  } else {
    _pf.SINT(0);
  }
}

void til::postfix_writer::do_return_node(til::return_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  auto symbol = _symtab.find("@", 1);
  auto rettype = cdk::functional_type::cast(symbol->type())->output(0);
  auto rettype_name = rettype->name();

  if (rettype_name != cdk::TYPE_VOID) {
    accept_covariant_node(rettype, node->retval(), lvl + 2);
    if (rettype_name == cdk::TYPE_DOUBLE) {
      _pf.STFVAL64();
    } else {
      _pf.STFVAL32();
    }
  }
  _pf.JMP(_current_func_ret_label);

  _loop_ended = true;
}

void til::postfix_writer::do_address_of_node(til::address_of_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->lvalue()->accept(this, lvl + 2);
}

void til::postfix_writer::do_block_node(til::block_node * const node, int lvl) {
   _symtab.push(); // for block-local variables
  node->declarations()->accept(this, lvl + 2);

  _loop_ended = false;
  for (size_t i = 0; i < node->instructions()->size(); i++) {
    auto child = node->instructions()->node(i);

    if (_loop_ended) {
      THROW_ERROR_FOR_NODE(child, "unreachable code; further instructions found after a final instruction");
    }

    child->accept(this, lvl + 2);
  }
  _loop_ended = false;

  _symtab.pop();
}

void til::postfix_writer::do_function_call_node(til::function_call_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  std::shared_ptr<cdk::functional_type> func_type;
  if (!node->func()) { 
    auto symbol = _symtab.find("@", 1);
    func_type = cdk::functional_type::cast(symbol->type());
  } else {
    func_type = cdk::functional_type::cast(node->func()->type());
  }

  size_t args_size = 0;
  for (size_t i = node->arguments()->size(); i > 0; i--) {
    auto arg = dynamic_cast<cdk::expression_node*>(node->arguments()->node(i - 1));

    args_size += arg->type()->size();
    accept_covariant_node(func_type->input(i - 1), arg, lvl + 2);
  }

  _external_func_name = std::nullopt;
  if (!node->func()) { 
    _pf.ADDR(_function_labels.top());
  } else {
    node->func()->accept(this, lvl);
  }

  if (_external_func_name) {
    _pf.CALL(*_external_func_name);
    _external_func_name = std::nullopt;
  } else {
    _pf.BRANCH();
  }

  if (args_size > 0) {
    _pf.TRASH(args_size);
  }

  if (node->is_typed(cdk::TYPE_DOUBLE)) {
    _pf.LDFVAL64();
  } else if (!node->is_typed(cdk::TYPE_VOID)) {
    _pf.LDFVAL32();
  }
}

void til::postfix_writer::do_function_definition_node(til::function_definition_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  
  std::string function_label;
  if (node->is_main()) {
    function_label = "_main";
  } else {
    function_label = mklbl(++_lbl);
  }
  _function_labels.push(function_label);

  _pf.TEXT(_function_labels.top());
  _pf.ALIGN();
  if (node->is_main()) _pf.GLOBAL("_main", _pf.FUNC());
  _pf.LABEL(_function_labels.top());

  int previous_offset = _offset;
  _offset = 8; 
  _symtab.push();

  _inFunctionArgs = true;
  node->arguments()->accept(this, lvl);
  _inFunctionArgs = false;

  // compute stack size to be reserved for local variables
  frame_size_calculator lsc(_compiler, _symtab);
  node->block()->accept(&lsc, lvl);
  _pf.ENTER(lsc.localsize()); // total stack size reserved for local variables
  
  auto previous_func_ret_label = _current_func_ret_label;
  _current_func_ret_label = mklbl(++_lbl);

  auto previousFunctionLoopLabels = _cur_func_loop_labels;
  _cur_func_loop_labels = new std::vector<std::pair<std::string, std::string>>();

  _offset = 0;

  node->block()->accept(this, lvl);

  if (node->is_main()) {
    _pf.INT(0);
    _pf.STFVAL32();
  }

  _pf.ALIGN();
  _pf.LABEL(_current_func_ret_label);
  _pf.LEAVE();
  _pf.RET();

  delete _cur_func_loop_labels;
  _cur_func_loop_labels = previousFunctionLoopLabels;
  _current_func_ret_label = previous_func_ret_label;
  _offset = previous_offset;
  _symtab.pop();
  _function_labels.pop();

  if (node->is_main()) {
    for (std::string s : _external_func_to_declare) {
      _pf.EXTERN(s);
    }
    return;
  }

  if (!_function_labels.empty() && !_outside_func) {
    _pf.TEXT(_function_labels.top());
    _pf.ADDR(function_label);
  } else {
    _pf.DATA();
    _pf.SADDR(function_label);
  }
}

void til::postfix_writer::do_index_node(til::index_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->base()->accept(this, lvl + 2);
  node->index()->accept(this, lvl + 2);
  _pf.INT(node->type()->size());
  _pf.MUL();
  _pf.ADD();
}

void til::postfix_writer::do_sizeof_node(til::sizeof_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  _pf.INT(node->expression()->type()->size());
}

void til::postfix_writer::do_objects_node(til::objects_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  auto ref = cdk::reference_type::cast(node->type())->referenced();
  node->argument()->accept(this, lvl);
  _pf.INT(std::max(static_cast<size_t>(1), ref->size()));
  _pf.MUL();
  _pf.ALLOC();
  _pf.SP();
}

//---------------------------------------------------------------------------

template<size_t P, typename T>
void til::postfix_writer::loop_controller(T * const node) {
  ASSERT_SAFE_EXPRESSIONS;

  auto level = static_cast<size_t>(node->level());

  if (level == 0) {
    THROW_ERROR("invalid loop control instruction level");
  } else if (_cur_func_loop_labels->size() < level) {
    THROW_ERROR("loop control instruction not within sufficient loops (expected at most " +
         std::to_string(_cur_func_loop_labels->size()) + ")");
  }

  auto index = _cur_func_loop_labels->size() - level;
  auto label = std::get<P>(_cur_func_loop_labels->at(index));
  _pf.JMP(label);

  _loop_ended = true;
}

void til::postfix_writer::do_next_node(til::next_node * const node, int lvl) {
  loop_controller<0>(node);
}

void til::postfix_writer::do_stop_node(til::stop_node * const node, int lvl) {
  loop_controller<1>(node);
}

void til::postfix_writer::do_declaration_node(til::declaration_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  auto symbol = new_symbol();
  reset_new_symbol();

  int offset = 0;
  int typesize = node->type()->size(); 
  if (_inFunctionArgs) {
    offset = _offset;
    _offset += typesize;
  } else if (!_function_labels.empty() && !_outside_func) {
    _offset -= typesize;
    offset = _offset;
  } else {
    offset = 0;
  }
  symbol->offset(offset);

  if (!_function_labels.empty() && !_outside_func) {
    if (_inFunctionArgs || !node->initializer()) {
      return;
    }
    accept_covariant_node(node->type(), node->initializer(), lvl);
    if (node->is_typed(cdk::TYPE_DOUBLE)) {
      _pf.LOCAL(symbol->offset());
      _pf.STDOUBLE();
    } else {
      _pf.LOCAL(symbol->offset());
      _pf.STINT();
    }
    return;
  }

  if (symbol->qualifier() == tFORWARD || symbol->qualifier() == tEXTERNAL) {
    _external_func_to_declare.insert(symbol->name());
    return;
  }

  _external_func_to_declare.erase(symbol->name());

  if (!node->initializer()) {
    _pf.BSS();
    _pf.ALIGN();
    if (symbol->qualifier() == tPUBLIC)  _pf.GLOBAL(symbol->name(), _pf.OBJ());
    _pf.LABEL(symbol->name());
    _pf.SALLOC(typesize);
    return;
  }

  if (!isInstanceOf<cdk::integer_node, cdk::double_node, cdk::string_node,
        til::null_node, til::function_definition_node>(node->initializer())) {
    THROW_ERROR("non-literal initializer for global variable '" + symbol->name() + "'");
  }

  _pf.DATA();
  _pf.ALIGN();
  if (symbol->qualifier() == tPUBLIC) _pf.GLOBAL(symbol->name(), _pf.OBJ());
  _pf.LABEL(symbol->name());

  if (node->is_typed(cdk::TYPE_DOUBLE) && node->initializer()->is_typed(cdk::TYPE_INT)) {
    auto int_node = dynamic_cast<cdk::integer_node*>(node->initializer());
    _pf.SDOUBLE(int_node->value());
  } else {
    node->initializer()->accept(this, lvl);
  }
  
}

//---------------------------------------------------------------------------

void til::postfix_writer::do_with_node(til::with_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  _symtab.push();

  auto low_name = std::string("_low"); 
  auto low_decl = new til::declaration_node(node->lineno(), tPRIVATE,
      cdk::primitive_type::create(4, cdk::TYPE_INT), low_name, node->low());
  low_decl->accept(this, lvl);
  auto low = new cdk::variable_node(node->lineno(), low_name);
  auto low_rvalue = new cdk::rvalue_node(node->lineno(), low);

  
  auto high_name = std::string("_high");
  auto high_decl = new til::declaration_node(node->lineno(), tPRIVATE,
      cdk::primitive_type::create(4, cdk::TYPE_INT), high_name, node->high());
  high_decl->accept(this, lvl);
  auto high = new cdk::variable_node(node->lineno(), high_name);
  auto high_rvalue = new cdk::rvalue_node(node->lineno(), high);

  auto el = new til::index_node(node->lineno(), node->vector(), low_rvalue);
  auto el_rvalue = new cdk::rvalue_node(node->lineno(), el);

  auto args = new cdk::sequence_node(node->lineno(), el_rvalue);
  auto func_call = new til::function_call_node(node->lineno(), node->function(), args);
  auto func_call_eval = new til::evaluation_node(node->lineno(), func_call);

  auto with_incr_sum = new cdk::add_node(node->lineno(), low_rvalue,
      new cdk::integer_node(node->lineno(), 1));
  auto with_incr_assign = new cdk::assignment_node(node->lineno(), low, with_incr_sum);
  auto with_incr_eval = new til::evaluation_node(node->lineno(), with_incr_assign);

  auto loop_cond = new cdk::lt_node(node->lineno(), low_rvalue, high_rvalue);

  auto loop_body = new cdk::sequence_node(node->lineno(), func_call_eval);
  loop_body = new cdk::sequence_node(node->lineno(), with_incr_eval, loop_body);

  auto loop = new til::loop_node(node->lineno(), loop_cond, loop_body);

  //auto if_instr = new til::if_node(node->lineno(), loop_cond, loop);
  //if_instr->accept(this, lvl);

  loop->accept(this, lvl);

  _symtab.pop();
}

void til::postfix_writer::do_unless_node(til::unless_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  int endLabel;
  _pf.ALIGN();
  node->condition()->accept(this, lvl);
  _pf.JNZ(mklbl(endLabel = ++_lbl));

  _symtab.push(); 

  auto unless_name = std::string("_unless"); 
  auto unless_decl = new til::declaration_node(node->lineno(), tPRIVATE,
      cdk::primitive_type::create(4, cdk::TYPE_INT), unless_name, new cdk::integer_node(node->lineno(), 0));
  unless_decl->accept(this, lvl);
  auto unless = new cdk::variable_node(node->lineno(), unless_name);
  auto unless_rvalue = new cdk::rvalue_node(node->lineno(), unless);

  
  auto count_name = std::string("_count");
  auto count_decl = new til::declaration_node(node->lineno(), tPRIVATE,
      cdk::primitive_type::create(4, cdk::TYPE_INT), count_name, node->count());
  count_decl->accept(this, lvl);
  auto count = new cdk::variable_node(node->lineno(), count_name);
  auto count_rvalue = new cdk::rvalue_node(node->lineno(), count);

  auto el = new til::index_node(node->lineno(), node->vector(), unless_rvalue);
  auto el_rvalue = new cdk::rvalue_node(node->lineno(), el);
  auto args = new cdk::sequence_node(node->lineno(), el_rvalue);
  auto func_call = new til::function_call_node(node->lineno(), node->function(), args);
  auto func_call_eval = new til::evaluation_node(node->lineno(), func_call);

  auto unless_incr_sum = new cdk::add_node(node->lineno(), unless_rvalue,
      new cdk::integer_node(node->lineno(), 1));
  auto unless_incr_assign = new cdk::assignment_node(node->lineno(), unless, unless_incr_sum);
  auto unless_incr_eval = new til::evaluation_node(node->lineno(), unless_incr_assign);

  auto loop_cond = new cdk::lt_node(node->lineno(), unless_rvalue, count_rvalue);

  auto loop_body = new cdk::sequence_node(node->lineno(), func_call_eval);
  loop_body = new cdk::sequence_node(node->lineno(), unless_incr_eval, loop_body);

  auto loop = new til::loop_node(node->lineno(), loop_cond, loop_body);

  loop->accept(this, lvl);

  _symtab.pop();

  _pf.ALIGN();
  _pf.LABEL(mklbl(endLabel));
}

void til::postfix_writer::do_sweep_node(til::sweep_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  int endLabel;
  _pf.ALIGN();
  node->condition()->accept(this, lvl);
  _pf.JZ(mklbl(endLabel = ++_lbl));

  _symtab.push();

  auto low_name = std::string("_low"); 
  auto low_decl = new til::declaration_node(node->lineno(), tPRIVATE,
      cdk::primitive_type::create(4, cdk::TYPE_INT), low_name, node->low());
  low_decl->accept(this, lvl);
  auto low = new cdk::variable_node(node->lineno(), low_name);
  auto low_rvalue = new cdk::rvalue_node(node->lineno(), low);

  auto el = new til::index_node(node->lineno(), node->vector(), low_rvalue);
  auto el_rvalue = new cdk::rvalue_node(node->lineno(), el);
  auto args = new cdk::sequence_node(node->lineno(), el_rvalue);
  auto func_call = new til::function_call_node(node->lineno(), node->function(), args);
  auto func_call_eval = new til::evaluation_node(node->lineno(), func_call);

  auto with_incr_sum = new cdk::add_node(node->lineno(), low_rvalue,
      new cdk::integer_node(node->lineno(), 1));
  auto with_incr_assign = new cdk::assignment_node(node->lineno(), low, with_incr_sum);
  auto with_incr_eval = new til::evaluation_node(node->lineno(), with_incr_assign);

  auto loop_cond = new cdk::lt_node(node->lineno(), low_rvalue, node->high());
  auto loop_body = new cdk::sequence_node(node->lineno(), func_call_eval);
  loop_body = new cdk::sequence_node(node->lineno(), with_incr_eval, loop_body);
  auto loop = new til::loop_node(node->lineno(), loop_cond, loop_body);
  loop->accept(this, lvl);

  _symtab.pop();

  _pf.ALIGN();
  _pf.LABEL(mklbl(endLabel));
}

void til::postfix_writer::do_iterate_node(til::iterate_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  int endLabel;
  auto lineno = node->lineno();

  _pf.ALIGN();
  node->condition()->accept(this, lvl);
  _pf.JZ(mklbl(endLabel = ++_lbl));

  _symtab.push();

  auto iterate_name = std::string("_iterate");
  auto iterate_decl = new til::declaration_node(lineno, tPRIVATE,
      cdk::primitive_type::create(4, cdk::TYPE_INT), iterate_name, new cdk::integer_node(lineno, 0));
  iterate_decl->accept(this, lvl);
  auto iterate = new cdk::variable_node(lineno, iterate_name);
  auto iterate_rvalue = new cdk::rvalue_node(lineno, iterate);

  auto el = new til::index_node(lineno, node->vector(), iterate_rvalue);
  auto el_rvalue = new cdk::rvalue_node(lineno, el);
  auto args = new cdk::sequence_node(lineno, el_rvalue);
  auto func_call = new til::function_call_node(lineno, node->function(), args);
  auto func_call_eval = new til::evaluation_node(lineno, func_call);

  auto incr_sum = new cdk::add_node(lineno, iterate_rvalue,  new cdk::integer_node(lineno, 1));
  auto incr_assign = new cdk::assignment_node(lineno, iterate, incr_sum);
  auto incr_eval = new til::evaluation_node(lineno, incr_assign);

  auto comparison = new cdk::lt_node(lineno, iterate_rvalue,  node->count());
  auto loop_body = new cdk::sequence_node(lineno, func_call_eval);
  loop_body = new cdk::sequence_node(lineno, incr_eval, loop_body);
  auto loop = new til::loop_node(lineno, comparison, loop_body);
  loop->accept(this, lvl);
  
  _symtab.pop();

  _pf.ALIGN();
  _pf.LABEL(mklbl(endLabel));
}