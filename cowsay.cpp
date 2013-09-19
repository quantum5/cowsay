#include <iostream>
#include <regex>
#include <vector>
#include <algorithm>
#include <cstdlib>
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
    if (PathFileExists(path.c_str()))
        cowpath.push_back(path);
    std::cout << path << '\n';
#else
    struct stat buf;
    if (stat(path, &buf) == 0 && S_ISDIR(st.st_mode))
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
    parser.add_option("-E", "--encoding").action("store").dest("encoding")
                     .set_default("utf-8").help("Encoding to use, utf-8 by default");
    parser.add_option("-W", "--wrap").action("store").type("int").dest("wrap")
                     .set_default(70).help("wraps the cow text, default 70");
    parser.add_option("--thoughts").action("store").dest("thoughts")
                     .help("the method of communication cow uses. "
                      "Default to `o` if invoked as cowthink, otherwise \\");
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
    
    for (auto i = cowpath.cbegin(); i < cowpath.cend(); ++i)
        std::cout << *i << '\n';
}
