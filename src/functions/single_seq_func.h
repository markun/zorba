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
#ifndef ZORBA_FUNCTIONS_SINGLE_SEQ_FUNC
#define ZORBA_FUNCTIONS_SINGLE_SEQ_FUNC

#include "functions/function_impl.h"

namespace zorba 
{

/*******************************************************************************
  A function that processes a single input sequence.

  Assumptions:
  - Return type is the same as type of the input expr.
  - Preserves sorted/disctinct node annotations from children.
  - Propagates IGNORES_* properties downward.
  - Only propagates sequence arg to output.

  Note: The function may have more than 1 params, but only one its params is
  the "real" input; the others are like "config" params that affect how the
  function processes its real input.

  theInput : The position of the function's "real" input among the function's
             params. 
*******************************************************************************/
class single_seq_function : public function 
{
protected:
  unsigned theInput;
  
public:
  single_seq_function(
        const signature& sig,
        FunctionConsts::FunctionKind kind,
        unsigned src = 0)
    :
    function(sig, kind),
    theInput(src)
  {
  }

  virtual xqtref_t getReturnType(
        const TypeManager* tm,
        const std::vector<xqtref_t>& arg_types) const;

  virtual FunctionConsts::AnnotationValue producesSortedNodes() const
  {
    return FunctionConsts::PRESERVE;
  }

  virtual FunctionConsts::AnnotationValue producesDuplicateNodes() const
  {
    return FunctionConsts::PRESERVE;
  }

  COMPUTE_ANNOTATION_DECL();
};


}

#endif

/*
 * Local variables:
 * mode: c++
 * End:
 */
