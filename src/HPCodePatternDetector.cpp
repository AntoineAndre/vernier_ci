/* 
 * This file is part of the VERNIER Library.
 *
 * Copyright (c) 2018-2025 CNRS, ENSMM, UMLP.
 */

#include "HPCodePatternDetector.hpp"

namespace vernier {

    HPCodePatternDetector::HPCodePatternDetector(double physicalPeriod, int snapshotSize, int numberHalfPeriods)
    : PeriodicPatternDetector(physicalPeriod) {
        resize(physicalPeriod, snapshotSize, numberHalfPeriods);
    }

    void HPCodePatternDetector::resize(double physicalPeriod, int snapshotSize, int numberHalfPeriods) {
        ASSERT(snapshotSize % 2 == 0);
        ASSERT(physicalPeriod > 0.0);
        ASSERT((numberHalfPeriods > 0) && (numberHalfPeriods % 4 == 1));
        this->snapshotSize = snapshotSize;
        this->physicalPeriod = physicalPeriod;
        this->numberHalfPeriods = numberHalfPeriods;
        snapshot.resize(snapshotSize, snapshotSize);
        patternPhase.resize(snapshotSize, snapshotSize);
    }

    void HPCodePatternDetector::readJSON(rapidjson::Value& document) {
        throw Exception("HPCodePatternDetector::readJSON is not implemented yet.");
    }

    void HPCodePatternDetector::takeSnapshot(int x, int y, cv::Mat image) {
        snapshot.setConstant(0);
        if (image.channels() > 1) {
            cv::cvtColor(image, image, cv::COLOR_BGR2GRAY);
        }

        cv::Mat snap64f;
        image.convertTo(snap64f, CV_64FC1);
        snap64f /= 256;
        int radius = snapshotSize / 2;
        for (int col = -radius; col < radius; col++) {
            for (int row = -radius; row < radius; row++) {
                double distanceToCenter = sqrt((row + 0.5) * (row + 0.5) + (col + 0.5) * (col + 0.5)); // +0.5 A VERIFIER MAIS PAS CRITIQUE 
                double hanning = (1 + cos(PI * distanceToCenter / radius)) / 2;
                if (distanceToCenter < radius && y + row > 0 && x + row > 0 && y + row < snap64f.rows && x + row < snap64f.cols) {
                    snapshot.real()(radius + row, radius + col) = hanning * snap64f.at<double>(y + row, x + col);
                }
            }
        }
    }

    void HPCodePatternDetector::compute(const cv::Mat& image) {

        detector.compute(image);

        for (int i = 0; i < detector.codes.size(); i++) {

            QRCode code = detector.codes[i];

            if ((int) code.getRadius()*2 > snapshotSize) {
                throw Exception("The HPCode is too large for pose estimation: increase the snapshot size.");
            }
            if ((int) code.getRadius() < numberHalfPeriods) {
                throw Exception("The HPCode is too tiny for pose estimation: increase the picture quality size.");
            }

            int centerX = (int) code.center.x;
            int centerY = (int) code.center.y;
            takeSnapshot(centerX, centerY, image);
            patternPhase.compute(snapshot);

            double alpha;
            double dx, dy;
            double diffAngle = angleInPiPi(patternPhase.getPlane1().getAngle() - code.getAngle());
            if (diffAngle >= -PI / 4 && diffAngle <= PI / 4) {
                alpha = patternPhase.getPlane1().getAngle();
                dx = -patternPhase.getPlane1().getPosition(physicalPeriod, 0.0, 0.0);
                dy = -patternPhase.getPlane2().getPosition(physicalPeriod, 0.0, 0.0);
            } else if (diffAngle >= 3 * PI / 4 || diffAngle <= -3 * PI / 4) {
                alpha = patternPhase.getPlane1().getAngle() + PI;
                dx = +patternPhase.getPlane1().getPosition(physicalPeriod, 0.0, 0.0);
                dy = +patternPhase.getPlane2().getPosition(physicalPeriod, 0.0, 0.0);
            } else if (diffAngle >= PI / 4 && diffAngle <= 3 * PI / 4) {
                alpha = patternPhase.getPlane1().getAngle() - PI / 2;
                dx = patternPhase.getPlane2().getPosition(physicalPeriod, 0.0, 0.0);
                dy = -patternPhase.getPlane1().getPosition(physicalPeriod, 0.0, 0.0);
            } else {
                alpha = patternPhase.getPlane1().getAngle() + PI / 2;
                dx = -patternPhase.getPlane2().getPosition(physicalPeriod, 0.0, 0.0);
                dy = patternPhase.getPlane1().getPosition(physicalPeriod, 0.0, 0.0);
            }

            double pixelSize = physicalPeriod / patternPhase.getPixelPeriod();
            double xImg = (centerX - image.cols / 2);
            double yImg = (centerY - image.rows / 2);
            double x = pixelSize * (xImg * cos(alpha) - yImg * sin(-alpha)) + dx;
            double y = pixelSize * (xImg * sin(-alpha) + yImg * cos(alpha)) + dy;

            Pose pose = Pose(x, y, alpha, pixelSize);

            if ((numberHalfPeriods - 1) % 4 == 0) {
                unsigned long number = readNumber(code, image, patternPhase.getPixelPeriod() / 2.0);

                codes.insert(std::make_pair(number, pose));
            } else {
                codes.insert(std::make_pair(i, pose));
            }

        }
    }

    unsigned long HPCodePatternDetector::readNumber(QRCode& code, const cv::Mat& image, double dotSize) {
        cv::Point2d rightDirection = (code.right - code.top);
        rightDirection *= dotSize / cv::norm(rightDirection);
        cv::Point2d upDirection = (code.top - code.bottom);
        upDirection *= dotSize / cv::norm(upDirection);
        cv::Point2d end = code.top + 3 * upDirection + 3.5 * rightDirection;
        cv::Point2d start = code.right + 3 * upDirection - 3.5 * rightDirection;

        // Getting the line
        cv::LineIterator it(image, cv::Point2i(start), cv::Point2i(end), 8);
        cv::Mat rasterLine(1, it.count, CV_8U);
        for (int i = 0; i < it.count; i++, ++it) {
            rasterLine.at<unsigned char>(0, i) = **it;
        }

        // Analysing the line
        int dotCount = 7 + (int) std::round((cv::norm(code.top - code.right) / dotSize));
        //std::cout << "dotCount : " << 7 + (double)(cv::norm(code.top - code.right) / dotSize) << std::endl;
        cv::Mat dataLine(1, dotCount - 14, CV_8U);
        cv::resize(rasterLine, dataLine, dataLine.size(), 0, 0, cv::INTER_AREA);
        cv::threshold(dataLine, dataLine, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
        //std::cout << "data line : " << dataLine << std::endl;
        int zeroCount = 0;
        for (int col = 0; col < dataLine.cols; col += 2) {
            if (dataLine.at<unsigned char>(0, col) == 0) {
                zeroCount++;
            }
        }
        int lowLevel = 0;
        if (zeroCount < dataLine.cols / 4) {
            lowLevel = 255;
        }

        // Reading the data line
        unsigned long number = 0;
        int n = 0;
        for (int col = 1; col < dataLine.cols - 1; col += 2) {
            if (dataLine.at<unsigned char>(0, col) == lowLevel) {
                number |= 1 << n;
            }
            n++;
        }

        return number;
    }

    Pose HPCodePatternDetector::get2DPose(int id) {
        return codes.at(id);
    }

    Pose HPCodePatternDetector::get3DPose(int id) {
        throw Exception("The 3D pose of HPCode is not implemented yet.");
        return codes.at(id);
    }

    bool HPCodePatternDetector::patternFound(int id) {
        return (codes.find(id) != codes.end());
    }

    std::vector<Pose> HPCodePatternDetector::getAll3DPoses(int id) {
        throw Exception("The 3D pose of HPCode is not implemented yet.");
        std::vector<Pose> vector;
        vector.push_back(codes.at(id));
        return vector;
    }

    void HPCodePatternDetector::draw(cv::Mat & image) {
        for (std::map<int, Pose>::iterator it = codes.begin(); it != codes.end(); it++) {
            double length = 2 * patternPhase.getPixelPeriod() * this->numberHalfPeriods / 4;
            it->second.draw(image, length, to_string(it->first));
        }
    }

    //    void HPCodePatternDetector::drawSnapshots(cv::Mat & image) {
    //        for (std::map<int, Pose>::iterator it = codes.begin(); it != codes.end(); it++) {
    //            int x = (int) (it->second.x);
    //            int y = (int) (it->second.y);
    //            cv::rectangle(image, cv::Rect(x - snapshotSize / 2, y - snapshotSize / 2, snapshotSize, snapshotSize), cv::Scalar(255, 0, 0));
    //        }
    //    }

    void HPCodePatternDetector::showControlImages() {
        cv::imshow("Canny image", detector.fiducialDetector.cannyImage);
        cv::imshow("Found peaks (red = dir 1, green = dir 2)", patternPhase.getPeaksImage());
        arrayShow("Snapshot image", snapshot);
    }

}