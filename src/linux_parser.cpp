#include "linux_parser.h"

#include <dirent.h>
#include <unistd.h>

#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using std::cout;
using std::endl;
using std::stof;
using std::string;
using std::to_string;
using std::unordered_map;
using std::vector;

vector<string> FindLines(string path, vector<string> prefixes = {}) {
  vector<string> lines;
  string line;
  string key;
  string value;
  std::ifstream filestream(path);
  if (filestream.is_open()) {
    while (std::getline(filestream, line)) {
      std::istringstream linestream(line);
      linestream >> key;
      for (auto prefix : prefixes) {
        if (prefix == key.substr(0, prefix.size())) {
          lines.push_back(line);
          break;
        }
      }
      if (prefixes.empty()) {
        lines.push_back(line);
      }
    }
  }
  return lines;
}

string LinuxParser::OperatingSystem() {
  string line;
  string key;
  string value;
  std::ifstream filestream(kOSPath);
  if (filestream.is_open()) {
    while (std::getline(filestream, line), " ") {
      std::replace(line.begin(), line.end(), ' ', '_');
      std::replace(line.begin(), line.end(), '=', ' ');
      std::replace(line.begin(), line.end(), '"', ' ');
      std::istringstream linestream(line);
      while (linestream >> key >> value) {
        if (key == "PRETTY_NAME") {
          std::replace(value.begin(), value.end(), '_', ' ');
          return value;
        }
      }
    }
  }
  return value;
}

string LinuxParser::Kernel() {
  string os, kernel, version;
  string line;
  std::ifstream stream(kProcDirectory + kVersionFilename);
  if (stream.is_open()) {
    std::getline(stream, line);
    std::istringstream linestream(line);
    linestream >> os >> version >> kernel;
  }
  return kernel;
}

// BONUS: Update this to use std::filesystem
vector<int> LinuxParser::Pids() {
  vector<int> pids;
  DIR* directory = opendir(kProcDirectory.c_str());
  struct dirent* file;
  while ((file = readdir(directory)) != nullptr) {
    // Is this a directory?
    if (file->d_type == DT_DIR) {
      // Is every character of the name a digit?
      string filename(file->d_name);
      if (std::all_of(filename.begin(), filename.end(), isdigit)) {
        int pid = stoi(filename);
        pids.push_back(pid);
      }
    }
  }
  closedir(directory);
  return pids;
}

float LinuxParser::MemoryUtilization() {
  unordered_map<string, long> data;
  auto lines =
      ::FindLines(kProcDirectory + kMeminfoFilename, {"MemTotal", "MemFree"});

  string line;
  for (auto line : lines) {
    std::replace(line.begin(), line.end(), ':', ' ');
    std::istringstream linestream(line);
    string key;
    long value;
    linestream >> key >> value;
    if (!key.empty()) {
      data[key] = value;
    }
  }

  long usedMemory = data["MemTotal"] - data["MemFree"];
  return usedMemory * 1.0 / data["MemTotal"];
}

long LinuxParser::UpTime() {
  string key, line;
  long value = 0;
  std::ifstream filestream(kProcDirectory + kUptimeFilename);
  if (filestream.is_open()) {
    std::getline(filestream, line);
    std::istringstream linestream(line);
    linestream >> value;
    return value;
  }
  return value;
}

float LinuxParser::CpuUtilization() {
  string key, line;
  std::ifstream filestream(kProcDirectory + kStatFilename);
  if (filestream.is_open()) {
    std::getline(filestream, line);
    std::istringstream linestream(line);
    long user, nice, system, idle, iowait, irq, softirq, steal, guest,
        guest_nice;
    linestream >> key >> user >> nice >> system >> idle >> iowait >> irq >>
        softirq >> steal >> guest >> guest_nice;

    long totalIdle = idle + iowait;
    long nonIdle = user + nice + system + irq + softirq + steal;
    long total = totalIdle + nonIdle;

    return (total - totalIdle) * 1.0 / total;
  }
  return 0;
}

int LinuxParser::TotalProcesses() {
  string key, line;
  line = ::FindLines(kProcDirectory + kStatFilename, {"processes"})[0];
  int value = 0;
  std::istringstream linestream(line);
  linestream >> key;
  linestream >> value;
  return value;
}

int LinuxParser::RunningProcesses() {
  string key, line;
  line = ::FindLines(kProcDirectory + kStatFilename, {"procs_running"})[0];
  int value = 0;
  std::istringstream linestream(line);
  linestream >> key;
  linestream >> value;
  return value;
}

float LinuxParser::CpuUtilization(int pid) {
  string line;
  string tmp;
  std::ifstream filestream(kProcDirectory + to_string(pid) + kStatFilename);
  if (filestream.is_open()) {
    std::getline(filestream, line);
    std::istringstream linestream(line);
    long utime, stime, cutime, cstime, starttime;
    for (int i = 0; i < 14; i++) {
      linestream >> tmp;
    }
    linestream >> utime;
    linestream >> stime;
    linestream >> cutime;
    linestream >> cstime;
    for (int i = 0; i < 5; i++) {
      linestream >> tmp;
    }
    linestream >> starttime;

    long uptime = LinuxParser::UpTime(pid);
    long hz = sysconf(_SC_CLK_TCK);

    long total_time = utime + stime;
    total_time = total_time + cutime + cstime;
    float seconds = uptime - (starttime * 1.0 / hz);
    if (seconds > 0) {
      float cpuUsage = ((total_time * 1.0 / hz) / seconds);
      return cpuUsage;
    } else {
      return 0;
    }
  }

  return 0;
}

// TODO: Read and return the command associated with a process
// REMOVE: [[maybe_unused]] once you define the function
string LinuxParser::Command(int pid) {
  string line;
  std::ifstream stream(kProcDirectory + to_string(pid) + kCmdlineFilename);
  if (stream.is_open()) {
    std::getline(stream, line);
    return line;
  }
  return "";
}

string LinuxParser::Ram(int pid) {
  string key;
  long ramKB = 0;
  auto lines = ::FindLines(kProcDirectory + to_string(pid) + kStatusFilename,
                           {"VmSize"});
  if (lines.empty()) {
    return "0";
  }
  string line = lines[0];
  std::istringstream linestream(line);
  linestream >> key >> ramKB;
  long ramMB = ramKB * 1.0 / 1024;
  return to_string(ramMB);
}

string LinuxParser::Uid(int pid) {
  string key, uid;
  string line = ::FindLines(kProcDirectory + to_string(pid) + kStatusFilename,
                            {"Uid"})[0];
  std::istringstream linestream(line);
  linestream >> key >> uid;
  return uid;
}

string LinuxParser::User(int pid) {
  auto uid = Uid(pid);

  vector<string> lines = ::FindLines(kPasswordPath);
  for (string& line : lines) {
    if (line.find(":" + uid + ":") != string::npos) {
      return line.substr(0, line.find(":"));
    }
  }
  return string();
}

long LinuxParser::UpTime(int pid) {
  string line;
  string tmp;
  std::ifstream filestream(kProcDirectory + to_string(pid) + kStatFilename);
  if (filestream.is_open()) {
    std::getline(filestream, line);
    std::istringstream linestream(line);
    long starttime;
    for (int i = 0; i < 21; i++) {
      linestream >> tmp;
    }
    linestream >> starttime;

    long hz = sysconf(_SC_CLK_TCK);

    return UpTime() - starttime * 1.0 / hz;
  }

  return 0;
}
