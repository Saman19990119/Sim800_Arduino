/********************
Author: Saman
Sim800IntegrationSystem 2.0
last update: 2026/08/02
*********************/
#include <SoftwareSerial.h>
SoftwareSerial sim800(10,11);
//*Arduino D10 = RX & Arduino D11 = TX*/
int retry = 10;
const char PHONE[]="write your number";
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
  Serial.begin(9600);
  sim800.begin(9600);
  pinMode(8,OUTPUT);
  digitalWrite(8,HIGH);
  pinMode(9,OUTPUT);
  digitalWrite(9,LOW);
  if(sendCommandUntilOK("AT"))
  {
    Serial.println("AT OK");
    digitalWrite(8,LOW);
    digitalWrite(9,HIGH);
    delay(1000);
    digitalWrite(9,LOW);
    digitalWrite(8,HIGH);
    if(checkSignal())
    {
      Serial.println("Signal OK");
      digitalWrite(8,LOW);
      digitalWrite(9,HIGH);
      delay(1000);
      digitalWrite(9,LOW);
      digitalWrite(8,HIGH);
      if(checkNetwork())
      { 
        Serial.println("Network Ok");
        Serial.println("Waiting For SMS...");
        digitalWrite(8,LOW);  
        digitalWrite(9,HIGH);
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

/********main LOOP
*********you can add your conditions here and write your speccific script at the sendSMS("your preffered text")
**********/

void loop()
{
    updateLatestSMSIndex();
    if(latestSMSIndex != -1)
    {
        String message = readSMS(latestSMSIndex);
        Serial.print("Received: ");
        Serial.println(message);
        if(message == "green on")
        {
            sendSMS("Green On recieved");
            for(int i = 0; i < 50; i++)
            {
                digitalWrite(8, HIGH);
                digitalWrite(9, LOW);
                delay(100);
                digitalWrite(8, LOW);
                digitalWrite(9, HIGH);
                delay(100);
            }
          deleteSMS(latestSMSIndex);
          latestSMSIndex = -1;
        }
        else if(message == "red on")
        {
            sendSMS("Red On recieved");
            for(int i = 0; i < 50; i++)
            {
                digitalWrite(8, LOW);
                digitalWrite(9, HIGH);
                delay(100);
                digitalWrite(8, HIGH);
                digitalWrite(9, LOW);
                delay(100);
            }
            deleteSMS(latestSMSIndex);
            latestSMSIndex = -1;
        }
        else if(message == "both on")
        {
            sendSMS("Both On recieved");
            for(int i = 0; i < 50; i++)
            {
                digitalWrite(8, LOW);
                digitalWrite(9, HIGH);
                delay(100);
                digitalWrite(8, HIGH);
                digitalWrite(9, LOW);
                delay(100);
            }
            deleteSMS(latestSMSIndex);
            latestSMSIndex = -1;
            digitalWrite(8, HIGH);
            digitalWrite(9, HIGH);
        }
        else if(message == "off")
        {
            sendSMS("Off recieved");
            for(int i = 0; i < 50; i++)
            {
                digitalWrite(8, LOW);
                digitalWrite(9, HIGH);
                delay(100);
                digitalWrite(8, HIGH);
                digitalWrite(9, LOW);
                delay(100);
            }
             digitalWrite(8, LOW);
             digitalWrite(9, LOW);            
          deleteSMS(latestSMSIndex);
          latestSMSIndex = -1;
         }
         else
         {
          sendSMS("Wrong Input");
          deleteSMS(latestSMSIndex);
          latestSMSIndex = -1;
          }
    }
}
