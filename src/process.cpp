#include "process.h"

#include <linux_parser.h>
#include <unistd.h>

#include <cctype>
#include <sstream>
#include <string>
#include <vector>

using std::string;
using std::to_string;
using std::vector;

Process::Process(int id) : id_(id) {}
int Process::Pid() { return id_; }

float Process::CpuUtilization() const {
  return LinuxParser::CpuUtilization(id_);
}

string Process::Command() { return LinuxParser::Command(id_); }

string Process::Ram() const { return LinuxParser::Ram(id_); }

string Process::User() { return LinuxParser::User(id_); }

long int Process::UpTime() { return LinuxParser::UpTime(id_); }

bool Process::operator<(Process const& a) const {
  return CpuUtilization() < a.CpuUtilization();
}