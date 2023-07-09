<h3>Trabalho Prático 2 - Redes de Computadores</h3>
<p>Universidade Federal de Minas Gerais</p>

##

<div>
  </br>
  <p>O presente trabalho tem o objetivo de implementar um sistema de chat em grupo, que é capaz de conectar simultaneamente 15 usuários (clientes) a um servidor central. Isso é feito na rede local, utilizando sockets em linguagem C. O sistema é composto basicamente por um servidor, responsável por coordenar múltiplas conexões (conforme os usuários entram e saem), e os usuários que trocam mensagens entre si, por meio do servidor. Para tanto, cria-se um programa multithread, tanto do lado do servidor quanto do usuário.</p>
  <p>Para fins de teste, utiliza-se dois ou mais terminais do prompt de comando, um que serve como servidor, e os demais como usuários. Na versão deste trabalho, os usuários trocam apenas mensagens textuais, com comandos pré-definidos. Todas as mensagens são inseridas nos clientes, e as respostas vêm do servidor. O protocolo de comunicação é simples e único, de modo que todos os programas entendem as mensagens.</p>
  <p>Por fim, vale ressaltar que as comunicações são feitas com IPv4 ou IPv6, bastando escolher no prompt de comando.</p>
  
  <h3>Em dois terminais diferentes, rode:</h3>
  <p>./server v4 51511</p>
  <p>./client 127.0.0.1 51511 (Para quantos clientes quiser)</p>
  
  <h3>Comandos disponíveis:</h3>
  <p>list users (lista o número de usuários/clientes conectados)</p>
  <p>send all "{Message}" (envia mensagem para todos os clientes)</p>
  <p>send to {User ID} "{Message}" (envia mensagem para o cliente com o ID especificado)</p>
  <p>close connection (fecha a conexão de um cliente)</p>
</div>
