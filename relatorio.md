# Relatório final 
## Visão geral
O sistema implementa um sistema simples de comunicação multi-usuários utilizando sockets e C++, se utilizando de uma estrutura servidor-cliente. O código permite que vários usuários acessem cinco diferentes salas (o que pode ser facilmente alterável) e troquem mensagens em tempo real entre si, isso com logging integrado.


## Diagrama Cliente-Servidor
![Diagrama da arquitetura do projeto](https://github.com/JosueGuedes/ProjetoFinal-LP2/blob/main/DiagramaFinal.jpeg)

## Mapeamento de requisitos:
### De tema
- Servidor TCP concorrente aceitando múltiplos clientes: Utilizado por meio do uso de threads para atendimento múltiplo e uso de sockets e AF_INET.
- Cada cliente atendido por thread, mensagens retransmitidas para os demais (broadcast): Função thread `mandar_mensagem` em `server.cpp`
- Logging concorrente de mensagens: Espalhado pelo código nas funções da família `log_`. Exemplos incluem as linhas 261 e 271 em `server.cpp`
- Cliente CLI: Conexão na linha `275` de `server.cpp` e a linha `60` de `cliente.cpp`, envio de mensagens no segundo arquivo mencionado (e broadcast em `mandar_mensagem` do primeiro arquivo) e recepção em cliente.cpp.
- Proteção de Estruturas compartilhadas: Utilização de mutexes e semaforos a fim de garantir integridade dos dados.

### Gerais
- Threads: Utilizado como maior força motriz do código.
- Exclusão mútua: Mutex é utilizado 
- Semaforos e condvars: Utilizados em `server.cpp` para inicialização dos struct chat e sua sincronia.
- Monitores: struct `chat` utiliza um encapsulamento de seus próprios mutexes e dados.
- Sockets: Utilizados para montar conexão entre cliente e servidor.
- Tratamento de erros: Checagem constante de integridade dos dados e operações feitas.
- Logging concorrente: Espalhado pelo código nas funções da família `log_`. Exemplos incluem as linhas 261 e 271 em `server.cpp`
- Documentação: Ver secção acima.
- Build: Arquivo `Makefile` presente e explicado como usar em `README.MD`.
- Uso de IA: Ver secção abaixo.

## Relatório de LLM (Deepseek)
Prompt utilizado:
> " Dado o seguinte projeto, me dê um relatorio a respeito de potenciais race conditions, pontos fortes, deadlocks e possíveis sugestões:"

Seguido do código de cliente.cpp, server.cpp e logger.hpp

Retorno:
### Pontos Fortes
#### 1. Arquitetura Bem Estruturada
- Separação clara entre threads de broadcast e threads de cliente
- Uso de estruturas dedicadas por sala (chat)
- Sistema de logging thread-safe

#### 2. Sincronização Adequada em Áreas Críticas
- Mutexes para proteção de listas de clientes e histórico de mensagens
- Uso de condition variables para notificação de novas mensagens
- Semáforo para inicialização controlada das salas

#### 3. Gerenciamento de Recursos
- Destruição adequada de mutexes e condition variables no destrutor
- Limpeza completa durante shutdown
- Uso de RAII implícito através de destrutores

4. Tratamento de Sinais
- Handler para SIGINT permite shutdown gracioso
- Mecanismo para interromper accept() bloqueante

### Potenciais race-conditions (trecho)
#### 2. Verificação de running
```cpp
while (running) {
    pthread_cond_wait(&sala->cond_new_msg, &sala->mutex_historico);
}
```

Problema: Race condition entre verificação de running e wait.
Solução: Já verificado dentro do mutex lock - CORRETO.

#### 3. Acesso a Socket após Remoção
```cpp

// Em mandar_mensagem:
for (int cliente_sock : sala->clientes) {
    send(cliente_sock, msg.c_str(), msg.size(), 0);
}
```

Problema: Socket pode ser fechado em outra thread durante o send.
Solução: Adicionar verificação de erro no send e ignorar falhas.

### Possíveis Deadlocks
#### 1. Ordem de Aquisição de Locks

O código mantém ordem consistente:
- Sempre: mutex_historico → mutex_clientes
- Nunca ocorre ordem inversa
- Status: SEM RISCO DE DEADLOCK

#### 2. Condition Variables
- Uso correto de pthread_cond_wait dentro do mutex lock.
- Status: IMPLEMENTAÇÃO CORRETA

### Problemas Identificados (trecho)
#### 1. Uso Inseguro de Socket no Broadcast
```cpp
// Problema: socket pode fechar durante send
send(cliente_sock, msg.c_str(), msg.size(), 0);

// Solução sugerida:
if (send(cliente_sock, msg.c_str(), msg.size(), MSG_NOSIGNAL) <= 0) {
    // Socket fechado, remover da lista (já será tratado na thread do cliente)
}
```

#### 2. Vazamento de Threads Detached
```cpp

pthread_create(&thread_salas[i], nullptr, initChatRoom, id);
pthread_detach(thread_salas[i]); // Não é possível fazer join depois
```

Risco: Se o servidor terminar abruptamente, threads podem não completar.

### Conclusão
O sistema apresenta uma arquitetura sólida com boa sincronização na maioria das áreas críticas. Os principais riscos estão relacionados ao gerenciamento de sockets durante condições de erro e à falta de timeouts em operações de rede. As correções sugeridas melhorariam significativamente a robustez do sistema em ambiente de produção.
