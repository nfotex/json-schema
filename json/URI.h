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

#ifndef JSON_URI_H
#define JSON_URI_H

#include <string>
#include <vector>

namespace Json {
#if 0
} // fix auto indent
#endif

class URI {
public:
    URI(const URI &other);
    URI() : scheme_present(false), authority_present(false), query_present(false), fragment_present(false), needs_update(false) { }
    URI(const std::string &uri);

    void clear_scheme() { needs_update = true; scheme = ""; scheme_present = false; }
    void clear_authority() { needs_update = true; authority = ""; authority_present = false; }
    void clear_query() { needs_update = true; query = ""; query_present = false; }
    void clear_fragment() { needs_update = true; fragment = ""; fragment_present = false; }

    void copy_scheme(const URI &other);
    void copy_authority(const URI &other);
    void copy_path(const URI &other) { set_path(other.get_path()); }
    void copy_query(const URI &other);
    void copy_fragment(const URI &other);

    const std::string &get_scheme() const { return scheme; }
    const std::string &get_authority() const { return authority; }
    const std::string &get_path() const { return path; }
    const std::string &get_query() const { return query; }
    const std::string &get_fragment() const { return fragment; }

    const std::string &get_uri() const { if (needs_update) { update(); } return uri; }

    bool has_scheme() const { return scheme_present; }
    bool has_authority() const { return authority_present; }
    bool has_query() const { return query_present; }
    bool has_fragment() const { return fragment_present; }

    void set_scheme(const std::string &value) { needs_update = true; scheme = value; scheme_present = true; }
    void set_authority(const std::string &value) { needs_update = true; authority = value; authority_present = true; }
    void set_path(const std::string &value) { needs_update = true; path = value; }
    void set_query(const std::string &value) { needs_update = true; query = value; query_present = true; }
    void set_fragment(const std::string &value) { needs_update = true; fragment = value; fragment_present = true; }

    URI resolve(const URI &reference) const;
    URI resolve(const std::string &reference) const { return resolve(URI(reference)); }

private:
    void parse();
    void update() const;

    static std::string remove_dot_segments(const std::string &path);
    std::string merge_path(const std::string &relative_path) const;

    static bool is_encoded(const std::string &str) { return str.find('%') != std::string::npos; }
    static std::string decode(const std::string &encoded);
    static char decode_hex(const std::string &escape, size_t pos);
    static void encode(std::stringstream &out, const std::string &decoded);

    bool scheme_present;
    bool authority_present;
    bool query_present;
    bool fragment_present;

    std::string scheme;
    std::string authority;
    std::string path;
    std::string query;
    std::string fragment;

    mutable std::string uri;
    mutable bool needs_update;
};

}

#endif //  JSON_URI_H
