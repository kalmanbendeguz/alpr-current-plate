// progi3.cpp : Defines the entry point for the application.
//
// KIMENET: currentplate
// KIMENET: first appearance time

#include "date/date.h"
#include "main.h"
#include <string>
#include <iostream>
#include <vector>
#include <chrono>
#include <unordered_set>
#include <algorithm>
#include "json/json/json.h"
#include "json/json/json-forwards.h"
#include "json/jsoncpp.cpp"
#include <fstream>


std::string config_file_location = "dummyconf.conf";
size_t top_n_display = 5;
double minimum_intersect_percent = 10;
unsigned int maximum_wait_for_same_plate_ms = 20000;
std::vector<std::pair<std::string, double>> previousplate;
std::vector<std::pair<std::string, double>> currentplate;

long long first_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
long long last_time = first_time;

// CHECKED
void parseArguments(int argc, char** argv) {
    for (int i = 0; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--config" || arg == "-c") {
            config_file_location = argv[i + 1];
        }
    }
}

// CHECKED
void readConfig() {
    std::ifstream infile(config_file_location);
    std::string key;
    std::string value;
    std::string eql_symbol;
    while (infile >> key >> eql_symbol >> value)
    {
        if (key == "top_n_display") top_n_display = std::stoi(value);
        else if (key == "minimum_intersect_percent") minimum_intersect_percent = std::stod(value);
        else if (key == "maximum_wait_for_same_plate_ms") maximum_wait_for_same_plate_ms = std::stoi(value);
    }
}

// CHECKED
template <class Precision>
std::string getISOTimestamp(long long time_ms)
{
    std::chrono::milliseconds dur(time_ms);
    std::chrono::time_point<std::chrono::system_clock> dt(dur);
    return date::format("%FT%TZ", date::floor<Precision>(dt));
}

// CHECKED
void printJSON() {
    Json::Value plate;
    Json::Value plate_values(Json::arrayValue);

    for (size_t i = 0; i < previousplate.size() && i < top_n_display; ++i) {
        Json::Value single_plate;
        single_plate["value"] = previousplate[i].first;
        single_plate["confidence"] = previousplate[i].second;
        plate_values.append(single_plate);
    }

    plate["plates"] = plate_values;

    plate["first_appearance"] = getISOTimestamp<std::chrono::milliseconds>(first_time);
    plate["last_appearance"] = getISOTimestamp<std::chrono::milliseconds>(last_time);
    Json::FastWriter fastWriter;
    std::cout << fastWriter.write(plate); 
}


// CHECKED
size_t vec_intersect(const std::vector<std::pair<std::string, double>>& A_, const std::vector<std::pair<std::string, double>>& B_)
{
    std::vector<std::string> previous_plates_only;
    std::vector<std::string> current_plates_only;
    std::transform(begin(A_), end(A_),
        std::back_inserter(previous_plates_only),
        [](auto const& pair) { return pair.first; });
    std::transform(begin(B_), end(B_),
        std::back_inserter(current_plates_only),
        [](auto const& pair) { return pair.first; });
    //for (int i = 0; i < previous_plates_only.size(); ++i) {
    //    std::cerr << previous_plates_only[i] << ":" << A_[i].second << "\t";
    //}
    //std::cerr << std::endl;
    //for (int i = 0; i < current_plates_only.size(); ++i) {
    //    std::cerr << current_plates_only[i] << ":" << B_[i].second << "\t";
    //}
    //std::cerr << std::endl;
    std::unordered_set<std::string> aSet(previous_plates_only.cbegin(), previous_plates_only.cend());
    return std::count_if(current_plates_only.cbegin(), current_plates_only.cend(), [&](std::string element) {
        return aSet.find(element) != aSet.end();
    });
}

// CHECKED
bool checkIntersection() {
    size_t smaller_collection_size = previousplate.size() < currentplate.size() ? previousplate.size() : currentplate.size();
    size_t intersection_size = vec_intersect(previousplate, currentplate);
    double intersection_rate = (1.0 * intersection_size) / (1.0 * smaller_collection_size);
    double intersection_rate_percent = 100.0 * intersection_rate;
    //std::cerr << "intersection percent: " << intersection_rate_percent << std::endl;
    if (intersection_rate_percent >= minimum_intersect_percent) {
        return true;
    }
    return false;
}

// CHECKED
void readCurrentPlate() {
    std::string line;
    while (std::getline(std::cin, line) && line != "----------")
    {
        size_t plate_first = line.find("[PLATE]") + 8;
        size_t plate_last = line.find("]", plate_first) - 1;
        size_t confidence_first = line.find("[", plate_last) + 1;
        size_t confidence_last = line.find("]", confidence_first) - 1;
        std::string plate = line.substr(plate_first, plate_last - plate_first + 1);
        double confidence = std::stod(line.substr(confidence_first, confidence_last - confidence_first + 1));
        currentplate.push_back({ plate, confidence });
    }
}

// CHECKED
bool compare_confidences(const std::pair<std::string, double>& first, const std::pair<std::string, double>& second) {
    return (first.second > second.second);
}

// CHECKED
bool compare_plates(const std::pair<std::string, double>& first, const std::pair<std::string, double>& second) {
    return (first.first == second.first);
}


// CHECKED
int main(int argc, char** argv)
{
    parseArguments(argc, argv);
    readConfig();
    std::string line;

    while (std::getline(std::cin, line))
    {
        // getting line
        if (line == "----------") {
            currentplate.clear();

            // line == "----------"
            if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() > last_time + maximum_wait_for_same_plate_ms) {
                // time expired 
                first_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                last_time = first_time;
                previousplate.clear();
            }

            // reading current plate
            readCurrentPlate();
            if (checkIntersection()) { // nem változott a rendszám
                // INTERSECT 
                last_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                
                //previousplate frissül currentplate-t figyelembe véve
                size_t previousplate_size = previousplate.size();
                previousplate.insert(previousplate.end(), currentplate.begin(), currentplate.end());

                std::sort(previousplate.begin(), previousplate.end(), compare_confidences);
                previousplate.erase(std::unique(previousplate.begin(), previousplate.end(), compare_plates), previousplate.end());
                previousplate.resize(previousplate_size);
            }
            else { // új rendszám
                // INTERSECT
                previousplate = currentplate;
                first_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                last_time = first_time;
                printJSON();
            }
            
        }
    }
	return 0;
}
