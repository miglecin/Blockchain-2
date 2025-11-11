CXX = g++
CXXFLAGS = -std=c++17 -O2 -pthread -Iinclude -Imy_hash

# Libbitcoin + priklausomybės
LIBS = \
    -lbitcoin-system \
    -lsecp256k1 \
    -lsodium \
    -lboost_system \
    -lboost_filesystem \
    -lboost_program_options \
    -lboost_thread \
    -lboost_chrono \
    -lboost_date_time \
    -lboost_iostreams \
    -lboost_regex \
    -lboost_log \
    -lboost_log_setup \
    -lboost_locale \
    -lpthread

SRC = \
    src/main.cpp \
    src/blockchain_state.cpp \
    src/blockchain_mining.cpp \
    src/blockchain_print.cpp \
    src/hash_adapter.cpp \
    my_hash/hash.cpp

BIN = blockchain

all: $(BIN)

$(BIN): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $(SRC) $(LIBS)

clean:
	rm -f $(BIN)
