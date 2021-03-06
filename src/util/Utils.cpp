/**
Copyright 2018 JasmineGraph Team
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
 */

#include <vector>
#include <sstream>
#include "Utils.h"

using namespace std;

map<std::string, std::string> Utils::getBatchUploadFileList(std::string file)
{
    std::vector<std::string> batchUploadFileContent = getFileContent(file);
    std::vector<std::string>::iterator iterator1 = batchUploadFileContent.begin();
    map<std::string, std::string>* result = new map<std::string, std::string>();
    while (iterator1 != batchUploadFileContent.end())
    {
        std::string str = *iterator1;

        if (str.length() > 0 && !(str.rfind("#", 0) == 0)) {

            std::vector<std::string> vec = split(str, ':');

//            ifstream batchUploadConfFile(vec.at(1));
//            string line;
//
//            if (batchUploadConfFile.is_open()) {
//                while (getline(batchUploadConfFile, line)) {
//                    cout << line << '\n';
//                }
//            }

            result->insert(std::pair<std::string, std::string>(vec.at(0), vec.at(1)));

        }

        iterator1++;
    }

    return *result;
}

std::vector<std::string> Utils::split(const std::string& s, char delimiter)
{
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter))
    {
        tokens.push_back(token);
    }
    return tokens;
}

std::vector<std::string> Utils::getFileContent(std::string file)
{
    ifstream in(file);

    std::string str;
    //map<std::string, std::string>* result = new map<std::string, std::string>();
    vector<std::string>* vec = new vector<std::string>();
    while (std::getline(in, str)) {
        // output the line
        //std::cout << str << std::endl;

        // now we loop back and get the next line in 'str'

        //if (str.length() > 0 && !(str.rfind("#", 0) == 0)) {
        if (str.length() > 0) {
            //std::vector<std::string> vec = split(str, '=');
            //std::cout << vec.size() << std::endl;
            //result->insert(std::pair<std::string, std::string>(vec.at(0), vec.at(1)));
            vec->insert(vec->begin(), str);
        }
    }

    return *vec;
};

std::string Utils::getJasmineGraphProperty(std::string key)
{
    std::vector<std::string>::iterator it;
    //TODO: Need to remove this hardcoded link
    vector<std::string> vec = getFileContent("/home/miyurud/git/jasminegraph/conf/jasminegraph-server.properties");
    it = vec.begin();

    for (it=vec.begin(); it<vec.end(); it++)
    {
        std::string item = *it;
        if (item.length() > 0 && !(item.rfind("#", 0) == 0)) {
            std::vector<std::string> vec2 = split(item, '=');
            if (vec2.at(0).compare(key) == 0)
            {
                return vec2.at(1);
            }
        }
    }

    return NULL;
}


inline std::string trim_right_copy(
        const std::string& s,
        const std::string& delimiters = " \f\n\r\t\v" )
{
    return s.substr( 0, s.find_last_not_of( delimiters ) + 1 );
}

inline std::string trim_left_copy(
        const std::string& s,
        const std::string& delimiters = " \f\n\r\t\v" )
{
    return s.substr( s.find_first_not_of( delimiters ) );
}

std::string Utils::trim_copy(
        const std::string& s,
        const std::string& delimiters = " \f\n\r\t\v" )
{
    return trim_left_copy( trim_right_copy( s, delimiters ), delimiters );
}




