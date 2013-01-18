/* 

Unlockatron
 
A device used to remotely unlock a door using a simple web server to check 
password input and a servo motor to turn the door handle.
 
Created and tested using an Arduino Uno, Ethernet Shield, and standard servo motor

Created January 2013
By Connor Ford

This code is public domain.
 
*/

#include <Servo.h> 
#include <SPI.h>
#include <Ethernet.h>

// Constants
const String password = "mypassword";
const int failure_led = 6;
const int success_led = 5;

// Web Server variables
byte mac[] = { 
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
EthernetServer server(80);
String buffer = String(200);
boolean is_post = false;

// Servo variables
Servo serv; 
int serv_pos = 0; 

// LED variables
long previous_millis = 0;
int led_state = LOW;
int blink_count = 0;
boolean failure_blink = false;
boolean success_blink = false;

// Function declarations

// Called every main loop iteration to blink status LEDs based on difference in 
// milliseconds since program started. Avoids the need to use delay() for blinking.
void check_led_blink() {
  if (failure_blink || success_blink) {
    unsigned long current_millis = millis();
      if (current_millis - previous_millis > 100) {
        previous_millis = current_millis;
        blink_count += 1;
        if (led_state == LOW)
          led_state = HIGH;
        else
          led_state = LOW;
       
      if (failure_blink)         
        digitalWrite(failure_led, led_state);
      else if (success_blink)  
        digitalWrite(success_led, led_state);
        
      if (blink_count >= 6) {
        blink_count = 0;
        digitalWrite(failure_led, LOW);
        digitalWrite(success_led, LOW);
        failure_blink = false;
        success_blink = false;
      }
    }
  }   
}

// Takes buffer containing HTTP request body as a parameter, parses and 
// compares the password, and outputs appropriate segment of response.
boolean check_password(String buffer, EthernetClient client) {
  int pw_start = buffer.indexOf('=') + 1;
  String pw_input = buffer.substring(pw_start);            
  if (pw_input.equals(password)) {
    success_blink = true;
    client.println("<span style='color:green'> Success! </span>");
    // Control servo to unlock door
    for(serv_pos = 20; serv_pos < 180; serv_pos += 1) {
      serv.write(serv_pos);
      delay(10);
    }
    delay(100);    
    serv.write(20);       
  }
  else {
    failure_blink = true;
    client.println("<span style='color:red'> Failure. Try again. </span>"); 
  }
}

// Setup
void setup() { 
  serv.attach(9);   
  serv.write(20);

  Ethernet.begin(mac);
  server.begin();
  Serial.begin(9600);
  Serial.print("Server running at: ");
  Serial.println(Ethernet.localIP());
  
  pinMode(failure_led, OUTPUT);
  pinMode(success_led, OUTPUT);
} 
 
// Main Loop
void loop() { 

  check_led_blink();
  
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Serial.println("new client");
    boolean current_line_blank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        
        // End of current requset line. Check for POST request and dump buffer.
        if (c == '\n') {
          if (buffer.substring(0, 4) == "POST") {
            is_post = true;
          }
          Serial.println(buffer);
          buffer = "";
        }
        else {
          buffer += c;
        }
        
        // End of request, construct a response.
        if (c == '\n' && current_line_blank) {
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connnection: close");
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
        
          // If request is POST, read body content to get post data.
          if (is_post) {
            buffer = "";
            while(true) {
              char c = client.read();
              if (c != -1) {
                buffer += c;
              }
              else {                    
                check_password(buffer, client);
                buffer = "";
                break;     
              } 
            }
          }
               
          client.println("<form method='POST'>");
          client.println("PASSWORD: <input name='pw' type='password' />");
          client.println("<input type='submit' />");
          client.println("</form>");
          client.println("</html>");
          break;
        }
        
        if (c == '\n') {
          current_line_blank = true;
        } 
        else if (c != '\r') {
          current_line_blank = false;
        }
      }
    }
    
    // Give web browser time to receive data
    delay(1);

    client.stop();    
    is_post = false;
    Serial.println("client disonnected");
  }  
} 
