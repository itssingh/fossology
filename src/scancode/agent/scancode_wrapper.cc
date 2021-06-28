#include "scancode_wrapper.hpp"
#include "scancode_utils.hpp"
#include <boost/tokenizer.hpp>
#include <iostream>
#include<fstream>

unsigned getFilePointer(const string &filename, size_t start_line,
                        const string &match_text) {
  ifstream checkfile(filename);
  string str;
  if (checkfile.is_open()) {
    for (size_t i = 0; i < start_line - 1; i++) {
      getline(checkfile, str);
    }
    unsigned int file_p = checkfile.tellg();
    cout << "File Pointer" << file_p << "\n";
    getline(checkfile, str);
    cout << "Checkline=" << str << "\n";
    cout << "match text" << match_text << "\n";
    unsigned int pos = str.find(match_text); 
    cout<<"pos"<<pos<<"\n";
    if (pos != string::npos) {
      cout << "file_p+pos" << file_p + pos << "\n";
      return file_p + pos;
    }
  }
  return -1;
}

string scanFileWithScancode(const State &state, const fo::File &file) {

  FILE *in;
  char buffer[512];

  string command =
      "scancode -lc --custom-output - --custom-template scancode_template.html " + file.getFileName() + " --license-text";
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
  unsigned int startjson = result.find("{");
  result=result.substr(startjson, string::npos);
  cout<<result<<"\n";
return result;
}

vector<LicenseMatch> extractLicensesFromScancodeResult(const string& scancodeResult, const string& filename) {
  Json::Reader scanner;
  Json::Value scancodevalue;
  bool isSuccessful = scanner.parse(scancodeResult, scancodevalue);
  vector<LicenseMatch> result;
  if (isSuccessful) {
    Json::Value resultarrays = scancodevalue["licenses"];
    for (unsigned int i = 0; i < resultarrays.size(); i++) {
        Json::Value oneresult = resultarrays[i];
          string licensename = oneresult["key"].asString();
          int percentage = (int)oneresult["score"].asFloat();
          string full_name=oneresult["name"].asString();
          string text_url=oneresult["text_url"].asString();
          string match_text = oneresult["matched_text"].asString();
          unsigned long start_line=oneresult["start_line"].asUInt();
          string temp_text= match_text.substr(0,match_text.find("\n"));
          unsigned start_pointer = getFilePointer(filename, start_line, temp_text);
          unsigned length = match_text.length();
          result.push_back(LicenseMatch(licensename,percentage,full_name,text_url,start_pointer,length));
    }
  } else {
    cerr << "JSON parsing failed " << scanner.getFormattedErrorMessages()
         << endl;
    bail(-30);
  }
  return result;
}

vector<Match> extractOthersFromScancodeResult(const string& scancodeResult, const string& filename) {
  Json::Reader scanner;
  Json::Value scancodevalue;
  cout<<scancodeResult<<"\n";
  bool isSuccessful = scanner.parse(scancodeResult, scancodevalue);
  cout<<"isSuccessful"<<isSuccessful<<"\n";
  vector<Match> result;
  if (isSuccessful) {
    Json::Value copyarrays = scancodevalue["copyrights"];
    for (unsigned int i = 0; i < copyarrays.size(); i++) {
        Json::Value oneresult = copyarrays[i];
          string type = "copyright";
          string copyrightname = oneresult["value"].asString();
          unsigned long start_line=oneresult["start"].asUInt();
          string temp_text= copyrightname.substr(0,copyrightname.find("[\n\t]"));
          unsigned start_pointer = getFilePointer(filename, start_line, temp_text);
          unsigned length = copyrightname.length();
          result.push_back(Match(type,copyrightname,start_pointer,length));
    }
    Json::Value holderarrays = scancodevalue["holders"];
    for (unsigned int i = 0; i < holderarrays.size(); i++) {
      cout<<"i="<<i<<"\n";
        Json::Value oneresult = holderarrays[i];
          string type = "holder";
          string holdername = oneresult["value"].asString();
          cout<<"holdername"<<holdername<<"\n";
          unsigned long start_line=oneresult["start"].asUInt();
          string temp_text= holdername.substr(0,holdername.find("\n"));
          unsigned start_pointer = getFilePointer(filename, start_line, temp_text);
          unsigned length = holdername.length();
          cout<<"second check"<<i<< temp_text<<"\n";
          result.push_back(Match(type,holdername,start_pointer,length));
    }
  } else {
    cerr << "JSON parsing failed " << scanner.getFormattedErrorMessages()
         << endl;
    bail(-30);
  }
  return result;
}
