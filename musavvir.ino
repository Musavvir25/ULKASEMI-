#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include<time.h>
#include <FS.h>
#include <WebServer.h>
#include <Wire.h>                    
//#include <LiquidCrystal_I2C.h> 
#include <Preferences.h>
#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>
#include <SPIFFS.h>
#include <Keypad.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <set>
#include <map>
#include <SD.h>
#include <RTClib.h>
#include <Wire.h>
#include <hd44780.h>                    
#include <hd44780ioClass/hd44780_I2Cexp.h>
hd44780_I2Cexp lcd;
RTC_DS3231 rtc;
#define MAX_STUDENTS 100  // Adjust this based on the maximum number of students
bool offlineDataSent = false;
bool isFingerprintSensorOperational = true; // Flag to track sensor status
bool isFingerprintEnabled = false;
QueueHandle_t supabaseQueue = xQueueCreate(10, sizeof(String*));


// Remove default parameters from declaration
bool sendToSupabase(const char* table, const String& jsonData, const String& method, int recordId);
bool attendedStudentsarray[MAX_STUDENTS] = {false};  // Array to track attendance
bool isAttendanceMode = false;

//56

std::vector<String> parseCSVLine(const String& line, const std::vector<int>& columns) {
    std::vector<String> result;
    int columnIndex = 0;
    int startIndex = 0;
    int foundIndex = line.indexOf(',');

    while (foundIndex != -1) {
        if (std::find(columns.begin(), columns.end(), columnIndex) != columns.end()) {
            result.push_back(line.substring(startIndex, foundIndex));
        }
        columnIndex++;
        startIndex = foundIndex + 1;
        foundIndex = line.indexOf(',', startIndex);
    }

    // Check last column after the last comma
    if (std::find(columns.begin(), columns.end(), columnIndex) != columns.end()) {
        result.push_back(line.substring(startIndex));
    }

    return result;
}

Preferences preferences;
const byte ROWS = 4;
const byte COLS = 4; 
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[COLS] = {26, 25, 33, 32}; // Adjust as per your wiring
byte colPins[ROWS] = {13, 15, 27, 4};   // Adjust as per your wiring
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
#define RX_PIN 16
#define TX_PIN 17
#define MAX_RETRIES 3
#define RETRY_DELAY 5000
SoftwareSerial mySerial(RX_PIN, TX_PIN);
Adafruit_Fingerprint finger(&mySerial);
WebServer server(80);
String ssid = "";
String password = "";
String coursecode = "";
const char* adminUsername = "admin";
const char* adminPassword = "admin123";
const char* teacherDataFile = "/teachers.csv";
const char* studentDataFile = "/students.csv";
const char* attendanceDataFile = "/attendance.csv";
const char* SUPABASE_URL = "https://vwhxnfnvtsvycwgmcmwh.supabase.co";
const char* SUPABASE_KEY = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6InZ3aHhuZm52dHN2eWN3Z21jbXdoIiwicm9sZSI6ImFub24iLCJpYXQiOjE3MzY4NzYyNTksImV4cCI6MjA1MjQ1MjI1OX0.o29ooc_KF-weSvlHqxUOjeRxuHmt7_hA6PLnilMWbS4";
const char* offlineAttendanceFile = "/offline_attendance.csv"; 
void handleEditTeacher();
void handleEditStudent();
void handleUpdateTeacher();
void handleUpdateStudent();
void handleDeleteTeacher();
void handleDeleteStudent();
void handleFileRequest();
void handleSDDownloadAttendance();
void handleSDDownloadStudents();
void handleSDDownloadTeachers();
void handleSDViewStudents();
void handleSDViewTeachers();
void handleSDImportStudents();
void handleSDImportTeachers();



void handleDownloadAttendanceByDate();


void sendOfflineDataToSupabase();
void handleDownloadOfflineData();
void saveOfflineAttendance();
void handleKeypadInput();
void checkWiFiConnection();

//1 for edit teacher no need to supabase or sd
void handleEditTeacher() {
    if (!authenticateUser()) {
        return;
    }

    String htmlPage = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
        <style>
            table { width: 100%; border-collapse: collapse; margin-bottom: 20px; }
            th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
            th { background-color: #4CAF50; color: white; }
            .delete-btn {
                background-color: #f44336;
                color: white;
                padding: 5px 10px;
                border: none;
                cursor: pointer;
            }
            .container { width: 80%; margin: 0 auto; }
            h2 { text-align: center; }
        </style>
    </head>
    <body>
        <div class="container">
            <h2>Edit Teachers</h2>
            <table>
                <tr>
                    <th>ID</th>
                    <th>Name</th>
                    <th>Course Name</th>
                    <th>Course Code</th>
                    <th>Actions</th>
                </tr>
    )rawliteral";

    File file = SPIFFS.open(teacherDataFile, "r");
    if (!file) {
        server.send(500, "text/html", "Error opening teacher data file.");
        return;
    }

    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        if (line.length() > 0) {
            int firstComma = line.indexOf(',');
            int secondComma = line.indexOf(',', firstComma + 1);
            int thirdComma = line.indexOf(',', secondComma + 1);
            String id = line.substring(0, firstComma);
            String name = line.substring(firstComma + 1, secondComma);
            String courseName = line.substring(secondComma + 1, thirdComma);
            String courseCode = line.substring(thirdComma + 1);

            htmlPage += "<tr><td>" + id + "</td><td>" + name + "</td><td>" + courseName + "</td><td>" + courseCode + 
                        "</td><td><button class='delete-btn' onclick='deleteTeacher(\"" + id + 
                        "\")'>Delete</button></td></tr>";
        }
    }
    file.close();

    htmlPage += R"rawliteral(
            </table>
            <script>
                function deleteTeacher(id) {
                    if (confirm("Are you sure you want to delete this teacher?")) {
                        window.location.href = `/deleteTeacher?id=${id}`;
                    }
                }
            </script>
        </div>
    </body>
    </html>
    )rawliteral";

    server.send(200, "text/html", htmlPage);
}

//2 
void handleUpdateTeacher() {
    if (!authenticateUser ()) {
        return;
    }

    // Get parameters from the request
    String id = server.arg("id");
    String newName = server.arg("name");
    String newCourseName = server.arg("courseName");
    String newCourseCode = server.arg("courseCode");

    // Ensure all parameters are provided
    if (id.isEmpty() || newName.isEmpty() || newCourseName.isEmpty() || newCourseCode.isEmpty()) {
        server.send(400, "text/plain", "Invalid input parameters. All fields are required.");
        return;
    }

    // Open files for reading and writing
    File oldFile = SPIFFS.open(teacherDataFile, "r");
    File newFile = SPIFFS.open("/temp.csv", "w");

    bool recordUpdated = false; // Track if the record was updated

    while (oldFile.available()) {
        String line = oldFile.readStringUntil('\n');
        line.trim();

        // Check if the line starts with the ID we want to update
        if (line.startsWith(id + ",")) {
            // Write updated data to the new file
            newFile.println(id + "," + newName + "," + newCourseName + "," + newCourseCode);
            recordUpdated = true;

            
        } else {
            // Write unchanged lines to the new file
            newFile.println(line);
        }
    }

    oldFile.close();
    newFile.close();

    // Replace the old file with the new one
    SPIFFS.remove(teacherDataFile);
    SPIFFS.rename("/temp.csv", teacherDataFile);

    // Handle response to the user
    if (recordUpdated) {
        server.sendHeader("Location", "/editTeacher"); // Redirect to edit teacher page
        server.send(303);
    } else {
        server.send(404, "text/plain", "Teacher record not found.");
    }
}
//3 delete
void handleDeleteTeacher() {
    if (!authenticateUser()) return;

    String id = server.arg("id");
    if (id.isEmpty()) {
        server.send(400, "text/html", "Error: Invalid ID");
        return;
    }

    // 1. Delete from SPIFFS
    bool deleted = false;
    File oldFile = SPIFFS.open(teacherDataFile, "r");
    File newFile = SPIFFS.open("/temp_teachers.csv", "w");

    if (!oldFile || !newFile) {
        server.send(500, "text/html", "Error: File Access Failed");
        if (oldFile) oldFile.close();
        if (newFile) newFile.close();
        return;
    }

    while (oldFile.available()) {
        String line = oldFile.readStringUntil('\n');
        if (!line.startsWith(id + ",")) {
            newFile.println(line);
        } else {
            deleted = true;
        }
    }

    oldFile.close();
    newFile.close();

    if (deleted) {
        SPIFFS.remove(teacherDataFile);
        if (!SPIFFS.rename("/temp_teachers.csv", teacherDataFile)) {
            SPIFFS.remove("/temp_teachers.csv");
            server.send(500, "text/html", "Error: Update Failed");
            return;
        }
    } else {
        SPIFFS.remove("/temp_teachers.csv");
    }

    // 2. Send response IMMEDIATELY
    server.sendHeader("Location", "/editTeacher?deleted=" + String(deleted ? "1" : "0"));
    server.send(303);

    // 3. Sync with Supabase in background (FreeRTOS task)
    if (deleted && WiFi.status() == WL_CONNECTED) {
        xTaskCreate(
            [](void* param) {
                String* id = (String*)param;
                sendToSupabase("teachers", "", "DELETE", id->toInt());
                delete id;
                vTaskDelete(NULL);
            },
            "supabase_delete_teacher",
            4096,
            new String(id),
            1,
            NULL
        );
    }
}

// 4
void handleEditStudent() {
    if (!authenticateUser()) {
        return;
    }

    String htmlPage = R"rawliteral(
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Edit Students</title>
        <style>
            body { font-family: Arial, sans-serif; background-color: #f4f4f4; margin: 0; padding: 20px; }
            table { width: 100%; border-collapse: collapse; margin-bottom: 20px; }
            th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
            th { background-color: #4CAF50; color: white; }
            .delete-btn {
                background-color: #f44336;
                color: white;
                padding: 5px 10px;
                border: none;
                cursor: pointer;
            }
            h2 { color: #333; }
        </style>
    </head>
    <body>
        <h2>Edit Students</h2>
        <table>
            <thead>
                <tr>
                    <th>ID</th>
                    <th>Name</th>
                    <th>Roll Number</th>
                    <th>Actions</th>
                </tr>
            </thead>
            <tbody>
    )rawliteral";

    File file = SPIFFS.open(studentDataFile, "r");
    if (!file) {
        server.send(500, "text/html", "Error reading student data.");
        return;
    }

    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();
        if (line.length() > 0) {
            int firstComma = line.indexOf(',');
            int lastComma = line.lastIndexOf(',');
            String id = line.substring(0, firstComma);
            String name = line.substring(firstComma + 1, lastComma);
            String rollNumber = line.substring(lastComma + 1);

            htmlPage += "<tr><td>" + id + "</td><td>" + name + "</td><td>" + rollNumber + 
                        "</td><td><button class='delete-btn' onclick='deleteStudent(\"" + id + 
                        "\")'>Delete</button></td></tr>";
        }
    }

    file.close();

    htmlPage += R"rawliteral(
            </tbody>
        </table>
        <script>
            function deleteStudent(id) {
                if (confirm("Are you sure you want to delete this student?")) {
                    window.location.href = `/deleteStudent?id=${id}`;
                }
            }
        </script>
    </body>
    </html>
    )rawliteral";

    server.send(200, "text/html", htmlPage);
}

//5
void handleUpdateStudent() {
    if (!authenticateUser()) {
        return;
    }

    String id = server.arg("id");
    String newName = server.arg("name");
    String newRollNumber = server.arg("rollNumber");

    // Open the old file for reading
    File oldFile = SPIFFS.open(studentDataFile, "r");
    if (!oldFile) {
        Serial.println("Failed to open the student data file for reading.");
        server.send(500, "text/html", "Failed to read student data.");
        return;
    }

    // Create a new file to store the updated data
    File newFile = SPIFFS.open("/temp.csv", "w");
    if (!newFile) {
        Serial.println("Failed to open the temp file for writing.");
        oldFile.close();
        server.send(500, "text/html", "Failed to update student data.");
        return;
    }

    bool updated = false;
    while (oldFile.available()) {
        String line = oldFile.readStringUntil('\n');

        // If the line starts with the student ID, update it
        if (line.startsWith(id + ",")) {
            newFile.println(id + "," + newName + "," + newRollNumber);
            updated = true;  // Mark that we updated the student
        } else {
            newFile.println(line);
        }
    }

    oldFile.close();
    newFile.close();

    // Check if the student was updated
    if (!updated) {
        Serial.println("Student ID not found.");
        server.send(404, "text/html", "Student not found.");
        return;
    }

    // Replace the old file with the updated file
    if (SPIFFS.remove(studentDataFile)) {
        if (SPIFFS.rename("/temp.csv", studentDataFile)) {
            Serial.println("Student updated successfully.");
        } else {
            Serial.println("Failed to rename the temp file.");
            server.send(500, "text/html", "Failed to update student data.");
            return;
        }
    } else {
        Serial.println("Failed to remove the old student data file.");
        server.send(500, "text/html", "Failed to update student data.");
        return;
    }

    // Redirect to the edit student page after updating
    server.sendHeader("Location", "/editStudent");
    server.send(303);
}

//6
void handleDeleteStudent() {
    if (!authenticateUser()) return;

    String id = server.arg("id");
    if (id.isEmpty()) {
        server.send(400, "text/html", "Error: Invalid ID");
        return;
    }

    // 1. Delete from SPIFFS
    bool deleted = false;
    File oldFile = SPIFFS.open(studentDataFile, "r");
    File newFile = SPIFFS.open("/temp_students.csv", "w");

    if (!oldFile || !newFile) {
        server.send(500, "text/html", "Error: File Access Failed");
        if (oldFile) oldFile.close();
        if (newFile) newFile.close();
        return;
    }

    while (oldFile.available()) {
        String line = oldFile.readStringUntil('\n');
        if (!line.startsWith(id + ",")) {
            newFile.println(line);
        } else {
            deleted = true;
        }
    }

    oldFile.close();
    newFile.close();

    if (deleted) {
        SPIFFS.remove(studentDataFile);
        if (!SPIFFS.rename("/temp_students.csv", studentDataFile)) {
            SPIFFS.remove("/temp_students.csv");
            server.send(500, "text/html", "Error: Update Failed");
            return;
        }
    } else {
        SPIFFS.remove("/temp_students.csv");
    }

    // 2. Send response IMMEDIATELY
    server.sendHeader("Location", "/editStudent?deleted=" + String(deleted ? "1" : "0"));
    server.send(303);

    // 3. Sync with Supabase in background (FreeRTOS task)
    if (deleted && WiFi.status() == WL_CONNECTED) {
        xTaskCreate(
            [](void* param) {
                String* id = (String*)param;
                sendToSupabase("students", "", "DELETE", id->toInt());
                delete id;
                vTaskDelete(NULL);
            },
            "supabase_delete_student",
            4096,
            new String(id),
            1,
            NULL
        );
    }
}
//7

bool initializeFingerprint() {
    mySerial.begin(57600);
    finger.begin(57600);

    // Short delay to ensure proper initialization
    delay(100);

    int retry = 0;
    const int maxRetries = 3;
    
    while (!finger.verifyPassword()) {
        // If we have tried too many times, display an error message
        if (retry >= maxRetries) {
            Serial.println("Fingerprint sensor not detected or communication error!");
            isFingerprintSensorOperational = false;  // Set flag to false
            
            // Display error message on LCD
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Sensor Error!");
            lcd.setCursor(0, 1);
            lcd.print("Check Connection");
            
            return false;  // Return false to indicate failure
        }

        // Retry initialization with an increasing delay between retries
        Serial.println("Retrying sensor connection...");
        delay(1000 * (retry + 1));  // Exponential backoff for retries
        retry++;
    }

    // Successfully initialized fingerprint sensor
    Serial.println("Fingerprint sensor initialized successfully!");
    isFingerprintSensorOperational = true;  // Set flag to true

    return true;  // Return true to indicate success
}

//8

bool sendToSupabase(const char* table, const String& jsonData, const String& method, int recordId) {
    HTTPClient http;
    http.setTimeout(10000); // 10-second timeout

    String url = String(SUPABASE_URL) + "/rest/v1/" + table;

    // Build Supabase query URL
    if (method == "DELETE" || method == "PATCH") {
        if (String(table) == "teachers") {
            url += "?teacher_id=eq." + String(recordId);
        } else if (String(table) == "students") {
            url += "?student_id=eq." + String(recordId);
        }
    }

    http.begin(url);
    http.addHeader("apikey", SUPABASE_KEY);
    http.addHeader("Authorization", "Bearer " + String(SUPABASE_KEY));
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Prefer", "return=minimal");

    int httpCode = 0;
    if (method == "DELETE") {
        httpCode = http.sendRequest("DELETE", jsonData);
    } else if (method == "PATCH") {
        httpCode = http.sendRequest("PATCH", jsonData);
    } else {
        httpCode = http.POST(jsonData);
    }

    bool success = (httpCode == 200 || httpCode == 201 || httpCode == 204);
    http.end(); // Ensure connection is closed

    return success;
}

//9

bool checkSerialExists(const String& serial) {
    // Prepare URL for the GET request to check if the serial exists
    String url = String(SUPABASE_URL) + "/rest/v1/ips?serial=eq." + serial;

    // Initialize HTTP client
    HTTPClient http;
    http.begin(url);
    http.addHeader("apikey", SUPABASE_KEY); // API Key for authorization
    http.addHeader("Authorization", "Bearer " + String(SUPABASE_KEY)); // Bearer token
    http.addHeader("Content-Type", "application/json"); // Content-Type header for JSON

    // Send GET request to check if serial exists
    int httpCode = http.GET();
    bool exists = false;

    // Check if the HTTP request was successful (200 OK)
    if (httpCode == 200) {
        String response = http.getString();

        // Log the response for debugging purposes
        Serial.println("Response: " + response);

        // A valid response will typically contain data if the serial exists
        if (response.length() > 2) { // Assuming a valid response will have more than just brackets
            exists = true;
        } else {
            Serial.println("No data found for serial: " + serial);
        }
    } else {
        // Log the error with the HTTP response code
        String errorMsg = "Error checking serial: " + String(httpCode);
        if (httpCode == 0) {
            errorMsg += " (Possible connection error or timeout)";
        }
        Serial.println(errorMsg);
    }

    // Clean up and close the HTTP connection
    http.end();
    return exists;
}

//10
bool updateIPAddress(const String& serial, const String& ip) {
    // Prepare JSON document for the update
    StaticJsonDocument<200> doc;
    doc["ip"] = ip; // New IP address to update
    String jsonString;
    serializeJson(doc, jsonString);

    // Prepare URL for the request
    String url = String(SUPABASE_URL) + "/rest/v1/ips?serial=eq." + serial;

    // Initialize HTTP client and set headers
    HTTPClient http;
    http.begin(url);
    http.addHeader("apikey", SUPABASE_KEY); // Supabase API Key
    http.addHeader("Authorization", "Bearer " + String(SUPABASE_KEY)); // Bearer token for authentication
    http.addHeader("Content-Type", "application/json"); // Content type as JSON
    http.addHeader("Prefer", "return=representation"); // Request updated data on success

    // Send PATCH request to update the IP address
    int httpCode = http.PATCH(jsonString);

    // Check the HTTP response code
    if (httpCode == 200 || httpCode == 204) {
        // If response code is 200 (OK) or 204 (No Content), the update is successful
        Serial.println("IP address updated successfully.");
        http.end();
        return true;
    } else {
        // Handle error if response code is not 200 or 204
        String errorMessage = "Failed to update IP address. HTTP Code: " + String(httpCode);
        if (httpCode > 0) {
            String responseBody = http.getString(); // Get response body for debugging
            Serial.println("Response: " + responseBody);
            errorMessage += "\nResponse Body: " + responseBody;
        } else {
            errorMessage += "\nError: " + String(http.errorToString(httpCode));
        }

        Serial.println(errorMessage);
        http.end();
        return false;
    }
}

//11
bool connectToWiFi(const char* ssid, const char* password) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Connecting...");
    Serial.print("Connecting to Wi-Fi");

    int retryCount = 0;
    const int maxRetries = 30;  // Retry up to 30 times (15 seconds)

    while (WiFi.status() != WL_CONNECTED && retryCount < maxRetries) {
        delay(500);
        Serial.print(".");
        retryCount++;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\nFailed to connect to Wi-Fi");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Wi-Fi Failed");
        delay(2000);
        return false;
    }

    Serial.println("\nConnected to Wi-Fi!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Connected");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP().toString());

    // Check if serial number exists and update the IP or add a new entry
    String serial = "1";  // Unique serial number (set according to your system)
    String ip = WiFi.localIP().toString();

    // Check if serial number exists and update the IP or add a new entry
    if (checkSerialExists(serial)) {
        if (updateIPAddress(serial, ip)) {
            Serial.println("IP address updated successfully.");
        } else {
            Serial.println("Failed to update IP address.");
            return false;
        }
    } else {
        StaticJsonDocument<200> doc;
        doc["serial"] = serial;  // Serial number
        doc["ip"] = ip;          // IP address
        String jsonString;
        serializeJson(doc, jsonString);

        if (!sendToSupabase("ips", jsonString, "POST", -1)) {
            Serial.println("Failed to send IP address to Supabase");
            return false;
        }
    }

    // Send offline data after connecting to Wi-Fi
    sendOfflineDataToSupabase();
    offlineDataSent = true; // Set the flag to true after sending

    return true;
}

//12

// Start Wi-Fi in AP mode
void startAccessPoint(const String& apSSID = "ESP32_Config", const String& apPassword = "123456789") {
    // Set Wi-Fi mode to Access Point (AP)
    WiFi.mode(WIFI_AP);
    
    // Attempt to start the Access Point
    if (WiFi.softAP(apSSID.c_str(), apPassword.c_str())) {
        Serial.println("Access Point Started"); // Log success
        IPAddress apIP = WiFi.softAPIP(); // Get the IP address of the AP
        Serial.print("AP IP Address: ");
        Serial.println(apIP);
        
        // Display on LCD
        lcd.clear(); // Clear the LCD
        lcd.setCursor(0, 0); // Set cursor to the first row
        lcd.print("AP Started");
        lcd.setCursor(0, 1); // Set cursor to the second row
        lcd.print("IP: ");
        lcd.print(apIP); // Display the actual IP address

        // Optionally, print the number of connected clients to serial monitor
        int numClients = WiFi.softAPgetStationNum();
        Serial.print("Number of clients connected: ");
        Serial.println(numClients);
    } else {
        Serial.println("Error: Failed to start Access Point."); // Log the error
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("AP Start Failed");
        lcd.setCursor(0, 1);
        lcd.print("Please try again");
    }
}

//13

// Authenticate User
bool authenticateUser() {
  // Check if the user provided credentials are valid
  if (!server.authenticate(adminUsername, adminPassword)) {
    // Log unauthorized attempt for monitoring purposes
    Serial.println("Authentication failed: Unauthorized access attempt.");
    
    // Send authentication request to the client (browser)
    server.requestAuthentication();

    // Optionally, you can send an HTTP error message instead of just requesting authentication
    server.send(401, "text/html", "<h2>Authentication Required</h2><p>Please enter valid credentials.</p>");

    // Return false to indicate failed authentication
    return false;
  }

  // Log successful authentication for debugging purposes
  Serial.println("Authentication successful.");
  
  // Return true to indicate successful authentication
  return true;
}

//14
void handleRoot() {
    if (!authenticateUser()) return;

    String wifiStatus = (WiFi.status() == WL_CONNECTED) ? 
                   "<i class='fas fa-wifi' style='color:#4CAF50'></i> Connected (" + WiFi.SSID() + ")" : 
                   "<i class='fas fa-wifi-slash' style='color:#f44336'></i> Offline";

    String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
        <title>JUST Attendance System</title>
        <style>
            :root {
                --primary: #2c3e50;
                --secondary: #3498db;
                --accent: #e74c3c;
                --background: #f8f9fa;
            }

            body {
                margin: 0;
                padding: 20px;
                font-family: 'Segoe UI', system-ui;
                background: linear-gradient(135deg, #ece9e6, #ffffff);
                min-height: 100vh;
            }

            .dashboard {
                max-width: 1200px;
                margin: 0 auto;
                background: rgba(255, 255, 255, 0.95);
                backdrop-filter: blur(10px);
                border-radius: 20px;
                box-shadow: 0 8px 32px rgba(0, 0, 0, 0.1);
                padding: 30px;
            }

            .header {
                text-align: center;
                padding: 20px;
                border-bottom: 2px solid var(--primary);
                margin-bottom: 30px;
            }

            .logo {
                height: 80px;
                margin-bottom: 15px;
                filter: drop-shadow(0 2px 4px rgba(0,0,0,0.1));
            }

            .grid {
                display: grid;
                grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
                gap: 25px;
                padding: 20px;
            }

            .card {
                background: white;
                border-radius: 15px;
                padding: 25px;
                transition: transform 0.3s, box-shadow 0.3s;
                border: 1px solid rgba(0,0,0,0.1);
            }

            .card:hover {
                transform: translateY(-5px);
                box-shadow: 0 8px 25px rgba(0, 0, 0, 0.1);
            }

            .card-title {
                font-size: 1.4em;
                color: var(--primary);
                margin-bottom: 15px;
                display: flex;
                align-items: center;
                gap: 10px;
            }

            .card-icon {
                font-size: 1.6em;
                color: var(--secondary);
            }

            .menu-item {
                display: flex;
                align-items: center;
                padding: 12px 20px;
                margin: 8px 0;
                border-radius: 10px;
                background: #f8f9fa;
                transition: all 0.2s;
                text-decoration: none;
                color: var(--primary);
                gap: 12px;
            }

            .menu-item:hover {
                background: var(--secondary);
                color: white !important;
                transform: translateX(5px);
            }

            .menu-item i {
                width: 25px;
                text-align: center;
            }

            .status-bar {
                display: flex;
                justify-content: space-between;
                align-items: center;
                background: var(--primary);
                color: white;
                padding: 15px 30px;
                border-radius: 12px;
                margin-bottom: 30px;
            }

            .danger-zone {
                border: 2px solid var(--accent);
                border-radius: 15px;
                padding: 20px;
                margin-top: 30px;
                background: #fff5f5;
            }
        </style>
        <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.4.0/css/all.min.css">
    </head>
    <body>
        <div class="dashboard">
            <div class="header">
                <img src="https://just.edu.bd/logo/download.png" class="logo" alt="JUST Logo">
                <h1>Smart Attendance System</h1>
                <p>Department of EEE - Developed by Musavvir, Rifat, Tanim, Pitom</p>
            </div>

            <div class="status-bar">
                <div><i class="fas fa-wifi"></i> Network Status: %WIFI_STATUS%</div>
                <div><i class="fas fa-microchip"></i> ESP32 | Free Memory: %HEAP% bytes</div>
            </div>

            <div class="grid">
                <!-- Teachers Card -->
                <div class="card">
                    <div class="card-title">
                        <i class="card-icon fas fa-chalkboard-teacher"></i>
                        Teacher Management
                    </div>
                    <a href="/enrollTeacher" class="menu-item">
                        <i class="fas fa-fingerprint"></i> Enroll Teacher
                    </a>
                    <a href="/editTeacher" class="menu-item">
                        <i class="fas fa-edit"></i> Edit Teachers
                    </a>
                    <a href="/downloadTeachers" class="menu-item">
                        <i class="fas fa-download"></i> Download Data
                    </a>
                </div>

                <!-- Students Card -->
                <div class="card">
                    <div class="card-title">
                        <i class="card-icon fas fa-user-graduate"></i>
                        Student Management
                    </div>
                    <a href="/enrollStudent" class="menu-item">
                        <i class="fas fa-fingerprint"></i> Enroll Student
                    </a>
                    <a href="/editStudent" class="menu-item">
                        <i class="fas fa-edit"></i> Edit Students
                    </a>
                    <a href="/downloadStudents" class="menu-item">
                        <i class="fas fa-download"></i> Download Data
                    </a>
                </div>

                <!-- SD Card Management -->
                <div class="card">
                    <div class="card-title">
                        <i class="card-icon fas fa-sd-card"></i>
                        SD Card Operations
                    </div>
                    <div class="sub-grid">
                        <a href="/sdImportTeachers" class="menu-item">
                            <i class="fas fa-file-import"></i> Import Teachers
                        </a>
                        <a href="/sdImportStudents" class="menu-item">
                            <i class="fas fa-file-import"></i> Import Students
                        </a>
                        <a href="/sdDownloadAttendance" class="menu-item">
                            <i class="fas fa-history"></i> Full Attendance
                        </a>
                    </div>
                </div>

                <!-- System Card -->
                <div class="card">
                    <div class="card-title">
                        <i class="card-icon fas fa-cogs"></i>
                        System Configuration
                    </div>
                    <a href="/wifi" class="menu-item">
                        <i class="fas fa-wifi"></i> Network Settings
                    </a>
                    <a href="/update" class="menu-item">
                        <i class="fas fa-sync"></i> Firmware Update
                    </a>
                    <a href="/stats" class="menu-item">
                        <i class="fas fa-chart-bar"></i> System Statistics
                    </a>
                </div>
            </div>

            <div class="danger-zone">
                <h3 style="color: var(--accent); margin-top: 0;">
                    <i class="fas fa-exclamation-triangle"></i> Danger Zone
                </h3>
                <a href="/deleteAllFiles" class="menu-item" 
                   onclick="return confirm('This will permanently delete ALL data! Continue?')">
                    <i class="fas fa-trash"></i> Purge All Data
                </a>
            </div>
        </div>
    </body>
    </html>
    )rawliteral";

    // Dynamic content replacement
    html.replace("%WIFI_STATUS%", wifiStatus);
    html.replace("%HEAP%", String(ESP.getFreeHeap()));

    server.send(200, "text/html", html);
}
//15

void handleWifiSettings() {
    if (!authenticateUser()) {
        return;
    }

    // Perform Wi-Fi scan
    int networkCount = WiFi.scanNetworks();
    String options = "";

    if (networkCount == 0) {
        options = "<option value='' disabled>No networks found</option>";
    } else {
        for (int i = 0; i < networkCount; i++) {
            String ssid = WiFi.SSID(i);
            options += "<option value='" + ssid + "'>" + ssid + "</option>";
        }
    }

    // HTML page for Wi-Fi settings
    String htmlPage = R"rawliteral(
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Change Wi-Fi Settings</title>
        <style>
            body {
                font-family: 'Arial', sans-serif;
                margin: 0;
                padding: 0;
                background: linear-gradient(to right, #74ebd5, #acb6e5);
                display: flex;
                justify-content: center;
                align-items: center;
                height: 100vh;
            }
            .wifi-settings-container {
                background: white;
                padding: 30px;
                border-radius: 16px;
                box-shadow: 0 8px 40px rgba(0, 0, 0, 0.2);
                width: 90%;
                max-width: 500px;
                text-align: center;
            }
            .wifi-settings-container h2 {
                font-size: 28px;
                color: #2c3e50;
                margin-bottom: 20px;
            }
            .wifi-settings-container label {
                display: block;
                font-size: 16px;
                color: #555;
                text-align: left;
                margin-bottom: 8px;
            }
            .wifi-settings-container select, .wifi-settings-container input[type="text"], .wifi-settings-container input[type="password"] {
                width: 100%;
                padding: 10px;
                margin-bottom: 20px;
                border: 1px solid #ddd;
                border-radius: 8px;
                box-shadow: inset 0 2px 4px rgba(0, 0, 0, 0.1);
                font-size: 16px;
            }
            .wifi-settings-container select:focus, .wifi-settings-container input[type="text"]:focus, .wifi-settings-container input[type="password"]:focus {
                outline: none;
                border-color: #4caf50;
                box-shadow: 0 0 8px rgba(76, 175, 80, 0.3);
            }
            .wifi-settings-container input[type="submit"] {
                background-color: #4caf50;
                color: white;
                padding: 12px 20px;
                font-size: 18px;
                border: none;
                border-radius: 8px;
                cursor: pointer;
                transition: all 0.3s ease;
                width: 100%;
            }
            .wifi-settings-container input[type="submit"]:hover {
                background-color: #45a049;
                box-shadow: 0 4px 10px rgba(0, 0, 0, 0.1);
                transform: scale(1.05);
            }
            .status-message {
                font-size: 14px;
                color: #888;
                margin-top: 10px;
            }
        </style>
    </head>
    <body>
        <div class="wifi-settings-container">
            <h2>Change Wi-Fi Settings</h2>
            <form action="/saveWifi" method="POST">
                <label for="ssid">Select SSID:</label>
                <select id="ssid" name="ssid">
                    <option value="">Select a network</option>
                    )rawliteral" + options + R"rawliteral(
                </select>
                <label for="password">Password:</label>
                <input type="password" id="password" name="password" placeholder="Enter Wi-Fi Password">
                <input type="submit" value="Save Wi-Fi Settings">
            </form>
            <p class="status-message">Please select a Wi-Fi network and enter the password.</p>
        </div>
    </body>
    </html>
    )rawliteral";

    // Send the HTML page to the client
    server.send(200, "text/html", htmlPage);
}

//16

void handleSaveWifi() {
    if (!server.hasArg("ssid") || !server.hasArg("password")) {
        // If SSID or password is missing, return a detailed error message
        server.send(400, "text/html", "<h2>Error: Missing SSID or password.</h2>");
        return;
    }

    // Get SSID and password from the HTTP request
    String ssid = server.arg("ssid");
    String password = server.arg("password");

    // Validate SSID and password length
    if (ssid.length() == 0 || password.length() == 0) {
        server.send(400, "text/html", "<h2>Error: SSID or password cannot be empty.</h2>");
        return;
    }

    // Save SSID and password to preferences
    preferences.begin("WiFi", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();

    // Log the saving of credentials for debugging purposes
    Serial.println("WiFi credentials saved:");
    Serial.print("SSID: ");
    Serial.println(ssid);
    Serial.print("Password: ");
    Serial.println(password);

    // Send a successful response and redirect the user
    server.sendHeader("Location", "https://rifaaateee.github.io/Attendance-System/");
    server.send(303);  // 303 See Other

    // Allow the user to see the redirection for 2 seconds
    delay(2000);  // Wait for 2 seconds to let the redirect be processed

    // Restart the ESP32 to apply the new settings
    Serial.println("Restarting ESP32 to apply WiFi settings...");
    ESP.restart();
}

//17
// Teacher enrollment page
void handleEnrollTeacher() {
  if (!authenticateUser()) {
    return;
  }
  if (WiFi.status() != WL_CONNECTED) {
        server.send(400, "text/html", "<h2>Error: Cannot enroll teacher while offline.</h2>");
        return;
    }

  String htmlPage = R"rawliteral(
    <!DOCTYPE html>
    <html lang="en">
    <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>Teacher Enrollment</title>
      <style>
        body {
          font-family: Arial, sans-serif;
          background-color: #f4f4f9;
          margin: 0;
          padding: 20px;
        }
        h2 {
          text-align: center;
          color: #333;
        }
        form {
          margin: 0 auto;
          max-width: 400px;
          padding: 20px;
          background-color: white;
          border: 1px solid #ddd;
          border-radius: 8px;
        }
        label {
          font-weight: bold;
        }
        input[type="text"] {
          width: 100%;
          padding: 8px;
          margin: 10px 0 20px 0;
          border: 1px solid #ccc;
          border-radius: 4px;
        }
        input[type="submit"] {
          background-color: #4CAF50;
          color: white;
          border: none;
          padding: 10px 20px;
          cursor: pointer;
          border-radius: 4px;
          font-size: 16px;
        }
        input[type="submit"]:hover {
          background-color: #45a049;
        }
      </style>
    </head>
    <body>
      <h2>Teacher Enrollment</h2>
      <form action="/enrollTeacherSubmit" method="POST">
        <label for="teacherId">Teacher ID (SL):</label><br>
        <input type="text" id="teacherId" name="teacherId" required><br><br>
        
        <label for="teacherName">Teacher Name:</label><br>
        <input type="text" id="teacherName" name="teacherName" required><br><br>
        
        <label for="courseName">Course Name:</label><br>
        <input type="text" id="courseName" name="courseName" required><br><br>
        
        <label for="coursecode">Course Code:</label><br>
        <input type="text" id="coursecode" name="coursecode" required><br><br>
        
        <input type="submit" value="Enroll Teacher">
      </form>
    </body>
    </html>
  )rawliteral";

  server.send(200, "text/html", htmlPage);
}

//18

// Enroll a teacher and take attendance
void enrollTeacher(int teacherId, const String& teacherName, 
                  const String& courseName, const String& coursecode) {
    lcd.clear();
    lcd.print("Enroll Teacher");
    lcd.setCursor(0, 1);
    lcd.print("ID: " + String(teacherId));
    delay(2000);

    // Step 1: Initial Finger Placement
    lcd.clear();
    lcd.print("Step 1/2");
    lcd.setCursor(0, 1);
    lcd.print("Place finger");
    while(finger.getImage() != FINGERPRINT_OK) {
        delay(100);
    }
    
    finger.image2Tz(1);
    lcd.clear();
    lcd.print("Scan 1/2 OK!");
    lcd.setCursor(0, 1);
    lcd.print("Remove finger");
    delay(2000);

    // Wait for finger removal
    while(finger.getImage() == FINGERPRINT_OK) {
        lcd.clear();
        lcd.print("Please remove");
        lcd.setCursor(0, 1);
        lcd.print("your finger");
        delay(500);
    }

    // Step 2: Second Finger Placement
    lcd.clear();
    lcd.print("Step 2/2");
    lcd.setCursor(0, 1);
    lcd.print("Same finger");
    delay(1000);
    
    int retry = 0;
    while(retry < 3) {
        lcd.clear();
        lcd.print("Place finger");
        lcd.setCursor(0, 1);
        lcd.print("again");
        
        if(finger.getImage() == FINGERPRINT_OK) {
            finger.image2Tz(2);
            break;
        }
        retry++;
        delay(1000);
    }

    if(retry >= 3) {
        lcd.clear();
        lcd.print("Enrollment");
        lcd.setCursor(0, 1);
        lcd.print("Failed! Retry");
        return;
    }

    // Final Processing
    lcd.clear();
    lcd.print("Processing...");
    finger.createModel();
    finger.storeModel(teacherId);
    
    lcd.clear();
    lcd.print("Enrollment");
    lcd.setCursor(0, 1);
    lcd.print("Successful!");
    delay(2000);

    saveTeacherData(teacherId, teacherName, courseName, coursecode);
}

//19
void saveTeacherData(int teacherId, const String& teacherName, const String& courseName, const String& coursecode) {
    String dataLine = String(teacherId) + "," + teacherName + "," + courseName + "," + coursecode + "\n";

    // Save to local storage (SPIFFS)
    File file = SPIFFS.open(teacherDataFile, FILE_APPEND);
    if (!file) {
        Serial.println("Error: Failed to open teacher data file in SPIFFS for appending.");
        return;
    }
    file.print(dataLine);
    file.close();
    Serial.println("Teacher data saved to SPIFFS.");

    // Save to SD card
    File sdFile = SD.open(teacherDataFile, FILE_APPEND);
    if (sdFile) {
        sdFile.print(dataLine);
        sdFile.close();
        Serial.println("Teacher data saved to SD card.");
    } else {
        Serial.println("Error: Failed to open teacher data file on SD card!");
    }

    // Prepare JSON for Supabase
    StaticJsonDocument<300> doc;
    doc["teacher_id"] = teacherId;
    doc["name"] = teacherName;
    doc["course_name"] = courseName;
    doc["course_code"] = coursecode;

    String jsonString;
    serializeJson(doc, jsonString);

    // Send to Supabase if connected to WiFi
    if (WiFi.status() == WL_CONNECTED) {
        if (sendToSupabase("teachers", jsonString, "POST", -1)) {
            Serial.println("Teacher data sent to Supabase successfully.");
        } else {
            Serial.println("Error: Failed to send teacher data to Supabase.");
        }
    } else {
        Serial.println("Error: Not connected to WiFi. Teacher data not sent to Supabase.");
    }
}

//20

// Serve the teacher data file for download
void handleDownloadTeachers() {
  if (!authenticateUser()) {
    server.send(403, "text/html", "<h2>Authentication Failed. Access Denied!</h2>");
    return;
  }

  File file = SPIFFS.open(teacherDataFile, FILE_READ);
  if (!file) {
    String errorMessage = "<h2>Failed to open teacher data file. Please try again later.</h2>";
    server.send(500, "text/html", errorMessage);
    return;
  }

  // Check if file is empty
  if (file.size() == 0) {
    String noDataMessage = "<h2>No teacher data available. The file is empty.</h2>";
    server.send(200, "text/html", noDataMessage);
    file.close();
    return;
  }

  // Set content type and stream the file
  server.streamFile(file, "text/csv");
  file.close();
}

//21

// Student enrollment page
void handleEnrollStudent() {
  if (!authenticateUser()) {
    server.send(403, "text/html", "<h2>Authentication Failed. Access Denied!</h2>");
    return;
  }
  if (WiFi.status() != WL_CONNECTED) {
        server.send(400, "text/html", "<h2>Error: Cannot enroll student while offline.</h2>");
        return;
    }
  String htmlPage = R"rawliteral(
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Student Enrollment</title>
        <style>
            body {
                font-family: Arial, sans-serif;
                background-color: #f4f4f9;
                margin: 0;
                padding: 20px;
            }
            h2 {
                color: #333;
            }
            label {
                font-weight: bold;
            }
            input[type="text"], input[type="submit"] {
                width: 100%;
                padding: 10px;
                margin: 8px 0;
                border: 1px solid #ccc;
                border-radius: 5px;
                box-sizing: border-box;
            }
            input[type="submit"] {
                background-color: #4CAF50;
                color: white;
                border: none;
            }
            input[type="submit"]:hover {
                background-color: #45a049;
            }
        </style>
    </head>
    <body>
        <h2>Student Enrollment</h2>
        <p>Please enter the details of the student to enroll:</p>
        <form action="/enrollStudentSubmit" method="POST">
          <label for="studentId">Student ID (SL):</label><br>
          <input type="text" id="studentId" name="studentId" required><br><br>
          
          <label for="studentName">Student Name:</label><br>
          <input type="text" id="studentName" name="studentName" required><br><br>
          
          <label for="stdroll">Student Roll Number:</label><br>
          <input type="text" id="stdroll" name="stdroll" required><br><br>
          
          <input type="submit" value="Enroll Student">
        </form>

        <br>
        <a href="/">Go back to Home</a>
    </body>
    </html>
  )rawliteral";

  server.send(200, "text/html", htmlPage);
}

//22


void enrollStudent(int studentId, const String& studentName, const String& stdroll) {
    lcd.clear();
    lcd.print("Enroll Student");
    lcd.setCursor(0, 1);
    lcd.print("Roll: " + stdroll);
    delay(2000);

    // Step 1: Initial Scan
    lcd.clear();
    lcd.print("First Scan");
    lcd.setCursor(0, 1);
    lcd.print("Place finger");
    
    unsigned long start = millis();
    while(finger.getImage() != FINGERPRINT_OK) {
        if(millis() - start > 10000) {
            lcd.clear();
            lcd.print("Timeout!");
            lcd.setCursor(0, 1);
            lcd.print("Restarting...");
            delay(2000);
            return;
        }
    }
    
    finger.image2Tz(1);
    lcd.clear();
    lcd.print("Scan 1 OK");
    lcd.setCursor(0, 1);
    lcd.print("Remove finger");
    delay(2000);

    // Clearance Check
    while(finger.getImage() == FINGERPRINT_OK) {
        lcd.clear();
        lcd.print("Finger still");
        lcd.setCursor(0, 1);
        lcd.print("detected!");
        delay(500);
    }

    // Step 2: Verification Scan
    lcd.clear();
    lcd.print("Second Scan");
    lcd.setCursor(0, 1);
    lcd.print("Same finger");
    
    bool success = false;
    for(int attempt=1; attempt<=3; attempt++) {
        lcd.clear();
        lcd.print("Attempt " + String(attempt));
        lcd.setCursor(0, 1);
        lcd.print("Place finger");
        
        if(finger.getImage() == FINGERPRINT_OK) {
            finger.image2Tz(2);
            success = true;
            break;
        }
        delay(1000);
    }

    if(!success) {
        lcd.clear();
        lcd.print("Too many");
        lcd.setCursor(0, 1);
        lcd.print("failed attempts");
        delay(2000);
        return;
    }

    // Finalization
    lcd.clear();
    lcd.print("Creating");
    lcd.setCursor(0, 1);
    lcd.print("fingerprint...");
    
    if(finger.createModel() != FINGERPRINT_OK) {
        lcd.clear();
        lcd.print("Error: Models");
        lcd.setCursor(0, 1);
        lcd.print("don't match!");
        return;
    }

    finger.storeModel(studentId);
    lcd.clear();
    lcd.print("Student Added!");
    lcd.setCursor(0, 1);
    lcd.print("Roll: " + stdroll);
    delay(2000);

    saveStudentData(studentId, studentName, stdroll);
}

//23

void saveStudentData(int studentId, const String& studentName, const String& stdroll) {
    String dataLine = String(studentId) + "," + studentName + "," + stdroll + "\n";

    // Save to SPIFFS
    File file = SPIFFS.open(studentDataFile, FILE_APPEND);
    if (!file) {
        Serial.println("Failed to open student data file for appending in SPIFFS.");
    } else {
        file.print(dataLine);
        file.close();
    }

    // Save to SD Card
    File sdFile = SD.open(studentDataFile, FILE_APPEND);
    if (!sdFile) {
        Serial.println("Failed to open student data file for appending on SD card.");
    } else {
        sdFile.print(dataLine);
        sdFile.close();
    }

    // Prepare JSON for Supabase
    StaticJsonDocument<200> doc;
    doc["student_id"] = studentId;
    doc["name"] = studentName;
    doc["roll_number"] = stdroll;

    String jsonString;
    serializeJson(doc, jsonString);

    // Send to Supabase if connected to WiFi
    if (WiFi.status() == WL_CONNECTED) {
       sendToSupabase("students", jsonString, "POST", -1);
    }
}


//24
void handleDownloadStudents() {
    if (!authenticateUser()) {
        // If authentication fails, send a 401 Unauthorized response
        server.send(401, "text/html", "<h2>Unauthorized Access. Please log in.</h2>");
        return;
    }

    // Try to open the student data file
    File file = SPIFFS.open(studentDataFile, FILE_READ);
    if (!file) {
        // Send a detailed error message if the file fails to open
        server.send(500, "text/html", "<h2>Failed to open student data file. Please try again later.</h2>");
        return;
    }

    // Send the file stream with the correct MIME type (CSV file)
    server.streamFile(file, "text/csv");

    // Close the file after streaming
    file.close();
}

//25

void handleAttendanceMode() {
    if (!authenticateUser()) {
        // Return if authentication fails
        return;
    }

    // Construct the HTML page for starting the attendance mode
    String htmlPage = R"rawliteral(
        <!DOCTYPE html>
        <html lang="en">
        <head>
            <meta charset="UTF-8">
            <meta name="viewport" content="width=device-width, initial-scale=1.0">
            <meta http-equiv="refresh" content="3;url=/attendanceMode"> <!-- Redirect after 3 seconds -->
            <title>Starting Attendance Mode</title>
            <style>
                body {
                    font-family: Arial, sans-serif;
                    text-align: center;
                    margin-top: 50px;
                }
                h2 {
                    color: #4CAF50;
                }
                .message {
                    font-size: 18px;
                }
            </style>
        </head>
        <body>
            <h2>Attendance Mode Starting...</h2>
            <p class="message">Redirecting to attendance mode...</p>
            <script>
                // Optional: Provide immediate redirection in case the meta refresh doesn't work
                setTimeout(function() {
                    window.location.href = '/attendanceMode';
                }, 3000); // 3 seconds delay
            </script>
        </body>
        </html>
    )rawliteral";

    // Send the HTML response to the client
    server.send(200, "text/html", htmlPage);
}

//26


bool isTeacherFingerprintValid(int teacherId, const String& coursecode) {
    // Check if the teacher data file exists
    if (!SPIFFS.exists(teacherDataFile)) {
        Serial.println("Teacher data file does not exist!");
        return false;  // Return false if the file does not exist
    }

    // Open the teacher CSV file for reading
    File file = SPIFFS.open(teacherDataFile, "r");
    if (!file) {
        Serial.println("Failed to open teacher data file!");
        return false;  // Return false if the file cannot be opened
    }

    // Read each line from the teacher data file
    while (file.available()) {
        String line = file.readStringUntil('\n');
        line.trim();  // Trim any extra whitespace
        if (line.length() == 0) continue;  // Skip empty lines

        // Parse the CSV line into components
        int comma1 = line.indexOf(',');
        int comma2 = line.lastIndexOf(',');

        // Ensure there are two commas (for ID and course code), and they are not the same
        if (comma1 == -1 || comma2 == -1 || comma1 == comma2) {
            continue;  // Skip malformed lines
        }

        String idStr = line.substring(0, comma1);
        String courseCodeStr = line.substring(comma2 + 1);

        // Validate teacher ID and course code
        if (idStr.toInt() == teacherId && courseCodeStr.equalsIgnoreCase(coursecode)) {
            file.close();  // Close file before returning
            Serial.println("Teacher fingerprint verified for course: " + coursecode);
            return true;  // Valid teacher ID and course code found
        }
    }

    file.close();  // Ensure file is closed after reading
    Serial.println("Teacher ID and course code do not match.");
    return false;  // No match found for teacher ID and course code
}



//27




int getFingerprintID() {
    // Capture the fingerprint image
    uint8_t p = finger.getImage();
    if (p == FINGERPRINT_NOFINGER) {
        // Inform the user that no finger was detected and wait for input
        Serial.println("No finger detected. Please place your finger.");
        return -1;  // Return early if no finger is detected
    }

    if (p != FINGERPRINT_OK) {
        // Handle other capture errors and provide feedback
        Serial.println("Error capturing fingerprint image. Error code: " + String(p));
        return -1;  // Return early for other capture errors
    }

    // Convert the captured image to a template
    p = finger.image2Tz();
    if (p != FINGERPRINT_OK) {
        // Inform the user about the conversion failure
        Serial.println("Error converting fingerprint image. Error code: " + String(p));
        return -1;
    }

    // Perform a fast search for the fingerprint in the database
    p = finger.fingerFastSearch();
    if (p != FINGERPRINT_OK) {
        // If no match is found, inform the user and return an error
        Serial.println("No matching fingerprint found. Error code: " + String(p));
        return -1;
    }

    // Return the ID of the matched fingerprint
    Serial.println("Fingerprint matched successfully! ID: " + String(finger.fingerID));
    return finger.fingerID;
}




//28
void clearSensorBuffer() {
    unsigned long startTime = millis();  // Track the start time to avoid infinite waiting
    unsigned long timeout = 5000; // Timeout after 5 seconds

    while (finger.getImage() != FINGERPRINT_NOFINGER) {
        delay(100); // Wait for sensor to report no finger

        // Check if the timeout has been exceeded
        if (millis() - startTime > timeout) {
            Serial.println("Error: Fingerprint sensor did not clear in time.");
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Sensor Timeout");
            delay(2000); // Show the error message on LCD for 2 seconds
            return; // Exit the function if timeout occurs
        }
    }

    // If the sensor cleared successfully, notify the user
    Serial.println("Sensor buffer cleared successfully.");
}


//29

bool verifyTeacherFingerprint(const String &coursecode) {
    int teacherId;
    clearSensorBuffer();  // Ensure the sensor buffer is clear before starting

    while (true) {
        teacherId = getFingerprintID();  // Fetch fingerprint ID

        // No finger detected, prompt the user to try again
        if (teacherId == -1) {
            Serial.println("No fingerprint detected. Please place your finger.");
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("No finger found");
            lcd.setCursor(0, 1);
            lcd.print("Try again...");
            delay(2000);
            continue;  // Keep waiting until a fingerprint is detected
        }

        // Check if the fingerprint matches the teacher ID and course code
        if (isTeacherFingerprintValid(teacherId, coursecode)) {
            Serial.println("Teacher fingerprint verified successfully.");
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Verification");
            lcd.setCursor(0, 1);
            lcd.print("Success!");
            delay(2000);
            return true;  // Verification successful
        } else {
            Serial.println("Fingerprint detected but verification failed.");
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Verification");
            lcd.setCursor(0, 1);
            lcd.print("Failed!");
            delay(2000);
            return false;  // Verification failed
        }
    }
}


//30

int processStudentAttendance(const String &coursecode) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Scan Student Finger");
    Serial.println("Awaiting student fingerprint...");

    while (true) {
        int result = finger.getImage();
        if (result == FINGERPRINT_NOFINGER) {
            delay(100);
            continue;
        }

        if (result != FINGERPRINT_OK) {
            Serial.println("Error capturing fingerprint image.");
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Error Reading");
            lcd.setCursor(0, 1);
            lcd.print("Try Again");
            delay(2000);
            return -2;
        }

        result = finger.image2Tz();
        if (result != FINGERPRINT_OK) {
            Serial.println("Error converting fingerprint image.");
            return -2;
        }

        result = finger.fingerFastSearch();
        if (result != FINGERPRINT_OK) {
            Serial.println("Fingerprint not recognized!");
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Invalid Finger");
            delay(2000);
            return -1;
        }

        int studentId = finger.fingerID;

        // Check if student exists in the database
        String studentName = fetchStudentName(studentId);
        if (studentName.isEmpty() || studentName == "Unknown Student") {
            Serial.println("Fingerprint matched but student not in database!");
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Unknown Student");
            lcd.setCursor(0, 1);
            lcd.print("Not Recorded");
            delay(2000);
            return -4; // New error code for unknown student
        }

        if (studentId < 0 || studentId >= MAX_STUDENTS) {
            Serial.println("Invalid student ID.");
            return -1;
        }

        if (attendedStudentsarray[studentId]) {
            Serial.println("Duplicate attendance detected.");
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Already Marked");
            lcd.setCursor(0, 1);
            lcd.print("Attendance");
            delay(2000);
            return -3; // Error code for duplicate attendance
        }

        attendedStudentsarray[studentId] = true;
        Serial.println("Fingerprint recognized. ID: " + String(studentId));
        
        // Once everything is processed, return studentId
        return studentId;
    }
}





//31
String fetchStudentName(int studentId) {
    // Open the student file in read mode
    File studentFile = SPIFFS.open("/students.csv", FILE_READ);
    if (!studentFile) {
        Serial.println("Failed to open students.csv file!");
        return "";
    }

    // Loop through the file line by line
    while (studentFile.available()) {
        String line = studentFile.readStringUntil('\n');
        
        // Find the first comma (student ID) and last comma (name end)
        int idIndex = line.indexOf(',');
        int nameIndex = line.lastIndexOf(',');

        if (idIndex == -1 || nameIndex == -1 || idIndex == nameIndex) {
            continue;  // Skip malformed lines
        }

        String id = line.substring(0, idIndex); // Extract student ID
        String name = line.substring(idIndex + 1, nameIndex); // Extract student name

        // Compare the extracted ID with the studentId
        if (id.toInt() == studentId) {
            studentFile.close();
            return name;  // Return the name if match is found
        }
    }

    // If student ID is not found
    studentFile.close();
    return "Unknown Student";  // Return a default name if not found
}



// void attendanceMode() {
//     if (!initializeFingerprint()) {
//         lcd.clear();
//         lcd.setCursor(0, 0);
//         lcd.print("Sensor Error!");
//         lcd.setCursor(0, 1);
//         lcd.print("Try Again Later");
//         delay(3000);
//         return;
//     }
//     lcd.clear();
//     lcd.setCursor(0, 0);
//     lcd.print("Attendance Mode");
//     lcd.setCursor(0, 1);
//     lcd.print("Starting...");
//     Serial.println("Attendance mode started.");

//     String coursecode = "";

//     // Reset the attendance tracking array
//     memset(attendedStudentsarray, false, sizeof(attendedStudentsarray));

//     // Prompt for course code
//     lcd.clear();
//     lcd.setCursor(0, 0);
//     lcd.print("Enter Course Code:");
//     Serial.println("Awaiting course code...");

//     while (true) {
//         char key = keypad.getKey();
//         if (key) {
//             if (key == '#') {  // End of input
//                 Serial.println("Course code entered: " + coursecode);
//                 break;
//             } else {
//                 coursecode += key;
//                 lcd.print(key);
//                 Serial.print(key);
//             }
//         }
//     }

//     // Verify teacher fingerprint
//     lcd.clear();
//     lcd.setCursor(0, 0);
//     lcd.print("Verify Teacher...");
//     Serial.println("Place teacher's finger on the sensor.");

//     if (!verifyTeacherFingerprint(coursecode)) {
//         lcd.clear();
//         lcd.setCursor(0, 0);
//         lcd.print("Verification Failed");
//         Serial.println("Teacher fingerprint verification failed.");
//         delay(3000);
//         return;
//     }

//     lcd.clear();
//     lcd.setCursor(0, 0);
//     lcd.print("Attendance Start");
//     Serial.println("Attendance started for course: " + coursecode);

//     while (true) {
//         lcd.clear();
//         lcd.setCursor(0, 0);
//         lcd.print("Awaiting Student");

//         int studentId = processStudentAttendance(coursecode);
           
//         if (studentId > 0) {
//             String studentName = fetchStudentName(studentId);
//             // Double check to ensure student exists in database
//             if (!studentName.isEmpty() && studentName != "Unknown Student") {
//                 saveAttendance(coursecode, studentName, studentId);
//                 lcd.clear();
//                 lcd.setCursor(0, 0);
//                 lcd.print("Marked for:");
//                 lcd.setCursor(0, 1);
//                 lcd.print(studentName);
//                 Serial.println("Attendance recorded for: " + studentName);
//                 delay(2000);
//             }
//         } else if (studentId == -1) {
//             Serial.println("Invalid fingerprint. Attendance not recorded.");
//         } else if (studentId == -3) {
//             Serial.println("Duplicate attendance. Skipping.");
//         } else if (studentId == -4) {
//             Serial.println("Unknown student. Attendance not recorded.");
//         }

//         // Allow teacher to end attendance session
//         if (verifyTeacherFingerprint(coursecode)) {
//             lcd.clear();
//             lcd.setCursor(0, 0);
//             lcd.print("Attendance Ended");
//             Serial.println("Attendance session ended.");
//             delay(3000);
//             break;
//         }
//     }
// }

//33
void saveAttendance(String coursecode, String studentName, int studentId) {
    DateTime now = rtc.now();
    char timestamp[32];
    sprintf(timestamp, "%04d-%02d-%02d", now.year(), now.month(), now.day());

    // Save to SD Card
    File sdAttendanceFile = SD.open("/attendance.csv", FILE_APPEND);
    if (!sdAttendanceFile) {
        Serial.println("Failed to open attendance file on SD card!");
    } else {
        String record = coursecode + "," + studentName + "," + String(studentId) + "," + String(timestamp) + "\n";
        sdAttendanceFile.print(record);
        sdAttendanceFile.close();
        Serial.println("Attendance saved to SD card: " + record);
    }

    // Save locally
    File attendanceFile = SPIFFS.open("/attendance.csv", FILE_APPEND);
    if (!attendanceFile) {
        Serial.println("Failed to open attendance file!");
        return;
    }

    String record = coursecode + "," + studentName + "," + String(studentId) + "," + String(timestamp) + "\n";
    attendanceFile.print(record);
    attendanceFile.close();
    Serial.println("Attendance saved: " + record);

    // --- MODIFICATION: Only save offline if not connected ---
    if (WiFi.status() != WL_CONNECTED) {
        saveOfflineAttendance(coursecode, studentName, studentId);
    }

    // Prepare JSON for Supabase
    StaticJsonDocument<300> doc;
    doc["course_code"] = coursecode;
    doc["student_name"] = studentName;
    doc["student_id"] = studentId;
    doc["timestamp"] = timestamp;

    String jsonString;
    serializeJson(doc, jsonString);

    // Send to Supabase
    if (WiFi.status() == WL_CONNECTED) {
        sendToSupabase("attendance", jsonString, "POST", -1);
    }
}

//34


//36



//37



//38




// 39
void deleteAllCSVFiles() {
    struct FileDetail {
        const char* filePath;
        const char* description;
    };

    // List of files to delete
    FileDetail files[] = {
        {teacherDataFile, "teachers.csv"},
        {studentDataFile, "students.csv"},
        {"/attendance.csv", "attendance.csv"}
    };

    for (const auto& file : files) {
        if (SPIFFS.exists(file.filePath)) {
            if (SPIFFS.remove(file.filePath)) {
                Serial.printf("%s deleted successfully.\n", file.description);
            } else {
                Serial.printf("Failed to delete %s.\n", file.description);
            }
        } else {
            Serial.printf("%s does not exist.\n", file.description);
        }
    }
}


//40
void handleDeleteAllCSVFiles() {
  if (!authenticateUser()) {
    return;
  }
  
  deleteAllCSVFiles();
  server.send(200, "text/html", "<h2>All CSV files deleted successfully!</h2>");
}


//41
struct AttendanceRecord {
    int studentId = -1; // Default invalid ID
    String studentName = ""; // Default empty name
    bool isValid = false; // Default not valid
};


//42 Array to store temporary attendance records
const int MAX_TEMP_RECORDS = 100;
AttendanceRecord tempAttendance[MAX_TEMP_RECORDS];
int tempAttendanceCount = 0;
//43
void startAttendanceProcess() {
    if (!initializeFingerprint()) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Sensor Error!");
        lcd.setCursor(0, 1);
        lcd.print("Try Again Later");
        delay(3000);
        isAttendanceMode = false;
        return;
    }

    // Reset temporary attendance storage
    tempAttendanceCount = 0;
    memset(attendedStudentsarray, false, sizeof(attendedStudentsarray));

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Attendance Mode");
    lcd.setCursor(0, 1);
    lcd.print("Starting...");
    Serial.println("Attendance mode started.");

    String coursecode = "";

    // Prompt for course code
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Enter Course Code:");
    Serial.println("Awaiting course code...");

    while (isAttendanceMode) {
        char key = keypad.getKey();
        if (key) {
            if (key == 'B') { // Cancel attendance mode
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Session Cancelled");
                lcd.setCursor(0, 1);
                lcd.print("No Records Saved");
                Serial.println("Attendance mode cancelled.");
                delay(2000);
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Ready");
                isAttendanceMode = false;
                tempAttendanceCount = 0; // Reset temporary storage
                return;
            } else if (key == '*') { // Backspace functionality
                if (coursecode.length() > 0) {
                    coursecode = coursecode.substring(0, coursecode.length() - 1);
                    lcd.clear();
                    lcd.setCursor(0, 0);
                    lcd.print("Enter Course Code:");
                    lcd.setCursor(0, 1);
                    lcd.print(coursecode);
                }
            } else if (key == '#') { // End of input
                if (coursecode.length() > 0) {
                    Serial.println("Course code entered: " + coursecode);
                    break;
                }
            } else { // Normal number input
                coursecode += key;
                lcd.setCursor(0, 1);
                lcd.print(coursecode);
                Serial.print(key);
            }
        }
    }

    if (!isAttendanceMode) return;

    // Verify initial teacher fingerprint
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Verify Teacher...");
    Serial.println("Place teacher's finger on the sensor.");

    while (true) {
        if (verifyTeacherFingerprint(coursecode)) {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Verification Success");
            Serial.println("Teacher fingerprint verified successfully.");
            delay(2000);
            break;
        } else {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Verification Failed");
            lcd.setCursor(0, 1);
            lcd.print("Try Again");
            Serial.println("Teacher fingerprint verification failed. Waiting for correct fingerprint...");
            delay(2000);
        }
    }

    // Main attendance collection loop
    while (isAttendanceMode) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Place Finger");
        lcd.setCursor(0, 1);
        lcd.print("Or End Session");

        int studentId = getFingerprintID();

        // Check if it's the teacher's fingerprint
        if (studentId > 0 && isTeacherFingerprintValid(studentId, coursecode)) {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Saving Records...");

            for (int i = 0; i < tempAttendanceCount; i++) {
                if (tempAttendance[i].isValid) {
                    saveAttendance(coursecode, tempAttendance[i].studentName, tempAttendance[i].studentId);
                }
            }

            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Session Ended");
            lcd.setCursor(0, 1);
            lcd.print("Records: ");
            lcd.print(tempAttendanceCount);
            delay(3000);
            isAttendanceMode = false;
            break;
        }

        // Process student fingerprint
        if (studentId > 0) {
            String studentName = fetchStudentName(studentId);
            if (!studentName.isEmpty() && studentName != "Unknown Student" && !attendedStudentsarray[studentId]) {
                attendedStudentsarray[studentId] = true;

                if (tempAttendanceCount < MAX_TEMP_RECORDS) {
                    tempAttendance[tempAttendanceCount].studentId = studentId;
                    tempAttendance[tempAttendanceCount].studentName = studentName;
                    tempAttendance[tempAttendanceCount].isValid = true;
                    tempAttendanceCount++;

                    lcd.clear();
                    lcd.setCursor(0, 0);
                    lcd.print("Recorded:");
                    lcd.setCursor(0, 1);
                    lcd.print(studentName);
                    delay(1500);
                }
            } else {
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Unknown Student");
                lcd.setCursor(0, 1);
                lcd.print("Not Recorded");
                delay(2000);
            }
        }

        // Check for cancel key
        char key = keypad.getKey();
        if (key == 'B') {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Session Cancelled");
            lcd.setCursor(0, 1);
            lcd.print("No Records Saved");
            Serial.println("Attendance mode cancelled.");
            delay(2000);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Ready");
            isAttendanceMode = false;
            tempAttendanceCount = 0; // Reset temporary storage
            return;
        }

        delay(100); // Small delay to prevent too rapid scanning
    }
}


//44
void cancelAttendanceProcess() {
    // Clear the LCD and display cancellation message
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Session Cancelled");
    lcd.setCursor(0, 1);
    lcd.print("No Records Saved");

    // Log to Serial Monitor
    Serial.println("Attendance mode cancelled.");

    // Pause for a brief moment before resetting
    delay(2000);

    // Reset LCD and status
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Ready");

    // Reset relevant variables to their initial state
    isAttendanceMode = false;
    tempAttendanceCount = 0;  // Reset temporary storage for attendance
}

//45



//46


//47


//48




// Usage Example: auto parsed = parseCSVLine(line, {0, 1});




//49



//50
void handleDownloadAttendanceByDate() {
   if (!authenticateUser()) {
       server.send(401, "text/plain", "Unauthorized access");
       return;
   }

   // Check if required parameters are present in the URL
   String dateRequested = server.arg("date");
   String courseCode = server.arg("courseCode");

   if (dateRequested.isEmpty() || courseCode.isEmpty()) {
       server.send(400, "text/plain", "Missing 'courseCode' or 'date' parameters");
       return;
   }

   // Build the file name based on the course and date, if necessary
   String fileName = "/attendance_" + courseCode + "_" + dateRequested + ".csv";

   File fileToDownload = SPIFFS.open(fileName, "r");
   if (!fileToDownload) {
       server.send(404, "text/plain", "Attendance file not found for the requested date and course.");
       return;
   }

   // Add Content-Disposition for proper download behavior
   server.sendHeader("Content-Disposition", "attachment; filename=" + fileName);
   
   // Stream the file content to the client
   server.streamFile(fileToDownload, "text/csv");

   fileToDownload.close();
}



//51
// void handleDownloadAttendance25() {
//     if (!authenticateUser()) {
//         server.send(401, "text/plain", "Unauthorized access");
//         return;
//     }

//     File file = SPIFFS.open("/attendance.csv", "r");
//     if (!file) {
//         server.send(500, "text/plain", "Failed to open attendance file.");
//         return;
//     }

//     // Adding Content-Disposition for proper download behavior
//     server.sendHeader("Content-Disposition", "attachment; filename=attendance.csv");

//     // Stream the file content to the client
//     server.streamFile(file, "text/csv");

//     file.close();
// }






//52



//53
void saveOfflineAttendance(String courseCode, String studentName, int studentId) {
    DateTime now = rtc.now();
    char dateStr[11];
    sprintf(dateStr, "%04d-%02d-%02d", now.year(), now.month(), now.day());

    String record = 
        courseCode + "," + 
        studentName + "," + 
        String(studentId) + "," + 
        dateStr + "\n";

    // Save to offline file
    File file = SPIFFS.open("/offline_attendance.csv", FILE_APPEND);
    if (file) {
        file.print(record);
        file.close();
        Serial.println("Saved to offline file: " + record);
        offlineDataSent = false; // Reset flag for new data
    } else {
        Serial.println("Failed to save offline attendance!");
    }

    // Optional: Save to SD card as backup
    File sdFile = SD.open("/offline_attendance.csv", FILE_APPEND);
    if (sdFile) {
        sdFile.print(record);
        sdFile.close();
    }
}

//54
void sendOfflineDataToSupabase() {
    // Check if the device is connected to Wi-Fi
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Wi-Fi not connected. Skipping sync.");
        return; // Exit if not connected
    }

    // Only attempt to send if data has not been sent successfully before
    if (offlineDataSent) return; // If data has been sent, exit immediately

    Serial.println("Attempting to send offline data to Supabase...");
    static unsigned long lastSyncTime = 0;
    const unsigned long syncInterval = 6000; // Sync every 60 seconds
    const int maxRetries = 3;

    // Check if enough time has passed since the last sync attempt
    if (millis() - lastSyncTime < syncInterval) return;
    lastSyncTime = millis();

    File offlineFile = SPIFFS.open("/offline_attendance.csv", "r");
    if (!offlineFile) {
        Serial.println("No offline data to sync.");
        offlineDataSent = true; // Mark as sent if no data exists
        return;
    }

    bool allSentSuccessfully = true;
    String unsyncedData;

    while (offlineFile.available()) {
        String line = offlineFile.readStringUntil('\n');
        line.trim();
        if (line.isEmpty()) continue;

        // Parse line into components
        int firstComma = line.indexOf(',');
        int secondComma = line.indexOf(',', firstComma + 1);
        int thirdComma = line.indexOf(',', secondComma + 1);

        if (firstComma == -1 || secondComma == -1 || thirdComma == -1) {
            Serial.println("Skipping malformed line: " + line);
            continue;
        }

        String courseCode = line.substring(0, firstComma);
        String studentName = line.substring(firstComma + 1, secondComma);
        String studentId = line.substring(secondComma + 1, thirdComma);
        String timestamp = line.substring(thirdComma + 1);

        // Build JSON payload
        StaticJsonDocument<300> doc;
        doc["course_code"] = courseCode;
        doc["student_name"] = studentName;
        doc["student_id"] = studentId.toInt();
        doc["timestamp"] = timestamp;

        String jsonString;
        serializeJson(doc, jsonString);

        // Send with retry logic
        bool sent = false;
        for (int attempt = 1; attempt <= maxRetries; attempt++) {
            HTTPClient http;
            http.begin(String(SUPABASE_URL) + "/rest/v1/attendance");
            http.addHeader("apikey", SUPABASE_KEY);
            http.addHeader("Authorization", "Bearer " + String(SUPABASE_KEY));
            http.addHeader("Content-Type", "application/json");

            int httpCode = http.POST(jsonString);
            http.end();

            if (httpCode == 200 || httpCode == 201) {
                sent = true;
                break; // Exit the retry loop on success
            } else {
                Serial.printf("Attempt %d failed. HTTP: %d\n", attempt, httpCode);
                delay(1000 * attempt); // Exponential backoff
            }
        }

        if (!sent) {
            allSentSuccessfully = false;
            unsyncedData += line + "\n"; // Retain unsent data
        }
    }
    offlineFile.close();

    // Update offline file
    if (!allSentSuccessfully) {
        File writeFile = SPIFFS.open("/offline_attendance.csv", "w");
        if (writeFile) {
            writeFile.print(unsyncedData);
            writeFile.close();
            Serial.println("Unsynced data retained in file.");
            offlineDataSent = false; // Ensure future sync attempts
        } else {
            Serial.println("Failed to update offline file.");
        }
    } else {
        SPIFFS.remove("/offline_attendance.csv");
        Serial.println("All data synced. File removed.");
        offlineDataSent = true; // Set flag to true after successful send
    }
}



void handleSDImportTeachers() {
  if (!authenticateUser()) return;

  if (!SD.exists("/teachers.csv")) {
    server.send(404, "text/html", "<h2>Error: teachers.csv not found on SD card.</h2>");
    return;
  }

  File sdFile = SD.open("/teachers.csv");
  File spiffsFile = SPIFFS.open("/teachers.csv", "w");

  if (!sdFile || !spiffsFile) {
    server.send(500, "text/html", "<h2>Error: File access failed.</h2>");
    return;
  }

  while (sdFile.available()) {
    spiffsFile.write(sdFile.read());
  }

  sdFile.close();
  spiffsFile.close();

  server.sendHeader("Location", "/editTeacher?import=1");
  server.send(303);
}

void handleSDImportStudents() {
  if (!authenticateUser()) return;

  if (!SD.exists("/students.csv")) {
    server.send(404, "text/html", "<h2>Error: students.csv not found on SD!</h2>");
    return;
  }

  File sdFile = SD.open("/students.csv", FILE_READ);
  File spiffsFile = SPIFFS.open("/students.csv", FILE_WRITE);

  if (!sdFile || !spiffsFile) {
    server.send(500, "text/html", "<h2>File access error!</h2>");
    if (sdFile) sdFile.close();
    return;
  }

  spiffsFile.print(sdFile.readString());
  
  sdFile.close();
  spiffsFile.close();

  server.sendHeader("Location", "/editStudent?import=success");
  server.send(303);
}

void handleSDViewTeachers() {
  if (!authenticateUser()) return;

  if (!SD.exists("/teachers.csv")) {
    server.send(404, "text/html", "<h2>No teacher data on SD card!</h2>");
    return;
  }

  File file = SD.open("/teachers.csv");
  String html = R"(
    <h2>SD Card Teacher Data</h2>
    <table border='1'>
      <tr><th>ID</th><th>Name</th><th>Course</th><th>Code</th></tr>
  )";

  while (file.available()) {
    String line = file.readStringUntil('\n');
    std::vector<String> cols = parseCSVLine(line, {0,1,2,3});
    html += "<tr>";
    for (String col : cols) {
      html += "<td>" + col + "</td>";
    }
    html += "</tr>";
  }

  html += "</table>";
  file.close();
  server.send(200, "text/html", html);
}

void handleSDViewStudents() {
  if (!authenticateUser()) return;

  if (!SD.exists("/students.csv")) {
    server.send(404, "text/html", "<h2>No student data on SD card!</h2>");
    return;
  }

  File file = SD.open("/students.csv");
  String html = R"(
    <h2>SD Card Student Data</h2>
    <table border='1'>
      <tr><th>ID</th><th>Name</th><th>Roll No</th></tr>
  )";

  while (file.available()) {
    String line = file.readStringUntil('\n');
    std::vector<String> cols = parseCSVLine(line, {0,1,2});
    html += "<tr>";
    for (String col : cols) {
      html += "<td>" + col + "</td>";
    }
    html += "</tr>";
  }

  html += "</table>";
  file.close();
  server.send(200, "text/html", html);
}
void handleSDDownloadTeachers() {
  if (!authenticateUser()) return;

  if (!SD.exists("/teachers.csv")) {
    server.send(404, "text/html", "<h2>File not found!</h2>");
    return;
  }

  File file = SD.open("/teachers.csv");
  server.sendHeader("Content-Disposition", "attachment; filename=teachers_sd.csv");
  server.streamFile(file, "text/csv");
  file.close();
}
void handleSDDownloadStudents() {
  if (!authenticateUser()) return;

  if (!SD.exists("/students.csv")) {
    server.send(404, "text/html", "<h2>File not found!</h2>");
    return;
  }

  File file = SD.open("/students.csv");
  server.sendHeader("Content-Disposition", "attachment; filename=students_sd.csv");
  server.streamFile(file, "text/csv");
  file.close();
}
void handleSDDownloadAttendance() {
  if (!authenticateUser()) return;

  if (!SD.exists("/attendance.csv")) {
    server.send(404, "text/html", "<h2>No attendance data on SD!</h2>");
    return;
  }

  File file = SD.open("/attendance.csv");
  server.sendHeader("Content-Disposition", "attachment; filename=full_attendance.csv");
  server.streamFile(file, "text/csv");
  file.close();
}

// Repeat similarly for handleSDDownloadStudents() and handleSDDownloadAttendance()
//55
void handleDownloadOfflineData() {
    // Attempt to open the offline attendance file
    File offlineFile = SPIFFS.open("/offline_attendance.csv", "r");
    if (!offlineFile) {
        Serial.println("Error: Failed to open offline attendance file.");
        server.send(500, "text/plain", "Error: Failed to open offline attendance file.");
        return;
    }

    // Check if the file is empty
    if (offlineFile.size() == 0) {
        Serial.println("Warning: Offline attendance file is empty.");
        server.send(204, "text/plain", "No offline attendance data available."); // 204: No Content
        offlineFile.close();
        return;
    }

    // Prepare HTTP headers for file download
    server.sendHeader("Content-Disposition", "attachment; filename=offline_attendance.csv");
    server.sendHeader("Cache-Control", "no-cache");

    // Stream the file to the client
    if (server.streamFile(offlineFile, "text/csv") != offlineFile.size()) {
        Serial.println("Warning: File streaming incomplete.");
    } else {
        Serial.println("Offline attendance file streamed successfully.");
    }

    offlineFile.close(); // Close the file after streaming
}


void setupWebServerRoutes() {
    server.on("/editTeacher", HTTP_GET, handleEditTeacher);
    server.on("/updateTeacher", HTTP_GET, handleUpdateTeacher);
    server.on("/deleteTeacher", HTTP_GET, handleDeleteTeacher);
    server.on("/editStudent", HTTP_GET, handleEditStudent);
    server.on("/updateStudent", HTTP_GET, handleUpdateStudent);
    server.on("/deleteStudent", HTTP_GET, handleDeleteStudent);
    server.on("/", HTTP_GET, handleRoot);
    server.on("/wifi", HTTP_GET, handleWifiSettings);
    server.on("/saveWifi", HTTP_POST, handleSaveWifi);
    server.on("/enrollTeacher", HTTP_GET, handleEnrollTeacher);
    server.on("/enrollTeacherSubmit", HTTP_POST, []() {
        String teacherId = server.arg("teacherId");
        String teacherName = server.arg("teacherName");
        String courseName = server.arg("courseName");
        String coursecode = server.arg("coursecode");

        enrollTeacher(teacherId.toInt(), teacherName, courseName, coursecode);
        server.send(200, "text/html", "<h2>Teacher enrolled successfully!</h2>");
    });
    server.on("/downloadTeachers", HTTP_GET, handleDownloadTeachers);
    server.on("/enrollStudent", HTTP_GET, handleEnrollStudent);
    server.on("/enrollStudentSubmit", HTTP_POST, []() {
        String studentId = server.arg("studentId");
        String studentName = server.arg("studentName");
        String stdroll = server.arg("stdroll");

        enrollStudent(studentId.toInt(), studentName, stdroll);
        server.send(200, "text/html", "<h2>Student enrolled successfully!</h2>");
    });
    server.on("/downloadStudents", HTTP_GET, handleDownloadStudents);

    server.on("/deleteAllFiles", HTTP_GET, handleDeleteAllCSVFiles);

    server.on("/downloadOfflineData", HTTP_GET, handleDownloadOfflineData);
    server.on("/offline_attendance.csv", HTTP_GET, handleFileRequest);
    server.on("/sdImportTeachers", HTTP_GET, handleSDImportTeachers);
server.on("/sdViewTeachers", HTTP_GET, handleSDViewTeachers);
server.on("/sdDownloadTeachers", HTTP_GET, handleSDDownloadTeachers);

server.on("/sdImportStudents", HTTP_GET, handleSDImportStudents);
server.on("/sdViewStudents", HTTP_GET, handleSDViewStudents);
server.on("/sdDownloadStudents", HTTP_GET, handleSDDownloadStudents);

server.on("/sdDownloadAttendance", HTTP_GET, handleSDDownloadAttendance);
}



void setup() {
    Serial.begin(115200);  // Initialize serial communication
    mySerial.begin(57600); // Start serial for the sensor
    finger.begin(57600);   // Initialize the fingerprint sensor

    // I2C initialization for LCD
    Wire.begin(21, 22);    // SDA = GPIO21, SCL = GPIO22
    lcd.begin(16, 2);      // Initialize 16x2 LCD
    lcd.backlight();
    lcd.clear();
    lcd.print("Starting up...");

    // Initialize the fingerprint sensor
    bool fingerprintInitialized = initializeFingerprint();
    if (!fingerprintInitialized) {
        Serial.println("Failed to initialize fingerprint sensor!");
        lcd.clear();
        lcd.print("Fingerprint Init Failed");
        // Disable fingerprint-related features
        isFingerprintEnabled = false;
    } else {
        isFingerprintEnabled = true;
        Serial.println("Fingerprint sensor initialized successfully.");
    }

    // Preferences initialization
    preferences.begin("WiFi", false);

    // SPIFFS initialization
    if (!SPIFFS.begin(true)) { // Format SPIFFS if mounting fails
        Serial.println("Failed to mount SPIFFS!");
        lcd.clear();
        lcd.print("SPIFFS Failed");
        return;
    }
    Serial.println("SPIFFS mounted successfully.");

    // Retrieve stored Wi-Fi credentials
    ssid = preferences.getString("ssid", "");
    password = preferences.getString("password", "");
    lcd.setCursor(0, 0); // Set cursor to first row, first column
    lcd.print("Connecting...");

    // Wi-Fi setup
    if (ssid == "" || password == "") {
        startAccessPoint(); // Start Access Point if no credentials are saved
    } else {
        if (!connectToWiFi(ssid.c_str(), password.c_str())) { // Attempt to connect to Wi-Fi
            startAccessPoint(); // Start Access Point if connection fails
        }
    }
     // Ensure Wi-Fi is connected
    

    // Set up web server routes
    setupWebServerRoutes();  // Moved all routes to a separate function for better organization

    // Start the web server
    server.begin();
    Serial.println("Web server started.");

    // SD Card initialization
    if (!SD.begin(5)) { // GPIO5 is the CS pin
        Serial.println("SD Card initialization failed!");
        lcd.clear();
        lcd.print("SD Card Failed");
        // Optional: Handle SD card absence gracefully
    } else {
        Serial.println("SD Card initialized.");
    }

    // RTC initialization
    if (!rtc.begin()) {
        Serial.println("RTC initialization failed!");
        lcd.clear();
        lcd.print("RTC Failed");
        while (1); // Halt if RTC is critical
    } else {
        Serial.println("RTC initialized.");
    }
     
    // Optional: Display a final ready message
    lcd.clear();
    lcd.print("System Ready");
    Serial.println("System setup completed successfully.");
    checkWiFiConnection();


}




//clint Server Handle with online check and key for attendance
void checkWiFiConnection() {
    if (WiFi.status() != WL_CONNECTED) {
        offlineDataSent = false; // Reset flag when Wi-Fi connection is lost
        Serial.println("Wi-Fi not connected. Attempting to reconnect...");
        WiFi.reconnect();  // Try reconnecting
        delay(5000);  // Wait 5 seconds before trying again
    }
}

// Function to handle keypad input with debouncing and non-blocking logic
void handleKeypadInput() {
    char key = keypad.getKey();
    if (key) {
        Serial.println("Key Pressed: " + String(key)); // Debug line
        switch (key) {
            case 'A':
                if (!isAttendanceMode) {
                    isAttendanceMode = true;
                    Serial.println("Starting Attendance Process..."); // Debug line
                    startAttendanceProcess();
                }
                break;
            case 'B':
                if (isAttendanceMode) {
                    isAttendanceMode = false;
                    cancelAttendanceProcess();
                }
                break;
        }
    }
}
void handleFileRequest() {
    if (SPIFFS.exists("/offline_attendance.csv")) {
        File file = SPIFFS.open("/offline_attendance.csv", "r");
        server.streamFile(file, "text/csv"); // Set the correct MIME type for CSV
        file.close();
    } else {
        server.send(404, "text/plain", "File not found"); // Send a 404 response if the file doesn't exist
    }
}
// Main loop
void loop() {
   String* taskData;
    if (xQueueReceive(supabaseQueue, &taskData, 0) == pdTRUE) {
        sendToSupabase("students", "", "DELETE", taskData->toInt());
        delete taskData;
    }
    server.handleClient();  // Handle web server client requests
    delay(1);  // Prevent blocking

  
    if (WiFi.status() == WL_CONNECTED) {
        // Save the last time we sent data
        sendOfflineDataToSupabase(); // Attempt to send offline data
    }
    

    handleKeypadInput();  // Process keypad input without blocking

    // Optionally, add more tasks or checks (e.g., for other sensors)
}