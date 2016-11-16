/*
  Chat Server

  A simple server that distributes any incoming messages to all
  connected clients.  To use, telnet to your device's IP address and type.
  You can see the client's input in the serial monitor as well.

  created 18 Dec 2009
  by David A. Mellis
  modified 9 Apr 2012
  by Tom Igoe
  modified 8 Jul 2016
  by Chris Shoemaker
*/

#include <SPI.h>
#include <Ethernet.h>

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network.
// gateway and subnet are optional:
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(192, 168, 0, 86);
IPAddress myDns(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);

// telnet defaults to port 23
EthernetServer server(23);
boolean alreadyConnected = false; // whether or not the client was connected previously

String g_inputStream = ""; // input stream to carry a command given by the client
int number = 10; // magic number initialized to 10
String newNum; // input stream for changing the magic number

void setup() {
  // initialize the ethernet device
  Ethernet.begin(mac, ip, myDns, gateway, subnet);
  // start listening for clients
  server.begin();
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.print("Server address is ");
  Serial.println(Ethernet.localIP());
}

void loop() {
  // wait for a new client:
  EthernetClient client = server.available();

  // when the client sends the first byte, say hello:
  if (client) {
    if (!alreadyConnected) {
      // clear out the input buffer:
      client.flush();
      Serial.println("New client is online");
      client.println("Please type a command and press Enter to finish");
      client.println("Current magic number is: " + String(number));
      alreadyConnected = true;
    }

    if (client.available() > 0) {
      // read the bytes incoming from the client:
      char thisChar = client.read();
      writeToServer(thisChar, client);
    }
  }
}

// Processes a few (simple) different user-inputted commands
void writeToServer(char input, EthernetClient client) {
  if (input != '\n') {
    if (input >= 32 && input <= 126) {
      g_inputStream.concat(input);
    } else {
      if (g_inputStream.length() < 2) { // if command is too short
        g_inputStream = "";
      } else if (g_inputStream.equalsIgnoreCase("Hello") || g_inputStream.equalsIgnoreCase("Hi")) { // send greeting to server, receive greeting in return
        Serial.println("Hello, what would you like to do?");
      } else if (g_inputStream.equalsIgnoreCase("Wait") || g_inputStream.equalsIgnoreCase("Begin") || g_inputStream.equalsIgnoreCase("Start")) { // tell server to wait
        Serial.println("Waiting for two seconds...");
        delay(2000);
        Serial.println("Done.");
      } else if (g_inputStream.startsWith("Change number")) {
        changeMagicNumber(input, client);
      } else if (g_inputStream.startsWith("What is the magic number?")) { // get magic number
        Serial.println("The current magic number is " + String(number) + ".");
        client.println("The current magic number is " + String(number) + ".");
      } else if (g_inputStream.startsWith("Add") || g_inputStream.startsWith("add")) {
        addNumbers(input, client);
      } else { // if no such command exists
        ;
      }
      g_inputStream = ""; // clear input stream after executing a command
    }
  }
}

void changeMagicNumber(char input, EthernetClient client) {
  client.println("Enter a number.");
  client.read();
  while (input != '\n') {
    char digit = client.read();
    if (digit >= 48 && digit <= 57) {
      newNum.concat(digit);
    } else if (digit == '\n') {
      break;
    }
  }
  number = newNum.toInt(); // change magic number to user input
  client.println("Magic number is now: " + String(number));
  newNum = "";
}

void addNumbers(char input, EthernetClient client) {
  int num1, num2;
  client.println("Enter a number.");
  client.read();
  while (input != '\n') {
    char digit = client.read();
    if (digit >= 48 && digit <= 57) {
      newNum.concat(digit);
    } else if (digit == '\n') {
      break;
    }
  }
  num1 = newNum.toInt(); // number to user input
  newNum = "";
  client.println("Enter a second number.");
  client.read();
  while (input != '\n') {
    char digit = client.read();
    if (digit >= 48 && digit <= 57) {
      newNum.concat(digit);
    } else if (digit == '\n') {
      break;
    }
  }
  num2 = newNum.toInt();
  int sum = num1 + num2;
  newNum = "";
  client.println("The sum of " + String(num1) + " and " + String(num2) + " is " + String(sum) + ".");
}

