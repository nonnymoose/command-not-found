/*
    This file is part of command-not-found.
    Copyright (C) 2011 Matthias Maennich <matthias@maennich.net>

    command-not-found is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    command-not-found is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with command-not-found.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <boost/regex.hpp>
#include <stdio.h>
#include <sstream>
#include <iostream>

#include "package.h"

namespace bf = boost::filesystem;
using namespace std;
using boost::regex;
using boost::regex_match;
using boost::cmatch;

namespace cnf {

Package::Package(const bf::path path, const bool lazy) throw (InvalidArgumentException)
        : itsFilesDetermined(false), itsPath(new bf::path(path)) {

    // checks
    if (!bf::is_regular_file(path)) {
        string message;
        message += "not a file: ";
        message += path.string();
        throw InvalidArgumentException(MISSING_FILE, message.c_str());
    }

    static const regex valid_name(
            "(.+)-(.+)-(.+)-(any|i686|x86_64).pkg.tar.(xz|gz)");

    cmatch what;

#if BOOST_FILESYSTEM_VERSION == 2
    const string filename = path.filename().c_str();
#else
    const string filename = path.filename().string();
#endif
    if (regex_match(filename.c_str(), what, valid_name)) {

        itsName = what[1];
        itsVersion = what[2];
        itsRelease = what[3];
        itsArchitecture = what[4];
        itsCompression = what[5];
    } else {
        string message;
        message += "this is not a valid package file: ";
        message += path.string();
        throw InvalidArgumentException(INVALID_FILE, message.c_str());

    }

    if (!lazy)
        updateFiles();
}

void Package::updateFiles() const {

    // read package file list

    //FIXME: this should be done in C++ with libtar

    if (!itsPath) {
        throw InvalidArgumentException(INVALID_FILE, "You should never end up here!");
    }

    string cmd("tar tf 2>&1 ");
    cmd.append(itsPath->string());

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        string message;
        message += "could not read file list from: ";
        message += itsPath->string();
        throw InvalidArgumentException(INVALID_FILE, message.c_str());
    }
    char buffer[256];
    string result;
    while (!feof(pipe)) {
        if (fgets(buffer, 256, pipe) != NULL
        )
            result += buffer;
    }
    pclose(pipe);

    istringstream iss(result);

    vector<string> candidates;
    copy(istream_iterator<string>(iss), istream_iterator<string>(),
         back_inserter<vector<string> >(candidates));

    const regex significant("((usr/)?(s)?bin/([0-9A-Za-z.-]+))");

    typedef vector<string>::const_iterator canditer;

    cmatch what;
    for (canditer iter(candidates.begin()); iter != candidates.end(); ++iter) {
        if (regex_match(iter->c_str(), what, significant)) {
            itsFiles.push_back(what[4]);
        }
    }

    itsFilesDetermined = true;

}

const string Package::hl_str(const string hl) const {
    stringstream out;
    out << name() << " (" << version() << "-" << release() << ") [ ";

    for (Package::const_file_iterator iter = files().begin();
            iter != files().end(); ++iter) {
        const char* color = (hl != "" && *iter == hl) ? "\033[0;31m" : "\033[0m";
        out << color << *iter << "\033[0m" << " ";
    }
    out << "]";
    return out.str();
}

ostream& operator<<(ostream& out, const Package& p) {
    out << p.hl_str();
    return out;
}

}
