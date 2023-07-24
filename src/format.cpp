#include "format.h"

#include <iomanip>
#include <sstream>
#include <string>
using std::string;

string Format::ElapsedTime(long seconds) {
  int hours = seconds / (60 * 60);
  seconds -= hours * 60 * 60;
  int minutes = seconds / (60);
  seconds -= minutes * 60;

  std::stringstream ss;
  ss << std::setfill('0');
  ss << hours << ":";
  ss << std::setw(2) << minutes << ":";
  ss << std::setw(2) << seconds;
  return ss.str();
}