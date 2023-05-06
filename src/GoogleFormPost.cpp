/* (c) 2023 John Mueller - MIT License -- see LICENSE file
 *
 */

#include "GoogleFormPost.h"

/**
 * --------------------------------------------------------------------------------
 *  GoogleFormPost 
 * --------------------------------------------------------------------------------
**/

// constructor
GoogleFormPost::GoogleFormPost() {
    GoogleFormPostInit();
}

// reset stuff
void GoogleFormPost::GoogleFormPostInit() {
    _data.clear();
    _fields.clear();
    _formRootUrl = "";
}

// destructor
GoogleFormPost::~GoogleFormPost() {
    GoogleFormPostInit();
}

// clear settings
void GoogleFormPost::reset() {
    GoogleFormPostInit();
}

// just reset the data
void GoogleFormPost::resetData() {
    _data.clear();
    _fields.clear();
}

// set the form URL + strip cruft
void GoogleFormPost::setFormUrl(String url) {
    _formRootUrl = url; 
    int i = _formRootUrl.indexOf("#"); // remove anchor
    if (i>0) _formRootUrl = _formRootUrl.substring(0, i);
    i = _formRootUrl.indexOf("?"); // remove querystrings
    if (i>0) _formRootUrl = _formRootUrl.substring(0, i);
    if (_formRootUrl.endsWith("/formResponse")) // clean out action part
        _formRootUrl = _formRootUrl.substring(0, _formRootUrl.length()-String("/formResponse").length()+1);
    if (_formRootUrl.endsWith("/viewform")) 
        _formRootUrl = _formRootUrl.substring(0, _formRootUrl.length()-String("/viewform").length()+1);
    DEBUG_MSG("URL set:", _formRootUrl);
}

// add some data to the form
void GoogleFormPost::addData(String data, String field) {
    _data.push_back(data);
    if (field && field!="") _fields.push_back(field);
}

// read the field IDs from form; returns number of fields
int GoogleFormPost::readFields() {
    // funky rolling window stream scraper to find data fields
    // returns number of fields, -1=error
    std::vector<String> found; // results go here
    String form_url = _formRootUrl + "viewform";

    _startFunctionTime();
    DEBUG_MSG("Reading fields from", form_url);
    #ifdef ESP8266
    BearSSL::WiFiClientSecure client;
    #else
    WiFiClientSecure client;
    #endif
    client.setInsecure();
    HTTPClient https;
    bool isok = https.begin(client, form_url.c_str());
    if (!isok) {
        DEBUG_MSG("https.begin()", "begin failed.");
        return -1;
    } else {
        DEBUG_MSG("https.begin()", "begin ok");
    }
    int httpResponseCode = https.GET();
    if (httpResponseCode != HTTP_CODE_OK) {
        DEBUG_MSG("Failed! HTTP result", httpResponseCode);
        https.end();
        return -1;
    }

    // create buffer for read
    #define READ_BUFFER 1024
    #define MIN_ROUND_BUFFER 100 // enough for start to end token
    #define MY_HTTP_TIMEOUT 2000

    uint32_t http_timeout = millis() + MY_HTTP_TIMEOUT;
    uint8_t input_buffer[READ_BUFFER+2];
    String round_buffer; // inefficient, but idgaf
    String TOKEN_START1 = "params=\"%.@.["; // before the ID for the field, brittle baby
    String TOKEN_START2 = "[["; // before the ID for the field, brittle baby
    String TOKEN_STOP = ","; // after the field ID, what a hack
    String TOKEN_PREFIX = "entry."; // add this to field IDs

    int len = https.getSize(); // get length of document (or -1)
    WiFiClient * stream = https.getStreamPtr();    

    // read all data from server
    while (https.connected() && (len > 0 || len == -1)) {
        size_t size = stream->available(); // get available data size
        if (size) {
            // read into input buffer
            int c = stream->readBytes(input_buffer, ((size>READ_BUFFER)?READ_BUFFER:size));
            input_buffer[c] = 0; // mark terminator for input buffer
            round_buffer += String((const char *)input_buffer); // append to round buffer
            
            // look for start token and end token
            int start_grp = round_buffer.indexOf(TOKEN_START1);
            while (start_grp>0) { // seek actual start
                int start_pos = round_buffer.indexOf(TOKEN_START2, 
                                                    start_grp+TOKEN_START1.length()+1);
                if (start_pos>0) {
                    int end_pos = round_buffer.indexOf(TOKEN_STOP, 
                                                        start_pos+TOKEN_START2.length()+3);
                    if (end_pos>0) { // got one
                        String item = round_buffer.substring(
                            start_pos + TOKEN_START2.length(), end_pos);
                        DEBUG_MSG("Found:", item);
                        found.push_back(TOKEN_PREFIX + item);
                        round_buffer = round_buffer.substring(end_pos);
                        start_grp = round_buffer.indexOf(TOKEN_START1);
                    } else start_grp = 0; // no more matches in this buffer
                } else start_grp = 0; // no more matches in this buffer
            }

            // truncate round buffer
            if (round_buffer.length()>MIN_ROUND_BUFFER) {
                round_buffer = round_buffer.substring(round_buffer.length()-MIN_ROUND_BUFFER);
            }
            http_timeout = millis() + MY_HTTP_TIMEOUT; // we can wait more, if it's working
        } else if (millis() > http_timeout) {
            break; // stopping, since it's taking too long
        } 
        delay(1); // give it a ms to get new data
    }

    stream->stop(); // no need to process rest
    https.end(); // Free resources

    // push fields to known field list
    _fields = found;
    //for (int i = 0; i < (int)found.size(); i++) _fields.push_back(found[i]);
    DEBUG_MSG("Fields found:", found.size());
    _stopFunctionTime("readFields()");
    return found.size();
}

// URL-Encode a string
const String _urlencode(const String s) {
    String output; // not very efficient, it's fine
    for (int i=0; i<(int)s.length(); i++) {
        char c = s[i];
        if ( (c>='a' && c<='z') || (c>='A' && c<='Z') || (c>='0' && c<='9') ) {
            output += c;
        } else if (c==' ') {
            output += '+';
        } else {
            output += "%" + String(c, HEX);
        }
    }
    return output;
}

// show what we have
void GoogleFormPost::showDebug() {
    if (!_debugMode) return;
    DEBUG_MSG("GoogleFormPost:");
    DEBUG_MSG(" URL:", _formRootUrl);
    DEBUG_MSG(" Fields:");
    for (int i = 0; i < (int)_fields.size(); i++)
        DEBUG_MSG("  ", _fields[i]);
    DEBUG_MSG(" Data:");
    for (int i = 0; i < (int)_data.size(); i++)
        DEBUG_MSG("  ", _data[i]);
}

// Send the data to the form
bool GoogleFormPost::send() {
    _startFunctionTime();
    String post_url = _formRootUrl + "formResponse";
    String post_data = ""; 
    for (int i = 0; i < (int)_fields.size(); i++) { // build URL-encoded form data
        if (i>0) post_data += "&";
        post_data += _urlencode(_fields[i]);
        post_data += "=";
        post_data += _urlencode(_data[i]);
    }

    // setup client & skip SSL validation
    #ifdef ESP8266
    BearSSL::WiFiClientSecure client;
    #else
    WiFiClientSecure client;
    #endif
    client.setInsecure();
    HTTPClient https;

    DEBUG_MSG("URL:", post_url);
    bool isok = https.begin(client, post_url.c_str());
    if (!isok) {
        DEBUG_MSG("https.begin()", "begin failed.");
        return false;
    } else {
        DEBUG_MSG("https.begin()", "begin ok");
    }
    https.addHeader("Content-Type", "application/x-www-form-urlencoded");
    DEBUG_MSG("Data:", post_data);
    int httpResponseCode = https.POST(post_data.c_str());

    DEBUG_MSG("HTTP Response code:", httpResponseCode);
    if (httpResponseCode<=0) {
        DEBUG_MSG("Error after POST");
        https.end();
        return false;
    }

    // Free resources
    https.end();
    _stopFunctionTime("send()");
    return true;
}

uint32_t _function_time;
void GoogleFormPost::_startFunctionTime() {
    _function_time = millis();
}

template <typename Generic>
void GoogleFormPost::_stopFunctionTime(Generic text) {
    if (!_debugMode) return;
    uint32_t duration = millis() - _function_time;
    DEBUG_MSG(String("Duration ") + text, duration);
}


// post a single debug message
template <typename Generic>
void GoogleFormPost::DEBUG_MSG(Generic text) {
    DEBUG_MSG(text, "");
}

// post a debug message with two parts
template <typename Generic, typename Genericb>
void GoogleFormPost::DEBUG_MSG(Generic text1, Genericb text2) {
    if (!_debugMode) return;
    _debugPort.print(_debugPrefix);
    _debugPort.print(text1);
    if (text2) {
        _debugPort.print(" ");
        _debugPort.print(text2);
    }
    _debugPort.println();
}
