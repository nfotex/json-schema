/*
    Pointer.h -- manipulate JSON documents via JSON Pointers
    Copyright 2015-2018 nfotex IT DL GmbH.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in
       the documentation and/or other materials provided with the
       distribution.

    3. Neither the name Google Inc., nfotex IT DL GmbH, nor the names of
       its contributors may be used to endorse or promote products derived
       from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef JSON_POINTER_H
#define JSON_POINTER_H

#include <string>
#include <vector>

#include <json/json.h>

namespace Json {
#if 0
} // fix auto indent
#endif

class Pointer {
public:
    static std::string escape(const std::string &str);

    Pointer() { }
    Pointer(const std::string &pointer, bool is_fragment = false);
    bool operator==(const Pointer &other) const { return elements == other.elements; }

    Json::Value &get(Json::Value &root, size_t start_index = 0);
    const Json::Value &get(const Json::Value &root, size_t start_index = 0);
    Json::Value &erase(Json::Value &root, size_t start_index = 0);
    void insert(Json::Value &root, Json::Value &value, size_t start_index = 0);
    Json::Value &replace(Json::Value &root, const Json::Value &value, size_t start_index = 0);
    Json::Value *set(Json::Value &root, const Json::Value &value, size_t start_index = 0); // TODO: optional reference return?

    std::string as_string() const;
    const std::string &operator[](size_t idx) const { return elements[idx]; }
    size_t size() const { return elements.size(); }
    size_t parse_array_index(size_t index, size_t size, bool allow_growing = false) const { return parse_array_index(elements[index], size, allow_growing); }
    size_t parse_array_index(const std::string &element, size_t size, bool allow_growing = false) const;

private:
    const Json::Value &get_internal(const Json::Value &root, size_t start_index, bool skip_last = false);

    std::string decode_fragment(const std::string &fragment);
    char decode_hex(const std::string &escape, size_t pos);
    
    void init(const std::string &pointer);

    static std::string decode(const std::string &element);

    std::vector<std::string> elements;
};

}

#endif //  JSON_POINTER_H
