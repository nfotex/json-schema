/*
    Pointer.cc -- manipulate JSON documents via JSON Pointers
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

#include <json/Pointer.h>

#include <inttypes.h>

#include <stdexcept>
#include <sstream>
#include <limits>

namespace Json {
#if 0
} // fix auto indent
#endif

static void encode(const std::string &element, std::stringstream &stream);

std::string Pointer::escape(const std::string &str) {
    std::stringstream ss;
    
    encode(str, ss);
    return ss.str();
}


Pointer::Pointer(const std::string &pointer, bool is_fragment) {
    if (is_fragment) {
        if (pointer[0] != '#') {
            throw std::invalid_argument("fragment doesn't begin with #");
        }
        init(decode_fragment(pointer));
    }
    else {
        init(pointer);
    }
}


void Pointer::init(const std::string &pointer) {
    if (pointer.empty()) {
        return;
    }
    
    if (pointer[0] != '/') {
        throw std::invalid_argument("doesn't begin with /");
    }

    size_t i = 1;
    size_t j = pointer.find('/', 1);
    while (j != std::string::npos) {
        elements.push_back(decode(pointer.substr(i, j-i)));
        i = ++j;
        j = pointer.find('/', j);
    }

    elements.push_back(decode(pointer.substr(i)));
}


std::string Pointer::decode(const std::string &element) {
    size_t j = element.find('~');

    if (j == std::string::npos) {
        return element;
    }

    size_t i = 0;
    std::string decoded;

    while (j != std::string::npos) {
        decoded += element.substr(i, j-i);

        if (j+1 >= element.size()) {
            throw std::invalid_argument("invalid ~ escape");
        }

        switch (element[j+1]) {
            case '0':
                decoded += '~';
                break;
            case '1':
                decoded += '/';
                break;
            default:
                throw std::invalid_argument("invalid ~ escape");
                break;
        }

        j += 2;
        i = j;
        j = element.find('~', j);
    }

    decoded += element.substr(i);

    return decoded;
}


static void encode(const std::string &element, std::stringstream &stream) {
    size_t i = 0;
    size_t j = element.find_first_of("~/");

    while (j != std::string::npos) {
        stream << element.substr(i, j-1);

        switch (element[j]) {
            case '/':
                stream << "~1";
                break;
            case '~':
                stream << "~0";
                break;
        }

        j++;
        i = j;
        j = element.find_first_of("~/", j);
    }

    stream << element.substr(i);
}


Value &Pointer::erase(Value &root, size_t start_index) {
    if (start_index > elements.size()) {
        throw std::range_error("start_index out of range");
    }

    Value &last = const_cast<Value &>(get_internal(root, start_index, true));

    std::string &element = elements[elements.size() - 1];
    switch (last.type()) {
        case arrayValue: {
            Value::ArrayIndex index = static_cast<ArrayIndex>(parse_array_index(element, last.size(), false));
            Value &old_value = last[index];
            for (; index < last.size() - 2; index++) {
                last[index] = last[index+1];
            }
            last.resize(last.size() - 1);
            return old_value;
        }

        case objectValue: {
            if (!last.isMember(element)) {
                throw std::range_error("member '" + element + "' doesn't exists");
            }
            Value &old_value = last[element];
            last.removeMember(element);
            return old_value;
        }

        default:
            throw std::domain_error("can't access component of scalar value");
    }
}


Value &Pointer::get(Value &root, size_t start_index) {
    return const_cast<Value &>(get_internal(root, start_index));
}


const Value &Pointer::get(const Value &root, size_t start_index) {
    return get_internal(root, start_index);
}


void Pointer::insert(Value &root, Value &value, size_t start_index) {
    if (start_index > elements.size()) {
        throw std::range_error("start_index out of range");
    }

    Value &last = const_cast<Value &>(get_internal(root, start_index, true));

    std::string &element = elements[elements.size() - 1];
    switch (last.type()) {
        case arrayValue: {
            Value::ArrayIndex index = static_cast<ArrayIndex>(parse_array_index(element, last.size(), true));
            last.resize(last.size() + 1);
            for (Value::ArrayIndex i = last.size() - 1; i > index; --i) {
                last[i] = last[i-1];
            }
            last[index] = value;
            break;
        }

        case objectValue:
            if (last.isMember(element)) {
                throw std::range_error("member '" + element + "' already exists");
            }
            last[element] = value;
            break;

        default:
            throw std::domain_error("can't access component of scalar value");
    }
}


Value &Pointer::replace(Value &root, const Value &value, size_t start_index) {
    if (start_index > elements.size()) {
        throw std::range_error("start_index out of range");
    }

    Value &last = const_cast<Value &>(get_internal(root, start_index, true));

    std::string &element = elements[elements.size() - 1];
    switch (last.type()) {
        case arrayValue: {
            Value::ArrayIndex index = static_cast<ArrayIndex>(parse_array_index(element, last.size(), false));
            Value &old_value = last[index];
            last[index] = value;
            return old_value;
        }

        case objectValue: {
            if (!last.isMember(element)) {
                throw std::range_error("member '" + element + "' doesn't exists");
            }
            Value &old_value = last[element];
            last[element] = value;
            return old_value;
        }

        default:
            throw std::domain_error("can't access component of scalar value");
    }
}


Value *Pointer::set(Value &root, const Value &value, size_t start_index) {
    if (start_index > elements.size()) {
        throw std::range_error("start_index out of range");
    }

    Value &last = const_cast<Value &>(get_internal(root, start_index, true));

    std::string &element = elements[elements.size() - 1];
    switch (last.type()) {
        case arrayValue: {
            Value::ArrayIndex index = static_cast<ArrayIndex>(parse_array_index(element, last.size(), true));
            Value *old_value = NULL;
            if (index <= last.size()) {
                old_value = &last[index];
            }
            last[index] = value;
            return old_value;
        }

        case objectValue: {
            Value *old_value = NULL;
            if (last.isMember(element)) {
                old_value = &last[element];
            }
            last[element] = value;
            return old_value;
        }

        default:
            throw std::domain_error("can't access component of scalar value");
    }
}


const Value &Pointer::get_internal(const Value &root, size_t start_index, bool skip_last) {
    size_t end_index = elements.size() - (skip_last ? 1 : 0);

    if (start_index == end_index) {
        // TODO: error?
        return root;
    }
    if (start_index > end_index) {
        throw std::range_error("start_index out of range");
    }

    const Value *node = &root;


    for (size_t index = start_index; index < end_index; index++) {
        std::string &element = elements[index];

        switch (node->type()) {
            case arrayValue:
                node = &(*node)[static_cast<ArrayIndex>(parse_array_index(element, node->size()))];
                break;

            case objectValue:
                if (!node->isMember(element)) {
                    throw std::range_error("member '" + element + "' doesn't exist");
                }
                node = &(*node)[element];
                break;

            default:
                throw std::domain_error("can't access component of scalar value");
        }
    }

    return *node;
}


size_t Pointer::parse_array_index(const std::string &element, size_t size, bool allow_growing) const {
    if (element.size() == 0) {
        throw std::range_error("invalid array index ''");
    }

    char *end;
    intmax_t i = strtoimax(element.c_str(), &end, 10);
    if (*end != '\0') {
        throw std::range_error("invalid array index '" + element + "'");
    }
    if (i < 0 || static_cast<size_t>(i) >= size) {
        throw std::range_error("index " + element + " out of range");
    }

    return static_cast<Value::ArrayIndex>(i);
}


std::string Pointer::as_string() const {
    std::stringstream stream;

    for (std::vector<std::string>::const_iterator it = elements.begin(); it != elements.end(); ++it) {
        stream << "/";
        encode(*it, stream);
    }

    return stream.str();
}


std::string Pointer::decode_fragment(const std::string &fragment) {
    size_t j = fragment.find_first_of("+%");
    
    if (j == std::string::npos) {
        return fragment.substr(1);
    }
    
    size_t i = 1;
    std::stringstream str;
    
    while (j != std::string::npos) {
        str << fragment.substr(i, j-i);
        
        if (fragment[j] == '+') {
            str << " ";
            j++;
        }
        else {
            if (j+2 >= fragment.size()) {
                throw std::invalid_argument("invalid % escape");
            }
            str << decode_hex(fragment, j+1);
            j += 3;
        }
        
        i = j;
        j = fragment.find_first_of("+%", j);
    }
    
    str << fragment.substr(i);
    
    return str.str();
}

char Pointer::decode_hex(const std::string &escape, size_t pos) {
    unsigned char c = 0;
    
    for (size_t i = 0; i < 2; i++) {
        c *= 16;
        
        char digit = escape[pos + i];
        
        if (digit >= '0' && digit <= '9') {
            c += digit - '0';
        }
        else if (digit >= 'a' && digit <= 'f') {
            c += digit - 'a' + 10;
        }
        else if (digit >= 'A' && digit <= 'F') {
            c += digit - 'A' + 10;
        }
        else {
            throw std::invalid_argument("invalid % escape");
        }
    }
    
    return static_cast<char>(c);
}

}
