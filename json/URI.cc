/*
    URI.cc -- acess parts of URIs
    Copyright 2018 nfotex IT DL GmbH.

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

#include <json/URI.h>

#include <sstream>
#include <stdexcept>

#include <pcrecpp.h>

namespace Json {
#if 0
} // fix auto indent
#endif

URI::URI(const URI &other) {
    copy_scheme(other);
    copy_authority(other);
    copy_path(other);
    copy_query(other);
    copy_fragment(other);
    if (other.needs_update == false) {
        uri = other.uri;
        needs_update = false;
    }
}

URI::URI(const std::string &uri_) : uri(uri_) {
    parse();
}


void URI::copy_scheme(const Json::URI &other) {
    if (other.has_scheme()) {
        set_scheme(other.get_scheme());
    }
    else {
        clear_scheme();
    }
}


void URI::copy_authority(const Json::URI &other) {
    if (other.has_authority()) {
        set_authority(other.get_authority());
    }
    else {
        clear_authority();
    }
}


void URI::copy_query(const Json::URI &other) {
    if (other.has_query()) {
        set_query(other.get_query());
    }
    else {
        clear_query();
    }
}


void URI::copy_fragment(const Json::URI &other) {
    if (other.has_fragment()) {
        set_fragment(other.get_fragment());
    }
    else {
        clear_fragment();
    }
}


// RFC 3986, 5.2.2. Transform References
URI URI::resolve(const URI &reference) const {
    if (reference.has_scheme()) {
        return reference;
    }

    URI resolved;

    if (reference.has_authority()) {
        resolved.copy_authority(reference);
        resolved.set_path(remove_dot_segments(reference.get_path()));
        resolved.copy_query(reference);
    }
    else {
        if (reference.get_path().empty()) {
            resolved.copy_path(*this);

            if (reference.has_query()) {
                resolved.copy_query(reference);
            }
            else {
                resolved.copy_query(*this);
            }
        }
        else {
            if (reference.get_path()[0] == '/') {
                resolved.set_path(remove_dot_segments(reference.get_path()));
            }
            else {
                resolved.set_path(remove_dot_segments(merge_path(reference.get_path())));
            }
            resolved.copy_query(reference);
        }
        resolved.copy_authority(*this);
    }
    resolved.copy_scheme(*this);
    resolved.copy_fragment(reference);

    return resolved;
}


// RFC 3986, Appendix B. Parsing a URI Reference with a Regular Expression
void URI::parse() {
    static pcrecpp::RE regex("^(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\\?([^#]*))?(#(.*))?");

    std::string scheme_sep;
    std::string authority_sep;
    std::string query_sep;
    std::string fragment_sep;

    regex.FullMatch(uri, &scheme_sep, &scheme, &authority_sep, &authority, &path, &query_sep, &query, &fragment_sep, &fragment);

    scheme_present = !scheme_sep.empty();
    authority_present = !authority_sep.empty();
    query_present = !query_sep.empty();
    fragment_present = !fragment_sep.empty();

    needs_update = false;

    if (is_encoded(authority)) {
        needs_update = true;
        authority = decode(authority);
    }
    if (is_encoded(path)) {
        needs_update = true;
        path = decode(path);
    }
    if (is_encoded(query)) {
        needs_update = true;
        query = decode(query);
    }
    if (is_encoded(fragment)) {
        needs_update = true;
        fragment = decode(fragment);
    }
}

// RFC 3986, 5.3.  Component Recomposition
void URI::update() const {
    // TODO: quoting
    std::stringstream out;

    if (has_scheme()) {
        out << get_scheme() << ':';
    }

    if (has_authority()) {
        out << "//";
        encode(out, get_authority());
    }

    encode(out, get_path());

    if (has_query()) {
        out << '?';
        encode(out, query);
    }

    if (has_fragment()) {
        out << '#';
        encode(out, fragment);
    }

    uri = out.str();
    needs_update = false;
}


std::string URI::remove_dot_segments(const std::string &path) {
    static pcrecpp::RE dotdot("(^|/)[^/]*/\\.\\.(/|$)");
    static pcrecpp::RE dot("(^|/)\\.(/|$)");
    static pcrecpp::RE leading_dotdot("^/?\\.\\./");

    std::string result = path;
    dot.GlobalReplace("/", &result);
    leading_dotdot.GlobalReplace("/", &result);
    dotdot.GlobalReplace("/", &result);
    return result;
}


// RFC 3986, 5.2.3.  Merge Paths
std::string URI::merge_path(const std::string &relative_path) const {
    if (has_authority() && get_path().empty()) {
        return "/" + relative_path;
    }

    auto pos = get_path().find_last_of('/');
    if (pos == std::string::npos) {
        pos = 0;
    }
    else {
        pos++;
    }
    return get_path().substr(0, pos) + relative_path;
}


std::string URI::decode(const std::string &encoded) {
    size_t j = encoded.find_first_of("%");

    if (j == std::string::npos) {
        return encoded;
    }

    size_t i = 0;
    std::stringstream str;

    while (j != std::string::npos) {
        str << encoded.substr(i, j-i);

        if (j+2 >= encoded.size()) {
            throw std::invalid_argument("invalid % escape");
        }
        str << decode_hex(encoded, j+1);

        i = j + 3;
        j = encoded.find_first_of("%", i);
    }

    str << encoded.substr(i);

    return str.str();
}

char URI::decode_hex(const std::string &escape, size_t pos) {
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


void URI::encode(std::stringstream &out, const std::string &decoded) {
    static char digits[] = "012345678abcdef";

    size_t j = decoded.find_first_of("?#%");

    size_t i = 0;

    while (j != std::string::npos) {
        out << decoded.substr(i, j-i);

        out << '%' << digits[decoded[j]>> 4] << digits[decoded[j] & 0xf];

        i = j + 1;
        j = decoded.find_first_of("?#%", i);
    }

    out << decoded.substr(i);
}

} // namespace Json
