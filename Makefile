# Compilador
CXX = g++
# Flags de compilação (C++17 + pthreads + warnings)
CXXFLAGS = -std=c++17 -Wall -pthread

# Arquivos fonte
SERVER_SRC = server.cpp
CLIENT_SRC = cliente.cpp

# Executáveis
SERVER_BIN = server
CLIENT_BIN = cliente

# Regra padrão: compilar tudo
all: $(SERVER_BIN) $(CLIENT_BIN)

# Compilar o servidor
$(SERVER_BIN): $(SERVER_SRC)
	$(CXX) $(CXXFLAGS) $^ -o $@

# Compilar o cliente
$(CLIENT_BIN): $(CLIENT_SRC)
	$(CXX) $(CXXFLAGS) $^ -o $@

# Limpeza dos binários
clean:
	rm -f $(SERVER_BIN) $(CLIENT_BIN)
