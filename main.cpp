#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <map>
#include <vector>
#include "tokenizer.h"

using namespace std;

// The name of the input data file
string filename = "fpga_reg.h";

// The (optional) name of the config file
string config_file;

// Maps a register name to its 32-bit address
map<string, uint32_t> symbol;

// This is a list of which symbols we want to output, and what they should
// be translated to
map<string, string> translate;

void read_file(string fn);
void parse_command_line(const char** argv);
void read_config_file(string fn);
void display_output();


//=============================================================================
// main() - Execution starts here
//=============================================================================
int main(int argc, const char** argv)
{
    // Parse the command line
    parse_command_line(argv);

    // If there is a configuration file, read it
    if (!config_file.empty()) read_config_file(config_file);

    // Now read the input file
    read_file(filename);   

    // And display our output
    display_output();
}
//=============================================================================



//=============================================================================
// parse_command_line() - Parses the command line parameters
//=============================================================================
void parse_command_line(const char** argv)
{
    // Loop through all of the arguments
    while (*(++argv))
    {
        // Fetch this argument from the command line
        string token = *argv;

        // Did the user give us the name of a config file?
        if (token == "-config" && argv[1])
        {
            config_file = *(++argv);
            continue;
        }

        // Any other command-line switch is invalid
        if (token[0] == '-')
        {
            fprintf(stderr, "invalid argument %s\n", token.c_str());
            exit(1);
        }

        // If we get here, this is the name of our input file
        filename = token;

    }
}
//=============================================================================


//=============================================================================
// read_file() - Fills in the "symbol" map that maps a register name to a 
//               32-bit address
//=============================================================================
void read_file(string fn)
{
    char line[10000];
    CTokenizer tokenizer;

    // Open the input file
    FILE* ifile = fopen(fn.c_str(), "r");

    // If we can't open the input file, complain
    if (ifile == nullptr)
    {
        fprintf(stderr, "fpgaregs: can't open '%s'\n", fn.c_str());
        exit(1);
    }

    while (fgets(line, sizeof(line), ifile))
    {
        // Point to the first character of the line
        char* in = line;
        
        // Skip over spaces and tabs
        while (*in == 32 || *in == 9) ++in;

        // If it's the end of the line, ignore the line
        if (*in == 10 || *in == 13 || *in == 0) continue;

        // If it's a comment, ignore the line
        if (in[0] == '/' && in[1] == '/') continue;

        // Parse this line into tokens
        vector<string> token = tokenizer.parse(in);

        // If there aren't exactly 3 tokens on the line, ignore the line
        if (token.size() != 3) continue;

        // If the first token isn't "#define", ignore the line
        if (token[0] != "#define") continue;

        // The 2nd token on the line is the name of the register
        string register_name = token[1];

        // The 3rd token on the line is the 64-bit address of the register
        uint64_t register_addr = strtoull(token[2].c_str(), nullptr, 0);

        // If the upper 32-bits of the register address aren't zero, skip it
        if (register_addr >> 32) continue;

        // Store this symbol and its 32-bit address
        symbol[register_name] = (uint32_t)(register_addr & 0xFFFFFFFF);
    }

    // We're done with the input file
    fclose(ifile);
}
//=============================================================================



//=============================================================================
// read_config_file() - Reads the configuration file into the "translate" map
//=============================================================================
void read_config_file(string fn)
{
    char line[10000];
    CTokenizer tokenizer;

    // Open the input file
    FILE* ifile = fopen(fn.c_str(), "r");

    // If we can't open the input file, complain
    if (ifile == nullptr)
    {
        fprintf(stderr, "fpgaregs: can't open '%s'\n", fn.c_str());
        exit(1);
    }

    // Loop through each line of the file
    while (fgets(line, sizeof(line), ifile))
    {
        // If there is an "=" in the line, find it
        char* in = strchr(line, '=');

        // If we found a "=", replace it with a space
        if (in) *in = ' ';

        // Point to the first character of the line
        in = line;

        // Skip over spaces and tabs
        while (*in == 32 || *in == 9) ++in;

        // If it's the end of the line, ignore the line
        if (*in == 10 || *in == 13 || *in == 0) continue;

        // If it's a comment, ignore the line
        if (in[0] == '/' && in[1] == '/') continue;

        // Parse this line into tokens
        vector<string> token = tokenizer.parse(in);

        // Place these two symbol names into the "translate" map
        if (token.size() == 1)
            translate[token[0]] = token[0];
        else
            translate[token[0]] = token[1];
    }

    // We're done with the input file
    fclose(ifile);
}
//=============================================================================



//=============================================================================
// display_output() = Displays a list of register names and their addresses
//=============================================================================
void display_output()
{
    // Are we using a configuration file?
    bool have_config = config_file != "";

    // Loop through each symbol
    for (auto it=symbol.begin(); it != symbol.end(); ++it)
    {
        // Fetch the name and value of this symbol
        string name    = it->first;
        uint64_t value = it->second;

        // Are we using a configuration file?
        if (have_config)
        {
            // Is this symbol name one of the ones we're supposed to output?
            auto translation = translate.find(name);

            // If not, then ignore this symbol
            if (translation == translate.end()) continue;

            // While we're here, find out what the translated symbol name is
            name = translation->second;
        }

        // Display this symbol and value in a format that bash can evaluate
        printf("%s=$((0x%lX))\n", name.c_str(), value);

    }
}
//=============================================================================