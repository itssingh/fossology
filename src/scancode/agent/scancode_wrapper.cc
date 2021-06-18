#include "scancode_wrapper.hpp"
#include "scancode_utils.hpp"
#include <boost/tokenizer.hpp>
#include <iostream>
#include<fstream>

string scanFileWithScancode(const State &state, const fo::File &file) {

  FILE *in;
  char buffer[512];

  // "scancode -l --custom-output - --custom-template template.html " + filename

  string command =
      "scancode -l --custom-output - --custom-template scancode_template.html " + file.getFileName();
  string result = "";

  if (!(in = popen(command.c_str(), "r"))) {
    cout << "could not execute scancode command: " << command << endl;
    bail(1);
  }

  while (fgets(buffer, sizeof(buffer), in) != NULL) {
    result += buffer;
  }

  if (pclose(in) != 0) {
    cout << "could not execute scancode command: " << command << endl;
    bail(1);
  }
  cout<<result<<endl;
  int startjson = result.find("{");
  return result.substr(startjson, string::npos);
}

vector<LicenseMatch> extractLicensesFromScancodeResult(string scancodeResult, const string& filename) {
  Json::Reader scanner;
  Json::Value scancodevalue;
  bool isSuccessful = scanner.parse(scancodeResult, scancodevalue);
  vector<LicenseMatch> result;
  if (isSuccessful) {
    Json::Value resultarrays = scancodevalue["licenses"];
    for (unsigned int i = 0; i < resultarrays.size(); i++) {
        Json::Value oneresult = resultarrays[i];
          string licensename = oneresult["key"].asString();
          float percentage = oneresult["score"].asFloat();
          result.push_back(LicenseMatch(licensename,percentage));
    }
  } else {
    cerr << "JSON parsing failed " << scanner.getFormattedErrorMessages()
         << endl;
    bail(-30);
  }
  return result;
}
