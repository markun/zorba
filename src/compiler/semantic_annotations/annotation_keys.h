#ifndef ZORBA_ANNOTATION_KEYS_H
#define ZORBA_ANNOTATION_KEYS_H

namespace zorba {

namespace AnnotationKey {

enum {
  IGNORES_SORTED_NODES,
  IGNORES_DUP_NODES,
  PRODUCES_SORTED_NODES,
  PRODUCES_DISTINCT_NODES,
  
  FREE_VARS,

  EXPENSIVE_OP,
  UNFOLDABLE_OP,
  
  MAX_ANNOTATION
};

}

}

#endif  // ZORBA_ANNOTATION_KEYS_H

/*
 * Local variables:
 * mode: c++
 * End:
 */
