/**
* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
* 
* Copyright (c) 2016 LIBSCAPI (http://crypto.biu.ac.il/SCAPI)
* This file is part of the SCAPI project.
* DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
* and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
* FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
* 
* We request that any publication and/or code referring to and/or based on SCAPI contain an appropriate citation to SCAPI, including a reference to
* http://crypto.biu.ac.il/SCAPI.
* 
* Libscapi uses several open source libraries. Please see these projects for any further licensing issues.
* For more information , See https://github.com/cryptobiu/libscapi/blob/master/LICENSE.MD
*
* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
* 
*/


#include "falcon/network/Comm.hpp"

/*****************************************/
/* SocketPartyData						 */
/*****************************************/
int SocketPartyData::compare(const SocketPartyData &other) const {
	string thisString = ipAddress.to_string() + ":" + to_string(port);
	string otherString = other.ipAddress.to_string() + ":" + to_string(other.port);
	return thisString.compare(otherString);
}

/*****************************************/
/* CommParty			                */
/*****************************************/

void CommParty::writeWithSize(const byte* data, int size) {
	write((const byte *)&size, sizeof(int));
	write(data, size);
}

int CommParty::readSize() {
	byte buf[sizeof(int)];
	read(buf, sizeof(int));
	int * res = (int *)buf;
	return *res;
}

size_t CommParty::readWithSizeIntoVector(vector<byte> & targetVector) {
	int msgSize = readSize();
	targetVector.resize(msgSize);
	auto res = read((byte*)&targetVector[0], msgSize);
	return res;
}

/*****************************************/
/* CommPartyTCPSynced                    */
/*****************************************/
int CommPartyTCPSynced::join(int sleepBetweenAttempts, int timeout, bool first) {
  int     totalSleep = 0;
  bool    isAccepted  = (role == 1);//false;
  bool    isConnected = (role == 0); //false;
  // establish connections
  while (!isConnected || !isAccepted) {
    try {
      if (!isConnected) {
        tcp::resolver resolver(ioServiceClient);
        tcp::resolver::query query(other.getIpAddress().to_string(), to_string(other.getPort()));
        tcp::resolver::iterator endpointIterator = resolver.resolve(query);
        boost::asio::connect(clientSocket, endpointIterator);
        isConnected = true;
      }
    }
    catch (const boost::system::system_error& ex)
    {
      if (totalSleep > timeout)
      {
        cerr << "Failed to connect after timeout, aborting!";
        throw ex;
      }
      cout << "Failed to connect. sleeping for " << sleepBetweenAttempts <<
      " milliseconds, " << ex.what() << endl;
      this_thread::sleep_for(chrono::milliseconds(sleepBetweenAttempts));
      totalSleep += sleepBetweenAttempts;
    }
    if (!isAccepted) {
      boost::system::error_code ec;
      cout << "accepting..." << endl;
      acceptor_.accept(serverSocket, ec);
      isAccepted = true;
    }
  }
  setSocketOptions();
  return 0;
}

void CommPartyTCPSynced::setSocketOptions() {
  boost::asio::ip::tcp::no_delay option(true);
  if (role != 1)
    serverSocket.set_option(option);
  if (role != 0)
    clientSocket.set_option(option);
}

size_t CommPartyTCPSynced::write(const byte* data, int size, int peer, int protocol) {
  boost::system::error_code ec;
  bytesOut += size;
  return boost::asio::write(socketForWrite(),
                            boost::asio::buffer(data, size),
                            boost::asio::transfer_all(), ec);
}

CommPartyTCPSynced::~CommPartyTCPSynced() {
  if (role != 1) {
    acceptor_.close();
  }
  if (role != 1) {
    serverSocket.close();
  }
  if (role != 0){
    clientSocket.close();
  }
}