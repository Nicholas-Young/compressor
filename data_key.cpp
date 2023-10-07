#include <functional>

struct data_key {
	char* symbol;
	size_t symbol_length;

	bool operator==(const data_key& other_key) const {
		if (symbol_length != other_key.symbol_length) {
			return false;
		}

		for (size_t data_index = 0; data_index < symbol_length; data_index++) {
			if (symbol[data_index] != other_key.symbol[data_index]) {
				return false;
			}
		}

		return true;
	}
};

template<>
struct std::hash<data_key> {
	std::size_t operator()(const data_key& input_key) const {
		//Based on boost's method of combining hashes
		size_t hash_value = 0;
		hash<char> hasher;
		for (size_t data_index = 0; data_index < input_key.symbol_length; data_index++) {
			hash_value ^= hasher(input_key.symbol[data_index]) + 0x9e3779b9 + (hash_value << 6) + (hash_value >> 2);
		}

		return hash_value;
	}
};