#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED
#include <string>
#include <vector>
// utility functions

void turnBitsOff(uint32_t& mask, uint32_t bits); // turn bits off in a mask

bool tokenize( char** buffer, const char* bufferEnd, char** token ); // break string into tokens

std::string& toLower(std::string& s); // convert string to lower case in place

std::string& ltrim(std::string& s); // trim white spaces on the left

std::string& rtrim(std::string& s); // trim white spaces on the right

// read lines from a stream and put them in a vector
void parseLines(std::istream& stream, std::vector<std::string>& lines);

// Determine if str is a key=value pair and parse it into key and value parameters.
// Return true if str is a key=value pair.
bool keyValue(const std::string& str, std::string& key, std::string& value);

#endif // UTILS_H_INCLUDED