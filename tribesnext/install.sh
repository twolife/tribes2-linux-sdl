#!/bin/bash

set -euo pipefail

README=$(cat <<-END
Usage:
1. Copy all the files to a subdirectory of your game folder (eg: ~/games/tribes2/tribesnext)
2. Run this shell script with either 'patch' or 'unpatch'
   eg: ./install.sh patch
END
)

if ! command -v 7z &> /dev/null; then
    echo "Error: 7z is not installed." >&2
    exit 1
fi
if ! command -v patch &> /dev/null; then
    echo "Error: patch is not installed." >&2
    exit 1
fi
if ! command -v unzip &> /dev/null; then
    echo "Error: unzip is not installed." >&2
    exit 1
fi
if ! command -v zip &> /dev/null; then
    echo "Error: zip is not installed." >&2
    exit 1
fi

tribesnext4linux_directory=$(cd "$(dirname "$0")" && pwd)
pushd "${tribesnext4linux_directory}" > /dev/null

if [ ! -f ../tribes2.dynamic ] ; then
    echo "Can't find tribes2.dynamic"
    printf '%s\n' "$README"
    exit 1
fi

if [ $# -ne 1 ] ; then
    printf '%s\n' "$README"
    exit 1
fi

case $1 in
    patch)
        if [ ! -f ./TribesNext_rc2a.exe ] ; then
            if ! command -v wget &> /dev/null; then
                echo "Error: TribesNext_rc2a.exe not found and wget is not installed." >&2
                exit 1
            fi
            wget https://www.tribesnext.com/files/TribesNext_rc2a.exe
        fi
        7z e -y TribesNext_rc2a.exe base/T2csri.vl2 > /dev/null
        unzip -o T2csri.vl2 t2csri/crypto.rb t2csri/certstore.rb
        patch -p1 -i certstore.rb.patch
        zip T2csri.vl2 t2csri/certstore.rb
        rm t2csri/certstore.rb

        mv T2csri.vl2 ../base/
        mv t2csri/crypto.rb ../
        rmdir t2csri/

        cp rubybase_embedded.rb ../
        cp strcmp.cs ../base/

        pushd .. > /dev/null
        patch --batch --forward -p1 -i "${tribesnext4linux_directory}/console_start.cs.patch"
        popd > /dev/null

        echo "Install complete. Enjoy !"
        ;;

    unpatch)
        pushd .. > /dev/null
        patch --batch -p1 -R -i "${tribesnext4linux_directory}/console_start.cs.patch"
        rm base/T2csri.vl2
        rm base/strcmp.cs
        rm crypto.rb
        rm rubybase_embedded.rb
        popd > /dev/null

        echo "Uninstall completed."
        ;;

    *)
        printf '%s\n' "$README"
        exit 1
        ;;
esac

