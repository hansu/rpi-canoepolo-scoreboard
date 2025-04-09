#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <thread>
#include <iostream>
#include <string>
#include "DisplayData.hh"
#include "socket.hh"
#include <sstream>


void ShotclockCom (DisplayData& dispData, const char* name, int port)
{
  Socket csocket(true);

 //Create socket
  while(1){
    if(csocket.SocketCreate() == -1)
    {
      printf("[%s] Could not create socket\n", name);
      sleep(5);
    } else{
      printf("[%s] Socket is created\n", name);
      break;
    }
  }

  //Bind
  while(1){
    if(csocket.BindCreatedSocket(port) < 0)
    {
      printf("[%s] bind failed\n", name);
      sleep(5);
    } else {
      printf("[%s] bind done\n", name);
      break;
    }
  }

  //Listen
  csocket.Listen();
  char tx_string[10];
  int retVal;

  while(1)
  {
      printf("[%s] Waiting for incoming connections...\n", name);
      //Accept incoming connection
      if (csocket.Accept() < 0)
      {
          printf("[%s] accept failed", name);
          break;
      }
      printf("[%s] Connection accepted\n", name);

      while(1){
        //if(dispData.NeedShotclockRefresh()){
        sprintf(tx_string, "%02d", dispData.getShotTimeout());
        printf("[%s] send %s\n", name, tx_string);

        if((retVal = csocket.SocketSend(std::string(tx_string))) < 0)
        {
          printf("[%s] send failed (error %d)\n", name, retVal);
          break;
        }
        usleep(250000);
      }
  }

}
