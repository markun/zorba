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
#pragma once
#ifndef ZORBA_TYPES_TYPEOPS_H
#define ZORBA_TYPES_TYPEOPS_H

#include <string>
#include <zorba/config.h>
#include <zorba/api_shared_types.h>
#include <zorba/typeident.h>

#include "common/shared_types.h"
#include "types/typeconstants.h"

#include "compiler/parser/query_loc.h"

#include <zorba/store_consts.h>
#include "store/api/item.h"

namespace zorba 
{

class TypeOps 
{
public:
  /**
   * Return true is q1 is a sub-quantifier of q2 (see QUANT_SUBTYPE_MATRIX
   * in root_typemanger.h).
   */
  static bool is_sub_quant(
        SequenceType::Quantifier q1,
        SequenceType::Quantifier q2);

  /**
   * Return the "intersection" of the 2 given quantifiers (see QUANT_INTERS_MATRIX
   * in root_typemanger.h).
   */
  static SequenceType::Quantifier intersect_quant(
        SequenceType::Quantifier,
        SequenceType::Quantifier);

  /**
   * Return the "union" of the 2 given quantifiers (see QUANT_UNION_MATRIX
   * in root_typemanger.h).
   */
  static SequenceType::Quantifier union_quant(
        SequenceType::Quantifier,
        SequenceType::Quantifier);

  /*
   * Returns the atomic_type_code_t for a given type, which is assumed to be
   * a quantified builtin atomic type.
   */
  static store::SchemaTypeCode get_atomic_type_code(const XQType& type);

  /**
   * Return true is the given type is among the known types of the given type mgr
   */
  static bool is_in_scope(const TypeManager* tm, const XQType& type);

  /**
   * Returns true is the given sequence type is a subtype of an atomic builtin
   * numeric type (xs:decimal, xs:double, or xs:float)
   */
  static bool is_numeric(const TypeManager* tm, const XQType& type);

  /**
   * Returns true is the given sequence type is a subtype of an atomic builtin
   * numeric type (xs:decimal, xs:double, or xs:float)
   */
  static bool is_numeric(store::SchemaTypeCode type);
 
  /**
   * Returns true is the given sequence type is a subtype of an atomic builtin
   * numeric type (xs:decimal?, xs:double?, or xs:float?) or of xs:untypedAtomic?
   */
  static bool is_numeric_or_untyped(const TypeManager* tm, const XQType& type);
  
  /**
   * Returns true is the given type could be a date / time type.
   */
  static bool maybe_date_time(const TypeManager* tm, const XQType& type);

  /*
   * Returns the prime type (ItemType) of the given type.
   */
  static xqtref_t prime_type(const TypeManager* tm, const XQType& type);

  /*
   * Returns true iff type1 is equal to type2 including the quantifier,
   * false otherwise.
   */
  static bool is_equal(
        const TypeManager* tm,
        const XQType& type1,
        const XQType& type2,
        const QueryLoc& loc = QueryLoc::null);

  /*
   * Returns true if _subtype_ is a subtype of _supertype_, false otherwise.
   */
  static bool is_subtype(
        store::SchemaTypeCode subtype,
        store::SchemaTypeCode supertype);

  /*
   * Returns true if _subtype_ is a subtype of _supertype_, false otherwise.
   */
  static bool is_subtype(
        const TypeManager* tm,
        const XQType& subtype,
        const XQType& supertype,
        const QueryLoc& loc = QueryLoc::null);

  /*
   * Returns true if the data type of the given item is a subtype of the
   * given supertype; otherwise, returns false.
   */
  static bool is_subtype(
      const TypeManager* tm,
      const store::Item* subitem,
      const XQType& supertype,
      const QueryLoc& loc);

  /**
   * Returns the castability fron the source ItemType to the target ItemType. It
   * works only if both source and target types are builtin atomic types.
   * Otherwise, it returns NOT_CASTABLE.
   */
  static TypeConstants::castable_t castability(
        const XQType& src,
        const XQType& target);

  /*
   * Computes the union type (type1 | type2), of the two types. The output _u_ 
   * of this call satisfies the following invariant:
   *
   * is_subtype(_type1_, _u_) == true && is_subtype(_type2_, _u_) == true
   */
  static xqtref_t union_type(
        const XQType& type1,
        const XQType& type2,
        const TypeManager* manager);

  /*
   * Computes the intersection of the two types. The output _u_ of this call
   * satisfies the following invariant: 
   *
   * there exists some type(not necessarily representable in this TypeSystem),
   * _t_ such that:
   * is_subtype(_t_, _type1_) == true &&
   * is_subtype(_t_, _type2_) == true &&
   * is_subtype(_t_, _u_) == true.
   *
   * Informally, the returned type is a supertype (not necessarily a perfect
   * supertype) of the actual intersection type.
   */
  static xqtref_t intersect_type(
        const XQType& type1,
        const XQType& type2,
        const TypeManager* manager);

  /*
   * Returns the type to be used for numeric arithmetic ops.
   */
  static xqtref_t arithmetic_type(
        const TypeManager* tm,
        const XQType& type1,
        const XQType& type2,
        bool division);

  static store::SchemaTypeCode arithmetic_type(
    store::SchemaTypeCode type1,
    store::SchemaTypeCode type2,
    bool division);

  /*
   * Writes a textual representation of the given type to the output stream.
   */
  static std::ostream& serialize(std::ostream& os, const XQType& type);
  
  /*
   * Returns a string with a textual representation of the given quantifier.
   */
  static const char* decode_quantifier(SequenceType::Quantifier quant);
};

}

#endif /* ZORBA_TYPEOPS_H */
/*
 * Local variables:
 * mode: c++
 * End:
 */
/* vim:set et sw=2 ts=2: */
