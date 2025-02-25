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

#ifndef ZORBA_TRANSCODE_STREAMBUF_H
#define ZORBA_TRANSCODE_STREAMBUF_H

#include <zorba/config.h>
#include <zorba/util/transcode_stream.h>

///////////////////////////////////////////////////////////////////////////////

#ifdef ZORBA_NO_ICU
# include "passthru_streambuf.h"
#else
# include "icu_streambuf.h"
#endif /* ZORBA_NO_ICU */

namespace zorba {
namespace internal {
namespace transcode {

#ifdef ZORBA_NO_ICU
typedef zorba::passthru_streambuf streambuf;
#else
typedef icu_streambuf streambuf;
#endif /* ZORBA_NO_ICU */

} // namespace transcode
} // namespace internal
} // namespace zorba

///////////////////////////////////////////////////////////////////////////////

#endif  /* ZORBA_TRANSCODE_STREAMBUF_H */
/* vim:set et sw=2 ts=2: */
