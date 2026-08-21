/********************
Author: Saman
4Terminal_Sim800
last update: 2026/08/21
controll 4rellays with sending R1~4 on/off or all on or all off
*********************/
#include <SoftwareSerial.h>
SoftwareSerial sim800(10,11);
//*Arduino D10 = RX & Arduino D11 = TX*/
int RedLED = 8;
int GreenLED = 9;
int retry = 10;
int R1 = 4;
int R2 = 3;
int R3 = 2;
int R4 = 5;
const char PHONE[]="+989396969233";/*receiver*/
int latestSMSIndex = -1;
int smsindex;
//BUFFER USED FOR RECEIVING DATA
String response="";


//CLEAR SERIAL BUFFER
void clearBuffer()
{
  while(sim800.available())
  {
    sim800.read();
  }
}
/*WAIT UNTIL A SPECIFIC TEXT ARRIVES*/
bool waitFor(String expected,unsigned long timeout)
{
  response="";
  unsigned long start=millis();
  while(millis()-start<timeout)
  {
    while(sim800.available())
    {
      char c=sim800.read();
      Serial.write(c);
      response+=c;
      if(response.indexOf(expected)>=0)
      {
        return true;
      }
    }
  }
  return false;
}
/*SEND COMMAND until returns ok and try retry times*/
bool sendCommandUntilOK(String command)
{
  for(int i=0;i<retry;i++)
  {
    clearBuffer();
    Serial.print("Sending : ");
    Serial.println(command);
    sim800.println(command);
    if(waitFor("OK",3000))
    {
      Serial.println("OK RECEIVED");
      return true;
    }
    Serial.println("Retry...");
  }
  Serial.println("FAILED");
  return false;
}
/*Used only for CMGS to return > */
bool waitForPrompt()
{
  for(int i=0;i<retry;i++)
  {
    clearBuffer();
    Serial.println("Sending CMGS");
    sim800.print("AT+CMGS=\"");
    sim800.print(PHONE);
    sim800.println("\"");

    if(waitFor(">",4000))
    {
      Serial.println("> RECEIVED");
      return true;
    }
    Serial.println("Retry CMGS...");
  }
  return false;
}
/*Check Signal Strength*/
bool checkSignal()
{
  for(int i=0;i<retry;i++)
  {
    clearBuffer();
    Serial.print("Checking Signal (Try ");
    Serial.print(i + 1);
    Serial.println(")...");
    sim800.println("AT+CSQ");
    response = "";
    unsigned long start = millis();
    while(millis() - start < 3000)
    {
      while(sim800.available())
      {
        char c = sim800.read();
        Serial.write(c);
        response += c;
      }
    }
    int index = response.indexOf("+CSQ:");
    if(index >= 0)
    {
      int comma = response.indexOf(",", index);
      String value = response.substring(index + 6, comma);
      int csq = value.toInt();
      
      Serial.print("Signal = ");
      Serial.println(csq);
      if(csq >= 10 && csq != 99)
      {
        return true;
      }
    }
    Serial.println("Retry...");
  }
  return false;
}
/*Check network connection*/
bool checkNetwork()
{
  for(int i = 0; i < retry; i++)
  {
    clearBuffer();
    Serial.print("Checking Network (Try ");
    Serial.print(i + 1);
    Serial.println(")...");
    sim800.println("AT+CREG?");
    response = "";
    unsigned long start = millis();
    while(millis() - start < 3000)
    {
      while(sim800.available())
      {
        char c = sim800.read();
        Serial.write(c);
        response += c;
      }
    }
    if(response.indexOf("+CREG: 0,1") >= 0 ||
       response.indexOf("+CREG: 0,5") >= 0)
    {
      Serial.println("NETWORK REGISTERED");
      return true;
    }
    Serial.println("Retry...");
  }
  Serial.println("NETWORK NOT REGISTERED");
  return false;
}
/*SEND SMS*/
bool sendSMS(String text)
{
  Serial.println();
  Serial.println("Sending Sms...");
  if(sendCommandUntilOK("AT"))
  {
    if(checkSignal())
    {
      if(checkNetwork())
      {
        if(sendCommandUntilOK("AT+CSMP=17,167,0,0"))
        {
          if(sendCommandUntilOK("AT+CMGF=1"))
          {
            if(waitForPrompt())
            {
              delay(300);
              sim800.print(text);
              delay(200);
              sim800.write(26);
              if(waitFor("+CMGS:",10000))
              {
                if(waitFor("OK",5000))
                {
                  Serial.println("MESSAGE SENT");
                  return true;
                 }
                Serial.println("NO FINAL OK");
                return false;      
               }
              Serial.println("NO +CMGS");
              return false;           
             }
            Serial.println("Could not bee ready for prompt");
            return false;
           }
          Serial.println("CMGF FAILED");
          return false;      
          }
        Serial.println("CSMP FAILED");
        return false;   
        }
      Serial.println("NETWORK NOT REGISTERED");
      return false;
      }
    Serial.println("SIGNAL TOO WEAK");
    return false; 
    }
  Serial.println("MODEM NOT RESPONDING");
  return false;
} 

/*Update Newsms Index*/
void updateLatestSMSIndex()
{
  if(response.length() > 100){
    response = "";}
    while(sim800.available())
    {
        char c = sim800.read();

        Serial.write(c);

        response += c;

        if(c == '\n')
        {
            if(response.indexOf("+CMTI:") >= 0)
            {
                int comma = response.indexOf(',');

                if(comma >= 0)
                {
                    String number = response.substring(comma + 1);

                    number.trim();

                    latestSMSIndex = number.toInt();

                    Serial.print("Latest SMS Index = ");
                    Serial.println(latestSMSIndex);
                }
            }

            response = "";
        }
    }
}

/*READ AN SMS*/
String readSMS(int index)
{
  if(sendCommandUntilOK("AT+CMGF=1"))
  {
    clearBuffer();
    sim800.print("AT+CMGR=");
    sim800.println(index);
    response = "";
    unsigned long start = millis();
    while(millis() - start < 5000)
    {
      while(sim800.available())
      {
        char c = sim800.read();
        response += c;
        if(response.indexOf("OK") >= 0)
        {
          break;
        }
      }
      if(response.indexOf("OK") >= 0)
      {
        break;
      }
    }
    if(response.indexOf("+CMGR:") >= 0)
    {
      int firstNewLine = response.indexOf('\n');
      int secondNewLine = response.indexOf('\n', firstNewLine + 1);
      if(firstNewLine >= 0)
      {
        int okIndex = response.indexOf("OK");
        if(okIndex >= 0)
        {
          String smsText = response.substring(secondNewLine + 1, okIndex);
          smsText.trim();
          return smsText;
        }
        else
        {
          Serial.println("OK NOT FOUND");
          return "";
        }
      }
      else
      {
        Serial.println("INVALID RESPONSE");
        return "";
      }
    }
    else
    {
      Serial.println("FAILED TO READ SMS");
      return "";
    }
  }
  else
  {
    Serial.println("FAILED TO SET TEXT MODE");
    return "";
  }
}

/*Delete Sms*/
bool deleteSMS(int index)
{
  for(int i = 0; i < retry; i++)
  {
    clearBuffer();
    Serial.print("Deleting SMS ");
    Serial.print(index);
    Serial.print(" (Try ");
    Serial.print(i + 1);
    Serial.println(")...");
    sim800.print("AT+CMGD=");
    sim800.println(index);
    if(waitFor("OK",3000))
    {
      Serial.println("SMS Deleted");
      return true;
    }
    Serial.println("Retry...");
  }
  Serial.println("FAILED TO DELETE SMS");
  return false;
}
/*SETUP*/
void setup()
{
  pinMode(R1,OUTPUT);
  pinMode(R2,OUTPUT);
  pinMode(R3,OUTPUT);
  pinMode(R4,OUTPUT);
  digitalWrite(R1,LOW);
  digitalWrite(R2,LOW);
  digitalWrite(R3,LOW);
  digitalWrite(R4,LOW);
  Serial.begin(9600);
  sim800.begin(9600);
  pinMode(RedLED,OUTPUT);
  digitalWrite(RedLED,HIGH);
  pinMode(GreenLED,OUTPUT);
  digitalWrite(GreenLED,LOW);
  if(sendCommandUntilOK("AT"))
  {
    Serial.println("AT OK");
    digitalWrite(RedLED,LOW);
    digitalWrite(GreenLED,HIGH);
    delay(1000);
    digitalWrite(GreenLED,LOW);
    digitalWrite(RedLED,HIGH);
    if(checkSignal())
    {
      Serial.println("Signal OK");
      digitalWrite(RedLED,LOW);
      digitalWrite(GreenLED,HIGH);
      delay(1000);
      digitalWrite(GreenLED,LOW);
      digitalWrite(RedLED,HIGH);
      if(checkNetwork())
      { 
        Serial.println("Network Ok");
        Serial.println("Waiting For SMS...");
        digitalWrite(RedLED,LOW);  
        digitalWrite(GreenLED,HIGH);
        return true;
        }
      Serial.println("NETWORK NOT REGISTERED");
      return false;
      }
    Serial.println("SIGNAL TOO WEAK");
    return false; 
    }
  Serial.println("MODEM NOT RESPONDING");
  return false;
}

/*Main LOOP*/

void loop()
{
    updateLatestSMSIndex();
    if(latestSMSIndex != -1)
    {
        String message = readSMS(latestSMSIndex);
        Serial.print("Received: ");
        Serial.println(message);
        if((message == "R1 on") || (message == "r1 on") )
        {
            sendSMS("Rellay 1 Turned on");
            digitalWrite(RedLED, HIGH);
            digitalWrite(GreenLED, LOW);
            delay(100);
            digitalWrite(RedLED, LOW);
            digitalWrite(GreenLED, HIGH);
            digitalWrite(R1,HIGH);
          deleteSMS(latestSMSIndex);
          latestSMSIndex = -1;
        }
        else if((message == "R1 off") || (message == "r1 off"))
        {
            sendSMS("Rellay 1 turned off");
            digitalWrite(RedLED, HIGH);
            digitalWrite(GreenLED, LOW);
            delay(100);
            digitalWrite(RedLED, LOW);
            digitalWrite(GreenLED, HIGH);
            digitalWrite(R1,LOW);
            deleteSMS(latestSMSIndex);
            latestSMSIndex = -1;
        }
        else if((message == "R2 on") || (message == "r2 on") )
        {
            sendSMS("Rellay 2 Turned on");
            digitalWrite(RedLED, HIGH);
            digitalWrite(GreenLED, LOW);
            delay(100);
            digitalWrite(RedLED, LOW);
            digitalWrite(GreenLED, HIGH);
            digitalWrite(R2,HIGH);
          deleteSMS(latestSMSIndex);
          latestSMSIndex = -1;
        }
        else if((message == "R2 off") || (message == "r2 off"))
        {
            sendSMS("Rellay 2 turned off");
            digitalWrite(RedLED, HIGH);
            digitalWrite(GreenLED, LOW);
            delay(100);
            digitalWrite(RedLED, LOW);
            digitalWrite(GreenLED, HIGH);
            digitalWrite(R2,LOW);
            deleteSMS(latestSMSIndex);
            latestSMSIndex = -1;
        }
        else if((message == "R3 on") || (message == "r3 on") )
        {
            sendSMS("Rellay 3 Turned on");
            digitalWrite(RedLED, HIGH);
            digitalWrite(GreenLED, LOW);
            delay(100);
            digitalWrite(RedLED, LOW);
            digitalWrite(GreenLED, HIGH);
            digitalWrite(R3,HIGH);
          deleteSMS(latestSMSIndex);
          latestSMSIndex = -1;
        }
        else if((message == "R3 off") || (message == "r3 off"))
        {
            sendSMS("Rellay 3 turned off");
            digitalWrite(RedLED, HIGH);
            digitalWrite(GreenLED, LOW);
            delay(100);
            digitalWrite(RedLED, LOW);
            digitalWrite(GreenLED, HIGH);
            digitalWrite(R3,LOW);
            deleteSMS(latestSMSIndex);
            latestSMSIndex = -1;
        }
        else if((message == "R4 on") || (message == "r4 on") )
        {
            sendSMS("Rellay 4 Turned on");
            digitalWrite(RedLED, HIGH);
            digitalWrite(GreenLED, LOW);
            delay(100);
            digitalWrite(RedLED, LOW);
            digitalWrite(GreenLED, HIGH);
            digitalWrite(R4,HIGH);
          deleteSMS(latestSMSIndex);
          latestSMSIndex = -1;
        }
        else if((message == "R4 off") || (message == "r4 off"))
        {
            sendSMS("Rellay 4 turned off");
            digitalWrite(RedLED, HIGH);
            digitalWrite(GreenLED, LOW);
            delay(100);
            digitalWrite(RedLED, LOW);
            digitalWrite(GreenLED, HIGH);
            digitalWrite(R4,LOW);
            deleteSMS(latestSMSIndex);
            latestSMSIndex = -1;
        }

        else if((message == "All ON") || (message == "all on"))
        {
            sendSMS("All of the Rellays turned on");
            digitalWrite(RedLED, HIGH);
            digitalWrite(GreenLED, LOW);
            delay(100);
            digitalWrite(RedLED, LOW);
            digitalWrite(GreenLED, HIGH);
            digitalWrite(R1,HIGH);
            digitalWrite(R2,HIGH);            
            digitalWrite(R3,HIGH);
            digitalWrite(R4,HIGH);
            deleteSMS(latestSMSIndex);
            latestSMSIndex = -1;
        }
        else if((message == "All Off") || (message == "all off"))
        {
            sendSMS("All of the Rellays turned off");
            digitalWrite(RedLED, HIGH);
            digitalWrite(GreenLED, LOW);
            delay(100);
            digitalWrite(RedLED, LOW);
            digitalWrite(GreenLED, HIGH);
            digitalWrite(R1,LOW);
            digitalWrite(R2,LOW);            
            digitalWrite(R3,LOW);
            digitalWrite(R4,LOW);
            deleteSMS(latestSMSIndex);
            latestSMSIndex = -1;
        }
        
         else
         {
            digitalWrite(RedLED, HIGH);
            digitalWrite(GreenLED, LOW);
            delay(100);
            digitalWrite(RedLED, LOW);
            digitalWrite(GreenLED, HIGH);
          sendSMS("Wrong Input, try again...");
          deleteSMS(latestSMSIndex);
          latestSMSIndex = -1;
          }
    }
}
