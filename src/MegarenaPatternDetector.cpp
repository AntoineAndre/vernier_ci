/* 
 * This file is part of the VERNIER Library.
 *
 * Copyright (c) 2018-2023 CNRS, ENSMM, UFC.
 */

#include "MegarenaPatternDetector.hpp"
#include "MegarenaBitSequences.cpp"

namespace Vernier {

    MegarenaPatternDetector::MegarenaPatternDetector()
    : PeriodicPatternDetector() {
        classname = "MegarenaPattern";
    }

    MegarenaPatternDetector::MegarenaPatternDetector(Eigen::ArrayXXi bitSequence, double physicalPeriod)
    : PeriodicPatternDetector() {
        classname = "MegarenaPattern";
        this->physicalPeriod = physicalPeriod;
        this->bitSequence = bitSequence;
    }

    void MegarenaPatternDetector::compute(Eigen::ArrayXXd& pattern) {
        resize(pattern.rows(), pattern.cols());
        PeriodicPatternDetector::compute(pattern);
        computeAbsolutePose(pattern);
    }

    void MegarenaPatternDetector::computeAbsolutePose(Eigen::ArrayXXd& pattern) {
        double approxPixelPeriod = (plane1.getPixelicPeriod() + plane2.getPixelicPeriod()) / 2.0;
        patternPhase.setPixelPeriod(approxPixelPeriod);

        length1 = (pattern.rows() / (approxPixelPeriod));
        length2 = (pattern.cols() / (approxPixelPeriod));

        length1++;
        length2++;

        if (length1 % 2 == 0) {
            length1++;
        }
        if (length2 % 2 == 0) {
            length2++;
        }

        thumbnail.resize(length1, length2);
        thumbnail.compute(plane1, plane2, pattern);

        Eigen::ArrayXXd sequence1 = thumbnail.getSequence1();
        Eigen::ArrayXXd sequence2 = thumbnail.getSequence2();

        int MSB1 = thumbnail.getMSB1();
        int MSB2 = thumbnail.getMSB2();

        codePosition1 = decoding.findCodePosition(sequence1, MSB1);
        codePosition2 = decoding.findCodePosition(sequence2, MSB2);

        plane1Save = plane1;
        plane2Save = plane2;

        if (codePosition1 >= 0 && codePosition2 >= 0) {
            // no change	
            //std::cout<<"code1>0 && code2>0"<<std::endl;
        } else if (codePosition1 < 0 && codePosition2 >= 0) {
            std::swap(codePosition1, codePosition2);
            std::swap(plane1, plane2);
            plane2.revertCoefficients();
            //std::cout<<"code1<0 && code2>0"<<std::endl;
            thumbnail.rotate270();
        } else if (codePosition1 >= 0 && codePosition2 < 0) {
            std::swap(codePosition1, codePosition2);
            std::swap(plane1, plane2);
            plane1.revertCoefficients();
            //std::cout<<"code1>0 && code2<0"<<std::endl;
            thumbnail.rotate90();
        } else {
            plane1.revertCoefficients();
            plane2.revertCoefficients();
            //std::cout<<"code1<0 && code2<0"<<std::endl;
            thumbnail.rotate180();
        }

    }

    Pose MegarenaPatternDetector::get2DPose() {
        double x = -plane1.getPosition(physicalPeriod, 0.0, 0.0, codePosition1);
        double y = -plane2.getPosition(physicalPeriod, 0.0, 0.0, codePosition2);
        double alpha = plane1.getAngle();
        double pixelSize = physicalPeriod / plane1.getPixelicPeriod();

        return Pose(x, y, alpha, pixelSize);
    }

    std::vector<Pose> MegarenaPatternDetector::getAll3DPoses() {
        double x = -plane1.getPosition(physicalPeriod, 0.0, 0.0, codePosition1);
        double y = -plane2.getPosition(physicalPeriod, 0.0, 0.0, codePosition2);

        std::vector<Pose> poseVec = PeriodicPatternDetector::getAll3DPoses();
        for (int i = 0; i < poseVec.size(); i++) {
            poseVec[i].x = x;
            poseVec[i].y = y;
        }
        return poseVec;
    }

    Thumbnail MegarenaPatternDetector::getThumbnail() {
        return thumbnail;
    }

    void MegarenaPatternDetector::showControlImages() {
#ifdef USE_OPENCV       
        cv::imshow("Thumbnail", this->thumbnail.getMeanDotsImage());
#endif // USE_OPENCV
        PeriodicPatternDetector::showControlImages();
    }

    void MegarenaPatternDetector::readJSON(rapidjson::Value& document) {

        PatternDetector::readJSON(document);

        if (document.HasMember("period") && document["period"].IsDouble()) {
            physicalPeriod = document["period"].GetDouble();
            PeriodicPatternDetector::setPhysicalPeriod(physicalPeriod);
        } else {
            throw Exception("The file is not a valid bitmap pattern file, the period is missing or has a wrong format.");
        }

        if (document.HasMember("bitSequence") && document["bitSequence"].IsArray()) {
            bitSequence.resize(1, document["bitSequence"].Size());

            for (rapidjson::SizeType row = 0; row < bitSequence.cols(); row++) {
                const rapidjson::Value& value = document["bitSequence"][row];
                if (value.IsInt()) {
                    bitSequence(0, row) = value.GetInt();
                } else {
                    throw Exception("The file is not a valid bitmap pattern file, the row " + toString(row) + " of the bitmap has a wrong format");
                }
            }
        } else if (document.HasMember("codeSize") && document["codeSize"].IsInt()) {
            int codeSize = document["codeSize"].GetInt();
            if (codeSize == 8) {
                initializeBitSequence8(bitSequence);
            } else if (codeSize == 12) {
                initializeBitSequence12(bitSequence);
            } else {
                throw Exception("The file is not a valid megarena pattern file, the code size must be 8 or 12.");
            }
        } else {
            throw Exception("The file is not a valid bitmap pattern file, the bitmap is missing or has a wrong format.");
        }

        decoding.resize(bitSequence);
    }

    int MegarenaPatternDetector::getInt(const std::string & attribute) {
        if (attribute == "codePosition1") {
            return codePosition1;
        } else if (attribute == "codePosition2") {
            return codePosition2;
        } else {
            return PeriodicPatternDetector::getInt(attribute);
        }
    }

    void* MegarenaPatternDetector::getObject(const std::string & attribute) {
        if (attribute == "bitSequence") {
            return &bitSequence;
        } else if (attribute == "decoding") {
            return &decoding;
        } else if (attribute == "thumbnail") {
            return &thumbnail;
        } else {
            return PatternDetector::getObject(attribute);
        }
    }

}