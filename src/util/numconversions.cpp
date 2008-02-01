#include <boost/lexical_cast.hpp>
#include <sstream>
#include <limits>
#include "util/numconversions.h"
#include "zorba/common.h"
#include "util/Assert.h"

namespace xqp {
  bool NumConversions::isNegZero(const xqpString& aStr) {
    xqpString lStr = aStr.trim(" \n\r\t", 4);
    size_t lLength = aStr.length();
    const char* lChars = aStr.c_str();
    if (lChars[0] == '-') {
      for(size_t i = 1; i < lLength; ++i) {
        if (lChars[i] != '0') {
          return false;
        }
      }
      return true;
    } else {
      return false;
    }
  }

  short NumConversions::isInfOrNan(const char* aCharStar) {
#if HAVE_STRCASECMP_FUNCTION && !WIN32
      if (strcasecmp(aCharStar, "inf") == 0 || strcasecmp(aCharStar, "+inf") == 0 )
#else
      if (_stricmp(aCharStar, "inf") == 0 || _stricmp(aCharStar, "+inf") == 0 )
#endif
      {
        return 1;
      }
#if HAVE_STRCASECMP_FUNCTION && !WIN32
      else if (strcasecmp(aCharStar, "-inf") == 0 )
#else
      else if (_stricmp(aCharStar, "-inf") == 0 )
#endif
      {
        return -1;
      }
#if HAVE_STRCASECMP_FUNCTION && !WIN32
      else if (strcasecmp(aCharStar, "nan") == 0 )
#else
      else if (_stricmp(aCharStar, "nan") == 0 )
#endif
      {
        return 0;
      }
      else
      {
        return -2;
      }
  }

  bool NumConversions::starCharToInteger(const char* aCharStar, xqp_integer& aInteger) {
    // correctness check
    const char* lCur = aCharStar;
    bool lGotSign = false;
    bool lStop = false;
    bool lGotDigit = false;
    while (*lCur != '\0' && !lStop) {
      char lTmp = *lCur++;
      switch(lTmp) {
        case '+': 
          if (lGotSign || lGotDigit) {
            lStop = true;
          } else {
            lGotSign = true;
          }
          break;
        case '-':
          if(lGotSign || lGotDigit) {
            lStop = true;
          } else {
            lGotSign = true;
          }
          break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': 
          lGotDigit = true;
          break;
        default:
          lStop = true;
          break;
      }
    }
    if (lStop) {
      return false;
    } else {
      aInteger = aCharStar;
      return true;
    }
  }

  bool NumConversions::strToInteger(const xqpString& aStr, xqp_integer& aInteger){
    return starCharToInteger(aStr.c_str(), aInteger);
  }
  xqpString NumConversions::integerToStr(xqp_integer aInteger){
    return aInteger.toString();
  }
  bool NumConversions::strToUInteger(const xqpString& aStr, xqp_uinteger& aUInteger){
    const char* lStarChar = aStr.c_str();

    // correctness check
    const char* lCur = lStarChar;
    bool lStop = false;
    while (*lCur != '\0' && !lStop) {
      char lTmp = *lCur++;
      switch(lTmp) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': 
          break;
        default:
          lStop = true;
          break;
      }
    }
    if (lStop) {
      return false;
    } else {
      aUInteger = lStarChar;
      return true;
    }
  }
  xqpString NumConversions::uintegerToStr(xqp_uinteger aUInteger){
    return aUInteger.toString();
  }
  bool NumConversions::starCharToInt(const char* aCharStar, xqp_int& aInt){
    try {
      aInt =  boost::lexical_cast<xqp_int>(aCharStar);
      return true;
    } catch (boost::bad_lexical_cast &) {
      return false;
    }
  }
  bool NumConversions::strToInt(const xqpString& aStr, xqp_int& aInt) {
    return starCharToInt(aStr.c_str(), aInt);
  }
  xqpString NumConversions::intToStr(xqp_int aInt){
    return boost::lexical_cast<std::string>(aInt);
  }
  bool NumConversions::strToUInt(const xqpString& aStr, xqp_uint& aUInt){
    if ( isNegZero(aStr)) {
      aUInt = 0;
      return true;
    }
    try {
      aUInt =  boost::lexical_cast<xqp_uint>(aStr.c_str());
      return true;
    } catch (boost::bad_lexical_cast &) {
      return false;
    }
  }
  xqpString NumConversions::uintToStr(xqp_uint aUInt){
    return boost::lexical_cast<std::string>(aUInt);
  }
  bool NumConversions::strToLong(const xqpString& aStr, xqp_long& aLong){
    try {
      aLong = boost::lexical_cast<xqp_long>(aStr.c_str());
      return true;
    } catch (boost::bad_lexical_cast &) {
      return false;
    }
  }
  xqpString NumConversions::longToStr(xqp_long aLong){
    return boost::lexical_cast<std::string>(aLong);
  }
  bool NumConversions::strToULong(const xqpString& aStr, xqp_ulong& aULong){
    if ( isNegZero(aStr)) {
      aULong = 0;
      return true;
    }
    try {
      aULong = boost::lexical_cast<xqp_ulong>(aStr.c_str());
      return true;
    } catch (boost::bad_lexical_cast &) {
      return false;
    }
  }
  xqpString NumConversions::ulongToStr(xqp_ulong aULong){
    return boost::lexical_cast<std::string>(aULong);
  }
  bool NumConversions::strToShort(const xqpString& aStr, xqp_short& aShort){
    try {
      aShort = boost::lexical_cast<xqp_short>(aStr.c_str());
      return true;
    } catch (boost::bad_lexical_cast &) {
      return false;
    }
  }
  xqpString NumConversions::shortToStr(xqp_short aShort){
    return boost::lexical_cast<std::string>(aShort);
  }
  bool NumConversions::strToUShort(const xqpString& aStr, xqp_ushort& aUShort){
    if ( isNegZero(aStr )) {
      aUShort = 0;
      return true;
    }
    try {
      aUShort = boost::lexical_cast<xqp_ushort>(aStr.c_str());
      return true;
    } catch (boost::bad_lexical_cast &) {
      return false;
    }
  }
  xqpString NumConversions::ushortToStr(xqp_ushort aUShort){
    return boost::lexical_cast<std::string>(aUShort);
  }
  bool NumConversions::starCharToDecimal(const char* aCharStar, xqp_decimal& aDecimal){
    // correctness check
    const char* lCur = aCharStar;
    bool lGotPoint = false;
    bool lGotSign = false;
    bool lStop = false;
    bool lGotDigit = false;
    while (*lCur != '\0' && !lStop) {
      char lTmp = *lCur++;
      switch(lTmp) {
        case '+': 
          if (lGotSign || lGotDigit || lGotPoint) {
            lStop = true;
          } else {
            lGotSign = true;
          }
          break;
        case '-':
          if(lGotSign || lGotDigit || lGotPoint) {
            lStop = true;
          } else {
            lGotSign = true;
          }
          break;
        case '.': 
          if (lGotPoint) {
            lStop = true;
          } else {
            lGotPoint = true;
          }
          break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': 
          lGotDigit = true;
          break;
        default:
          lStop = true;
          break;
      }
    }
    if (lStop) {
      return false;
    } else {
      aDecimal = aCharStar;
      return true;
    }
  }
  bool NumConversions::strToDecimal(const xqpString& aStr, xqp_decimal& aDecimal) {
    return starCharToDecimal(aStr.c_str(), aDecimal);
  }
  xqpString NumConversions::decimalToStr(xqp_decimal aDecimal){
    return aDecimal.toString();
  }

  bool NumConversions::starCharToFloat(const char* aCharStar, xqp_float& aFloat) {
    char* lEndPtr;

    // Not all systems support strtof
#if HAVE_STRTOF_FUNCTION && !WIN32
    aFloat = strtof(aCharStar, &lEndPtr);
#else
    // If strtof is not supported, zorba uses strtod 
    // and makes a max, min check before casting to float
    xqp_double lTmpDouble = strtod(aCharStar, &lEndPtr);
    if (*lEndPtr == '\0') {
// undef's are used because Windows has some makro definitions on min and max
// => without undef, 'min()' and 'max()' would be replace by something
#  undef max
#  undef min
      if ( lTmpDouble > std::numeric_limits<xqp_float>::max() )
        aFloat = std::numeric_limits<xqp_float>::infinity();
      else if ( lTmpDouble < std::numeric_limits<xqp_float>::min() )
        aFloat = -std::numeric_limits<xqp_float>::infinity();
      else
        aFloat = static_cast<xqp_float>(lTmpDouble);
    }
#endif
     
    if (*lEndPtr != '\0') {
#ifdef UNIX
      return false;
#else
      // Only for unix systems, we are sure that they can parse 'inf' and '-inf' correclty
      // => we try to do it by hand for all other systems in case of a parsing error
      short lInf = NumConversions::isInfOrNan(aCharStar);
      if (lInf == -1) {
        aFloat = -std::numeric_limits<xqp_float>::infinity();
        return true;
      } else if (lInf == 1) {
        aFloat = std::numeric_limits<xqp_float>::infinity();
        return true;
      } else if (lInf == 0) {
        // TODO Handling when compiler does not support quiet_NaN
        Assert(std::numeric_limits<xqp_double>::has_quiet_NaN);
        aFloat = std::numeric_limits<xqp_float>::quiet_NaN(); 
      } else {
        return false;
      }
#endif
    } 
    return true;
  }

  bool NumConversions::strToFloat(const xqpString& aStr, xqp_float& aFloat){
    return NumConversions::starCharToFloat(aStr.c_str(), aFloat);
  }
  xqpString NumConversions::floatToStr(xqp_float aFloat){
    if (aFloat == std::numeric_limits<xqp_float>::infinity())
      return "INF";
    else if (aFloat == -std::numeric_limits<xqp_float>::infinity())
      return "-INF";
    else if (aFloat != aFloat)
      return "NaN";
    else {
      std::stringstream lStream;
      lStream << aFloat;
      return lStream.str();
    }
  }
  bool NumConversions::starCharToDouble(const char* aCharStar, xqp_double& aDouble){
    char* lEndPtr;
    aDouble = strtod(aCharStar, &lEndPtr);
    if (*lEndPtr != '\0') {
#ifdef UNIX
      return false;
#else
      // Only for unix systems, we are sure that they can parse 'inf' and '-inf' correclty
      // => we try to do it by hand for all other systems in case of a parsing error
      short lInf = NumConversions::isInfOrNan(aCharStar);
      if (lInf == -1) {
        aDouble = -std::numeric_limits<xqp_double>::infinity();
        return true;
      } else if (lInf == 1) {
        aDouble = std::numeric_limits<xqp_double>::infinity();
        return true;
      } else if (lInf == 0) {
        // TODO Handling when compiler does not suppoert quiet_NaN
        Assert(std::numeric_limits<xqp_double>::has_quiet_NaN);
        aDouble = std::numeric_limits<xqp_double>::quiet_NaN(); 
      } else {
        return false;
      }
#endif
    } 
    return true;
  }
  bool NumConversions::strToDouble(const xqpString& aStr, xqp_double& aDouble) {
    return starCharToDouble(aStr.c_str(), aDouble);
  }
  xqpString NumConversions::doubleToStr(xqp_double aDouble){
    if (aDouble == std::numeric_limits<xqp_double>::infinity())
      return "INF";
    else if (aDouble == -std::numeric_limits<xqp_double>::infinity())
      return "-INF";
    else if (aDouble != aDouble)
      return "NaN";
    else {
      std::stringstream lStream;
      lStream << aDouble;
      return lStream.str();
    }
  }
  bool NumConversions::strToByte(const xqpString& aStr, xqp_byte& aByte){
    try {
      xqp_int lInt = boost::lexical_cast<xqp_int>(aStr.c_str());
      if (lInt >= -128 && lInt <= 127) {
        aByte = lInt;
        return true;
      } else {
        return false;
      }
    } catch (boost::bad_lexical_cast &) {
      return false;
    }
  }
  xqpString NumConversions::byteToStr(xqp_byte aByte){
    xqp_int lInt = aByte;
    return boost::lexical_cast<std::string>(lInt);
  }
  bool NumConversions::strToUByte(const xqpString& aStr, xqp_ubyte& aUByte){
    if ( isNegZero(aStr)) {
      aUByte = 0;
      return true;
    }
    try {
      xqp_uint lUInt = boost::lexical_cast<xqp_uint>(aStr.c_str());
      if (lUInt >= 0 && lUInt <= 255) {
        aUByte = lUInt;
        return true;
      } else {
        return false;
      }
    } catch (boost::bad_lexical_cast &) {
      return false;
    }
  }
  xqpString NumConversions::ubyteToStr(xqp_ubyte aUByte){
    xqp_uint lUInt = aUByte;
    return boost::lexical_cast<std::string>(lUInt);
  }

  bool NumConversions::isNaN(xqp_double aDouble) {
    return aDouble != aDouble;
  }

  bool NumConversions::isNaN(xqp_float aFloat) {
    return aFloat != aFloat;
  }

  bool NumConversions::isPosOrNegInf(xqp_double aDouble) {
    return (aDouble == std::numeric_limits<xqp_double>::infinity() 
      || aDouble == -std::numeric_limits<xqp_double>::infinity());
  }

  bool NumConversions::isPosOrNegInf(xqp_float aFloat) {
    return (aFloat == std::numeric_limits<xqp_float>::infinity()
      || aFloat == -std::numeric_limits<xqp_float>::infinity());
  }

  xqpString NumConversions::sizetToStr(size_t aSizeT) {
    return boost::lexical_cast<std::string>(aSizeT);
  }
} /* namespace xqp */
