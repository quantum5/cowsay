#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <vector>
#include <algorithm>
#include <functional>
#include <cstdlib>
#include <cctype>
#include "OptionParser.h"

#ifdef WIN32
#   ifndef WIN32_LEAN_AND_MEAN
#       define WIN32_LEAN_AND_MEAN
#   endif
#   include <windows.h>
#   include <shlwapi.h>
#else
#   include <unistd.h>
#   include <sys/types.h>
#   include <sys/stat.h>
#   include <pwd.h>
#endif


bool get_executable_directory(std::string &buffer) {
#ifdef WIN32
    char name[MAX_PATH];
    if (!GetModuleFileName(NULL, name, MAX_PATH))
        return false;
    PathRemoveFileSpec(name);
    buffer = name;
    return true;
#else
    char path[512];
    int ch = readlink("/proc/self/exe", path, 512);
    if (ch != -1) {
        path[ch] = 0;
        buffer = path;
        buffer = buffer.substr(0, buffer.find_last_of("/"));
    }
    return ch != -1;
#endif
}

void replace(std::string &string, const std::string &find, const std::string &replace) {
    size_t index = 0;
    while (true) {
        index = string.find(find, index);
        if (index == std::string::npos)
            break;

        string.replace(index, find.length(), replace);
        index += replace.length();
    }
}

std::string get_home_directory() {
    static std::string home;
    
    if (!home.empty())
        return home;
    
    char *env = getenv("HOME");
    if (env)
        return home = env;
#ifdef WIN32
    env = getenv("USERPROFILE");
    if (env)
        return home = env;
    
    home = getenv("HOMEDRIVE");
    home += getenv("HOMEPATH");
    return home;
#else
    struct passwd *pw = getpwuid(getuid());
    return home = pw->pw_dir;
#endif
}

std::string expanduser(const std::string &path) {
    std::string copy(path);
    replace(copy, "~", get_home_directory());
    return copy;
}

void add_cow_path_if_exists(std::vector<std::string> &cowpath, const std::string &path) {
#ifdef WIN32
    if (PathIsDirectory(path.c_str()))
        cowpath.push_back(path);
#else
    struct stat buf;
    if (stat(path.c_str(), &buf) == 0 && S_ISDIR(buf.st_mode))
        cowpath.push_back(path);
#endif
}

void add_default_cowpath(std::vector<std::string> &cowpath) {
    std::string path;
    if (!get_executable_directory(path))
        return;
    
    add_cow_path_if_exists(cowpath, expanduser("~/.cows"));
    add_cow_path_if_exists(cowpath, expanduser("~/cows"));
    add_cow_path_if_exists(cowpath, path + "/../share/cows");
    add_cow_path_if_exists(cowpath, path + "/../share/cows");
    add_cow_path_if_exists(cowpath, "/usr/share/cows");
    add_cow_path_if_exists(cowpath, "/usr/local/share/cows");
    add_cow_path_if_exists(cowpath, path + "/cows");
}

bool file_exist(const std::string &file) {
#ifdef WIN32
    return PathFileExists(file.c_str());
#else
    struct stat buf;
    return stat(file.c_str(), &buf) == 0 && S_ISREG(buf.st_mode);
#endif
}

bool endswith(std::string const &string, std::string const &ending) {
    if (string.length() >= ending.length()) {
        return !string.compare(string.length() - ending.length(), ending.length(), ending);
    } else {
        return false;
    }
}

bool startswith(std::string const &string, std::string const &ending) {
    if (string.length() >= ending.length()) {
        return !string.compare(0, ending.length(), ending);
    } else {
        return false;
    }
}

std::string findcow(const std::vector<std::string> &cowpath, const std::string &cow) {
    if (file_exist(cow))
        return cow;
    for (auto i = cowpath.cbegin(); i < cowpath.cend(); ++i) {
        std::string check = *i + "/" + cow;
        if (file_exist(check))
            return check;
    }
    if (endswith(cow, ".cow"))
        throw std::string("Cow exists not: ") + cow;
    return findcow(cowpath, cow + ".cow");
}

static inline std::string &ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

static inline std::string &rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}

static inline std::string &trim(std::string &s) {
        return ltrim(rtrim(s));
}

int isnewline(int ch) {
    switch (ch) {
        case '\r':
        case '\n':
        case '\f':
            return 1;
    }
    return 0;
}

std::string loadcow(const std::string &file, const std::string &thoughts,
                    const std::string &eyes, const std::string &tongue) {
    if (!file_exist(file))
        throw std::string("Can't find cow: ") + file;
    std::string cow, buf;
    
    try {
        std::ifstream cowfile(file);
        if (!cowfile)
            throw std::string("Can't open cow: ") + file;
        cowfile.exceptions(std::ifstream::badbit);
        while (std::getline(cowfile, buf))
            if (!startswith(ltrim(std::string(buf)), "#"))
                cow += buf + '\n';
        cowfile.close();
    } catch (std::ifstream::failure e) {
        throw std::string("Can't open cow: ") + e.what();
    }
    
    if (cow.find("$the_cow") != std::string::npos) {
        // Perl format, 'tis because of thee that I need regex
        std::regex cowstart("\\$the_cow\\s*=\\s*<<[\"']?(\\w+)[\"']?;?$");
        std::smatch match;
        if (!regex_search(cow, match, cowstart))
            throw std::string("Can't find a perl cow declaration");
        int start = match.position() + match.length(), end;
        std::string heredoc = match.str(1);
        const std::regex esc("[\\^\\.\\$\\|\\(\\)\\[\\]\\*\\+\\?\\/\\\\]");
        const std::string escaped("\\\\\\1");
        std::regex cowend("^" + std::regex_replace(heredoc, esc, escaped) + "$");
        if (regex_search(cow, match, cowend))
            end = match.position();
        else
            end = cow.length();
        cow = cow.substr(start, end - start);
        cow = std::regex_replace(cow, std::regex("\\$\\{?thoughts(?:\\}|\\b)"), thoughts);
        cow = std::regex_replace(cow, std::regex("\\$\\{?eyes(?:\\}|\\b)"), eyes);
        cow = std::regex_replace(cow, std::regex("\\$\\{?tongue(?:\\}|\\b)"), tongue);
        replace(cow, "\\\\", "\\");
        replace(cow, "\\@", "@");
        cow.erase(cow.begin(), std::find_if(cow.begin(), cow.end(), std::not1(std::ptr_fun<int, int>(isnewline))));
    } else {
        // Now, my own cow format, just basic formatting
        rtrim(cow);
        replace(cow, "{thoughts}", thoughts);
        replace(cow, "{eyes}", eyes);
        replace(cow, "{tongue}", tongue);
    }
    return cow;
}

void write_ballon(FILE *out, const std::vector<std::string> &lines, int width, bool think=false) {
    std::stringstream formatter;
    formatter << "%c %-" << width << "s %c\n";
    width += 2;
    std::string format = formatter.str();
    fprintf(out, " %s \n", std::string(width, '_').c_str());
    if (think) {
        for (auto line = lines.cbegin(); line < lines.cend(); ++line)
            fprintf(out, format.c_str(), '(', line->c_str(), ')');
    } else if (lines.size() < 2) {
        fprintf(out, format.c_str(), '<', lines.size() ? lines[0].c_str() : "", '>');
    } else {
        auto line = lines.cbegin();
        auto end = lines.cend();
        --end;
        fprintf(out, format.c_str(), '/', (line++)->c_str(), '\\');
        for (; line < end; ++line)
            fprintf(out, format.c_str(), '|', line->c_str(), '|');
        fprintf(out, format.c_str(), '\\', line->c_str(), '/');
    }
    fprintf(out, " %s \n", std::string(width, '-').c_str());
}

int wrap(const std::string& input, std::vector<std::string>& result, size_t width) {
    std::string line;
    size_t index = 0, maxwidth = 0;
    while (index < input.length()) {
        int i = input.find_first_of(" \t\n", index);
        if (i == std::string::npos)
            i = input.length() - 1;
        if (line.length() + i - index > width) {
            rtrim(line);
            result.push_back(line);
            if (line.length() > maxwidth)
                maxwidth = line.length();
            line.clear();
        }
        line += input.substr(index, i - index) + " ";
        if (input[i] == '\n') {
            result.push_back(line);
            line.clear();
        }
        index = i + 1;
    }

    if (!line.empty()) {
        rtrim(line);
        result.push_back(line);
        if (line.length() > maxwidth)
            maxwidth = line.length();
    }
    return maxwidth;
}

void open_streams(std::string &data, const std::vector<std::string> &files) {
    if (!files.size()) {
        std::stringstream stream;
        stream << std::cin.rdbuf();
        data = stream.str();
        return;
    }
    data = "";
    for (auto file = files.cbegin(); file < files.cend(); ++file) {
        if (*file == "-") {
            std::stringstream stream;
            stream << std::cin.rdbuf();
            data += stream.str();
            continue;
        }
        std::ifstream stream;
        stream.exceptions(std::ifstream::badbit);
        try {
            stream.open(*file);
            std::stringstream string;
            string << stream.rdbuf();
            data += string.str();
        } catch (std::ifstream::failure e) {
            std::cerr << "Can't open file: " << *file << ": " << e.what() << std::endl;
        }
    }
}

int main(int argc, char *argv[]) {
    optparse::OptionParser parser = optparse::OptionParser()
          .description("C++ reimplementation of the classic cowsay.");
    parser.set_defaults("eyes", "oo");
    parser.set_defaults("tongue", "");
    parser.set_defaults("thoughts", strstr(argv[0], "think") ? "o" : "\\");
    parser.add_option("-b", "--borg").    action("store_const").dest("eyes").set_const("==");
    parser.add_option("-d", "--dead").    action("store_const").dest("eyes").set_const("xx");
    parser.add_option("-g", "--greedy").  action("store_const").dest("eyes").set_const("$$");
    parser.add_option("-p", "--paranoid").action("store_const").dest("eyes").set_const("@@");
    parser.add_option("-s", "--stoned").  action("store_const").dest("eyes").set_const("**");
    parser.add_option("-t", "--tired").   action("store_const").dest("eyes").set_const("--");
    parser.add_option("-w", "--wired").   action("store_const").dest("eyes").set_const("OO");
    parser.add_option("-y", "--young").   action("store_const").dest("eyes").set_const("..");
    parser.add_option("-e", "--eyes").    action("store").dest("eyes");
    parser.add_option("-T", "--tongue").  action("store").dest("tongue");
    parser.add_option("-l", "--list").action("store_true").dest("list")
                     .help("displays cow file location");
    parser.add_option("-f", "--file").action("store").dest("file")
                     .set_default("default.cow").help("cow file, searches in cowpath. "
                      ".cow is automatically appended");
    parser.add_option("-W", "--wrap").action("store").type("int").dest("wrap")
                     .set_default(70).help("wraps the cow text, default 70");
    parser.add_option("--thoughts").action("store").dest("thoughts")
                     .help("the method of communication cow uses. "
                      "Default to `o` if invoked as cowthink, otherwise `\\`");
    parser.add_option("-c", "--command-line").action("store_true").dest("cmd")
                     .help("treat command line as text, not files");
    std::string cowpath_opts[5] = {"-a", "--add", "--add-cow", "--add-path", "--add-cow-path"};
    parser.add_option(std::vector<std::string>(&cowpath_opts[0], &cowpath_opts[5]))
                     .action("append").dest("cowpath");

    optparse::Values& options = parser.parse_args(argc, argv);
    std::vector<std::string> args = parser.args();
    std::vector<std::string> cowpath(options.all("cowpath").cbegin(), options.all("cowpath").cend());
    std::reverse(cowpath.begin(), cowpath.end());
    add_default_cowpath(cowpath);
    
    std::string tongue, eyes;
    eyes = (options["eyes"] + "  ").substr(0, 2);
    if (options["tongue"].empty()) {
        if (eyes == "xx" || eyes == "**") // one of the predefined dead faces
            tongue = "U ";
        else
            tongue = "  ";
    } else
        tongue = (options["tongue"] + "  ").substr(0, 2);
    
    if (options.is_set("list")) {
        try {
            std::string path = findcow(cowpath, options["file"]);
#ifdef WIN32
            replace(path, "/", "\\");
#endif
            std::cout << path << std::endl;
            return 0;
        } catch (std::string e) {
            std::cerr << argv[0] << ": " << e << std::endl;
            return 1;
        }
    }
    
    /*for (auto i = cowpath.cbegin(); i < cowpath.cend(); ++i)
        std::cout << *i << '\n';*/
    std::string cow;
    std::vector<std::string> lines;
    std::string input;
    try {
        cow = loadcow(findcow(cowpath, options["file"]), options["thoughts"], eyes, tongue);
        open_streams(input, args);
        int width = wrap(input, lines, options.get("wrap"));
        write_ballon(stdout, lines, width, options["thoughts"] == "o");
        fputs(cow.c_str(), stdout);
    } catch (std::string e) {
        std::cerr << argv[0] << ": " << e << std::endl;
    }
}
