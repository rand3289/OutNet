#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED
#include <string>
#include <vector>
// collection of utility functions
// It is sad that there are no standard versions of these functions in std::
// Probably why python is more popular :(

void turnBitsOff(uint32_t& mask, uint32_t bits); // turn bits off in a mask

int32_t timeMinutes(); // get time in minutes since Jan 1, 1970

// break string into tokens
bool tokenize( char*& buffer, const char* bufferEnd, char*& token, const std::string& separators );

std::string& toLower(std::string& s); // convert string to lower case in place

std::string& toUpper(std::string& s); // convert string to upper case in place

std::string& ltrim(std::string& s); // trim white spaces on the left in place

std::string& rtrim(std::string& s); // trim white spaces on the right in place

// read lines from a stream and put them in a vector
void parseLines(std::istream& stream, std::vector<std::string>& lines);

// run parseLines() on all files with specific extension
int parseFilesIntoLines(const std::string& extension, std::vector<std::string>& lines);

// Determine if str is a key=value pair and parse it into key and value parameters.
// Return true if str is a key=value pair.
bool keyValue(const std::string& str, std::string& key, std::string& value);

void printHex(std::ostream& os, const unsigned char* buff, int len); // print buffer using hex digits (4 + space)
void printAscii(std::ostream& os, const unsigned char* buff, int len); // ascii printable chars or "."

int writeString(std::ofstream& file, const std::string& str); // write string to file. format: size+string
std::string readString(std::ifstream& file);             // read string from file written by writeString()


#endif // UTILS_H_INCLUDED
