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

#include "debugger_server.h"

#include <sstream>
#ifndef WIN32
# include <signal.h>
#endif

#include <zorba/util/base64_util.h>
#include <zorba/util/uri.h>

#include "api/xqueryimpl.h"

#include "debugger/debugger_communicator.h"
#include "debugger/debugger_runtime.h"
#include "debugger/debugger_common.h"
#include "debugger/debugger_commons.h"

#include "zorbautils/synchronous_logger.h"


namespace zorba {

bool theInterruptBreak = false;

// this will make sure the zorba when run in debug (i.e. with the debugger)
// will not terminate when Ctrl-C is pressed but trigger a query interruption
// break through the theInterruptBreak variable that is continuously monitored
// by the debugger commons through the debugger runtime
#ifdef WIN32
BOOL WINAPI
DebuggerServer::ctrlC_Handler(DWORD aCtrlType)
{
  if (CTRL_C_EVENT == aCtrlType) {
    theInterruptBreak = true;
    return true;
  }
  return false;
}
#else
void
DebuggerServer::ctrlC_Handler(int lParam)
{
  theInterruptBreak = true;
}
#endif


DebuggerServer::DebuggerServer(
  XQueryImpl*               aQuery,
  Zorba_SerializerOptions&  aSerializerOptions,
  std::ostream&             aOstream,
  itemHandler               aHandler,
  void*                     aCallbackData,
  const std::string&        aHost,
  unsigned short            aPort)
  : theStopping(false)
{
  theCommunicator = new DebuggerCommunicator(aHost, aPort);
  theRuntime = new DebuggerRuntime(
    aQuery, aOstream, aSerializerOptions,
    theCommunicator, aHandler, aCallbackData,
    &theInterruptBreak);
#ifdef WIN32
  theFileName = aQuery->getFileName().str();
#else
  // TODO: under Linux, when trying to get the file name of the query
  //       the call fails because getFileName tries to get a lock that
  //       is already taken. Therefore the assertion in mutex.cpp:63
  //       terminates the execution 
  theFileName = "";
#endif
}


DebuggerServer::~DebuggerServer()
{
  delete theRuntime;
  delete theCommunicator;
}


bool
DebuggerServer::run()
{
  // add the interrupt handlers to catch Ctrl-C
  #ifdef WIN32
    SetConsoleCtrlHandler(DebuggerServer::ctrlC_Handler, TRUE);
  #else
    signal(SIGINT, ctrlC_Handler);
  #endif

  theCommunicator->connect();

  if (!theCommunicator->isConnected()) {
    return false;
  }

  init();

  std::string lCommand;
  std::string lCommandName;
  int lTransactionID = 0;

  while (!theStopping &&
      theRuntime->getExecutionStatus() != QUERY_DETACHED) {

    // read next command
    theCommunicator->receive(lCommand);
    DebuggerCommand lCmd = DebuggerCommand(lCommand);

    lCommandName = lCmd.getName();
    lCmd.getArg("i", lTransactionID);

    if (theRuntime->getExecutionStatus() == QUERY_TERMINATED) {
      // clone the existing runtime
      DebuggerRuntime* lNewRuntime = theRuntime->clone();

      // reset and delete the existing runtime
      theRuntime->terminate();
      theRuntime->resetRuntime();
      theRuntime->join();
      delete theRuntime;

      // and save the new runtime
      theRuntime = lNewRuntime;
    }

    // process the received command
    std::string lResponse = processCommand(lCmd);
    if (lResponse != "") {
      theCommunicator->send(lResponse);
    }
  }

  theRuntime->terminate();

  std::stringstream lResult;
  lResult << "<response command=\"" << lCommandName << "\" "
    << "status=\"stopped\" "
    << "reason=\"ok\" "
    << "transaction_id=\"" << lTransactionID << "\">"
    << "</response>";
  theCommunicator->send(lResult.str());

  //theRuntime->resetRuntime();
  theRuntime->join();

  return true;
}


void
DebuggerServer::init()
{
  std::string lIdeKey;
  if (!getEnvVar("DBGP_IDEKEY", lIdeKey)) {
    lIdeKey = "";
  }
  std::string lSession;
  if (!getEnvVar("DBGP_SESSION", lSession)) {
    lSession = "";
  }
  ThreadId tid = Runnable::self();
  std::stringstream lInitMsg;
  lInitMsg << "<init appid=\"zorba\" "
    << "idekey=\"" << lIdeKey << "\" "
    << "session=\"" + lSession + "\" "
    << "thread=\"" << tid <<"\" "
    << "parent=\"zorba\" "
    << "language=\"XQuery\" "
    << "protocol_version=\"1.0\" "
    << "fileuri=\"" << URIHelper::encodeFileURI(theFileName).str() << "\"/>";
  
  theCommunicator->send(lInitMsg.str());
}


std::string
DebuggerServer::processCommand(DebuggerCommand aCommand)
{
  std::stringstream lResponse;
  int lTransactionID;
  bool lZorbaExtensions = false;
  ExecutionStatus lStatus;

  if (aCommand.getArg("i", lTransactionID)) {
    lResponse << "<response command=\"" << aCommand.getName() << "\" transaction_id=\"" << lTransactionID << "\" ";
    lStatus = theRuntime->getExecutionStatus();

    int lExtOpt;
    if (aCommand.getArg("z", lExtOpt)) {
      if (lExtOpt) {
        lZorbaExtensions = true;
      }
    }

    std::string lCmdName = aCommand.getName();

    switch (lCmdName.at(0)) {

    // breakpoint_get, breakpoint_set, breakpoint_update, breakpoint_remove, breakpoint_list
    case 'b':
      if (lCmdName == "breakpoint_get" || lCmdName == "breakpoint_list") {
        lResponse << ">"; // response start tag close
        if (lCmdName == "breakpoint_get") {
          int lBID;
          if (aCommand.getArg("d", lBID)) {
            try {
              Breakable lBkp = theRuntime->getBreakpoint(lBID);
              buildBreakpoint(lBkp, lBID, lResponse);
            } catch (std::string& lErr) {
              return buildErrorResponse(lTransactionID, lCmdName, 205, lErr);
            }
          } else {
            // error
          }
        } else {
          // breakpoint_list
          BreakableVector lBkps = theRuntime->getBreakpoints();

          BreakableVector::iterator lIter = lBkps.begin();
          int lId = -1;
          while (lIter != lBkps.end()) {
            Breakable lBkp = *(lIter++);
            lId++;

            // this is only a location without a breakpoint set by the user
            if (!lBkp.isSet()) {
              continue;
            }

            buildBreakpoint(lBkp, lId, lResponse);
          }
        }
      } else {
        if (aCommand.getName() == "breakpoint_set") {

          unsigned int lLineNo;
          aCommand.getArg("n", lLineNo);
          std::string lFileNameTmp;
          aCommand.getArg("f", lFileNameTmp);
          String lFileName(lFileNameTmp);

          std::string lState;
          bool lEnabled = true;
          if (!aCommand.getArg("s", lState)) {
            lState = "enabled";
          }
          lEnabled = (lState == "disabled" ? false : true);
          lState = (lEnabled ? "enabled" : "disabled");

          try {
            unsigned int lBID = theRuntime->addBreakpoint(lFileName, lLineNo, lEnabled);
            lResponse << "state=\"" << lState << "\" id=\"" << lBID << "\" ";
          } catch (std::string& lErr) {
            return buildErrorResponse(lTransactionID, lCmdName, 200, lErr);
          }
 
        } else if (aCommand.getName() == "breakpoint_update") {

          int lBID;
          std::string lState;
          std::string lCondition;
          int lHitValue;
//          bool lHasCondition = false;

          // since we can not change the line number of a breakpoint, we throw an error for that
          if (aCommand.getArg("n", lBID)) {
            return buildErrorResponse(lTransactionID, lCmdName, 208, "A breakpoint line number can not be changed. Omit the -n argument.");
          }

          // the breakpoint ID must be present or we throw an error
          if (!aCommand.getArg("d", lBID)) {
            return buildErrorResponse(lTransactionID, lCmdName, 200, "No breakpoint could not be updated: missing breakpoint ID (-d) argument.");
          }
          if (!aCommand.getArg("s", lState)) {
            lState = "enabled";
          }
          if (aCommand.getArg("h", lHitValue)) {
            if (!aCommand.getArg("o", lCondition)) {
              lCondition = ">=";
            }
          } else {
            lHitValue = 0;
          }

          try {
            bool lEnabled = (lState == "disabled" ? false : true);
            theRuntime->updateBreakpoint(lBID, lEnabled, lCondition, lHitValue);
          } catch (std::string& lErr) {
            return buildErrorResponse(lTransactionID, lCmdName, 205, lErr);
          }

        } else if (aCommand.getName() == "breakpoint_remove") {
          int lBID;
          if (aCommand.getArg("d", lBID)) {
            try {
              theRuntime->removeBreakpoint(lBID);
            } catch (std::string& lErr) {
              return buildErrorResponse(lTransactionID, lCmdName, 205, lErr);
            }
          }
        }

        lResponse << ">"; // response start tag close
      }
    
      break;

    // context_names, context_get
    case 'c':

      int lContextDepth;
      if (!aCommand.getArg("d", lContextDepth)) {
        lContextDepth = 0;
      }

      lResponse << ">";

      if (aCommand.getName() == "context_names") {

        lResponse << "<context name=\"Local\" id=\"0\" />";
        lResponse << "<context name=\"Global\" id=\"1\" />";
 
      } else if (aCommand.getName() == "context_get") {

        int lContextID;
        if (!aCommand.getArg("c", lContextID)) {
          lContextID = 0;
        }
        // we allow -1 to return the properties (variables) in all contexts
        if (lContextID < -1 || lContextID > 1) {
          std::stringstream lErrMsg;
          lErrMsg << "Context invalid (" << lContextID << ") invalid. Check the context list for a correct ID.";
          return buildErrorResponse(lTransactionID, lCmdName, 302, lErrMsg.str());
        }

        // currently we only support contexts for the topmost stack frame
        if (lContextDepth == 0) {

          // get the variables depenging on the context ID (all, local, global)
          std::vector<std::pair<std::string, std::string> > lVariables;
          switch (lContextID) {
          case -1: 
            lVariables = theRuntime->getVariables();
            break;
          case 0: 
            lVariables = theRuntime->getVariables(true);
            break;
          case 1: 
            lVariables = theRuntime->getVariables(false);
            break;
          }

          std::vector<std::pair<std::string, std::string> >::iterator lIter =
            lVariables.begin();

          // and build the corresponding property element for each variable
          for (; lIter != lVariables.end(); ++lIter) {
            std::string lFullName = lIter->first;
            std::string lName = getVariableName(lFullName);
            buildProperty(lFullName, lName, lIter->second, lResponse);
          }
        } else {
          std::stringstream lErrMsg;
          if (lContextDepth < 0) {
            lErrMsg << "Invalid stack depth: " << lContextDepth << "";
            return buildErrorResponse(lTransactionID, lCmdName, 301, lErrMsg.str());
          } else if (lContextDepth > 0) {
            lErrMsg << "Invalid stack depth: " << lContextDepth << ". Currently we only support data commands for the top-most stack frame.";
            return buildErrorResponse(lTransactionID, lCmdName, 301, lErrMsg.str());
          }
        }

      }

      break;

    // eval
    case 'e':
      {
        lResponse << "success=\"1\">";

        try {

          String const lEncodedData(aCommand.getData());
          String lDecodedData;
          base64::decode( lEncodedData, &lDecodedData );

          zstring lVar(lDecodedData.c_str());
          std::list<std::pair<zstring, zstring> > lResults = theRuntime->eval(lVar);

          std::string lName = "";
          buildChildProperties(lName, lResults, lResponse);

        } catch (std::string aMessage) {
          return buildErrorResponse(lTransactionID, lCmdName, 206, aMessage);
        } catch (...) {
          return buildErrorResponse(lTransactionID, lCmdName, 206, "Error while evaluating expression.");
        }
      }

      break;

    // feature_get, feature_set
    case 'f':
      {
        std::string lFeatureName;
        if (aCommand.getArg("n", lFeatureName)) {
          lResponse << "feature_name=\"" << lFeatureName << "\" ";

          if (aCommand.getName() == "feature_get") {
            lResponse << "supported=\"0\" ";
          } else if (aCommand.getName() == "feature_set") {
            lResponse << "success=\"0\" ";
          }
        }
      }
      lResponse << ">";
      break;

    // property_get, property_set, property_value
    case 'p':

      {
        lResponse << ">";

        int lDepth;
        if (!aCommand.getArg("d", lDepth)) {
          lDepth = 0;
        }
        std::string lFullName;
        if (!aCommand.getArg("n", lFullName)) {
          std::stringstream lSs;
          lSs << "Invalid options: " << "missing -n option for a property command";
          return buildErrorResponse(lTransactionID, lCmdName, 3, lSs.str());
        }

        if (aCommand.getName() == "property_get") {
          // currently ignoring -p (page) option

          std::string lName = getVariableName(lFullName);
          // the type is not needed anymore here
          std::string lDummyType("");
          buildProperty(lFullName, lName, lDummyType, lResponse);
        
        } else if (aCommand.getName() == "property_set") {

          std::stringstream lSs;
          lSs << "Unimplemented command: " << lCmdName;
          return buildErrorResponse(lTransactionID, lCmdName, 4, lSs.str());

        } else if (aCommand.getName() == "property_value") {

          std::stringstream lSs;
          lSs << "Unimplemented command: " << lCmdName;
          return buildErrorResponse(lTransactionID, lCmdName, 4, lSs.str());

        }
      }
      break;

    // run
    case 'r':

      if (lStatus == QUERY_SUSPENDED || lStatus == QUERY_IDLE || lStatus == QUERY_TERMINATED) {
        theRuntime->setLastContinuationCommand(lTransactionID, aCommand.getName());
        if (lStatus == QUERY_IDLE || lStatus == QUERY_TERMINATED) {
          theRuntime->start();
        } else if (lStatus == QUERY_SUSPENDED) {
          theRuntime->resumeRuntime();
        }
        return "";
      }

      lResponse << ">";
      break;

    // stack_depth, stack_get, status, stop, step_into, step_over, step_out
    case 's':

      if (aCommand.getName() == "source") {
        lResponse << "success=\"1\" ";
        lResponse << ">";

        int lBeginLine;
        if (!aCommand.getArg("b", lBeginLine)) {
          lBeginLine = 0;
        }
        int lEndLine;
        if (!aCommand.getArg("e", lEndLine)) {
          lEndLine = 0;
        }
        std::string lTmp;
        if (!aCommand.getArg("f", lTmp)) {
          lTmp = "";
        }
        String lFileName(lTmp);

        // we wrap the source code in CDATA to have it unparsed because it might contain XML
        lResponse << "<![CDATA[";
        try {
          lResponse << theRuntime->listSource(lFileName, lBeginLine, lEndLine, lZorbaExtensions) << std::endl;
        } catch (std::string& lErr) {
          return buildErrorResponse(lTransactionID, lCmdName, 100, lErr);
        }
        lResponse << "]]>";

      } else if (aCommand.getName() == "stop") {
        theRuntime->setLastContinuationCommand(lTransactionID, aCommand.getName());
        // sending the zorba extensions flag, the debugger server will not terminate
        // when the stop command is sent. This way the zorba debugger client can
        // perform multiple execution of the same query even when the user terminates
        // the execution using the stop command.
        // NOTE: theStopping is controlling the main debugger server loop
        if (!lZorbaExtensions) {
          theStopping = true;
        }

        lResponse << "status=\"stopping\" reason=\"ok\"";
        lResponse << ">";

        theRuntime->terminateRuntime();

      } else if (aCommand.getName() == "stack_depth") {

        lResponse << "depth=\"" << theRuntime->getStackDepth() << "\"";
        lResponse << ">";

      } else if (aCommand.getName() == "stack_get") {

        try {
          lResponse << ">";
          std::vector<StackFrameImpl> lFrames = theRuntime->getStackFrames();

          std::vector<StackFrameImpl>::reverse_iterator lRevIter;
          int i = 0;
          for (lRevIter = lFrames.rbegin(); lRevIter < lFrames.rend(); ++lRevIter, ++i) {
            buildStackFrame(*lRevIter, i, lResponse);
          }
        } catch (std::string& lErr) {
          return buildErrorResponse(lTransactionID, lCmdName, 303, lErr);
        }
      } else if (aCommand.getName() == "status") {

        ExecutionStatus lStatus = theRuntime->getExecutionStatus();
        std::string lStatusStr;
        switch (lStatus) {
        case QUERY_IDLE:
          lStatusStr = "starting";
          break;
        case QUERY_RUNNING:
          lStatusStr = "running";
          break;
        case QUERY_SUSPENDED:
          lStatusStr = "break";
          break;
        case QUERY_TERMINATED:
        case QUERY_DETACHED:
          lStatusStr = "stopped";
          break;
        }

        lResponse << "status=\"" << lStatusStr << "\" "
          << "reason=\"ok\" "
          << ">";

      } else if (aCommand.getName() == "step_into") {
        theRuntime->setLastContinuationCommand(lTransactionID, aCommand.getName());
        theRuntime->stepIn();
        return "";
      } else if (aCommand.getName() == "step_over") {
        theRuntime->setLastContinuationCommand(lTransactionID, aCommand.getName());
        theRuntime->stepOver();
        return "";
      } else if (aCommand.getName() == "step_out") {
        if (theRuntime->getExecutionStatus() != QUERY_SUSPENDED) {
          return buildErrorResponse(lTransactionID, lCmdName, 6, "Can not step out since the execution is not started.");
        }
        theRuntime->setLastContinuationCommand(lTransactionID, aCommand.getName());
        theRuntime->stepOut();
        return "";
      }

      break;

    default:
      std::stringstream lSs;
      lSs << "Unimplemented command: " << lCmdName;
      return buildErrorResponse(lTransactionID, lCmdName, 4, lSs.str());
    }

    lResponse << "</response>";
    return lResponse.str();
  }

  return "";
}

std::string
DebuggerServer::getVariableName(std::string& aFullName) {

  std::string lName(aFullName);
  std::size_t lIndex = aFullName.find("\\");

  if (lIndex != std::string::npos) {
    lName = aFullName.substr(0, lIndex);
  }
  return lName;
}

void
DebuggerServer::buildStackFrame(
  StackFrame& aFrame,
  int aSNo,
  std::ostream& aStream)
{
  // the file names in query locations always come as URIs, so decode it
  String lFileName(aFrame.getLocation().getFileName());
  if (lFileName.substr(0, 7) == "file://") {
    lFileName = URIHelper::decodeFileURI(lFileName);
  }

  unsigned int lLB = aFrame.getLocation().getLineBegin();
  unsigned int lLE = aFrame.getLocation().getLineEnd();

  // for the client, the column numbers are 1-based
  unsigned int lCB = aFrame.getLocation().getColumnBegin() - 1;
  // moreover, the column end points to the last character to be selected
  unsigned int lCE = aFrame.getLocation().getColumnEnd() - 2;

  aStream << "<stack "
    << "level=\"" << aSNo << "\" "
    << "type=\"" << "file" << "\" "
    << "filename=\"" << lFileName << "\" "
    << "lineno=\"" << lLB << "\" "
    << "where=\"" << aFrame.getSignature() << "\" "
    << "cmdbegin=\"" << lLB << ":" << lCB << "\" "
    << "cmdend=\"" << lLE << ":" << lCE << "\" "
    << "/>";
};

void
DebuggerServer::buildBreakpoint(
  Breakable& aBreakpoint,
  int aBID,
  std::ostream& aStream)
{
  // the file names in query locations always come as URIs, so decode it
  String lFileName(aBreakpoint.getLocation().getFilename().str());
  if (lFileName.substr(0, 7) == "file://") {
    lFileName = URIHelper::decodeFileURI(lFileName);
  }

  aStream << "<breakpoint "
    << "id=\"" << aBID << "\" "
    << "type=\"line\" "
    << "state=\"" << (aBreakpoint.isEnabled() ? "enabled" : "disabled") << "\" "
    << "filename=\"" << lFileName << "\" "
    << "lineno=\"" << aBreakpoint.getLocation().getLineBegin() << "\" "
//  << "function=\"FUNCTION\" "
//  << "exception=\"EXCEPTION\" "
//  << "hit_value=\"HIT_VALUE\" "
//  << "hit_condition=\"HIT_CONDITION\" "
//  << "hit_count=\"HIT_COUNT\" "
    << ">"
//  << "<expression>EXPRESSION</expression>"
  << "</breakpoint>";
};

void
DebuggerServer::buildProperty(
  std::string& aFullName,
  std::string& aName,
  std::string& aType,
  std::ostream& aStream)
{
  bool lFetchChildren = aType == "";
  std::list<std::pair<zstring, zstring> > lResults;

  // TODO: currently we don't evaluate the context item because of an exception
  // thrown in the dynamic context that messes up the plan wrapper in the eval
  // iterator and subsequent evals will fails
  if (aName != ".") {
    zstring lVar;
    lVar.append("$");
    lVar.append(aName);
    try {
      lResults = theRuntime->eval(lVar);
    } catch (...) {
      // TODO: a better reporting
    }
  }

  std::size_t lSize = lResults.size();

/* not using the following property attributes */
//  << "classname=\"name_of_object_class\" "
//  << "size=\"9\" "
//  << "page=\"{NUM}\" "
//  << "pagesize=\"{NUM}\" "
//  << "address=\"{NUM}\" "
//  << "key=\"language_dependent_key\" "

  aStream << "<property "
    << "name=\"" << aName << "\" "
    << "fullname=\"" << aFullName << "\" "
    << "type=\"" << "" << aType << "" << "\" "
    << "encoding=\"base64\" "
    << "constant=\"1\" "
    << "children=\"" << (lSize > 1 ? 1 : 0) << "\" ";
  if (lSize > 1) {
    aStream << "numchildren=\"" << lSize << "\" ";
  }
  aStream << ">";

  if (lFetchChildren && lSize > 1) {
    buildChildProperties(aName, lResults, aStream);
  } else if (lResults.size() == 1) {
    String const lValue(lResults.front().first.c_str());
    base64::encode( lValue, aStream );
  }

  aStream << "</property>";
}

void
DebuggerServer::buildChildProperties(
  std::string& aName,
  std::list<std::pair<zstring, zstring> >& aResults,
  std::ostream& aStream)
{
  std::list<std::pair<zstring, zstring> >::iterator lIter = aResults.begin();
  for (int i = 1; lIter != aResults.end(); ++lIter, ++i) {
    String lValue(lIter->first.c_str());
    std::stringstream lNameSs;
    if (aName != "") {
      lNameSs << aName << "[" << i << "]";
    }

    aStream << "<property "
      << "name=\"" << lNameSs.str() << "\" "
      << "fullname=\"" << lNameSs.str() << "\" "
      << "type=\"" << lIter->second << "\" "
      << "encoding=\"base64\" "
      << "constant=\"1\" "
      << "children=\"0\">";
    base64::encode( lValue, aStream );
    aStream << "</property>";
  }
}

bool
DebuggerServer::getEnvVar(const std::string& aName, std::string& aValue) 
{
  char *lValue;
    
#ifdef __unix__
  lValue = getenv(aName.c_str()); 
#elif defined WINCE
  lValue = NULL;
#elif defined WIN32
  size_t len;
  _dupenv_s(&lValue, &len, aName.c_str());
#else
  lValue = getenv(aName.c_str());
#endif
  if (lValue == 0) {
    return false;
  }

  aValue = lValue;

  return true;
}

std::string
DebuggerServer::buildErrorResponse(
  int aTransactionID,
  std::string aCommandName,
  int aErrorCode,
  std::string aErrorMessage)
{
  std::stringstream lResponse;
  lResponse << "<response command=\"" << aCommandName << "\" transaction_id=\"" << aTransactionID << "\">"
    << "<error code=\"" << aErrorCode << "\" apperr=\"" << aErrorCode << "\">"
    << "<message>" << aErrorMessage << "</message>"
    << "</error></response>";

  return lResponse.str();
}


void
DebuggerServer::throwError()
{
  std::stringstream lSs;
  lSs << theCommunicator->getHost() << ":" << theCommunicator->getPort();
  throw ZORBA_EXCEPTION(
    zerr::ZGDB0001_CANNOT_CONNECT_TO_CLIENT, ERROR_PARAMS( lSs.str() )
  );
}

} // namespace zorba
/* vim:set et sw=2 ts=2: */
