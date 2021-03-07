// It is sad that there are no standard versions of these functions in std::
// Probably why python is more popular :(
#include "utils.h"
#include <algorithm>
#include <iostream>
using namespace std;


void turnBitsOff(uint32_t& mask, uint32_t bits){
    mask = mask & (0xFFFFFFFF^bits); // TODO: return a new mask instead of taking a reference???
}


// TODO: accept buffer and token by reference ???
// TODO: add separators as parameters
// parse a string into tokens
bool tokenize( char** buffer, const char* bufferEnd, char** token ){
    while(*buffer != bufferEnd){ // skip leading separators
        char c = **buffer;
        if( 0==c || '\r'==c || '\n'==c ) { // end of line - no more tokens
            return false;
        }
        if( ' '==c || '&'==c || '?'==c || '/'==c ) { // skip separators
            ++*buffer;
        } else {
            break;
        }
    }

    *token = *buffer;

    while(*buffer != bufferEnd){
        char c = **buffer;
        if( ' '==c || '&'==c || '?'==c || '/'==c || '\r'==c || '\n'==c ) { // end of token
            **buffer=0; // separate strings
            ++*buffer;
            return true;
        }
        ++*buffer; // skip to the end of token
    }

    return false;
}



// toLower() is a complete hack as it modifies the string through iterators
string& toLower(string& s){ // convert string to lower case in place
    std::transform(begin(s), end(s), begin(s), [](char c){ return std::tolower(c); } );
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
