/*
 * Copyright 2006-2009 The FLWOR Foundation.
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

#include <zorba/item.h>
#include <zorba/options.h>
#include <zorba/singleton_item_sequence.h>
#include <zorba/diagnostic_handler.h>

#include <diagnostics/assert.h>
#include <api/zorbaimpl.h>
#include <api/unmarshaller.h>

#include "serializerimpl.h"

namespace zorba {

Serializer_t
Serializer::createSerializer(const Zorba_SerializerOptions_t& aOptions)
{
  return new SerializerImpl(aOptions);
}

Serializer_t
Serializer::createSerializer(ItemSequence* aOptions)
{
  return new SerializerImpl(aOptions);
}

SerializerImpl::SerializerImpl(const Zorba_SerializerOptions_t& aOptions, DiagnosticHandler* aDiagnosticHandler)
  : theDiagnosticHandler(aDiagnosticHandler),
    theInternalSerializer(&theXQueryDiagnostics)
{
  setSerializationParameters(theInternalSerializer, aOptions);
  own_error_handler = false;
  if (!theDiagnosticHandler) {
    theDiagnosticHandler = new DiagnosticHandler();
    own_error_handler = true;
  }
}

SerializerImpl::SerializerImpl(ItemSequence* aOptions, DiagnosticHandler* aDiagnosticHandler)
  : theDiagnosticHandler(aDiagnosticHandler),
    theInternalSerializer(&theXQueryDiagnostics)
{
  setSerializationParameters(theInternalSerializer, aOptions);
  own_error_handler = false;
  if (!theDiagnosticHandler) {
    theDiagnosticHandler = new DiagnosticHandler();
    own_error_handler = true;
  }
}

SerializerImpl::~SerializerImpl() 
{
  if(own_error_handler)
    delete theDiagnosticHandler;
}


void
SerializerImpl::serialize(ItemSequence* aObject, std::ostream& aOs) const
{
  try {
    Iterator_t  object_iter = aObject->getIterator();
    object_iter->open();
    theInternalSerializer.serialize(Unmarshaller::getInternalIterator(object_iter.get()), aOs);
    object_iter->close();
  } catch (ZorbaException const &e) {
    ZorbaImpl::notifyError(theDiagnosticHandler, e);
  }
}

int
SerializerImpl::getSerializationMethod() const
{
  switch (theInternalSerializer.getSerializationMethod())
  {
  case serializer::PARAMETER_VALUE_XML:
    return ZORBA_SERIALIZATION_METHOD_XML;
  case serializer::PARAMETER_VALUE_HTML:
    return ZORBA_SERIALIZATION_METHOD_HTML;
  case serializer::PARAMETER_VALUE_XHTML:
    return ZORBA_SERIALIZATION_METHOD_XHTML;
  case serializer::PARAMETER_VALUE_TEXT:
    return ZORBA_SERIALIZATION_METHOD_TEXT;
  case serializer::PARAMETER_VALUE_BINARY:
    return ZORBA_SERIALIZATION_METHOD_BINARY;
#ifdef ZORBA_WITH_JSON
  case serializer::PARAMETER_VALUE_JSON:
    return ZORBA_SERIALIZATION_METHOD_JSON;
  case serializer::PARAMETER_VALUE_JSONIQ:
    return ZORBA_SERIALIZATION_METHOD_JSONIQ;
#endif
  default:
    ZORBA_ASSERT(0);
  }
}

static void
convertSerializationMethod(
    serializer&            aInternalSerializer,
    const char*            aParameter,
    Zorba_serialization_method_t   aMethod)
{
  switch (aMethod)
  {
  case ZORBA_SERIALIZATION_METHOD_XML:
    aInternalSerializer.setParameter(aParameter, "xml"); break;
  case ZORBA_SERIALIZATION_METHOD_HTML:
    aInternalSerializer.setParameter(aParameter, "html"); break;
  case ZORBA_SERIALIZATION_METHOD_XHTML:
    aInternalSerializer.setParameter(aParameter, "xhtml"); break;
  case ZORBA_SERIALIZATION_METHOD_TEXT:
    aInternalSerializer.setParameter(aParameter, "text"); break;
  case ZORBA_SERIALIZATION_METHOD_BINARY:
    aInternalSerializer.setParameter(aParameter, "binary"); break;
#ifdef ZORBA_WITH_JSON
  case ZORBA_SERIALIZATION_METHOD_JSON:
    aInternalSerializer.setParameter(aParameter, "json"); break;
  case ZORBA_SERIALIZATION_METHOD_JSONIQ:
    aInternalSerializer.setParameter(aParameter, "jsoniq"); break;
#endif
  }
}

void
SerializerImpl::setSerializationParameters(
  serializer&                       aInternalSerializer,
  const Zorba_SerializerOptions_t&  aSerializerOptions)
{
  convertSerializationMethod(aInternalSerializer, "method",
                             aSerializerOptions.ser_method);

  switch (aSerializerOptions.byte_order_mark)
  {
  case ZORBA_BYTE_ORDER_MARK_YES:
    aInternalSerializer.setParameter("byte-order-mark", "yes"); break;
  case ZORBA_BYTE_ORDER_MARK_NO:
    aInternalSerializer.setParameter("byte-order-mark", "no"); break;
  }

  switch (aSerializerOptions.include_content_type)
  {
  case ZORBA_INCLUDE_CONTENT_TYPE_YES:
    aInternalSerializer.setParameter("include-content-type", "yes"); break;
  case ZORBA_INCLUDE_CONTENT_TYPE_NO:
    aInternalSerializer.setParameter("include-content-type", "no"); break;
  }

  switch (aSerializerOptions.indent)
  {
  case ZORBA_INDENT_YES:
    aInternalSerializer.setParameter("indent", "yes"); break;
  case ZORBA_INDENT_NO:
    aInternalSerializer.setParameter("indent", "no"); break;
  }

  switch (aSerializerOptions.omit_xml_declaration)
  {
  case ZORBA_OMIT_XML_DECLARATION_YES:
    aInternalSerializer.setParameter("omit-xml-declaration", "yes"); break;
  case ZORBA_OMIT_XML_DECLARATION_NO:
    aInternalSerializer.setParameter("omit-xml-declaration", "no"); break;
  }

  switch (aSerializerOptions.standalone)
  {
  case ZORBA_STANDALONE_YES:
    aInternalSerializer.setParameter("standalone", "yes"); break;
  case ZORBA_STANDALONE_NO:
    aInternalSerializer.setParameter("standalone", "no"); break;
  case ZORBA_STANDALONE_OMIT:
    aInternalSerializer.setParameter("standalone", "omit"); break;
  }

  switch (aSerializerOptions.undeclare_prefixes)
  {
  case ZORBA_UNDECLARE_PREFIXES_YES:
    aInternalSerializer.setParameter("undeclare-prefixes", "yes"); break;
  case ZORBA_UNDECLARE_PREFIXES_NO:
    aInternalSerializer.setParameter("undeclare-prefixes", "no"); break;
  }

  switch(aSerializerOptions.encoding)
  {
  case ZORBA_ENCODING_UTF8:
    aInternalSerializer.setParameter("encoding", "UTF-8"); break;
  case ZORBA_ENCODING_UTF16:
    aInternalSerializer.setParameter("encoding", "UTF-16"); break;
  }

  if (aSerializerOptions.media_type != "")
    aInternalSerializer.setParameter("media-type", aSerializerOptions.media_type.c_str());

  if (aSerializerOptions.doctype_system != "")
    aInternalSerializer.setParameter("doctype-system", aSerializerOptions.doctype_system.c_str());

  if (aSerializerOptions.doctype_public != "")
    aInternalSerializer.setParameter("doctype-public", aSerializerOptions.doctype_public.c_str());

  if (aSerializerOptions.cdata_section_elements != "")
    aInternalSerializer.setParameter("cdata-section-elements", aSerializerOptions.cdata_section_elements.c_str());

  if (aSerializerOptions.version != "")
    aInternalSerializer.setParameter("version", aSerializerOptions.version.c_str());

#ifdef ZORBA_WITH_JSON
  switch (aSerializerOptions.jsoniq_extensions)
  {
    case JSONIQ_EXTENSIONS_YES:
      aInternalSerializer.setParameter("jsoniq-extensions", "yes");
      break;
    case JSONIQ_EXTENSIONS_NO:
      aInternalSerializer.setParameter("jsoniq-extensions", "no");
      break;
  }

  switch (aSerializerOptions.jsoniq_multiple_items)
  {
    case JSONIQ_MULTIPLE_ITEMS_NO:
      aInternalSerializer.setParameter("jsoniq-multiple-items", "no");
      break;
    case JSONIQ_MULTIPLE_ITEMS_ARRAY:
      aInternalSerializer.setParameter("jsoniq-multiple-items", "array");
      break;
    case JSONIQ_MULTIPLE_ITEMS_APPENDED:
      aInternalSerializer.setParameter("jsoniq-multiple-items", "appended");
      break;
  }

  switch (aSerializerOptions.jsoniq_allow_mixed_xdm_jdm)
  {
    case JSONIQ_ALLOW_MIXED_XDM_JDM_NO:
      aInternalSerializer.setParameter("jsoniq-allow-mixed-xdm-jdm", "no");
      break;
    case JSONIQ_ALLOW_MIXED_XDM_JDM_YES:
      aInternalSerializer.setParameter("jsoniq-allow-mixed-xdm-jdm", "yes");
      break;
  }

  convertSerializationMethod(aInternalSerializer,
                             "jsoniq-xdm-node-output-method",
                             aSerializerOptions.jsoniq_xdm_method);

#endif /* ZORBA_WITH_JSON */
}

void
SerializerImpl::setSerializationParameters(
  serializer&   aInternalSerializer,
  ItemSequence* aSerializerOptions)
{
  Item lItem;
  Iterator_t  ser_iter = aSerializerOptions->getIterator();
  ser_iter->open();
  while (ser_iter->next(lItem)) {
    Item lNodeName;
    lItem.getNodeName(lNodeName);
    aInternalSerializer.setParameter(lNodeName.getLocalName().c_str(), lItem.getStringValue().c_str());
  }
  ser_iter->close();
}

} // namespace zorba
/* vim:set et sw=2 ts=2: */
