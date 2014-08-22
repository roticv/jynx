// Copyright 2014-present. All rights reserved.
//
// Author: Victor Loh

#include "folly/String.h"

#include <iostream>
#include <fstream>

using namespace std;

struct JstackThreadInfo {
  string threadName;
  string threadState;
  vector<string> stacktrace;

  // A JstackThreadInfo is considered empty if its threadName is not set
  bool empty() {
    return threadName.empty();
  }

  void clear() {
    threadName.clear();
    threadState.clear();
    stacktrace.clear();
  }
};

struct JstackInfo {
  vector<JstackThreadInfo> threadsInfo;
};

int main(int argc, char* argv[]) {
  if (argc < 2) {
    cout << "usage is ./a.out <jstack_file>" << endl;
    exit(0);
  }
  ifstream inFile(argv[1]);

  stringstream strStream;
  strStream << inFile.rdbuf();
  string str = strStream.str();

  vector<folly::StringPiece> v;
  folly::split("\n", str, v);
  // Skip the first 2 rows (timestamp + jstack junk)
  JstackInfo jstackInfo;
  JstackThreadInfo threadInfo;
  for (int i = 2; i < v.size(); ++i) {
    auto& sp = v[i];
    if (sp.front() == '"') {
      sp.advance(1);
      auto p = sp.split_step('"');
      threadInfo.threadName = std::move(p.toString());
    } else if (sp.empty()) {
      // Reached a newline, the right course of action would be to add
      // JstackThreadInfo to JstackInfo and then clear out the current
      // JstackThreadInfo
      if (!threadInfo.empty()) {
        jstackInfo.threadsInfo.emplace_back(std::move(threadInfo));
        threadInfo.clear();
      }
    } else {
      folly::StringPiece threadStateNeedle("java.lang.Thread.State: ");
      auto pos = sp.find(threadStateNeedle);
      if (pos != string::npos) {
        sp.advance(pos + threadStateNeedle.size());
        threadInfo.threadState = std::move(sp.toString());
        continue;
      }
      folly::StringPiece stackTraceStartNeedle("at ");
      pos = sp.find(stackTraceStartNeedle);
      if (pos != string::npos) {
        sp.advance(pos + stackTraceStartNeedle.size());
        auto p = sp.split_step('(');
        string m = std::move(p.toString());
        threadInfo.stacktrace.emplace_back(std::move(p.toString()));
      }
    }
  }

  cout << "Jstack info" << endl;
  for (auto& info : jstackInfo.threadsInfo) {
    cout << "Thread name: " << info.threadName << endl;
    cout << "Thread state: " << info.threadState << endl;
    cout << "Stacktrace: " << endl;
    for (int i = 0; i < info.stacktrace.size(); ++i) {
      if (i != 0) cout << ", ";
      cout << info.stacktrace[i];
    }
    cout << endl;
  }

  return 0;
}
