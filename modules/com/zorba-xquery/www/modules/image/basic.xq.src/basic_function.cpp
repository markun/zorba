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

#include "basic_function.h"
#include <sstream>
#include <zorba/zorba.h>
#include "basic_module.h"
#include <Magick++.h>
#include <zorba/base64.h>



namespace zorba { namespace imagemodule {  namespace basicmodule {



BasicFunction::BasicFunction(const BasicModule* aModule)
        : theModule(aModule)
{
}

BasicFunction::~BasicFunction()
{
}


String
BasicFunction::getURI() const
{
    return theModule->getURI();
}


} /* namespace basicmodule */
} /* namespace imagemodule */ 
} /* namespace zorba */
