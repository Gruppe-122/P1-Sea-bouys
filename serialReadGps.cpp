#include <iostream>
#include <fstream>
#include <windows.h>
#include <string>
#include <ctime>
#include <sstream>
#include <regex>
#include <iomanip>

// Function to parse parsed GPS line: [parsed] Lat: 57.014841  Lon: 9.985845  Sats:19  HDOP:0.7  Alt:44.5 m  Time:13:04:15Z
bool parseParsedLine(const std::string& line, double& lat, double& lon, int& sats, double& hdop, double& alt, std::string& time) {
    std::regex pattern(R"(\[parsed\].*Lat:\s*([-\d.]+).*Lon:\s*([-\d.]+).*Sats:(\d+).*HDOP:([\d.-]+).*Alt:([\d.-]+).*Time:(\d{2}:\d{2}:\d{2}))");
    std::smatch match;
    
    if (std::regex_search(line, match, pattern)) {
        lat = std::stod(match[1]);
        lon = std::stod(match[2]);
        sats = std::stoi(match[3]);
        hdop = std::stod(match[4]);
        alt = std::stod(match[5]);
        time = match[6];
        return true;
    }
    return false;
}

// Function to parse status line: [status] chars=1379871 sats=19 hdop=0.7 loc.valid=yes
bool parseStatusLine(const std::string& line, int& chars, int& sats, double& hdop, bool& locValid) {
    std::regex pattern(R"(\[status\].*chars=(\d+).*sats=(\d+).*hdop=([\d.-]+).*loc\.valid=(yes|no))");
    std::smatch match;

    if (std::regex_search(line, match, pattern)) {
        chars = std::stoi(match[1]);
        sats = std::stoi(match[2]);
        hdop = std::stod(match[3]);
        locValid = (match[4] == "yes");
        return true;
    }
    return false;
}

int main() {
    // Open COM7
    HANDLE hSerial = CreateFileA("COM7", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    
    if (hSerial == INVALID_HANDLE_VALUE) {
        std::cerr << "Error opening COM7" << std::endl;
        return 1;
    }

    // Configure serial port
    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    
    if (!GetCommState(hSerial, &dcbSerialParams)) {
        std::cerr << "Error getting serial state" << std::endl;
        CloseHandle(hSerial);
        return 1;
    }
    
    dcbSerialParams.BaudRate = CBR_115200;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    
    if (!SetCommState(hSerial, &dcbSerialParams)) {
        std::cerr << "Error setting serial state" << std::endl;
        CloseHandle(hSerial);
        return 1;
    }

    // Open CSV file for parsed GPS data
    std::ofstream csvFile("gps_data.csv");
    csvFile << "PC_Timestamp,Latitude,Longitude,Satellites,HDOP,Altitude_m,GPS_Time\n";

    // Open raw log file for debugging
    std::ofstream logFile("gps_raw.log");

    // Read data
    char buffer[1024];
    DWORD bytesRead;
    std::string lineBuffer = "";

    std::cout << "Reading from COM7... (Press Ctrl+C to stop)" << std::endl;

    while (true) {
        if (ReadFile(hSerial, buffer, sizeof(buffer) - 1, &bytesRead, NULL)) {
            if (bytesRead > 0) {
                buffer[bytesRead] = '\0';
                lineBuffer += std::string(buffer);

                // Process complete lines (delimited by \n)
                size_t pos = 0;
                while ((pos = lineBuffer.find('\n')) != std::string::npos) {
                    std::string line = lineBuffer.substr(0, pos);
                    lineBuffer.erase(0, pos + 1);

                    // Remove carriage returns
                    if (!line.empty() && line.back() == '\r') {
                        line.pop_back();
                    }

                    // Write raw data to log
                    logFile << line << "\n";
                    logFile.flush();

                    // Parse parsed GPS line
                    if (line.find("[parsed]") != std::string::npos) {
                        double lat, lon, hdop, alt;
                        int sats;
                        std::string gpsTime;

                        if (parseParsedLine(line, lat, lon, sats, hdop, alt, gpsTime)) {
                            // Get current PC time
                            time_t now = time(0);
                            char pcTimestamp[26];
                            ctime_s(pcTimestamp, sizeof(pcTimestamp), &now);
                            std::string pcTime(pcTimestamp);
                            if (!pcTime.empty() && pcTime.back() == '\n') {
                                pcTime.pop_back();
                            }

                            // Write to CSV
                            csvFile << pcTime << ","
                                   << std::fixed << std::setprecision(6) << lat << ","
                                   << std::fixed << std::setprecision(6) << lon << ","
                                   << sats << ","
                                   << std::fixed << std::setprecision(2) << hdop << ","
                                   << std::fixed << std::setprecision(2) << alt << ","
                                   << gpsTime << "\n";
                            csvFile.flush();

                            std::cout << "GPS Fix: Lat=" << std::fixed << std::setprecision(6) << lat
                                     << " Lon=" << lon << " Sats=" << sats
                                     << " HDOP=" << std::setprecision(2) << hdop << std::endl;
                        }
                    }
                    // Parse status line
                    else if (line.find("[status]") != std::string::npos) {
                        int chars, sats;
                        double hdop;
                        bool locValid;

                        if (parseStatusLine(line, chars, sats, hdop, locValid)) {
                            std::cout << "Status: Chars=" << chars << " Sats=" << sats
                                     << " HDOP=" << std::fixed << std::setprecision(2) << hdop
                                     << " LocValid=" << (locValid ? "Yes" : "No") << std::endl;
                        }
                    }
                }
            }
        } else {
            std::cerr << "Error reading from COM7" << std::endl;
            break;
        }
    }

    csvFile.close();
    logFile.close();
    CloseHandle(hSerial);
    return 0;
}