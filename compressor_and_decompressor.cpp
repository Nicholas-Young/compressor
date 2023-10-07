#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <queue>
#include "data_key.cpp"

class compressor {
private:
    char* file_contents;
    size_t file_len;
    char print_len_size_t;

    size_t min_replications;
    size_t min_symbol_len;
    size_t max_symbol_len;

    std::ifstream input_file_stream;
    std::ofstream output_file_stream;

    std::unordered_map<data_key, std::vector<size_t>> current_symbol_map;
    std::unordered_map<data_key, std::vector<size_t>> next_symbol_map;
    std::vector <std::pair<data_key, std::vector<size_t>>> all_symbols;

    size_t num_symbols = 0;

    std::priority_queue<size_t, std::vector<size_t>, std::greater<size_t>> removed_bytes;

    char calculate_bytes_needed_to_represent(size_t input) {
        char bytes = 0;
        while (input != 0) {
            input >>= 8;
            bytes++;
        }
        return bytes;
    }

    void remove_vector_overlaps(std::vector<size_t>& input, size_t size) {
        if (input.size() > 0) {
            size_t base = input.at(0);
            for (auto vector_iterator = ++input.begin(); vector_iterator != input.end();) {
                if ((*vector_iterator) < base + size) {
                    vector_iterator = input.erase(vector_iterator);
                }
                else {
                    base = *vector_iterator;
                    vector_iterator++;
                }
            }
        }
    }

    void remove_internal_overlaps(std::pair<data_key, std::vector<size_t>>& symbol) {
        size_t size = symbol.first.symbol_length;
        remove_vector_overlaps(symbol.second, size);
    }

    void get_file_len() {
        //Assert file open?
        input_file_stream.seekg(0, input_file_stream.end);
        file_len = input_file_stream.tellg(); //Check if -1?
        input_file_stream.seekg(0, input_file_stream.beg);
    }

    void open_file(const char* file_name) {
        input_file_stream.open(file_name, std::ios::binary);
    }

    void read_file() {
        file_contents = new char[file_len];
        input_file_stream.read(file_contents, file_len);
    }

    data_key make_key(size_t start, size_t length) {
        return data_key{ file_contents + start, length };
    }

    void build_shortest_symbol_map(size_t shortest_symbol_length) {
        for (size_t data_index = 0; data_index < (file_len - shortest_symbol_length + 1); data_index++) {
            current_symbol_map[make_key(data_index, shortest_symbol_length)].push_back(data_index);
        }
    }

    void add_current_symbol_map() {
        for (auto map_item : current_symbol_map) {
            all_symbols.push_back({ map_item.first, map_item.second });
        }
    }

    void move_next_to_current() {
        current_symbol_map = next_symbol_map;
        next_symbol_map.clear();
    }

    void build_next_symbol_map() {
        for (auto map_iterator = current_symbol_map.begin(); map_iterator != current_symbol_map.end();) {
            if (map_iterator->first.symbol_length < max_symbol_len) {
                for (auto vector_iterator = map_iterator->second.begin(); vector_iterator != map_iterator->second.end();) {
                    if (*vector_iterator + map_iterator->first.symbol_length < file_len) {
                        next_symbol_map[make_key(*vector_iterator, map_iterator->first.symbol_length + 1)].push_back(*vector_iterator);
                        if (next_symbol_map[make_key(*vector_iterator, map_iterator->first.symbol_length + 1)].size() >= min_replications) {
                            vector_iterator = map_iterator->second.erase(vector_iterator);
                        }
                        else {
                            vector_iterator++;
                        }
                    }
                    else {
                        vector_iterator++;
                    }
                }
                if (map_iterator->second.size() < min_replications) {
                    map_iterator = current_symbol_map.erase(map_iterator);
                }
                else {
                    map_iterator++;
                }
            }
            else {
                map_iterator++;
            }
        }
    }

    void prune_current_symbol_map() {
        for (auto map_iterator = current_symbol_map.begin(); map_iterator != current_symbol_map.end();) {
            remove_vector_overlaps(map_iterator->second, map_iterator->first.symbol_length);
            if (map_iterator->second.size() < min_replications) {
                map_iterator = current_symbol_map.erase(map_iterator);
            }
            else {
                map_iterator++;
            }
        }
    }

    //Probably not the best "weight" function but it's probably good enough
    void sort_all_symbols() {
        std::sort(all_symbols.begin(), all_symbols.end(), [this](std::pair<data_key, std::vector<size_t>> left, std::pair<data_key, std::vector<size_t>> right) {
            remove_internal_overlaps(left);
            remove_internal_overlaps(right);
            size_t orig_len_left = left.first.symbol_length * left.second.size();
            size_t new_len_left = print_len_size_t + left.first.symbol_length + print_len_size_t + left.second.size() * print_len_size_t;
            size_t orig_len_right = right.first.symbol_length * right.second.size();
            size_t new_len_right = print_len_size_t + right.first.symbol_length + print_len_size_t + right.second.size() * print_len_size_t;
            return (new_len_left - orig_len_left) > (new_len_right - orig_len_right);
            });
    }

    void write_size_t_binary_little_endian(const size_t& data, char size = sizeof(size_t)) {
        char* bytes = new char[size];
        size_t mask = 0xFF;

        for (int byte = 0; byte < size; byte++) {
            bytes[byte] = (data & mask) >> (8 * byte);
            mask = mask << 8;
        }

        output_file_stream.write(bytes, size);
        delete[] bytes;
    }

    void write_initial_bytes() {
        output_file_stream.put(print_len_size_t);
        output_file_stream.write("00000000", print_len_size_t);
        write_size_t_binary_little_endian(file_len, print_len_size_t);
    }

    void add_bytes_to_remove(size_t start, size_t len) {
        for (size_t byte_index = 0; byte_index < len; byte_index++) {
            removed_bytes.push(start + byte_index);
        }
    }

    void remove_overlaps(std::pair<data_key, std::vector<size_t>>& symbol) {
        size_t used_symbol_len = symbol.first.symbol_length;
        for (size_t replication_index = 0; replication_index < symbol.second.size(); replication_index++) {
            size_t base = symbol.second.at(replication_index);
            for (auto vector_iterator = all_symbols.begin(); vector_iterator != all_symbols.end();) {
                size_t checking_symbol_len = vector_iterator->first.symbol_length;
                vector_iterator->second.erase(std::remove_if(vector_iterator->second.begin(), vector_iterator->second.end(), [used_symbol_len, base, checking_symbol_len](size_t start_index) {
                    return (start_index < (base + used_symbol_len)) && ((start_index + checking_symbol_len) > base);
                    }), vector_iterator->second.end());

                if (vector_iterator->second.size() < min_replications) {
                    vector_iterator = all_symbols.erase(vector_iterator);
                }
                else {
                    vector_iterator++;
                }
            }
        }
    }

    void handle_symbol(std::pair<data_key, std::vector<size_t>>& symbol) {
        remove_internal_overlaps(symbol);
        if (symbol.second.size() >= min_replications) {
            write_size_t_binary_little_endian(symbol.first.symbol_length, print_len_size_t);
            output_file_stream.write(symbol.first.symbol, symbol.first.symbol_length);
            write_size_t_binary_little_endian(symbol.second.size(), print_len_size_t);
            for (size_t vector_index = 0; vector_index < symbol.second.size(); vector_index++) {
                write_size_t_binary_little_endian(symbol.second.at(vector_index), print_len_size_t);
                add_bytes_to_remove(symbol.second.at(vector_index), symbol.first.symbol_length);
                remove_overlaps(symbol);
            }
            num_symbols++;
        }
    }

    void write_remaining_data() {
        for (size_t byte_index = 0; byte_index < file_len; byte_index++) {
            if (!removed_bytes.empty() && byte_index == removed_bytes.top()) {
                removed_bytes.pop();
            }
            else {
                output_file_stream.put(file_contents[byte_index]);
            }
        }
    }

    void write_num_symbols(size_t num_symbols) {
        output_file_stream.seekp(1, output_file_stream.beg);
        write_size_t_binary_little_endian(num_symbols, print_len_size_t);
    }

public:
    compressor(const char* input_file_name, size_t minimum_replications, size_t min_len, size_t max_len) : min_replications(minimum_replications), min_symbol_len(min_len), max_symbol_len(max_len) {
        open_file(input_file_name);
        get_file_len();
        print_len_size_t = calculate_bytes_needed_to_represent(file_len);
        read_file();
    }

    void compress_file() {
        build_shortest_symbol_map(min_symbol_len);

        do {
            prune_current_symbol_map();
            build_next_symbol_map();
            add_current_symbol_map();
            move_next_to_current();
        } while (!current_symbol_map.empty());
    }

    void write_compressed_file(const char* output_file_name) {
        output_file_stream.open(output_file_name, std::ios::binary);

        write_initial_bytes();

        while (!all_symbols.empty()) {
            sort_all_symbols();

            std::pair<data_key, std::vector<size_t>> symbol = all_symbols.back();
            all_symbols.pop_back();

            handle_symbol(symbol);
        }

        write_remaining_data();

        write_num_symbols(num_symbols);

        output_file_stream.close();
    }
};

class decompressor {
private:
    char bytes_per_size_t;

    std::unordered_map<size_t, char> removed_bytes;

    std::ifstream input_file_stream;
    std::ofstream output_file_stream;

    size_t read_size_t_binary_little_endian(char size = sizeof(size_t)) {
        char* bytes = new char[size];

        input_file_stream.read(bytes, size);

        size_t recreated_data = 0;
        for (size_t byte = 0; byte < size; byte++) {
            recreated_data += (((size_t)(unsigned char)bytes[byte]) << (8 * byte));
        }

        delete[] bytes;

        return recreated_data;
    }

    void read_symbol() {
        size_t bytes_in_symbol = read_size_t_binary_little_endian(bytes_per_size_t);
        char* bytes = new char[bytes_in_symbol];

        input_file_stream.read(bytes, bytes_in_symbol);

        size_t symbol_replications = read_size_t_binary_little_endian(bytes_per_size_t);

        for (size_t replication = 0; replication < symbol_replications; replication++) {
            size_t byte_index = read_size_t_binary_little_endian(bytes_per_size_t);
            for (size_t byte = 0; byte < bytes_in_symbol; byte++) {
                removed_bytes[byte_index + byte] = bytes[byte];
            }
        }
    }

    void read_remaining_data_and_print(size_t orig_file_size) {
        for (size_t byte_index = 0; byte_index < orig_file_size; byte_index++) {
            if (removed_bytes.count(byte_index) == 1) {
                output_file_stream.put(removed_bytes[byte_index]);
            }
            else {
                output_file_stream.put(input_file_stream.get());
            }
        }
    }

public:
    decompressor(const char* input_file_name) {
        input_file_stream.open(input_file_name, std::ios::binary);
        bytes_per_size_t = input_file_stream.get();
    }

    void decompress_and_write_file(const char* output_file_name) {
        output_file_stream.open(output_file_name, std::ios::binary);

        size_t num_symbols = read_size_t_binary_little_endian(bytes_per_size_t);
        size_t orig_file_size = read_size_t_binary_little_endian(bytes_per_size_t);

        for (size_t symbol_index = 0; symbol_index < num_symbols; symbol_index++) {
            read_symbol();
        }

        read_remaining_data_and_print(orig_file_size);
    }
};