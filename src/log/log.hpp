#pragma once

#include <cstdarg>
#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <iomanip>

constexpr char fsBeginCh = '<';
constexpr char fsSeperatorCh = ',';
constexpr char fsEndCh = '>';

inline std::map<std::string, std::string> m_formatMap;
inline std::string m_log;

template <class StrListContainer = std::initializer_list<std::string>>
std::string createFontStyle(const StrListContainer& ansiAttributes) {
  std::string fontAttributes = "\033[";

  for (size_t i = 0; i < ansiAttributes.size(); i++) {
    fontAttributes.append(*(ansiAttributes.begin() + i));
    if (i + 1 < ansiAttributes.size())
      fontAttributes.push_back(';');
  }

  return fontAttributes + 'm';
}

inline std::string trim(const std::string& str_) {
  std::string str = str_;

  str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](char x) { return !std::isspace(x); }));
  str.erase(std::find_if(str.rbegin(), str.rend(), [](char ch) { return !std::isspace(ch); }).base(),
            str.end());
  return str;
}

inline std::string formatStr(const std::string& fmt) {
  std::string formatted = fmt;
  std::vector<std::string> ansiAttr; // all atttributes with in the "<" ">"

  size_t fsBegin = SIZE_MAX;
  size_t fsSize = 0;
  for (size_t i = 0; i < formatted.size(); i++) {
    if (formatted[i] == fsBeginCh) {
      fsBegin = i;
      fsSize++;
      ansiAttr.push_back("");
      continue;
    }
    if (fsBegin != SIZE_MAX) {
      fsSize++;
      if (formatted[i] == fsSeperatorCh) {
        ansiAttr.push_back("");
        continue;
      } else if (formatted[i] == fsEndCh) {
        for (std::string& str : ansiAttr) {
          std::string key = trim(str);
          if (!m_formatMap.contains(key)) {
            std::cout << "Unknown Key: " << key << std::endl;
          } else {
            str = m_formatMap.at(trim(str));
          }
        }

        i = fsBegin;
        formatted.erase(fsBegin, fsSize);
        formatted.insert(fsBegin, createFontStyle<std::vector<std::string>>(ansiAttr));
        fsBegin = SIZE_MAX;
        fsSize = 0;
        ansiAttr.clear();
        continue;
      }

      ansiAttr.back().push_back(formatted[i]);
    }
  }

  return formatted;
}

inline void log(int ec, const char* fmt, va_list args) {
  std::string format = fmt;
  FILE* stream = stdout;

  if (ec != 0)
    format = "[" + std::to_string(ec) + "]" + format;

  format = formatStr(format);

  vfprintf(stream, format.c_str(), args);
  m_log += format;
}

inline void logInit() {
  m_formatMap["reset"] = "0";
  m_formatMap["bold"] = "1";
  m_formatMap["faint"] = "2";
  m_formatMap["italic"] = "3";
  m_formatMap["underline"] = "4";
  m_formatMap["slowBlink"] = "5";
  m_formatMap["fastBlink"] = "6";
  m_formatMap["reverseVideo"] = "7";
  m_formatMap["concealOn"] = "8";
  m_formatMap["crossedOut"] = "9";
  m_formatMap["primaryFont"] = "10";
  m_formatMap["selectAlternateFontBegin"] = "11";
  m_formatMap["selectAlternateFontEnd"] = "19";
  m_formatMap["fraktur"] = "20";
  m_formatMap["blinkOff"] = "25";
  m_formatMap["inverseOff"] = "27";
  m_formatMap["concealOff"] = "28";
  m_formatMap["fgBlack"] = "30";
  m_formatMap["bgBlack"] = "40";
  m_formatMap["fgRed"] = "31";
  m_formatMap["bgRed"] = "41";
  m_formatMap["fgGreen"] = "32";
  m_formatMap["bgGreen"] = "42";
  m_formatMap["fgYellow"] = "33";
  m_formatMap["bgYellow"] = "43";
  m_formatMap["fgBlue"] = "34";
  m_formatMap["bgBlue"] = "44";
  m_formatMap["fgMagenta"] = "35";
  m_formatMap["bgMagenta"] = "45";
  m_formatMap["fgCyan"] = "36";
  m_formatMap["bgCyan"] = "46";
  m_formatMap["fgWhite"] = "37";
  m_formatMap["bgWhite"] = "47";
  m_formatMap["framed"] = "51";
  m_formatMap["encircledOn"] = "52";
  m_formatMap["overlinedOn"] = "53";
  m_formatMap["encircledAndFramedOff"] = "54";
  m_formatMap["overlinedOff"] = "55";
  m_formatMap["fgBrightBlack"] = "90";
  m_formatMap["bgBrightBlack"] = "100";
  m_formatMap["fgBrightRed"] = "91";
  m_formatMap["bgBrightRed"] = "101";
  m_formatMap["fgBrightGreen"] = "92";
  m_formatMap["bgBrightGreen"] = "102";
  m_formatMap["fgBrightYellow"] = "93";
  m_formatMap["bgBrightYellow"] = "103";
  m_formatMap["fgBrightBlue"] = "94";
  m_formatMap["bgBrightBlue"] = "104";
  m_formatMap["fgBrightMagenta"] = "95";
  m_formatMap["bgBrightMagenta"] = "105";
  m_formatMap["fgBrightCyan"] = "96";
  m_formatMap["bgBrightCyan"] = "106";
  m_formatMap["fgBrightWhite"] = "97";
  m_formatMap["bgBrightWhite"] = "107";

  std::cout << std::setw(2);
}

inline void logGeneric(const char* fmt, ...) {
  va_list args;

  va_start(args, fmt);
  log(0, fmt, args);
  va_end(args);
}

inline void logError(int ec, const char* fmt, ...) {
  va_list args;

  va_start(args, fmt);
  log(ec, fmt, args);
  va_end(args);
}
