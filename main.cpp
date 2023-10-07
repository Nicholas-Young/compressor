#include <cstring>
#include "compressor_and_decompressor.cpp"

void print_help() {
    const char* help_msg = "Command Line Options:\n"
        "--help (-h)\t\thelp\n"
        "--input (-i)\t\tInput file name (Required)\n"
        "--output (-o)\t\toutput file name (Required)\n"
        "--compress (-c)\t\tCompression mode (Either this or -d required)\n"
        "--decompress (-d)\tDecompression mode (Either this or -c required)\n"
        "--min_reps\t\tminimum replications\n"
        "--min_sym_len\t\tminimum symbol length\n"
        "--max_sym_len\t\tmaximum symbol length\n\n"
        "Example:\n"
        "\t--compress -i in.txt -o out.smol --max_sym_len 256\n"
        "Will compress in.txt with a max symbol length of 256, and output the result to out.smol";

    std::cout << help_msg << std::endl;
}

int find_arg_index(const char* arg, int argc, char* argv[]) {
    for (int arg_index = 0; arg_index < argc; arg_index++) {
        if (strcmp(arg, argv[arg_index]) == 0) {
            return arg_index;
        }
    }

    return -1;
}

int find_arg_index(const char* arg_long, const char* arg_short, int argc, char* argv[]) {
    for (int arg_index = 0; arg_index < argc; arg_index++) {
        if (strcmp(arg_long, argv[arg_index]) == 0 || strcmp(arg_short, argv[arg_index]) == 0) {
            return arg_index;
        }
    }

    return -1;
}

size_t set_value_if_specified(const char* arg, int argc, char* argv[], size_t default_value) {
    int index = find_arg_index(arg, argc, argv);
    if (index == -1) {
        return default_value;
    }
    else {
        return (size_t)atoi(argv[index + 1]);
    }
}

int main(int argc, char* argv[]) {
    if (find_arg_index("--help", "-h", argc, argv) != -1) {
        print_help();
        return 0;
    }

    int input_file_name_index = find_arg_index("--input", "-i", argc, argv);
    int output_file_name_index = find_arg_index("--output", "-o", argc, argv);
    int compress_index = find_arg_index("--compress", "-c", argc, argv);
    int decompress_index = find_arg_index("--decompress", "-d", argc, argv);

    if (input_file_name_index == -1) {
        std::cout << "Error: No input file specified" << std::endl;
        return 1;
    }

    if (output_file_name_index == -1) {
        std::cout << "Error: No output file specified" << std::endl;
        return 1;
    }

    if (compress_index == -1 && decompress_index == -1) {
        std::cout << "Error: Neither compression or decompression mode specified" << std::endl;
        return 1;
    }

    if (compress_index != -1 && decompress_index != -1) {
        std::cout << "Error: Both compression and decompression mode specified" << std::endl;
        return 1;
    }

    char* input_file_name = argv[input_file_name_index + 1];
    char* output_file_name = argv[output_file_name_index + 1];

    size_t min_replications = set_value_if_specified("--min_reps", argc, argv, 16);
    size_t min_symbol_length = set_value_if_specified("--min_sym_len", argc, argv, 8);
    size_t max_symbol_length = set_value_if_specified("--max_sym_len", argc, argv, 128);

    if (compress_index != -1) {
        compressor c(input_file_name, min_replications, min_symbol_length, max_symbol_length);
        c.compress_file();
        c.write_compressed_file(output_file_name);
    }
    else if (decompress_index != -1) {
        decompressor d = decompressor(input_file_name);
        d.decompress_and_write_file(output_file_name);
    }

    return 0;
}

// .smol file
//       1B      |      N      |        N       |       N      | <N>B |     4B      |      4B      |      4B      | ... |       4B     |       4B     | <N>B |     4B      | ...
//  Size(size_t) | Num Symbols | Orig File Size | Size of Orig | Orig | Num matches | Match addr 0 | Match addr 1 | ... | Match addr n | Size of Orig | Orig | Num matches | ...
//{                   HEADER                   } {                                      SYMBOL A                                      } {                SYMBOL B ...