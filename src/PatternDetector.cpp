/* 
 * This file is part of the VERNIER Library.
 *
 * Copyright (c) 2018-2025 CNRS, ENSMM, UMLP.
 */

#include "PatternDetector.hpp"

namespace vernier {

    PatternDetector::PatternDetector() {
        classname = "PatternDetector";
        description = "";
        date = "";
        author = "";
        unit = "";                        
    }

    void PatternDetector::readJSON(rapidjson::Value& document) {
        if (document.HasMember("description") && document["description"].IsString()) {
            description = document["description"].GetString();
        } else {
            description = "";
        }
        if (document.HasMember("date") && document["date"].IsString()) {
            date = document["date"].GetString();
        } else {
            date = "";
        }
        if (document.HasMember("author") && document["author"].IsString()) {
            author = document["author"].GetString();
        } else {
            author = "";
        }
        if (document.HasMember("unit") && document["unit"].IsString()) {
            unit = document["unit"].GetString();
        } else {
            unit = "";
        }
    }

    void PatternDetector::loadFromJSON(std::string filename) {
        BufferedReader bufferedReader(filename);

        rapidjson::Document document;
        document.ParseInsitu(bufferedReader.data());
        if (!document.IsObject()) {
            throw Exception(filename + " is not a valid JSON file.");
        }
        if (document.MemberBegin() == document.MemberEnd()) {
            throw Exception(filename + " is empty.");
        }
        readJSON(document.MemberBegin()->value);
    }

    void PatternDetector::compute(char* data, int rows, int cols) {
        Eigen::MatrixXd mPatternMatrix(rows, cols);
        std::memcpy(mPatternMatrix.data(), data, rows * cols * sizeof (double));

        Eigen::ArrayXXd patternImage = mPatternMatrix.array();
        computeArray(patternImage);
    }

    void PatternDetector::compute(const cv::Mat & image) {
        Eigen::ArrayXXd array = image2array(image);
        computeArray(array);
    }
    
    void PatternDetector::computeArray(const Eigen::ArrayXXd & array) {
        cv::Mat image = array2image(array);
        compute(image);
    }
    
    void PatternDetector::draw(cv::Mat& image) {
        cv::putText(image, toString(), cv::Point(3, 15), cv::FONT_HERSHEY_PLAIN, 1, cv::Scalar(0, 0, 255), 2);
    }

    std::string PatternDetector::toString() {
        std::string result = classname + " detector";
        if (description!="") result += " " + description;
        if (author!="") result += " by " + author;
        if (date!="") result+= " " + date;
        result+= " with";
        return result;
    }

    std::string PatternDetector::getAuthor() {
        return author;
    }

    std::string PatternDetector::getDate() {
        return date;
    }

    std::string PatternDetector::getDescription() {
        return description;
    }

    std::string PatternDetector::getUnit() {
        return unit;
    }

    std::string PatternDetector::getClassname() {
        return classname;
    }

    void* PatternDetector::getObject(const std::string & attribute) {
        throw Exception("The parameter " + attribute + " is not accessible or defined in class " + classname + ".");
    }

    double PatternDetector::getDouble(const std::string & attribute) {
        throw Exception("The parameter " + attribute + " is not accessible or defined in class " + classname + ".");
    }

    int PatternDetector::getInt(const std::string & attribute) {
        throw Exception("The parameter " + attribute + " is not accessible or defined in class " + classname + ".");
    }

    bool PatternDetector::getBool(const std::string & attribute) {
        throw Exception("The parameter " + attribute + " is not accessible or defined in class " + classname + ".");
    }

    std::string PatternDetector::getString(const std::string & attribute) {
        if (attribute == "classname") {
            return classname;
        } else if (attribute == "description") {
            return description;
        } else if (attribute == "date") {
            return date;
        } else if (attribute == "description") {
            return description;
        } else if (attribute == "author") {
            return author;
        } else if (attribute == "unit") {
            return unit;
        } else {
            throw Exception("The parameter " + attribute + " is not accessible or defined in class " + classname + ".");
        }
    }

    void PatternDetector::setDouble(const std::string & attribute, double value) {
        std::cout << "The parameter " + attribute + " is not accessible or defined in class " + classname + "." << std::endl;
    }

    void PatternDetector::setInt(const std::string & attribute, int value) {
        std::cout << "The parameter " + attribute + " is not accessible or defined in class " + classname + "." << std::endl;
    }

    void PatternDetector::setBool(const std::string & attribute, bool value) {
        std::cout << "The parameter " + attribute + " is not accessible or defined in class " + classname + "." << std::endl;
    }

    void PatternDetector::setString(const std::string & attribute, std::string value) {
        if (attribute == "classname") {
            classname = value;
        } else if (attribute == "description") {
            description = value;
        } else if (attribute == "date") {
            date = value;
        } else if (attribute == "description") {
            description = value;
        } else if (attribute == "author") {
            author = value;
        } else if (attribute == "unit") {
            unit = value;
        } else {
            std::cout << "The parameter " + attribute + " is not accessible or defined in class " + classname + "." << std::endl;
        }
    }

}

