#include <iostream>
#include <iterator>
#include <string>
#include <regex>

int main(int argc, char** argv) {

    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <str>" << std::endl;
        return 0;
    }

    std::string str(argv[1]);
    std::regex re("([a-zA-Z_][a-zA-Z0-9_]*)(?::([0-9]+|client|admin))?(?:@(.+))?");
    std::smatch match;

    if (std::regex_search(str, match, re)) {

        std::cout << "Match size = " << match.size() << std::endl;

        for (unsigned i=0; i < match.size(); i++) {
            std::cout << "Match " << i << ": \"" << match.str(i) << "\" at position " << match.position(i) << std::endl;
        }

    }
    else {
        std::cout << "No match is found" << std::endl;
    }

}
