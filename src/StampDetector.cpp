/* 
 * This file is part of the VERNIER Library.
 *
 * Copyright (c) 2018-2025 CNRS, ENSMM, UMLP.
 */

#include "StampDetector.hpp"

namespace Vernier {
#ifdef USE_OPENCV

    StampDetector::StampDetector(double physicalPeriod, int snapshotSize, int numberHalfPeriods) {
        resize(physicalPeriod, snapshotSize, numberHalfPeriods);
    }

    void StampDetector::resize(double physicalPeriod, int snapshotSize, int numberHalfPeriods) {
        ASSERT(snapshotSize % 2 == 0);
        ASSERT(physicalPeriod > 0.0);
        ASSERT((numberHalfPeriods > 0) && (numberHalfPeriods % 4 == 1));
        this->snapshotSize = snapshotSize;
        this->physicalPeriod = physicalPeriod;
        this->numberHalfPeriods = numberHalfPeriods;
        snapshot.resize(snapshotSize, snapshotSize);
        patternPhase.resize(snapshotSize, snapshotSize);
    }

    void StampDetector::takeSnapshot(int x, int y, cv::Mat image) {
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

    void StampDetector::compute(const cv::Mat& image) {



        detector.compute(image);

        stamps.clear();
        for (int i = 0; i < detector.squares.size(); i++) {

            Square square = detector.squares[i];

            if ((int) square.getRadius() > snapshotSize) {
                throw Exception("The QRCode is too large for pose estimation: increase the snapshot size.");
            }
            if ((int) square.getRadius() < numberHalfPeriods) {
                throw Exception("The QRCode is too tiny for pose estimation: increase the picture quality size.");
            }

            int centerX = (int) square.getCenter().x;
            int centerY = (int) square.getCenter().y;
            takeSnapshot(centerX, centerY, image);
            patternPhase.compute(snapshot);

            double alpha;
            double dx, dy;
            double diffAngle = angleInPiPi(patternPhase.getPlane1().getAngle() - square.getAngle());
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

            stamps.push_back(pose);
        }
    }

    void StampDetector::draw(cv::Mat & image) {
        detector.draw(image);

        for (int i = 0; i < stamps.size(); i++) {
            stamps[i].draw(image);
        }
    }

    void StampDetector::showControlImages() {
        //cv::imshow("Phase fringes (red = dir 1, green = dir 2)", patternPhase.getFringesImage()); // erreur spatial est vide ???
        //cv::moveWindow("Phase fringes (red = dir 1, green = dir 2)", 0, 0);
        cv::imshow("Found peaks (red = dir 1, green = dir 2)", patternPhase.getPeaksImage());
        //cv::moveWindow("Found peaks (red = dir 1, green = dir 2)", 0, patternPhase.getNRows());
    }

#endif //USE_OPENCV
}