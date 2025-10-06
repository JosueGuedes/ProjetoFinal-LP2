## Projeto Final de Linguagem de Programação II: 

### Tema escolhido
O tema escolhido para o projeto em mão foi o Tema A: Um chat multiusuário utilizando de sockets.

### Diagrama da arquitetura do projeto (FASE I)
![Diagrama da arquitetura do projeto](https://github.com/JosueGuedes/ProjetoFinal-LP2/blob/main/DiagramaFase1.png)

### Instruções de compilação e execução - v3
#### Compilação
Basta digitar:
`make clear`

E então, 
`make`
Para compilar ambos server.cpp e cliente.cpp

#### Execução
Para poder executar o programa, basta abrir dois ou mais terminais (um para server-side e o resto para clientes) e em cada um digitar:
`./server` para server-side
`./cliente` para cliente-side

Para terminar execução no server-side, basta um Ctrl-C que o programa fará o procedimento de limpeza dos dados, threads e recursos alocados.
Para terminar a execução no client-side, basta digitar "/sair"

##### Instruções de compilação - v2
Basta digitar `make` no terminal para compilar cliente.cpp e server.cpp
Alternativamente, para os testes com script com a versão alterada de cliente.cpp (vulgo cliente_testes.cpp), utilize de:

Para dar autorização:
`chmod +x script_testes.sh`

Para executar:
`./script_testes.sh`

