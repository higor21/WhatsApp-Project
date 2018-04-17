#include <iostream>

#include "winsocket.h"

using namespace std;

/*********************************************
 A classe base winsocket
 *********************************************/

// Construtores e destrutores

winsocket::winsocket()
{
  state = WINSOCKET_IDLE;
  id = INVALID_SOCKET;
}

// Faz com que o socket nao possa mais enviar dados
// Ainda pode receber algum que jah tenha sido enviado
// Pode ser usado antes do close para evitar perda de dados em transito
WINSOCKET_STATUS winsocket::shutdown()
{
  WINSOCKET_STATUS iResult;

  if (id != INVALID_SOCKET && state != WINSOCKET_IDLE) {
    state = WINSOCKET_IDLE;
    iResult = ::shutdown(id, SD_SEND);
    if (iResult == SOCKET_ERROR) {
      cerr << "winsocket shutdown failed: " << WSAGetLastError() << endl;
      ::closesocket(id);
      id = INVALID_SOCKET;
      return iResult;
    }
  }
  return(SOCKET_OK);
}

// Fecha (destroi) o socket
void winsocket::close()
{
  shutdown();
  if (id != INVALID_SOCKET) {
    ::closesocket(id);
  }
  id = INVALID_SOCKET;
}

// Impress�o

std::ostream& operator<<(std::ostream& os, const winsocket &s)
{
  os << s.id << '(';
  switch (s.state) {
  case WINSOCKET_IDLE:
    os << 'I';
    break;
  case WINSOCKET_ACCEPTING:
    os << 'A';
    break;
  case WINSOCKET_CONNECTED:
    os << 'C';
    break;
  }
  os << ')';
  return os;
}

/*********************************************
 Sockets orientados a conex�o (STREAM SOCKET)
 *********************************************/

// Sockets clientes

WINSOCKET_STATUS tcp_winsocket::connect(const char *name, const char *port)
{
  if (id!=INVALID_SOCKET || state!=WINSOCKET_IDLE) {
    cerr << "winsocket connect - already being used: " << *this << endl;
    return(SOCKET_ERROR);
  }

  // The getaddrinfo function is used to determine the values in the sockaddr structure:
  // The getaddrinfo function provides protocol-independent translation from an ANSI host name to an address.
  // Parameters:
  // pNodeName: A pointer to a NULL-terminated ANSI string that contains a host name or a numeric host address as string
  //   NULL = any
  // pServiceName: A pointer to a NULL-terminated ANSI string that contains either a service name or port number as string
  //   DEFAULT_PORT ("27015") is the port number associated with the server that the client will connect to.
  // pHints: A pointer to an addrinfo structure that provides hints about the type of socket the caller supports.
  //   AF_INET is used to specify the IPv4 address family.
  //   SOCK_STREAM is used to specify a stream socket.
  //   IPPROTO_TCP is used to specify the TCP protocol .
  //   AI_PASSIVE flag indicates the caller intends to use the returned socket address structure in a call to the bind function.
  //     When the AI_PASSIVE flag is set and pNodeName parameter to the getaddrinfo function is a NULL pointer,
  //     the IP address portion of the socket address structure is set to INADDR_ANY for IPv4 addresses or
  //     IN6ADDR_ANY_INIT for IPv6 addresses.
  // ppResult: A pointer to a linked list of one or more addrinfo structures that contains response information about the host.

  struct addrinfo hints, *result = NULL;  // para getaddrinfo, bind
  WINSOCKET_STATUS iResult;

  memset(&hints, 0, sizeof (hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  // Call the getaddrinfo function requesting the IP address for the server.
  iResult = getaddrinfo(name, port, &hints, &result);
  if (iResult != 0) {
      cerr << "winsocket connect - getaddrinfo failed: " << iResult << endl;
      return iResult;
  }

  // Create a SOCKET for the client to communicate with the server
  // Use the first IP address returned by the call to getaddrinfo that matched the address family, socket type, and protocol
  // specified in the hints parameter. In this example, a TCP stream socket was specified with a socket type of SOCK_STREAM
  // and a protocol of IPPROTO_TCP. The address family was left unspecified (AF_UNSPEC), so the returned IP address could be
  // either an IPv6 or IPv4 address for the server.
  id = ::socket(result->ai_family, result->ai_socktype, result->ai_protocol);
  if (id == INVALID_SOCKET) {
      cerr << "winsocket connect - error at socket(): " << WSAGetLastError() << endl;
      freeaddrinfo(result);
      return id;
  }

  // Connect to server.
  iResult = ::connect( id, result->ai_addr, (int)result->ai_addrlen);
  if (iResult == SOCKET_ERROR) {
      // Should really try the next address returned by getaddrinfo if the connect call failed
      // But for this simple example we just free the resources
      // returned by getaddrinfo and print an error message
      cerr << "winsocket connect - connect failed with error: " << WSAGetLastError() << endl;
      freeaddrinfo(result);
      close();
      return iResult;
  }

  // Once the connect function is called, the address information returned by the getaddrinfo function is no longer needed.
  // The freeaddrinfo function is called to free the memory allocated by the getaddrinfo function for this address information.
  freeaddrinfo(result);

  state = WINSOCKET_CONNECTED;
  return(SOCKET_OK);
}

WINSOCKET_STATUS tcp_winsocket::write(const char *dado, size_t len) const
{
  if (!connected()) {
    cerr << "winsocket write - not connected: " << *this << endl;
    return(SOCKET_ERROR);
  }
  if (len==0) {
    cerr << "winsocket write - empty data: " << *this << endl;
    return(SOCKET_ERROR);
  }

  // send: sends data on a connected socket
  // Parameters:
  // s: The descriptor that identifies a connected socket.
  // buf: A pointer to a buffer containing the data to be transmitted.
  // len: length, in bytes, of the data in buffer pointed to by the buf parameter.
  // flags: A set of flags that specify the way in which the call is made.
  int ultimo_envio,enviados=0,falta_enviar=len;
  do
  {
    ultimo_envio = ::send(id, dado, falta_enviar, 0);
    if ( ultimo_envio == SOCKET_ERROR ) {
      cerr << "winsocket write - send failed with error: " << WSAGetLastError() << endl;
      return ultimo_envio;
    }
    enviados += ultimo_envio;
    falta_enviar -= ultimo_envio;
    if (falta_enviar > 0) {
      dado += ultimo_envio;
    }
  } while (falta_enviar>0);
  return(len);
}

WINSOCKET_STATUS tcp_winsocket::read(char *dado, size_t len, long milisec) const
{
  WINSOCKET_STATUS iResult;
  if (!connected()) {
    cerr << "winsocket read - not connected: " << *this << endl;
    return(SOCKET_ERROR);
  }
  if (len<=0) {
    cerr << "winsocket read - empty data: " << *this << endl;
    return(SOCKET_ERROR);
  }
  if (milisec>=0) {
    // Com timeout
    winsocket_queue f;
    f.include(*this);
    iResult=f.wait_read(milisec);
    if (iResult == SOCKET_ERROR)
    {
        cerr << "winsocket read - select failed: " << WSAGetLastError() << endl;
        return iResult;
    }
    if (iResult == 0)
    {
        cerr << "winsocket read - timeout: " << *this << endl;
        return iResult;
    }
  }
  // recv: receives data from a connected socket
  // Parameters:
  // s: The descriptor that identifies a connected socket.
  // buf: A pointer to the buffer to receive the incoming data.
  // len: The length, in bytes, of the buffer pointed to by the buf parameter.
  // flags: A set of flags that influences the behavior of this function.
  iResult = ::recv(id,dado,len,0);
  if ( iResult == SOCKET_ERROR )
  {
      cerr << "winsocket read - recv failed with error: " << WSAGetLastError() << endl;
      return iResult;
  }
  if (iResult == 0)
  {
      cerr << "winsocket read - Servidor desconectado\n";
      return iResult;
  }
  return iResult;
}

// Sockets servidores

WINSOCKET_STATUS tcp_winsocket_server::listen(const char *port, int nconex)
{
    if (id!=INVALID_SOCKET || state!=WINSOCKET_IDLE) {
        cerr << "winsocket listen - already being used: " << *this << endl;
        return(SOCKET_ERROR);
    }
    // Cria o socket

    // The getaddrinfo function is used to determine the values in the sockaddr structure:
    // The getaddrinfo function provides protocol-independent translation from an ANSI host name to an address.
    // Parameters:
    // pNodeName: A pointer to a NULL-terminated ANSI string that contains a host name or a numeric host address as string
    //   NULL = any
    // pServiceName: A pointer to a NULL-terminated ANSI string that contains either a service name or port number as string
    //   DEFAULT_PORT ("27015") is the port number associated with the server that the client will connect to.
    // pHints: A pointer to an addrinfo structure that provides hints about the type of socket the caller supports.
    //   AF_INET is used to specify the IPv4 address family.
    //   SOCK_STREAM is used to specify a stream socket.
    //   IPPROTO_TCP is used to specify the TCP protocol .
    //   AI_PASSIVE flag indicates the caller intends to use the returned socket address structure in a call to the bind function.
    //     When the AI_PASSIVE flag is set and pNodeName parameter to the getaddrinfo function is a NULL pointer,
    //     the IP address portion of the socket address structure is set to INADDR_ANY for IPv4 addresses or
    //     IN6ADDR_ANY_INIT for IPv6 addresses.
    // ppResult: A pointer to a linked list of one or more addrinfo structures that contains response information about the host.

    struct addrinfo hints, *result = NULL;  // para getaddrinfo, bind
    WINSOCKET_STATUS iResult;


    memset(&hints, 0, sizeof (hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the local address and port to be used by the server
    iResult = getaddrinfo(NULL, port, &hints, &result);
    if (iResult != 0) {
        cerr << "winsocket listen - getaddrinfo failed: " << iResult << endl;
        return(SOCKET_ERROR);
    }

    // Create a SOCKET for the server to listen for client connections
    // Use the first IP address returned by the call to getaddrinfo that matched the address family, socket type, and protocol
    // specified in the hints parameter
    id = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (id == INVALID_SOCKET) {
        cerr << "winsocket listen - error at socket(): " << WSAGetLastError() << endl;
        freeaddrinfo(result);
        return(SOCKET_ERROR);
    }

    // Atribui��o do nome do socket

    // For a server to accept client connections, it must be bound to a network address within the system.
    // Call the bind function, passing the created socket and sockaddr structure returned from the getaddrinfo function as parameters.
    // Setup the TCP listening socket
    iResult = bind( id, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        cerr << "winsocket listen - bind failed with error: " << WSAGetLastError() << endl;
        freeaddrinfo(result);
        close();
        return iResult;
    }

    // Once the bind function is called, the address information returned by the getaddrinfo function is no longer needed.
    // The freeaddrinfo function is called to free the memory allocated by the getaddrinfo function for this address information.
    freeaddrinfo(result);

    // After the socket is bound to an IP address and port on the system,
    // the server must then listen on that IP address and port for incoming connection requests
    // Parameters:
    // s: A descriptor identifying a bound, unconnected socket.
    // backlog: The maximum length of the queue of pending connections.
    //   If set to SOMAXCONN, will set the backlog to a maximum reasonable value.
    // the created socket and a value for the maximum length of the queue of pending connections to accept
    iResult = ::listen( id, nconex );
    if ( iResult == SOCKET_ERROR ) {
        cerr << "winsocket listen - listen failed with error: " << WSAGetLastError() << endl;
        close();
        return iResult;
    }

    state = WINSOCKET_ACCEPTING;
    return(SOCKET_OK);
}

WINSOCKET_STATUS tcp_winsocket_server::accept(tcp_winsocket &a) const
{
    if (!accepting()) {
        cerr << "winsocket accept - not accepting: " << *this << endl;
        return(SOCKET_ERROR);
    }
    a.id = ::accept(id,NULL,NULL);
    if (a.id == INVALID_SOCKET) {
        cerr << "winsocket accept - accept failed with error: " << WSAGetLastError() << endl;
        return(SOCKET_ERROR);
    }
    a.state = WINSOCKET_CONNECTED;
    return(SOCKET_OK);
}

/*********************************************
A CLASSE winsocket_queue (FILA DE SOCKETS)
*********************************************/

WINSOCKET_STATUS winsocket_queue::include(const winsocket &a)
{
  FD_SET(a.id,&set);
  return(SOCKET_OK);
}

WINSOCKET_STATUS winsocket_queue::exclude(const winsocket &a)
{
  if (set.fd_count == 0) {
    cerr << "winsocket_queue exclude - empty queue" << endl;
    return(SOCKET_ERROR);
  }
  FD_CLR(a.id,&set);
  return(SOCKET_OK);
}

WINSOCKET_STATUS winsocket_queue::wait_read(long milisec)
{
    if (set.fd_count == 0) {
        cerr << "winsocket_queue wait_read - empty queue" << endl;
        return(SOCKET_ERROR);
    }
    //cout << "Num sockets: " << set.fd_count << ' ';
    //for (unsigned i=0; i<set.fd_count; i++) cout << set.fd_array[i] << ' ';
    //cout << endl;
    int iResult;
    if (milisec >= 0) {
        struct timeval t;
        t.tv_sec = milisec/1000;
        t.tv_usec = 1000*(milisec - 1000*t.tv_sec);
        iResult = ::select(0, &set, NULL, NULL, &t);
    }
    else {
        iResult = ::select(0, &set, NULL, NULL, NULL);
    }

    if (iResult == SOCKET_ERROR)
    {
        cerr << "winsocket_queue wait_read - select failed: " << WSAGetLastError() << endl;
        return iResult;
    }
    if (iResult == 0)
    {
        cout << "winsocket_queue wait_read - select timeout" << endl;
        return iResult;
    }
    return iResult;
}

WINSOCKET_STATUS winsocket_queue::wait_write(long milisec)
{
    if (set.fd_count == 0) {
        cerr << "winsocket_queue wait_write - empty queue" << endl;
        return(SOCKET_ERROR);
    }
    int iResult;
    if (milisec >= 0) {
        struct timeval t;
        t.tv_sec = milisec/1000;
        t.tv_usec = 1000*(milisec - 1000*t.tv_sec);
        iResult = ::select(0, NULL, &set, NULL, &t);
    }
    else {
        iResult = ::select(0, NULL, &set, NULL, NULL);
    }

    if (iResult == SOCKET_ERROR)
    {
        cerr << "winsocket_queue wait_write - select failed: " << WSAGetLastError() << endl;
        return iResult;
    }
    if (iResult == 0)
    {
        cout << "winsocket_queue wait_write - select timeout" << endl;
        return iResult;
    }
    return(SOCKET_OK);
}

bool winsocket_queue::had_activity(const winsocket &a)
{
    if (set.fd_count == 0) {
        cerr << "winsocket_queue had_activity - empty queue" << endl;
        return(SOCKET_ERROR);
    }
    return(FD_ISSET(a.id,&set));
}

ostream& operator<<(ostream& os, const winsocket_queue &f)
{
    os << '[';
    for (unsigned i=0; i<f.set.fd_count; i++)
    {
        os << f.set.fd_array[i];
        os << (i+1==f.set.fd_count ? ']' : ';');
    }
    return os;
}


