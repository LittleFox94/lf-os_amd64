#!/bin/sh

for arg in "$@"; do
    if echo "$arg" | grep ".xml"; then
        outfile=$(echo "$arg" | sed 's/^.*=\(.*\)$/\1/')
    fi
done

export startTimestamp=$(date +%s)

"$@" > "$outfile.qemu-log" 2>&1

yq eval '
    with(
        .. | select(has("+@timestamp"));
        with_dtf(
            "2006-01-02T15:04:05.999999999";
            .+@timestamp |= . + (
                env(startTimestamp)
            ) + "s"
        )
    )'                          \
    -p xml -o xml               \
    --xml-attribute-prefix +@   \
    --inplace "$outfile"        \

yq \
    eval '.testsuites.+@failures == 0 and .testsuites.+@errors == 0' \
    -p xml -e --xml-attribute-prefix +@ \
    "$outfile"
