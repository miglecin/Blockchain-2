CXX=g++
CXXFLAGS=-std=c++17 -O2 -Iinclude -Imy_hash

SRC=src/main.cpp src/blockchain_state.cpp src/blockchain_mining.cpp src/blockchain_print.cpp src/hash_adapter.cpp my_hash/hash.cpp
BIN=blockchain

all: $(BIN)

$(BIN): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $(SRC)

clean:
	rm -f $(BIN)
