/*
 * Copyright 2006-2008 The FLWOR Foundation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#include "functions/func_sequences.h"
#include "functions/func_sequences_impl.h"

#include "runtime/sequences/sequences.h"
#include "runtime/sequences/SequencesImpl.h"
#include "runtime/core/var_iterators.h"

#include "system/globalenv.h"

#include "compiler/expression/expr.h"
#include "compiler/expression/fo_expr.h"
#include "compiler/expression/path_expr.h"

#include "types/typeops.h"

namespace zorba 
{


/*******************************************************************************

********************************************************************************/
xqtref_t op_concatenate::getReturnType(
    const TypeManager* tm,
    const std::vector<xqtref_t>& arg_types) const 
{
  int sz = arg_types.size();

  if (sz == 0)
  {
    return GENV_TYPESYSTEM.EMPTY_TYPE;
  }
  else 
  {
    xqtref_t t = arg_types[0];
    TypeConstants::quantifier_t q = TypeConstants::QUANT_STAR;
    for (int i = 1; i < sz; i++) 
    {
      t = TypeOps::union_type(*t, *arg_types[i], tm);
      TypeConstants::quantifier_t pq = TypeOps::quantifier(*t);
      if (pq == TypeConstants::QUANT_ONE || pq == TypeConstants::QUANT_PLUS)
        q = TypeConstants::QUANT_PLUS;
    }

    TypeManager* tm = t->get_manager();

    return tm->create_type_x_quant(*t, q);
  }
}

  
BoolAnnotationValue op_concatenate::ignoresSortedNodes(expr* fo, ulong input) const 
{
  return fo->getIgnoresSortedNodes();
}


BoolAnnotationValue op_concatenate::ignoresDuplicateNodes(expr* fo, ulong input) const 
{
  return fo->getIgnoresDuplicateNodes();
}


/*******************************************************************************

********************************************************************************/
BoolAnnotationValue fn_empty::ignoresSortedNodes(expr* fo, ulong input) const 
{
  return ANNOTATION_TRUE;
}


BoolAnnotationValue fn_empty::ignoresDuplicateNodes(expr* fo, ulong input) const 
{
  return ANNOTATION_TRUE;
}


/*******************************************************************************

********************************************************************************/
BoolAnnotationValue fn_exists::ignoresSortedNodes(expr* fo, ulong input) const 
{
  return ANNOTATION_TRUE;
}


BoolAnnotationValue fn_exists::ignoresDuplicateNodes(expr* fo, ulong input) const 
{
  return ANNOTATION_TRUE;
}


/*******************************************************************************

********************************************************************************/
xqtref_t fn_distinct_values::getReturnType(
    const TypeManager* tm,
    const std::vector<xqtref_t>& arg_types) const
{
  return arg_types[0];
}


BoolAnnotationValue fn_distinct_values::ignoresSortedNodes(
    expr* fo, 
    ulong input) const 
{
  if (input == 0)
    return fo->getIgnoresSortedNodes();

  return ANNOTATION_FALSE;
}


BoolAnnotationValue fn_distinct_values::ignoresDuplicateNodes(
    expr* fo,
    ulong input) const 
{
  if (input == 0)
    return ANNOTATION_TRUE;

  return ANNOTATION_FALSE;
}


/*******************************************************************************

********************************************************************************/
BoolAnnotationValue fn_insert_before::ignoresSortedNodes(
    expr* fo,
    ulong input) const 
{
  if (input == 0 || input == 2)
    return fo->getIgnoresSortedNodes();

  return ANNOTATION_TRUE;
}


BoolAnnotationValue fn_insert_before::ignoresDuplicateNodes(
    expr* fo,
    ulong input) const 
{
  if (input == 0 || input == 2)
    return fo->getIgnoresSortedNodes();

  return ANNOTATION_FALSE;
}


/*******************************************************************************

********************************************************************************/
xqtref_t fn_remove::getReturnType(
    const TypeManager* tm,
    const std::vector<xqtref_t>& arg_types) const
{
  return tm->create_type_x_quant(*arg_types[0], TypeConstants::QUANT_QUESTION);
}


BoolAnnotationValue fn_remove::ignoresSortedNodes(
    expr* fo,
    ulong input) const 
{
  if (input == 0)
    return ANNOTATION_FALSE;

  return ANNOTATION_TRUE;
}


BoolAnnotationValue fn_remove::ignoresDuplicateNodes(
    expr* fo,
    ulong input) const 
{
  return ANNOTATION_FALSE;
}


/*******************************************************************************

********************************************************************************/
xqtref_t fn_reverse::getReturnType(
    const TypeManager* tm,
    const std::vector<xqtref_t>& arg_types) const
{
  return arg_types[0];
}


BoolAnnotationValue fn_reverse::ignoresSortedNodes(
    expr* fo,
    ulong input) const 
{
  return fo->getIgnoresSortedNodes();
}


BoolAnnotationValue fn_reverse::ignoresDuplicateNodes(
    expr* fo,
    ulong input) const 
{
  return fo->getIgnoresSortedNodes();
}


/*******************************************************************************

********************************************************************************/
xqtref_t fn_subsequence::getReturnType(
    const TypeManager* tm,
    const std::vector<xqtref_t>& arg_types) const
{
  return tm->create_type_x_quant(*arg_types[0], TypeConstants::QUANT_QUESTION);
}


PlanIter_t fn_subsequence::codegen(
    CompilerCB* /*cb*/,
    static_context* aSctx,
    const QueryLoc& aLoc,
    std::vector<PlanIter_t>& aArgs,
    AnnotationHolder& aAnn) const
{
  fo_expr& subseqExpr = static_cast<fo_expr&>(aAnn);

  const expr* inputExpr = subseqExpr.get_arg(0);

  const expr* posExpr = subseqExpr.get_arg(1);

  const expr* lenExpr = (subseqExpr.num_args() > 2 ? subseqExpr.get_arg(2) : NULL);

  if (inputExpr->get_expr_kind() == relpath_expr_kind &&
      posExpr->get_expr_kind() == const_expr_kind && 
      lenExpr != NULL &&
      lenExpr->get_expr_kind() == const_expr_kind)
  {
    xqp_double dpos = static_cast<const const_expr*>(posExpr)->
                      get_val()->getDoubleValue().round();
    long pos = static_cast<long>(dpos.getNumber());

    xqp_double dlen = static_cast<const const_expr*>(lenExpr)->
                      get_val()->getDoubleValue().round();
    long len = static_cast<long>(dlen.getNumber());

    const relpath_expr* pathExpr = static_cast<const relpath_expr*>(inputExpr);

    ulong numSteps = pathExpr->numSteps();

    if (pos > 0 && len == 1 && numSteps == 2)
    {
      AxisIteratorHelper* input = dynamic_cast<AxisIteratorHelper*>(aArgs[0].getp());
      assert(input != NULL);

      if (input->setTargetPos(pos-1))
        return aArgs[0];
    }
  }

  return new FnSubsequenceIterator(aSctx, aLoc, aArgs);
}


/******************************************************************************

*******************************************************************************/
xqtref_t op_zorba_subsequence_int::getReturnType(
    const TypeManager* tm,
    const std::vector<xqtref_t>& arg_types) const
{
  return tm->create_type_x_quant(*arg_types[0], TypeConstants::QUANT_QUESTION);
}


PlanIter_t op_zorba_subsequence_int::codegen(
    CompilerCB* /*cb*/,
    static_context* aSctx,
    const QueryLoc& aLoc,
    std::vector<PlanIter_t>& aArgs,
    AnnotationHolder& aAnn) const
{
  fo_expr& subseqExpr = static_cast<fo_expr&>(aAnn);

  const expr* inputExpr = subseqExpr.get_arg(0);

  const expr* posExpr = subseqExpr.get_arg(1);

  const expr* lenExpr = (subseqExpr.num_args() > 2 ? subseqExpr.get_arg(2) : NULL);

  LetVarIterator* letVarIter;
  CtxVarIterator* prologVarIter;

  if (inputExpr->get_expr_kind() == relpath_expr_kind &&
      posExpr->get_expr_kind() == const_expr_kind && 
      lenExpr != NULL &&
      lenExpr->get_expr_kind() == const_expr_kind)
  {
    xqp_long pos = static_cast<const const_expr*>(posExpr)->
                      get_val()->getLongValue();

    xqp_long len = static_cast<const const_expr*>(lenExpr)->
                      get_val()->getLongValue();

    const relpath_expr* pathExpr = static_cast<const relpath_expr*>(inputExpr);

    ulong numSteps = pathExpr->numSteps();

    if (pos > 0 && len == 1 && numSteps == 2)
    {
      AxisIteratorHelper* input = dynamic_cast<AxisIteratorHelper*>(aArgs[0].getp());
      assert(input != NULL);

      if (input->setTargetPos(pos-1))
        return aArgs[0];
    }
  }
  else if ((letVarIter = dynamic_cast<LetVarIterator*>(aArgs[0].getp())) != NULL)
  {
    const var_expr* inputVar = inputExpr->get_var();
    if (inputVar != NULL &&
        lenExpr != NULL &&
        inputVar->get_kind() != var_expr::win_var &&
        letVarIter->setTargetPosIter(aArgs[1]) &&
        letVarIter->setTargetLenIter(aArgs[2]))
    {
      return aArgs[0];
    }
  }
  else if ((prologVarIter = dynamic_cast<CtxVarIterator*>(aArgs[0].getp())) != NULL)
  {
    const var_expr* inputVar = inputExpr->get_var();
    if (inputVar != NULL &&
        lenExpr != NULL &&
        !inputVar->is_context_item() &&
        prologVarIter->setTargetPosIter(aArgs[1]) &&
        prologVarIter->setTargetLenIter(aArgs[2]))
    {
      return aArgs[0];
    }
  }

  return new SubsequenceIntIterator(aSctx, aLoc, aArgs);
}



/*******************************************************************************

********************************************************************************/
xqtref_t op_zorba_sequence_point_access::getReturnType(
    const TypeManager* tm,
    const std::vector<xqtref_t>& arg_types) const
{
  return tm->create_type(*arg_types[0], TypeConstants::QUANT_QUESTION);
}


PlanIter_t op_zorba_sequence_point_access::codegen(
    CompilerCB* /*cb*/,
    static_context* aSctx,
    const QueryLoc& aLoc,
    std::vector<PlanIter_t>& aArgs,
    AnnotationHolder& aAnn) const
{
  fo_expr& subseqExpr = static_cast<fo_expr&>(aAnn);

  const expr* inputExpr = subseqExpr.get_arg(0);

  const expr* posExpr = subseqExpr.get_arg(1);

  LetVarIterator* inputVarIter;
  CtxVarIterator* prologVarIter;

  if (posExpr->get_expr_kind() == const_expr_kind)
  {
    store::Item* posItem = static_cast<const const_expr*>(posExpr)->get_val();
    xqp_long pos = posItem->getLongValue();

    if (inputExpr->get_expr_kind() == relpath_expr_kind)
    {
      const relpath_expr* pathExpr = static_cast<const relpath_expr*>(inputExpr);
      
      ulong numSteps = pathExpr->numSteps();

      if (pos > 0 && numSteps == 2)
      {
        AxisIteratorHelper* input = dynamic_cast<AxisIteratorHelper*>(aArgs[0].getp());
        assert(input != NULL);
        
        if (input->setTargetPos(pos-1))
          return aArgs[0];
      }
    }
    else if ((inputVarIter = dynamic_cast<LetVarIterator*>(aArgs[0].getp())) != NULL)
    {
      if (inputVarIter->setTargetPos(pos))
        return aArgs[0];
    }
  }
  else if ((inputVarIter = dynamic_cast<LetVarIterator*>(aArgs[0].getp())) != NULL)
  {
    const var_expr* inputVar = inputExpr->get_var();
      
    if (inputVar != NULL &&
        inputVar->get_kind() != var_expr::win_var &&
        inputVarIter->setTargetPosIter(aArgs[1]))
      return aArgs[0];
  }
  else if ((prologVarIter = dynamic_cast<CtxVarIterator*>(aArgs[0].getp())) != NULL)
  {
    const var_expr* inputVar = inputExpr->get_var();

    if (inputVar != NULL &&
        !inputVar->is_context_item() &&
        prologVarIter->setTargetPosIter(aArgs[1]))
    {
      return aArgs[0];
    }
  }

  return new SequencePointAccessIterator(aSctx, aLoc, aArgs);
}


/*******************************************************************************

********************************************************************************/
BoolAnnotationValue fn_unordered::ignoresSortedNodes(expr* fo, ulong input) const
{
  return ANNOTATION_TRUE;
}


BoolAnnotationValue fn_unordered::ignoresDuplicateNodes(expr* fo, ulong input) const
{
  return fo->getIgnoresDuplicateNodes();
}


PlanIter_t fn_unordered::codegen(
    CompilerCB* /*cb*/,
    static_context* sctx,
    const QueryLoc& loc,
    std::vector<PlanIter_t>& argv,
    AnnotationHolder& ) const
{
  return argv[0];
}


/*******************************************************************************

********************************************************************************/
xqtref_t fn_zero_or_one::getReturnType(
    const TypeManager* tm,
    const std::vector<xqtref_t>& arg_types) const
{
  xqtref_t srcType = arg_types[0];

  return tm->create_type(*TypeOps::prime_type(tm, *srcType),
                         TypeConstants::QUANT_QUESTION);
}


BoolAnnotationValue fn_zero_or_one::ignoresSortedNodes(expr* fo, ulong input) const 
{
  return ANNOTATION_TRUE;
}


PlanIter_t fn_zero_or_one::codegen(
    CompilerCB* /*cb*/,
    static_context* aSctx,
    const QueryLoc& aLoc,
    std::vector<PlanIter_t>& aArgs,
    AnnotationHolder& aAnn) const
{
  return new FnZeroOrOneIterator(aSctx,
                                 aLoc,
                                 aArgs,
                                 testFlag(FunctionConsts::DoDistinct));
}


/*******************************************************************************

********************************************************************************/
xqtref_t fn_one_or_more::getReturnType(
    const TypeManager* tm,
    const std::vector<xqtref_t>& arg_types) const
{
  return tm->create_type(*TypeOps::prime_type(tm, *arg_types[0]),
                         TypeConstants::QUANT_PLUS);
}


BoolAnnotationValue fn_one_or_more::ignoresSortedNodes(expr* fo, ulong input) const 
{
  return fo->getIgnoresSortedNodes();
}


BoolAnnotationValue fn_one_or_more::ignoresDuplicateNodes(expr* fo, ulong input) const 
{
  return fo->getIgnoresDuplicateNodes();
}


/*******************************************************************************

********************************************************************************/
xqtref_t fn_exactly_one_noraise::getReturnType(
    const TypeManager* tm,
    const std::vector<xqtref_t>& arg_types) const
{
  if (theRaiseError)
    return TypeOps::prime_type(tm, *arg_types[0]);
  else
    return function::getReturnType(tm, arg_types);
}


PlanIter_t fn_exactly_one_noraise::codegen(
    CompilerCB* aCb,
    static_context* aSctx,
    const QueryLoc& aLoc,
    std::vector<PlanIter_t>& aArgs,
    AnnotationHolder& aAnn) const
{
  return new FnExactlyOneIterator(aSctx,
                                  aLoc,
                                  aArgs,
                                  theRaiseError,
                                  testFlag(FunctionConsts::DoDistinct));
}


/*******************************************************************************

********************************************************************************/
PlanIter_t fn_union::codegen(
    CompilerCB* /*cb*/,
    static_context* sctx,
    const QueryLoc& loc,
    std::vector<PlanIter_t>& argv,
    AnnotationHolder& ann) const
{
  return new FnConcatIterator(sctx, loc, argv, false);
}


/*******************************************************************************

********************************************************************************/
PlanIter_t fn_intersect::codegen(
    CompilerCB* /*cb*/,
    static_context* sctx,
    const QueryLoc& loc, 
    std::vector<PlanIter_t>& argv,
    AnnotationHolder& ann) const
{
#if 0  // we can't access PRODUCES_* from the inputs, must rethink
  bool distinct = ann.get_annotation (Annotations::IGNORES_DUP_NODES) != TSVAnnotationValue::TRUE_VAL;
  bool sort = ann.get_annotation (Annotations::IGNORES_SORTED_NODES) != TSVAnnotationValue::TRUE_VAL;

  std::vector<PlanIter_t> inputs;
  for (std::vector<PlanIter_t>::iterator i = argv.begin ();
       i != argv.end (); i++)
    inputs.push_back (new NodeSortIterator (loc, *i, true, distinct, false));
  return new SortSemiJoinIterator(loc, inputs);
#endif

  return new HashSemiJoinIterator(sctx, loc, argv);
}


/*******************************************************************************

********************************************************************************/
PlanIter_t fn_except::codegen(
    CompilerCB* /*cb*/,
    static_context* sctx,
    const QueryLoc& loc,
    std::vector<PlanIter_t>& argv,
    AnnotationHolder& ann) const
{
  // TODO: use SortAntiJoinIterator when available (trac ticket 254)
  return new HashSemiJoinIterator(sctx, loc, argv, true);
}


/*******************************************************************************

********************************************************************************/
BoolAnnotationValue fn_count::ignoresSortedNodes(expr* fo, ulong input) const 
{
  return ANNOTATION_TRUE;
}


/*******************************************************************************

********************************************************************************/
BoolAnnotationValue fn_avg::ignoresSortedNodes(expr* fo, ulong input) const 
{
  return ANNOTATION_TRUE;
}


/*******************************************************************************

********************************************************************************/
PlanIter_t fn_max::codegen(
    CompilerCB* /*cb*/,
    static_context* aSctx,
    const QueryLoc& aLoc,
    std::vector<PlanIter_t>& aArgs,
    AnnotationHolder& /*aAnn*/) const
{
  return new FnMinMaxIterator(aSctx, aLoc, aArgs, FnMinMaxIterator::MAX);
}


/*******************************************************************************

********************************************************************************/
PlanIter_t fn_min::codegen(
    CompilerCB* /*cb*/,
    static_context* aSctx,
    const QueryLoc& aLoc,
    std::vector<PlanIter_t>& aArgs,
    AnnotationHolder& /*ann*/) const
{
  return new FnMinMaxIterator(aSctx, aLoc, aArgs, FnMinMaxIterator::MIN);
}


/*******************************************************************************

********************************************************************************/
BoolAnnotationValue fn_sum::ignoresSortedNodes(expr* fo, ulong input) const 
{
  return ANNOTATION_TRUE;
}


/*******************************************************************************

********************************************************************************/
function* fn_sum::specialize(
   static_context* sctx,
   const std::vector<xqtref_t>& argTypes) const
{
  RootTypeManager& rtm = GENV_TYPESYSTEM;
  TypeManager* tm = sctx->get_typemanager();

  xqtref_t argType = argTypes[0];

  if (TypeOps::is_subtype(tm, *argType, *rtm.UNTYPED_ATOMIC_TYPE_STAR))
  {
    return (getArity() == 1 ?
            GET_BUILTIN_FUNCTION(OP_SUM_DOUBLE_1) :
            GET_BUILTIN_FUNCTION(OP_SUM_DOUBLE_2));
  }
  else if (TypeOps::is_subtype(tm, *argType, *rtm.DOUBLE_TYPE_STAR))
  {
    return (getArity() == 1 ?
            GET_BUILTIN_FUNCTION(OP_SUM_DOUBLE_1) :
            GET_BUILTIN_FUNCTION(OP_SUM_DOUBLE_2));
  }
  else if (TypeOps::is_subtype(tm, *argType, *rtm.FLOAT_TYPE_STAR))
  {
    return (getArity() == 1 ?
            GET_BUILTIN_FUNCTION(OP_SUM_FLOAT_1) :
            GET_BUILTIN_FUNCTION(OP_SUM_FLOAT_2));
  }
  else if (TypeOps::is_subtype(tm, *argType, *rtm.INTEGER_TYPE_STAR))
  {
    return (getArity() == 1 ?
            GET_BUILTIN_FUNCTION(OP_SUM_INTEGER_1) :
            GET_BUILTIN_FUNCTION(OP_SUM_INTEGER_2));
  }
  else if (TypeOps::is_subtype(tm, *argType, *rtm.DECIMAL_TYPE_STAR))
  {
    return (getArity() == 1 ?
            GET_BUILTIN_FUNCTION(OP_SUM_DECIMAL_1) :
            GET_BUILTIN_FUNCTION(OP_SUM_DECIMAL_2));
  }
  else
  {
    return NULL;
  }
}


/*******************************************************************************

********************************************************************************/
BoolAnnotationValue op_sum_double::ignoresSortedNodes(expr* fo, ulong input) const
{
  return ANNOTATION_TRUE;
}


/*******************************************************************************

********************************************************************************/
BoolAnnotationValue op_sum_float::ignoresSortedNodes(expr* fo, ulong input) const
{
  return ANNOTATION_TRUE;
}


/*******************************************************************************

********************************************************************************/
BoolAnnotationValue op_sum_decimal::ignoresSortedNodes(expr* fo, ulong input) const
{
  return ANNOTATION_TRUE;
}


/*******************************************************************************

********************************************************************************/
BoolAnnotationValue op_sum_integer::ignoresSortedNodes(expr* fo, ulong input) const
{
  return ANNOTATION_TRUE;
}


/*******************************************************************************

********************************************************************************/
BoolAnnotationValue fn_id::ignoresSortedNodes(expr* fo, ulong input) const 
{
  return ANNOTATION_TRUE;
}


BoolAnnotationValue fn_id::ignoresDuplicateNodes(expr* fo, ulong input) const 
{
  if (input == 0)
    return ANNOTATION_TRUE;

  return ANNOTATION_FALSE;
}


/*******************************************************************************

********************************************************************************/
BoolAnnotationValue fn_element_with_id::ignoresSortedNodes(
    expr* fo,
    ulong input) const 
{
  return ANNOTATION_TRUE;
}


BoolAnnotationValue fn_element_with_id::ignoresDuplicateNodes(
    expr* fo,
    ulong input) const 
{
  if (input == 0)
    return ANNOTATION_TRUE;

  return ANNOTATION_FALSE;
}


/*******************************************************************************

********************************************************************************/
BoolAnnotationValue fn_idref::ignoresSortedNodes(expr* fo, ulong input) const 
{
  return ANNOTATION_TRUE;
}


BoolAnnotationValue fn_idref::ignoresDuplicateNodes(expr* fo, ulong input) const 
{
  if (input == 0)
    return ANNOTATION_TRUE;

  return ANNOTATION_FALSE;
}


/*******************************************************************************

********************************************************************************/
void populate_context_sequences_impl(static_context* sctx)
{
  const char* xquery_op_ns = static_context::XQUERY_OP_NS.c_str();
  const char* zorba_op_ns = static_context::ZORBA_OP_NS.c_str();

  DECL(sctx, fn_unordered,
       (createQName(XQUERY_FN_NS, "", "unordered"),
        GENV_TYPESYSTEM.ITEM_TYPE_STAR,
        GENV_TYPESYSTEM.ITEM_TYPE_STAR));

  DECL(sctx, fn_exactly_one,
       (createQName(XQUERY_FN_NS, "", "exactly-one"),
        GENV_TYPESYSTEM.ITEM_TYPE_STAR, 
        GENV_TYPESYSTEM.ITEM_TYPE_ONE));

  DECL(sctx, fn_exactly_one_noraise,
       (createQName(zorba_op_ns, "", "exactly-one-noraise"),
        GENV_TYPESYSTEM.ITEM_TYPE_STAR,
        GENV_TYPESYSTEM.BOOLEAN_TYPE_ONE));

  DECL(sctx, fn_union,
       (createQName(xquery_op_ns,"","union"),
        GENV_TYPESYSTEM.ANY_NODE_TYPE_STAR,
        GENV_TYPESYSTEM.ANY_NODE_TYPE_STAR,
        GENV_TYPESYSTEM.ANY_NODE_TYPE_STAR));

  DECL(sctx, fn_intersect,
       (createQName(xquery_op_ns,"","intersect"),
        GENV_TYPESYSTEM.ANY_NODE_TYPE_STAR,
        GENV_TYPESYSTEM.ANY_NODE_TYPE_STAR,
        GENV_TYPESYSTEM.ANY_NODE_TYPE_STAR));

  DECL(sctx, fn_except,
       (createQName(xquery_op_ns,"","except"),
        GENV_TYPESYSTEM.ANY_NODE_TYPE_STAR,
        GENV_TYPESYSTEM.ANY_NODE_TYPE_STAR,
        GENV_TYPESYSTEM.ANY_NODE_TYPE_STAR));

  DECL(sctx, fn_max,
       (createQName(XQUERY_FN_NS,"","max"),
        GENV_TYPESYSTEM.ANY_ATOMIC_TYPE_STAR, 
        GENV_TYPESYSTEM.ANY_ATOMIC_TYPE_QUESTION));
  
  DECL(sctx, fn_max,
       (createQName(XQUERY_FN_NS,"","max"),
        GENV_TYPESYSTEM.ANY_ATOMIC_TYPE_STAR,
        GENV_TYPESYSTEM.STRING_TYPE_ONE,
        GENV_TYPESYSTEM.ANY_ATOMIC_TYPE_QUESTION));

  DECL(sctx, fn_min,
       (createQName(XQUERY_FN_NS,"","min"),
        GENV_TYPESYSTEM.ANY_ATOMIC_TYPE_STAR, 
        GENV_TYPESYSTEM.ANY_ATOMIC_TYPE_QUESTION));

  DECL(sctx, fn_min,
       (createQName(XQUERY_FN_NS,"","min"),
        GENV_TYPESYSTEM.ANY_ATOMIC_TYPE_STAR, 
        GENV_TYPESYSTEM.STRING_TYPE_ONE, 
        GENV_TYPESYSTEM.ANY_ATOMIC_TYPE_QUESTION));

}

} /* namespace zorba */