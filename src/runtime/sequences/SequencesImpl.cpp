/*
 * Copyright 2006-2016 zorba.io
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
#include "stdafx.h"

#include <string>
#include <fstream>
#include <vector>
#include <sstream>

#include "zorbautils/fatal.h"

#include "diagnostics/xquery_diagnostics.h"
#include "diagnostics/util_macros.h"

#include "zorbatypes/URI.h"

// For timing
#include "zorbatypes/zorbatypes_decl.h"
#include "zorbatypes/datetime.h"
#include "zorbatypes/duration.h"
#include "zorbatypes/float.h"

#include "compiler/api/compilercb.h"

#include "runtime/sequences/pregenerated/sequences.h"
#include "runtime/sequences/SequencesImpl.h"
#include "runtime/core/arithmetic_impl.h"
#include "runtime/util/iterator_impl.h"
#include "runtime/booleans/BooleanImpl.h"
#include "runtime/visitors/planiter_visitor.h"

#include "system/globalenv.h"

#include "types/casting.h"
#include "types/typeops.h"

#include "store/api/store.h"
#include "store/api/iterator.h"
#include "store/api/item_factory.h"
#include "store/api/pul.h"

#include "context/static_context.h"
#include "context/dynamic_context.h"


namespace zorba {

SERIALIZABLE_CLASS_VERSIONS(FnMinMaxIterator)

NARY_ACCEPT(FnMinMaxIterator);
DEF_GET_NAME_AS_STRING(FnMinMaxIterator)


/////////////////////////////////////////////////////////////////////////////////
//                                                                             //
//  15.1 General Functions and Operators on Sequences                          //
//                                                                             //
/////////////////////////////////////////////////////////////////////////////////


/*******************************************************************************
  15.4.3 fn:max
  15.4.4 fn:min
********************************************************************************/
FnMinMaxIterator::FnMinMaxIterator(
    static_context* sctx,
    const QueryLoc& loc,
    std::vector<PlanIter_t>& aChildren,
    Type aType)
  :
  NaryBaseIterator<FnMinMaxIterator, PlanIteratorState>(sctx, loc, aChildren),
  theType(aType),
  theCompareType((aType == MIN ?
                  CompareConsts::VALUE_LESS :
                  CompareConsts::VALUE_GREATER))
{
}


bool
FnMinMaxIterator::nextImpl(store::Item_t& result, PlanState& planState) const
{
  store::Item_t runningItem = NULL;
  store::SchemaTypeCode maxType = store::XS_LAST;

  const TypeManager* tm = theSctx->get_typemanager();
  long timezone = planState.theLocalDynCtx->get_implicit_timezone();
  XQPCollator* collator = 0;
  unsigned elems_in_seq = 0;
  result = NULL;

  try
  {
    PlanIteratorState* state;
    DEFAULT_STACK_INIT(PlanIteratorState, state, planState);

    if (theChildren.size() == 2)
    {
      store::Item_t collationItem;
      consumeNext(collationItem, theChildren[1], planState);
      collator = theSctx->get_collator(collationItem->getStringValue().str(), loc);
    }
    else
    {
      collator = theSctx->get_default_collator(loc);
    }

    if (consumeNext(runningItem, theChildren[0], planState))
    {
      do
      {
        // casting of untyped atomic
        store::SchemaTypeCode runningType = runningItem->getTypeCode();

        if (runningType == store::XS_UNTYPED_ATOMIC)
        {
          GenericCast::castToBuiltinAtomic(runningItem,
                                           runningItem,
                                           store::XS_DOUBLE,
                                           NULL,
                                           loc);
          runningType = store::XS_DOUBLE;
        }

        // implementation dependent: return the first occurence)
        if (runningItem->isNaN())
        {
          // It must be checked if the sequence contains any
          // xs:double("NaN") [xs:double("NaN") is returned] or
          // only xs:float("NaN")'s [xs:float("NaN") is returned]'.

          result = runningItem;
          if (TypeOps::is_subtype(runningType, store::XS_DOUBLE))
            break;

          maxType = runningType;
        }

        if (result != 0)
        {
          // Type Promotion
          store::Item_t lItemCur;
          if (!GenericCast::promote(lItemCur, runningItem, maxType, NULL, tm, loc))
          {
            if (GenericCast::promote(lItemCur, result, runningType, NULL, tm, loc))
            {
              result.transfer(lItemCur);
              maxType = result->getTypeCode();
            }
            else
            {
              RAISE_ERROR(err::FORG0006, loc,
						  ERROR_PARAMS(ZED(PromotionImpossible)));
            }
          }
          else
          {
            runningItem.transfer(lItemCur);
            runningType = runningItem->getTypeCode();
          }
 
          store::Item_t current_copy(runningItem);
          store::Item_t max_copy(result);
          if (CompareIterator::valueComparison(loc,
                                               current_copy,
                                               max_copy,
                                               theCompareType,
                                               tm,
                                               timezone,
                                               collator))
          {
            maxType = runningType;
            result.transfer(runningItem);
          }
        }
        else
        {
          maxType = runningType;
          result.transfer(runningItem);
        }

        elems_in_seq++;
      }
      while (consumeNext(runningItem, theChildren[0], planState));

    if (elems_in_seq == 1)
    {
      //check type compatibility
      store::Item_t dummy1(result);
      store::Item_t dummy2(result);
      CompareIterator::valueComparison(loc,
                                       dummy1,
                                       dummy2,
                                       theCompareType,
                                       tm,
                                       timezone,
                                       collator);
    }

    STACK_PUSH(result != NULL, state);
  } // if non-empty seq

  STACK_END(state);

  }
  catch(ZorbaException &e)
  {
    if(e.diagnostic() == err::XPTY0004)
			e.set_diagnostic(err::FORG0006);

		throw;
  }
}



/////////////////////////////////////////////////////////////////////////////////
//                                                                             //
//  15.5 Functions and Operators that Generate Sequences                       //
//                                                                             //
/////////////////////////////////////////////////////////////////////////////////


/*******************************************************************************
  14.5.2 fn:id
********************************************************************************/
void FnIdIteratorState::init(PlanState& planState)
{
  DescendantAxisState::init(planState);
  theIsInitialized = false;
  theAttrsIte = GENV_ITERATOR_FACTORY->createAttributesIterator();
  assert(theIds.empty());
  assert(theDocNode == NULL);
}


void FnIdIteratorState::reset(PlanState& planState)
{
  DescendantAxisState::reset(planState);
  theIsInitialized = false;
  theIds.clear();
  theDocNode = NULL;
}

bool FnIdIterator::nextImpl(store::Item_t& result, PlanState& planState) const
{
  store::Item_t  id;
  store::Item*   child = 0;
  store::Item*   attr = 0;
  store::Item_t  tmp;
  bool           isMatchingId;
  ulong i;

  FnIdIteratorState *state;
  DEFAULT_STACK_INIT(FnIdIteratorState, state, planState);

  if (!state->theIsInitialized)
  {
    while (consumeNext(id, theChildren[0], planState))
    {
      state->theIds.push_back(id->getStringValue());
    }

    if(!consumeNext(state->theDocNode, theChildren[1], planState))
    {
			throw XQUERY_EXCEPTION(
				err::FODC0001, ERROR_PARAMS( "fn:id" ), ERROR_LOC( loc )
			);
    }

    while (state->theDocNode->getParent() != NULL)
    {
      state->theDocNode = state->theDocNode->getParent();
    }

    if (state->theDocNode->getNodeKind() != store::StoreConsts::documentNode)
    {
			throw XQUERY_EXCEPTION(
				err::FODC0001, ERROR_PARAMS( "fn:id" ), ERROR_LOC( loc )
			);
    }

    state->push(state->theDocNode);

    state->theIsInitialized = true;
  }

	while (true)
  {
    while ((child = state->top()->next()) == NULL)
    {
      state->pop();
      if (state->empty())
        break;
    }

    if (child == NULL)
      break;

    if (child->getNodeKind() != store::StoreConsts::elementNode)
      continue;

    tmp = child;
    state->push(tmp);

    isMatchingId = false;

    if (child->isId())
    {
      for (i = 0; i < state->theIds.size(); ++i)
      {
        if (child->getStringValue() == state->theIds[i])
        {
          result = child;
          STACK_PUSH(true, state);
          isMatchingId = true;
          break;
        }
      }
    }
    else
    {
      tmp = child;
      state->theAttrsIte->init(tmp);
      state->theAttrsIte->open();

      attr = state->theAttrsIte->next();

      while (attr != NULL)
      {
        if (attr->isId())
        {
          for (i = 0; i < state->theIds.size(); ++i)
          {
            if (attr->getStringValue() == state->theIds[i])
            {
              result = child;
              STACK_PUSH(true, state);
              isMatchingId = true;
              break;
            }
          }
        }

        if (isMatchingId)
          break;

        attr = state->theAttrsIte->next();
      }

      state->theAttrsIte->close();
    }
  }

  STACK_END (state);
}

/*******************************************************************************
  14.5.3 fn:element-with-id
********************************************************************************/
void FnElementWithIdIteratorState::init(PlanState& planState)
{
  DescendantAxisState::init(planState);
  theIsInitialized = false;
  theAttrsIte = GENV_ITERATOR_FACTORY->createAttributesIterator();
  assert(theIds.empty());
  assert(theDocNode == NULL);
}


void FnElementWithIdIteratorState::reset(PlanState& planState)
{
  DescendantAxisState::reset(planState);
  theIsInitialized = false;
  theIds.clear();
  theDocNode = NULL;
}

bool FnElementWithIdIterator::nextImpl(store::Item_t& result, PlanState& planState) const
{
  store::Item_t  id;
  store::Item*   child = 0;
  store::Item*   attr = 0;
  store::Item_t  tmp;
  bool           isMatchingId;
  ulong i;

  FnElementWithIdIteratorState* state;
  DEFAULT_STACK_INIT(FnElementWithIdIteratorState, state, planState);

  if (!state->theIsInitialized)
  {
    while (consumeNext(id, theChildren[0], planState))
    {
      state->theIds.push_back(id->getStringValue());
    }

    if(!consumeNext(state->theDocNode, theChildren[1], planState))
    {
			throw XQUERY_EXCEPTION(
				err::FODC0001, ERROR_PARAMS( "fn:id" ), ERROR_LOC( loc )
			);
    }

    while (state->theDocNode->getParent() != NULL)
    {
      state->theDocNode = state->theDocNode->getParent();
    }

    if (state->theDocNode->getNodeKind() != store::StoreConsts::documentNode)
    {
			throw XQUERY_EXCEPTION(
				err::FODC0001, ERROR_PARAMS( "fn:id" ), ERROR_LOC( loc )
			);
    }

    state->push(state->theDocNode);

    state->theIsInitialized = true;
  }

  while (true)
  {
    while ((child = state->top()->next()) == NULL)
    {
      state->pop();
      if (state->empty())
        break;
    }

    if (child == NULL)
      break;

    if (child->getNodeKind() != store::StoreConsts::elementNode)
      continue;

    tmp = child;
    state->push(tmp);

    isMatchingId = false;

    if (child->isId())
    {
      for (i = 0; i < state->theIds.size(); ++i)
      {
        if (child->getStringValue() == state->theIds[i])
        {
          result = child->getParent();
          if(result) 
          {
            STACK_PUSH(true, state);
            isMatchingId = true;
          }
          break;
        }
      }
    }
    else
    {
      tmp = child;
      state->theAttrsIte->init(tmp);
      state->theAttrsIte->open();

      attr = state->theAttrsIte->next();

      while (attr != NULL)
      {
        if (attr->isId())
        {
          for (i = 0; i < state->theIds.size(); ++i)
          {
            if (attr->getStringValue() == state->theIds[i])
            {
              result = child;
              STACK_PUSH(true, state);
              isMatchingId = true;
              break;
            }
          }
        }

        if (isMatchingId)
          break;

        attr = state->theAttrsIte->next();
      }

      state->theAttrsIte->close();
    }
  }

  STACK_END (state);
}


/*******************************************************************************
  14.5.4 fn:idref
********************************************************************************/
void FnIdRefIteratorState::init(PlanState& planState)
{
  DescendantAxisState::init(planState);
  theIsInitialized = false;
  theAttrsIte = GENV_ITERATOR_FACTORY->createAttributesIterator();
  assert(theIds.empty());
  assert(theDocNode == NULL);
}


void FnIdRefIteratorState::reset(PlanState& planState)
{
  DescendantAxisState::reset(planState);
  theIsInitialized = false;
  theIds.clear();
  theDocNode = NULL;
}

bool FnIdRefIterator::nextImpl(store::Item_t& result, PlanState& planState) const
{
  store::Item_t     id;
  store::Item*      child = NULL;
  store::Item*      attr = NULL;
  store::Item_t     tmp;
  store::Item_t     typedValue;
  store::Iterator_t typedValueIte;

  bool           isMatchingId;
  ulong          i;

  std::vector<zstring> idrefs;

  FnIdRefIteratorState* state;
  DEFAULT_STACK_INIT(FnIdRefIteratorState, state, planState);

  if (!state->theIsInitialized)
  {
    while (consumeNext(id, theChildren[0], planState))
    {
      state->theIds.push_back(id->getStringValue());
    }

    if(!consumeNext(state->theDocNode, theChildren[1], planState))
    {
			throw XQUERY_EXCEPTION(
				err::FODC0001, ERROR_PARAMS( "fn:idref" ), ERROR_LOC( loc )
			);
    }

    while (state->theDocNode->getParent() != NULL)
    {
      state->theDocNode = state->theDocNode->getParent();
    }

    if (state->theDocNode->getNodeKind() != store::StoreConsts::documentNode)
    {
			throw XQUERY_EXCEPTION(
				err::FODC0001, ERROR_PARAMS( "fn:idref" ), ERROR_LOC( loc )
			);
    }

    state->push(state->theDocNode);

    state->theIsInitialized = true;
  }


  while (true)
  {
    while ((child = state->top()->next()) == NULL)
    {
      state->pop();
      if (state->empty())
        break;
    }

    if (child == NULL)
      break;

    if (child->getNodeKind() != store::StoreConsts::elementNode)
      continue;

    tmp = child;
    state->push(tmp);

    if (child->isIdRefs())
    {
      isMatchingId = false;

      child->getTypedValue(typedValue, typedValueIte);

      for (i = 0; i < state->theIds.size(); ++i)
      {
        if (typedValue != NULL)
        {
          if (typedValue->getStringValue() == state->theIds[i])
          {
            result = child;
            STACK_PUSH(true, state);
            isMatchingId = true;
            break;
          }
        }
        else if (typedValueIte != NULL)
        {
          typedValueIte->open();

          while(typedValueIte->next(typedValue))
          {
            if (typedValue->getStringValue() == state->theIds[i])
            {
              isMatchingId = true;
              result = child;
              break;
            }
          }

          typedValueIte->close();

          if (isMatchingId)
          {
            STACK_PUSH(true, state);
            break;
          }
        }
      }
    }

    tmp = state->topNode();
    state->theAttrsIte->init(tmp);
    state->theAttrsIte->open();

    attr = state->theAttrsIte->next();

    while (attr != NULL)
    {
      if (attr->isIdRefs())
      {
        isMatchingId = false;

        attr->getTypedValue(typedValue, typedValueIte);

        for (i = 0; i < state->theIds.size(); ++i)
        {
          if (typedValue != NULL)
          {
            if (typedValue->getStringValue() == state->theIds[i])
            {
              result = attr;
              STACK_PUSH(true, state);
              isMatchingId = true;
              break;
            }
          }
          else if (typedValueIte != NULL)
          {
            typedValueIte->open();

            while(typedValueIte->next(typedValue))
            {
              if (typedValue->getStringValue() == state->theIds[i])
              {
                isMatchingId = true;
                result = attr;
                break;
              }
            }

            typedValueIte->close();

            if (isMatchingId)
            {
              STACK_PUSH(true, state);
              break;
            }
          }
        }
      }

      attr = state->theAttrsIte->next();
    }

    state->theAttrsIte->close();
  }

  STACK_END (state);
}


} // namespace zorba
/* vim:set et sw=2 ts=2: */
