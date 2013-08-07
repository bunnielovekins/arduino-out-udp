#include <Servo.h>
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

//Internet stuff
byte mac[] = { 0x8E, 0x8D, 0xBE, 0x8F, 0xFE, 0xEE };
//char server[] = "ec2-54-213-123-121.us-west-2.compute.amazonaws.com";
char server[] = "10.32.25.136";
//IPAddress serverIP(54,213,123,121);
IPAddress serverIP(10,32,25,136);
char buffer[UDP_TX_PACKET_MAX_SIZE];

//Ports
int sendPort = 4001;
int recvPort = 4002;

//Clients
EthernetClient client;
EthernetUDP Udp;

//Servos
Servo myservo8;
Servo myservo9;
int inValue = -1;
int outValue = -1;

//Strings
char contype[] = "Content-Type: application/x-www-form-urlencoded";
char conclose[] = "Connection: close";
char conlen[] = "Content-Length: 20";
char udpMessageOut[] = "get   ";

//Various
int sensorNum = -1;
int debugInfoTCP = 0;
int debugInfoUDP = 0;
int moreDebugInfo = 0;
int delayAmount = 20;

void setup(){
  Serial.begin(9600);
  Serial.print('.');
  if(Ethernet.begin(mac)==0){
    Serial.println("Failed to configure Ethernet with DHCP");
    while(true);
  }
  else Serial.println("Connected to Ethernet");
  delay(1000);
  
  
  getMyNum();
  
  udpMessageOut[4] = (char)(sensorNum+48);
  Udp.begin(recvPort);
  
  myservo8.attach(8);
  myservo9.attach(9);
}


void loop(){
  //Send a request for the value
  if(moreDebugInfo)
    Serial.println("Sending...");
  Udp.beginPacket(serverIP, sendPort);
  Udp.write(udpMessageOut);
  Udp.endPacket();
  
  delay(delayAmount);
  
  //Attempt to get a message
  int packetSize = Udp.parsePacket();
  if(moreDebugInfo){
    Serial.print("Response size: ");
    Serial.println(packetSize);
  }
  if(packetSize){
    
    Udp.read(buffer,packetSize);
    inValue = myParse(buffer,packetSize);
    if(moreDebugInfo){
      Serial.print("inValue:");
      Serial.println(inValue);
    }
    if(inValue>=0 && inValue<=1024)
      outValue = map(inValue,0,1023,0,179);
    if(moreDebugInfo){
      Serial.print("Got message: \n");
      Serial.println(buffer);
    }
  }
  if(debugInfoUDP){
    Serial.print("outVal:");
    Serial.println(outValue);
    if(outValue>180 || outValue<0){
      Serial.println("Got a weird value. More info:");
      Serial.print("inVal:");
      Serial.println(inValue);
      Serial.print("buffer:");
      Serial.println(buffer);
    }
  }
  
  //Set servos accordingly
    myservo8.write(outValue);
    myservo9.write(outValue);
}

void simpleUDPMessage(char str[]){
  Udp.beginPacket(serverIP, sendPort);
  Udp.write(str);
  Udp.endPacket();
}

int myParse(char buf[], int len){
  int i = 0;
  int ret = -1;
  for(i;i<len;i++){
    if(buf[i] >= '0' && buf[i] <= '9'){
      if(ret==-1)
        ret=buf[i]-48;
      else{
        ret*=10;
        ret+=buf[i]-48;
      }
    }
    else break;
    
  }
  if(moreDebugInfo){
    Serial.print("Parsed ");
    Serial.println(ret);
  }
  return ret;
}

//Initial thing to be called; gets the sensor number to watch via TCP, right now just the last one
void getMyNum(){
  TCPConnect();
  client.println("GET /sensors/num HTTP/1.0");
  client.println(contype);
  client.println(conlen);
  client.println(conclose);
  client.println();
  client.print("ip=");
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    client.print(Ethernet.localIP()[thisByte], DEC);
    client.print("."); 
  }
  client.println();
  client.println();
  Serial.println("Supposedly sent request");
  char c = 0;
  int lastNum = -1;
  while(sensorNum ==-1){  
    while(client.available()){
      c = client.read();
      if(c>='0' && c<='9'){
        lastNum = c-48;
        if(debugInfoTCP)
          Serial.print("#");
      }
      if(debugInfoTCP)
        Serial.print(c);
    }
    if(sensorNum==0)
      while(true);
    if(lastNum != -1)
      sensorNum = lastNum-1;
    else delay(50);
  }
  if(debugInfoTCP)
    Serial.println();
  Serial.print("Got sensor number: ");
  Serial.println((char)(sensorNum+48));
  TCPStop();
}

//Ends TCP Connection
void TCPStop(){
  while(client.available())
    client.read();
  client.flush();
  if(!client.connected()){
    client.stop();
    Serial.println("Stopping TCP...");
  }
}

//Connects to server via TCP
void TCPConnect(){
  Serial.println("connecting TCP...");
  while(!client.connect(server,4000)){
    Serial.println("Connection failed, trying again in 1s");
    delay(1000);
  }
  if(debugInfoTCP)
    Serial.println("connected TCP");
}





