#!/bin/bash

# Configurações
SERVER=./server
CLIENT=./ctestes
NUM_CLIENTS=5

echo "=== [1] Compilando servidor e cliente ==="
g++ -pthread -std=c++17 server.cpp -o server
g++ -pthread -std=c++17 cliente_testes.cpp -o ctestes

if [[ ! -f "$SERVER" || ! -f "$CLIENT" ]]; then
  echo "[ERRO] Falha na compilação."
  exit 1
fi

echo
echo "=== [2] Iniciando servidor ==="
$SERVER &
SERVER_PID=$!
sleep 1
echo "Servidor rodando (PID $SERVER_PID)"
echo

echo "=== [3] Iniciando $NUM_CLIENTS clientes simultaneamente ==="

# Criar arquivo de sincronização
SIGNAL_FILE="/tmp/start_clients.$$"
rm -f "$SIGNAL_FILE"

for i in $(seq 1 $NUM_CLIENTS); do
  (
    # Cada cliente espera até o sinal ser dado
    while [ ! -f "$SIGNAL_FILE" ]; do
      sleep 0.01
    done
    echo "[CLIENTE $i] iniciado"
    $CLIENT
  ) &
done

# Dá tempo para os clientes subirem e esperarem
sleep 1
echo ">>> Todos os clientes vão começar AGORA!"
echo "go" > "$SIGNAL_FILE"

# Espera todos os clientes terminarem
wait

echo
echo "=== [4] Finalizando servidor ==="
kill $SERVER_PID 2>/dev/null

echo "=== Teste concluído ==="
