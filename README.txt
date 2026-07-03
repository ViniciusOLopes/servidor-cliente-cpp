Nome do Aluno: [SEU NOME AQUI]

Instruções de Compilação:
O compilador utilizado é o gcc/g++ em ambiente Linux.
Para compilar o servidor, abra o terminal e digite:
g++ -pthread servidor.cpp -o servidor

Para compilar o cliente, abra um novo terminal e digite:
g++ -pthread cliente.cpp -o cliente

Instruções de Uso:
1. Inicie o servidor:
./servidor
O programa solicitará a porta (ex: 4242). Digite e pressione Enter.

2. Inicie os clientes (em terminais separados):
./cliente
O programa solicitará o IP (ex: 127.0.0.1), a porta (a mesma do servidor) e o apelido do usuário.
Para sair do chat, digite exatamente a palavra "tchau" e pressione Enter.