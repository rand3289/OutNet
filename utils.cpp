#include "utils.h"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <filesystem>
using namespace std;
using namespace std::filesystem; // directory_iterator()


void turnBitsOff(uint32_t& mask, uint32_t bits){
    mask = mask & (0xFFFFFFFF^bits); // TODO: return a new mask instead of taking a reference???
}


// parse a string into tokens
bool tokenize( char*& buffer, const char* bufferEnd, char*& token, const string& separators ){
    while(buffer != bufferEnd){ // skip leading separators
        char c = *buffer;
        if( 0==c || '\r'==c || '\n'==c ) { // end of line - no more tokens
            return false;
        }
        if( string::npos != separators.find(c) ) { // skip separators
            ++buffer;
        } else {
            break;
        }
    }

    token = buffer;

    while(buffer != bufferEnd){
        char c = *buffer;
        if( string::npos != separators.find(c) || '\r'==c || '\n'==c ) { // end of token
            *buffer=0; // separate strings
            ++buffer;
            return true;
        }
        ++buffer; // skip to the end of token
    }

    return buffer>token; // at last one char in the token
}


// toLower() is a complete hack as it modifies the string through iterators
string& toLower(string& s){ // convert string to lower case in place
    std::transform(begin(s), end(s), begin(s), [](char c){ return std::tolower(c); } );
    return s;
}


string& toUpper(string& s){ // convert string to lower case in place
    std::transform(begin(s), end(s), begin(s), [](char c){ return std::toupper(c); } );
    return s;
}


string& ltrim(string& s){ // trim white spaces on the left
    auto ends = find_if(begin(s), end(s), [](char ch){ return !isspace(ch); }  );
    s.erase(begin(s), ends );
    return s;
}


string& rtrim(string& s){ // trim white spaces on the right
    auto start = find_if(rbegin(s), rend(s), [](char ch){ return !isspace(ch); } );
    s.erase( start.base(), end(s) );
    return s;
}


void parseLines(istream& stream, vector<string>& lines){
    string line;
    while( getline(stream, line) ){
        auto comment = find( begin(line), end(line), '#' );
        if( comment != end(line) ){
            line.erase(comment, end(line)); // line = line.substr(0,comment-begin(line));
        }
        rtrim(ltrim(line));
        if(line.length() > 0){
            lines.push_back( move(line) );
        }
    }
}


int parseFilesIntoLines(const string& extension, vector<string>& lines){
    // ~path() destructor crashes on some systems // path cwd = current_path();
    int fCount = 0;
    for(auto& p: directory_iterator(".") ){
        if(p.path().extension() != extension){ continue; }
        cout << p.path() << " ";
        ifstream listf (p.path());
        if( !listf ){ continue; }
        parseLines(listf, lines);
        ++fCount;
    }
    return fCount;
}


// Determine if str is a key=value pair and parse it into key and value parameters.
// Return true if str is a key=value pair.
bool keyValue(const string& str, string& key, string& value){
    auto eq = str.find('=');
    if(string::npos == eq){ return false; }
    key = str.substr(0,eq);
    rtrim(key);
    value = str.substr(eq+1);
    ltrim(value);
    return true;
}


#include <iomanip> // std::hex, std::dec
// print buffer using hex digits 4 + space
void printHex(const unsigned char* buff, int len){
    for(int i = 0; i< len; ++i){
        cout << std::setw(2) << std::setfill('0') << std::hex << (int) buff[i] << ( 1==i%2 ? " ": "");
    }
    cout << std::setw(1) << std::dec << endl; // reset back to decimal
    cout.flush();
}


void printAscii(const unsigned char* buff, int len){
    for(int i = 0; i< len; ++i){
        unsigned char val = buff[i];
        val = ( val > 32 && val< 127 ) ? val : '.';
        cout <<  val;
    }
    cout << endl;
    cout.flush();
}


int writeString(ofstream& file, const string& str){
    unsigned char size = (unsigned char) str.length();
    file.write( (char*) &size, sizeof(size));
    file.write(str.c_str(), size);
    return size+1;
}


string readString(ifstream& file){
    unsigned char size; // since size is an unsigned char it can not be illegal
    file.read( (char*) &size, sizeof(size) );
    if( !file ){ return ""; } // ERROR
    string str((int)size,' ');
    file.read( &str[0], size);
    if( !file ){ return ""; } // ERROR
    return str;
}


#include <chrono>
using namespace std::chrono;
int32_t timeMinutes(){
    static auto epoch = system_clock::from_time_t(0);
    return duration_cast<minutes>(system_clock::now() - epoch).count();
}
