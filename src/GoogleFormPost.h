/* (c) 2023 John Mueller - MIT License -- see LICENSE file
 *
 */

#ifndef GOOGLEFORMPOST_H
#define GOOGLEFORMPOST_H
#if defined(ESP8266) || defined(ESP32)

#include <vector>
#include <string>

#ifdef ESP8266

    #include <ESP8266WiFi.h>
    #include <ESP8266HTTPClient.h>
    #include <WiFiClientSecureBearSSL.h>

#elif defined(ESP32)

    #include <WiFi.h>
    #include <HTTPClient.h>
    #include <WiFiClientSecure.h>

#endif

class GoogleFormPost {
    public:
        GoogleFormPost();
        ~GoogleFormPost();
        void GoogleFormPostInit();

        void reset();
        void resetData();
        void showDebug();
        void setFormUrl(String url);

        void addData(String data) { addData(data, ""); }
        void addData(String data, String field);
        int readFields();
        bool send(); // true = ok

        void setDebugPort(Print& port) { _debugPort = port; }
        void setDebugMode(boolean debug_on) { _debugMode = debug_on; }
        void setDebugPrefix(String prefix) { _debugPrefix = prefix; }

    private:
        String _formRootUrl;
        std::vector<String> _fields;
        std::vector<String> _data;
        boolean _debugMode = false;
        Print& _debugPort = Serial;
        String _debugPrefix = "";
        uint32_t _function_time;
        void _startFunctionTime();
        template <typename Generic>
        void _stopFunctionTime(Generic text);

        template <typename Generic>
        void DEBUG_MSG(Generic text);
         
        template <typename Generic, typename Genericb>
        void DEBUG_MSG(Generic text1, Genericb text2);

};

const String _urlencode(const String s);

#endif // ESP8266 or ESP32
#endif
