#!/bin/sh

set -e

IN=$1
OUT=$2

name=`basename "$IN"`

cat <<EOF > $OUT
// This file was automatically created from $name. Do not edit directly.

#include <json/SchemaValidator.h>
#include <string>

namespace Json {

const std::string SchemaValidator::meta_schema = "\\
EOF

sed -e 's/\\/\\\\/g' -e 's/"/\\"/g' -e 's/$/\\n\\/' $IN >> $OUT

cat <<EOF >> $OUT
";

}
EOF
