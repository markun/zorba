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

#define USE_JSON_CAST 1

#include "stdafx.h"

#include <algorithm>
#include <iterator>
#include <set>

#include <zorba/config.h>
#include <zorba/internal/cxx_util.h>
#include <zorba/diagnostic_list.h>
#include <zorba/store_consts.h>

#include "runtime/csv/csv.h"
#include "store/api/item_factory.h"
#include "system/globalenv.h"
#include "types/casting.h"
#include "types/root_typemanager.h"
#include "types/typeops.h"
#include "util/ascii_util.h"
#include "util/stl_util.h"

#if USE_JSON_CAST
#include <sstream>
#include "util/json_parser.h"
#include "zorbatypes/decimal.h"
#include "zorbatypes/float.h"
#include "zorbatypes/integer.h"
#endif /* USE_JSON_CAST */

#include "csv_util.h"

using namespace std;

namespace zorba {

///////////////////////////////////////////////////////////////////////////////

#define CAST_TO(STRING,TYPE,RESULT) \
  GenericCast::castStringToAtomic( RESULT, STRING, GENV_TYPESYSTEM.TYPE##_TYPE_ONE.getp(), &GENV_TYPESYSTEM, nullptr, QueryLoc::null, false )

#define IS_ATOMIC_TYPE(ITEM,TYPE) \
    ( (ITEM)->isAtomic() && TypeOps::is_subtype( (ITEM)->getTypeCode(), store::TYPE ) )

static bool get_option( store::Item_t const &object, char const *opt_name,
                        store::Item_t *result ) {
  store::Item_t key_item;
  zstring s( opt_name );
  GENV_ITEMFACTORY->createString( key_item, s );
  *result = object->getObjectValue( key_item );
  return !result->isNull();
}

static bool get_boolean_option( store::Item_t const &object,
                                char const *opt_name, bool *result,
                                QueryLoc const &loc ) {
  store::Item_t opt_item;
  if ( get_option( object, opt_name, &opt_item ) ) {
    if ( !IS_ATOMIC_TYPE( opt_item, XS_BOOLEAN ) )
      throw XQUERY_EXCEPTION(
        zerr::ZCSV0001_INVALID_OPTION,
        ERROR_PARAMS(
          opt_item->getStringValue(), opt_name, ZED( ZCSV0001_MustBeBoolean )
        ),
        ERROR_LOC( loc )
      );
    *result = opt_item->getBooleanValue();
    return true;
  }
  return false;
}

static bool get_char_option( store::Item_t const &object,
                             char const *opt_name, char *result,
                             QueryLoc const &loc ) {
  store::Item_t opt_item;
  if ( get_option( object, opt_name, &opt_item ) ) {
    zstring const value( opt_item->getStringValue() );
    if ( !IS_ATOMIC_TYPE( opt_item, XS_STRING ) ||
         value.size() != 1 || !ascii::is_ascii( value[0] ) ) {
      throw XQUERY_EXCEPTION(
        zerr::ZCSV0001_INVALID_OPTION,
        ERROR_PARAMS( value, opt_name, ZED( ZCSV0001_MustBeASCIIChar ) ),
        ERROR_LOC( loc )
      );
    }
    *result = value[0];
    return true;
  }
  return false;
}

static bool get_string_option( store::Item_t const &object,
                               char const *opt_name, zstring *result,
                               QueryLoc const &loc ) {
  store::Item_t opt_item;
  if ( get_option( object, opt_name, &opt_item ) ) {
    if ( !IS_ATOMIC_TYPE( opt_item, XS_STRING ) )
      throw XQUERY_EXCEPTION(
        zerr::ZCSV0001_INVALID_OPTION,
        ERROR_PARAMS(
          opt_item->getStringValue(), opt_name, ZED( ZCSV0001_MustBeString )
        ),
        ERROR_LOC( loc )
      );
    opt_item->getStringValue2( *result );
    return true;
  }
  return false;
}

static json::type parse_json( zstring const &s, json::token *ptoken ) {
  mem_streambuf buf( (char*)s.data(), s.size() );
  istringstream iss;
  iss.ios::rdbuf( &buf );
  json::lexer lex( iss );
  try {
    if ( lex.next( ptoken ) )
      return json::map_type( ptoken->get_type() );
  }
  catch ( json::exception const& ) {
    // ignore
  }
  return json::none;
}

///////////////////////////////////////////////////////////////////////////////

bool CsvParseIterator::count( store::Item_t &result,
                              PlanState &plan_state ) const {
  unsigned long count;
  store::Item_t item;

  CsvParseIteratorState *state;
  DEFAULT_STACK_INIT( CsvParseIteratorState, state, plan_state );

  // $csv as string
  consumeNext( item, theChildren[0], plan_state );
  if ( item->isStreamable() ) {
    istream_iterator<char> begin( item->getStream() );
    istream_iterator<char> const end;
    count = std::count( begin, end, '\n' );
  } else {
    zstring const s( item->getStringValue() );
    count = std::count( s.begin(), s.end(), '\n' );
  }

  STACK_PUSH(
    GENV_ITEMFACTORY->createInteger( result, xs_integer( count ) ),
    state
  );
  STACK_END( state );
}

bool CsvParseIterator::nextImpl( store::Item_t &result,
                                 PlanState &plan_state ) const {
  char char_opt;
  unsigned field_no = 0;
  store::Item_t item, opt_item;
  vector<store::Item_t> keys_copy, values;
  set<unsigned> keys_omit;
  zstring value;
  bool eol, quoted, swap_keys = false;

  CsvParseIteratorState *state;
  DEFAULT_STACK_INIT( CsvParseIteratorState, state, plan_state );

  // $csv as string
  consumeNext( item, theChildren[0], plan_state );
  if ( item->isStreamable() )
    state->csv_.set_stream( item->getStream() );
  else {
    item->getStringValue2( state->string_ );
    state->mem_streambuf_.set( state->string_.data(), state->string_.size() );
    state->iss_.ios::rdbuf( &state->mem_streambuf_ );
    state->csv_.set_stream( state->iss_ );
  }

  // $options as object()
  consumeNext( item, theChildren[1], plan_state );
  if ( !get_boolean_option( item, "cast-unquoted-values", &state->cast_unquoted_, loc ) )
    state->cast_unquoted_ = true;
  if ( get_option( item, "extra-name", &opt_item ) )
    opt_item->getStringValue2( state->extra_name_ );
  if ( get_option( item, "field-names", &opt_item ) ) {
    store::Iterator_t i( opt_item->getArrayValues() );
    i->open();
    store::Item_t name_item;
    while ( i->next( name_item ) )
      state->keys_.push_back( name_item );
    i->close();
  }
  if ( get_option( item, "missing-value", &opt_item ) ) {
    opt_item->getStringValue2( value );
    if ( value == "error" )
      state->missing_ = missing::error;
    else if ( value == "omit" )
      state->missing_ = missing::omit;
    else if ( value == "null" )
      state->missing_ = missing::null;
    else
      ZORBA_ASSERT( false );            // should be caught by JSON schema
  } else
    state->missing_ = missing::null;
  if ( get_char_option( item, "quote-char", &char_opt, loc ) ) {
    state->csv_.set_quote( char_opt );
    state->csv_.set_quote_esc( char_opt );
  }
  if ( get_char_option( item, "quote-escape", &char_opt, loc ) )
    state->csv_.set_quote_esc( char_opt );
  if ( get_char_option( item, "separator", &char_opt, loc ) )
    state->csv_.set_separator( char_opt );

  state->line_no_ = 1;

  while ( state->csv_.next_value( &value, &eol, &quoted ) ) {
    if ( state->keys_.size() && values.size() == state->keys_.size() &&
         state->extra_name_.empty() ) {
      //
      // We've already max'd out on the number of values for a record and the
      // "extra-name" option wasn't specified.
      //
      throw XQUERY_EXCEPTION(
        zerr::ZCSV0003_EXTRA_VALUE,
        ERROR_PARAMS( value, state->line_no_ ),
        ERROR_LOC( loc )
      );
    }

    item = nullptr;
    if ( value.empty() ) {
      if ( state->keys_.empty() ) {
        //
        // Header field names can never be empty.
        //
        throw XQUERY_EXCEPTION(
          zerr::ZCSV0002_MISSING_VALUE,
          ERROR_PARAMS( ZED( ZCSV0002_EmptyHeaderValue ) ),
          ERROR_LOC( loc )
        );
      }
      if ( quoted )
        GENV_ITEMFACTORY->createString( item, value );
      else
        switch ( state->missing_ ) {
          case missing::error:
            goto missing_error;
          case missing::null:
            GENV_ITEMFACTORY->createJSONNull( item );
            break;
          case missing::omit:
            keys_omit.insert( field_no );
            break;
        }
    } else if ( state->cast_unquoted_ && !quoted && !state->keys_.empty() ) {
#if USE_JSON_CAST
      if ( value == "T" || value == "Y" )
        GENV_ITEMFACTORY->createBoolean( item, true );
      else if ( value == "F" || value == "N" )
        GENV_ITEMFACTORY->createBoolean( item, false );
      else {
        json::token t;
        switch ( parse_json( value, &t ) ) {
          case json::boolean:
            GENV_ITEMFACTORY->createBoolean( item, value[0] == 't' );
            break;
          case json::null:
            GENV_ITEMFACTORY->createJSONNull( item );
            break;
          case json::number:
            switch ( t.get_numeric_type() ) {
              case json::token::integer:
                GENV_ITEMFACTORY->createInteger( item, xs_integer( value ) );
                break;
              case json::token::decimal:
                GENV_ITEMFACTORY->createDecimal( item, xs_decimal( value ) );
                break;
              case json::token::floating_point:
                GENV_ITEMFACTORY->createDouble( item, xs_double( value ) );
                break;
              default:
                ZORBA_ASSERT( false );
            }
            break;
          default:
            GENV_ITEMFACTORY->createString( item, value );
        } // switch
      } // else
#else
      if ( value == "null" )
        GENV_ITEMFACTORY->createJSONNull( item );
      else if ( value == "T" || value == "Y" )
        GENV_ITEMFACTORY->createBoolean( item, true );
      else if ( value == "F" || value == "N" )
        GENV_ITEMFACTORY->createBoolean( item, false );
      else
        if ( !CAST_TO( value, INTEGER, item ) &&
             !CAST_TO( value, DECIMAL, item ) &&
             !CAST_TO( value, DOUBLE , item ) &&
             !CAST_TO( value, BOOLEAN, item ) ) {
          GENV_ITEMFACTORY->createString( item, value );
        }
#endif /* USE_JSON_CAST */
    } else {
      GENV_ITEMFACTORY->createString( item, value );
    }

    if ( !item.isNull() )
      values.push_back( item );

    if ( eol ) {
      if ( state->keys_.empty() ) {
        //
        // The first line of values are taken to be the header field names.
        //
        state->keys_.swap( values );
      } else {
        if ( values.size() < state->keys_.size() ) {
          //
          // At least one value is missing.
          //
          switch ( state->missing_ ) {
            case missing::error:
              //
              // We don't actually know which field is missing; we know only
              // that there's at least one less field than there should be.
              //
              field_no = values.size();
              goto missing_error;
            case missing::null:
              GENV_ITEMFACTORY->createJSONNull( item );
              while ( values.size() < state->keys_.size() )
                values.push_back( item );
              break;
            case missing::omit:
              //
              // We have to remove the keys for those fields that should be
              // omitted temporarily.
              //
              if ( keys_omit.empty() ) {
                //
                // The last field is the one that's missing and there's no
                // trailing ',' (which is why keys_omit is empty).
                //
                keys_copy = state->keys_;
                state->keys_.pop_back();
              } else {
                for ( unsigned i = 0; i < state->keys_.size(); ++i )
                  if ( !ztd::contains( keys_omit, i ) )
                    keys_copy.push_back( state->keys_[i] );
                keys_copy.swap( state->keys_ );
              }
              swap_keys = true;
              break;
          }
        } else if ( values.size() > state->keys_.size() ) {
          //
          // There's at least one extra value: add in extra fields for keys
          // temporarily.
          //
          keys_copy = state->keys_;
          zstring::size_type const num_pos =
            state->extra_name_.find_first_of( '#' );
          for ( unsigned f = state->keys_.size() +1; f <= values.size(); ++f ) {
            ascii::itoa_buf_type buf;
            ascii::itoa( f, buf );
            zstring extra_name( state->extra_name_ );
            if ( num_pos != zstring::npos )
              extra_name.replace( num_pos, 1, buf );
            else
              extra_name += buf;
            GENV_ITEMFACTORY->createString( item, extra_name );
            state->keys_.push_back( item );
          }
          swap_keys = true;
        }

        GENV_ITEMFACTORY->createJSONObject( result, state->keys_, values );
        if ( swap_keys ) {
          //
          // Put the original set of field names (keys) back the way it was.
          //
          keys_copy.swap( state->keys_ );
        }
        STACK_PUSH( true, state );
      } // else
      ++state->line_no_, field_no = 0;
      continue;
    } // if ( eol )
    ++field_no;
  } // while

  STACK_END( state );

missing_error:
  throw XQUERY_EXCEPTION(
    zerr::ZCSV0002_MISSING_VALUE,
    ERROR_PARAMS(
      ZED( ZCSV0002_MissingValue ),
      state->keys_[ field_no ]->getStringValue(),
      state->line_no_
    ),
    ERROR_LOC( loc )
  );
}

///////////////////////////////////////////////////////////////////////////////

bool CsvSerializeIterator::nextImpl( store::Item_t &result,
                                     PlanState &plan_state ) const {
  bool do_header, separator;
  store::Item_t item, opt_item;
  zstring line, value;

  CsvSerializeIteratorState *state;
  DEFAULT_STACK_INIT( CsvSerializeIteratorState, state, plan_state );

  // $options as object()
  consumeNext( item, theChildren[1], plan_state );
  if ( get_option( item, "field-names", &opt_item ) ) {
    store::Iterator_t i( opt_item->getArrayValues() );
    i->open();
    store::Item_t name_item;
    while ( i->next( name_item ) )
      state->keys_.push_back( name_item );
    i->close();
  }
  if ( !get_boolean_option( item, "serialize-header", &do_header, loc ) )
    do_header = true;
  if ( !get_char_option( item, "separator", &state->separator_, loc ) )
    state->separator_ = ',';
  if ( !get_string_option( item, "serialize-null-as", &state->null_string_, loc ) )
    state->null_string_ = "null";
  if ( get_option( item, "serialize-boolean-as", &opt_item ) ) {
    if ( !get_string_option( opt_item, "false", &state->boolean_string_[0], loc ) ||
         !get_string_option( opt_item, "true", &state->boolean_string_[1], loc ) )
      throw XQUERY_EXCEPTION(
        zerr::ZCSV0001_INVALID_OPTION,
        ERROR_PARAMS(
          "", "serialize-boolea-as",
          ZED( ZCSV0001_MustBeTrueFalse )
        ),
        ERROR_LOC( loc )
      );
  } else
    state->boolean_string_[0] = "false", state->boolean_string_[1] = "true";

  if ( state->keys_.empty() ) {
    //
    // We have to take the header field names from the first object, but we
    // have to save the first object to return its values.
    //
    if ( consumeNext( state->header_item_, theChildren[0], plan_state ) ) {
      store::Iterator_t i( state->header_item_->getObjectKeys() );
      i->open();
      while ( i->next( item ) )
        state->keys_.push_back( item );
      i->close();
    }
  }

  if ( do_header ) {
    separator = false;
    line.clear();
    FOR_EACH( vector<store::Item_t>, key, state->keys_ ) {
      if ( separator )
        line += state->separator_;
      else
        separator = true;
      line += (*key)->getStringValue();
    }
    GENV_ITEMFACTORY->createString( result, line );
    STACK_PUSH( true, state );
    item = state->header_item_;
    goto skip_while;
  }

  while ( consumeNext( item, theChildren[0], plan_state ) ) {
skip_while:
    line.clear();
    separator = false;
    FOR_EACH( vector<store::Item_t>, key, state->keys_ ) {
      if ( separator )
        line += state->separator_;
      else
        separator = true;
      store::Item_t const value_item( item->getObjectValue( *key ) );
      if ( !value_item.isNull() ) {
        if ( IS_ATOMIC_TYPE( value_item, XS_BOOLEAN ) )
          line += state->boolean_string_[ value_item->getBooleanValue() ];
        else if ( IS_ATOMIC_TYPE( item, JS_NULL ) )
          line += state->null_string_;
        else
          line += value_item->getStringValue();
      }
    }
    GENV_ITEMFACTORY->createString( result, line );
    STACK_PUSH( true, state );
  } // while

  STACK_END( state );
}

///////////////////////////////////////////////////////////////////////////////

} // namespace zorba

/* vim:set et sw=2 ts=2: */
